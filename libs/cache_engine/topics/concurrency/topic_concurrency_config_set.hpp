#pragma once
// V41.F.6.1.R7.3 TopicConfigSet fuer concurrency-Topic

#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::concurrency {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_08 = axis_08_concurrency::EnabledStrategies;
    using StaticAxisVariants    = StaticAxisVariants_08;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<requires { typename Wrapper::iterable_aspect_t; }, void, void>;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::concurrency
