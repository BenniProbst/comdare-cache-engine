// 234-V-b (2026-07-08) -- Rest-Familien Shaped-Emission + binary_id-Shape-Segment + Baum-Emitter-Naht.
//
// Beweist additiv zu 234-V-a:
//   (1) Trait-Vervollstaendigung fuer BST/Hash/SkipList inkl. void-Neutralitaet.
//   (2) binary_id-Shape-Segment bleibt default-OFF fuer Shape=void.
//   (3) build_pilot_source_map_shaped haengt am echten for_each_permutation-Baum-Pfad.
//   (4) Eine repraesentative Rest-Familie (BST) materialisiert via SHAPED-Makro funktionsfaehig.

#include "builder/measurement_snapshot.hpp"

#include <anatomy/observable_tier.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_bst.hpp>
#include <organ_axes/lookup/composable/organ_for_search_algo_shaped.hpp>
#include <organ_axes/lookup/composable/tier_to_organ_mapping.hpp>

#include <builder/codegen/all_axes_umbrella.hpp>

#include <builder/experiment_tree/axis_path_serialization.hpp>
#include <builder/experiment_tree/pilot_source_map_shaped.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <compositions/art_reference.hpp>
#include <permutations/permutation_engine.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_registry.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_registry.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_registry.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace an        = ::comdare::cache_engine::anatomy;
namespace b         = ::comdare::cache_engine::builder;
namespace comp      = ::comdare::cache_engine::compositions;
namespace ce_exp    = ::comdare::cache_engine::builder::experiment;
namespace lk        = ::comdare::cache_engine::lookup;
namespace lkc       = ::comdare::cache_engine::lookup::composable;
namespace perm      = ::comdare::cache_engine::permutations;
namespace bst_shape = ::comdare::cache_engine::nodes::axis_bst_shape;
namespace hsh_shape = ::comdare::cache_engine::nodes::axis_hash_probe_shape;
namespace skl_shape = ::comdare::cache_engine::nodes::axis_skip_list_shape;

namespace {

using U64 = std::uint64_t;

constexpr std::string_view kBstShapeAxis    = "bst_shape";
constexpr std::string_view kBstShapeInclude = "topics/nodes/axis_bst_shape/axis_bst_shape_registry.hpp";

// Trait-Beweise: void-Neutralitaet bleibt typ-identisch zur einarmigen Naht.
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BinarySearchTreeSearchAlgo, void>,
                             lkc::organ_for_search_algo_t<lk::BinarySearchTreeSearchAlgo>>);
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::HashSearchAlgo, void>,
                             lkc::organ_for_search_algo_t<lk::HashSearchAlgo>>);
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::SkipListSearchAlgo, void>,
                             lkc::organ_for_search_algo_t<lk::SkipListSearchAlgo>>);

// Trait-Beweise: echte Shape-Traeger selektieren die Shaped-Organe der jeweiligen Rest-Familie.
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BinarySearchTreeSearchAlgo, bst_shape::BstPtrU16>,
                             lkc::BstTreeOrganShaped<bst_shape::BstPtrU16>>);
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::HashSearchAlgo, hsh_shape::HashOaLf50>,
                             lkc::HashSearchOrganShaped<hsh_shape::HashOaLf50>>);
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::SkipListSearchAlgo, skl_shape::SkipListMax8P50>,
                             lkc::SkipListOrganShaped<skl_shape::SkipListMax8P50>>);

// Fremde Familie: Shape wirkt nicht auf flache Wrapper.
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::Array256SearchAlgo, bst_shape::BstPtrU16>, void>);

