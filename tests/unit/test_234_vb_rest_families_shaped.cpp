// 234-V-b (2026-07-08) -- Rest-Familien Shaped-Emission + binary_id-Shape-Segment + Baum-Emitter-Naht.
//
// Beweist additiv zu 234-V-a:
//   (1) Trait-Vervollstaendigung fuer BST/Hash/SkipList inkl. void-Neutralitaet.
//   (2) binary_id-Shape-Segment bleibt default-OFF fuer Shape=void.
//   (3) build_pilot_source_map_shaped haengt am echten for_each_permutation-Baum-Pfad.
//   (4) Eine repraesentative Rest-Familie (BST) materialisiert via SHAPED-Makro funktionsfaehig.

#include "builder/measurement_snapshot.hpp"

#include <anatomy/observable_tier.hpp>
#include <axes/lookup/axis_03a_search_algo_bst.hpp>
#include <axes/lookup/composable/organ_for_search_algo_shaped.hpp>
#include <axes/lookup/composable/tier_to_organ_mapping.hpp>

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

template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct PoolFlipComposition {
    using search_algo                          = SearchAlgoWrapper;
    using cache_traversal                      = comp::ArtComposition::cache_traversal;
    using mapping                              = comp::ArtComposition::mapping;
    using path_compression                     = comp::ArtComposition::path_compression;
    using node_type                            = comp::ArtComposition::node_type;
    using memory_layout                        = comp::ArtComposition::memory_layout;
    using allocator                            = comp::ArtComposition::allocator;
    using prefetch                             = comp::ArtComposition::prefetch;
    using concurrency                          = comp::ArtComposition::concurrency;
    using serialization                        = comp::ArtComposition::serialization;
    using telemetry                            = comp::ArtComposition::telemetry;
    using value_handle                         = comp::ArtComposition::value_handle;
    using isa                                  = comp::ArtComposition::isa;
    using index_organization                   = comp::ArtComposition::index_organization;
    using io_dispatch                          = comp::ArtComposition::io_dispatch;
    using migration_policy                     = comp::ArtComposition::migration_policy;
    using filter                               = comp::ArtComposition::filter;
    using queuing_q1                           = comp::ArtComposition::queuing_q1;
    using queuing_q2                           = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "234-V-b rest-family shaped emission proof";
    static constexpr std::string_view name     = "V234bRestFamilyShapedComposition";
};

using BstC = PoolFlipComposition<lk::BinarySearchTreeSearchAlgo>;

template <class SearchAlgoWrapper>
using PoolFlipPerm =
    perm::PermTuple<SearchAlgoWrapper, BstC::cache_traversal, BstC::mapping, BstC::path_compression, BstC::node_type,
                    BstC::memory_layout, BstC::allocator, BstC::prefetch, BstC::concurrency, BstC::serialization,
                    BstC::telemetry, BstC::value_handle, BstC::isa, BstC::index_organization, BstC::io_dispatch,
                    BstC::migration_policy, BstC::filter, BstC::queuing_q1, BstC::queuing_q2>;

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
COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(bst_shape::BstPtrU16, BstC::search_algo, BstC::cache_traversal,
                                           BstC::mapping, BstC::path_compression, BstC::node_type, BstC::memory_layout,
                                           BstC::allocator, BstC::prefetch, BstC::concurrency, BstC::serialization,
                                           BstC::telemetry, BstC::value_handle, BstC::isa, BstC::index_organization,
                                           BstC::io_dispatch, BstC::migration_policy, BstC::filter, BstC::queuing_q1,
                                           BstC::queuing_q2)

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
    EXPECT_EQ(base->organ_count(), 19u);
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

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 5u);
}
