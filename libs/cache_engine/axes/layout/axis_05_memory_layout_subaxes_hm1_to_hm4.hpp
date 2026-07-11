#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout Subaxes-Tags
//
// @topic memory_layout
// @achse 05
//
// 3 orthogonale Sub-Klassifizierungen (Subaxes) der Memory-Layout-Achse (HM1-HM3).
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

// HM4 (Stride-Pattern) war als vierte Subaxis geplant, wurde aber nie durch einen Wrapper getragen
// (kein `axis_tag` referenziert sie — repo-weit null Nutzung, Achsen-Ontologie-Verifikation 2026-07-11) →
// toter Tag entfernt (kein Waisen-Slot). Ein permutierbarer Stride-*Wert* wuerde AllLayouts (die 5
// golden-320-Werte) / Gate-1 brechen; die IMC-Runtime-Heuristik bleibt compile-time-Doku-Backlog
// (docs/architecture/16_axis_05_imc_runtime_heuristik.md).

} // namespace comdare::cache_engine::layout::subaxes
