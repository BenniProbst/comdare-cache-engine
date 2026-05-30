#pragma once
// V41.F.6.1.R7.5.h axis_01_index_organization CacheEngine-Permutation-Concept

#include "axis_01_index_organization_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::index_organization::concepts {

template <typename I>
concept CacheEnginePermutationStrategy =
    IndexOrganizationStrategy<I>
    && requires {
        typename I::axis_tag;
        typename I::family_id;
        { I::name() }         noexcept -> std::convertible_to<std::string_view>;
        { I::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { I::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { I::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace
