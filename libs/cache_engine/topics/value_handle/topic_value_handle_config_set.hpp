#pragma once
// V41.F.6.1.R7.5.d TopicConfigSet fuer value_handle-Topic

#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::value_handle {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_14 = axis_14_value_handle::EnabledHandles;
    using StaticAxisVariants    = StaticAxisVariants_14;

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
