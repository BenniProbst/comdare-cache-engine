// Querschnitt M -- SystemAxis-Wurzel + CMD-1-b MeasurementVisitable.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/system_axis.hpp>
#include <topics/axis_command_base.hpp>

#include <axes/alloc/axis_06_allocator_std_malloc.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_insert_counter.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace alloc  = ::comdare::cache_engine::alloc;
namespace an     = ::comdare::cache_engine::anatomy;
namespace m      = ::comdare::cache_engine::measurement;
namespace tel    = ::comdare::cache_engine::telemetry_axis;
namespace topics = ::comdare::cache_engine::topics;

namespace {

struct CountingVisitor {
    int observable_count = 0;

    template <class Axis>
    constexpr void visit_observable() noexcept {
        ++observable_count;
    }
};

struct NoMeasurementVisitor {};

template <class Axis>
[[nodiscard]] m::SystemAxisSample collect(Axis const& axis, m::MeasurementCategory category) {
    m::SystemAxisSample sample{.category = category};
    axis.collect(sample);
    return sample;
}

[[nodiscard]] constexpr std::size_t count_regime(m::MeasurementRegime regime) {
    std::size_t count = 0;
    for (auto const category : m::kAllMeasurementCategories) {
        if (m::regime_of(category) == regime) ++count;
    }
    return count;
}

[[nodiscard]] constexpr bool is_pmc_category(m::MeasurementCategory category) {
    for (auto const pmc_category : m::kPmcCounterCategories) {
        if (pmc_category == category) return true;
    }
    return false;
}

static_assert(m::SystemAxisConcept<m::WallClockSystemAxis>);
static_assert(m::SystemAxisConcept<m::ObserverSnapshotSystemAxis>);
static_assert(m::SystemAxisConcept<m::PmcSystemAxis>);

static_assert(std::is_empty_v<m::SystemAxis<m::WallClockSystemAxis>>);
static_assert(std::is_empty_v<m::SystemAxis<m::ObserverSnapshotSystemAxis>>);
static_assert(std::is_empty_v<m::SystemAxis<m::PmcSystemAxis>>);

static_assert(!std::is_polymorphic_v<m::SystemAxis<m::WallClockSystemAxis>>);
static_assert(!std::is_polymorphic_v<m::SystemAxis<m::ObserverSnapshotSystemAxis>>);
static_assert(!std::is_polymorphic_v<m::SystemAxis<m::PmcSystemAxis>>);
static_assert(!std::is_polymorphic_v<m::WallClockSystemAxis>);
static_assert(!std::is_polymorphic_v<m::ObserverSnapshotSystemAxis>);
static_assert(!std::is_polymorphic_v<m::PmcSystemAxis>);

#ifdef COMDARE_CE_ENABLE_STATISTICS
// Nur mit Statistics ist StdMalloc ObservableAxis (statistics()/observer() sind #ifdef-gated) — die
// Negativ-Behauptung gilt sonst nicht (Review wf_f1604ba3: STATISTICS=OFF-Build braeche hier).
static_assert(topics::MeasurementVisitable<alloc::StdMalloc, CountingVisitor&>);
static_assert(!topics::MeasurementVisitable<alloc::StdMalloc, NoMeasurementVisitor&>);
#endif
static_assert(topics::MeasurementVisitable<tel::InsertCounter, NoMeasurementVisitor&>);

} // namespace

// FK-2-LAYOUT-PIN: der Groessen-/Ausrichtungs-Vertrag des host-seitigen Samples, VOR der Status-Hebung
// gemessen. Der Header traegt denselben Pin compile-time; hier steht er als LITERALE Laufzeit-Ausgabe,
// damit ein Drift auch im ctest-Protokoll sichtbar wird und nicht nur den Compiler bricht.
TEST(MSystemAxisWurzel, SampleLayoutIstGepinntUndGroessenNeutral) {
    EXPECT_EQ(sizeof(m::SystemAxisSample), 24u);
    EXPECT_EQ(alignof(m::SystemAxisSample), 8u);
    EXPECT_TRUE(std::is_standard_layout_v<m::SystemAxisSample>);
    EXPECT_TRUE(std::is_trivially_copyable_v<m::SystemAxisSample>);
}

