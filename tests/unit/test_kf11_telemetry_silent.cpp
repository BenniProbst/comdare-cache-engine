// test_kf11_telemetry_silent — KF-11 (2026-06-02)
// Belegt: Telemetrie ist Default-AN (Active emittiert); der SILENT-Modus misst per Snapshot-Diff, ohne die
// gemessene Region zu perturbieren (emit_count-Delta == 0), während die Metrik (value-Delta) korrekt erfasst wird.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf11_telemetry_silent.cpp

#include "builder/experiment_tree/telemetry_mode.hpp"

#include <iostream>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

// Mock-Telemetrie: value (echte Metrik) + emit_count (Emissions-/Perturbations-Zähler).
struct MockTelemetry {
    std::uint64_t value = 0;
    std::uint64_t emit_count = 0;
    ex::TelemetrySnapshot snapshot() const { return {value, emit_count}; }
    // 100 Operationen; im Active-Modus EMITTIERT je Op (Perturbation), im Silent-Modus NICHT.
    void run_region(ex::TelemetryMode mode) {
        for (int i = 0; i < 100; ++i) {
            ++value;                                       // echte Arbeit (immer)
            if (mode == ex::TelemetryMode::Active) ++emit_count;  // Emission NUR Active
        }
    }
};

int main() {
    std::cout << "KF-11: Telemetrie Default-AN + Silent-Diff-Modus:\n";

    // Default-AN
    ex::TelemetryConfig cfg{};
    check_true("Default-Modus == Active (Telemetrie AN)", cfg.mode == ex::TelemetryMode::Active);
    check_true("Default ist NICHT silent", !cfg.is_silent());

    // telemetry_diff reine Subtraktion
    auto d = ex::telemetry_diff({5, 2}, {105, 2});
    check_eq("telemetry_diff.value (105-5)", d.value, std::uint64_t{100});
    check_eq("telemetry_diff.emit_count (2-2)", d.emit_count, std::uint64_t{0});

    // ACTIVE-Region: emittiert je Op → Perturbation sichtbar (emit_count steigt).
    {
        MockTelemetry t;
        t.run_region(ex::TelemetryMode::Active);
        check_eq("Active: value == 100", t.value, std::uint64_t{100});
        check_eq("Active: emit_count == 100 (Telemetrie emittiert je Op)", t.emit_count, std::uint64_t{100});
    }

    // SILENT-DIFF: Metrik korrekt erfasst (value-Delta 100), ABER Region unverfälscht (emit_count-Delta 0).
    {
        MockTelemetry t;
        auto diff = ex::measure_silent_diff(
            [&] { return t.snapshot(); },
            [&](ex::TelemetryMode m) { t.run_region(m); });
        check_eq("Silent-Diff: value-Delta == 100 (Metrik erfasst)", diff.value, std::uint64_t{100});
        check_eq("Silent-Diff: emit_count-Delta == 0 (Region NICHT perturbiert)", diff.emit_count, std::uint64_t{0});
        check_eq("Silent: tatsächliche emit_count bleibt 0 (keine Emission in Mess-Region)", t.emit_count, std::uint64_t{0});
        check_eq("Silent: echte Arbeit lief trotzdem (value==100)", t.value, std::uint64_t{100});
    }

    std::cout << "\n==== KF-11 Telemetrie Default-AN + Silent-Diff: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
