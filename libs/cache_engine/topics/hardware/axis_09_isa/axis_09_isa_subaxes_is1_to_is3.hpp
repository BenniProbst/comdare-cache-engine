#pragma once
// V41.F.6.1.R7.5.i.2 axis_09_isa Subaxes-Tags (Haupt-CPU-ISA)
//
// User-Direktive 2026-05-27: axis_09 modelliert die HAUPT-ISA der CPU
// (x86_64 / ARM64 / RISC-V / PowerPC). SIMD-Extensions wie AVX2/NEON/SVE
// gehoeren in axis_09b_simd_extension als Sub-Achse mit Compat-Constraint.

namespace comdare::cache_engine::hardware::axis_09_isa::subaxes {

// IS1: Word-Size (32 / 64 / 128 bit)
struct word_size_tag {};

// IS2: Vendor (intel_amd / arm / open / ibm)
struct vendor_tag {};

// IS3: ZIH-Cluster-Zugehoerigkeit (Barnard / Capella / GraceHopper / N/A)
struct cluster_zih_tag {};

}  // namespace
