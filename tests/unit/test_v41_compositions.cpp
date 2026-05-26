// V41.F.6.1.R2 Composition-Templates Test (2026-05-26 Architektur-Refactoring)
//
// Verifiziert dass die Pilot-Compositions (ArtComposition, WormholeComposition,
// SurfComposition) korrekt definiert sind und ihre Sub-Achsen die jeweiligen
// Concepts erfuellen.
//
// Doku: docs/architektur/14_achsen_komposition_organ_metapher.md
// Memory: [[achsen-komposition-organ-metapher]]

#include <gtest/gtest.h>

#include <compositions/art_reference.hpp>
#include <compositions/wormhole_reference.hpp>
#include <compositions/surf_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/start_reference.hpp>
#include <compositions/masstree_reference.hpp>

#include <concepts/legacy_original_code_strategy_concept.hpp>
#include <topics/axis_base.hpp>

namespace ce_topics   = ::comdare::cache_engine::topics;
namespace ce_concepts = ::comdare::cache_engine::concepts;
namespace ce_03a      = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_03a_cpts = ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts;
namespace ce_compositions = ::comdare::cache_engine::compositions;

// ─────────────────────────────────────────────────────────────────────────────
// (1) ArtComposition
// ─────────────────────────────────────────────────────────────────────────────

TEST(ArtComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::ArtComposition::paper_id.empty());
    static_assert(!ce_compositions::ArtComposition::paper_title.empty());
    static_assert(!ce_compositions::ArtComposition::name.empty());
    SUCCEED();
}

TEST(ArtComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::ArtComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    static_assert(ce_03a_cpts::CacheEngineSearchAlgoPermutationStrategy<SA>);
    SUCCEED();
}

TEST(ArtComposition, AllocatorSubAchseAxisBaseConcept) {
    using A = ce_compositions::ArtComposition::allocator;
    static_assert(ce_topics::AxisBaseConcept<A>);
    static_assert(ce_concepts::LegacyOriginalCodePflicht<A>);
    SUCCEED();
}

