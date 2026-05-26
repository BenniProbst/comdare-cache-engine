#pragma once
#include "concepts/axis_11_telemetry_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::telemetry::axis_11_telemetry {
template <typename Derived> class TelemetryBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    TelemetryBase() noexcept {
        static_assert(concepts::TelemetryStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
