#pragma once
// V41.F.6.1.R7.5.g axis_migration CacheEngine-Permutation-Concept

#include "axis_migration_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::migration_policy::concepts {

template <typename M>
concept CacheEnginePermutationStrategy =
    MigrationStrategy<M>
    && requires {
        typename M::axis_tag;
        typename M::family_id;
        { M::name() }         noexcept -> std::convertible_to<std::string_view>;
        { M::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { M::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { M::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace
