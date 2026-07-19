// Querschnitt M -- SystemAxis-Wurzel + CMD-1-b MeasurementVisitable.

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
    EXPECT_TRUE(mean.valid);
    EXPECT_EQ(mean.value, 500u);

    // honest-0: der Mittelwert darf NICHT als Perzentil etikettiert werden (echte Perzentile = HdrHistogramm).
    for (auto const category : std::array{m::MeasurementCategory::LATENCY_P50, m::MeasurementCategory::LATENCY_P95,
                                          m::MeasurementCategory::LATENCY_P99, m::MeasurementCategory::LATENCY_P999}) {
        auto const sample = collect(axis, category);
        EXPECT_FALSE(sample.valid);
        EXPECT_EQ(sample.value, 0u);
    }

    auto const throughput = collect(axis, m::MeasurementCategory::THROUGHPUT);
    EXPECT_TRUE(throughput.valid);
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
    EXPECT_TRUE(clu.valid);
    EXPECT_EQ(clu.value, 50u); // 2048 Nutzbytes auf 64 Lines a 64 B = 50 %

    // honest-0: bytes_in_use ist nicht der Thesis-Kanon bytes_in_use_peak; peak_size ist Software-Queue,
    // kein Hardware-Line-Fill-Buffer — beide Kategorien bleiben invalid statt falsch etikettiert.
    auto const footprint = collect(axis, m::MeasurementCategory::MEMORY_FOOTPRINT);
    EXPECT_FALSE(footprint.valid);
    EXPECT_EQ(footprint.value, 0u);

    auto const fill = collect(axis, m::MeasurementCategory::FILL_BUFFER_OCCUPANCY);
    EXPECT_FALSE(fill.valid);
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
    EXPECT_FALSE(ipc_cpi.valid);
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
        EXPECT_FALSE(sample.valid);
        EXPECT_EQ(sample.value, 0u);
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
