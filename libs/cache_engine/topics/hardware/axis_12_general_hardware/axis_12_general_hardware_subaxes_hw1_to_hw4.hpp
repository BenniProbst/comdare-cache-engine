#pragma once
// V41.F.6.1.R7.1.a.2 Sub-Achsen-Tags HW1-HW4 fuer Achse 12 GeneralHardware
//
// @topic hardware
// @achse 12
//
// Pflicht-Klassifikation jeder Plattform-Familie nach Goldstandard-Pattern
// (analog axis_06_allocator AA1-AA7). Die 4 Sub-Achsen HW1-HW4 stehen
// ORTHOGONAL zueinander — eine Plattform-Familie kann ihre primaere
// Charakteristik genau einem Tag zuordnen (typename axis_tag).
//
// Permutations-Konsequenz: der CacheEngineBuilder kann Plattformen nach
// Sub-Achsen gruppieren, um cross-orthogonale Permutationen zu erzeugen
// (z.B. Sub-Achse HW1 cpu_family x HW2 simd_capability ueber alle Plattformen).
//
// Verwendet in:
//   topics/hardware/axis_12_general_hardware/concepts/
//     axis_12_general_hardware_cache_engine_permutation_concept.hpp
//   (CacheEnginePermutationStrategy verlangt `typename axis_tag`)

namespace comdare::cache_engine::hardware::axis_12_general_hardware::subaxes {

/// HW1 cpu_family — CPU-Architektur-Familie
/// Beispiele: x86_64 (Intel/AMD), aarch64 (ARM/Apple), riscv64 (RISC-V),
/// generic (plattform-agnostisch).
struct cpu_family_tag {};

/// HW2 simd_capability — SIMD-Instruction-Set-Family
/// Beispiele: SSE/AVX/AVX2/AVX-512 (x86), NEON/SVE (ARM), RVV (RISC-V),
/// scalar (kein SIMD).
struct simd_capability_tag {};

/// HW3 memory_topology — NUMA + Cache-Hierarchie
/// Beispiele: UMA-single-socket, NUMA-multi-socket, Apple-unified-memory,
/// HBM-on-package (Sapphire Rapids HBM, Grace Hopper), CXL-attached.
struct memory_topology_tag {};

/// HW4 page_topology — Memory-Page-Konfiguration (Standard + HugePage)
/// Beispiele: 4K-standard (POSIX), 16K-apple-silicon, 2M-hugepage (x86),
/// 64K-arm-server, 1G-gigantic-hugepage.
struct page_topology_tag {};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware::subaxes
