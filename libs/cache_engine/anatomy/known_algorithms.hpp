#pragma once
// V41.F.6.1.R3 — Bekannte Algorithmen als SearchAlgorithmAnatomy-Instantiationen
//
// Die 6 bekannten Suchalgorithmen sind reine Template-Spezialisierungen der
// zentralen Anatomie-Klasse. KEIN eigener Algorithmus-Code — nur Komposition
// aus 17 Achsen-Auspraegungen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§13

#include "search_algorithm_anatomy.hpp"

// V41.F.6.1.R3 — 6 CE-Re-Impl Compositions (Stufe-A Defaults)
#include "../compositions/art_reference.hpp"
#include "../compositions/hot_reference.hpp"
#include "../compositions/wormhole_reference.hpp"
#include "../compositions/surf_reference.hpp"
#include "../compositions/start_reference.hpp"
#include "../compositions/masstree_reference.hpp"

// V41.F.6.1.R3.2 — 5 Paper-Binding Compositions (Habich-SHA256, OriginalXxx-Wrapper)
#include "../compositions/art_paper_binding_reference.hpp"
#include "../compositions/hot_paper_binding_reference.hpp"
#include "../compositions/start_paper_binding_reference.hpp"
#include "../compositions/wormhole_paper_binding_reference.hpp"
#include "../compositions/surf_paper_binding_reference.hpp"

namespace comdare::cache_engine::anatomy {

/// 6 CE-Re-Impl Tiere — Stufe-A Defaults (search_algo = Array256SearchAlgo/VectorU8U8SearchAlgo/VectorU16U16SearchAlgo).
/// Alle nutzen DIESELBE Anatomie-Klasse, unterscheiden sich nur durch 17 Achsen-Auspraegungen.

using Art      = SearchAlgorithmAnatomy<compositions::ArtComposition>;
using Hot      = SearchAlgorithmAnatomy<compositions::HotComposition>;
using Wormhole = SearchAlgorithmAnatomy<compositions::WormholeComposition>;
using SuRF     = SearchAlgorithmAnatomy<compositions::SurfComposition>;
using Masstree = SearchAlgorithmAnatomy<compositions::MasstreeComposition>;
using Start    = SearchAlgorithmAnatomy<compositions::StartComposition>;

/// 5 Paper-Binding Tiere — Habich-konform (search_algo = OriginalXxxSearchAlgo S04-S08).
/// Beweis: identische Anatomie, alternativer search_algo-Wert → austauschbare Tier-Variante.
/// Audit-Korrektur 2026-05-26 spät: OriginalXxx sind keine monolithischen Algorithmen,
/// sondern legitime Achsen-Werte. R3.2 ist Promotion, nicht Deprecation.

using ArtPaperBinding      = SearchAlgorithmAnatomy<compositions::ArtPaperBindingComposition>;
using HotPaperBinding      = SearchAlgorithmAnatomy<compositions::HotPaperBindingComposition>;
using StartPaperBinding    = SearchAlgorithmAnatomy<compositions::StartPaperBindingComposition>;
using WormholePaperBinding = SearchAlgorithmAnatomy<compositions::WormholePaperBindingComposition>;
using SurfPaperBinding     = SearchAlgorithmAnatomy<compositions::SurfPaperBindingComposition>;

}  // namespace comdare::cache_engine::anatomy
