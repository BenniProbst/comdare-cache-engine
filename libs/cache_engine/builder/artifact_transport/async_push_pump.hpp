#pragma once
// async_push_pump.hpp -- W11 (Ledger §43.c, 2026-07-19): der BAU-MODUS asynchrone Push-Pump.
//
// PROBLEM (Task #28): im provision_only-Bau feuerte cache_push erst als BATCH NACH provision_all -> die Push-Zeit
// (mc cp je perm.dll, 1-2.5h je 32768er-Zelle) ueberlappte NICHT mit dem Bau -> Wall-Clock = Bau + Push. Zusaetzlich
// fehlte bei Job-Abbruch jeder Cluster-Teilstand (nur ein Whole-Chunk-Marker am Ende).
//
// LOESUNG: EIN dedizierter Push-Thread + Queue. Die Build-Worker bleiben COMPILE-REIN (max. Compile-Durchsatz) und
// reichen jede fertige perm.dll ueber den Completion-Hook (BuildOrchestrator::set_on_binary_done) in die Queue; der
// Push-Thread SERIALISIERT die mc-Aufrufe (netz-schonend, kein mc-Sturm) und UEBERLAPPT sie mit dem weiterlaufenden
// Bau -> Wall-Clock ~ max(Bau, Push). Reihenfolge egal (Objekt-Store-Keys sind per-stem eindeutig) -> Completion-
// Reihenfolge ist zulaessig; die index-geordnete progress_sink-/Determinismus-Naht (W6/§38) bleibt UNBERUEHRT.
//
// DOKTRIN: NUR im BAU-Modus (provision_only). Der MESS-Modus bleibt STRIKT synchron (async = I/O-Contention =
// Messfehler, artifact_cache.hpp-Doktrin) -- der Aufrufer erzeugt den Pump ausschliesslich im provision_only-Zweig.
//
// TEIL-MARKER: nach je part_size gepushten DLLs feuert der Pump einen Teil-Marker (PartialMarkerFn) -> ein Runner,
// der einen abgebrochenen Chunk wieder aufnimmt, pullt die bereits gepushten DLLs (Cluster-Resume). part_size==0 =
// keine Teil-Marker. Header-only, C++23, ASCII. Nur stdlib (thread/mutex/condition_variable/deque) -- KEINE neuen Deps.

#include "artifact_cache.hpp" // CachePushFn / PartialMarkerFn (Injektions-Naht-Typen)

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace comdare::cache_engine::builder::artifact_transport {

/// EIN Push-Thread, der bin_dir-Eintraege aus einer Queue serialisiert in den Objekt-Store schiebt (push_) und nach
/// je part_size Pushes einen Teil-Marker (partial_marker_) feuert. Nicht kopier-/verschiebbar (haelt Thread + Sync).
class AsyncPushPump {
public:
    AsyncPushPump(CachePushFn push, std::string build_version, PartialMarkerFn partial_marker = {},
                  std::size_t part_size = 0)
        : push_{std::move(push)}, build_version_{std::move(build_version)}, partial_marker_{std::move(partial_marker)},
          part_size_{part_size} {
        worker_ = std::thread([this] { run(); });
    }

    AsyncPushPump(AsyncPushPump const&)            = delete;
    AsyncPushPump& operator=(AsyncPushPump const&) = delete;
    AsyncPushPump(AsyncPushPump&&)                 = delete;
    AsyncPushPump& operator=(AsyncPushPump&&)      = delete;

    ~AsyncPushPump() { close(); }

    /// Thread-safe: aus mehreren Build-Workern gleichzeitig aufgerufen (Completion-Reihenfolge). Nimmt bin_dir in die
    /// Queue -> der Push-Thread schiebt es ueberlappend mit dem Bau. Nach close() eingereihte Eintraege verfallen.
    void enqueue(std::filesystem::path bin_dir) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (closed_) return; // nach dem Drain nichts mehr annehmen
            queue_.push_back(std::move(bin_dir));
        }
        cv_.notify_one();
    }

    /// Drain (alle eingereihten Pushes abarbeiten) + join. Idempotent. NACH provision_all aufrufen, BEVOR der Whole-
    /// Chunk-Marker gepusht wird (Marker = Vollstaendigkeits-Garantie bleibt: er erscheint erst nach vollem Drain).
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (closed_) return;
            closed_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    /// Zahl bislang erfolgreich abgearbeiteter Pushes (Test-/Diagnose-Sicht). Thread-safe.
    [[nodiscard]] std::size_t pushed_count() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return pushed_;
    }

private:
    void run() {
        for (;;) {
            std::filesystem::path bin_dir;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this] { return closed_ || !queue_.empty(); });
                if (queue_.empty()) return; // closed_ && leer => fertig (Drain vollstaendig)
                bin_dir = std::move(queue_.front());
                queue_.pop_front();
            }
            // (1) Push OHNE Lock (mc-Shellout, Sekunden) -> Build-Worker + enqueue blockieren nicht. Ein throwender
            //     push_ darf den Thread NIE terminieren (Bau laeuft weiter, artifact_cache loggt selbst) -> catch(...).
            try {
                if (push_) push_(bin_dir, build_version_);
            } catch (...) {
                // Bewusst geschluckt: der Transport-Client loggt ArtefaktIo selbst; der Bau darf nie an einem
                // Push-Fehler sterben (Fehler-Sichtbarkeits-Doktrin: lokale Kopie bleibt, weiter).
            }
            std::size_t part_to_mark = 0;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                ++pushed_;
                if (part_size_ != 0 && pushed_ % part_size_ == 0) part_to_mark = pushed_ / part_size_;
            }
            // (2) Teil-Marker (falls faellig) ebenfalls ohne Lock; auch hier nie den Thread terminieren.
            if (part_to_mark != 0 && partial_marker_) {
                try {
                    partial_marker_(build_version_, part_to_mark);
                } catch (...) {}
            }
        }
    }

    CachePushFn     push_;
    std::string     build_version_;
    PartialMarkerFn partial_marker_;
    std::size_t     part_size_ = 0;

    mutable std::mutex                mtx_;
    std::condition_variable           cv_;
    std::deque<std::filesystem::path> queue_;
    std::size_t                       pushed_ = 0;
    bool                              closed_ = false;
    std::thread                       worker_;
};

} // namespace comdare::cache_engine::builder::artifact_transport
