// V41.F.6.1.R3 + R3.2 + R5.B — SearchAlgorithmAnatomy Tests
//
// **R5.B Refactoring 2026-05-26 sehr spät:** Container-Operationen
// (insert/lookup/erase/clear/size/empty) wurden aus der Anatomie entfernt.
// Diese Operationen sind jetzt in `builder::anatomy_commands::AnatomyExecutionContext<C>`.
//
// **Diese Datei testet daher nur:**
// - Composition-Conformance (IsComposition)
// - Anatomie-Instantiation (default-konstruierbar)
// - Composition-Inspection (composition_name/paper_id/organ_count)
// - Tier-Organ-Beweis (alle 11 Algos nutzen DIESELBE Anatomie-Klasse)
// - Frankenstein-Demo (AdHoc-Composition instantiiert)
//
// Container-Roundtrip-Tests sind in `test_v41_builder_anatomy_commands.cpp`.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §17.2 + §24
// @task #694 R3 + #695 R3.2 + #698 R5.B

#include <gtest/gtest.h>

#include <anatomy/composition_concept.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/known_algorithms.hpp>

#include <type_traits>

namespace ana       = ::comdare::cache_engine::anatomy;
namespace ce_compos = ::comdare::cache_engine::compositions;

// #42: robuste paper_source-Detektion (MSVC hart-fehlert bei `requires { typename T::x; }` fuer fehlende x).
template <class T, class = void> constexpr bool has_paper_source_v = false;
template <class T> constexpr bool has_paper_source_v<T, std::void_t<typename T::paper_source>> = true;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — IsComposition Concept-Conformance fuer alle 6 bekannten Compositions
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_Concept, AllSixCompositionsConform) {
    static_assert(ana::IsComposition<ce_compos::ArtComposition>);
    static_assert(ana::IsComposition<ce_compos::HotComposition>);
    static_assert(ana::IsComposition<ce_compos::WormholeComposition>);
    static_assert(ana::IsComposition<ce_compos::SurfComposition>);
    static_assert(ana::IsComposition<ce_compos::MasstreeComposition>);
    static_assert(ana::IsComposition<ce_compos::StartComposition>);
    SUCCEED();
}

