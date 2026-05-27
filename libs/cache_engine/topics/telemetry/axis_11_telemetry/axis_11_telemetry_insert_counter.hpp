#pragma once
// V41.F.6.1.R7.5.b axis_11 InsertCounter (HOT global insert-counter)

#include "axis_11_telemetry_strategy_base.hpp"
#include "axis_11_telemetry_subaxes_tm1_to_tm3.hpp"
#include "concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp"
#include "axis_11_telemetry_flags.hpp"
#include "../concepts/topic_telemetry_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::telemetry::axis_11_telemetry {

/// InsertCounter — globaler Insert-Zaehler (HOT Binna PVLDB 2018).
/// Niedriger Overhead (1 atomic increment pro insert), keine per-node-Daten.
/// Genuegt fuer Throughput-Mess-Reihen.
class InsertCounter : public TelemetryStrategyBase<InsertCounter> {
public:
    using topic_tag = ::comdare::cache_engine::telemetry::concepts::TelemetryTopicTag;
    using axis_tag  = subaxes::scope_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::insert_counter_enabled;

    [[nodiscard]] static constexpr bool             is_leaf_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "telemetry_insert_counter"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "InsertCounter (HOT global insert counter, low-overhead)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "INSERT_COUNTER"; }
};

}  // namespace

namespace comdare::cache_engine::telemetry::axis_11_telemetry {
    static_assert(concepts::TelemetryStrategy<InsertCounter>);
    static_assert(concepts::CacheEnginePermutationStrategy<InsertCounter>);
}
