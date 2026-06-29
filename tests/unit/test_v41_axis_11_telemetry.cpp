// V41.F.6.1.R7.5.b axis_11_telemetry Tests (Goldstandard-konform)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.b (Optional-Topics: telemetry)

#include <gtest/gtest.h>

#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_density_tracker.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_insert_counter.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_latency_histogram.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_registry.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_subaxes_tm1_to_tm3.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_flags.hpp>
#include <topics/telemetry/topic_telemetry_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax11 = ::comdare::cache_engine::telemetry::axis_11_telemetry;
namespace tel  = ::comdare::cache_engine::telemetry;
namespace mp   = ::boost::mp11;

TEST(R7_5_b_Axis11, AllTelemetriesSatisfyConcepts) {
    static_assert(ax11::concepts::TelemetryStrategy<ax11::LeafOnlyCounter>);
    static_assert(ax11::concepts::TelemetryStrategy<ax11::DensityTracker>);
    static_assert(ax11::concepts::TelemetryStrategy<ax11::InsertCounter>);
    static_assert(ax11::concepts::TelemetryStrategy<ax11::LatencyHistogram>);
    static_assert(ax11::concepts::CacheEnginePermutationStrategy<ax11::LeafOnlyCounter>);
    static_assert(ax11::concepts::CacheEnginePermutationStrategy<ax11::DensityTracker>);
    static_assert(ax11::concepts::CacheEnginePermutationStrategy<ax11::InsertCounter>);
    static_assert(ax11::concepts::CacheEnginePermutationStrategy<ax11::LatencyHistogram>);
    SUCCEED();
}

TEST(R7_5_b_Axis11, IsLeafOnlyDifferentiated) {
    static_assert(ax11::LeafOnlyCounter::is_leaf_only() == true);
    static_assert(ax11::DensityTracker::is_leaf_only() == false);
    static_assert(ax11::InsertCounter::is_leaf_only() == false);
    static_assert(ax11::LatencyHistogram::is_leaf_only() == false);
    SUCCEED();
}

TEST(R7_5_b_Axis11, FlagSuffixUppercase) {
    static_assert(ax11::LeafOnlyCounter::flag_suffix() == std::string_view{"LEAF_ONLY_COUNTER"});
    static_assert(ax11::DensityTracker::flag_suffix() == std::string_view{"DENSITY_TRACKER"});
    static_assert(ax11::InsertCounter::flag_suffix() == std::string_view{"INSERT_COUNTER"});
    static_assert(ax11::LatencyHistogram::flag_suffix() == std::string_view{"LATENCY_HISTOGRAM"});
    SUCCEED();
}

TEST(R7_5_b_Axis11, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax11::LeafOnlyCounter::axis_tag, ax11::subaxes::scope_tag>);
    static_assert(std::is_same_v<ax11::DensityTracker::axis_tag, ax11::subaxes::metric_type_tag>);
    static_assert(std::is_same_v<ax11::InsertCounter::axis_tag, ax11::subaxes::scope_tag>);
    static_assert(std::is_same_v<ax11::LatencyHistogram::axis_tag, ax11::subaxes::metric_type_tag>);
    SUCCEED();
}

TEST(R7_5_b_Axis11, RegistryHas4Telemetries) {
    static_assert(mp::mp_size<ax11::AllTelemetries>::value == 4);
    static_assert(mp::mp_size<ax11::EnabledTelemetries>::value > 0);
    SUCCEED();
}

TEST(R7_5_b_Axis11, FamilyIdsDistinct) {
    static_assert(ax11::LeafOnlyCounter::family_id::value == 1);
    static_assert(ax11::DensityTracker::family_id::value == 2);
    static_assert(ax11::InsertCounter::family_id::value == 3);
    static_assert(ax11::LatencyHistogram::family_id::value == 4);
    SUCCEED();
}

TEST(R7_5_b_Axis11, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax11::flags::leaf_only_counter_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax11::flags::density_tracker_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax11::flags::insert_counter_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax11::flags::latency_histogram_enabled), const bool>);
    SUCCEED();
}

TEST(R7_5_b_Telemetry, TopicConfigSetExposesAxis11) {
    static_assert(mp::mp_size<tel::TopicConfigSet::StaticAxisVariants_11>::value > 0);
    static_assert(std::is_same_v<tel::TopicConfigSet::StaticAxisVariants, tel::TopicConfigSet::StaticAxisVariants_11>);
    SUCCEED();
}
