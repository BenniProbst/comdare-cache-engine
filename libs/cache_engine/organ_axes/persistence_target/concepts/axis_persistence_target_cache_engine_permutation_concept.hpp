#pragma once
// STRUKT-R ORG-18 axis_persistence_target CacheEngine-Permutation-Concept
// Vorbild 1:1 organ_axes/io_dispatch/concepts/axis_io_cache_engine_permutation_concept.hpp.
//
// Pflicht-Oberflaeche jedes Kompositions-Slot-Werts: axis_tag/family_id (Sub-Achsen-/Familien-Zuordnung),
// name() (binary_id-Segment + Registry-XML), family_name()/flag_suffix() (Reflexion), enabled (mp_filter).

#include "axis_persistence_target_concept.hpp"
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::persistence_target::concepts {

template <typename P>
concept CacheEnginePermutationStrategy = PersistenceTargetStrategy<P> && requires {
    typename P::axis_tag;
    typename P::family_id;
    { P::name() } noexcept -> std::convertible_to<std::string_view>;
    { P::family_name() } noexcept -> std::convertible_to<std::string_view>;
    { P::flag_suffix() } noexcept -> std::convertible_to<std::string_view>;
    { P::enabled } -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::persistence_target::concepts
