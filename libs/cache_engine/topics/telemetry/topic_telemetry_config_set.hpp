#pragma once
// V41.F.6.1.R7.5.b TopicConfigSet fuer telemetry-Topic (1 Achse)

#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::telemetry {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_11 = axis_11_telemetry::EnabledTelemetries;
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
