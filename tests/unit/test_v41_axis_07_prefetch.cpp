// V41.F.6.1.R7.5.a axis_07_prefetch Tests (Goldstandard-konform)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.a (Optional-Topics Start: prefetch)

#include <gtest/gtest.h>

#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_distance_estimator.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_hardware_prefetch.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_path_oriented.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_registry.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_subaxes_pf1_to_pf3.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_flags.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax07 = ::comdare::cache_engine::prefetch::axis_07_prefetch;
namespace pf   = ::comdare::cache_engine::prefetch;
namespace mp   = ::boost::mp11;

TEST(R7_5_a_Axis07, AllPrefetchersSatisfyConcepts) {
    static_assert(ax07::concepts::PrefetchStrategy<ax07::PrefetchNone>);
    static_assert(ax07::concepts::PrefetchStrategy<ax07::DistanceEstimatorPrefetch>);
    static_assert(ax07::concepts::PrefetchStrategy<ax07::HardwarePrefetch>);
    static_assert(ax07::concepts::PrefetchStrategy<ax07::PathOrientedPrefetch>);
    static_assert(ax07::concepts::CacheEnginePermutationStrategy<ax07::PrefetchNone>);
    static_assert(ax07::concepts::CacheEnginePermutationStrategy<ax07::DistanceEstimatorPrefetch>);
    static_assert(ax07::concepts::CacheEnginePermutationStrategy<ax07::HardwarePrefetch>);
    static_assert(ax07::concepts::CacheEnginePermutationStrategy<ax07::PathOrientedPrefetch>);
    SUCCEED();
}

TEST(R7_5_a_Axis07, IsActiveDifferentiated) {
    static_assert(ax07::PrefetchNone::is_active()              == false);
    static_assert(ax07::DistanceEstimatorPrefetch::is_active() == true);
    static_assert(ax07::HardwarePrefetch::is_active()          == true);
    static_assert(ax07::PathOrientedPrefetch::is_active()      == true);
    SUCCEED();
}

TEST(R7_5_a_Axis07, FlagSuffixUppercase) {
    static_assert(ax07::PrefetchNone::flag_suffix()              == std::string_view{"NONE"});
    static_assert(ax07::DistanceEstimatorPrefetch::flag_suffix() == std::string_view{"DISTANCE_ESTIMATOR"});
    static_assert(ax07::HardwarePrefetch::flag_suffix()          == std::string_view{"HARDWARE_PREFETCH"});
    static_assert(ax07::PathOrientedPrefetch::flag_suffix()      == std::string_view{"PATH_ORIENTED"});
    SUCCEED();
}

TEST(R7_5_a_Axis07, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax07::PrefetchNone::axis_tag,              ax07::subaxes::trigger_mechanism_tag>);
    static_assert(std::is_same_v<ax07::DistanceEstimatorPrefetch::axis_tag, ax07::subaxes::distance_heuristic_tag>);
    static_assert(std::is_same_v<ax07::HardwarePrefetch::axis_tag,          ax07::subaxes::trigger_mechanism_tag>);
    static_assert(std::is_same_v<ax07::PathOrientedPrefetch::axis_tag,      ax07::subaxes::granularity_tag>);
    SUCCEED();
}

TEST(R7_5_a_Axis07, RegistryHas4Prefetchers) {
    static_assert(mp::mp_size<ax07::AllPrefetchers>::value == 4);
    static_assert(mp::mp_size<ax07::EnabledPrefetchers>::value > 0);
    SUCCEED();
}

TEST(R7_5_a_Axis07, FamilyIdsDistinct) {
    static_assert(ax07::PrefetchNone::family_id::value              == 0);
    static_assert(ax07::DistanceEstimatorPrefetch::family_id::value == 1);
    static_assert(ax07::HardwarePrefetch::family_id::value          == 2);
    static_assert(ax07::PathOrientedPrefetch::family_id::value      == 3);
    SUCCEED();
}

TEST(R7_5_a_Axis07, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax07::flags::none_enabled),               const bool>);
    static_assert(std::is_same_v<decltype(ax07::flags::distance_estimator_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax07::flags::hardware_prefetch_enabled),  const bool>);
    static_assert(std::is_same_v<decltype(ax07::flags::path_oriented_enabled),      const bool>);
    SUCCEED();
}

TEST(R7_5_a_Prefetch, TopicConfigSetExposesAxis07) {
    static_assert(mp::mp_size<pf::TopicConfigSet::StaticAxisVariants_07>::value > 0);
    static_assert(std::is_same_v<pf::TopicConfigSet::StaticAxisVariants,
                                  pf::TopicConfigSet::StaticAxisVariants_07>);
    SUCCEED();
}
