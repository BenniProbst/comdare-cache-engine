#pragma once
// V41.F.6.1 axis_q2_flush_policy WatermarkFlush F-WATERMARK (2026-05-26)
// @topic queuing @achse Q2 @family F02 WatermarkFlush
// @subaxis FS2 threshold_triggered

#include "concepts/axis_q2_flush_policy_concept.hpp"
#include "concepts/axis_q2_flush_policy_cache_engine_permutation_concept.hpp"
#include "axis_q2_flush_policy_subaxes_fs1_to_fs3.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q2_flush_policy/axis_q2_flush_policy_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {

/**
 * @brief WatermarkFlush — spuelt wenn fill >= threshold (Default 75% Kapazitaet)
 *
 * iterable_aspect_t fuer Watermark-Prozent (50/65/75/85/95). PermutationEngine
 * generiert 1 Binary mit Runtime-Loop ueber Watermark-Werte.
 */
class WatermarkFlush {
public:
    static constexpr bool enabled = flags::watermark_enabled;

    /// Watermark-Prozent als iterable_aspect_t (F.6.1.E hybride Permutation)
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 5> kIterableWatermarks{50u, 65u, 75u, 85u, 95u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableWatermarks.data(), kIterableWatermarks.size()};
    }

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::threshold_triggered_tag;
    using family_id = std::integral_constant<int, 2>;  // F02

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "watermark_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "WatermarkFlush (threshold-getriggert, Default 75%)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "WATERMARK"; }

    [[nodiscard]] static constexpr bool is_time_based()      noexcept { return false; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_event_driven()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_adaptive()        noexcept { return false; }

    static constexpr unsigned kDefaultWatermarkPct = 75;
    WatermarkFlush() noexcept : threshold_pct_(kDefaultWatermarkPct) {}
    explicit WatermarkFlush(unsigned pct) noexcept : threshold_pct_(pct) {}

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t fill, std::size_t cap) const noexcept {
        concepts::FlushDecision dec = concepts::FlushDecision::NoFlush;
        if (cap > 0) {
            unsigned current_pct = static_cast<unsigned>((fill * 100u) / cap);
            if (current_pct >= threshold_pct_) dec = concepts::FlushDecision::FullFlush;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_decisions_evaluated;
        if (dec == concepts::FlushDecision::FullFlush) ++stats_.full_flush_count;
        else                                            ++stats_.no_flush_count;
        observer_.notify(stats_);
#endif
        return dec;
    }
    void on_flush_complete() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.flush_complete_count;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] unsigned threshold_pct() const noexcept { return threshold_pct_; }
    void set_threshold_pct(unsigned pct) noexcept { threshold_pct_ = pct; }

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
    unsigned threshold_pct_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::FlushPolicyStatistics stats_{};
    mutable observer_t                       observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q2_flush_policy {
    static_assert(concepts::FlushPolicy<WatermarkFlush>);
    static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<WatermarkFlush>);
}
