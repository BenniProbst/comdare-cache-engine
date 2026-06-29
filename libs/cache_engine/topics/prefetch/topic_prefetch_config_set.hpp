#pragma once
// V41.F.6.1.R7.5.a TopicConfigSet fuer prefetch-Topic (1 Achse)
//
// @topic prefetch
//
// 1-Achse Topic. StaticAxisVariants = EnabledPrefetchers der Achse.

#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::prefetch {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_07 = axis_07_prefetch::EnabledPrefetchers;
    using StaticAxisVariants    = StaticAxisVariants_07;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<requires { typename Wrapper::iterable_aspect_t; }, void, void>;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::prefetch
