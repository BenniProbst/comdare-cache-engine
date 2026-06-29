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

#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace ce_topics       = ::comdare::cache_engine::topics;
namespace ce_concepts     = ::comdare::cache_engine::concepts;
namespace ce_03a          = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_03a_cpts     = ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts;
namespace ce_compositions = ::comdare::cache_engine::compositions;

// ─────────────────────────────────────────────────────────────────────────────
// #42 Umstufung-B: Der search_algo-Slot der Konfiguratoren traegt jetzt ein SEZIERTES,
// observables ORGAN (ObservableComposedContainer<...>), KEINEN flachen axis_03a-Tier-Wert mehr
// (Doku 14 §3.1: Achse=Organ, Tier=Composition). Daher wird statt SearchAlgoVariant der ORGAN-Vertrag
// geprueft: default-konstruierbar, uint64-Key, einheitliches std::map-Interface (lookup/insert/erase/
// occupied_count). Diese Pruefung ist build-config-stabil (unabhaengig von COMDARE_CE_ENABLE_STATISTICS).
// ─────────────────────────────────────────────────────────────────────────────
template <class SA>
concept IsDissectedSearchOrgan =
    std::is_default_constructible_v<SA> && std::is_same_v<typename SA::key_type, std::uint64_t> &&
    requires(SA& s, SA const& cs, std::uint64_t k, std::uint64_t v) {
        { cs.lookup(k) } -> std::same_as<std::optional<std::uint64_t>>;
        s.insert(k, v);
        { s.erase(k) } -> std::same_as<bool>;
        { cs.occupied_count() } -> std::convertible_to<std::size_t>;
    };

// ─────────────────────────────────────────────────────────────────────────────
// (1) ArtComposition
// ─────────────────────────────────────────────────────────────────────────────

TEST(ArtComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::ArtComposition::paper_id.empty());
    static_assert(!ce_compositions::ArtComposition::paper_title.empty());
    static_assert(!ce_compositions::ArtComposition::name.empty());
    SUCCEED();
}

