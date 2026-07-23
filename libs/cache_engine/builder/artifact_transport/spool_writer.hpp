#pragma once
// spool_writer.hpp -- G3 / #46b Lagerhaltung, Scheibe B6 (Ledger §62-B-NACHTRAG-2 + PLATTFORM-AUFLAGE).
//
// Die WRITER-CT-STRATEGY: SpoolWriter<Backend> haelt einen RAM-Spool + einen dedizierten Writer-
// Thread (der ZWEITE Thread ausserhalb des Compile-Pool-Budgets). Der Compile-Pool ruft submit() und
// bleibt compile-rein; bei Trigger (Groesse ODER Dutzend) uebergibt der Spool einen Batch an den
// Writer-Thread, der Backend::write je Eintrag ausfuehrt. Das Backend wird zur COMPILE-ZEIT gewaehlt
// (COMDARE_WRITER_BACKEND_* Define aus CMake, Default portable) -- KEIN Runtime-Switch (benannte
// Strategy). Das portable Backend ist die Korrektheits-Referenz aller Backends (B21).
//
// RESUME-SEMANTIK: flush() ist ein SYNCHRONER Drain-Barrier -> der Aufrufer schreibt die .version-
// Sidecar-Marke erst NACH flush (Verlust = max. das RAM-Fenster, Sidecar-Resume baut nach).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib (thread-sync). Kein Runtime-
// Switch, keine vtable im Hot-Path (der Compile-Pool ruft nur submit(); der IO ist die Writer-Ebene).

#include "artifact_transport/ram_spool.hpp"

