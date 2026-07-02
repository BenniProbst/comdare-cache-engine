// DOSSIER W2/F3 #234-F3 BstShape-Varianten real: Index-Packing-Kapazitaetsschutz.
// Deutsche Regressionsanker: U32/U16-Konformitaet, Sentinel-Schutz und Level-0-Alias bleiben gekoppelt.

#include <gtest/gtest.h>

#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_ptr_size_t.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_ptr_u16.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_ptr_u32.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace ce_cmp = ::comdare::cache_engine::lookup::composable;
namespace ce_bst = ::comdare::cache_engine::nodes::axis_bst_shape;

namespace {

constexpr std::array<std::uint64_t, 8> kWideKeys{
    0ull,
    7ull,
    65535ull,
    65536ull,
    65537ull,
    (1ull << 32),
    (1ull << 40),
    std::numeric_limits<std::uint64_t>::max(),
};

[[nodiscard]] std::uint64_t value_for(std::uint64_t key, std::uint64_t salt) noexcept {
    return (key * 0x9E3779B97F4A7C15ull) ^ (salt + 0xD1B54A32D192ED03ull);
}

template <class Organ>
void expect_matches_reference(Organ const& organ, std::map<std::uint64_t, std::uint64_t> const& ref,
                              std::vector<std::uint64_t> const& probes) {
    EXPECT_EQ(organ.occupied_count(), ref.size());

    for (auto const& [key, value] : ref) {
        auto const got = organ.lookup(key);
        ASSERT_TRUE(got.has_value()) << "fehlender Key " << key;
        EXPECT_EQ(*got, value) << "Wertabweichung fuer Key " << key;
    }
    for (std::uint64_t const key : probes) {
        auto const it  = ref.find(key);
        auto const got = organ.lookup(key);
        EXPECT_EQ(got.has_value(), it != ref.end()) << "Praesenzabweichung fuer Probe " << key;
        if (it != ref.end() && got.has_value()) EXPECT_EQ(*got, it->second) << "Probe-Wertabweichung " << key;
    }

    std::set<std::uint64_t> seen;
    std::size_t const      visited = organ.for_each_record([&](std::uint64_t key, std::uint64_t value) {
        auto const [_, inserted] = seen.insert(key);
        EXPECT_TRUE(inserted) << "for_each_record besuchte Key doppelt: " << key;
        auto const it = ref.find(key);
        if (it == ref.end()) {
            ADD_FAILURE() << "for_each_record lieferte fremden Key: " << key;
            return;
        }
        EXPECT_EQ(value, it->second) << "for_each_record-Wertabweichung fuer Key " << key;
    });
    EXPECT_EQ(visited, ref.size());
    EXPECT_EQ(seen.size(), ref.size());
}

template <class Shape>
void run_bst_shape_conformance(std::size_t target_count) {
    using Organ = ce_cmp::BstTreeOrganShaped<Shape>;

    static_assert(std::is_same_v<typename Organ::key_type, std::uint64_t>);
    static_assert(std::is_same_v<typename Organ::value_type, std::uint64_t>);

    Organ                                  organ;
    std::map<std::uint64_t, std::uint64_t> ref;
    std::vector<std::uint64_t>             keys;
    keys.reserve(target_count);

    std::set<std::uint64_t> generated;
    for (std::uint64_t key : kWideKeys) {
        generated.insert(key);
        keys.push_back(key);
    }

    std::mt19937_64 rng{0x234F3B57A6E5D00Dull ^ static_cast<std::uint64_t>(target_count)};
    while (generated.size() < target_count) {
        std::uint64_t const candidate = rng();
        if (generated.insert(candidate).second) keys.push_back(candidate);
    }

    std::vector<std::uint64_t> probes{kWideKeys.begin(), kWideKeys.end()};
    probes.insert(probes.end(), {1ull, 8ull, 65534ull, 65538ull, (1ull << 32) + 1ull, (1ull << 40) + 1ull,
                                 std::numeric_limits<std::uint64_t>::max() - 1ull});
    for (std::size_t i = 0; i < 128u; ++i) probes.push_back(rng() ^ (static_cast<std::uint64_t>(i) << 48));

    for (std::size_t i = 0; i < keys.size(); ++i) {
        std::uint64_t const key = keys[i];
        std::uint64_t       val = value_for(key, i);
        organ.insert(key, val);
        ref[key] = val;

        if (i % 97u == 0u) {
            val = value_for(key, i + 0xF300u);
            organ.insert(key, val);
            ref[key] = val;
        }
        if (i % 211u == 0u) {
            auto const got = organ.lookup(key);
            ASSERT_TRUE(got.has_value()) << "Lookup direkt nach insert/update fehlgeschlagen fuer " << key;
            EXPECT_EQ(*got, ref[key]);
        }
    }
    expect_matches_reference(organ, ref, probes);

    std::vector<std::uint64_t> erased;
    for (std::size_t i = 0; i < keys.size(); i += 11u) {
        std::uint64_t const key = keys[i];
        bool const          had = (ref.erase(key) != 0u);
        EXPECT_EQ(organ.erase(key), had) << "erase-Abweichung fuer Key " << key;
        if (had) erased.push_back(key);
        EXPECT_FALSE(organ.lookup(key).has_value()) << "geloeschter Key noch auffindbar: " << key;
    }
    for (std::uint64_t key : probes) {
        if (ref.find(key) == ref.end()) EXPECT_FALSE(organ.erase(key)) << "erase-Miss muss false liefern: " << key;
    }
    expect_matches_reference(organ, ref, probes);

    for (std::size_t i = 0; i < erased.size(); i += 2u) {
        std::uint64_t const key = erased[i];
        std::uint64_t const val = value_for(key, i + 0x234F3000ull);
        organ.insert(key, val);
        ref[key] = val;
    }
    expect_matches_reference(organ, ref, probes);

    organ.clear();
    ref.clear();
    EXPECT_EQ(organ.occupied_count(), 0u);
    EXPECT_EQ(organ.for_each_record([](std::uint64_t, std::uint64_t) { ADD_FAILURE() << "clear liess Record stehen"; }),
              0u);
    expect_matches_reference(organ, ref, probes);

    std::vector<std::uint64_t> reset_keys{kWideKeys.begin(), kWideKeys.end()};
    std::reverse(reset_keys.begin(), reset_keys.end());
    for (std::size_t i = 0; i < reset_keys.size(); ++i) {
        std::uint64_t const key = reset_keys[i];
        std::uint64_t const val = value_for(key, i + 0xC1EA0000ull);
        organ.insert(key, val);
        ref[key] = val;
    }
    expect_matches_reference(organ, ref, probes);
}

} // namespace

