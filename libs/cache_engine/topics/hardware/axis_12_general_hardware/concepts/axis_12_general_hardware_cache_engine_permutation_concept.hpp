#pragma once
// V41.F.6.1.R7.1.a.2 cache-engine-spezifisches Pflicht-Concept fuer GeneralHardware
//
// @topic hardware
// @achse 12
//
// **Parallel zu GeneralHardwareStrategy:** cache-engine-spezifische Pflicht-API
// die jede Plattform-Variante anbieten muss, damit sie als Permutations-Variante
// in der cache-engine-Pipeline registrierbar ist.
//
// Trennung zum Domain-Concept:
//   - GeneralHardwareStrategy = Plattform-Properties (cache_line_size, simd_width_bits, ...)
//   - CacheEnginePermutationStrategy = cache-engine-Permutationen-Identitaet
//
// Eine Klasse erfuellt typischerweise BEIDE Concepts.

#include "axis_12_general_hardware_concept.hpp"
#include "../axis_12_general_hardware_subaxes_hw1_to_hw4.hpp"

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::hardware::axis_12_general_hardware::concepts {

/**
 * @brief CacheEnginePermutationStrategy — Pflicht-API fuer Permutations-Identitaet
 * @topic hardware
 * @achse 12
 *
 * Pflicht-Members:
 *   - typename axis_tag      (Subaxis-Tag aus subaxes/ — HW1/HW2/HW3/HW4)
 *   - typename family_id     (std::integral_constant<int, N> — Wrapper-Index)
 *   - name()                 (constexpr string_view — z.B. "general_hardware_x86_64")
 *   - family_name()          (constexpr string_view — beschreibender Name)
 *   - flag_suffix()          (constexpr string_view — fuer CMake-Flag-Mapping z.B. "X86_64")
 *   - static constexpr bool enabled  (aus flags.hpp via configure_file)
 */
template <typename P>
concept CacheEnginePermutationStrategy =
    GeneralHardwareStrategy<P>
    && requires {
        typename P::axis_tag;
        typename P::family_id;
        { P::name() }         noexcept -> std::convertible_to<std::string_view>;
        { P::family_name() }  noexcept -> std::convertible_to<std::string_view>;
        { P::flag_suffix() }  noexcept -> std::convertible_to<std::string_view>;
        { P::enabled }                 -> std::convertible_to<bool>;
    };

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware::concepts
