#pragma once
// V41.F.6.1.R7.5.h TopicConfigSet fuer search_engine-Topic

#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::search_engine {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_01 = axis_01_index_organization::EnabledOrganizations;
    using StaticAxisVariants    = StaticAxisVariants_01;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<requires { typename Wrapper::iterable_aspect_t; }, void, void>;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::search_engine
