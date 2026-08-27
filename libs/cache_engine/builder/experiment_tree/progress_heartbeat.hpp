#pragma once
// experiment_tree/progress_heartbeat.hpp -- S1 (Section 62-B Log-Flush, Befund CI-Smoke 12160 "6h stumm"): ein
// thread-sicheres, ZEIT-gatetes und GEFLUSHTES Fortschritts-Testat fuer die langen, sonst stillen Bau- und Mess-
// Schleifen (provision_core / run_lazy_static_then_dynamic). Jede fertiggestellte Einheit ruft tick(); tick() emittiert
// HOECHSTENS alle `interval` Sekunden (plus IMMER die erste Einheit = Sofort-Lebenszeichen) EINE geflushte Zeile nach
// `os` (Default std::cerr); done() emittiert genau EINE Abschluss-Zeile. Rein beobachtend: schreibt NUR auf den
// Progress-Stream (nie in die Mess-CSV/binary_id) -> golden-/CRC-NEUTRAL. Thread-sicher: atomarer Zaehler + CAS auf den
// Emit-Zeitstempel -> genau EIN Emitter je Intervall (auch im Debug-Mess-Pool). Header-only, nur std.

#include <atomic>
#include <charconv> // E-04-P1: COMDARE_HEARTBEAT_EVERY vollstaendig-dezimal parsen (kein atoi-Teilfrass)
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib> // E-04-P1: std::getenv (Trace-Budget-Deckel des Voll-Bau-Profils)
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// E-04-P1 (Trace-Budget, Teil 1d): die ZAEHL-Kadenz des Heartbeats env-steuerbar. COMDARE_HEARTBEAT_EVERY
/// ersetzt `voreinstellung` (heute die Compile-Worker-Zahl K); "0" schaltet das Zaehl-Gate ab (nur noch das
/// Zeit-Gate). UNGESETZT oder nicht als VOLLSTAENDIGE Dezimalzahl lesbar => `voreinstellung` => byte-identisch
/// zum Vor-E-04-P1-Verhalten (der Default aendert sich HEUTE nicht).
/// WARUM: beim Voll-Bau (2^17 je Perm) ergibt K=24 rund 33k Heartbeat-Zeilen je Lane-Job und damit eine
/// Trace-Groesse nahe am GitLab-Default-Limit -- ein truncierter Trace killt den Live-Kanal ausgerechnet am
/// Job-Ende. Das Voll-Bau-Profil hebt die Kadenz spaeter (Vorschlag 256); die Infra-Seite (runner
/// output_limit) ist der zweite, davon unabhaengige Hebel.
[[nodiscard]] inline std::size_t heartbeat_every_n(std::size_t voreinstellung) noexcept {
    char const* const roh = std::getenv("COMDARE_HEARTBEAT_EVERY");
    if (roh == nullptr) return voreinstellung;
    std::string_view const s{roh};
    if (s.empty()) return voreinstellung;
    std::size_t v      = 0;
    auto const [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size()) return voreinstellung; // Teil-Parse zaehlt NICHT
    return v;
}

/// Ein geflushtes, zeit-gatetes Fortschritts-Testat je Bau-/Mess-Schleife. Format der Zeile:
///   "[heartbeat] <phase> <done>/<total> t+<sekunden-seit-start>s"  bzw.  "... fertig <total>/<total> t+..s".
class ProgressHeartbeat {
public:
    /// #27 (2026-07-23): `every_n` = zusaetzliches ZAEHL-Gate (0 = aus, byte-identisch zum Vor-#27-Verhalten). Ist es
    /// gesetzt, emittiert JEDE every_n-te Einheit ein Testat -- kombiniert mit dem Zeit-Gate (`interval`): was zuerst
    /// kommt. Der Bau-Loop setzt every_n = die Compile-Worker-Zahl K (lane_build_parallelism: amd 16/intel 24) -> der
    /// Job-Log zeigt "alle K Builds" den Slice-Fortschritt (User-Wunsch beim Zuschauen), auch wenn K Builds < 30s dauern.
    explicit ProgressHeartbeat(std::string_view phase, std::size_t total, std::ostream& os = std::cerr,
                               std::chrono::seconds interval = std::chrono::seconds{30}, std::size_t every_n = 0)
        : phase_{phase}, total_{total}, os_{&os}, interval_s_{static_cast<std::int64_t>(interval.count())},
          every_n_{every_n}, start_s_{now_s()}, last_emit_s_{start_s_} {}

    /// Je fertiggestellter Einheit aufrufen. Emittiert bei der ERSTEN Einheit (Sofort-Lebenszeichen), zeit-gated
    /// (>= interval seit der letzten Zeile) ODER zaehl-gated (jede every_n-te Einheit) -- was zuerst kommt. Thread-sicher.
    void tick() {
        std::size_t const  n   = ++done_; // atomar: genau EINE Einheit erhaelt jeden Wert von n
        std::int64_t const now = now_s();
        // #27: ZAEHL-Gate. n ist je Thread eindeutig -> jede every_n-te Einheit ist ein NATUERLICHER Ein-Emitter (kein
        // CAS noetig). Zeitstempel mitfuehren, damit direkt danach kein Doppel-Beat aus dem Zeit-Gate faellt.
        if (every_n_ != 0 && n % every_n_ == 0) {
            last_emit_s_.store(now, std::memory_order_relaxed);
            emit_line("", n, now);
            return;
        }
        std::int64_t last = last_emit_s_.load(std::memory_order_relaxed);
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
    std::size_t               every_n_; // #27: ZAEHL-Gate-Kadenz (0 = aus)
    std::int64_t              start_s_;
    std::atomic<std::size_t>  done_{0};
    std::atomic<std::int64_t> last_emit_s_;
};

} // namespace comdare::cache_engine::builder::experiment
