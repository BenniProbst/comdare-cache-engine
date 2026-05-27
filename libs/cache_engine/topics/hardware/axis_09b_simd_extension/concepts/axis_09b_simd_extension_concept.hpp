#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension Strategy-Concept (SIMD/Accelerator)
//
// Pflicht-API:
//   - is_active() : true wenn Extension benutzt wird (NoSimdExtension=false)
//   - vector_width_bits() : 0 (None), 128 (SSE/NEON), 256 (AVX2), 512 (AVX-512), -1 (scalable)
//   - compatible_with_x86() : Compat-Constraint zur Haupt-ISA
//   - compatible_with_arm() : "
//   - compatible_with_riscv() : "
//   - compatible_with_powerpc() : "

#include "../../concepts/topic_hardware_concept.hpp"
#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::hardware::axis_09b_simd_extension::concepts {

template <typename E>
concept SimdExtensionStrategy =
    ::comdare::cache_engine::hardware::concepts::HardwareComponent<E>
    && requires {
        { E::is_active() }              noexcept -> std::convertible_to<bool>;
        { E::vector_width_bits() }      noexcept -> std::convertible_to<int>;
        { E::compatible_with_x86() }    noexcept -> std::convertible_to<bool>;
        { E::compatible_with_arm() }    noexcept -> std::convertible_to<bool>;
        { E::compatible_with_riscv() }  noexcept -> std::convertible_to<bool>;
        { E::compatible_with_powerpc() } noexcept -> std::convertible_to<bool>;
    };

}  // namespace