// =============================================================================================
// A8-S5 PHASE B (2026-08-05) -- DER DRITTE TRAIT-PARAMETER: LEVEL 1 UND SEINE GEGENPROBE.
// =============================================================================================
// Der Shaped-Trait traegt seit Phase B einen dritten, mit dem Achsen-Default defaultierten
// Parameter: die Allokator-Strategie der KOMPOSITION. Die Level-0-Neutralitaet (Default bewegt
// keinen Typ) steht compile-hart im Header selbst -- sie ist eine Eigenschaft jener Datei. Was der
// Header NICHT beweisen kann, ist Level 1: dass der Parameter bei einer FREMDEN Strategie auch
// wirklich ANKOMMT. Dazu braucht es eine zweite, echte Strategie, und der einzige Allokator-Header,
// der ueber die Pool-Stores bis in den Shaped-Header reist, ist der Achsen-Default. Diese TU fuehrt
// die ausgelieferte Referenz-Komposition ohnehin (art_reference.hpp) -- ihr T6-Wert ist die fremde
// Strategie, ohne dass eine einzige Include-Zeile waechst.
//
// ABGELEITET, NICHT HINGESCHRIEBEN: kein "MimallocAllocator"-Literal. Wechselt die Referenz-
// Komposition ihre T6-Wahl, prueft diese Wache die NEUE Wahl.
using PhaseBFremdeStrategie = comp::ArtComposition::allocator;
static_assert(!std::is_same_v<PhaseBFremdeStrategie, ::comdare::cache_engine::alloc::ExgenAllocator>,
              "PHASE B GEGENPROBE VAKUOOS: die Referenz-Komposition fuehrt den Achsen-Default -- dann probt "
              "die Level-1-Aussage unten nichts. Andere Komposition waehlen.");

// (a) Der Typ MUSS sich bewegen. Ohne diese Zeile koennte der Level-0-Pin auch dann gruen sein,
//     wenn der dritte Parameter schlicht ignoriert wird -- er pinnte dann nichts.
static_assert(!std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BTreeSearchAlgo, void, PhaseBFremdeStrategie>,
                              lkc::organ_for_search_algo_t<lk::BTreeSearchAlgo>>,
              "PHASE B GATE TOT: der dritte Trait-Parameter aendert den Organ-Typ auch bei FREMDER Strategie "
              "nicht -- dann kommt die T6-Wahl der Komposition im konstitutiven Pool-Pfad gar nicht an.");

// (b) Und er MUSS im ORGAN ankommen, nicht bloss im Typ-Namen: das Composed*-Organ reicht die
//     allocator_type seines Knoten-Pools nach aussen durch (composed_btree_search.hpp:30,
//     composed_art_trie_search.hpp:39). Genau dort liegt der Speicher, um den es geht. Je
//     Familien-Art einmal: Shape-Familie ueber den void-Arm (BTree), Shape-Familie ueber den
//     SHAPE-Arm (BST) und shape-lose Trie-Familie (ART).
static_assert(
    std::is_same_v<
        typename lkc::organ_for_search_algo_shaped_t<lk::BTreeSearchAlgo, void, PhaseBFremdeStrategie>::allocator_type,
        PhaseBFremdeStrategie>,
    "PHASE B DURCHBINDUNG VERFEHLT (BTree, void-Arm): der Knoten-Pool traegt nicht die Strategie der Komposition.");
static_assert(
    std::is_same_v<typename lkc::organ_for_search_algo_shaped_t<lk::BinarySearchTreeSearchAlgo, bst_shape::BstPtrU16,
                                                                PhaseBFremdeStrategie>::allocator_type,
                   PhaseBFremdeStrategie>,
    "PHASE B DURCHBINDUNG VERFEHLT (BST, Shape-Arm): der Shape-Arm bindet die Strategie nicht -- dann driftete "
    "er gegen den void-Arm auseinander.");
static_assert(std::is_same_v<typename lkc::organ_for_search_algo_shaped_t<lk::OriginalArtSearchAlgo, void,
                                                                          PhaseBFremdeStrategie>::allocator_type,
                             PhaseBFremdeStrategie>,
              "PHASE B DURCHBINDUNG VERFEHLT (ART, shape-lose Familie): der Trie-Knoten-Pool traegt nicht die "
              "Strategie der Komposition -- der Faden reisst genau da, wo der meiste Speicher liegt.");
