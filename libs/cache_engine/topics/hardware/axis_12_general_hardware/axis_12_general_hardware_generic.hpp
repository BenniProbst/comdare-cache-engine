#pragma once
// V41.F.6.1.R7.1 axis_12 GenericHardwareProfile — konservative Defaults (Pre-Detection-Fallback).
//
// Verwendung: wenn kein spezifischer Plattform-Adapter (x86_64/aarch64/etc.)
// passt, oder fuer Plattform-agnostische Permutations-Konfigurationen.
//
// Werte sind "lowest-common-denominator":
// - cache_line_size = 64 (x86, ARM Cortex-A, Modern-Default)
// - memory_page_size = 4096 (POSIX-Default)
// - simd_width_bits = 0 (scalar-baseline)
// - numa_capable = false (single-socket Assumption)
// - huge_page_capable = false

#include "axis_12_general_hardware_strategy_base.hpp"
#include "axis_12_general_hardware_subaxes_hw1_to_hw4.hpp"
#include "../concepts/topic_hardware_concept.hpp"

// Generated flags via CMake configure_file
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_flags.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

class GenericHardwareProfile : public GeneralHardwareStrategyBase<GenericHardwareProfile> {
public:
    // Topic + Subaxis Tags (Pflicht-API CacheEnginePermutationStrategy)
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::cpu_family_tag;
    using family_id = std::integral_constant<int, 0>;

    // Plattform-Properties (GeneralHardwareStrategy)
    [[nodiscard]] static constexpr std::size_t cache_line_size()   noexcept { return 64; }
    [[nodiscard]] static constexpr std::size_t memory_page_size()  noexcept { return 4096; }
    [[nodiscard]] static constexpr std::size_t simd_width_bits()   noexcept { return 0; }
    [[nodiscard]] static constexpr bool        numa_capable()      noexcept { return false; }
    [[nodiscard]] static constexpr bool        huge_page_capable() noexcept { return false; }

    // Identifikation (Pflicht-API CacheEnginePermutationStrategy)
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "general_hardware_generic"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept {
        return "GenericHardwareProfile (cache_line=64, page=4K, no SIMD, no NUMA, no huge-pages — conservative defaults)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "GENERIC"; }

    // CMake-Flag-Mapping (Pflicht fuer Registry::is_enabled)
    static constexpr bool enabled = flags::generic_enabled;
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware

namespace comdare::cache_engine::hardware::axis_12_general_hardware {
    static_assert(concepts::GeneralHardwareStrategy<GenericHardwareProfile>);
    static_assert(concepts::CacheEnginePermutationStrategy<GenericHardwareProfile>);
}
