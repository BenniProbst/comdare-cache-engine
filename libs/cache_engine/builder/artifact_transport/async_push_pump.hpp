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
#include <vector>

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

    /// T2-A/F4-NB2 (Codex-Voll-Scope, Befund 1) -- DIE ZWISCHEN-BARRIERE: warten, bis alles bislang
    /// Eingereihte abgearbeitet ist, OHNE den Pump zu schliessen. Danach ist failed_count() eine
    /// vollstaendige Aussage ueber genau diese Menge, und der Bau laeuft mit demselben Pump weiter.
    ///
    /// WOFUER: der Plan-Zaehler des Bau-Laufs darf ein Fenster erst fortschreiben, wenn dessen Artefakte
    /// den Store WIRKLICH erreicht haben. close() taugt dafuer nicht -- es beendet den Push-Thread, und
    /// der naechste Slice haette keinen mehr. Ohne diese Barriere blieb nur die Wahl zwischen "Zaehler
    /// vor dem Vollzug" (die Luege) und "Zaehler erst ganz am Ende" (Verlust des inkrementellen Resume).
    ///
    /// PREIS, ehrlich benannt: an der Fenster-Grenze wartet der Bau auf den Push-SCHWANZ dieses Fensters.
    /// Der Pump lief waehrend des ganzen Fensters mit, es bleibt also der Rest weniger Pushes -- die
    /// W11-Zusage "Wall-Clock ~ max(Bau, Push)" gilt ab jetzt je Fenster statt ueber den ganzen Lauf.
    /// Ohne Plan-Ablage wird die Barriere nie gerufen (der Aufrufer gated sie), der Ist-Pfad ist unberuehrt.
    ///
    /// Nach close() kehrt sie sofort zurueck: der Drain ist dann bereits vollzogen. KONTRAKT: sie laeuft
    /// aus DEMSELBEN Thread wie close() (dem Bau-Treiber), nie nebenlaeufig dazu -- eine parallele
    /// Schliessung waehrend eines Wartens wuerde die Barriere zwar nicht haengen lassen (der schliessende
    /// Thread drainiert die Queue selbst), aber die Aussage von failed_count() danach unklar machen.
    void drain() {
        std::unique_lock<std::mutex> lk(mtx_);
        if (closed_) return;
        drain_cv_.wait(lk, [this] { return queue_.empty() && !in_flight_; });
    }

    /// Zahl der bislang GEWORFENEN Pushes. Dieselbe Menge, die failed_dirs() als Pfade auflistet -- nur
    /// ohne die Pfad-Kopie, weil der Zaehler-Pfad nur die Frage "war einer dabei" beantworten muss.
    [[nodiscard]] std::size_t failed_count() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return failed_.size();
    }

    /// Zahl bislang erfolgreich abgearbeiteter Pushes (Test-/Diagnose-Sicht). Thread-safe.
    /// TP1-N2: zaehlt seit der B-1-Nachbesserung NUR nicht-werfende Pushes -- ein geworfener Push ist
    /// kein "erfolgreich abgearbeitet" (der Kommentar versprach das schon immer; jetzt stimmt er).
    [[nodiscard]] std::size_t pushed_count() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return pushed_;
    }

    /// TP1-N2 (B-1): die bin_dirs, deren Push GEWORFEN hat -- die einzige Fehler-Sicht, die die
    /// void-CachePushFn hergibt. Stabil NACH close(); der Registrierungs-Pfad des Iterators schliesst
    /// genau diese Verzeichnisse aus dem Bestandslog aus (ein Eintrag ohne Store-Objekt waere unter
    /// dem Bau-Filter ein stiller Verlustpfad: der Folgelauf skippte eine nirgends existierende
    /// Binary). GRENZE, ehrlich benannt: ein Push, der intern scheitert OHNE zu werfen, bleibt hier
    /// unsichtbar -- solange CachePushFn void ist, ist die Exception das einzige Signal (Befund an
    /// die Transport-Schicht, nicht hier heilbar).
    [[nodiscard]] std::vector<std::filesystem::path> failed_dirs() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return failed_;
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
                // T2-A/F4-NB2: der Eintrag ist aus der Queue, aber NICHT fertig. Ohne diese Marke saehe
                // drain() eine leere Queue und meldete "vollzogen", waehrend der Push noch laeuft.
                in_flight_ = true;
            }
            // (1) Push OHNE Lock (mc-Shellout, Sekunden) -> Build-Worker + enqueue blockieren nicht. Ein throwender
            //     push_ darf den Thread NIE terminieren (Bau laeuft weiter, artifact_cache loggt selbst) -> catch(...).
            //     TP1-N2 (B-1): gefangen heisst nicht mehr VERSCHLUCKT -- das Verzeichnis wird als failed
            //     registriert, damit der Bestandslog-Registrierungs-Pfad es ausschliessen kann (s. failed_dirs()).
            bool push_ok = true;
            try {
                if (push_) push_(bin_dir, build_version_);
            } catch (...) {
                push_ok = false; // der Transport-Client loggt ArtefaktIo selbst; der Bau laeuft weiter
            }
            std::size_t part_to_mark = 0;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (push_ok) {
                    ++pushed_;
                    if (part_size_ != 0 && pushed_ % part_size_ == 0) part_to_mark = pushed_ / part_size_;
                } else {
                    failed_.push_back(bin_dir);
                }
                // T2-A/F4-NB2: erst HIER ist der Eintrag vollzogen (Erfolg wie Fehlschlag sind gebucht)
                // -- ein Warter auf der Barriere darf ab jetzt weiter. Der Teil-Marker unten steht
                // bewusst DAHINTER: er ist eine Folge des Pushes, keine Bedingung fuer seine Buchung.
                in_flight_ = false;
            }
            drain_cv_.notify_all();
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

    mutable std::mutex                 mtx_;
    std::condition_variable            cv_;
    std::condition_variable            drain_cv_; // T2-A/F4-NB2: die Zwischen-Barriere, s. drain()
    std::deque<std::filesystem::path>  queue_;
    std::size_t                        pushed_ = 0;
    std::vector<std::filesystem::path> failed_; // TP1-N2 (B-1): geworfene Pushes, s. failed_dirs()
    bool                               closed_    = false;
    bool                               in_flight_ = false; // ein Eintrag ist gezogen, aber noch nicht gebucht
    std::thread                        worker_;
};

} // namespace comdare::cache_engine::builder::artifact_transport
