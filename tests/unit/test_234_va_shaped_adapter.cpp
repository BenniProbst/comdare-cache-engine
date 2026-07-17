// 234-V-a (2026-07-07, User-GO Option A) -- Shaped-Emission Mechanik: Beweis-Familie btree_order.
//
// Beweist die vier V-a-Bausteine LITERAL (S7-2-Muster):
//   (1) Trait: organ_for_search_algo_shaped<S,Shape> -- void-Neutralitaet + Kt8-Selektion + fremde Familie.
//   (2) Adapter-Traeger: SearchAlgorithmAbiAdapter<Anatomy, BtreeOrderKt2> treibt den Shape BIS IN den
//       Pool -- gleiche Keys erzeugen unter Kt2 (max 3 Keys/Knoten) MEHR Knoten-Allokationen als der
//       Default (Kt4, max 7 Keys/Knoten): axis_stats[6][2] (T6 alloc_cnt) Kt2 > Default.
//   (3) Emitter-Render: SHAPED-Quelle traegt COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED + Shape-FQ als
//       ERSTES Argument; die Default-Quelle bleibt unveraendert OHNE Shape.
//   (4) Emitter-Filter: emit_adhoc_modules_shaped emittiert NUR organ-backed Kompositionen
//       (organ_for!=void) -- MiniEngine {BTree, Array256} ergibt EXAKT 1 Datei.
// Default-OFF: der Standard-Emissions-Pfad (emit_adhoc_modules, apps main.cpp) ist unveraendert;
// golden-320/ABI-MAJOR-4/POD bleiben unberuehrt (Neutralitaets-Guards unten).

#include "builder/measurement_snapshot.hpp"
#include "comdare_test_tmp.hpp"

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/lookup/axis_03a_search_algo_btree.hpp>
#include <axes/lookup/composable/organ_for_search_algo_shaped.hpp>
#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <builder/codegen/adhoc_emitter_shaped.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <compositions/art_reference.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_registry.hpp> // deklariert ALLE Kt-Shapes (Critical-1)

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace an   = ::comdare::cache_engine::anatomy;
namespace b    = ::comdare::cache_engine::builder;
namespace cg   = ::comdare::cache_engine::builder::codegen;
namespace comp = ::comdare::cache_engine::compositions;
namespace lk   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;
namespace ord  = ::comdare::cache_engine::nodes::axis_btree_order;

namespace {

using U64 = std::uint64_t;

// ── (1) Trait-Beweise (self-proving, compile-time) ──────────────────────────────────────────────
// Level-0 (void) == einarmige Naht (Adapter-Default byte-identisch).
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BTreeSearchAlgo, void>,
                             lkc::organ_for_search_algo_t<lk::BTreeSearchAlgo>>);
// Echter Shape-Traeger waehlt das Shaped-Organ der Beweis-Familie.
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BTreeSearchAlgo, ord::BtreeOrderKt8>,
                             lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt8>>);
// F1-Anker transitiv: Default-Organ == Kt4-Shaped -> Kt4-Traeger ist typ-identisch zum Default.
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::BTreeSearchAlgo, ord::BtreeOrderKt4>,
                             lkc::BTreeSearchOrgan>);
// Fremde Familie: Shape wirkt nicht (flacher Wrapper bleibt void -> keine Scheinmultiplikation).
static_assert(std::is_same_v<lkc::organ_for_search_algo_shaped_t<lk::Array256SearchAlgo, ord::BtreeOrderKt8>, void>);

// ── Komposition (S7-2-Muster: BTree-Wrapper in ArtComposition-Slots) ─────────────────────────────
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
    using value_handle                         = comp::ArtComposition::value_handle;
    using isa                                  = comp::ArtComposition::isa;
    using index_organization                   = comp::ArtComposition::index_organization;
    using io_dispatch                          = comp::ArtComposition::io_dispatch;
    using migration_policy                     = comp::ArtComposition::migration_policy;
    using filter                               = comp::ArtComposition::filter;
    using queuing_q1                           = comp::ArtComposition::queuing_q1;
    using queuing_q2                           = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "234-V-a shaped emission proof";
    static constexpr std::string_view name     = "V234aShapedComposition";
};