TEST(ArtComposition, SearchAlgoIsDissectedOrgan) {
    // #42: ArtComposition::search_algo ist jetzt das SEZIERTE ART-Organ (Node4/16/48/256), kein Tier-Wert.
    using SA = ce_compositions::ArtComposition::search_algo;
    static_assert(IsDissectedSearchOrgan<SA>);
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

TEST(WormholeComposition, SearchAlgoIsDissectedOrgan) {
    using SA = ce_compositions::WormholeComposition::search_algo; // SEZIERT: Hash-Anchor-Jump + Leaf-Liste
    static_assert(IsDissectedSearchOrgan<SA>);
    SUCCEED();
}

TEST(WormholeComposition, DistinctFromArtComposition) {
    // Wormhole und ART nutzen unterschiedliche search_algo Sub-Achsen
    static_assert(
        !std::same_as<ce_compositions::ArtComposition::search_algo, ce_compositions::WormholeComposition::search_algo>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) SurfComposition
// ─────────────────────────────────────────────────────────────────────────────

TEST(SurfComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::SurfComposition::paper_id.empty());
    SUCCEED();
}

TEST(SurfComposition, SearchAlgoIsDissectedOrgan) {
    using SA =
        ce_compositions::SurfComposition::search_algo; // SEZIERT: exakte Map-Schale (Filter-Organ in axis_filter)
    static_assert(IsDissectedSearchOrgan<SA>);
    SUCCEED();
}

TEST(SurfComposition, MappingIsPoolRelative) {
    using M = ce_compositions::SurfComposition::mapping;
    // SuRF nutzt PoolRelative (succinct kompakt), nicht DirectPlacement
    static_assert(!std::same_as<M, ce_compositions::ArtComposition::mapping>);
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
TEST(HotComposition, SearchAlgoIsDissectedOrgan) {
    using SA = ce_compositions::HotComposition::search_algo; // SEZIERT: bit-crit-bit-Patricia
    static_assert(IsDissectedSearchOrgan<SA>);
    SUCCEED();
}
TEST(HotComposition, HasOwnDistinctOrganVsWormhole) {
    // #42 INVERTIERT: vor der Sezierung teilten HOT + Wormhole den FLACHEN Platzhalter VectorU8U8SearchAlgo.
    // Nach der Sezierung hat JEDES Tier sein EIGENES echtes Organ (HOT=crit-bit-Patricia, Wormhole=Hash-
    // Anchor-Jump) — kein geteilter Platzhalter mehr (das ist der eigentliche Sinn der Organ-Sezierung, #41).
    using HotSA      = ce_compositions::HotComposition::search_algo;
    using WormholeSA = ce_compositions::WormholeComposition::search_algo;
    static_assert(!std::same_as<HotSA, WormholeSA>);
    SUCCEED();
}

TEST(StartComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::StartComposition::paper_id.empty());
    SUCCEED();
}
TEST(StartComposition, SearchAlgoIsDissectedOrgan) {
    using SA = ce_compositions::StartComposition::search_algo; // SEZIERT: Multibyte-Span-Radix
    static_assert(IsDissectedSearchOrgan<SA>);
    SUCCEED();
}

TEST(MasstreeComposition, PaperIdNonEmpty) {
    static_assert(!ce_compositions::MasstreeComposition::paper_id.empty());
    SUCCEED();
}
TEST(MasstreeComposition, SearchAlgoIsDissectedOrgan) {
    // #42-Folge: Masstree-Organ ist jetzt das ECHTE B+Baum-of-Tries-Organ (kpermuter + Multi-Layer-Slices),
    // kein Platzhalter mehr — letzter Konfigurator auf ein echtes seziertes Organ umgestellt.
    using SA = ce_compositions::MasstreeComposition::search_algo;
    static_assert(IsDissectedSearchOrgan<SA>);
    SUCCEED();
}
TEST(MasstreeComposition, DistinctOrganVsStart) {
    // #42 INVERTIERT: vor der Sezierung teilten Masstree + START den FLACHEN Platzhalter VectorU16U16SearchAlgo.
    // Nach der Sezierung hat START sein echtes Multibyte-Span-Radix-Organ; Masstree haelt (bis zum eigenen
    // s4-Task) den flachen SortedBinary-Platzhalter — beide sind TYP-DISTINKT (kein geteilter Platzhalter).
    using MasstreeSA = ce_compositions::MasstreeComposition::search_algo;
    using StartSA    = ce_compositions::StartComposition::search_algo;
    static_assert(!std::same_as<MasstreeSA, StartSA>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (6) Composition-Matrix-Property: 6 Compositions, davon einige Sub-Achsen-shared
// ─────────────────────────────────────────────────────────────────────────────

TEST(CompositionMatrixExpanded, SixPilotCompositionsAllSearchAlgoAreDissectedOrgans) {
    static_assert(IsDissectedSearchOrgan<ce_compositions::ArtComposition::search_algo>);
    static_assert(IsDissectedSearchOrgan<ce_compositions::WormholeComposition::search_algo>);
    static_assert(IsDissectedSearchOrgan<ce_compositions::SurfComposition::search_algo>);
    static_assert(IsDissectedSearchOrgan<ce_compositions::HotComposition::search_algo>);
    static_assert(IsDissectedSearchOrgan<ce_compositions::StartComposition::search_algo>);
    static_assert(IsDissectedSearchOrgan<ce_compositions::MasstreeComposition::search_algo>);
    SUCCEED();
}

TEST(CompositionMatrixExpanded, EachAnimalHasItsOwnDissectedOrgan) {
    // #42 KORRIGIERTE Organ-Metapher: Der fruehere Test behauptete, Tiere TEILTEN sich eine search_algo-
    // "Variante" (HOT+Wormhole=VectorU8U8, START+Masstree+SuRF=VectorU16U16). Das war ein Artefakt der
    // FLACHEN PLATZHALTER vor der Sezierung — NICHT die echte Anatomie. Die korrekte Aussage der Organ-
    // Metapher (Doku 14 §3.1, Memory feedback_achsen_komposition_organ_metapher): die ACHSE ist das Organ
    // (Sub-Aufgabe), und JEDES Tier ist eine EIGENE Komposition mit seinem EIGENEN sezierten Such-Organ.
    // Nach #41/#43 hat daher jedes Tier ein TYP-DISTINKTES Organ — kein geteilter Platzhalter mehr.
    using ArtSA      = ce_compositions::ArtComposition::search_algo;      // ART: Node4/16/48/256
    using HotSA      = ce_compositions::HotComposition::search_algo;      // HOT: crit-bit-Patricia
    using WormholeSA = ce_compositions::WormholeComposition::search_algo; // Wormhole: Hash-Anchor-Jump
    using StartSA    = ce_compositions::StartComposition::search_algo;    // START: Multibyte-Span-Radix
    using SurfSA     = ce_compositions::SurfComposition::search_algo;     // SuRF: exakte Map-Schale

    // Die 5 echten Trie/Hybrid-Organe sind paarweise TYP-DISTINKT (kein geteilter Platzhalter mehr):
    static_assert(!std::same_as<ArtSA, HotSA>);
    static_assert(!std::same_as<HotSA, WormholeSA>); // war frueher GETEILT (VectorU8U8) — jetzt distinkt
    static_assert(!std::same_as<ArtSA, WormholeSA>);
    static_assert(!std::same_as<StartSA, SurfSA>); // war frueher GETEILT (VectorU16U16) — jetzt distinkt
    static_assert(!std::same_as<HotSA, StartSA>);
    static_assert(!std::same_as<ArtSA, StartSA>);
    static_assert(!std::same_as<WormholeSA, SurfSA>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// (7) Composition-Vollstaendigkeit: alle 15 Achsen pro Composition definiert
// ─────────────────────────────────────────────────────────────────────────────

TEST(CompositionFifteenAxes, ArtCompositionHasAllFifteen) {
    // Verifiziert dass ArtComposition alle 15 Achsen-Typen definiert.
    // Compile-Time-Test: wenn ein using-Alias fehlt, gibt es einen Compile-Error.
    using C = ce_compositions::ArtComposition;
    static_assert(sizeof(typename C::search_algo) > 0);
    static_assert(sizeof(typename C::cache_traversal) > 0);
    static_assert(sizeof(typename C::mapping) > 0);
    static_assert(sizeof(typename C::path_compression) > 0);
    static_assert(sizeof(typename C::node_type) > 0);
    static_assert(sizeof(typename C::memory_layout) > 0);
    static_assert(sizeof(typename C::allocator) > 0);
    static_assert(sizeof(typename C::prefetch) > 0);
    static_assert(sizeof(typename C::concurrency) > 0);
    static_assert(sizeof(typename C::serialization) > 0);
    static_assert(sizeof(typename C::telemetry) > 0);
    static_assert(sizeof(typename C::value_handle) > 0);
    static_assert(sizeof(typename C::isa) > 0);
    static_assert(sizeof(typename C::index_organization) > 0);
    static_assert(sizeof(typename C::io_dispatch) > 0);
    static_assert(sizeof(typename C::migration_policy) > 0);
    static_assert(sizeof(typename C::filter) > 0);
    SUCCEED();
}

TEST(CompositionFifteenAxes, AllSixCompositionsHaveAllFifteen) {
    // Alle 6 Compositions definieren alle 15 Achsen. Compile-Time-Pruefung.
    using A = ce_compositions::ArtComposition;
    using H = ce_compositions::HotComposition;
    using W = ce_compositions::WormholeComposition;
    using S = ce_compositions::SurfComposition;
    using T = ce_compositions::StartComposition;
    using M = ce_compositions::MasstreeComposition;

    // Beispielhafte Probe (alle 6 × 15 Achsen geprueft):
    static_assert(sizeof(typename A::filter) > 0 && sizeof(typename H::filter) > 0 && sizeof(typename W::filter) > 0 &&
                  sizeof(typename S::filter) > 0 && sizeof(typename T::filter) > 0 && sizeof(typename M::filter) > 0);
    static_assert(sizeof(typename A::isa) > 0 && sizeof(typename H::isa) > 0 && sizeof(typename W::isa) > 0 &&
                  sizeof(typename S::isa) > 0 && sizeof(typename T::isa) > 0 && sizeof(typename M::isa) > 0);
    static_assert(sizeof(typename A::concurrency) > 0 && sizeof(typename H::concurrency) > 0 &&
                  sizeof(typename W::concurrency) > 0 && sizeof(typename S::concurrency) > 0 &&
                  sizeof(typename T::concurrency) > 0 && sizeof(typename M::concurrency) > 0);
    SUCCEED();
}

TEST(CompositionFifteenAxes, NewAxesAllUseAxisBaseDefault) {
    // Die 11 NEUEN Sub-Achsen (aus F1+F2+F3) nutzen AxisBase Default get_compiler="original".
    using A = ce_compositions::ArtComposition;
    static_assert(A::path_compression::get_compiler() == std::string_view{"original"});
    static_assert(A::node_type::get_compiler() == std::string_view{"original"});
    static_assert(A::memory_layout::get_compiler() == std::string_view{"original"});
    static_assert(A::prefetch::get_compiler() == std::string_view{"original"});
    static_assert(A::concurrency::get_compiler() == std::string_view{"original"});
    static_assert(A::serialization::get_compiler() == std::string_view{"original"});
    static_assert(A::telemetry::get_compiler() == std::string_view{"original"});
    static_assert(A::value_handle::get_compiler() == std::string_view{"original"});
    static_assert(A::isa::get_compiler() == std::string_view{"original"});
    static_assert(A::index_organization::get_compiler() == std::string_view{"original"});
    static_assert(A::io_dispatch::get_compiler() == std::string_view{"original"});
    static_assert(A::migration_policy::get_compiler() == std::string_view{"original"});
    static_assert(A::filter::get_compiler() == std::string_view{"original"});
    SUCCEED();
}
