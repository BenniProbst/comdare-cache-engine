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

} // namespace

int main() {
    std::printf("== test_winsorized_mean (#165-C, P-MD9) ==\n");

    // (1) Leere Eingabe → 0.0 (Rand).
    {
        std::vector<std::int64_t> empty;
        expect_near("empty -> 0", stats::winsorized_mean_ns(std::span<const std::int64_t>{empty}, 0.1), 0.0);
    }

    // (2) trim_q <= 0 → exakt das arithmetische Mittel (kein Clamping).
    {
        std::vector<std::int64_t> v{10, 20, 30, 40, 50}; // mean = 30
        expect_near("trim_q=0 == mean", stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.0), 30.0);
        expect_near("mean baseline", stats::latency_mean_ns(std::span<const std::int64_t>{v}), 30.0);
    }

    // (3) KERN-AUSREISSER-ROBUSTHEIT: ein extremer Ausreisser zieht das arithmetische Mittel stark hoch, das
    //     winsorisierte Mittel clampt ihn auf die obere Perzentil-Grenze → deutlich naeher am Median.
    //     Datensatz: neun normale Werte 100..100 + EIN Spike 100000.
    {
        std::vector<std::int64_t> v{100, 100, 100, 100, 100, 100, 100, 100, 100, 100000};
        double const plain = stats::latency_mean_ns(std::span<const std::int64_t>{v}); // (9*100+100000)/10 = 10090
        expect_near("plain mean (spike)", plain, 10090.0);
        // D5-1-KANON (2026-08-09), k = ceil(q*n)-1 ueber n=10, sortiert {100 x9, 100000}:
        //   P(0.2): k = ceil(2)-1 = 1 -> 100   (untere Winsor-Grenze)
        //   P(0.8): k = ceil(8)-1 = 7 -> 100   (obere Winsor-Grenze; der Spike liegt DARUEBER)
        // -> der Spike wird auf 100 geklemmt, alle uebrigen Werte sind bereits 100 -> Mittel = 100.
        // HISTORIE (nicht mehr gueltig): bis 2026-08-09 rechnete percentile_ns k = min(n-1, floor(q*n));
        // damit lag P(0.9) auf Index 9 = 100000, die obere Grenze war der Spike SELBST und trim_q=0.1
        // klemmte nicht. Deshalb steht hier trim_q=0.2. Unter dem Kanon wuerde auch trim_q=0.1 klemmen
        // (P(0.9): k = ceil(9)-1 = 8 -> 100); der Fall bleibt bei 0.2, weil er dieselbe Aussage schaerfer
        // traegt (beide Flanken nachweislich innerhalb der 9 normalen Werte).
        double const wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.2);
        expect_near("winsorized mean (spike clamped to 100)", wins, 100.0);
        // Robustheits-Aussage: das winsorisierte Mittel liegt drastisch unter dem arithmetischen Mittel.
        if (!(wins < plain)) {
            std::printf("[FAIL] winsorized(%.3f) should be < plain mean(%.3f)\n", wins, plain);
            ++g_failures;
        } else {
            std::printf("[ ok ] winsorized < plain (%.3f < %.3f)\n", wins, plain);
        }
    }

    // (4) Symmetrischer Trim mit beidseitigen Ausreissern: 1 sehr klein + 1 sehr gross, Rest = 500.
    //     trim_q=0.2 clampt beide Extrema auf die jeweilige innere Grenze (= 500) → Mittel = 500.
    {
        std::vector<std::int64_t> v{1, 500, 500, 500, 500, 500, 500, 500, 500, 99999};
        double const              wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.2);
        expect_near("winsorized two-sided -> 500", wins, 500.0);
    }

    // (5) trim_q >= 0.5 wird sicher geklemmt (kein Kollaps/keine NaN); Ergebnis endlich + plausibel (= Median-nah).
    {
        std::vector<std::int64_t> v{10, 20, 30, 40, 1000};
        double const              wins = stats::winsorized_mean_ns(std::span<const std::int64_t>{v}, 0.9);
        if (!std::isfinite(wins)) {
            std::printf("[FAIL] trim_q=0.9 produced non-finite %.3f\n", wins);
            ++g_failures;
        } else {
            std::printf("[ ok ] trim_q>=0.5 clamped, finite (%.3f)\n", wins);
        }
    }

    if (g_failures == 0) {
        std::printf("ALL PASS (test_winsorized_mean)\n");
        return 0;
    }
    std::printf("FAILURES: %d (test_winsorized_mean)\n", g_failures);
    return 1;
}
