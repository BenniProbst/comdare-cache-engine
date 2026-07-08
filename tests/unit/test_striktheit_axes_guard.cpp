// Prospektiver Metaprogrammierungs-Striktheits-Guard, Teil 2 (#50, Organ-Ebene).
//
// **Zweck:** ergaenzt test_striktheit_metaprog_guard (11 Referenz-Anatomien = Tier-Ebene) um die
// ORGAN-Ebene: JEDER Strategie-Marker JEDER der 26 Achsen (die permutierbaren Organe) ist CRTP+Concept
// und traegt KEINE vtable — der Hot-Path-Dispatch ist compile-time (kein Runtime-Switch). Prospektiv:
// ein versehentlich polymorpher Achsen-Marker bricht ab jetzt Build/ctest.
//
// **Separate Datei (bewusst):** registry_to_axis_levels.hpp zieht die 23 topic_config_sets (schwerer
// Header). Isoliert gehalten, damit der Kern-Guard test_striktheit_metaprog_guard leicht+schnell bleibt
// und ein etwaiger Cold-Cache-ICE (#21/K87b, last-korreliert) hier isoliert retriggerbar ist.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner static_assert-Test ueber die bestehenden axes26-Enabled-Listen
// (registry_to_axis_levels.hpp). KEINE Aenderung an Achsen/Registries/golden/permutation_axes/ABI.

#include <builder/experiment_tree/registry_to_axis_levels.hpp> // axes26::T00..T25 (26 Achsen-Enabled-Listen)

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

namespace ax = ::comdare::cache_engine::builder::experiment::axes26;
namespace mp = ::boost::mp11;

// Flach-Aggregat aller 26 Achsen-Enabled-Listen = die Strategie-Marker aller permutierbaren Organe.
using AllAxisStrategies =
    mp::mp_append<ax::T00_search_algo, ax::T01_cache_traversal, ax::T02_mapping, ax::T03_path_compression,
                  ax::T04_node_type, ax::T05_memory_layout, ax::T06_allocator, ax::T07_prefetch, ax::T08_concurrency,
                  ax::T09_serialization, ax::T10_telemetry, ax::T11_value_handle, ax::T12_isa,
                  ax::T13_index_organization, ax::T14_io_dispatch, ax::T15_migration_policy, ax::T16_filter,
                  ax::T17_page_type, ax::T18_simd_extension, ax::T19_general_hardware, ax::T20_queuing_q1,
                  ax::T21_queuing_q2, ax::T22_btree_order, ax::T23_skip_list_shape, ax::T24_bst_shape,
                  ax::T25_hash_probe_shape>;

// is_non_polymorphic_marker<T> == true gdw. der Strategie-Marker keine vtable traegt (compile-time-Dispatch).
template <class Marker>
struct is_non_polymorphic_marker : std::bool_constant<!std::is_polymorphic_v<Marker>> {};

static_assert(mp::mp_size<AllAxisStrategies>::value > 0, "Achsen-Strategie-Aggregat darf nicht leer sein.");
static_assert(mp::mp_all_of<AllAxisStrategies, is_non_polymorphic_marker>::value,
              "Metaprog-Striktheit (Organ-Ebene): ALLE Strategie-Marker der 26 Achsen muessen "
              "nicht-polymorph sein (CRTP+Concept, keine vtable im Hot-Path). Ein virtual/vtable-Marker "
              "waere eine compile-time->runtime-Degradation und bricht diesen Guard.");

TEST(MetaprogStriktheitAxesGuard, All26AxisStrategyMarkersAreNonPolymorphic) {
    std::size_t count               = 0;
    bool        all_non_polymorphic = true;
    mp::mp_for_each<AllAxisStrategies>([&]<class Marker>(Marker) {
        ++count;
        if (std::is_polymorphic_v<Marker>) { all_non_polymorphic = false; }
    });
    EXPECT_GT(count, 0u);             // das Aggregat der 26 Achsen ist nicht leer
    EXPECT_TRUE(all_non_polymorphic); // kein Achsen-Organ-Marker traegt eine vtable
}
