#pragma once
// V41.F.6.1.R7.1 axis_12 X86_64Hardware — modern x86-64 desktop/server Plattform.
//
// Werte:
// - cache_line_size = 64 (alle modernen x86, Sandy Bridge+)
// - memory_page_size = 4096 (Default 4 KiB Pages)
// - simd_width_bits = 256 (AVX2 Conservative-Default; AVX-512 nur ausgewaehlte Xeons)
// - numa_capable = true (Multi-Socket-Server typisch)
// - huge_page_capable = true (2 MiB + 1 GiB MAP_HUGETLB seit Linux 2.6)
//
// Hinweis: das ist der Build-Time-Wert ("welche Plattform plant der
// Konfiguration aus?"), nicht der Runtime-Detection-Wert. Letztere kommt aus
// IPlatformProbe (Doku 11 §K07 / V42-Task #650).

#include "axis_12_general_hardware_base.hpp"
#include "axis_12_general_hardware_subaxes_hw1_to_hw4.hpp"
#include "../concepts/topic_hardware_concept.hpp"

#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_flags.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

class X86_64Hardware : public GeneralHardwareBase<X86_64Hardware> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using axis_tag  = subaxes::cpu_family_tag;
    using family_id = std::integral_constant<int, 1>;

    [[nodiscard]] static constexpr std::size_t cache_line_size()   noexcept { return 64; }
    [[nodiscard]] static constexpr std::size_t memory_page_size()  noexcept { return 4096; }
    [[nodiscard]] static constexpr std::size_t simd_width_bits()   noexcept { return 256; }  // AVX2 Conservative
    [[nodiscard]] static constexpr bool        numa_capable()      noexcept { return true; }
    [[nodiscard]] static constexpr bool        huge_page_capable() noexcept { return true; }

    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "general_hardware_x86_64"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept {
        return "X86_64Hardware (cache_line=64, page=4K, AVX2 256-bit SIMD, NUMA + huge-pages capable)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "X86_64"; }

    static constexpr bool enabled = flags::x86_64_enabled;
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware

namespace comdare::cache_engine::hardware::axis_12_general_hardware {
    static_assert(concepts::GeneralHardwareStrategy<X86_64Hardware>);
    static_assert(concepts::CacheEnginePermutationStrategy<X86_64Hardware>);
}
