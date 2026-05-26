// V41.F.6.1.R3 — SearchAlgorithmAnatomy Smoke-Tests (Saeugetier-Anatomie)
//
// Beweist:
// 1. Alle 6 Compositions erfuellen IsComposition Concept
// 2. SearchAlgorithmAnatomy<C> instantiiert sich fuer alle 6 Compositions
// 3. Pflicht-API insert/lookup/erase/clear funktioniert (Pilot-Pipeline)
// 4. Composition-Inspection via composition_name()/paper_id()/organ_count()
// 5. Tier-Organ-Beweis: alle 6 Tiere nutzen IDENTISCHE Anatomie-Klasse
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @task #694

#include <gtest/gtest.h>

#include <anatomy/composition_concept.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/known_algorithms.hpp>

#include <type_traits>

namespace ana       = ::comdare::cache_engine::anatomy;
namespace ce_compos = ::comdare::cache_engine::compositions;

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
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_Instantiation, AllSixAlgorithmsInstantiate) {
    ana::Art art;
    ana::Hot hot;
    ana::Wormhole wh;
    ana::SuRF surf;
    ana::Masstree mt;
    ana::Start st;
    EXPECT_TRUE(art.empty());
    EXPECT_TRUE(hot.empty());
    EXPECT_TRUE(wh.empty());
    EXPECT_TRUE(surf.empty());
    EXPECT_TRUE(mt.empty());
    EXPECT_TRUE(st.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Pflicht-API Smoke-Tests (insert/lookup/erase/clear)
// ─────────────────────────────────────────────────────────────────────────────

template <typename Algo>
class AnatomyPilotApi : public ::testing::Test {};

using AllSixAlgos = ::testing::Types<
    ana::Art, ana::Hot, ana::Wormhole, ana::SuRF, ana::Masstree, ana::Start
>;
TYPED_TEST_SUITE(AnatomyPilotApi, AllSixAlgos);

TYPED_TEST(AnatomyPilotApi, InsertLookupEraseClearRoundtrip) {
    TypeParam algo;
    EXPECT_EQ(algo.size(), 0u);

    // Insert 3 Eintraege
    EXPECT_TRUE(algo.insert(1, 100));
    EXPECT_TRUE(algo.insert(2, 200));
    EXPECT_TRUE(algo.insert(3, 300));
    EXPECT_EQ(algo.size(), 3u);

    // Lookup
    auto v1 = algo.lookup(1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 100u);
    auto v2 = algo.lookup(2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, 200u);
    EXPECT_FALSE(algo.lookup(999).has_value());

    // Re-Insert ueberschreibt (insert_or_assign liefert false fuer Update)
    EXPECT_FALSE(algo.insert(1, 111));
    EXPECT_EQ(*algo.lookup(1), 111u);

    // Erase
    EXPECT_TRUE(algo.erase(2));
    EXPECT_FALSE(algo.erase(999));
    EXPECT_EQ(algo.size(), 2u);

    // Clear
    algo.clear();
    EXPECT_EQ(algo.size(), 0u);
    EXPECT_TRUE(algo.empty());
}

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
    Frankenstein f;
    EXPECT_TRUE(f.insert(42, 4242));
    EXPECT_EQ(*f.lookup(42), 4242u);
    // Identitaet: neues Tier, identische Anatomie
    static_assert(Frankenstein::composition_name() == std::string_view{"FrankensteinComposition"});
    static_assert(Frankenstein::organ_count() == 17);
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — R3.2 PaperBinding-Compositions (Audit-Korrektur Promotion statt Deprecation)
//      Beweis: OriginalXxx-Wrappers sind alternative search_algo-Achsen-Werte,
//      austauschbar in der Anatomie-Klasse.
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
    ana::ArtPaperBinding      art_pb;
    ana::HotPaperBinding      hot_pb;
    ana::StartPaperBinding    start_pb;
    ana::WormholePaperBinding wh_pb;
    ana::SurfPaperBinding     surf_pb;
    EXPECT_TRUE(art_pb.empty());
    EXPECT_TRUE(hot_pb.empty());
    EXPECT_TRUE(start_pb.empty());
    EXPECT_TRUE(wh_pb.empty());
    EXPECT_TRUE(surf_pb.empty());
}

template <typename Algo>
class PaperBindingPilotApi : public ::testing::Test {};

using AllFivePaperBindings = ::testing::Types<
    ana::ArtPaperBinding, ana::HotPaperBinding, ana::StartPaperBinding,
    ana::WormholePaperBinding, ana::SurfPaperBinding
>;
TYPED_TEST_SUITE(PaperBindingPilotApi, AllFivePaperBindings);

TYPED_TEST(PaperBindingPilotApi, InsertLookupEraseClearRoundtrip) {
    TypeParam algo;
    EXPECT_TRUE(algo.insert(7, 77));
    EXPECT_TRUE(algo.insert(8, 88));
    auto v = algo.lookup(7);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 77u);
    EXPECT_TRUE(algo.erase(7));
    EXPECT_EQ(algo.size(), 1u);
    algo.clear();
    EXPECT_TRUE(algo.empty());
}

TEST(AnatomyR3_2_PaperBinding, NameAndPaperIdMarkedAsPaperBinding) {
    static_assert(ana::ArtPaperBinding::composition_name()      == std::string_view{"ArtPaperBindingComposition"});
    static_assert(ana::HotPaperBinding::composition_name()      == std::string_view{"HotPaperBindingComposition"});
    static_assert(ana::StartPaperBinding::composition_name()    == std::string_view{"StartPaperBindingComposition"});
    static_assert(ana::WormholePaperBinding::composition_name() == std::string_view{"WormholePaperBindingComposition"});
    static_assert(ana::SurfPaperBinding::composition_name()     == std::string_view{"SurfPaperBindingComposition"});
    // paper_id enthaelt "Paper-Binding" Marker
    static_assert(ana::ArtPaperBinding::paper_id().find("Paper-Binding") != std::string_view::npos);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — Beweis: Art vs ArtPaperBinding tauschen NUR search_algo aus
//      (Promotion-Statement der Audit-Korrektur)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR3_2_Promotion, ArtVsArtPaperBindingDifferOnlyInSearchAlgo) {
    using A  = ce_compos::ArtComposition;
    using AP = ce_compos::ArtPaperBindingComposition;
    // search_algo unterschiedlich:
    static_assert(!std::is_same_v<A::search_algo, AP::search_algo>);
    // 16 weitere Achsen identisch:
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
    // 6 CE-Re-Impl + 5 PaperBinding = 11 Algorithmen
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
