#pragma once
// V41.F.6.1.R7.1 axis_12 General-Hardware Strategy-Concept
//
// Achse 12 (General Hardware): Plattform-Eigenschaften die jeder Algorithmus
// kennen muss um cache-aware Datenstrukturen zu bauen.
//
// Organ-Metapher (Doku 14 §1): kein "Algorithmus" — beschreibt die
// Hardware-Plattform-Familie (Generic / x86-64 / aarch64 / ...) und liefert
// statische Plattform-Konstanten (cache_line_size, memory_page_size,
// simd_width_bits, numa_capable, huge_page_capable).

#include "../../concepts/topic_hardware_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::hardware::axis_12_general_hardware::concepts {

template <typename H>
concept GeneralHardwareStrategy = ::comdare::cache_engine::hardware::concepts::HardwareComponent<H> && requires {
    { H::cache_line_size() } noexcept -> std::convertible_to<std::size_t>;
    { H::memory_page_size() } noexcept -> std::convertible_to<std::size_t>;
    { H::simd_width_bits() } noexcept -> std::convertible_to<std::size_t>;
    { H::numa_capable() } noexcept -> std::convertible_to<bool>;
    { H::huge_page_capable() } noexcept -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::hardware::axis_12_general_hardware::concepts
