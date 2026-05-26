#pragma once
// V41.F.6.1.R3 — Bekannte Algorithmen als SearchAlgorithmAnatomy-Instantiationen
//
// Die 6 bekannten Suchalgorithmen sind reine Template-Spezialisierungen der
// zentralen Anatomie-Klasse. KEIN eigener Algorithmus-Code — nur Komposition
// aus 17 Achsen-Auspraegungen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§13

#include "search_algorithm_anatomy.hpp"

#include "../compositions/art_reference.hpp"
#include "../compositions/hot_reference.hpp"
#include "../compositions/wormhole_reference.hpp"
#include "../compositions/surf_reference.hpp"
#include "../compositions/start_reference.hpp"
#include "../compositions/masstree_reference.hpp"

namespace comdare::cache_engine::anatomy {

/// 6 bekannte Tiere — alle nutzen DIESELBE Anatomie-Klasse, unterscheiden sich
/// nur durch die 17 Achsen-Auspraegungen ihrer Composition.

using Art      = SearchAlgorithmAnatomy<compositions::ArtComposition>;
using Hot      = SearchAlgorithmAnatomy<compositions::HotComposition>;
using Wormhole = SearchAlgorithmAnatomy<compositions::WormholeComposition>;
using SuRF     = SearchAlgorithmAnatomy<compositions::SurfComposition>;
using Masstree = SearchAlgorithmAnatomy<compositions::MasstreeComposition>;
using Start    = SearchAlgorithmAnatomy<compositions::StartComposition>;

}  // namespace comdare::cache_engine::anatomy
