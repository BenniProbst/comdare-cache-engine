#pragma once
// V41.F.6.1 axis_q2_flush_policy LazyFlush F-LAZY (2026-05-26)
// @topic queuing @achse Q2 @family F04 LazyFlush
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
 * @brief LazyFlush — spuelt NIE automatisch (nur bei expliziter Eviction)
 *
 * Verwendung: Maximum-Batching, niedriger Spuelaufwand. Klassischer
 * Optimistic-Defer-Pattern. should_flush liefert IMMER NoFlush.
 */
class LazyFlush {
public:
    static constexpr bool enabled = flags::lazy_enabled;

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::event_triggered_tag;
    using family_id = std::integral_constant<int, 4>;  // F04

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "lazy_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "LazyFlush (defer until eviction, maximum batching)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LAZY"; }

    [[nodiscard]] static constexpr bool is_time_based()      noexcept { return false; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_event_driven()    noexcept { return true; }
    [[nodiscard]] static constexpr bool is_adaptive()        noexcept { return false; }

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t, std::size_t) const noexcept {
        return concepts::FlushDecision::NoFlush;
    }
    void on_flush_complete() noexcept {}
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {
    static_assert(concepts::FlushPolicy<LazyFlush>);
    static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<LazyFlush>);
}
