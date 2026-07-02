// #234-F1 (2026-07-02) -- BTree-Order-Shape: Verhaltens- und Strukturanker fuer DOSSIER W2/F1.
// GTest im Nachbar-Stil von test_188_4bb0_pool_organ_wide_key_conformance.cpp:
// direkter Organ-Drive gegen std::map<uint64,uint64>, weiter Key-Raum, for_each_record EXACTLY-ONCE.
//
// **Warum:** W1/#234-K hat den BTreeNodePoolStore ueber axis_btree_order Shapes parametrisiert. Dieses
// Test-only-Increment belegt, dass die vier nicht-Level-0 Shapes (Kt2/Kt3/Kt8/Kt16) nicht nur typseitig
// instanziieren, sondern dieselbe std::map-Semantik ueber weite uint64-Keys tragen. Zusaetzlich zeigt ein
// Struktur-Wirkungsanker an gleicher 200-Key-Sequenz, dass Kt2 und Kt16 tatsaechlich andere Baumformen
// erzeugen (mehr erreichbare Knoten bei kleinerem kMaxKeys), ohne Flags oder Runtime-Auswahl.
//
// Additiv/gate-frei: beruehrt weder CMake noch Header noch bestehende Tests; prueft nur die public APIs
// BTreeSearchOrganShaped<Shape> und BTreeNodePoolStore<Shape>.

#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt16.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt2.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt3.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt4.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt8.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <type_traits>
#include <vector>