// Und die Gegenprobe dazu, damit die drei Zeilen nicht bloss "trivialerweise gleich" sind: am
// ACHSEN-DEFAULT traegt dasselbe Organ die DEFAULT-Strategie (der Wert wandert also wirklich mit).
static_assert(
    std::is_same_v<typename lkc::organ_for_search_algo_shaped_t<lk::OriginalArtSearchAlgo, void>::allocator_type,
                   ::comdare::cache_engine::alloc::ExgenAllocator>,
    "PHASE B: das ART-Organ traegt am Achsen-Default nicht mehr den Achsen-Default -- Level-0 waere gebrochen.");

// (c) Und die Shape-Wirkung bleibt, was sie war: der dritte Parameter hat den Shape-Arm nicht
//     verdraengt (sonst waere die 234-V-Naht still auf den void-Arm zusammengefallen).
static_assert(
    !std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BinarySearchTreeSearchAlgo, bst_shape::BstPtrU16,
                                                        PhaseBFremdeStrategie>,
                    lkc::organ_for_search_algo_shaped_t<lk::BinarySearchTreeSearchAlgo, void, PhaseBFremdeStrategie>>,
    "PHASE B: Shape wirkt unter fremder Strategie nicht mehr -- die 234-V-Naht waere auf den void-Arm "
    "zusammengefallen.");

template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct PoolFlipComposition {
    using search_algo        = SearchAlgoWrapper;
    using cache_traversal    = comp::ArtComposition::cache_traversal;
    using mapping            = comp::ArtComposition::mapping;
    using path_compression   = comp::ArtComposition::path_compression;
    using node_type          = comp::ArtComposition::node_type;
    using memory_layout      = comp::ArtComposition::memory_layout;
    using allocator          = comp::ArtComposition::allocator;
    using prefetch           = comp::ArtComposition::prefetch;
    using concurrency        = comp::ArtComposition::concurrency;
    using serialization      = comp::ArtComposition::serialization;
    using value_handle       = comp::ArtComposition::value_handle;
    using index_organization = comp::ArtComposition::index_organization;
    using io_dispatch        = comp::ArtComposition::io_dispatch;
    using migration_policy   = comp::ArtComposition::migration_policy;
    using filter             = comp::ArtComposition::filter;
    using queuing_q1         = comp::ArtComposition::queuing_q1;
    using queuing_q2         = comp::ArtComposition::queuing_q2;
    // STRUKT-R ORG-18: 18. Organ-Slot (Pflicht, kein Default). MemoryOnlyTarget = Durchreich-Wert:
    // kein Rueckschreib-Pfad. VOLL qualifiziert, weil der Member-Alias den Namespace sonst verdeckt.
    using persistence_target                   = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
    static constexpr std::string_view paper_id = "234-V-b rest-family shaped emission proof";
    static constexpr std::string_view name     = "V234bRestFamilyShapedComposition";
};

using BstC = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;

template <class SearchAlgoWrapper>
using PoolFlipPerm =
    perm::PermTuple<SearchAlgoWrapper, BstC::cache_traversal, BstC::mapping, BstC::path_compression, BstC::node_type,
                    BstC::memory_layout, BstC::allocator, BstC::prefetch, BstC::concurrency, BstC::serialization,
                    BstC::value_handle, BstC::index_organization, BstC::io_dispatch, BstC::migration_policy,
                    BstC::filter, BstC::queuing_q1, BstC::queuing_q2,
                    ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

using BstPerm   = PoolFlipPerm<lk::BinarySearchTreeSearchAlgo>;
using ArrayPerm = PoolFlipPerm<lk::Array256SearchAlgo>;

struct MiniEngineBst {
    [[nodiscard]] static constexpr std::size_t count() noexcept { return 2u; }

    template <class Visitor>
    static constexpr void for_each_permutation(Visitor&& v) {
        v.template operator()<BstPerm>();   // organ_for != void
        v.template operator()<ArrayPerm>(); // organ_for == void
    }
};

