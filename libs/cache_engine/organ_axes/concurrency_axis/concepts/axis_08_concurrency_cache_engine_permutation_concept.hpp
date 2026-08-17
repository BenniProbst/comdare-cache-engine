#pragma once
// V41.F.6.1.R7.3 axis_08_concurrency CacheEngine-Permutation-Concept (Goldstandard)

#include "axis_08_concurrency_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::concurrency_axis::concepts {

/// CacheEnginePermutationStrategy — Pflicht-API jeder permutierbaren Achsen-Wahl:
/// axis_tag (Subaxis-Zuordnung), family_id (eindeutig), name/family_name/flag_suffix,
/// enabled (CMake-Flag). Analog axis_14-Goldstandard.
template <typename C>
concept CacheEnginePermutationStrategy = ConcurrencyStrategy<C> && requires {
    typename C::axis_tag;
    typename C::family_id;
    { C::name() } noexcept -> std::convertible_to<std::string_view>;
    { C::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { C::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { C::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::concurrency_axis::concepts