TEST(AnatomyR3_Concept, OrganCountIsSeventeenForAllCompositions) {
    static_assert(ana::composition_organ_count<ce_compos::ArtComposition>::value == 17);
    static_assert(ana::composition_organ_count<ce_compos::HotComposition>::value == 17);
    static_assert(ana::composition_organ_count<ce_compos::WormholeComposition>::value == 17);
    static_assert(ana::composition_organ_count<ce_compos::SurfComposition>::value == 17);
    static_assert(ana::composition_organ_count<ce_compos::MasstreeComposition>::value == 17);
    static_assert(ana::composition_organ_count<ce_compos::StartComposition>::value == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — SearchAlgorithmAnatomy Instantiation fuer alle 6 Compositions
// (R5.B: kein .empty() mehr — Anatomie hat keinen Container mehr)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_Instantiation, AllSixAlgorithmsInstantiate) {
    [[maybe_unused]] ana::Art art;
    [[maybe_unused]] ana::Hot hot;
    [[maybe_unused]] ana::Wormhole wh;
    [[maybe_unused]] ana::SuRF surf;
    [[maybe_unused]] ana::Masstree mt;
    [[maybe_unused]] ana::Start st;
    SUCCEED();  // Compile-Time-Beweis: alle 6 Anatomien default-konstruierbar
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Container-Roundtrip-Tests entfernt — siehe test_v41_builder_anatomy_commands.cpp
// (R5.B: insert/lookup/erase/clear/size/empty in Builder-Commands verschoben)
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Composition-Inspection (composition_name/paper_id/organ_count statisch)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_Inspection, CompositionNameAndPaperIdMatch) {
    static_assert(ana::Art::composition_name() == std::string_view{"ArtComposition"});
    static_assert(ana::Hot::composition_name() == std::string_view{"HotComposition"});
    static_assert(ana::Wormhole::composition_name() == std::string_view{"WormholeComposition"});
    static_assert(ana::SuRF::composition_name() == std::string_view{"SurfComposition"});
    static_assert(ana::Masstree::composition_name() == std::string_view{"MasstreeComposition"});
    static_assert(ana::Start::composition_name() == std::string_view{"StartComposition"});

    static_assert(ana::Art::paper_id().starts_with("P01"));
    static_assert(ana::Hot::paper_id().starts_with("P02"));
    static_assert(ana::Masstree::paper_id().starts_with("P03"));
    static_assert(ana::Start::paper_id().starts_with("P05"));
    static_assert(ana::SuRF::paper_id().starts_with("P10"));
    SUCCEED();
}

TEST(AnatomyR3_Inspection, AllSixAlgosHaveSeventeenOrgans) {
    static_assert(ana::Art::organ_count()      == 17);
    static_assert(ana::Hot::organ_count()      == 17);
    static_assert(ana::Wormhole::organ_count() == 17);
    static_assert(ana::SuRF::organ_count()     == 17);
    static_assert(ana::Masstree::organ_count() == 17);
    static_assert(ana::Start::organ_count()    == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Tier-Organ-Beweis: alle 6 Tiere nutzen DIESELBE Anatomie-Klasse-Template
//      → Saeugetier-Anatomie-Metapher technisch verifiziert
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_MammalProof, AllSixUseSameAnatomyTemplate) {
    // Alle 6 sind Spezialisierungen von SearchAlgorithmAnatomy<...>
    static_assert(std::is_same_v<
        typename ana::Art::composition_t, ce_compos::ArtComposition>);
    static_assert(std::is_same_v<
        typename ana::Hot::composition_t, ce_compos::HotComposition>);
    static_assert(std::is_same_v<
        typename ana::Wormhole::composition_t, ce_compos::WormholeComposition>);
    static_assert(std::is_same_v<
        typename ana::SuRF::composition_t, ce_compos::SurfComposition>);
    static_assert(std::is_same_v<
        typename ana::Masstree::composition_t, ce_compos::MasstreeComposition>);
    static_assert(std::is_same_v<
        typename ana::Start::composition_t, ce_compos::StartComposition>);

    // Alle 6 haben IDENTISCHE key_type/value_type (Anatomie-Pflicht)
    static_assert(std::is_same_v<typename ana::Art::key_type,      typename ana::Hot::key_type>);
    static_assert(std::is_same_v<typename ana::Hot::key_type,      typename ana::Wormhole::key_type>);
    static_assert(std::is_same_v<typename ana::Wormhole::key_type, typename ana::SuRF::key_type>);
    static_assert(std::is_same_v<typename ana::SuRF::key_type,     typename ana::Masstree::key_type>);
    static_assert(std::is_same_v<typename ana::Masstree::key_type, typename ana::Start::key_type>);
    SUCCEED();
}

TEST(AnatomyR3_MammalProof, DifferentTiereDifferentCompositionTypes) {
    // Saeugetier-Beweis: Tiere sind verschieden, Anatomie ist gleich
    static_assert(!std::is_same_v<ana::Art, ana::Hot>);
    static_assert(!std::is_same_v<ana::Hot, ana::Wormhole>);
    static_assert(!std::is_same_v<ana::Wormhole, ana::SuRF>);
    static_assert(!std::is_same_v<ana::SuRF, ana::Masstree>);
    static_assert(!std::is_same_v<ana::Masstree, ana::Start>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Forschungs-Mission: AdHoc-Composition (Frankenstein-Tier)
//      Demonstration: NEUE Permutation = neues Tier, dieselbe Anatomie
// ─────────────────────────────────────────────────────────────────────────────

struct FrankensteinComposition {
    using search_algo        = ce_compos::ArtComposition::search_algo;
    using cache_traversal    = ce_compos::WormholeComposition::cache_traversal;  // Hash von Wormhole
    using mapping            = ce_compos::SurfComposition::mapping;              // PoolRelative von SuRF
    using path_compression   = ce_compos::ArtComposition::path_compression;
    using node_type          = ce_compos::ArtComposition::node_type;
    using memory_layout      = ce_compos::ArtComposition::memory_layout;
    using allocator          = ce_compos::ArtComposition::allocator;
    using prefetch           = ce_compos::ArtComposition::prefetch;
    using concurrency        = ce_compos::ArtComposition::concurrency;
    using serialization      = ce_compos::ArtComposition::serialization;
    using telemetry          = ce_compos::ArtComposition::telemetry;
    using value_handle       = ce_compos::ArtComposition::value_handle;
    using isa                = ce_compos::ArtComposition::isa;
    using index_organization = ce_compos::ArtComposition::index_organization;
    using io_dispatch        = ce_compos::ArtComposition::io_dispatch;
    using migration_policy   = ce_compos::ArtComposition::migration_policy;
    using filter             = ce_compos::ArtComposition::filter;
    static constexpr std::string_view name     = "FrankensteinComposition";
    static constexpr std::string_view paper_id = "P00 AdHoc Frankenstein 2026";
};

TEST(AnatomyR3_Frankenstein, AdHocCompositionInstantiatesNewTier) {
    static_assert(ana::IsComposition<FrankensteinComposition>);
    using Frankenstein = ana::SearchAlgorithmAnatomy<FrankensteinComposition>;
    [[maybe_unused]] Frankenstein f;
    // Identitaet: neues Tier, identische Anatomie
    static_assert(Frankenstein::composition_name() == std::string_view{"FrankensteinComposition"});
    static_assert(Frankenstein::organ_count() == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — R3.2 PaperBinding-Compositions (Audit-Korrektur Promotion statt Deprecation)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_2_PaperBinding, FivePaperBindingCompositionsConform) {
    static_assert(ana::IsComposition<ce_compos::ArtPaperBindingComposition>);
    static_assert(ana::IsComposition<ce_compos::HotPaperBindingComposition>);
    static_assert(ana::IsComposition<ce_compos::StartPaperBindingComposition>);
    static_assert(ana::IsComposition<ce_compos::WormholePaperBindingComposition>);
    static_assert(ana::IsComposition<ce_compos::SurfPaperBindingComposition>);
    SUCCEED();
}

TEST(AnatomyR3_2_PaperBinding, FivePaperBindingAlgosInstantiate) {
    [[maybe_unused]] ana::ArtPaperBinding      art_pb;
    [[maybe_unused]] ana::HotPaperBinding      hot_pb;
    [[maybe_unused]] ana::StartPaperBinding    start_pb;
    [[maybe_unused]] ana::WormholePaperBinding wh_pb;
    [[maybe_unused]] ana::SurfPaperBinding     surf_pb;
    SUCCEED();
}

TEST(AnatomyR3_2_PaperBinding, NameAndPaperIdMarkedAsPaperBinding) {
    static_assert(ana::ArtPaperBinding::composition_name()      == std::string_view{"ArtPaperBindingComposition"});
    static_assert(ana::HotPaperBinding::composition_name()      == std::string_view{"HotPaperBindingComposition"});
    static_assert(ana::StartPaperBinding::composition_name()    == std::string_view{"StartPaperBindingComposition"});
    static_assert(ana::WormholePaperBinding::composition_name() == std::string_view{"WormholePaperBindingComposition"});
    static_assert(ana::SurfPaperBinding::composition_name()     == std::string_view{"SurfPaperBindingComposition"});
    static_assert(ana::ArtPaperBinding::paper_id().find("Paper-Binding") != std::string_view::npos);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — Beweis (#42 Umstufung-B AKTUALISIERT): Art vs ArtPaperBinding teilen DASSELBE
//      sezierte ART-Organ; sie unterscheiden sich nur noch in der PAPER-PROVENIENZ.
//
//      Vor #42: search_algo war ein Tier (Array256SearchAlgo vs OriginalArtSearchAlgo) — die Varianten
//      unterschieden sich im search_algo-Tier. Nach #42 (Doku 14 §3.1: Achse=Organ): beide Konfiguratoren
//      tragen DASSELBE sezierte ART-Organ (ObservableArtTrieOrgan) im search_algo-Slot. Die Habich-/is_original-
//      Bindung lebt jetzt im separaten paper_source-Slot (Provenienz-Traeger, KEIN Achsen-Wert), den nur die
//      Paper-Binding-Composition besitzt. Das ist die korrekte Ordnung: der Such-Algorithmus ist EIN Organ,
//      die Paper-Bindung ist Metadatum.
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_2_Promotion, ArtVsArtPaperBindingShareOrganDifferInProvenance) {
    using A  = ce_compos::ArtComposition;
    using AP = ce_compos::ArtPaperBindingComposition;
    // #42: BEIDE tragen jetzt dasselbe sezierte ART-Organ (war frueher unterschiedlicher Tier).
    static_assert(std::is_same_v<A::search_algo, AP::search_algo>);
    // Die Unterscheidung wandert in die Provenienz: nur die Paper-Binding hat einen paper_source-Slot.
    static_assert(has_paper_source_v<AP>);
    static_assert(!has_paper_source_v<A>);
    // Alle 17 Achsen (inkl. search_algo) sind nun identisch — der Unterschied ist rein die Provenienz:
    static_assert(std::is_same_v<A::cache_traversal,    AP::cache_traversal>);
    static_assert(std::is_same_v<A::mapping,            AP::mapping>);
    static_assert(std::is_same_v<A::path_compression,   AP::path_compression>);
    static_assert(std::is_same_v<A::node_type,          AP::node_type>);
    static_assert(std::is_same_v<A::memory_layout,      AP::memory_layout>);
    static_assert(std::is_same_v<A::allocator,          AP::allocator>);
    static_assert(std::is_same_v<A::prefetch,           AP::prefetch>);
    static_assert(std::is_same_v<A::concurrency,        AP::concurrency>);
    static_assert(std::is_same_v<A::serialization,      AP::serialization>);
    static_assert(std::is_same_v<A::telemetry,          AP::telemetry>);
    static_assert(std::is_same_v<A::value_handle,       AP::value_handle>);
    static_assert(std::is_same_v<A::isa,                AP::isa>);
    static_assert(std::is_same_v<A::index_organization, AP::index_organization>);
    static_assert(std::is_same_v<A::io_dispatch,        AP::io_dispatch>);
    static_assert(std::is_same_v<A::migration_policy,   AP::migration_policy>);
    static_assert(std::is_same_v<A::filter,             AP::filter>);
    SUCCEED();
}

TEST(AnatomyR3_2_Promotion, ElevenAlgosFromAnatomyOrganCount17) {
    static_assert(ana::Art::organ_count()                 == 17);
    static_assert(ana::Hot::organ_count()                 == 17);
    static_assert(ana::Wormhole::organ_count()            == 17);
    static_assert(ana::SuRF::organ_count()                == 17);
    static_assert(ana::Masstree::organ_count()            == 17);
    static_assert(ana::Start::organ_count()               == 17);
    static_assert(ana::ArtPaperBinding::organ_count()     == 17);
    static_assert(ana::HotPaperBinding::organ_count()     == 17);
    static_assert(ana::StartPaperBinding::organ_count()   == 17);
    static_assert(ana::WormholePaperBinding::organ_count() == 17);
    static_assert(ana::SurfPaperBinding::organ_count()    == 17);
    SUCCEED();
}