namespace {

namespace ord = ::comdare::cache_engine::nodes::axis_btree_order;
namespace lkc = ::comdare::cache_engine::lookup::composable;

using U64 = std::uint64_t;

// Level-0-Regressions-Anker: der Default-Organ-Alias muss exakt die Kt4-Shape-Komposition bleiben.
static_assert(std::is_same_v<lkc::BTreeSearchOrgan, lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt4>>,
              "#234-F1: BTreeSearchOrgan muss Level-0/Kt4 bleiben");

// Struktur-Shape-Beleg: kMaxKeys folgt direkt aus dem Minimum-Degree t.
static_assert(lkc::BTreeNodePoolStore<ord::BtreeOrderKt2>::kMaxKeys == 3, "#234-F1: Kt2 muss 3 Keys je Knoten tragen");
static_assert(lkc::BTreeNodePoolStore<ord::BtreeOrderKt3>::kMaxKeys == 5, "#234-F1: Kt3 muss 5 Keys je Knoten tragen");
static_assert(lkc::BTreeNodePoolStore<ord::BtreeOrderKt8>::kMaxKeys == 15,
              "#234-F1: Kt8 muss 15 Keys je Knoten tragen");
static_assert(lkc::BTreeNodePoolStore<ord::BtreeOrderKt16>::kMaxKeys == 31,
              "#234-F1: Kt16 muss 31 Keys je Knoten tragen");

static_assert(std::is_same_v<typename lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt2>::key_type, U64>);
static_assert(std::is_same_v<typename lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt16>::value_type, U64>);

[[nodiscard]] U64 val_for(U64 key, U64 salt) noexcept {
    return (key ^ 0x9E3779B97F4A7C15ull) + (salt * 0xD1B54A32D192ED03ull);
}

[[nodiscard]] std::vector<U64> fixed_wide_keys() {
    return {0u, 7u, 65535u, 65536u, 65537u, (1ull << 32), (1ull << 40), std::numeric_limits<U64>::max()};
}

// Weiter Key-Raum: Pflicht-Keys plus mindestens 5000 deterministische random-wide Keys.
[[nodiscard]] std::vector<U64> wide_key_sequence() {
    std::vector<U64> keys;
    std::set<U64>    seen;
    auto const       add = [&](U64 key) {
        if (seen.insert(key).second) keys.push_back(key);
    };

    for (U64 key : fixed_wide_keys()) add(key);

    std::set<U64> const reserved_misses{1u,
                                        2u,
                                        42u,
                                        999u,
                                        131072u,
                                        (1ull << 20),
                                        (1ull << 33),
                                        (1ull << 48),
                                        0x00000000FFFFFFFFull,
                                        0xFFFFFFFFFFFF0000ull,
                                        0xDEADBEEFCAFEBABEull};
    std::mt19937_64     rng{0x234F1B7EED5EEDull};
    std::size_t         random_unique = 0;
    while (random_unique < 5000u) {
        U64 const key = rng();
        if (reserved_misses.find(key) != reserved_misses.end()) continue;
        if (seen.insert(key).second) {
            keys.push_back(key);
            ++random_unique;
        }
    }

    return keys;
}

[[nodiscard]] std::vector<U64> miss_key_sequence() {
    return {1u,
            2u,
            42u,
            999u,
            131072u,
            (1ull << 20),
            (1ull << 33),
            (1ull << 48),
            0x00000000FFFFFFFFull,
            0xFFFFFFFFFFFF0000ull,
            0xDEADBEEFCAFEBABEull};
}

template <class Organ>
void expect_lookup_equals(char const* name, Organ const& organ, std::map<U64, U64> const& oracle, U64 key,
                          char const* phase) {
    std::optional<U64> const actual = organ.lookup(key);
    auto const               it     = oracle.find(key);
    if (it == oracle.end()) {
        EXPECT_FALSE(actual.has_value()) << name << ": lookup Miss erwartet key=" << key << " phase=" << phase;
        return;
    }
    ASSERT_TRUE(actual.has_value()) << name << ": lookup Treffer erwartet key=" << key << " phase=" << phase;
    EXPECT_EQ(*actual, it->second) << name << ": lookup Wert key=" << key << " phase=" << phase;
}

template <class Organ>
void expect_lookup_samples(char const* name, Organ const& organ, std::map<U64, U64> const& oracle,
                           std::vector<U64> const& keys, char const* phase) {
    if (!keys.empty()) {
        expect_lookup_equals(name, organ, oracle, keys.front(), phase);
        expect_lookup_equals(name, organ, oracle, keys[keys.size() / 2u], phase);
        expect_lookup_equals(name, organ, oracle, keys.back(), phase);
        for (std::size_t i = 0; i < keys.size(); i += 257u) expect_lookup_equals(name, organ, oracle, keys[i], phase);
    }
    for (U64 miss : miss_key_sequence()) expect_lookup_equals(name, organ, oracle, miss, phase);
}

template <class Organ>
void expect_for_each_exactly_once(char const* name, Organ const& organ, std::map<U64, U64> const& oracle,
                                  char const* phase) {
    std::map<U64, U64> harvested;
    std::set<U64>      seen;

    std::size_t const visits = organ.for_each_record([&](U64 key, U64 value) {
        auto const oracle_it = oracle.find(key);
        EXPECT_NE(oracle_it, oracle.end()) << name << ": for_each_record Fremd-Key=" << key << " phase=" << phase;
        if (oracle_it != oracle.end()) {
            EXPECT_EQ(value, oracle_it->second)
                << name << ": for_each_record Wert-Drift key=" << key << " phase=" << phase;
        }

        bool const first_seen = seen.insert(key).second;
        EXPECT_TRUE(first_seen) << name << ": for_each_record Duplicate key=" << key << " phase=" << phase;
        bool const harvested_new = harvested.emplace(key, value).second;
        EXPECT_TRUE(harvested_new) << name << ": harvested Duplicate key=" << key << " phase=" << phase;
    });

    EXPECT_EQ(visits, oracle.size()) << name << ": for_each_record Rueckgabe phase=" << phase;
    EXPECT_EQ(visits, organ.occupied_count()) << name << ": for_each_record vs occupied_count phase=" << phase;
    EXPECT_EQ(seen.size(), oracle.size()) << name << ": seen-set Groesse phase=" << phase;
    EXPECT_EQ(harvested, oracle) << name << ": for_each_record Paare phase=" << phase;
}

template <class Organ>
void expect_phase_matches(char const* name, Organ const& organ, std::map<U64, U64> const& oracle,
                          std::vector<U64> const& keys, char const* phase) {
    EXPECT_EQ(organ.occupied_count(), oracle.size()) << name << ": size-Gleichheit phase=" << phase;
    expect_lookup_samples(name, organ, oracle, keys, phase);
    expect_for_each_exactly_once(name, organ, oracle, phase);
}

template <class Shape>
void drive_shape_against_std_map(char const* name) {
    using Organ = lkc::BTreeSearchOrganShaped<Shape>;
    using Store = lkc::BTreeNodePoolStore<Shape>;
    static_assert(std::is_same_v<typename Organ::key_type, U64>);
    static_assert(std::is_same_v<typename Organ::value_type, U64>);
    static_assert(std::is_same_v<typename Store::key_type, U64>);
    static_assert(Store::kMaxKeys == Shape::kMaxKeys);

    Organ              organ;
    std::map<U64, U64> oracle;
    auto const         keys = wide_key_sequence();

    expect_phase_matches(name, organ, oracle, keys, "leer");

    // Phase 1: Insert ueber den kompletten weiten Key-Raum.
    for (U64 key : keys) {
        organ.insert(key, val_for(key, 1u));
        oracle.insert_or_assign(key, val_for(key, 1u));
    }
    expect_phase_matches(name, organ, oracle, keys, "nach insert");

    // Phase 2: Updates auf existierenden Keys duerfen die Groesse nicht veraendern.
    std::size_t const before_update_size = oracle.size();
    for (std::size_t i = 0; i < keys.size(); i += 3u) {
        U64 const key = keys[i];
        organ.insert(key, val_for(key, 2u));
        oracle.insert_or_assign(key, val_for(key, 2u));
    }
    EXPECT_EQ(oracle.size(), before_update_size) << name << ": Oracle-Groesse nach update";
    expect_phase_matches(name, organ, oracle, keys, "nach update");

    // Phase 3: Lookup-Sweep ohne Zustandsaenderung, inklusive Misses.
    for (std::size_t i = 0; i < keys.size(); i += 97u)
        expect_lookup_equals(name, organ, oracle, keys[i], "lookup sweep");
    for (U64 miss : miss_key_sequence()) expect_lookup_equals(name, organ, oracle, miss, "lookup sweep");
    expect_phase_matches(name, organ, oracle, keys, "nach lookup sweep");

    // Phase 4: Erase auf vorhandenen und bewusst fehlenden Keys, paargenau gegen std::map.
    for (std::size_t i = 0; i < keys.size(); i += 4u) {
        U64 const  key        = keys[i];
        bool const org_erased = organ.erase(key);
        bool const map_erased = (oracle.erase(key) != 0u);
        EXPECT_EQ(org_erased, map_erased) << name << ": erase Rueckgabe key=" << key;
    }
    for (U64 miss : miss_key_sequence()) {
        bool const org_erased = organ.erase(miss);
        bool const map_erased = (oracle.erase(miss) != 0u);
        EXPECT_EQ(org_erased, map_erased) << name << ": erase Miss-Rueckgabe key=" << miss;
    }
    expect_phase_matches(name, organ, oracle, keys, "nach erase");

    // Phase 5: Gemischte Reinsert/Update-Runde ohne Runtime-Shape-Switch.
    for (std::size_t i = 1; i < keys.size(); i += 5u) {
        U64 const key = keys[i];
        organ.insert(key, val_for(key, 3u));
        oracle.insert_or_assign(key, val_for(key, 3u));
    }
    for (std::size_t i = 2; i < keys.size(); i += 11u) {
        U64 const key = keys[i];
        expect_lookup_equals(name, organ, oracle, key, "mixed lookup");
        if ((i / 11u) % 2u == 0u) {
            bool const org_erased = organ.erase(key);
            bool const map_erased = (oracle.erase(key) != 0u);
            EXPECT_EQ(org_erased, map_erased) << name << ": mixed erase key=" << key;
        } else {
            organ.insert(key, val_for(key, 4u));
            oracle.insert_or_assign(key, val_for(key, 4u));
        }
    }
    expect_phase_matches(name, organ, oracle, keys, "nach mixed");

    // Phase 6: Clear muss den Store vollstaendig leeren und danach wiederverwendbar lassen.
    organ.clear();
    oracle.clear();
    expect_phase_matches(name, organ, oracle, keys, "nach clear");

    std::vector<U64> post_clear_keys;
    for (std::size_t i = 0; i < 128u; ++i) {
        U64 const key = keys[keys.size() - 1u - i];
        post_clear_keys.push_back(key);
        organ.insert(key, val_for(key, 5u));
        oracle.insert_or_assign(key, val_for(key, 5u));
    }
    expect_phase_matches(name, organ, oracle, post_clear_keys, "nach post-clear insert");
}

template <class Store>
std::size_t count_reachable_nodes(Store const& store, std::size_t node, std::set<std::size_t>& seen) {
    if (node == Store::kNil) return 0u;

    bool const first_seen = seen.insert(node).second;
    EXPECT_TRUE(first_seen) << "#234-F1: Store-Knoten doppelt erreicht node=" << node;
    if (!first_seen) return 0u;

    int const n = store.node_n(node);
    EXPECT_GE(n, 0) << "#234-F1: negativer node_n";
    EXPECT_LE(n, Store::kMaxKeys) << "#234-F1: node_n ueber kMaxKeys";

    if (store.node_leaf(node)) return 1u;

    std::size_t total = 1u;
    for (int child_slot = 0; child_slot <= n; ++child_slot) {
        std::size_t const child = store.node_child_at(node, child_slot);
        EXPECT_NE(child, Store::kNil) << "#234-F1: innerer Knoten mit NIL-Kind slot=" << child_slot;
        total += count_reachable_nodes(store, child, seen);
    }
    return total;
}

template <class Store>
std::size_t count_reachable_nodes(Store const& store) {
    std::set<std::size_t> seen;
    return count_reachable_nodes(store, store.root(), seen);
}

[[nodiscard]] std::vector<U64> structure_probe_sequence() {
    std::vector<U64> keys;
    keys.reserve(200u);
    for (U64 i = 0; i < 200u; ++i) keys.push_back((i * 7919u) ^ (i << 32));
    return keys;
}

} // namespace