using BTreeC   = PoolFlipComposition<lk::BTreeSearchAlgo>;
using Anatomy  = an::SearchAlgorithmAnatomy<BTreeC>;
using AdapterD = an::SearchAlgorithmAbiAdapter<Anatomy>;                     // Default (Level-0)
using AdapterS = an::SearchAlgorithmAbiAdapter<Anatomy, ord::BtreeOrderKt2>; // Shaped-Traeger Kt2

// Default-Arg-Identitaet: Bestands-Instanzen SIND die void-Spezialisierung (kein zweiter Typ).
static_assert(std::is_same_v<AdapterD, an::SearchAlgorithmAbiAdapter<Anatomy, void>>);

[[nodiscard]] U64 value_for(U64 key) noexcept { return key ^ 0x9E3779B97F4A7C15ull; }

// Sequentielle Keys 1..n (Worst-Case-Fuellgrad im B-Baum -> maximale Knotenzahl-Differenz Kt2 vs Kt4).
[[nodiscard]] std::vector<U64> sequential_keys(U64 n) {
    std::vector<U64> keys;
    keys.reserve(static_cast<std::size_t>(n));
    for (U64 k = 1; k <= n; ++k) keys.push_back(k);
    return keys;
}

// Treibt N Keys ueber die ABI-Route und liefert den T6-alloc_cnt (axis_stats[6][2], S7-2-Route).
template <class Adapter>
[[nodiscard]] U64 t6_alloc_cnt_after_inserts(std::vector<U64> const& keys) {
    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());
    for (U64 const key : keys) {
        if (!drv->tier_insert(key, value_for(key))) return 0u; // ehrlicher Fail -> Test schlaegt an
    }
    // Roundtrip-Ehrlichkeit: der Shaped-Adapter muss funktional korrekt bleiben.
    for (U64 const key : keys) {
        U64 out = 0u;
        if (!drv->tier_lookup(key, &out) || out != value_for(key)) return 0u;
    }
    auto* obs = dynamic_cast<an::IObservableTier*>(drv);
    if (obs == nullptr) return 0u;
    an::ComdareTierObserverSnapshot snap{};
    obs->tier_observe(&snap);
    return snap.axis_stats[6][2];
}

// ── (4) MiniEngine fuer den Filter-Beweis: 1 organ-backed + 1 flache Komposition ────────────────
struct MiniEngine {
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        v.template operator()<PoolFlipComposition<lk::BTreeSearchAlgo>>();    // organ_for != void
        v.template operator()<PoolFlipComposition<lk::Array256SearchAlgo>>(); // organ_for == void
    }
};

