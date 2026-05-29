// V41.F.6.1.R5.C.B — SearchAlgorithmPermutationEngine Tests
//
// Beweist:
// 1. SearchAlgorithmPermutationEngine::genus == AnatomyGenus::SearchAlgorithm
// 2. count()/arity() delegieren korrekt an PermutationEngine
// 3. slot_genus_v<Slot> Default = SearchAlgorithm (kein explicit genus)
// 4. slot_genus_v<Slot> explicit-SearchAlgorithm = SearchAlgorithm
// 5. slot_genus_v<Slot> explicit-Sequence = Sequence
// 6. IsSearchAlgorithmSlot Concept positiv (Mammal-Slot)
// 7. IsSearchAlgorithmSlot Concept negativ (Sequence-Slot)
// 8. assert_pruefling_slot_genus<MammalSlot>() kompiliert OK
// 9. slots_match_genus_v<...> bei Mammal-only = true
// 10. slots_match_genus_v<...> bei Cross-Genus = false
// 11. for_each_search_algorithm Visitor iteriert alle Permutationen
// 12. for_each_abi_adapter Visitor liefert IAnatomyBase-Pointer pro Permutation
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32
// @task #703 V41.F.6.1.R5.C.B

#include <gtest/gtest.h>

#include <anatomy/abi_adapter.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/pruefling_merge.hpp>
#include <anatomy/search_algorithm_permutation_engine.hpp>
#include <anatomy/combinatorial_coverage.hpp>   // R5.D: Sampling-Grundlage fuer den kartesischen Raum
// R5.D — echte Achsen-Registry-Listen (fuer Coverage ueber die REALEN Achsen-Groessen):
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_registry.hpp>
#include <builder/codegen/type_name.hpp>   // R5.G Auto-Emitter: FQ-Typ-Namen pro Achse

// 17 Topic-Achsen Wrappers (identisch zu test_v41_anatomy_r4_driver.cpp)
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u16u16.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_hash_lookup.hpp>
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_direct_placement.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/prefetch/axis_07_prefetch/axis_07_prefetch_none.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/telemetry/axis_11_telemetry/axis_11_telemetry_leaf_only.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_index_organized_table.hpp>
#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>

#include <boost/mp11.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ana = ::comdare::cache_engine::anatomy;
namespace pf  = ::comdare::cache_engine::anatomy::pruefling;
namespace mp  = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// Type-Aliases (kompakter)
// ─────────────────────────────────────────────────────────────────────────────

using Array256SearchAlgo             = ::comdare::cache_engine::traversal::axis_03a_search_algo::Array256SearchAlgo;
using VectorU8U8SearchAlgo           = ::comdare::cache_engine::traversal::axis_03a_search_algo::VectorU8U8SearchAlgo;
using VectorU16U16SearchAlgo         = ::comdare::cache_engine::traversal::axis_03a_search_algo::VectorU16U16SearchAlgo;
using LinearFanout         = ::comdare::cache_engine::traversal::axis_03b_cache_traversal::LinearFanout;
using HashLookup           = ::comdare::cache_engine::traversal::axis_03b_cache_traversal::HashLookup;
using DirectPlacement      = ::comdare::cache_engine::traversal::axis_03m_mapping::DirectPlacement;
using PathCompressionNone  = ::comdare::cache_engine::nodes::axis_02_path_compression::PathCompressionNone;
using Node256Layout          = ::comdare::cache_engine::nodes::axis_04_node_type::Node256Layout;
using CacheLineAligned     = ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout;
using MimallocAllocator    = ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator;
using NonePrefetch         = ::comdare::cache_engine::prefetch::axis_07_prefetch::NonePrefetch;
using OlcOptimisticConcurrency        = ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
using RawBinarySer         = ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization;
using LeafOnlyCounter      = ::comdare::cache_engine::telemetry::axis_11_telemetry::LeafOnlyCounter;
using InlineValueHandle         = ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineValueHandle;
using Amd64Isa            = ::comdare::cache_engine::hardware::axis_09_isa::Amd64Isa;
using IotIndexOrganization           = ::comdare::cache_engine::search_engine::axis_01_index_organization::IotIndexOrganization;
using InMemoryOnly         = ::comdare::cache_engine::io::axis_io::InMemoryOnly;
using NoMigration          = ::comdare::cache_engine::migration::axis_migration::NoMigration;
using BloomFilter          = ::comdare::cache_engine::filter::axis_filter::BloomFilter;

