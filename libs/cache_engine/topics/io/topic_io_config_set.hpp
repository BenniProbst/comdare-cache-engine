#pragma once
// V41.F.6.1.R7.5.f TopicConfigSet fuer io-Topic

#include <topics/io/axis_io/axis_io_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::io {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_IO = axis_io::EnabledIos;
    using StaticAxisVariants    = StaticAxisVariants_IO;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<requires { typename Wrapper::iterable_aspect_t; }, void, void>;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::io