[[nodiscard]] std::string read_all(std::filesystem::path const& p) {
    std::ifstream      in{p};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

// ── Makro-Materialisierungs-Beweis (REVIEW-FIX Critical-1): diese TU repliziert EXAKT den Inhalt der
// emittierten SHAPED-Quelle (Umbrella-Include + Registry-Include + SHAPED-Makro mit Nicht-Default-Shape)
// und beweist damit KOMPILIERBARKEIT + Funktion des Makro-Pfads in-process (extern-C-Symbole unten). ──
COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(ord::BtreeOrderKt2, BTreeC::search_algo, BTreeC::cache_traversal,
                                           BTreeC::mapping, BTreeC::path_compression, BTreeC::node_type,
                                           BTreeC::memory_layout, BTreeC::allocator, BTreeC::prefetch,
                                           BTreeC::concurrency, BTreeC::serialization, BTreeC::value_handle,
                                           BTreeC::isa, BTreeC::index_organization, BTreeC::io_dispatch,
                                           BTreeC::migration_policy, BTreeC::filter, BTreeC::queuing_q1,
                                           BTreeC::queuing_q2)

// (2a) Adapter-Traeger-Beweis STRUKTURELL (REVIEW-FIX Important-2, F1-Praezedenz): live_nodes ist die
// pool-native Groesse (zaehlt je new_node, unabhaengig von vector-Growth-Buckets) -- Kt2 (max 3 Keys/
// Knoten) MUSS fuer dieselben sequentiellen Keys strikt mehr lebende Knoten tragen als Kt4 (max 7).
TEST(V234aShapedAdapter, ShapedOrganHasStructurallyMoreLiveNodesThanDefault) {
    using DefaultHull = lkc::ObservableComposedContainer<lkc::BTreeSearchOrgan>;
    using Kt2Hull     = lkc::ObservableComposedContainer<lkc::BTreeSearchOrganShaped<ord::BtreeOrderKt2>>;

    auto const keys = sequential_keys(40u);

    DefaultHull dh;
    Kt2Hull     kh;
    for (U64 const key : keys) {
        EXPECT_TRUE(dh.insert(key, value_for(key)));
        EXPECT_TRUE(kh.insert(key, value_for(key)));
    }

    // Phase 0.3a: die pool-native LEBENDE Knotenzahl kommt jetzt aus pool_node_count() (nodes_.size()-free_.size(),
    // growth-bucket-UNABHAENGIG) statt aus dem entfallenen allocator-snapshot-live_nodes — genau die Groesse, die
    // der Kommentar oben verlangt (die Allocator-Stats zaehlen jetzt vector-Reallokationen, growth-abhaengig).
    std::size_t const ds = dh.pool_node_count();
    std::size_t const ks = kh.pool_node_count();
    std::cout << "234-V-a pool_node_count: default(Kt4)=" << ds << " shaped(Kt2)=" << ks << '\n';

    EXPECT_GT(ds, 0u);
    EXPECT_GT(ks, 0u);
    EXPECT_GT(ks, ds) << "Kt2 muss strukturell mehr Knoten tragen als Kt4";
}

// (2b) Adapter-Traeger-Beweis ueber die ABI-Route: der Shape erreicht den Pool DURCH den Adapter.
// 200 sequentielle Keys (F1-Praezedenz) machen die alloc_cnt-Differenz robust gegen jeden
// implementierungsabhaengigen vector-Growth-Faktor (1.5x wie 2x).
TEST(V234aShapedAdapter, ShapeCarrierReachesPoolThroughAbiAdapter) {
    auto const keys = sequential_keys(200u);

    U64 const default_allocs = t6_alloc_cnt_after_inserts<AdapterD>(keys);
    U64 const kt2_allocs     = t6_alloc_cnt_after_inserts<AdapterS>(keys);

    std::cout << "234-V-a T6 alloc_cnt: default(Kt4)=" << default_allocs << " shaped(Kt2)=" << kt2_allocs << '\n';

    EXPECT_GT(default_allocs, 0u);
    EXPECT_GT(kt2_allocs, 0u);
    EXPECT_GT(kt2_allocs, default_allocs) << "Kt2 muss mehr Knoten allozieren als Kt4 (Shape wirkt im Pool)";
}

// (2c) Makro-E2E (Critical-1-Beweis): die oben materialisierten extern-C-Symbole liefern ein
// funktionsfaehiges SHAPED-Tier -- exakt der Pfad, den eine emittierte DLL naehme.
TEST(V234aShapedAdapter, ShapedMacroMaterializesWorkingTier) {
    EXPECT_EQ(::comdare_anatomy_abi_version() >> 32, static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR));
    EXPECT_EQ(::comdare_anatomy_abi_magic(), static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAGIC));

    auto* base = ::comdare_create_anatomy();
    ASSERT_NE(base, nullptr);
    auto* drv = dynamic_cast<an::IDriveableTier*>(base);
    ASSERT_NE(drv, nullptr);

    for (U64 const key : sequential_keys(32u)) { EXPECT_TRUE(drv->tier_insert(key, value_for(key))); }
    U64 out = 0u;
    EXPECT_TRUE(drv->tier_lookup(7u, &out));
    EXPECT_EQ(out, value_for(7u));
    EXPECT_FALSE(drv->tier_lookup(404u, &out));

    ::comdare_destroy_anatomy(base);
}

