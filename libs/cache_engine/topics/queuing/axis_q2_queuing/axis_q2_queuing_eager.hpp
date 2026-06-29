#pragma once
// V41.F.6.1 axis_q2_queuing EagerFlush F-EAGER (2026-05-26)
// @topic queuing @achse Q2 @family F01 EagerFlush
// @subaxis FS1 event_triggered

#include "axis_q2_queuing_strategy_base.hpp"
#include "concepts/axis_q2_queuing_concept.hpp"
#include "concepts/axis_q2_queuing_cache_engine_permutation_concept.hpp"
#include "axis_q2_queuing_subaxes_fs1_to_fs4.hpp"
#include "../concepts/topic_queuing_concept.hpp"
#include "../../axis_base.hpp"

#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing {

/**
 * @brief EagerFlush — spuelt nach JEDEM put() (FullFlush)
 *
 * Verwendung: niedrige Latenz, keine Batching-Vorteile. Klassischer
 * Pessimistic-Sync-Pattern.
 */
class EagerFlush : public FlushPolicyStrategyBase<EagerFlush> {
public:
    static constexpr bool enabled = flags::eager_enabled;
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (is_original_module = false).

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::event_triggered_tag;
    using family_id = std::integral_constant<int, 1>; // F01

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "eager_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "EagerFlush (per-op FullFlush, niedrige Latenz)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EAGER"; }

    [[nodiscard]] static constexpr bool is_time_based() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_event_driven() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_adaptive() noexcept { return false; }

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t, std::size_t) const noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_decisions_evaluated;
        ++stats_.full_flush_count;
        observer_.notify(stats_);
#endif
        return concepts::FlushDecision::FullFlush;
    }
    void on_flush_complete() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.flush_complete_count;
        observer_.notify(stats_);
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::FlushPolicyStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

private:
    mutable concepts::FlushPolicyStatistics stats_{};
    mutable observer_t                      observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q2_queuing

namespace comdare::cache_engine::queuing::axis_q2_queuing {
static_assert(concepts::FlushPolicy<EagerFlush>);
static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<EagerFlush>);
static_assert(::comdare::cache_engine::topics::AxisBaseConcept<EagerFlush>);
} // namespace comdare::cache_engine::queuing::axis_q2_queuing