// ============================================================================
// #234-F3 U32-Konformitaet gegen std::map ueber breite Keys + Zufallsstrom
// ============================================================================

TEST(Comdare234F3BstShape, BstPtrU32MatchesStdMapWithWideAndRandomKeys) {
    run_bst_shape_conformance<ce_bst::BstPtrU32>(6000u); // #234-F3: U32 >= 5000 Keys
}

// ============================================================================
// #234-F3 U16-Konformitaet gegen std::map, bewusst unter Sentinel-Kapazitaet
// ============================================================================

TEST(Comdare234F3BstShape, BstPtrU16MatchesStdMapWithWideAndRandomKeys) {
    run_bst_shape_conformance<ce_bst::BstPtrU16>(30000u); // #234-F3: U16 <= 30000 Keys
}

// ============================================================================
// #234-F3 Packing- und Sentinel-Selbstbeweise ohne Zugriff auf private Node
// ============================================================================

TEST(Comdare234F3BstShape, PackingStaticAssertsAndNilRelations) {
    using StoreSizeT = ce_cmp::TreeNodePoolStore<ce_bst::BstPtrSizeT>;
    using StoreU32   = ce_cmp::TreeNodePoolStore<ce_bst::BstPtrU32>;
    using StoreU16   = ce_cmp::TreeNodePoolStore<ce_bst::BstPtrU16>;

    static_assert(std::is_same_v<typename StoreSizeT::index_type, std::size_t>);
    static_assert(std::is_same_v<typename StoreU32::index_type, std::uint32_t>);
    static_assert(std::is_same_v<typename StoreU16::index_type, std::uint16_t>);
    static_assert(ce_bst::BstPtrSizeT::kIndexBytes == sizeof(std::size_t));
    static_assert(ce_bst::BstPtrU32::kIndexBytes == sizeof(std::uint32_t));
    static_assert(ce_bst::BstPtrU16::kIndexBytes == sizeof(std::uint16_t));
    static_assert(StoreU32::kNilIndex == std::numeric_limits<std::uint32_t>::max());
    static_assert(StoreU16::kNilIndex == std::numeric_limits<std::uint16_t>::max());
    static_assert(StoreU32::kNil == static_cast<std::size_t>(StoreU32::kNilIndex));
    static_assert(StoreU16::kNil == static_cast<std::size_t>(StoreU16::kNilIndex));
    static_assert(StoreU16::kNil == 65535u);

    SUCCEED();
}

