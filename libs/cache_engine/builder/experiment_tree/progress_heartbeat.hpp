#pragma once
// experiment_tree/progress_heartbeat.hpp -- S1 (Section 62-B Log-Flush, Befund CI-Smoke 12160 "6h stumm"): ein
// thread-sicheres, ZEIT-gatetes und GEFLUSHTES Fortschritts-Testat fuer die langen, sonst stillen Bau- und Mess-
// Schleifen (provision_core / run_lazy_static_then_dynamic). Jede fertiggestellte Einheit ruft tick(); tick() emittiert
// HOECHSTENS alle `interval` Sekunden (plus IMMER die erste Einheit = Sofort-Lebenszeichen) EINE geflushte Zeile nach
// `os` (Default std::cerr); done() emittiert genau EINE Abschluss-Zeile. Rein beobachtend: schreibt NUR auf den
// Progress-Stream (nie in die Mess-CSV/binary_id) -> golden-/CRC-NEUTRAL. Thread-sicher: atomarer Zaehler + CAS auf den
// Emit-Zeitstempel -> genau EIN Emitter je Intervall (auch im Debug-Mess-Pool). Header-only, nur std.

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// Ein geflushtes, zeit-gatetes Fortschritts-Testat je Bau-/Mess-Schleife. Format der Zeile:
///   "[heartbeat] <phase> <done>/<total> t+<sekunden-seit-start>s"  bzw.  "... fertig <total>/<total> t+..s".
class ProgressHeartbeat {
public:
    explicit ProgressHeartbeat(std::string_view phase, std::size_t total, std::ostream& os = std::cerr,
                               std::chrono::seconds interval = std::chrono::seconds{30})
        : phase_{phase}, total_{total}, os_{&os}, interval_s_{static_cast<std::int64_t>(interval.count())},
          start_s_{now_s()}, last_emit_s_{start_s_} {}

    /// Je fertiggestellter Einheit aufrufen. Emittiert zeit-gated (>= interval seit der letzten Zeile) ODER bei der
    /// ERSTEN Einheit (Sofort-Lebenszeichen). Thread-sicher; genau EIN Emitter je Intervall.
    void tick() {
        std::size_t const  n    = ++done_; // atomar: genau EINE Einheit erhaelt n == 1
        std::int64_t const now  = now_s();
        std::int64_t       last = last_emit_s_.load(std::memory_order_relaxed);
        if (n != 1 && now - last < interval_s_) return;               // zu frueh (und nicht die erste Einheit)
        if (!last_emit_s_.compare_exchange_strong(last, now)) return; // ein anderer Thread hat gerade emittiert
        emit_line("", n, now);
    }

    /// Abschluss-Zeile (immer genau einmal, geflusht).
    void done() {
        std::int64_t const now = now_s();
        last_emit_s_.store(now, std::memory_order_relaxed);
        emit_line("fertig ", total_, now);
    }

private:
    [[nodiscard]] static std::int64_t now_s() {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
    void emit_line(std::string_view tag, std::size_t n, std::int64_t now) {
        (*os_) << "[heartbeat] " << phase_ << " " << tag << n << "/" << total_ << " t+" << (now - start_s_) << "s\n"
               << std::flush;
    }

    std::string               phase_;
    std::size_t               total_;
    std::ostream*             os_;
    std::int64_t              interval_s_;
    std::int64_t              start_s_;
    std::atomic<std::size_t>  done_{0};
    std::atomic<std::int64_t> last_emit_s_;
};

} // namespace comdare::cache_engine::builder::experiment