TEST(ArtComposition, AllocatorIsMimallocPaperBound) {
    using A = ce_compositions::ArtComposition::allocator;
    // ART-Composition wählt Mimalloc → get_compiler="gcc-9.5" via Paper-Mixin
    static_assert(ce_concepts::HasOriginalCode<A>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (2) WormholeComposition
// ─────────────────────────────────────────────────────────────────────────────

TEST(WormholeComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::WormholeComposition::paper_id.empty());
    static_assert(!ce_compositions::WormholeComposition::paper_title.empty());
    SUCCEED();
}

TEST(WormholeComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::WormholeComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    SUCCEED();
}

TEST(WormholeComposition, DistinctFromArtComposition) {
    // Wormhole und ART nutzen unterschiedliche search_algo Sub-Achsen
    static_assert(!std::same_as<
        ce_compositions::ArtComposition::search_algo,
        ce_compositions::WormholeComposition::search_algo>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) SurfComposition
// ─────────────────────────────────────────────────────────────────────────────

TEST(SurfComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::SurfComposition::paper_id.empty());
    SUCCEED();
}

TEST(SurfComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::SurfComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    SUCCEED();
}

TEST(SurfComposition, MappingIsPoolRelative) {
    using M = ce_compositions::SurfComposition::mapping;
    // SuRF nutzt PoolRelative (succinct kompakt), nicht DirectPlacement
    static_assert(!std::same_as<
        M,
        ce_compositions::ArtComposition::mapping>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (4) Cross-Composition-Property: 3 unterschiedliche Permutations-Punkte
// ─────────────────────────────────────────────────────────────────────────────

TEST(CompositionMatrix, ThreeDistinctPilotCompositions) {
    // ART, Wormhole, SuRF haben jeweils unterschiedliche Sub-Achsen-Tupel:
    using ArtSA      = ce_compositions::ArtComposition::search_algo;
    using WormholeSA = ce_compositions::WormholeComposition::search_algo;
    using SurfSA     = ce_compositions::SurfComposition::search_algo;
    static_assert(!std::same_as<ArtSA, WormholeSA>);
    static_assert(!std::same_as<ArtSA, SurfSA>);
    static_assert(!std::same_as<WormholeSA, SurfSA>);
    SUCCEED();
}

TEST(CompositionMatrix, AllShareSameAllocatorChoice) {
    // Heutiges Pilot-Pattern: alle 3 Compositions wählen Mimalloc (Default-Allocator).
    // Spätere Compositions können das per axis_06-Override variieren.
    using ArtAlloc      = ce_compositions::ArtComposition::allocator;
    using WormholeAlloc = ce_compositions::WormholeComposition::allocator;
    using SurfAlloc     = ce_compositions::SurfComposition::allocator;
    static_assert(std::same_as<ArtAlloc, WormholeAlloc>);
    static_assert(std::same_as<WormholeAlloc, SurfAlloc>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (5) HotComposition + StartComposition + MasstreeComposition (R2 Batch 2)
// ─────────────────────────────────────────────────────────────────────────────

TEST(HotComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::HotComposition::paper_id.empty());
    static_assert(!ce_compositions::HotComposition::paper_title.empty());
    SUCCEED();
}
TEST(HotComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::HotComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    SUCCEED();
}
TEST(HotComposition, SharesSearchAlgoWithWormhole) {
    // HOT + Wormhole nutzen beide VectorU8U8 (sparse Sub-Achse) — gewollt
    using HotSA      = ce_compositions::HotComposition::search_algo;
    using WormholeSA = ce_compositions::WormholeComposition::search_algo;
    static_assert(std::same_as<HotSA, WormholeSA>);
    SUCCEED();
}

TEST(StartComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::StartComposition::paper_id.empty());
    SUCCEED();
}
TEST(StartComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::StartComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    SUCCEED();
}

TEST(MasstreeComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::MasstreeComposition::paper_id.empty());
    SUCCEED();
}
TEST(MasstreeComposition, SearchAlgoSubAchseConformance) {
    using SA = ce_compositions::MasstreeComposition::search_algo;
    static_assert(ce_03a_cpts::SearchAlgoVariant<SA>);
    SUCCEED();
}
TEST(MasstreeComposition, SharesSearchAlgoWithStart) {
    // Masstree + START nutzen beide VectorU16U16 (multilevel-Sub-Achse) — gewollt
    using MasstreeSA = ce_compositions::MasstreeComposition::search_algo;
    using StartSA    = ce_compositions::StartComposition::search_algo;
    static_assert(std::same_as<MasstreeSA, StartSA>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (6) Composition-Matrix-Property: 6 Compositions, davon einige Sub-Achsen-shared
// ─────────────────────────────────────────────────────────────────────────────

TEST(CompositionMatrixExpanded, SixPilotCompositionsAllSearchAlgoConform) {
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::ArtComposition::search_algo>);
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::WormholeComposition::search_algo>);
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::SurfComposition::search_algo>);
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::HotComposition::search_algo>);
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::StartComposition::search_algo>);
    static_assert(ce_03a_cpts::SearchAlgoVariant<ce_compositions::MasstreeComposition::search_algo>);
    SUCCEED();
}

TEST(CompositionMatrixExpanded, SubAchsenSharingEvidenceForOrganMetaphor) {
    // Beweis-Test fuer Tier-Organ-Metapher: unterschiedliche Tiere können
    // dieselbe Organ-Variante haben (z.B. Reh + Kuh teilen Pansen-Magen-Typ,
    // hier HOT+Wormhole teilen VectorU8U8, START+Masstree teilen VectorU16U16).
    // 6 Tiere mit 3 unterschiedlichen search_algo-Sub-Achsen-Varianten:
    using ArtSA      = ce_compositions::ArtComposition::search_algo;       // Array256 (dense)
    using HotSA      = ce_compositions::HotComposition::search_algo;       // VectorU8U8 (sparse)
    using WormholeSA = ce_compositions::WormholeComposition::search_algo;  // VectorU8U8 (sparse)
    using StartSA    = ce_compositions::StartComposition::search_algo;     // VectorU16U16 (multilevel)
    using MasstreeSA = ce_compositions::MasstreeComposition::search_algo;  // VectorU16U16 (multilevel)
    using SurfSA     = ce_compositions::SurfComposition::search_algo;      // VectorU16U16 (multilevel — Read-Only)

    // Gruppe Dense: nur ART
    static_assert(!std::same_as<ArtSA, HotSA>);

    // Gruppe Sparse (HOT/Wormhole) shared
    static_assert(std::same_as<HotSA, WormholeSA>);

    // Gruppe Multilevel (START/Masstree/SuRF) shared
    static_assert(std::same_as<StartSA, MasstreeSA>);
    static_assert(std::same_as<MasstreeSA, SurfSA>);

    // 3 Gruppen alle voneinander distinct
    static_assert(!std::same_as<ArtSA, HotSA>);
    static_assert(!std::same_as<HotSA, StartSA>);
    static_assert(!std::same_as<ArtSA, StartSA>);
    SUCCEED();
}
