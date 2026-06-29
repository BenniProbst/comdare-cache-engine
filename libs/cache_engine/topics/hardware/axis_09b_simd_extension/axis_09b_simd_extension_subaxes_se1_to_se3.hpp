#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension Subaxes-Tags
//
// User-Direktive 2026-05-27: SIMD/Accelerator-Erweiterungen als Sub-Achse
// von axis_09 (Haupt-CPU-ISA). Pro Permutation 0..1 Extension; falls keine,
// dann NoSimdExtension-Wrapper. Compat-Constraint zur Haupt-ISA via
// `compatible_with<MainIsa>()` Compile-Time-Predicate.

namespace comdare::cache_engine::hardware::axis_09b_simd_extension::subaxes {

// SE1: Vector-Width (0=none / 128 / 256 / 512 / scalable)
struct vector_width_tag {};

// SE2: Accelerator-Type (none / simd / matrix / gpu / npu)
struct accelerator_type_tag {};

// SE3: Compat-Family (none / x86 / arm / riscv / cuda)
struct compat_family_tag {};

} // namespace comdare::cache_engine::hardware::axis_09b_simd_extension::subaxes
