#pragma once
// V41.F.6.1.R7.5.i axis_09_isa Subaxes-Tags (Instruction Set Architecture)

namespace comdare::cache_engine::hardware::axis_09_isa::subaxes {

// IS1: SIMD-Width (none / 128 / 256 / 512 bit)
struct simd_width_tag {};

// IS2: Vector-Arch (x86 / arm / scalar)
struct vector_arch_tag {};

// IS3: Alignment (scalar / vector-aligned)
struct alignment_tag {};

}  // namespace
