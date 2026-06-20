// #165-C (P-MD9, 2026-06-20) — Unit-Test fuer winsorized_mean_ns (latency_stats.hpp).
//
// Lehrbuch-Robust-Statistik "Winsorized Mean": behaelt ALLE n Samples, clampt aber die unteren/oberen Ausreisser
// auf die Perzentil-Grenzen [P(trim_q), P(1-trim_q)] und mittelt ueber alle n geklemmten Werte. Anders als das
// getrimmte Mittel (entfernt Extrema) bleibt die Stichprobengroesse erhalten → robuster zentraler Lagewert.
//
// Leichter Standalone-Test (KEIN gtest, KEIN Anatomie-/Boost-Header): nur latency_stats.hpp (std-only) + <cstdio>.
// `int main()` → 0 = alle Faelle gruen, 1 = mind. ein Fall rot (CTest wertet den Exit-Code). Identisches Muster wie
// die uebrigen leichten Standalone-Tests (test_v5_io_real_fixture).

#include "builder/commands/latency_stats.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <vector>

namespace stats = comdare::cache_engine::builder::commands::stats;

namespace {

int g_failures = 0;

// Float-Vergleich mit absoluter Toleranz (ns-Mittelwerte sind exakt rechenbar, eps grosszuegig).
void expect_near(char const* what, double got, double want, double eps = 1e-6) {
    if (std::fabs(got - want) > eps) {
        std::printf("[FAIL] %s: got=%.6f want=%.6f (eps=%.1e)\n", what, got, want, eps);
        ++g_failures;
    } else {
        std::printf("[ ok ] %s: %.6f\n", what, got);
    }
}

}  // namespace

int main() {
    std::printf("== test_winsorized_mean (#165-C, P-MD9) ==\n");

    // (1) Leere Eingabe → 0.0 (Rand).
    {
        std::vector<std::int64_t> empty;
        expect_near("empty -> 0", stats::winsorized_mean_ns(std::span<const std::int64_t>{empty}, 0.1), 0.0);
    }

    // (2) trim_q <= 0 → exakt das arithmetische Mittel (kein Clamping).
    {
        std::vector<std::int64_t> v{10, 20, 30, 40, 50};   // mean = 30
        expect_near("trim_q=0 == mean", stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.0), 30.0);
        expect_near("mean baseline",    stats::latency_mean_ns(std::span<const std::int64_t>{v}),         30.0);
    }

    // (3) KERN-AUSREISSER-ROBUSTHEIT: ein extremer Ausreisser zieht das arithmetische Mittel stark hoch, das
    //     winsorisierte Mittel clampt ihn auf die obere Perzentil-Grenze → deutlich naeher am Median.
    //     Datensatz: neun normale Werte 100..100 + EIN Spike 100000.
    {
        std::vector<std::int64_t> v{100, 100, 100, 100, 100, 100, 100, 100, 100, 100000};
        double const plain = stats::latency_mean_ns(std::span<const std::int64_t>{v});         // (9*100+100000)/10 = 10090
        expect_near("plain mean (spike)", plain, 10090.0);
        // trim_q=0.1: untere Grenze P(0.1), obere Grenze P(0.9). Nearest-Rank ueber n=10:
        //   k_lo = floor(0.1*10)=1 → 1.-kleinster (sortiert 100..100,100000) = 100.
        //   k_hi = floor(0.9*10)=9 → 9.-kleinster (Index 9, 0-basiert min(9, 9)) = 100000? -> nein:
        // Nearest-Rank P(0.9): k=min(n-1, floor(0.9*10))=min(9,9)=9 → groesster Wert = 100000. Damit clampt 0.1
        // NICHT (obere Grenze = der Spike selbst). Wir pruefen daher trim_q so, dass die obere Grenze < Spike liegt:
        // trim_q=0.2 → P(0.8): k=min(9, floor(0.8*10)=8)=8 → 9.-kleinster (0-basiert Index 8) = 100 (alle ausser Spike).
        // Untere Grenze P(0.2): k=floor(0.2*10)=2 → 100. → alle Spike-Werte werden auf 100 geclampt → Mittel = 100.
        double const wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.2);
        expect_near("winsorized mean (spike clamped to 100)", wins, 100.0);
        // Robustheits-Aussage: das winsorisierte Mittel liegt drastisch unter dem arithmetischen Mittel.
        if (!(wins < plain)) { std::printf("[FAIL] winsorized(%.3f) should be < plain mean(%.3f)\n", wins, plain); ++g_failures; }
        else                 { std::printf("[ ok ] winsorized < plain (%.3f < %.3f)\n", wins, plain); }
    }

    // (4) Symmetrischer Trim mit beidseitigen Ausreissern: 1 sehr klein + 1 sehr gross, Rest = 500.
    //     trim_q=0.2 clampt beide Extrema auf die jeweilige innere Grenze (= 500) → Mittel = 500.
    {
        std::vector<std::int64_t> v{1, 500, 500, 500, 500, 500, 500, 500, 500, 99999};
        double const wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.2);
        expect_near("winsorized two-sided -> 500", wins, 500.0);
    }

    // (5) trim_q >= 0.5 wird sicher geklemmt (kein Kollaps/keine NaN); Ergebnis endlich + plausibel (= Median-nah).
    {
        std::vector<std::int64_t> v{10, 20, 30, 40, 1000};
        double const wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.9);
        if (!std::isfinite(wins)) { std::printf("[FAIL] trim_q=0.9 produced non-finite %.3f\n", wins); ++g_failures; }
        else                      { std::printf("[ ok ] trim_q>=0.5 clamped, finite (%.3f)\n", wins); }
    }

    if (g_failures == 0) { std::printf("ALL PASS (test_winsorized_mean)\n"); return 0; }
    std::printf("FAILURES: %d (test_winsorized_mean)\n", g_failures);
    return 1;
}
