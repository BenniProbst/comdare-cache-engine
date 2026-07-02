// #188-4c-ii (2026-07-02) -- Faithful Traversal-Organe fuer die 4 markerlosen Flach-Wrapper.
//
// Neuer Gate-Test: je ein store-backed AdHoc-Tier pro Wrapper, getrieben ueber den realen
// SearchAlgorithmAbiAdapter. Belegt std::map-Konformitaet, Store-Routing und Size/Clear/Reuse inklusive
// breiter uint64-Keys (K9-d-Truncation darf im Store-Pfad nicht mehr auftreten).

#include <anatomy/composition_concept.hpp>
#include <anatomy/composition_factory.hpp>
#include <axes/lookup/axis_03a_search_algo_array256.hpp>
#include <axes/lookup/axis_03a_search_algo_array65535.hpp>
#include <axes/lookup/axis_03a_search_algo_vector_u8u8.hpp>
#include <axes/lookup/axis_03a_search_algo_vector_u16u16.hpp>
#include <axes/lookup/composable/direct_address_traversal_organ.hpp> // Review-F5: organ-scharfe Sparse-Proben
#include <axes/lookup/composable/store_traversable_search_algo.hpp> // Review-B1: Concept UNGEGATED fuer die static_asserts
#include <axes/lookup/composable/traversal_for_search_algo.hpp>
#include <compositions/art_reference.hpp>

#if COMDARE_MEASUREMENT_ON
#include <anatomy/abi_adapter.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/pruef_dock/conformance_gate.hpp>
#endif

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace sa   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

template <class SearchAlgo>
using StoreBackedAdHocComposition = an::AdHocComposition<
    SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::telemetry, comp::ArtComposition::value_handle,
    comp::ArtComposition::isa, comp::ArtComposition::index_organization, comp::ArtComposition::io_dispatch,
    comp::ArtComposition::migration_policy, comp::ArtComposition::filter, comp::ArtComposition::queuing_q1,
    comp::ArtComposition::queuing_q2>;

using FlatWrapperSearchAlgos1884cii = ::testing::Types<sa::Array256SearchAlgo, sa::Array65535SearchAlgo,
                                                       sa::VectorU8U8SearchAlgo, sa::VectorU16U16SearchAlgo>;