// CT-Wahl des Backends: genau EIN COMDARE_WRITER_BACKEND_* Define. Nur der gewaehlte Header wird
// inkludiert -> keine Abhaengigkeit auf noch nicht vorhandene Backend-Header (io_uring folgt in B7).
#if defined(COMDARE_WRITER_BACKEND_IO_URING)
#include "artifact_transport/writer_backend_io_uring.hpp"
#elif defined(COMDARE_WRITER_BACKEND_WINRING)
#include "artifact_transport/writer_backend_winring.hpp"
#else
#include "artifact_transport/writer_backend_portable.hpp"
#endif

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::artifact_transport {

// CT-Strategy-Vertrag: ein Writer-Backend persistiert einen SpoolEntry und nennt sich.
template <typename Bk>
concept SpoolWriterBackend = requires(SpoolEntry const& e) {
    { Bk::write(e) } -> std::convertible_to<bool>;
    { Bk::name() } -> std::convertible_to<std::string_view>;
};

// Das zur Compile-Zeit gewaehlte aktive Backend (Default portable).
#if defined(COMDARE_WRITER_BACKEND_IO_URING)
using ActiveWriterBackend = IoUringWriterBackend;
#elif defined(COMDARE_WRITER_BACKEND_WINRING)
using ActiveWriterBackend = WinRingWriterBackend;
#else
using ActiveWriterBackend = PortableWriterBackend;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// SpoolWriter<Backend> -- RAM-Spool + dedizierter Writer-Thread. Nicht kopier-/verschiebbar.
// ─────────────────────────────────────────────────────────────────────────────
template <SpoolWriterBackend Backend>
class SpoolWriter {
public:
    explicit SpoolWriter(std::size_t max_bytes = kMaxSpoolBytes, std::size_t count_trigger = kSpoolCountTrigger)
        : spool_{max_bytes, count_trigger} {
        worker_ = std::thread([this] { run(); });
    }

    SpoolWriter(SpoolWriter const&)            = delete;
    SpoolWriter& operator=(SpoolWriter const&) = delete;

    ~SpoolWriter() { close(); }

    // Compile-Pool: eine fertige Binary in den RAM-Spool legen. Bei Trigger wird der Spool als Batch
    // an den Writer-Thread uebergeben (ueberlappt mit dem weiterlaufenden Bau).
    void submit(std::filesystem::path dest, std::string bytes) {
        std::vector<SpoolEntry> batch;
        {
            std::lock_guard<std::mutex> lk(spool_mtx_);
            spool_.add(SpoolEntry{std::move(dest), std::move(bytes)});
            if (spool_.should_flush()) batch = spool_.drain();
        }
        if (!batch.empty()) enqueue_batch(std::move(batch));
    }

    // Synchroner Drain-Barrier: haengt den Rest-Spool an und wartet, bis ALLE Batches persistiert
    // sind. Danach ist der Store konsistent (Sidecar-Marke darf jetzt gesetzt werden).
    void flush() {
        std::vector<SpoolEntry> batch;
        {
            std::lock_guard<std::mutex> lk(spool_mtx_);
            if (!spool_.empty()) batch = spool_.drain();
        }
        if (!batch.empty()) enqueue_batch(std::move(batch));
        std::unique_lock<std::mutex> lk(q_mtx_);
        drained_cv_.wait(lk, [this] { return enqueued_ == written_; });
    }

    // flush + Writer-Thread joinen. Idempotent.
    void close() {
        flush();
        {
            std::lock_guard<std::mutex> lk(q_mtx_);
            if (closed_) return;
            closed_ = true;
        }
        q_cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    // Zahl bislang PERSISTIERTER Eintraege (Diagnose/Test).
    [[nodiscard]] std::size_t persisted_count() const {
        std::lock_guard<std::mutex> lk(q_mtx_);
        return persisted_entries_;
    }

    // Zahl der noch im RAM haengenden, noch nicht als Batch uebergebenen Eintraege = das Crash-Fenster
    // (ein Absturz vor dem Trigger/flush verliert genau diese).
    [[nodiscard]] std::size_t pending_count() const {
        std::lock_guard<std::mutex> lk(spool_mtx_);
        return spool_.count();
    }

    [[nodiscard]] static constexpr std::string_view backend_name() noexcept { return Backend::name(); }

private:
    void enqueue_batch(std::vector<SpoolEntry> batch) {
        {
            std::lock_guard<std::mutex> lk(q_mtx_);
            queue_.push_back(std::move(batch));
            ++enqueued_;
        }
        q_cv_.notify_one();
    }

    void run() {
        for (;;) {
            std::vector<SpoolEntry> batch;
            {
                std::unique_lock<std::mutex> lk(q_mtx_);
                q_cv_.wait(lk, [this] { return closed_ || !queue_.empty(); });
                if (queue_.empty()) return; // Praedikat garantiert closed_ -> fertig
                batch = std::move(queue_.front());
                queue_.pop_front();
            }
            for (auto const& e : batch) {
                bool const ok = Backend::write(e);
                if (ok) {
                    std::lock_guard<std::mutex> lk(q_mtx_);
                    ++persisted_entries_;
                }
            }
            {
                std::lock_guard<std::mutex> lk(q_mtx_);
                ++written_;
            }
            drained_cv_.notify_all();
        }
    }

    RamSpool           spool_;
    mutable std::mutex spool_mtx_; // schuetzt spool_
    std::thread        worker_;

    mutable std::mutex                  q_mtx_; // schuetzt queue_ + Zaehler + closed_
    std::condition_variable             q_cv_;
    std::condition_variable             drained_cv_;
    std::deque<std::vector<SpoolEntry>> queue_;
    std::size_t                         enqueued_          = 0;
    std::size_t                         written_           = 0;
    std::size_t                         persisted_entries_ = 0;
    bool                                closed_            = false;
};

// Konvenienz-Alias auf den zur Compile-Zeit gewaehlten Writer.
using ActiveSpoolWriter = SpoolWriter<ActiveWriterBackend>;

static_assert(SpoolWriterBackend<ActiveWriterBackend>, "Writer-Backend muss den CT-Strategy-Vertrag erfuellen");

} // namespace comdare::cache_engine::builder::artifact_transport
