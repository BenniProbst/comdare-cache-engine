// V41.F.6.1 Topic traversal Pilot Tests (2026-05-26)
//
// TYPED_TEST_SUITE ueber alle 3 Achsen (03a search_algo, 03b cache_traversal, 03m mapping)
// + spezifische Verhaltens-Tests fuer Sonderfaelle (DensityClassified, SimdCapable,
// pool-relative-Rebase, hash-resize).

#include <gtest/gtest.h>

#include <topics/traversal/concepts/topic_traversal_concept.hpp>
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_iterable_aspect_strategy_concept.hpp>
#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_registry.hpp>
#include <topics/traversal/axis_03b_cache_traversal/concepts/axis_03b_cache_traversal_hashed_strategy_concept.hpp>
#include <topics/traversal/axis_03b_cache_traversal/concepts/axis_03b_cache_traversal_iterable_aspect_strategy_concept.hpp>
#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_registry.hpp>
#include <topics/traversal/axis_03m_mapping/concepts/axis_03m_mapping_pool_rebasable_strategy_concept.hpp>
#include <permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace mp = boost::mp11;
namespace ce_traversal = comdare::cache_engine::traversal;
namespace ce_03a = comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_03b = comdare::cache_engine::traversal::axis_03b_cache_traversal;
namespace ce_03m = comdare::cache_engine::traversal::axis_03m_mapping;

// =================================================================
// Topic-Marker (M2-Layer-Test)
// =================================================================

TEST(V41_TopicTraversal, TopicTagIsDefined) {
    static_assert(std::is_class_v<ce_traversal::concepts::TraversalTopicTag>);
    SUCCEED();
}

TEST(V41_TopicTraversal, RegistryEnabledCounts) {
    EXPECT_GT(mp::mp_size<ce_03a::EnabledStrategies>::value, 0u);
    EXPECT_GT(mp::mp_size<ce_03b::EnabledStrategies>::value, 0u);
    EXPECT_GT(mp::mp_size<ce_03m::EnabledStrategies>::value, 0u);
}

TEST(V41_TopicTraversal, TopicConfigSetCartesianProductSize) {
    using Cart = ce_traversal::TopicConfigSet::Cartesian03ax03bx03m;
    constexpr std::size_t a = mp::mp_size<ce_03a::EnabledStrategies>::value;
    constexpr std::size_t b = mp::mp_size<ce_03b::EnabledStrategies>::value;
    constexpr std::size_t m = mp::mp_size<ce_03m::EnabledStrategies>::value;
    EXPECT_EQ(mp::mp_size<Cart>::value, a * b * m);
}

// =================================================================
// TYPED_TEST_SUITE — axis_03a search_algo
// =================================================================

template <class... Vs> using ToGTestTypes = ::testing::Types<Vs...>;
using AllSearchAlgoTypes = mp::mp_apply<ToGTestTypes, ce_03a::AllStrategies>;

template <class T> class SearchAlgoTest : public ::testing::Test {};
TYPED_TEST_SUITE(SearchAlgoTest, AllSearchAlgoTypes);