// ============================================================================
// #234-F3 U16-Append-Ueberlauf wirft, Free-List-Recycling bleibt erlaubt
// ============================================================================

TEST(Comdare234F3BstShape, U16OverflowThrowsAndFreeListRecycleStillWorks) {
    using Store = ce_cmp::TreeNodePoolStore<ce_bst::BstPtrU16>;

    Store store;
    constexpr std::size_t kSentinel = static_cast<std::size_t>(Store::kNilIndex);
    for (std::size_t i = 0; i < kSentinel; ++i) {
        std::size_t const idx = store.allocate_node(static_cast<std::uint64_t>(i), static_cast<std::uint64_t>(i + 1u));
        ASSERT_EQ(idx, i);
    }
    ASSERT_EQ(store.node_count(), kSentinel);
    ASSERT_EQ(store.node_key(kSentinel - 1u), static_cast<std::uint64_t>(kSentinel - 1u));

    EXPECT_THROW({ (void)store.allocate_node(0xDEADull, 0xBEEFull); }, std::length_error);

    store.free_node(42u);
    ASSERT_EQ(store.node_count(), kSentinel - 1u);
    std::size_t const recycled = store.allocate_node(0xDEADull, 0xBEEFull);
    EXPECT_EQ(recycled, 42u);
    EXPECT_EQ(store.node_count(), kSentinel);
    EXPECT_EQ(store.node_key(recycled), 0xDEADull);
    EXPECT_EQ(store.node_value(recycled), 0xBEEFull);
}

// ============================================================================
// #234-F3 kNil-Konsistenz fuer frischen U16-Store und Ein-Knoten-Baum
// ============================================================================

TEST(Comdare234F3BstShape, U16NilConsistencyForFreshStoreAndSingleRoot) {
    using Store = ce_cmp::TreeNodePoolStore<ce_bst::BstPtrU16>;
    using Organ = ce_cmp::BstTreeOrganShaped<ce_bst::BstPtrU16>;

    Store store;
    EXPECT_EQ(store.root(), Store::kNil);

    Organ organ;
    organ.insert(42u, 84u);
    auto const& pool = organ.pool();
    ASSERT_NE(pool.root(), Store::kNil);
    EXPECT_EQ(pool.left(pool.root()), Store::kNil);
    EXPECT_EQ(pool.right(pool.root()), Store::kNil);
}

// ============================================================================
// #234-F3 Level-0-Regressionsanker: Default bleibt size_t-Shape
// ============================================================================

TEST(Comdare234F3BstShape, Level0BstTreeOrganAliasUsesSizeTShape) {
    static_assert(std::is_same_v<ce_cmp::BstTreeOrgan, ce_cmp::BstTreeOrganShaped<ce_bst::BstPtrSizeT>>);
    SUCCEED();
}