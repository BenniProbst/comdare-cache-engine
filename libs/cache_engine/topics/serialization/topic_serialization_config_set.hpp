#pragma once
// V41.F.6.1.R7.5.c TopicConfigSet fuer serialization-Topic

#include <topics/serialization/axis_10_serialization/axis_10_serialization_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::serialization {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_10 = axis_10_serialization::EnabledSerializers;
    using StaticAxisVariants    = StaticAxisVariants_10;

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
