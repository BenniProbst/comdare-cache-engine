#pragma once
// V41.F.6.1 axis_q2_flush_policy EagerFlush F-EAGER (2026-05-26)
// @topic queuing @achse Q2 @family F01 EagerFlush
// @subaxis FS1 event_triggered

#include "concepts/axis_q2_flush_policy_concept.hpp"
#include "concepts/axis_q2_flush_policy_cache_engine_permutation_concept.hpp"
#include "axis_q2_flush_policy_subaxes_fs1_to_fs3.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q2_flush_policy/axis_q2_flush_policy_flags.hpp>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {

/**
 * @brief EagerFlush — spuelt nach JEDEM put() (FullFlush)
 *
 * Verwendung: niedrige Latenz, keine Batching-Vorteile. Klassischer
 * Pessimistic-Sync-Pattern.
 */
class EagerFlush {
public:
    static constexpr bool enabled = flags::eager_enabled;

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::event_triggered_tag;
    using family_id = std::integral_constant<int, 1>;  // F01

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "eager_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "EagerFlush (per-op FullFlush, niedrige Latenz)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EAGER"; }

    [[nodiscard]] static constexpr bool is_time_based()      noexcept { return false; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_event_driven()    noexcept { return true; }
    [[nodiscard]] static constexpr bool is_adaptive()        noexcept { return false; }

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t, std::size_t) const noexcept {
        return concepts::FlushDecision::FullFlush;
    }
    void on_flush_complete() noexcept {}
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {
    static_assert(concepts::FlushPolicy<EagerFlush>);
    static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<EagerFlush>);
}
