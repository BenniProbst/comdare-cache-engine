#pragma once
// V41.F.6.1.R7.1 axis_12 GenericHardware — konservative Defaults (Pre-Detection-Fallback).
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

#include "axis_12_general_hardware_base.hpp"
#include "../concepts/topic_hardware_concept.hpp"

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

class GenericHardware : public GeneralHardwareBase<GenericHardware> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using family_id = std::integral_constant<int, 0>;

    [[nodiscard]] static constexpr std::size_t cache_line_size()   noexcept { return 64; }
    [[nodiscard]] static constexpr std::size_t memory_page_size()  noexcept { return 4096; }
    [[nodiscard]] static constexpr std::size_t simd_width_bits()   noexcept { return 0; }
    [[nodiscard]] static constexpr bool        numa_capable()      noexcept { return false; }
    [[nodiscard]] static constexpr bool        huge_page_capable() noexcept { return false; }

    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "general_hardware_generic"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept {
        return "GenericHardware (cache_line=64, page=4K, no SIMD, no NUMA, no huge-pages — conservative defaults)";
    }
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware

namespace comdare::cache_engine::hardware::axis_12_general_hardware {
    static_assert(concepts::GeneralHardwareStrategy<GenericHardware>);
}