// ─────────────────────────────────────────────────────────────────────────────
// Pilot 17 Topic-Config-Sets (3 × 2 × 1^15 = 6 Permutationen, identisch zu R4-Test)
// ─────────────────────────────────────────────────────────────────────────────

struct T0_SearchAlgo     { using StaticAxisVariants = mp::mp_list<Array256SearchAlgo, VectorU8U8SearchAlgo, VectorU16U16SearchAlgo>; };
struct T1_CacheTraversal { using StaticAxisVariants = mp::mp_list<LinearFanout, HashLookup>; };
struct T2_Mapping        { using StaticAxisVariants = mp::mp_list<DirectPlacement>; };
struct T3_PathCompr      { using StaticAxisVariants = mp::mp_list<PathCompressionNone>; };
struct T4_NodeType       { using StaticAxisVariants = mp::mp_list<Node256Layout>; };
struct T5_MemoryLayout   { using StaticAxisVariants = mp::mp_list<CacheLineAligned>; };
struct T6_Allocator      { using StaticAxisVariants = mp::mp_list<MimallocAllocator>; };
struct T7_Prefetch       { using StaticAxisVariants = mp::mp_list<NonePrefetch>; };
struct T8_Concurrency    { using StaticAxisVariants = mp::mp_list<OlcOptimisticConcurrency>; };
struct T9_Serialization  { using StaticAxisVariants = mp::mp_list<RawBinarySer>; };
struct T10_Telemetry     { using StaticAxisVariants = mp::mp_list<LeafOnlyCounter>; };
struct T11_ValueHandle   { using StaticAxisVariants = mp::mp_list<InlineValueHandle>; };
struct T12_Isa           { using StaticAxisVariants = mp::mp_list<Amd64Isa>; };
struct T13_IndexOrg      { using StaticAxisVariants = mp::mp_list<IotIndexOrganization>; };
struct T14_IoDispatch    { using StaticAxisVariants = mp::mp_list<InMemoryOnly>; };
struct T15_Migration     { using StaticAxisVariants = mp::mp_list<NoMigration>; };
struct T16_Filter        { using StaticAxisVariants = mp::mp_list<BloomFilter>; };

using PilotEngine = ana::SearchAlgorithmPermutationEngine<
    T0_SearchAlgo, T1_CacheTraversal, T2_Mapping, T3_PathCompr, T4_NodeType,
    T5_MemoryLayout, T6_Allocator, T7_Prefetch, T8_Concurrency, T9_Serialization,
    T10_Telemetry, T11_ValueHandle, T12_Isa, T13_IndexOrg, T14_IoDispatch,
    T15_Migration, T16_Filter
>;

// ─────────────────────────────────────────────────────────────────────────────
// Sample-Slots fuer Gattungs-Constraint-Tests
// ─────────────────────────────────────────────────────────────────────────────

// Default-Slot ohne explicit genus → slot_genus_v Default = SearchAlgorithm
struct ImplicitMammalSlot {
    using PrueflingVariants = mp::mp_list<Array256SearchAlgo>;
    static constexpr bool has_pruefling = true;
};

// Explicit-SearchAlgorithm Slot
struct ExplicitMammalSlot {
    using PrueflingVariants = mp::mp_list<Array256SearchAlgo>;
    static constexpr bool has_pruefling = true;
    static constexpr ana::AnatomyGenus genus = ana::AnatomyGenus::SearchAlgorithm;
};

// Cross-Genus Slot (Sequence-Gattung)
struct SequenceSlot {
    using PrueflingVariants = mp::mp_list<>;
    static constexpr bool has_pruefling = false;
    static constexpr ana::AnatomyGenus genus = ana::AnatomyGenus::Sequence;
};

