#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout Subaxes-Tags
//
// @topic memory_layout
// @achse 05
//
// 4 orthogonale Sub-Klassifizierungen (Subaxes) der Memory-Layout-Achse.
// Wrappers deklarieren ihren `axis_tag = subaxes::<one of these>_tag` und
// die CacheEngineBuilder kann ueber gleichzeitige Subaxes-Varianten
// permutieren.

namespace comdare::cache_engine::layout::subaxes {

// HM1: Alignment-Strategie (cache-line-aligned vs strict-packed)
struct alignment_strategy_tag {};

// HM2: Daten-Organisation (AoS vs SoA vs AoSoA)
struct data_organization_tag {};

// HM3: Packing-Dichte (cache-line vs bit-packed succinct)
struct packing_density_tag {};

// HM4: Stride-Pattern (sequential vs interleaved access)
struct stride_pattern_tag {};

}  // namespace