TYPED_TEST(SearchAlgoTest, ConceptConformance) {
    static_assert(ce_03a::concepts::SearchAlgoVariant<TypeParam>);
    static_assert(ce_03a::concepts::CacheEngineSearchAlgoPermutationStrategy<TypeParam>);
    static_assert(ce_03a::concepts::DensityClassifiedStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(SearchAlgoTest, IdentificationFields) {
    EXPECT_FALSE(TypeParam::name().empty());
    EXPECT_FALSE(TypeParam::family_name().empty());
    EXPECT_FALSE(TypeParam::flag_suffix().empty());
    EXPECT_GT(TypeParam::max_fanout(), 0u);
}

TYPED_TEST(SearchAlgoTest, EmptyAfterDefault) {
    TypeParam s{};
    EXPECT_EQ(s.occupied_count(), 0u);
    EXPECT_EQ(s.density_percent(), 0.0);
}

TYPED_TEST(SearchAlgoTest, InsertLookupRoundtrip) {
    TypeParam s{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    s.insert(K{42}, V{100});
    s.insert(K{7},  V{200});
    auto v42 = s.lookup(K{42});
    ASSERT_TRUE(v42.has_value());
    EXPECT_EQ(*v42, V{100});
    auto v7 = s.lookup(K{7});
    ASSERT_TRUE(v7.has_value());
    EXPECT_EQ(*v7, V{200});
    EXPECT_EQ(s.occupied_count(), 2u);
}

TYPED_TEST(SearchAlgoTest, LookupMissReturnsNullopt) {
    TypeParam s{};
    using K = typename TypeParam::key_type;
    auto v = s.lookup(K{99});
    EXPECT_FALSE(v.has_value());
}

TYPED_TEST(SearchAlgoTest, EraseRemovesEntry) {
    TypeParam s{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    s.insert(K{42}, V{100});
    EXPECT_TRUE(s.erase(K{42}));
    auto v = s.lookup(K{42});
    EXPECT_FALSE(v.has_value());
    EXPECT_FALSE(s.erase(K{42}));  // bereits weg
}

TYPED_TEST(SearchAlgoTest, ClearMakesEmpty) {
    TypeParam s{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    s.insert(K{42}, V{100});
    s.clear();
    EXPECT_EQ(s.occupied_count(), 0u);
}

TYPED_TEST(SearchAlgoTest, DensityClassReturnsValid) {
    TypeParam s{};
    auto dc = s.density_class();
    EXPECT_TRUE(dc == ce_03a::concepts::DensityClass::Sparse ||
                dc == ce_03a::concepts::DensityClass::Balanced ||
                dc == ce_03a::concepts::DensityClass::Dense ||
                dc == ce_03a::concepts::DensityClass::AdaptiveTransition);
}

TYPED_TEST(SearchAlgoTest, SonderfallPropertiesAccessible) {
    [[maybe_unused]] bool simd = TypeParam::supports_simd();
    [[maybe_unused]] bool rs   = TypeParam::supports_range_scan();
    [[maybe_unused]] bool dense = TypeParam::is_dense();
    [[maybe_unused]] bool aligned = TypeParam::has_cache_line_alignment();
    SUCCEED();
}

#ifdef COMDARE_CE_ENABLE_STATISTICS
TYPED_TEST(SearchAlgoTest, ObserverAliasIsMeasurableObserver) {
    using snap_t = typename TypeParam::snapshot_t;
    using obs_t  = typename TypeParam::observer_t;
    static_assert(std::is_same_v<obs_t,
        ::comdare::cache_engine::measurement::MeasurableObserver<snap_t>>);
    SUCCEED();
}

TYPED_TEST(SearchAlgoTest, ObserverNotifiedOnInsert) {
    TypeParam s{};
    int events = 0;
    s.observer().on_event([&events](auto const&) { ++events; });
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    s.insert(K{1}, V{10});
    EXPECT_GE(events, 1);
}
#endif

// =================================================================
// Spezifische 03a Tests
// =================================================================

TEST(SearchAlgo_Array256, DensityClassAlwaysDense) {
    ce_03a::Array256 s{};
    EXPECT_EQ(s.density_class(), ce_03a::concepts::DensityClass::Dense);
}

TEST(SearchAlgo_Array256, MaxFanoutIs256) {
    EXPECT_EQ(ce_03a::Array256::max_fanout(), 256u);
}

TEST(SearchAlgo_VectorU8U8, SparseAtLowDensity) {
    ce_03a::VectorU8U8 s{};
    s.insert(1, 100);
    s.insert(2, 200);
    EXPECT_EQ(s.density_class(), ce_03a::concepts::DensityClass::Sparse);
}

TEST(SearchAlgo_VectorU16U16, BalancedDefault) {
    ce_03a::VectorU16U16 s{};
    EXPECT_EQ(s.density_class(), ce_03a::concepts::DensityClass::Balanced);
}

TEST(SearchAlgo_VectorU16U16, DoesNotErfuellSimdConcept) {
    static_assert(!ce_03a::concepts::SimdCapableStrategy<ce_03a::VectorU16U16>,
        "VectorU16U16 darf SimdCapableStrategy NICHT erfuellen (Cost-DP nicht vectorisierbar)");
    SUCCEED();
}

TEST(SearchAlgo_Array256, ErfuellSimdConcept) {
    static_assert(ce_03a::concepts::SimdCapableStrategy<ce_03a::Array256>);
    SUCCEED();
}

TEST(SearchAlgo_VectorU8U8, ErfuellSimdConcept) {
    static_assert(ce_03a::concepts::SimdCapableStrategy<ce_03a::VectorU8U8>);
    SUCCEED();
}

// =================================================================
// TYPED_TEST_SUITE — axis_03b cache_traversal
// =================================================================

using AllCacheTraversalTypes = mp::mp_apply<ToGTestTypes, ce_03b::AllStrategies>;

template <class T> class CacheTraversalTest : public ::testing::Test {};
TYPED_TEST_SUITE(CacheTraversalTest, AllCacheTraversalTypes);

TYPED_TEST(CacheTraversalTest, ConceptConformance) {
    static_assert(ce_03b::concepts::CacheTraversalVariant<TypeParam>);
    static_assert(ce_03b::concepts::CacheEngineCacheTraversalPermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(CacheTraversalTest, RegisterResolveRoundtrip) {
    TypeParam t{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    t.register_entry(K{42}, V{100});
    auto v = t.resolve(K{42});
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, V{100});
    EXPECT_EQ(t.tracked_count(), 1u);
}

TYPED_TEST(CacheTraversalTest, ResolveMissReturnsNullopt) {
    TypeParam t{};
    using K = typename TypeParam::key_type;
    EXPECT_FALSE(t.resolve(K{99}).has_value());
}

TYPED_TEST(CacheTraversalTest, UnregisterRemovesEntry) {
    TypeParam t{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    t.register_entry(K{42}, V{100});
    EXPECT_TRUE(t.unregister(K{42}));
    EXPECT_FALSE(t.resolve(K{42}).has_value());
    EXPECT_FALSE(t.unregister(K{42}));
}

TYPED_TEST(CacheTraversalTest, ClearMakesEmpty) {
    TypeParam t{};
    using K = typename TypeParam::key_type;
    using V = typename TypeParam::value_type;
    t.register_entry(K{1}, V{10});
    t.register_entry(K{2}, V{20});
    t.clear();
    EXPECT_EQ(t.tracked_count(), 0u);
}

TYPED_TEST(CacheTraversalTest, SonderfallPropertiesAccessible) {
    [[maybe_unused]] bool h  = TypeParam::is_hashed();
    [[maybe_unused]] bool cc = TypeParam::has_collision_chains();
    [[maybe_unused]] bool o1 = TypeParam::amortized_o1();
    SUCCEED();
}

// =================================================================
// Spezifische 03b Tests
// =================================================================

TEST(CacheTraversal_LinearFanout, IsNotHashed) {
    EXPECT_FALSE(ce_03b::LinearFanout::is_hashed());
    EXPECT_FALSE(ce_03b::LinearFanout::amortized_o1());
}

TEST(CacheTraversal_HashLookup, IsHashed) {
    EXPECT_TRUE(ce_03b::HashLookup::is_hashed());
    EXPECT_TRUE(ce_03b::HashLookup::amortized_o1());
}

TEST(CacheTraversal_HashLookup, ResizesUnderLoad) {
    ce_03b::HashLookup t{};
    // Trigger resize: > 70% load factor von initial 16 = 12 entries
    for (std::uint64_t k = 0; k < 20; ++k) {
        t.register_entry(k, k * 10);
    }
    EXPECT_EQ(t.tracked_count(), 20u);
    for (std::uint64_t k = 0; k < 20; ++k) {
        auto v = t.resolve(k);
        ASSERT_TRUE(v.has_value()) << "key=" << k;
        EXPECT_EQ(*v, k * 10);
    }
}

// =================================================================
// TYPED_TEST_SUITE — axis_03m mapping
// =================================================================

using AllMappingTypes = mp::mp_apply<ToGTestTypes, ce_03m::AllStrategies>;

template <class T> class MappingTest : public ::testing::Test {};
TYPED_TEST_SUITE(MappingTest, AllMappingTypes);

TYPED_TEST(MappingTest, ConceptConformance) {
    static_assert(ce_03m::concepts::MappingVariant<TypeParam>);
    static_assert(ce_03m::concepts::CacheEngineMappingPermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(MappingTest, RegisterResolveRoundtrip) {
    TypeParam m{};
    using S = typename TypeParam::slot_index_type;
    using O = typename TypeParam::offset_type;
    m.register_slot(S{5}, O{1024});
    auto o = m.resolve_offset(S{5});
    ASSERT_TRUE(o.has_value());
    EXPECT_EQ(*o, O{1024});
    EXPECT_EQ(m.mapped_count(), 1u);
}

TYPED_TEST(MappingTest, ReverseLookupRoundtrip) {
    TypeParam m{};
    using S = typename TypeParam::slot_index_type;
    using O = typename TypeParam::offset_type;
    m.register_slot(S{7}, O{2048});
    auto s = m.reverse_lookup(O{2048});
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(*s, S{7});
}

TYPED_TEST(MappingTest, ClearMakesEmpty) {
    TypeParam m{};
    using S = typename TypeParam::slot_index_type;
    using O = typename TypeParam::offset_type;
    m.register_slot(S{1}, O{100});
    m.clear();
    EXPECT_EQ(m.mapped_count(), 0u);
}

TYPED_TEST(MappingTest, SonderfallPropertiesAccessible) {
    [[maybe_unused]] bool pr = TypeParam::is_pool_relative();
    [[maybe_unused]] bool rl = TypeParam::supports_reverse_lookup();
    [[maybe_unused]] bool pb = TypeParam::requires_pool_base();
    SUCCEED();
}

// =================================================================
// Spezifische 03m Tests
// =================================================================

TEST(Mapping_DirectPlacement, IsNotPoolRelative) {
    EXPECT_FALSE(ce_03m::DirectPlacement::is_pool_relative());
    EXPECT_FALSE(ce_03m::DirectPlacement::requires_pool_base());
}

TEST(Mapping_PoolRelative, IsPoolRelative) {
    EXPECT_TRUE(ce_03m::PoolRelative::is_pool_relative());
    EXPECT_TRUE(ce_03m::PoolRelative::requires_pool_base());
}

TEST(Mapping_PoolRelative, RebaseChangesAbsoluteOffsets) {
    ce_03m::PoolRelative m{1000};  // pool_base = 1000
    m.register_slot(5, 1500);  // relative=500
    auto o1 = m.resolve_offset(5);
    ASSERT_TRUE(o1.has_value());
    EXPECT_EQ(*o1, 1500u);
    // Rebase auf 2000
    m.rebase(2000);
    auto o2 = m.resolve_offset(5);
    ASSERT_TRUE(o2.has_value());
    EXPECT_EQ(*o2, 2500u);  // 2000 + 500 = 2500
}

TEST(Mapping_PoolRelative, PoolBaseAccessor) {
    ce_03m::PoolRelative m{4096};
    EXPECT_EQ(m.pool_base(), 4096u);
}

// =================================================================
// Sub-Concept-Konformitaet (P0-Konsolidierung 2026-05-26)
// =================================================================

TEST(SubConcepts_03a, IterableAspectSearchAlgoStrategy) {
    static_assert(ce_03a::concepts::IterableAspectSearchAlgoStrategy<ce_03a::VectorU8U8>);
    static_assert(!ce_03a::concepts::IterableAspectSearchAlgoStrategy<ce_03a::Array256>,
                  "Array256 hat keinen iterable_aspect_t");
    static_assert(!ce_03a::concepts::IterableAspectSearchAlgoStrategy<ce_03a::VectorU16U16>,
                  "VectorU16U16 hat keinen iterable_aspect_t");
    SUCCEED();
}

TEST(SubConcepts_03b, HashedTraversalStrategy) {
    static_assert(ce_03b::concepts::HashedTraversalStrategy<ce_03b::HashLookup>);
    static_assert(!ce_03b::concepts::HashedTraversalStrategy<ce_03b::LinearFanout>,
                  "LinearFanout ist nicht hashed");
    SUCCEED();
}

TEST(SubConcepts_03b, IterableAspectCacheTraversalStrategy) {
    static_assert(ce_03b::concepts::IterableAspectCacheTraversalStrategy<ce_03b::HashLookup>);
    static_assert(!ce_03b::concepts::IterableAspectCacheTraversalStrategy<ce_03b::LinearFanout>);
    SUCCEED();
}

TEST(SubConcepts_03m, PoolRebasableStrategy) {
    static_assert(ce_03m::concepts::PoolRebasableStrategy<ce_03m::PoolRelative>);
    static_assert(!ce_03m::concepts::PoolRebasableStrategy<ce_03m::DirectPlacement>,
                  "DirectPlacement hat keinen pool_base / rebase");
    SUCCEED();
}

// =================================================================
// iterable_aspect_t Verhalten (Runtime-Permutation)
// =================================================================

TEST(IterableAspect_VectorU8U8, DensityThresholdAffectsClass) {
    // 5 von 256 Slots = ~2% density; threshold 1% (klein) → Balanced
    ce_03a::VectorU8U8 s{1};
    for (int i = 0; i < 5; ++i) s.insert(static_cast<std::uint8_t>(i * 50), i + 100);
    EXPECT_EQ(s.density_class(), ce_03a::concepts::DensityClass::Balanced);

    // gleicher Buffer, threshold dynamisch hochgesetzt → Sparse
    s.set_iterable_aspect(50);
    EXPECT_EQ(s.density_class(), ce_03a::concepts::DensityClass::Sparse);
}

TEST(IterableAspect_VectorU8U8, IterableValuesContainsDefault) {
    auto vals = ce_03a::VectorU8U8::iterable_values();
    bool found_default = false;
    for (auto v : vals) {
        if (v == ce_03a::VectorU8U8::kDefaultDensityThresholdPct) found_default = true;
    }
    EXPECT_TRUE(found_default);
}

TEST(IterableAspect_HashLookup, InitialCapacityAffectsBucketCount) {
    ce_03b::HashLookup t{64};
    EXPECT_EQ(t.bucket_count(), 64u);
    ce_03b::HashLookup t2{1024};
    EXPECT_EQ(t2.bucket_count(), 1024u);
}

TEST(IterableAspect_HashLookup, SetIterableAspectRehashes) {
    ce_03b::HashLookup t{16};
    for (std::uint64_t k = 0; k < 5; ++k) t.register_entry(k, k * 10);
    t.set_iterable_aspect(256);  // Rehash auf 256 buckets
    EXPECT_EQ(t.bucket_count(), 256u);
    for (std::uint64_t k = 0; k < 5; ++k) {
        auto v = t.resolve(k);
        ASSERT_TRUE(v.has_value()) << "key=" << k << " nach rehash verloren";
        EXPECT_EQ(*v, k * 10);
    }
}

// =================================================================
// Edge-Case + Zero-Boundary-Tests (Allocator-Lessons-Learned)
// =================================================================

TEST(EdgeCase_HashLookup, ZeroCapacityThrows) {
    EXPECT_THROW(ce_03b::HashLookup{0}, std::invalid_argument);
}

TEST(EdgeCase_HashLookup, NonPowerOf2Throws) {
    EXPECT_THROW(ce_03b::HashLookup{3},   std::invalid_argument);
    EXPECT_THROW(ce_03b::HashLookup{17},  std::invalid_argument);
    EXPECT_THROW(ce_03b::HashLookup{100}, std::invalid_argument);
}

TEST(EdgeCase_HashLookup, SetIterableAspectZeroThrows) {
    ce_03b::HashLookup t{16};
    EXPECT_THROW(t.set_iterable_aspect(0), std::invalid_argument);
    EXPECT_THROW(t.set_iterable_aspect(7), std::invalid_argument);
}

TEST(EdgeCase_HashLookup, AllIterableCapacitiesAreValidPowerOf2) {
    for (auto cap : ce_03b::HashLookup::iterable_values()) {
        EXPECT_GT(cap, 0u);
        EXPECT_EQ((cap & (cap - 1)), 0u) << "cap=" << cap << " ist nicht Power-of-2";
        EXPECT_NO_THROW(ce_03b::HashLookup{cap});
    }
}

// =================================================================
// Runtime-Permutationen via constexpr-Configs (Allocator-Pattern)
// =================================================================

namespace {
struct SearchAlgoTestConfig {
    std::uint64_t key_base;
    std::size_t   count;
};

constexpr std::array<SearchAlgoTestConfig, 5> kTestSearchAlgoConfigs{{
    {0, 1}, {0, 10}, {64, 50}, {128, 100}, {200, 30},
}};
}  // namespace

TYPED_TEST(SearchAlgoTest, InsertLookupAllRuntimeConfigs) {
    for (auto const& cfg : kTestSearchAlgoConfigs) {
        TypeParam s{};
        using K = typename TypeParam::key_type;
        using V = typename TypeParam::value_type;
        for (std::size_t i = 0; i < cfg.count; ++i) {
            K k = static_cast<K>(cfg.key_base + i);
            s.insert(k, V{static_cast<V>(i + 1)});
        }
        for (std::size_t i = 0; i < cfg.count; ++i) {
            K k = static_cast<K>(cfg.key_base + i);
            auto v = s.lookup(k);
            ASSERT_TRUE(v.has_value()) << "cfg.key_base=" << cfg.key_base << " i=" << i;
            EXPECT_EQ(*v, V{static_cast<V>(i + 1)});
        }
    }
}

namespace {
constexpr std::array<std::size_t, 5> kTestHashCapacities{8u, 16u, 64u, 256u, 1024u};
}  // namespace

TEST(Runtime_HashLookup, RoundtripAllCapacities) {
    for (auto cap : kTestHashCapacities) {
        ce_03b::HashLookup t{cap};
        // fill bis ~50% load
        std::size_t fill = cap / 2;
        for (std::uint64_t k = 0; k < fill; ++k) t.register_entry(k, k * 7);
        EXPECT_EQ(t.tracked_count(), fill);
        for (std::uint64_t k = 0; k < fill; ++k) {
            auto v = t.resolve(k);
            ASSERT_TRUE(v.has_value()) << "cap=" << cap << " k=" << k;
            EXPECT_EQ(*v, k * 7);
        }
    }
}

// =================================================================
// PermutationEngine-Integration (analog Q1/Q2/Allocator-Pattern)
// =================================================================

namespace ce_perm = ::comdare::cache_engine::permutations;

// Wrapper TopicConfigSet — eine Achse pro Engine-Argument.
// Hier verwenden wir 1-Achsen-Variante mit Default-StaticAxisVariants
// (03a search_algo als primaere Achse).
struct TraversalEnginePrimaryAxisTopic {
    using StaticAxisVariants = ce_traversal::TopicConfigSet::StaticAxisVariants_03a;
};

TEST(TopicTraversal_PermutationEngine, ForEach03a) {
    using Engine = ce_perm::PermutationEngine<TraversalEnginePrimaryAxisTopic>;
    EXPECT_EQ(Engine::count(), mp::mp_size<ce_03a::EnabledStrategies>::value);
    int seen = 0;
    Engine::for_each_permutation([&seen]<class P>(){ ++seen; });
    EXPECT_EQ(seen, static_cast<int>(Engine::count()));
}

TEST(TopicTraversal_PermutationEngine, ForEachAspectPerVendor) {
    // VectorU8U8 hat iterable_aspect_t (5 density thresholds)
    int aspects = 0;
    ce_perm::for_each_aspect<ce_03a::VectorU8U8>([&aspects](auto /*value*/){ ++aspects; });
    EXPECT_EQ(aspects, 5);
    // Array256 hat KEINEN iterable_aspect_t → exakt 1 Aufruf ohne Argument
    int single = 0;
    ce_perm::for_each_aspect<ce_03a::Array256>([&single](){ ++single; });
    EXPECT_EQ(single, 1);
}

TEST(TopicTraversal_PermutationEngine, AspectCount) {
    EXPECT_EQ(ce_perm::aspect_count<ce_03a::VectorU8U8>(), 5u);
    EXPECT_EQ(ce_perm::aspect_count<ce_03a::Array256>(), 1u);
    EXPECT_EQ(ce_perm::aspect_count<ce_03b::HashLookup>(), 5u);
    EXPECT_EQ(ce_perm::aspect_count<ce_03b::LinearFanout>(), 1u);
}

// =================================================================
// Algorithmus-Eigenschaften-Filterung (mp_filter — User-Direktive
// "Algorithmen bezueglich ihrer Eigenschaften unterscheiden")
// =================================================================

template <class S> using is_simd_search_algo = mp::mp_bool<S::supports_simd()>;
template <class S> using is_dense_search_algo = mp::mp_bool<S::is_dense()>;
template <class T> using is_hashed_cache_traversal = mp::mp_bool<T::is_hashed()>;
template <class M> using is_pool_relative_mapping = mp::mp_bool<M::is_pool_relative()>;

TEST(PropertyFilter_03a, SimdCapableSubset) {
    using SimdSubset = mp::mp_filter<is_simd_search_algo, ce_03a::AllStrategies>;
    // Array256 + VectorU8U8 sind SIMD, VectorU16U16 nicht → 2/3
    EXPECT_EQ(mp::mp_size<SimdSubset>::value, 2u);
}

TEST(PropertyFilter_03a, DenseSubset) {
    using DenseSubset = mp::mp_filter<is_dense_search_algo, ce_03a::AllStrategies>;
    // Nur Array256 ist statisch dense → 1/3
    EXPECT_EQ(mp::mp_size<DenseSubset>::value, 1u);
}

TEST(PropertyFilter_03b, HashedSubset) {
    using HashedSubset = mp::mp_filter<is_hashed_cache_traversal, ce_03b::AllStrategies>;
    EXPECT_EQ(mp::mp_size<HashedSubset>::value, 1u);
}

TEST(PropertyFilter_03m, PoolRelativeSubset) {
    using PRSubset = mp::mp_filter<is_pool_relative_mapping, ce_03m::AllStrategies>;
    EXPECT_EQ(mp::mp_size<PRSubset>::value, 1u);
}
