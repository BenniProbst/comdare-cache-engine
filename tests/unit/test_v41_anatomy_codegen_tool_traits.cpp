// V41.F.6.1.R5.G — Composition-Location-Traits + descriptor_from_composition<C>() Tests
//
// Beweist:
// 1. HasCompositionLocation Concept Conformance fuer alle 11 Reference-Compositions
// 2. AdHocComposition<...> erfuellt HasCompositionLocation NICHT (negative-Test)
// 3. descriptor_from_composition<C>() liefert korrekte Werte fuer alle 11
// 4. Konsistenz mit hardcoded-Tabelle (R5.F find_composition):
//    descriptor_from_composition<ArtComposition>() == *find_composition("art")
//    (cpp_type_name + header_include identisch)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §49
// @task #711 V41.F.6.1.R5.G

#include <gtest/gtest.h>

#include <anatomy/composition_concept.hpp>
#include <anatomy/composition_factory.hpp>
#include <builder/anatomy_codegen_tool/anatomy_codegen_tool.hpp>

// Alle 11 Reference-Compositions
#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/wormhole_reference.hpp>
#include <compositions/surf_reference.hpp>
#include <compositions/masstree_reference.hpp>
#include <compositions/start_reference.hpp>
#include <compositions/art_paper_binding_reference.hpp>
#include <compositions/hot_paper_binding_reference.hpp>
#include <compositions/start_paper_binding_reference.hpp>
#include <compositions/wormhole_paper_binding_reference.hpp>
#include <compositions/surf_paper_binding_reference.hpp>

#include <string_view>

namespace ana  = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace tool = ::comdare::cache_engine::builder::codegen_tool;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — HasCompositionLocation Concept-Conformance (alle 11)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5G_CompositionLocation, AllSixCeReimplConform) {
    static_assert(ana::HasCompositionLocation<comp::ArtComposition>);
    static_assert(ana::HasCompositionLocation<comp::HotComposition>);
    static_assert(ana::HasCompositionLocation<comp::WormholeComposition>);
    static_assert(ana::HasCompositionLocation<comp::SurfComposition>);
    static_assert(ana::HasCompositionLocation<comp::MasstreeComposition>);
    static_assert(ana::HasCompositionLocation<comp::StartComposition>);
    SUCCEED();
}

