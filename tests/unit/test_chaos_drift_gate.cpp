// test_chaos_drift_gate.cpp — #197 chaos: Mess-Drift-Detektor + Rerun-Gate (Pipeline P0, EPIC #186)
//
// ZWECK (hartes CI-Gate, Stage `chaos`, Job `chaos:drift` in der cache-engine-.gitlab-ci.yml):
//   Beweise DETERMINISTISCH, dass der Drift-MECHANISMUS korrekt ist:
//     (1) assess_drift klassifiziert stabile (< Schwelle) vs. instabile (> Schwelle) Wiederholungs-
//         Gruppen über die Median-relative Spannweite richtig und stürzt an keinem Rand ab
//         (0/1 Sample, Median 0 → kein Division-durch-0).
//     (2) run_with_drift_gate heilt sich selbst: instabile Gruppe → Rerun(s) + Warn-Log; wird die
//         Messung stabil, akzeptiert es sie; bleibt sie instabil, erschöpft es das Rerun-Budget und
//         gibt advisory die stabilste Gruppe + finale Warnung zurück (kein Abbruch/throw).
//
//   Der Mechanismus ist hier hart grün (deterministische Test-Sequenzen). Die ANWENDUNG auf reale
//   PMC-Messungen + die Schwellen-Kalibrierung sind #156-gegatet (mehrtägiger Voll-Lauf) und bewusst
//   NICHT Teil dieses Gates — analog zu test_config_durability (#195) als fokussierter Mechanismus-Test.
//
// Include-Konvention gespiegelt von test_config_durability (#195): Include-Root libs/cache_engine →
// <builder/commands/drift_detector.hpp> (zieht transitiv builder/commands/latency_stats.hpp). Header-
// only → keine LIBRARIES nötig.

#include <builder/commands/drift_detector.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace cmd = ::comdare::cache_engine::builder::commands;

namespace {

constexpr double kThreshold = 0.05; // 5 % — die Diplomarbeit-Schwelle (parametrisiert in der API)

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// (1) assess_drift — Klassifikation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosDriftGate, AssessStableSeriesBelowThreshold) {
    // {1000,1010,1020}: median 1010, Spannweite 20 → drift ≈ 1,98 % < 5 % → stabil.
    std::vector<std::int64_t> const s{1000, 1010, 1020};
    cmd::DriftVerdict const         v = cmd::assess_drift(s, kThreshold);
    EXPECT_EQ(v.samples, 3u);
    EXPECT_EQ(v.median_ns, 1010);
    EXPECT_EQ(v.min_ns, 1000);
    EXPECT_EQ(v.max_ns, 1020);
    EXPECT_LT(v.relative_drift, kThreshold);
    EXPECT_FALSE(v.unstable);
}

TEST(ChaosDriftGate, AssessUnstableSeriesAboveThreshold) {
    // {1000,1000,1100}: median 1000, Spannweite 100 → drift = 10 % > 5 % → instabil.
    std::vector<std::int64_t> const s{1000, 1000, 1100};
    cmd::DriftVerdict const         v = cmd::assess_drift(s, kThreshold);
    EXPECT_EQ(v.median_ns, 1000);
    EXPECT_DOUBLE_EQ(v.relative_drift, 0.1);
    EXPECT_TRUE(v.unstable);
}

TEST(ChaosDriftGate, AssessBoundaryNotStrictlyGreaterIsStable) {
    // Genau an der Schwelle: drift == threshold ist NICHT unstable (Vergleich ist strikt „>").
    // {1000,1050}: median (nearest-rank q=0.5, n=2, k=1) = 1050, Spannweite 50 → drift = 50/1050.
    // Wir setzen die Schwelle exakt auf diesen Wert und erwarten stable (drift > threshold ist false).
    std::vector<std::int64_t> const s{1000, 1050};
    cmd::DriftVerdict const         probe = cmd::assess_drift(s, kThreshold);
    cmd::DriftVerdict const         v     = cmd::assess_drift(s, probe.relative_drift);
    EXPECT_FALSE(v.unstable) << "drift == threshold darf nicht als instabil gelten (striktes >).";
}

TEST(ChaosDriftGate, EmptyAndSingleSampleAreStableNoCrash) {
    cmd::DriftVerdict const e = cmd::assess_drift(std::span<const std::int64_t>{}, kThreshold);
    EXPECT_EQ(e.samples, 0u);
    EXPECT_DOUBLE_EQ(e.relative_drift, 0.0);
    EXPECT_FALSE(e.unstable);

    std::vector<std::int64_t> const one{500};
    cmd::DriftVerdict const         v = cmd::assess_drift(one, kThreshold);
    EXPECT_EQ(v.samples, 1u);
    EXPECT_EQ(v.median_ns, 500);
    EXPECT_EQ(v.min_ns, 500);
    EXPECT_EQ(v.max_ns, 500);
    EXPECT_DOUBLE_EQ(v.relative_drift, 0.0);
    EXPECT_FALSE(v.unstable);
}

TEST(ChaosDriftGate, ZeroMedianGuardNoDivisionByZero) {
    // Degenerierte Null-Messung: median 0 → drift muss 0 bleiben (kein Division-durch-0/NaN/Crash).
    std::vector<std::int64_t> const z{0, 0, 0};
    cmd::DriftVerdict const         v = cmd::assess_drift(z, kThreshold);
    EXPECT_EQ(v.median_ns, 0);
    EXPECT_DOUBLE_EQ(v.relative_drift, 0.0);
    EXPECT_FALSE(v.unstable);
}