inline constexpr std::uint64_t                kNoFailure            = 0u;
inline constexpr std::uint64_t                kConformanceSeed      = 42u;
inline constexpr std::uint64_t                kConformanceRandomOps = 2000u;
inline constexpr std::uint64_t                kLookupSentinel       = 0xBAD0C0DEu;
inline constexpr std::uint64_t                kValueSalt            = 0x9E3779B97F4A7C15ull;
inline constexpr std::array<std::uint64_t, 6> kWideKeys{7u, 255u, 256u, 511u, 65'536u, 1ull << 40};
inline constexpr std::array<std::uint64_t, 4> kReuseKeys{4'096u, 65'535u, 65'537u, (1ull << 40) + 17u};

template <class SearchAlgo>
std::string label_for() {
    return std::string{std::string_view{SearchAlgo::name()}};
}

[[nodiscard]] std::uint64_t value_for(std::uint64_t key) noexcept { return key ^ kValueSalt; }

static_assert(lkc::StoreTraversableSearchAlgo<sa::Array256SearchAlgo>);
static_assert(lkc::StoreTraversableSearchAlgo<sa::Array65535SearchAlgo>);
static_assert(lkc::StoreTraversableSearchAlgo<sa::VectorU8U8SearchAlgo>);
static_assert(lkc::StoreTraversableSearchAlgo<sa::VectorU16U16SearchAlgo>);
static_assert(std::is_same_v<lkc::traversal_for_search_algo_t<sa::Array256SearchAlgo>, lkc::DirectAddressTraversal>);
static_assert(std::is_same_v<lkc::traversal_for_search_algo_t<sa::Array65535SearchAlgo>, lkc::DirectAddressTraversal>);
static_assert(std::is_same_v<lkc::traversal_for_search_algo_t<sa::VectorU8U8SearchAlgo>, lkc::SortedVectorTraversal>);
static_assert(std::is_same_v<lkc::traversal_for_search_algo_t<sa::VectorU16U16SearchAlgo>, lkc::SortedVectorTraversal>);

} // namespace

// #188-4c-ii Review-F5 (2026-07-02): Der lokale Korrektur-Pfad der Direktadress-SCHAETZUNG wird explizit ueber
// einen SPARSE Store (Luecken im Keyraum) belegt — organ-scharf ueber RawSlotStore, OHNE Adapter (laeuft daher
// auch im Measurement-OFF-Build). Nach aufsteigenden Inserts landet die geklemmte Schaetzung fuer alle drei
// Keys rechts (offset >= n) und MUSS links auf den exakten Slot korrigieren; Misses decken Luecken + Raender.
TEST(DirectAddressTraversal1884cii, SparseCorrectionOnGappyStore) {
    lkc::RawSlotStore                      s{};
    constexpr std::array<std::uint64_t, 3> gappy{10u, 100u, 200u};
    for (std::uint64_t const k : gappy) lkc::DirectAddressTraversal::insert_into(s, k, value_for(k));
    for (std::uint64_t const k : gappy) {
        auto const v = lkc::DirectAddressTraversal::lookup_in(s, k);
        ASSERT_TRUE(v.has_value()) << "sparse hit key=" << k;
        EXPECT_EQ(*v, value_for(k)) << "sparse value key=" << k;
    }
    constexpr std::array<std::uint64_t, 7> misses{5u, 50u, 99u, 101u, 199u, 201u, 300u};
    for (std::uint64_t const k : misses)
        EXPECT_FALSE(lkc::DirectAddressTraversal::lookup_in(s, k).has_value()) << "sparse miss key=" << k;
    EXPECT_TRUE(lkc::DirectAddressTraversal::erase_from(s, 100u));
    EXPECT_FALSE(lkc::DirectAddressTraversal::lookup_in(s, 100u).has_value()) << "erased key darf nicht treffen";
}

#if COMDARE_MEASUREMENT_ON

namespace dock = ::comdare::cache_engine::builder::pruef_dock;

template <class SearchAlgo>
using AdapterFor = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<StoreBackedAdHocComposition<SearchAlgo>>>;

template <class SearchAlgo>
class FlatWrapperTraversal1884cii : public ::testing::Test {};

TYPED_TEST_SUITE(FlatWrapperTraversal1884cii, FlatWrapperSearchAlgos1884cii);

TYPED_TEST(FlatWrapperTraversal1884cii, PassesStdMapConformanceGate) {
    using SearchAlgo = TypeParam;
    using Adapter    = AdapterFor<SearchAlgo>;

    SCOPED_TRACE(label_for<SearchAlgo>());

    auto  tier = std::make_unique<Adapter>();
    auto& drv  = static_cast<an::IDriveableTier&>(*tier);

    auto const result = dock::run_conformance_gate(drv, kConformanceSeed, kConformanceRandomOps);
    EXPECT_TRUE(result.passed()) << label_for<SearchAlgo>() << " cases=" << result.cases_passed << "/"
                                 << result.cases_total << " first_fail=" << result.first_fail
                                 << " (1-basiert, 0 = keine Verletzung)";
    EXPECT_GT(result.cases_total, kNoFailure) << label_for<SearchAlgo>() << " Gate wurde nicht getrieben";
    EXPECT_EQ(result.first_fail, kNoFailure) << label_for<SearchAlgo>() << " erste Gate-Verletzung";
}

TYPED_TEST(FlatWrapperTraversal1884cii, RoutesSearchThroughStore) {
    using SearchAlgo = TypeParam;
    using Adapter    = AdapterFor<SearchAlgo>;

    constexpr bool routes_through_store = Adapter::tier_search_routes_through_store();
    ::testing::Test::RecordProperty(label_for<SearchAlgo>() + "_routes_through_store",
                                    routes_through_store ? "true" : "false");
    EXPECT_TRUE(routes_through_store) << label_for<SearchAlgo>();
}

TYPED_TEST(FlatWrapperTraversal1884cii, SizeClearReuseRoundTripWithWideKeys) {
    using SearchAlgo = TypeParam;
    using Adapter    = AdapterFor<SearchAlgo>;

    SCOPED_TRACE(label_for<SearchAlgo>());

    auto  tier = std::make_unique<Adapter>();
    auto& drv  = static_cast<an::IDriveableTier&>(*tier);
    drv.tier_clear();

    for (std::uint64_t const key : kWideKeys) {
        std::uint64_t const expected = value_for(key);
        EXPECT_TRUE(drv.tier_insert(key, expected)) << label_for<SearchAlgo>() << " insert key=" << key;

        std::uint64_t actual = kLookupSentinel;
        EXPECT_TRUE(drv.tier_lookup(key, &actual)) << label_for<SearchAlgo>() << " lookup key=" << key;
        EXPECT_EQ(actual, expected) << label_for<SearchAlgo>() << " value key=" << key;
    }
    EXPECT_EQ(drv.tier_size(), kWideKeys.size()) << label_for<SearchAlgo>() << " size nach Wide-Key-Inserts";

    std::uint64_t const update_key = kWideKeys.back();
    std::uint64_t const updated    = value_for(update_key) ^ 0x55u;
    EXPECT_FALSE(drv.tier_insert(update_key, updated)) << label_for<SearchAlgo>() << " update ist kein neuer Key";
    std::uint64_t actual = kLookupSentinel;
    EXPECT_TRUE(drv.tier_lookup(update_key, &actual));
    EXPECT_EQ(actual, updated) << label_for<SearchAlgo>() << " update value";
    EXPECT_EQ(drv.tier_size(), kWideKeys.size()) << label_for<SearchAlgo>() << " size nach Update";

    drv.tier_clear();
    EXPECT_EQ(drv.tier_size(), kNoFailure) << label_for<SearchAlgo>() << " size nach clear";
    for (std::uint64_t const key : kWideKeys) {
        actual = kLookupSentinel;
        EXPECT_FALSE(drv.tier_lookup(key, &actual)) << label_for<SearchAlgo>() << " stale key nach clear=" << key;
    }

    for (std::uint64_t const key : kReuseKeys) {
        std::uint64_t const expected = value_for(key);
        EXPECT_TRUE(drv.tier_insert(key, expected)) << label_for<SearchAlgo>() << " reuse insert key=" << key;
        actual = kLookupSentinel;
        EXPECT_TRUE(drv.tier_lookup(key, &actual)) << label_for<SearchAlgo>() << " reuse lookup key=" << key;
        EXPECT_EQ(actual, expected) << label_for<SearchAlgo>() << " reuse value key=" << key;
    }
    EXPECT_EQ(drv.tier_size(), kReuseKeys.size()) << label_for<SearchAlgo>() << " size nach Wiederverwendung";
}

#else

TEST(FlatWrapperTraversal1884cii, AdapterGateRequiresMeasurementBuild) {
    GTEST_SKIP() << "COMDARE_MEASUREMENT_ON=1 ist fuer Adapter-/Diagnosepfade erforderlich.";
}

#endif // COMDARE_MEASUREMENT_ON
