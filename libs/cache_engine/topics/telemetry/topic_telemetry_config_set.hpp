#pragma once
// V41.F.6.1.R7.5.b TopicConfigSet fuer telemetry-Topic (1 Achse)

#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_registry.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_observable.hpp>   // V42 L-74c: ObservableTelemetry-Huelle

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::telemetry {

namespace mp = boost::mp11;

// V42 L-74c: macht den Permutations-Pfad (BR-1 registry_to_axis_levels) telemetry-OBSERVABLE — jede
// Enabled-Strategie wird in die ObservableTelemetry-Huelle gewickelt, sodass auch AUTO-emittierte AdHoc-
// Kompositionen (und die DLL) telemetry als getriebene ObservableAxis tragen (nicht nur die benannten
// Reference-Compositions). Die Huelle reicht name()/topic_tag durch → bricht BR-1/BR-2/BR-3 nicht.
template <class S> using make_observable_telemetry = ::comdare::cache_engine::telemetry_axis::ObservableTelemetry<S>;

struct TopicConfigSet {
    using StaticAxisVariants_11 = mp::mp_transform<make_observable_telemetry, axis_11_telemetry::EnabledTelemetries>;
    using StaticAxisVariants    = StaticAxisVariants_11;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<
        requires { typename Wrapper::iterable_aspect_t; },
        void,
        void
    >;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

}  // namespace
