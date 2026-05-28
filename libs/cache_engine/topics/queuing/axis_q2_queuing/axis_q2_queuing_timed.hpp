#pragma once
// V41.F.6.1 axis_q2_queuing TimedFlush F-TIMED (2026-05-26)
// @topic queuing @achse Q2 @family F03 TimedFlush
// @subaxis FS3 time_triggered (erste FS3-Belegung)
//
// Zeitbasierte Flush-Policy: spuelt wenn seit dem letzten Flush mindestens
// `window_ms` Millisekunden vergangen sind. Klassisches Time-Window-Pattern
// (z.B. Hadoop/Spark micro-batching, Kafka log segments).
//
// **iterable_aspect_t Sonderfall:** window_ms ist iterable Aspekt fuer hybride
// Laufzeit-Permutation (analog WatermarkFlush). PermutationEngine generiert
// 1 Binary mit Runtime-Loop ueber kIterableWindowsMs statt 4 separate Binaries.
// **2. Q2-Strategie mit iterable_aspect_t** (nach WatermarkFlush).
//
// Pflicht-Vertrag: should_flush() liest aktuelle Zeit via std::chrono::steady_clock.
// Erfuellt IterableAspectFlushStrategy Sub-Concept.

#include "concepts/axis_q2_queuing_concept.hpp"
#include "concepts/axis_q2_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q2_queuing_iterable_aspect_strategy_concept.hpp"
#include "axis_q2_queuing_subaxes_fs1_to_fs4.hpp"
#include "../concepts/topic_queuing_concept.hpp"
#include "axis_q2_queuing_strategy_base.hpp"
#include "../../axis_base.hpp"

#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing {

/**
 * @brief TimedFlush — spuelt wenn seit letztem Flush window_ms Millisekunden vergangen
 *
 * iterable_aspect_t fuer window_ms (10/100/1000/10000). PermutationEngine generiert
 * 1 Binary mit Runtime-Loop ueber Window-Werte.
 */
class TimedFlush : public FlushPolicyStrategyBase<TimedFlush> {
public:
    static constexpr bool enabled = flags::timed_enabled;
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (is_original_module = false).

    /// Window in Millisekunden als iterable_aspect_t (F.6.1.E hybride Permutation)
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 4> kIterableWindowsMs{10u, 100u, 1000u, 10000u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableWindowsMs.data(), kIterableWindowsMs.size()};
    }

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::time_triggered_tag;
    using family_id = std::integral_constant<int, 3>;  // F03

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "timed_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "TimedFlush (FS3 time_window, micro-batching Hadoop/Spark/Kafka)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "TIMED"; }

    [[nodiscard]] static constexpr bool is_time_based()      noexcept { return true; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_event_driven()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_adaptive()        noexcept { return false; }

    static constexpr std::size_t kDefaultWindowMs = 100;
    TimedFlush() noexcept : window_ms_(kDefaultWindowMs), last_flush_(std::chrono::steady_clock::now()) {}
    explicit TimedFlush(std::size_t window_ms) noexcept
        : window_ms_(window_ms), last_flush_(std::chrono::steady_clock::now()) {}

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t /*fill*/, std::size_t /*cap*/) const noexcept {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush_).count();
        concepts::FlushDecision dec = (static_cast<std::size_t>(elapsed_ms) >= window_ms_)
            ? concepts::FlushDecision::FullFlush
            : concepts::FlushDecision::NoFlush;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_decisions_evaluated;
        if (dec == concepts::FlushDecision::FullFlush) ++stats_.full_flush_count;
        else                                            ++stats_.no_flush_count;
        observer_.notify(stats_);
#endif
        return dec;
    }
    void on_flush_complete() noexcept {
        last_flush_ = std::chrono::steady_clock::now();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.flush_complete_count;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::size_t window_ms() const noexcept { return window_ms_; }

    /// Setter fuer Runtime-Window-Switch ([[iterable-aspect-strategy]] Sub-Concept).
    void set_iterable_aspect(std::size_t new_window_ms) noexcept { window_ms_ = new_window_ms; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::FlushPolicyStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    std::size_t                                    window_ms_;
    mutable std::chrono::steady_clock::time_point  last_flush_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::FlushPolicyStatistics stats_{};
    mutable observer_t                       observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q2_queuing {
    static_assert(concepts::FlushPolicy<TimedFlush>);
    static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<TimedFlush>);
    static_assert(concepts::IterableAspectFlushStrategy<TimedFlush>);
    static_assert(::comdare::cache_engine::topics::AxisBaseConcept<TimedFlush>);
}
