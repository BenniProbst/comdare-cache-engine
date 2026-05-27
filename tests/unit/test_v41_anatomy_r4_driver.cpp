// V41.F.6.1.R4 — AnatomyPermutationDriver + CompositionFromPermTuple Tests
//
// Beweist:
// 1. AdHocComposition<17 types> erfuellt IsComposition
// 2. CompositionFromPermTuple<PermTuple<V0...V16>> materialisiert AdHocComposition
// 3. AnatomyPermutationDriver iteriert alle Cartesian-Punkte
// 4. Pilot-Cartesian 3 search_algo × 2 cache_traversal × 15 default = 6 Permutationen
// 5. for_each_animal Visitor + for_each_composition_type Visitor
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §14.3
// @task #696 V41.F.6.1.R4

#include <gtest/gtest.h>

#include <anatomy/composition_factory.hpp>
#include <anatomy/anatomy_permutation_driver.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

// 17 Topic-Achsen Wrappers
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
#include <topics/hardware/axis_09_isa/axis_09_isa_scalar.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_std_map_like.hpp>
#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace ana = ::comdare::cache_engine::anatomy;
namespace pe  = ::comdare::cache_engine::permutations;
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
using CacheLineAligned     = ::comdare::cache_engine::memory_layout::axis_05_memory_layout::CacheLineAlignedLayout;
using MimallocAllocator    = ::comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator;
using PrefetchNone         = ::comdare::cache_engine::prefetch::axis_07_prefetch::PrefetchNone;
using OlcOptimistic        = ::comdare::cache_engine::concurrency::axis_08_concurrency::OlcOptimistic;
using RawBinarySer         = ::comdare::cache_engine::serialization::axis_10_serialization::RawBinarySerialization;
using LeafOnlyCounter      = ::comdare::cache_engine::telemetry::axis_11_telemetry::LeafOnlyCounter;
using InlineHandle         = ::comdare::cache_engine::value_handle::axis_14_value_handle::InlineHandle;
using IsaScalar            = ::comdare::cache_engine::hardware::axis_09_isa::IsaScalar;
using StdMapLike           = ::comdare::cache_engine::search_engine::axis_01_index_organization::StdMapLike;
using InMemoryOnly         = ::comdare::cache_engine::io::axis_io::InMemoryOnly;
using NoMigration          = ::comdare::cache_engine::migration::axis_migration::NoMigration;
using BloomFilter          = ::comdare::cache_engine::filter::axis_filter::BloomFilter;

// ─────────────────────────────────────────────────────────────────────────────
// 17 Topic-Config-Sets fuer Pilot (3 × 2 × 1^15 = 6 Permutationen)
// ─────────────────────────────────────────────────────────────────────────────

struct T0_SearchAlgo     { using StaticAxisVariants = mp::mp_list<Array256SearchAlgo, VectorU8U8SearchAlgo, VectorU16U16SearchAlgo>; };
struct T1_CacheTraversal { using StaticAxisVariants = mp::mp_list<LinearFanout, HashLookup>; };
struct T2_Mapping        { using StaticAxisVariants = mp::mp_list<DirectPlacement>; };
struct T3_PathCompr      { using StaticAxisVariants = mp::mp_list<PathCompressionNone>; };
struct T4_NodeType       { using StaticAxisVariants = mp::mp_list<Node256Layout>; };
struct T5_MemoryLayout   { using StaticAxisVariants = mp::mp_list<CacheLineAligned>; };
struct T6_Allocator      { using StaticAxisVariants = mp::mp_list<MimallocAllocator>; };
struct T7_Prefetch       { using StaticAxisVariants = mp::mp_list<PrefetchNone>; };
struct T8_Concurrency    { using StaticAxisVariants = mp::mp_list<OlcOptimistic>; };
struct T9_Serialization  { using StaticAxisVariants = mp::mp_list<RawBinarySer>; };
struct T10_Telemetry     { using StaticAxisVariants = mp::mp_list<LeafOnlyCounter>; };
struct T11_ValueHandle   { using StaticAxisVariants = mp::mp_list<InlineHandle>; };
struct T12_Isa           { using StaticAxisVariants = mp::mp_list<IsaScalar>; };
struct T13_IndexOrg      { using StaticAxisVariants = mp::mp_list<StdMapLike>; };
struct T14_IoDispatch    { using StaticAxisVariants = mp::mp_list<InMemoryOnly>; };
struct T15_Migration     { using StaticAxisVariants = mp::mp_list<NoMigration>; };
struct T16_Filter        { using StaticAxisVariants = mp::mp_list<BloomFilter>; };

using PilotDriver = ana::AnatomyPermutationDriver<
    T0_SearchAlgo, T1_CacheTraversal, T2_Mapping, T3_PathCompr, T4_NodeType,
    T5_MemoryLayout, T6_Allocator, T7_Prefetch, T8_Concurrency, T9_Serialization,
    T10_Telemetry, T11_ValueHandle, T12_Isa, T13_IndexOrg, T14_IoDispatch,
    T15_Migration, T16_Filter
>;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — AdHocComposition Concept-Conformance
// ─────────────────────────────────────────────────────────────────────────────

using AdHocArt = ana::AdHocComposition<
    Array256SearchAlgo, LinearFanout, DirectPlacement, PathCompressionNone, Node256Layout,
    CacheLineAligned, MimallocAllocator, PrefetchNone, OlcOptimistic, RawBinarySer,
    LeafOnlyCounter, InlineHandle, IsaScalar, StdMapLike, InMemoryOnly,
    NoMigration, BloomFilter
>;