// ─────────────────────────────────────────────────────────────────────────────
// (2) run_with_drift_gate — Selbstheilung / Erschöpfung / kein Rerun
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosDriftGate, StableFirstTryNoRerunNoWarn) {
    // Stets stabile Proben (~2 % Drift) → akzeptiert beim 1. Versuch, 0 Reruns, kein Warn-Log.
    std::vector<std::int64_t> const seq{1000, 1010, 1020};
    std::size_t                     idx     = 0;
    auto                            measure = [&]() -> std::int64_t { return seq[idx++ % seq.size()]; };

    std::ostringstream         warn;
    cmd::DriftGateResult const r =
        cmd::run_with_drift_gate(measure, /*reps=*/3, kThreshold, /*max_reruns=*/3, &warn, "stable");

    EXPECT_TRUE(r.stable);
    EXPECT_FALSE(r.exhausted);
    EXPECT_EQ(r.attempts, 1u);
    EXPECT_EQ(r.reruns, 0u);
    EXPECT_EQ(r.samples.size(), 3u);
    EXPECT_TRUE(warn.str().empty()) << "Stabile Erstmessung darf keine Warnung loggen.";
}

TEST(ChaosDriftGate, HealsAfterInstabilityWithWarnLog) {
    // Versuch 1: {1000,1000,1200} → drift 20 % instabil; Versuch 2: {1000,1010,1020} → stabil.
    std::vector<std::int64_t> const seq{1000, 1000, 1200, /*Rerun:*/ 1000, 1010, 1020};
    std::size_t                     idx     = 0;
    auto                            measure = [&]() -> std::int64_t { return seq.at(idx++); };

    std::ostringstream         warn;
    cmd::DriftGateResult const r =
        cmd::run_with_drift_gate(measure, /*reps=*/3, kThreshold, /*max_reruns=*/3, &warn, "heal");

    EXPECT_TRUE(r.stable);
    EXPECT_FALSE(r.exhausted);
    EXPECT_EQ(r.attempts, 2u);
    EXPECT_EQ(r.reruns, 1u);
    EXPECT_EQ(idx, 6u) << "genau 2 Gruppen à 3 Proben verbraucht";
    EXPECT_NE(warn.str().find("Rerun 1/3"), std::string::npos)
        << "Rerun-Ereignis muss als Warnung geloggt sein. Log: " << warn.str();
    // Die akzeptierte (stabile) Gruppe ist die zweite.
    EXPECT_EQ(r.samples, (std::vector<std::int64_t>{1000, 1010, 1020}));
}

TEST(ChaosDriftGate, ExhaustsBudgetThenAdvisoryReturnsBestNoThrow) {
    // Dauerhaft instabil: jede Gruppe {1000,1000,1300} → drift 30 %. Nach max_reruns=3 → 4 Versuche,
    // dann advisory: exhausted, stabilste Gruppe zurück, finale Warnung, KEIN throw/Abbruch.
    std::size_t i       = 0;
    auto        measure = [&]() -> std::int64_t {
        std::int64_t const v = ((i % 3) == 2) ? 1300 : 1000; // pro 3er-Gruppe: 1000,1000,1300
        ++i;
        return v;
    };

    std::ostringstream   warn;
    cmd::DriftGateResult r;
    ASSERT_NO_THROW(r = cmd::run_with_drift_gate(measure, /*reps=*/3, kThreshold, /*max_reruns=*/3, &warn, "exhaust"));

    EXPECT_FALSE(r.stable);
    EXPECT_TRUE(r.exhausted);
    EXPECT_EQ(r.attempts, 4u); // 1 initial + 3 Reruns
    EXPECT_EQ(r.reruns, 3u);
    EXPECT_EQ(i, 12u); // 4 Gruppen à 3 Proben
    EXPECT_EQ(r.samples.size(), 3u);
    EXPECT_TRUE(r.verdict.unstable);
    EXPECT_NE(warn.str().find("unzuverlaessig"), std::string::npos)
        << "Finale advisory-Warnung muss geloggt sein. Log: " << warn.str();
}

TEST(ChaosDriftGate, RepsZeroIsNoMeasurementNotStable) {
    // reps==0 (Fehlkonfig): keine Messung → measure_one nie aufgerufen, nicht „stabil", 0 Versuche.
    std::size_t calls   = 0;
    auto        measure = [&]() -> std::int64_t {
        ++calls;
        return 1000;
    };
    cmd::DriftGateResult const r =
        cmd::run_with_drift_gate(measure, /*reps=*/0, kThreshold, /*max_reruns=*/3, nullptr, "zero");
    EXPECT_EQ(calls, 0u) << "Bei reps==0 darf nicht gemessen werden.";
    EXPECT_EQ(r.attempts, 0u);
    EXPECT_FALSE(r.stable);
    EXPECT_TRUE(r.samples.empty());
}

TEST(ChaosDriftGate, NullWarnStreamIsSilentAndSafe) {
    // warn=nullptr darf nicht crashen (kein Dereferenzieren eines Null-Streams).
    std::size_t i       = 0;
    auto        measure = [&]() -> std::int64_t {
        std::int64_t const v = ((i % 3) == 2) ? 1300 : 1000;
        ++i;
        return v;
    };
    cmd::DriftGateResult r;
    ASSERT_NO_THROW(r = cmd::run_with_drift_gate(measure, 3, kThreshold, 2, nullptr, "silent"));
    EXPECT_TRUE(r.exhausted);
    EXPECT_EQ(r.attempts, 3u); // 1 + 2 Reruns
}