[[nodiscard]] U64 value_for(U64 key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

} // namespace

// Dateiweite Makro-Materialisierung: EINE repraesentative Rest-Familie (BST) als SHAPED-TU.
COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(
    bst_shape::BstPtrU16, BstC::search_algo, BstC::cache_traversal, BstC::mapping, BstC::path_compression,
    BstC::node_type, BstC::memory_layout, BstC::allocator, BstC::prefetch, BstC::concurrency, BstC::serialization,
    BstC::value_handle, BstC::index_organization, BstC::io_dispatch, BstC::migration_policy, BstC::filter,
    BstC::queuing_q1, BstC::queuing_q2,
    /* STRUKT-R ORG-18: T17 persistence_target */ ::comdare::cache_engine::persistence_target::MemoryOnlyTarget)

TEST(V234bRestFamiliesShaped, BinaryIdShapeSegmentIsDefaultOff) {
    EXPECT_EQ(ce_exp::with_shape_segment<void>("a=b/c=d", kBstShapeAxis), "a=b/c=d");

    std::string const expected = std::string{"a=b/bst_shape="} + std::string{bst_shape::BstPtrU16::name()};
    EXPECT_EQ(ce_exp::with_shape_segment<bst_shape::BstPtrU16>("a=b", kBstShapeAxis), expected);
}

TEST(V234bRestFamiliesShaped, PilotSourceMapShapedFiltersAndSegmentsBstOnly) {
    auto const shaped =
        ce_exp::build_pilot_source_map_shaped<MiniEngineBst, bst_shape::BstPtrU16>(kBstShapeAxis, kBstShapeInclude);

    ASSERT_EQ(shaped.size(), 1u) << "Array256 (organ_for=void) muss gefiltert bleiben";
    auto const& [key, value] = *shaped.begin();

    std::string const suffix = std::string{"/bst_shape="} + std::string{bst_shape::BstPtrU16::name()};
    EXPECT_TRUE(key.ends_with(suffix)) << key;
    EXPECT_NE(value.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED("), std::string::npos);
    EXPECT_NE(value.find("#include <topics/nodes/axis_bst_shape/axis_bst_shape_registry.hpp>"), std::string::npos);
    EXPECT_NE(value.find("BstPtrU16"), std::string::npos);

    auto const unshaped = ce_exp::build_pilot_source_map_shaped<MiniEngineBst, void>(kBstShapeAxis, kBstShapeInclude);
    ASSERT_EQ(unshaped.size(), 1u);
    EXPECT_EQ(unshaped.begin()->first, ce_exp::serialize_composition_path<BstPerm>());
    EXPECT_EQ(unshaped.begin()->first.find("bst_shape="), std::string::npos);
}

TEST(V234bRestFamiliesShaped, BstShapedMacroMaterializesWorkingTier) {
    EXPECT_EQ(::comdare_anatomy_abi_version() >> 32, static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR));
    EXPECT_EQ(::comdare_anatomy_abi_magic(), static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAGIC));

    auto* base = ::comdare_create_anatomy();
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->organ_count(), 18u);
    auto* drv = dynamic_cast<an::IDriveableTier*>(base);
    ASSERT_NE(drv, nullptr);

    constexpr U64 keys[] = {11u, 4u, 19u, 2u, 7u, 13u, 23u};
    for (U64 const key : keys) { EXPECT_TRUE(drv->tier_insert(key, value_for(key))); }

    U64 out = 0u;
    EXPECT_TRUE(drv->tier_lookup(7u, &out));
    EXPECT_EQ(out, value_for(7u));
    EXPECT_FALSE(drv->tier_lookup(404u, &out));

    ::comdare_destroy_anatomy(base);
}

TEST(V234bRestFamiliesShaped, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 9); // NAHT-1 (d4c0b49c): 8 -> 9, Mess-Visitor am Genus-Interface
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1344u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 8u);
}