// Cross-Genus Slot (Set-Gattung)
struct SetSlot {
    using PrueflingVariants = mp::mp_list<>;
    static constexpr bool has_pruefling = false;
    static constexpr ana::AnatomyGenus genus = ana::AnatomyGenus::Set;
};

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Genus-Marker
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SearchAlgoEngine, GenusIsSearchAlgorithm) {
    static_assert(PilotEngine::genus == ana::AnatomyGenus::SearchAlgorithm);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — count()/arity() delegieren an PermutationEngine
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SearchAlgoEngine, ArityAndCountMatchPermutationEngine) {
    static_assert(PilotEngine::arity() == 17, "17 Topic-Achsen Pflicht");
    static_assert(PilotEngine::count() == 6, "3 search_algo × 2 cache_traversal × 1^15 = 6");
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — slot_genus_v<Slot> Default = SearchAlgorithm (kein explicit genus)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SlotGenusDetection, ImplicitSlotDefaultsToSearchAlgorithm) {
    static_assert(!pf::HasExplicitGenus<ImplicitMammalSlot>);
    static_assert(pf::slot_genus_v<ImplicitMammalSlot> == ana::AnatomyGenus::SearchAlgorithm);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — slot_genus_v<Slot> explicit-SearchAlgorithm = SearchAlgorithm
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SlotGenusDetection, ExplicitSearchAlgorithmSlotMatches) {
    static_assert(pf::HasExplicitGenus<ExplicitMammalSlot>);
    static_assert(pf::slot_genus_v<ExplicitMammalSlot> == ana::AnatomyGenus::SearchAlgorithm);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — slot_genus_v<Slot> explicit-Sequence = Sequence (Cross-Genus)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SlotGenusDetection, ExplicitSequenceSlotMatches) {
    static_assert(pf::HasExplicitGenus<SequenceSlot>);
    static_assert(pf::slot_genus_v<SequenceSlot> == ana::AnatomyGenus::Sequence);
    SUCCEED();
}

TEST(R5CB_SlotGenusDetection, ExplicitSetSlotMatches) {
    static_assert(pf::slot_genus_v<SetSlot> == ana::AnatomyGenus::Set);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — IsSearchAlgorithmSlot Concept (positiv + negativ)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SlotGenusConcept, MammalSlotsConformToConcept) {
    static_assert(pf::IsSearchAlgorithmSlot<ImplicitMammalSlot>);
    static_assert(pf::IsSearchAlgorithmSlot<ExplicitMammalSlot>);
    SUCCEED();
}

