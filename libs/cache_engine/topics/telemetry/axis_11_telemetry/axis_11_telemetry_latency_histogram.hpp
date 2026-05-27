#pragma once
// V41.F.6.1.R7.5.b axis_11 LatencyHistogram (Wormhole latency distribution)

#include "axis_11_telemetry_strategy_base.hpp"
#include "axis_11_telemetry_subaxes_tm1_to_tm3.hpp"
#include "concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp"
#include "axis_11_telemetry_flags.hpp"
#include "../concepts/topic_telemetry_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::telemetry::axis_11_telemetry {

/// LatencyHistogram — Lookup-Latenz-Histogramm (Wormhole Wu SIGMOD 2019).
/// HDR-Histogramm (Gil Tene) pro Lookup. Hoechster Overhead, aber liefert
/// p50/p95/p99 Latenz-Statistik fuer SLA-orientierte Mess-Reihen.
class LatencyHistogram : public TelemetryStrategyBase<LatencyHistogram> {
public:
    using topic_tag = ::comdare::cache_engine::telemetry::concepts::TelemetryTopicTag;
    using axis_tag  = subaxes::metric_type_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::latency_histogram_enabled;

    [[nodiscard]] static constexpr bool             is_leaf_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "telemetry_latency_histogram"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "LatencyHistogram (HDR-Histogram p50/p95/p99, Wormhole)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "LATENCY_HISTOGRAM"; }
};

}  // namespace

namespace comdare::cache_engine::telemetry::axis_11_telemetry {
    static_assert(concepts::TelemetryStrategy<LatencyHistogram>);
    static_assert(concepts::CacheEnginePermutationStrategy<LatencyHistogram>);
}
