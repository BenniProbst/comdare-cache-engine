#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout CacheEngine-Permutation-Concept
//
// @topic memory_layout
// @achse 05
//
// Goldstandard-Pattern: alle Wrappers muessen die Pflicht-API exponieren
// damit der CacheEngineBuilder ueber sie permutieren kann.

#include "axis_05_memory_layout_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::layout::concepts {

/// CacheEnginePermutationStrategy — Pflicht-API fuer alle Memory-Layout-Wrapper:
///   - axis_tag         : Subaxis-Tag (HM1-HM4)
///   - family_id        : numerischer Family-Identifier
///   - name()           : eindeutiger Name (kebab-case)
///   - family_name()    : human-readable Beschreibung
///   - flag_suffix()    : CMake-Flag-Suffix (UPPERCASE)
///   - enabled          : Compile-Time-Boolean ob via CMake-Flag aktiviert
template <typename P>
concept CacheEnginePermutationStrategy = MemoryLayoutStrategy<P> && requires {
    typename P::axis_tag;
    typename P::family_id;
    { P::name() } noexcept -> std::convertible_to<std::string_view>;
    { P::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { P::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { P::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::layout::concepts