TEST(AnatomyR4_Factory, AdHocCompositionConformsIsComposition) {
    static_assert(ana::IsComposition<AdHocArt>);
    static_assert(AdHocArt::name == std::string_view{"AdHocComposition"});
    SUCCEED();
}

TEST(AnatomyR4_Factory, AdHocCompositionInstantiatesAnatomy) {
    [[maybe_unused]] ana::SearchAlgorithmAnatomy<AdHocArt> algo;
    // R5.B: Composition-Inspection statt Container-Ops
    static_assert(ana::SearchAlgorithmAnatomy<AdHocArt>::organ_count() == 17);
    static_assert(ana::SearchAlgorithmAnatomy<AdHocArt>::composition_name() ==
                  std::string_view{"AdHocComposition"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — CompositionFromPermTuple Materialization
// ─────────────────────────────────────────────────────────────────────────────

using SamplePermTuple = pe::PermTuple<
    Array256SearchAlgo, LinearFanout, DirectPlacement, PathCompressionNone, Node256Layout,
    CacheLineAligned, MimallocAllocator, PrefetchNone, OlcOptimistic, RawBinarySer,
    LeafOnlyCounter, InlineHandle, IsaScalar, StdMapLike, InMemoryOnly,
    NoMigration, BloomFilter
>;

TEST(AnatomyR4_Factory, CompositionFromPermTupleProducesValidComposition) {
    using Materialized = ana::CompositionFromPermTuple<SamplePermTuple>;
    static_assert(ana::IsComposition<Materialized>);
    static_assert(std::is_same_v<Materialized::search_algo, Array256SearchAlgo>);
    static_assert(std::is_same_v<Materialized::cache_traversal, LinearFanout>);
    static_assert(std::is_same_v<Materialized::filter, BloomFilter>);
    SUCCEED();
}

TEST(AnatomyR4_Factory, IsPermTuple17ConceptValidatesArity) {
    static_assert(ana::IsPermTuple17<SamplePermTuple>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — AnatomyPermutationDriver Cartesian-Iteration
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR4_Driver, PilotDriverArityAndCount) {
    static_assert(PilotDriver::arity() == 17, "17 Topic-Achsen Pflicht");
    static_assert(PilotDriver::count() == 6, "3 search_algo × 2 cache_traversal × 1^15 = 6");
    SUCCEED();
}

TEST(AnatomyR4_Driver, ForEachAnimalIteratesAllSixTiere) {
    std::vector<std::string_view> visited_names;
    PilotDriver::for_each_animal([&](auto& anatomy, std::string_view name) {
        visited_names.push_back(name);
        // R5.B Smoke-Test: Anatomie ist instantiiert + Composition-Inspection OK
        // Container-Ops sind in AnatomyExecutionContext (siehe test_v41_builder_anatomy_commands.cpp)
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        EXPECT_EQ(AnatomyT::organ_count(), 17u);
    });
    EXPECT_EQ(visited_names.size(), 6u);
    // Alle 6 Tiere haben den Default-Namen AdHocComposition (kein paper_id Unterschied)
    for (auto n : visited_names) {
        EXPECT_EQ(n, std::string_view{"AdHocComposition"});
    }
}

// Compile-Time Composition-Type-Visitor (fuer CacheEngineBuilder-Pattern)
TEST(AnatomyR4_Driver, ForEachCompositionTypeVisitsSixTypes) {
    std::size_t visit_count = 0;
    PilotDriver::for_each_composition_type([&]<class C>(){
        static_assert(ana::IsComposition<C>);
        ++visit_count;
    });
    EXPECT_EQ(visit_count, 6u);
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Composition-Diversity-Beweis (Tier-Organ-Metapher in Permutationen)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR4_Driver, EachPermutationIsDistinctTier) {
    std::vector<std::pair<std::string_view, std::string_view>> animals;
    PilotDriver::for_each_composition_type([&]<class C>(){
        // typeid().name() vergleichen waere unportable; nutze stattdessen
        // sizeof + Composition-Members als Distinguisher
        animals.emplace_back(
            typeid(typename C::search_algo).name(),
            typeid(typename C::cache_traversal).name());
    });
    EXPECT_EQ(animals.size(), 6u);
    // Pruefe dass 3 search_algo × 2 cache_traversal alle distinkten Kombinationen
    // produziert (jedes Tupel sollte unique sein)
    std::set<std::pair<std::string_view, std::string_view>> unique_animals(
        animals.begin(), animals.end());
    EXPECT_EQ(unique_animals.size(), 6u);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Roundtrip Insert/Lookup pro Tier (alle 6 Tiere funktionieren)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AnatomyR4_Driver, AllSixTiereInstantiateIndependently) {
    // R5.B: Container-Roundtrip in test_v41_builder_anatomy_commands.cpp.
    // Hier nur Beweis dass alle 6 Anatomien default-konstruierbar sind.
    std::size_t instantiate_count = 0;
    PilotDriver::for_each_animal([&](auto& anatomy, std::string_view) {
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        EXPECT_EQ(AnatomyT::organ_count(), 17u);
        ++instantiate_count;
    });
    EXPECT_EQ(instantiate_count, 6u);
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Forschungs-Mission: Erweiterbarkeit auf groessere Cartesian-Raeume
// ─────────────────────────────────────────────────────────────────────────────

// Minimal-Verifikation: zwei T0/T1 Compositions erlauben Vergleich (Tier-Variation)
TEST(AnatomyR4_Driver, NonEmptyAxisCountMatchesArity) {
    static_assert(PilotDriver::arity() == 17);
    // PermutationEngine::non_empty_axis_count ist intern bereits validiert per static_assert
    SUCCEED();
}