TEST(R5CB_SlotGenusConcept, NonMammalSlotsRejectedByConcept) {
    static_assert(!pf::IsSearchAlgorithmSlot<SequenceSlot>);
    static_assert(!pf::IsSearchAlgorithmSlot<SetSlot>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — assert_pruefling_slot_genus<MammalSlot>() kompiliert OK
//      (Negativ-Test fuer Cross-Genus ist Compile-Time-only — siehe §32.3 Doku 14)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_GenusAssertion, AssertMammalSlotPassesCompileTime) {
    PilotEngine::assert_pruefling_slot_genus<ImplicitMammalSlot>();
    PilotEngine::assert_pruefling_slot_genus<ExplicitMammalSlot>();
    PilotEngine::assert_pruefling_slot_genus<pf::EmptyPrueflingSlot>();
    SUCCEED();
}

TEST(R5CB_GenusAssertion, AssertAllMammalSlotsPassesVariadic) {
    PilotEngine::assert_all_pruefling_slots_genus<
        ImplicitMammalSlot, ExplicitMammalSlot, pf::EmptyPrueflingSlot>();
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — slots_match_genus_v Compile-Time-Predicate
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_SlotsMatchGenus, MammalOnlySlotsMatch) {
    static_assert(PilotEngine::slots_match_genus_v<
        ImplicitMammalSlot, ExplicitMammalSlot, pf::EmptyPrueflingSlot>);
    SUCCEED();
}

TEST(R5CB_SlotsMatchGenus, CrossGenusSlotsFail) {
    static_assert(!PilotEngine::slots_match_genus_v<
        ImplicitMammalSlot, SequenceSlot>);
    static_assert(!PilotEngine::slots_match_genus_v<SetSlot>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §9 — for_each_search_algorithm Visitor (technisch benannte API)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_VisitorApi, ForEachSearchAlgorithmIteratesAllPermutations) {
    std::vector<std::string_view> visited_names;
    PilotEngine::for_each_search_algorithm([&](auto& anatomy, std::string_view name) {
        visited_names.push_back(name);
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        EXPECT_EQ(AnatomyT::organ_count(), 17u);
        EXPECT_EQ(AnatomyT::genus(), ana::AnatomyGenus::SearchAlgorithm);
    });
    EXPECT_EQ(visited_names.size(), 6u);
}

TEST(R5CB_VisitorApi, ForEachCompositionTypeIteratesAllPermutations) {
    std::size_t visit_count = 0;
    PilotEngine::for_each_composition_type([&]<class C>(){
        static_assert(ana::IsComposition<C>);
        ++visit_count;
    });
    EXPECT_EQ(visit_count, 6u);
}

// ─────────────────────────────────────────────────────────────────────────────
// §10 — for_each_abi_adapter Visitor (R5.E Module-Loader Vorbereitung)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5CB_AbiAdapterIteration, ForEachAbiAdapterProducesIAnatomyBasePerPermutation) {
    namespace ee = ::comdare::cache_engine::execution_engine;
    std::size_t adapter_count = 0;
    std::vector<ana::AnatomyGenus> genera_seen;
    std::vector<std::string_view> names_seen;
    PilotEngine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view name) {
        ++adapter_count;
        // Pro Permutation ein IAnatomyBase mit korrekter Gattung
        genera_seen.push_back(base.genus());
        names_seen.push_back(name);
        EXPECT_EQ(base.organ_count(), 17u);
        EXPECT_EQ(base.engine_kind(), ee::ExecutionEngineKind::Anatomy);

        // R5.C.A4: vollstaendiger Mess-Lifecycle (CacheEngineBuilder-Pattern R5.D)
        base.warm_up();
        EXPECT_EQ(base.lifecycle_state(), ee::EngineLifecycleState::Warming);
        base.run();
        EXPECT_EQ(base.lifecycle_state(), ee::EngineLifecycleState::Running);
        base.shutdown();
        EXPECT_EQ(base.lifecycle_state(), ee::EngineLifecycleState::Shutdown);
    });
    EXPECT_EQ(adapter_count, 6u);
    // Alle 6 Adapter haben SearchAlgorithm-Gattung
    for (auto g : genera_seen) {
        EXPECT_EQ(g, ana::AnatomyGenus::SearchAlgorithm);
    }
    // Alle Namen sind nicht-leer (AdHocComposition Default)
    for (auto n : names_seen) {
        EXPECT_FALSE(n.empty());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §R5.G — Auto-Emitter-Kern-Logik: for_each_composition_type × type_name
//         → der COMDARE_DEFINE_ANATOMY_MODULE_ADHOC-Argument-String pro Permutation.
//         (Das ist exakt, was der Auto-Emitter pro Permutation in ein Modul-.cpp schreibt.)
// ─────────────────────────────────────────────────────────────────────────────

namespace {
namespace cg = ::comdare::cache_engine::builder::codegen;

/// Baut den 17-Achsen-FQ-Typ-Namen-String einer Composition C (Komma-getrennt) — der
/// Argument-Block für COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...).
template <class C>
std::string adhoc_macro_args() {
    std::string s;
    auto add = [&](std::string_view t) { if (!s.empty()) s += ", "; s += t; };
    add(cg::type_name<typename C::search_algo>());
    add(cg::type_name<typename C::cache_traversal>());
    add(cg::type_name<typename C::mapping>());
    add(cg::type_name<typename C::path_compression>());
    add(cg::type_name<typename C::node_type>());
    add(cg::type_name<typename C::memory_layout>());
    add(cg::type_name<typename C::allocator>());
    add(cg::type_name<typename C::prefetch>());
    add(cg::type_name<typename C::concurrency>());
    add(cg::type_name<typename C::serialization>());
    add(cg::type_name<typename C::telemetry>());
    add(cg::type_name<typename C::value_handle>());
    add(cg::type_name<typename C::isa>());
    add(cg::type_name<typename C::index_organization>());
    add(cg::type_name<typename C::io_dispatch>());
    add(cg::type_name<typename C::migration_policy>());
    add(cg::type_name<typename C::filter>());
    return s;
}
}  // namespace

TEST(R5G_AutoEmitter, BuildsAdHocMacroArgsPerPermutation) {
    std::vector<std::string> emitted;
    PilotEngine::for_each_composition_type([&]<class C>() {
        emitted.push_back(adhoc_macro_args<C>());
    });

    // Eine Argument-Zeile pro Permutation des gemergten Raums.
    ASSERT_EQ(emitted.size(), PilotEngine::count());           // 6
    for (auto const& s : emitted) {
        // 17 FQ-Typ-Namen → 16 Kommas; reale Achsen-Namespaces vorhanden.
        EXPECT_EQ(std::count(s.begin(), s.end(), ','), 16);
        EXPECT_NE(s.find("comdare::cache_engine::"), std::string::npos);
        EXPECT_EQ(s.find("class "), std::string::npos);        // codegen-nutzbar (kein Elaborated-Prefix)
    }
    // Alle 6 Permutationen sind distinkt (unterschiedliche search_algo/cache_traversal-Achsen).
    std::set<std::string> uniq(emitted.begin(), emitted.end());
    EXPECT_EQ(uniq.size(), emitted.size());

    std::cout << "[R5G auto-emitter] " << emitted.size()
              << " Permutationen; Perm[0] ADHOC-args:\n  " << emitted.front() << "\n";
}

// Der EIGENTLICHE Emitter: schreibt pro Permutation ein kompilierbares Modul-.cpp
// (Umbrella-Include + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<args>)). Das ist die Datei-Emission des
// Auto-Emitters — die CMake-add_library-Schleife darüber ist das letzte mechanische Plumbing.
TEST(R5G_AutoEmitter, EmitsModuleCppPerPermutation) {
    namespace fs = std::filesystem;
    fs::path const dir = fs::temp_directory_path() / "comdare_r5g_emit";
    std::error_code ec;
    fs::create_directories(dir, ec);

    std::vector<fs::path> files;
    int idx = 0;
    PilotEngine::for_each_composition_type([&]<class C>() {
        fs::path const f = dir / ("comdare_anatomy_perm_auto_" + std::to_string(idx) + ".cpp");
        std::ofstream out(f, std::ios::trunc);
        out << "// AUTO-GENERATED by R5.G Auto-Emitter — Permutation " << idx << " — DO NOT EDIT\n"
            << "#include <builder/codegen/all_axes_umbrella.hpp>\n\n"
            << "COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(\n    "
            << adhoc_macro_args<C>() << ")\n";
        files.push_back(f);
        ++idx;
    });

    ASSERT_EQ(files.size(), PilotEngine::count());
    // Inhalt jedes emittierten Modul-.cpp prüfen (kompilierbares Permutations-Modul).
    for (auto const& f : files) {
        std::ifstream in(f);
        std::string const content((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("all_axes_umbrella.hpp"), std::string::npos);
        EXPECT_NE(content.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC("), std::string::npos);
        EXPECT_NE(content.find("comdare::cache_engine::traversal::axis_03a_search_algo::"),
                  std::string::npos);
    }
    std::cout << "[R5G emitter] " << files.size() << " Modul-.cpp geschrieben nach " << dir << "\n";

    for (auto const& f : files) fs::remove(f, ec);  // Aufräumen
}

// V41.F.6.1 R5.D — kombinatorische Coverage des Permutations-Raums (Sampling-Grundlage).
// Quantifizierung (full/1-wise/pairwise) + 1-wise-Stichprobe deckt jede Achsen-Variante mind. 1×.
TEST(R5D_CombinatorialCoverage, QuantifyAndOneWiseSampleCoversEveryValue) {
    std::array<std::size_t, 3> counts{17, 25, 5};   // z. B. search × allocator × layout
    auto const rep = ana::analyze_coverage(std::span<const std::size_t>{counts});
    EXPECT_EQ(rep.axis_count, 3u);
    EXPECT_EQ(rep.full_space, 17u * 25u * 5u);          // 2125
    EXPECT_FALSE(rep.full_space_saturated);
    EXPECT_FALSE(rep.any_axis_empty);
    EXPECT_EQ(rep.one_wise_cover, 25u);                  // = max(counts)
    EXPECT_EQ(rep.pairwise_lower_bound, 25u * 17u);      // zwei groesste

    // 1-wise-Stichprobe: 25 Zeilen; JEDE Variante JEDER Achse muss vorkommen.
    auto const sample = ana::one_wise_cover_sample(std::span<const std::size_t>{counts});
    ASSERT_EQ(sample.size(), 25u);
    for (std::size_t axis = 0; axis < counts.size(); ++axis) {
        std::set<std::size_t> seen;
        for (auto const& row : sample) { ASSERT_EQ(row.size(), 3u); seen.insert(row[axis]); }
        EXPECT_EQ(seen.size(), counts[axis]) << "Achse " << axis << " nicht voll abgedeckt";
        EXPECT_EQ(*seen.begin(), 0u);
        EXPECT_EQ(*seen.rbegin(), counts[axis] - 1);
    }
}

TEST(R5D_CombinatorialCoverage, SaturationEmptyAxisAndEdgeCases) {
    // Overflow → gesaettigt (viele grosse Achsen).
    std::array<std::size_t, 8> big{}; big.fill(1000000u);  // 1e6^8 = 1e48 >> uint64
    auto const rs = ana::analyze_coverage(std::span<const std::size_t>{big});
    EXPECT_TRUE(rs.full_space_saturated);
    EXPECT_EQ(rs.full_space, ana::kCoverageSaturated);

    // Leere Achse (0 Varianten) → Raum leer, Stichprobe leer.
    std::array<std::size_t, 3> withzero{4, 0, 7};
    auto const rz = ana::analyze_coverage(std::span<const std::size_t>{withzero});
    EXPECT_TRUE(rz.any_axis_empty);
    EXPECT_EQ(rz.full_space, 0u);
    EXPECT_TRUE(ana::one_wise_cover_sample(std::span<const std::size_t>{withzero}).empty());

    // Leere Achsenliste.
    auto const re = ana::analyze_coverage(std::span<const std::size_t>{});
    EXPECT_EQ(re.axis_count, 0u);
    EXPECT_EQ(re.full_space, 0u);
}

// V41.F.6.1 R5.D — Bruecke zur REALEN Achsen-Bibliothek: die Coverage-Utility, gespeist mit den
// ECHTEN Registry-Groessen (mp_size der AllStrategies/AllVendors/AllLayouts), dimensioniert die
// 1-wise-Stichprobe korrekt UND liefert ausschliesslich Indizes < der jeweiligen Listengroesse —
// d. h. ein mp_at_c<Liste, idx> waere fuer JEDE Stichproben-Zeile gueltig (Voraussetzung fuers
// spaetere Emitter-Verdrahten der Voll-Coverage). Beweis ohne 2125 DLLs zu bauen.
TEST(R5D_CombinatorialCoverage, DimensionedFromRealAxisRegistries) {
    namespace s03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
    namespace a06  = ::comdare::cache_engine::allocator::axis_06_allocator;
    namespace m05  = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;

    constexpr std::size_t kSearch = mp::mp_size<s03a::AllStrategies>::value;  // 17
    constexpr std::size_t kAlloc  = mp::mp_size<a06::AllVendors>::value;      // 25
    constexpr std::size_t kLayout = mp::mp_size<m05::AllLayouts>::value;      // 5
    static_assert(kSearch == 17 && kAlloc == 25 && kLayout == 5,
                  "Achsen-Groessen-Snapshot (bei Erweiterung anpassen)");

    std::array<std::size_t, 3> counts{kSearch, kAlloc, kLayout};
    auto const rep = ana::analyze_coverage(std::span<const std::size_t>{counts});
    EXPECT_EQ(rep.full_space, std::uint64_t{kSearch} * kAlloc * kLayout);  // 2125 voller Raum
    EXPECT_EQ(rep.one_wise_cover, kAlloc);          // 25 = max → traktabel statt 2125
    EXPECT_EQ(rep.pairwise_lower_bound, std::uint64_t{kAlloc} * kSearch);  // 425

    // Jeder Stichproben-Index ist < der jeweiligen Listengroesse → mp_at_c stets gueltig.
    auto const sample = ana::one_wise_cover_sample(std::span<const std::size_t>{counts});
    ASSERT_EQ(sample.size(), kAlloc);  // 25 Permutationen decken die volle Variantenmenge
    std::set<std::size_t> sset, aset, lset;
    for (auto const& row : sample) {
        ASSERT_EQ(row.size(), 3u);
        EXPECT_LT(row[0], kSearch); EXPECT_LT(row[1], kAlloc); EXPECT_LT(row[2], kLayout);
        sset.insert(row[0]); aset.insert(row[1]); lset.insert(row[2]);
    }
    EXPECT_EQ(sset.size(), kSearch);  // ALLE 17 Such-Algos abgedeckt
    EXPECT_EQ(aset.size(), kAlloc);   // ALLE 25 Allokatoren abgedeckt
    EXPECT_EQ(lset.size(), kLayout);  // ALLE 5 Layouts abgedeckt
}