// (3) Render-Beweis: SHAPED-Quelle traegt das SHAPED-Makro + den Shape-FQ ZUERST; Default unveraendert.
TEST(V234aShapedAdapter, ShapedRenderCarriesShapeFirstAndDefaultStaysUnshaped) {
    std::string const          args          = cg::adhoc_macro_args<BTreeC>();
    constexpr std::string_view kShapeInclude = "topics/nodes/axis_btree_order/axis_btree_order_registry.hpp";

    std::string const shaped =
        cg::render_adhoc_module_source_shaped(0, cg::type_name<ord::BtreeOrderKt2>(), kShapeInclude, args);
    EXPECT_NE(shaped.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(\n"), std::string::npos);
    // Critical-1: der Shape-Include MUSS emittiert sein, sonst ist die TU fuer Nicht-Default-Shapes
    // nicht kompilierbar (die Umbrella-Closure erreicht nur den Kt4-Default).
    EXPECT_NE(shaped.find("#include <topics/nodes/axis_btree_order/axis_btree_order_registry.hpp>"), std::string::npos);
    EXPECT_NE(shaped.find("BtreeOrderKt2"), std::string::npos);
    // Shape steht VOR dem ersten Achsen-Argument (BUILDVARIANT-Praezedenz: benannte Args zuerst).
    // Eindeutige Tokens statt Namespace-Praefix (der Shape-FQ beginnt selbst mit comdare::cache_engine::...).
    EXPECT_LT(shaped.find("BtreeOrderKt2"), shaped.find("BTreeSearchAlgo"));

    std::string const plain = cg::render_adhoc_module_source(0, args);
    EXPECT_NE(plain.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(\n"), std::string::npos);
    EXPECT_EQ(plain.find("ADHOC_SHAPED"), std::string::npos);
    EXPECT_EQ(plain.find("BtreeOrderKt2"), std::string::npos);
    EXPECT_EQ(plain.find("axis_btree_order_registry"), std::string::npos);
}

// (4) Filter-Beweis: NUR die organ-backed Komposition wird emittiert (keine Scheinmultiplikation).
TEST(V234aShapedAdapter, ShapedEmitterFiltersToOrganBackedFamiliesOnly) {
    auto const out_dir = comdare::test::user_tmp_dir() /
                         ("v234a_shaped_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
    std::filesystem::remove_all(out_dir);

    auto const files = cg::emit_adhoc_modules_shaped<MiniEngine, ord::BtreeOrderKt2>(
        out_dir, "topics/nodes/axis_btree_order/axis_btree_order_registry.hpp");

    ASSERT_EQ(files.size(), 1u) << "genau die BTree-Komposition; Array256 (organ_for=void) gefiltert";
    EXPECT_NE(files[0].filename().string().find("shaped_0"), std::string::npos);

    std::string const src = read_all(files[0]);
    EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(\n"), std::string::npos);
    EXPECT_NE(src.find("#include <topics/nodes/axis_btree_order/axis_btree_order_registry.hpp>"), std::string::npos);
    EXPECT_NE(src.find("BtreeOrderKt2"), std::string::npos);
    EXPECT_NE(src.find("BTreeSearchAlgo"), std::string::npos);

    std::filesystem::remove_all(out_dir);
}

// Neutralitaets-Guards (S7-Muster): ABI-MAJOR 4, POD-Snapshot, CSV-Schemata unveraendert.
TEST(V234aShapedAdapter, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1344u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 6u);
}