TEST(R5G_CompositionLocation, AllFivePaperBindingConform) {
    static_assert(ana::HasCompositionLocation<comp::ArtPaperBindingComposition>);
    static_assert(ana::HasCompositionLocation<comp::HotPaperBindingComposition>);
    static_assert(ana::HasCompositionLocation<comp::StartPaperBindingComposition>);
    static_assert(ana::HasCompositionLocation<comp::WormholePaperBindingComposition>);
    static_assert(ana::HasCompositionLocation<comp::SurfPaperBindingComposition>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — AdHocComposition<...> erfuellt HasCompositionLocation NICHT (Negativ-Test)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5G_CompositionLocation, AdHocCompositionDoesNotConform) {
    // AdHocComposition mit denselben Topic-Achsen wie ArtComposition.
    // Wichtig: AdHoc hat KEIN cpp_type_name/header_include (generisches
    // Cartesian-Element ohne fixe Datei-Lokation) — Concept muss false sein.
    using AdHocArt = ana::AdHocComposition<
        comp::ArtComposition::search_algo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
        comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
        comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
        comp::ArtComposition::serialization, comp::ArtComposition::value_handle,
        comp::ArtComposition::index_organization, comp::ArtComposition::io_dispatch,
        comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
        comp::ArtComposition::queuing_q1,                  // T17 (Doc 30 §8.0)
        comp::ArtComposition::queuing_q2>;                 // T18 (Doc 30 §8.0)
    static_assert(ana::IsComposition<AdHocArt>);           // existing Concept passt
    static_assert(!ana::HasCompositionLocation<AdHocArt>); // R5.G NEU: kein Location
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — descriptor_from_composition<C>() liefert korrekte Werte
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5G_DescriptorFromComposition, ArtCompositionDescriptorIsCorrect) {
    constexpr auto desc = tool::descriptor_from_composition<comp::ArtComposition>();
    static_assert(desc.short_name == std::string_view{"ArtComposition"});
    static_assert(desc.cpp_type_name == std::string_view{"::comdare::cache_engine::compositions::ArtComposition"});
    static_assert(desc.header_include == std::string_view{"compositions/art_reference.hpp"});
    SUCCEED();
}

TEST(R5G_DescriptorFromComposition, HotPaperBindingDescriptorIsCorrect) {
    constexpr auto desc = tool::descriptor_from_composition<comp::HotPaperBindingComposition>();
    static_assert(desc.short_name == std::string_view{"HotPaperBindingComposition"});
    static_assert(desc.cpp_type_name ==
                  std::string_view{"::comdare::cache_engine::compositions::HotPaperBindingComposition"});
    static_assert(desc.header_include == std::string_view{"compositions/hot_paper_binding_reference.hpp"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Konsistenz mit hardcoded-Tabelle (R5.F find_composition)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5G_TableConsistency, ArtCompositionMatchesHardcodedEntry) {
    constexpr auto desc        = tool::descriptor_from_composition<comp::ArtComposition>();
    auto const*    table_entry = tool::find_composition("art");
    ASSERT_NE(table_entry, nullptr);
    EXPECT_EQ(table_entry->cpp_type_name, desc.cpp_type_name);
    EXPECT_EQ(table_entry->header_include, desc.header_include);
}

TEST(R5G_TableConsistency, AllSixCeReimplMatchHardcoded) {
    struct Pair {
        std::string_view short_name;
        std::string_view cpp_type_name;
        std::string_view header_include;
    };

    constexpr auto art_desc   = tool::descriptor_from_composition<comp::ArtComposition>();
    constexpr auto hot_desc   = tool::descriptor_from_composition<comp::HotComposition>();
    constexpr auto wh_desc    = tool::descriptor_from_composition<comp::WormholeComposition>();
    constexpr auto surf_desc  = tool::descriptor_from_composition<comp::SurfComposition>();
    constexpr auto mass_desc  = tool::descriptor_from_composition<comp::MasstreeComposition>();
    constexpr auto start_desc = tool::descriptor_from_composition<comp::StartComposition>();

    Pair const pairs[] = {
        {"art", art_desc.cpp_type_name, art_desc.header_include},
        {"hot", hot_desc.cpp_type_name, hot_desc.header_include},
        {"wormhole", wh_desc.cpp_type_name, wh_desc.header_include},
        {"surf", surf_desc.cpp_type_name, surf_desc.header_include},
        {"masstree", mass_desc.cpp_type_name, mass_desc.header_include},
        {"start", start_desc.cpp_type_name, start_desc.header_include},
    };

    for (auto const& p : pairs) {
        auto const* entry = tool::find_composition(p.short_name);
        ASSERT_NE(entry, nullptr) << "short_name=" << p.short_name;
        EXPECT_EQ(entry->cpp_type_name, p.cpp_type_name) << "Mismatch fuer short_name=" << p.short_name;
        EXPECT_EQ(entry->header_include, p.header_include) << "Mismatch fuer short_name=" << p.short_name;
    }
}

TEST(R5G_TableConsistency, AllFivePaperBindingMatchHardcoded) {
    struct Pair {
        std::string_view short_name;
        std::string_view cpp_type_name;
        std::string_view header_include;
    };

    constexpr auto art_pb      = tool::descriptor_from_composition<comp::ArtPaperBindingComposition>();
    constexpr auto hot_pb      = tool::descriptor_from_composition<comp::HotPaperBindingComposition>();
    constexpr auto start_pb    = tool::descriptor_from_composition<comp::StartPaperBindingComposition>();
    constexpr auto wormhole_pb = tool::descriptor_from_composition<comp::WormholePaperBindingComposition>();
    constexpr auto surf_pb     = tool::descriptor_from_composition<comp::SurfPaperBindingComposition>();

    Pair const pairs[] = {
        {"art_pb", art_pb.cpp_type_name, art_pb.header_include},
        {"hot_pb", hot_pb.cpp_type_name, hot_pb.header_include},
        {"start_pb", start_pb.cpp_type_name, start_pb.header_include},
        {"wormhole_pb", wormhole_pb.cpp_type_name, wormhole_pb.header_include},
        {"surf_pb", surf_pb.cpp_type_name, surf_pb.header_include},
    };

    for (auto const& p : pairs) {
        auto const* entry = tool::find_composition(p.short_name);
        ASSERT_NE(entry, nullptr) << "short_name=" << p.short_name;
        EXPECT_EQ(entry->cpp_type_name, p.cpp_type_name) << "Mismatch fuer short_name=" << p.short_name;
        EXPECT_EQ(entry->header_include, p.header_include) << "Mismatch fuer short_name=" << p.short_name;
    }
}
