#pragma once
// V41.F.6.1 axis_q2_queuing AdaptiveLsmFlush F-ADAPTIVE_LSM (2026-05-26)
// @topic queuing @achse Q2 @family F05 AdaptiveLsmFlush
// @subaxis FS4 adaptive_triggered (erste FS4-Belegung)
//
// Adaptive Flush-Policy fuer LSM-Trees (Levandoski 2013, RocksDB Dynamic
// Level-Based Compaction). Lernt aus Workload: Write-Rate, Read-Rate,
// Compact-Stalls. Ein laufender EWMA (Exponential Weighted Moving Average)
// ueber die letzte Write-Burst-Rate steuert den Watermark-Threshold
// adaptiv (60-95%).
//
// Konzept:
//   - Hohe Write-Rate → fruehzeitig spuelen (niedriger threshold)
//   - Niedrige Write-Rate → spaet spuelen (hoeher threshold, mehr Batching)
//   - on_flush_complete() updated EWMA mit Time-Delta
//
// **Erste Q2-Strategie mit is_adaptive()=true**. Erfuellt KEIN
// IterableAspectFlushStrategy Sub-Concept (kein iterable Aspekt — lernt selbst).
//
// Allocation: keine (alle State-Variablen Stack-allokiert).

#include "concepts/axis_q2_queuing_concept.hpp"
#include "concepts/axis_q2_queuing_cache_engine_permutation_concept.hpp"
#include "axis_q2_queuing_subaxes_fs1_to_fs4.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <chrono>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing {

/**
 * @brief AdaptiveLsmFlush — adaptiver Watermark mit EWMA ueber Write-Burst-Rate
 *
 * **Erste Q2-Strategie mit is_adaptive()=true.** Sonderfall: kein iterable_aspect_t
 * — Algorithmus passt Threshold selbst an. ewma_burst_rate_ steuert
 * effektiven Threshold zwischen kMinThreshold (60%) und kMaxThreshold (95%).
 */
class AdaptiveLsmFlush {
public:
    static constexpr bool enabled = flags::adaptive_lsm_enabled;

    using topic_tag = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag  = subaxes::adaptive_triggered_tag;
    using family_id = std::integral_constant<int, 5>;  // F05

    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "adaptive_lsm_flush"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "AdaptiveLsmFlush (FS4, EWMA-adaptiver Watermark — RocksDB DynamicLevel)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ADAPTIVE_LSM"; }

    [[nodiscard]] static constexpr bool is_time_based()      noexcept { return false; }
    [[nodiscard]] static constexpr bool is_threshold_based() noexcept { return true; }  // intern Threshold-basiert
    [[nodiscard]] static constexpr bool is_event_driven()    noexcept { return false; }
    /// SONDERFALL: erste Q2-Strategie mit is_adaptive=true.
    [[nodiscard]] static constexpr bool is_adaptive()        noexcept { return true; }

    static constexpr unsigned kMinThresholdPct    = 60;
    static constexpr unsigned kMaxThresholdPct    = 95;
    static constexpr unsigned kInitialThresholdPct = 75;
    static constexpr double   kEwmaAlpha           = 0.2;  // Glaettungsfaktor (0=no learning, 1=immediate)

    AdaptiveLsmFlush() noexcept
        : current_threshold_pct_(kInitialThresholdPct)
        , ewma_burst_rate_(0.0)
        , last_decision_(std::chrono::steady_clock::now()) {}

    [[nodiscard]] concepts::FlushDecision should_flush(std::size_t fill, std::size_t cap) const noexcept {
        concepts::FlushDecision dec = concepts::FlushDecision::NoFlush;
        if (cap > 0) {
            unsigned current_pct = static_cast<unsigned>((fill * 100u) / cap);
            if (current_pct >= current_threshold_pct_) dec = concepts::FlushDecision::FullFlush;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_decisions_evaluated;
        if (dec == concepts::FlushDecision::FullFlush) ++stats_.full_flush_count;
        else                                            ++stats_.no_flush_count;
        observer_.notify(stats_);
#endif
        return dec;
    }

    /// Updated EWMA aus Time-Delta zwischen Flushes + adjusted Threshold.
    /// Hohe Frequenz → ewma steigt → threshold sinkt (frueher spuelen).
    void on_flush_complete() noexcept {
        auto now = std::chrono::steady_clock::now();
        auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_decision_).count();
        double rate = (delta_us > 0) ? (1'000'000.0 / static_cast<double>(delta_us)) : 1.0;
        ewma_burst_rate_ = kEwmaAlpha * rate + (1.0 - kEwmaAlpha) * ewma_burst_rate_;
        // Mapping: hohe Rate → niedriger Threshold (60), niedrige Rate → hoeher (95)
        // Heuristik: clamp ewma in [0, 100] und linear interpolieren
        double clamped = (ewma_burst_rate_ > 100.0) ? 100.0 : ewma_burst_rate_;
        double t = clamped / 100.0;  // [0,1]
        current_threshold_pct_ = static_cast<unsigned>(
            kMaxThresholdPct - t * (kMaxThresholdPct - kMinThresholdPct));
        last_decision_ = now;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.flush_complete_count;
        observer_.notify(stats_);
#endif
    }

    /// Adaptiv-spezifisch: aktueller berechneter Threshold (zwischen 60-95).
    [[nodiscard]] unsigned current_threshold_pct() const noexcept { return current_threshold_pct_; }
    /// Adaptiv-spezifisch: aktuelle EWMA-Burst-Rate.
    [[nodiscard]] double   ewma_burst_rate()       const noexcept { return ewma_burst_rate_; }

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
    mutable unsigned                              current_threshold_pct_;
    mutable double                                ewma_burst_rate_;
    mutable std::chrono::steady_clock::time_point last_decision_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::FlushPolicyStatistics stats_{};
    mutable observer_t                       observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q2_queuing {
    static_assert(concepts::FlushPolicy<AdaptiveLsmFlush>);
    static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<AdaptiveLsmFlush>);
}