TEST(Comdare234F1BtreeOrderShape, Kt2MatchesStdMapWideKeys) {
    drive_shape_against_std_map<ord::BtreeOrderKt2>("BtreeOrderKt2");
}

TEST(Comdare234F1BtreeOrderShape, Kt3MatchesStdMapWideKeys) {
    drive_shape_against_std_map<ord::BtreeOrderKt3>("BtreeOrderKt3");
}

TEST(Comdare234F1BtreeOrderShape, Kt8MatchesStdMapWideKeys) {
    drive_shape_against_std_map<ord::BtreeOrderKt8>("BtreeOrderKt8");
}

TEST(Comdare234F1BtreeOrderShape, Kt16MatchesStdMapWideKeys) {
    drive_shape_against_std_map<ord::BtreeOrderKt16>("BtreeOrderKt16");
}

TEST(Comdare234F1BtreeOrderShape, Kt2AndKt16ProduceDifferentPublicStoreShape) {
    using StoreKt2  = lkc::BTreeNodePoolStore<ord::BtreeOrderKt2>;
    using StoreKt16 = lkc::BTreeNodePoolStore<ord::BtreeOrderKt16>;
    using OrganKt2  = lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt2>;
    using OrganKt16 = lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt16>;

    static_assert(StoreKt2::kMaxKeys == 3);
    static_assert(StoreKt16::kMaxKeys == 31);

    OrganKt2  kt2;
    OrganKt16 kt16;
    for (U64 key : structure_probe_sequence()) {
        kt2.insert(key, val_for(key, 6u));
        kt16.insert(key, val_for(key, 6u));
    }

    StoreKt2 const&  pool2  = kt2.pool();
    StoreKt16 const& pool16 = kt16.pool();
    ASSERT_NE(pool2.root(), StoreKt2::kNil);
    ASSERT_NE(pool16.root(), StoreKt16::kNil);
    EXPECT_EQ(kt2.occupied_count(), 200u);
    EXPECT_EQ(kt16.occupied_count(), 200u);

    std::size_t const kt2_nodes  = count_reachable_nodes(pool2);
    std::size_t const kt16_nodes = count_reachable_nodes(pool16);

    EXPECT_GT(kt2_nodes, kt16_nodes) << "#234-F1: kleineres Kt2-Fanout muss bei gleicher Sequenz mehr Knoten bilden";
    EXPECT_LE(pool2.node_n(pool2.root()), StoreKt2::kMaxKeys);
    EXPECT_LE(pool16.node_n(pool16.root()), StoreKt16::kMaxKeys);
}
