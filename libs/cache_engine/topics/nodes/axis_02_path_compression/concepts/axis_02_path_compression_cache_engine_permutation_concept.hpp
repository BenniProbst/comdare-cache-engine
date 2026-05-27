#pragma once
// V41.F.6.1.R7.1.c axis_02_path_compression CacheEngine-Permutation-Concept

#include "axis_02_path_compression_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::nodes::axis_02_path_compression::concepts {

template <typename P>
concept CacheEnginePermutationStrategy =
    PathCompressionStrategy<P>
    && requires {
        typename P::axis_tag;
        typename P::family_id;
        { P::name() }         noexcept -> std::convertible_to<std::string_view>;
        { P::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { P::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { P::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace
