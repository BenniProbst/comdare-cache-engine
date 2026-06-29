#pragma once
// V41.F.6.1.R7.5.i.2 axis_09_isa Strategy-Concept (Haupt-CPU-ISA)
//
// User-Direktive 2026-05-27: Pflicht-API fuer Haupt-CPU-ISA-Identifikation:
//   - is_64bit() : Word-Size
//   - cpu_family() : Identifier "x86_64" / "aarch64" / "riscv64" / "ppc64le"
//   - supports_native_simd() : true wenn baseline-SIMD verfuegbar (Compat-Hint
//     fuer axis_09b SIMD-Extensions)

#include <topics/hardware/concepts/topic_hardware_concept.hpp>
#include <concepts>
#include <string_view>

namespace comdare::cache_engine::simd::concepts {

template <typename I>
concept IsaStrategy = ::comdare::cache_engine::hardware::concepts::HardwareComponent<I> && requires {
    { I::is_64bit() } noexcept -> std::convertible_to<bool>;
    { I::cpu_family() } noexcept -> std::convertible_to<std::string_view>;
    { I::supports_native_simd() } noexcept -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::simd::concepts