TEST(MSystemAxisWurzel, RegimeOfCoversAllCategoriesAndPmcThesisSet) {
    EXPECT_EQ(m::kAllMeasurementCategories.size(), 16u);
    EXPECT_EQ(count_regime(m::MeasurementRegime::PmcCounter), m::kPmcCounterCategories.size());
    EXPECT_EQ(count_regime(m::MeasurementRegime::TimeObserver), m::kTimeObserverCategories.size());
    EXPECT_TRUE(m::system_axes_always_present());

    for (auto const category : m::kAllMeasurementCategories) {
        EXPECT_EQ(m::regime_of(category) == m::MeasurementRegime::PmcCounter, is_pmc_category(category));
    }
}

TEST(MSystemAxisWurzel, WallClockCollectsLatencyAndThroughputFromHostValues) {
    m::WallClockSystemAxis axis{2'000, 4};

    EXPECT_EQ(m::WallClockSystemAxis::regime(), m::MeasurementRegime::TimeObserver);
    EXPECT_TRUE(axis.available());

    auto const mean = collect(axis, m::MeasurementCategory::LATENCY_MEAN);
    EXPECT_TRUE(mean.valid());
    EXPECT_EQ(mean.value, 500u);

    // honest-0: der Mittelwert darf NICHT als Perzentil etikettiert werden (echte Perzentile = HdrHistogramm).
    for (auto const category : std::array{m::MeasurementCategory::LATENCY_P50, m::MeasurementCategory::LATENCY_P95,
                                          m::MeasurementCategory::LATENCY_P99, m::MeasurementCategory::LATENCY_P999}) {
        auto const sample = collect(axis, category);
        EXPECT_FALSE(sample.valid());
        EXPECT_EQ(sample.value, 0u);
    }

    auto const throughput = collect(axis, m::MeasurementCategory::THROUGHPUT);
    EXPECT_TRUE(throughput.valid());
    EXPECT_EQ(throughput.value, 2'000'000u);
}

TEST(MSystemAxisWurzel, ObserverSnapshotCollectsReadOnlyHostPodValues) {
    an::ComdareTierObserverSnapshot snapshot{};
    snapshot.axis_stats[5][2]  = 2048;   // memory_layout.field_bytes (Nutzbytes)
    snapshot.axis_stats[5][3]  = 64;     // memory_layout.cache_lines (beruehrte 64-B-Lines)
    snapshot.axis_stats[6][1]  = 4096;   // allocator.bytes_in_use (bewusst NICHT als FOOTPRINT etikettiert)
    snapshot.axis_stats[16][4] = 12;     // queuing_q1.peak_size (T16 seit Bau-INC-2c) (Software-Puffer, kein LFB)
    snapshot.tier_fill_level   = 123456; // should not be mutated or consumed by these categories

    m::ObserverSnapshotSystemAxis axis{snapshot};
    EXPECT_EQ(m::ObserverSnapshotSystemAxis::regime(), m::MeasurementRegime::TimeObserver);
    EXPECT_TRUE(axis.available());

    // CLU = Auslastung field_bytes/(cache_lines*64) in Prozent (Thesis 03:383) — NICHT der rohe Zaehler.
    auto const clu = collect(axis, m::MeasurementCategory::CLU);
    EXPECT_TRUE(clu.valid());
    EXPECT_EQ(clu.value, 50u); // 2048 Nutzbytes auf 64 Lines a 64 B = 50 %

    // honest-0: bytes_in_use ist nicht der Thesis-Kanon bytes_in_use_peak; peak_size ist Software-Queue,
    // kein Hardware-Line-Fill-Buffer — beide Kategorien bleiben invalid statt falsch etikettiert.
    auto const footprint = collect(axis, m::MeasurementCategory::MEMORY_FOOTPRINT);
    EXPECT_FALSE(footprint.valid());
    EXPECT_EQ(footprint.value, 0u);

    auto const fill = collect(axis, m::MeasurementCategory::FILL_BUFFER_OCCUPANCY);
    EXPECT_FALSE(fill.valid());
    EXPECT_EQ(fill.value, 0u);
    EXPECT_EQ(snapshot.tier_fill_level, 123456u);
}

TEST(MSystemAxisWurzel, PmcCollectsAvailableCountersAndDoesNotInventIpcCpi) {
    m::PmcCounters counters{};
    counters.available           = true;
    counters.cache_misses_l1     = 1111;
    counters.cache_misses_l2     = 222;
    counters.cache_misses_l3     = 33;
    counters.dtlb_misses         = 7;
    counters.branch_misses       = 17;
    counters.energy_micro_joules = 99000;

    m::PmcSystemAxis axis{counters};
    EXPECT_EQ(m::PmcSystemAxis::regime(), m::MeasurementRegime::PmcCounter);
    EXPECT_TRUE(axis.available());

    EXPECT_EQ(collect(axis, m::MeasurementCategory::CACHE_MISS_L1).value, 1111u);
    EXPECT_EQ(collect(axis, m::MeasurementCategory::CACHE_MISS_L2).value, 222u);
    EXPECT_EQ(collect(axis, m::MeasurementCategory::CACHE_MISS_L3).value, 33u);
    EXPECT_EQ(collect(axis, m::MeasurementCategory::DTLB_MISS).value, 7u);
    EXPECT_EQ(collect(axis, m::MeasurementCategory::BRANCH_MISS).value, 17u);
    EXPECT_EQ(collect(axis, m::MeasurementCategory::ENERGY_J).value, 99000u);

    auto const ipc_cpi = collect(axis, m::MeasurementCategory::IPC_CPI);
    EXPECT_FALSE(ipc_cpi.valid());
    EXPECT_EQ(ipc_cpi.value, 0u);
}

TEST(MSystemAxisWurzel, PmcHonestZeroWhenUnavailable) {
    m::NullPmcSource source;
    source.begin();
    m::PmcCounters const counters = source.end();

    m::PmcSystemAxis axis{counters};
    EXPECT_FALSE(axis.available());

    for (auto const category : m::kPmcCounterCategories) {
        auto const sample = collect(axis, category);
        EXPECT_FALSE(sample.valid());
        EXPECT_EQ(sample.value, 0u);
    }
}

// FK-2/K1 PFLICHT-NEGATIVTEST: eine default-konstruierte, nie collectete Sample darf NIE als gueltige
// Messung lesen. Genau hier saesse sonst die Blindstelle "Messung als Nullen" -- ein vergessener
// collect()-Aufruf laese als "gueltiger Wert 0". Der Test prueft BEIDE Konstruktionsformen, auch die
// Designated-Init-Form, die der Harness-Helfer oben benutzt.
TEST(MSystemAxisWurzel, DefaultKonstruierteSampleIstNiemalsValid) {
    m::SystemAxisSample const frisch{};
    EXPECT_FALSE(frisch.valid());
    EXPECT_NE(frisch.status, m::SampleStatus::Ok);
    EXPECT_EQ(frisch.status, m::SampleStatus::SourceUnavailable);
    EXPECT_EQ(frisch.value, 0u);

    m::SystemAxisSample const benannt{.category = m::MeasurementCategory::CLU};
    EXPECT_FALSE(benannt.valid());
    EXPECT_NE(benannt.status, m::SampleStatus::Ok);

    // Zweiter, vom Accessor UNABHAENGIGER Ableitungsweg (Lehre "gruene Tests zementieren alte
    // Ordnung"): das CSV-Zell-Token der Taxonomie. Eine nie erhobene Zelle rendert "n/a", nie eine Zahl.
    EXPECT_EQ(m::sample_status_token(frisch.status), "n/a");
    EXPECT_NE(m::sample_status_token(frisch.status), m::sample_status_token(m::SampleStatus::Ok));
}

// FK-2: die benannten Helfer setzen Wert UND Status gemeinsam -- kein Zustand "value gesetzt, Status
// vergessen". mark_ok ist der EINZIGE Weg zu valid().
TEST(MSystemAxisWurzel, BenannteHelferSetzenWertUndStatusGemeinsam) {
    m::SystemAxisSample sample{.category = m::MeasurementCategory::CLU};

    sample.mark_ok(42);
    EXPECT_TRUE(sample.valid());
    EXPECT_EQ(sample.status, m::SampleStatus::Ok);
    EXPECT_EQ(sample.value, 42u);

    sample.mark_not_applicable();
    EXPECT_FALSE(sample.valid());
    EXPECT_EQ(sample.status, m::SampleStatus::NotApplicable);
    EXPECT_EQ(sample.value, 0u); // der alte Wert darf nicht stehenbleiben

    sample.mark_ok(7);
    sample.mark_source_unavailable();
    EXPECT_FALSE(sample.valid());
    EXPECT_EQ(sample.status, m::SampleStatus::SourceUnavailable);
    EXPECT_EQ(sample.value, 0u);

    sample.mark_ok(7);
    sample.mark_failed();
    EXPECT_FALSE(sample.valid());
    EXPECT_EQ(sample.status, m::SampleStatus::Failed);
    EXPECT_EQ(sample.value, 0u);
    EXPECT_EQ(m::sample_status_token(sample.status), "failed"); // NIE eine stille Null
}

// FK-2/K6: die vier honest-0-Stellen tragen jetzt EINZELN ihre Klassifikation statt eines gemeinsamen
// "invalid". Das ist der eigentliche Zugewinn des Pakets: "die Quelle fehlt" und "die Kategorie ist
// hier sinnlos" sind ab jetzt unterscheidbar -- beide bleiben ehrliches n/a, keines wird zum Fehler.
TEST(MSystemAxisWurzel, HonestNullStellenTragenIhreEinzelklassifikation) {
    m::WallClockSystemAxis const wall{2'000, 4};
    for (auto const category : std::array{m::MeasurementCategory::LATENCY_P50, m::MeasurementCategory::LATENCY_P95,
                                          m::MeasurementCategory::LATENCY_P99, m::MeasurementCategory::LATENCY_P999}) {
        auto const sample = collect(wall, category);
        EXPECT_EQ(sample.status, m::SampleStatus::NotApplicable) << "Mittelwert ist kein Perzentil (Kategorie sinnlos)";
    }

    an::ComdareTierObserverSnapshot snapshot{};
    snapshot.axis_stats[5][2] = 2048;
    snapshot.axis_stats[5][3] = 64;
    m::ObserverSnapshotSystemAxis const observer{snapshot};

    // MEMORY_FOOTPRINT: die Kategorie ist sinnvoll, es fehlt die peak-SPALTE -> SourceUnavailable.
    EXPECT_EQ(collect(observer, m::MeasurementCategory::MEMORY_FOOTPRINT).status, m::SampleStatus::SourceUnavailable);
    // FILL_BUFFER_OCCUPANCY: Software-Puffer vs. Hardware-LFB, physisch unverwandt -> NotApplicable.
    EXPECT_EQ(collect(observer, m::MeasurementCategory::FILL_BUFFER_OCCUPANCY).status, m::SampleStatus::NotApplicable);

    m::PmcCounters counters{};
    counters.available = true;
    m::PmcSystemAxis const pmc{counters};
    // IPC_CPI: echte PMC-Kategorie dieser Achse, aber ohne instructions/cycles-Spalten -> SourceUnavailable.
    EXPECT_EQ(collect(pmc, m::MeasurementCategory::IPC_CPI).status, m::SampleStatus::SourceUnavailable);

    // PMC unprivilegiert/ohne Zugang: der ZUGANG fehlt, kein Urteil ueber die Kategorie.
    m::NullPmcSource source;
    source.begin();
    m::PmcCounters const   leer = source.end();
    m::PmcSystemAxis const gesperrt{leer};
    for (auto const category : m::kPmcCounterCategories) {
        EXPECT_EQ(collect(gesperrt, category).status, m::SampleStatus::SourceUnavailable);
    }
}

// FK-2: die alte Sammel-Bedingung der WallClock-Achse hat "unplausibler Rohwert" und "Quelle lieferte
// nichts" zu einem einzigen invalid gefaltet. Beides ist jetzt getrennt -- ein negativer Zeitwert ist
// ein FEHLER (Zelle "failed"), eine leere Erhebung nicht.
TEST(MSystemAxisWurzel, DegenerierteRohwerteTrennenFehlerVonFehlenderQuelle) {
    m::WallClockSystemAxis const negativ{-1, 4};
    EXPECT_EQ(collect(negativ, m::MeasurementCategory::LATENCY_MEAN).status, m::SampleStatus::Failed);
    EXPECT_EQ(collect(negativ, m::MeasurementCategory::THROUGHPUT).status, m::SampleStatus::Failed);

    m::WallClockSystemAxis const ohne_ops{2'000, 0};
    EXPECT_EQ(collect(ohne_ops, m::MeasurementCategory::LATENCY_MEAN).status, m::SampleStatus::SourceUnavailable);

    m::WallClockSystemAxis const ohne_zeit{0, 4};
    EXPECT_EQ(collect(ohne_zeit, m::MeasurementCategory::THROUGHPUT).status, m::SampleStatus::SourceUnavailable);

    an::ComdareTierObserverSnapshot leer_snapshot{};
    leer_snapshot.axis_stats[5][2] = 2048;
    leer_snapshot.axis_stats[5][3] = 0; // kein Nenner: die Quelle hat nichts geliefert
    m::ObserverSnapshotSystemAxis const observer{leer_snapshot};
    EXPECT_EQ(collect(observer, m::MeasurementCategory::CLU).status, m::SampleStatus::SourceUnavailable);
}

// Fixture-UNABHAENGIGER Ableitungsweg (Auflage aus der Risiko-Liste): statt handgepflegter Erwartungen
// je Kategorie laeuft dieser Test ueber die KATEGORIE-Aufzaehlung und prueft nur die Invarianten, die
// fuer jede Achse und jede Kategorie gelten muessen. Eine dritte, hier vergessene Ableitung faellt
// damit auf, ohne dass jemand den Test nachpflegt.
TEST(MSystemAxisWurzel, StatusInvarianteGiltUeberAlleKategorienUndAchsen) {
    an::ComdareTierObserverSnapshot snapshot{};
    snapshot.axis_stats[5][2] = 2048;
    snapshot.axis_stats[5][3] = 64;
    m::PmcCounters counters{};
    counters.available = true;

    m::WallClockSystemAxis const        wall{2'000, 4};
    m::ObserverSnapshotSystemAxis const observer{snapshot};
    m::PmcSystemAxis const              pmc{counters};

    auto pruefe = [](m::SystemAxisSample const& sample) {
        // (1) valid() ist NICHTS anderes als status==Ok -- kein zweiter, abweichender Gueltigkeitsbegriff.
        EXPECT_EQ(sample.valid(), sample.status == m::SampleStatus::Ok);
        // (2) Ein nicht gueltiger Zustand traegt NIE einen Restwert (sonst wanderte eine Zahl in eine n/a-Zelle).
        if (!sample.valid()) EXPECT_EQ(sample.value, 0u);
        // (3) Der Status liegt immer im deklarierten Wertebereich der Taxonomie (kein Cast-Loch).
        EXPECT_LT(static_cast<std::size_t>(sample.status), m::kSampleStatusCount);
        // (4) Das Zell-Token ist nie leer -- jede Zelle sagt etwas.
        EXPECT_FALSE(m::sample_status_token(sample.status).empty());
    };

    for (auto const category : m::kAllMeasurementCategories) {
        pruefe(collect(wall, category));
        pruefe(collect(observer, category));
        pruefe(collect(pmc, category));
    }
}

TEST(MSystemAxisWurzel, MeasurementVisitableConstrainsObservableVisitors) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    CountingVisitor visitor{};
    topics::axis_accept_measurement<alloc::StdMalloc>(visitor);
    EXPECT_EQ(visitor.observable_count, 1);
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus: StdMalloc ist nicht observable";
#endif

    NoMeasurementVisitor no_measurement{};
    topics::axis_accept_measurement<tel::InsertCounter>(no_measurement);
    SUCCEED();
}
