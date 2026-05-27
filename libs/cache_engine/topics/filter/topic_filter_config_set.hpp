#pragma once
// V41.F.6.1.R7.5.e TopicConfigSet fuer filter-Topic

#include <topics/filter/axis_filter/axis_filter_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::filter {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_F = axis_filter::EnabledFilters;
    using StaticAxisVariants   = StaticAxisVariants_F;

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
