#pragma once
// V41.F.6.1.R7.5.b axis_11_telemetry CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_11_telemetry_concept.hpp"
#include "concepts/axis_11_telemetry_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

namespace comdare::cache_engine::telemetry_axis {

template <typename Derived>
class TelemetryStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    TelemetryStrategyBase() noexcept {
        static_assert(concepts::TelemetryStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::telemetry_axis
