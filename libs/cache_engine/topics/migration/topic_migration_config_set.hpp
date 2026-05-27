#pragma once
// V41.F.6.1.R7.5.g TopicConfigSet fuer migration-Topic

#include <topics/migration/axis_migration/axis_migration_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::migration {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_MG = axis_migration::EnabledMigrations;
    using StaticAxisVariants    = StaticAxisVariants_MG;

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
