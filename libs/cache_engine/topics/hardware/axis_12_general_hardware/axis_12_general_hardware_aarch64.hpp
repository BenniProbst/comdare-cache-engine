#pragma once
// V41.F.6.1.R7.1 axis_12 Aarch64Hardware — ARM64 (server/mobile/Apple Silicon).
//
// Werte:
// - cache_line_size = 64 (ARM Cortex-A/Neoverse Standard;
//                         Apple Silicon M1/M2/M3 hat 128 — siehe AppleSiliconHardware-Variante zukuenftig)
// - memory_page_size = 4096 (ARM-Linux Default;
//                            macOS Apple Silicon = 16384, siehe Variante)
// - simd_width_bits = 128 (NEON Standard-Width auf allen Aarch64)
// - numa_capable = true (Multi-Socket Aarch64-Server wie Ampere Altra)
// - huge_page_capable = true (transparent hugepages auf Aarch64-Linux)

#include "axis_12_general_hardware_base.hpp"
#include "../concepts/topic_hardware_concept.hpp"

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

class Aarch64Hardware : public GeneralHardwareBase<Aarch64Hardware> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using family_id = std::integral_constant<int, 2>;

    [[nodiscard]] static constexpr std::size_t cache_line_size()   noexcept { return 64; }
    [[nodiscard]] static constexpr std::size_t memory_page_size()  noexcept { return 4096; }
    [[nodiscard]] static constexpr std::size_t simd_width_bits()   noexcept { return 128; }  // NEON Standard
    [[nodiscard]] static constexpr bool        numa_capable()      noexcept { return true; }
    [[nodiscard]] static constexpr bool        huge_page_capable() noexcept { return true; }

    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "general_hardware_aarch64"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept {
        return "Aarch64Hardware (cache_line=64, page=4K, NEON 128-bit SIMD, NUMA + huge-pages capable)";
    }
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware

namespace comdare::cache_engine::hardware::axis_12_general_hardware {
    static_assert(concepts::GeneralHardwareStrategy<Aarch64Hardware>);
}
