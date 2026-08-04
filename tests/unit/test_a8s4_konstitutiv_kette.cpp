// A8-S4 (2026-08-04) -- KONSTITUTIV-KETTEN-WACHE zur Konstitutiv-Matrix (Gattungs-Vertrag).
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.3
// Matrix:  docs/architecture/20260804-a8_s4_konstitutiv_matrix_gattungs_vertrag.md
//
// GEGENSTAND: die Matrix erklaert je (Gattungs-Funktion x Achse) eine Rolle. Fuer den Gattungs-Kern
// (tier_insert/tier_lookup/tier_erase/tier_clear/tier_size) ist die KONSTITUTIVE Menge
// {T0 search_algo, T4 node_type, T5 memory_layout, T6 allocator} = die Store-Kette; die uebrigen 14
// Achsen sind BEOBACHTEND (nur unter COMDARE_MEASUREMENT_ON). Diese TU pinnt genau das AM OBJEKT:
//
//   A) COMPILE-TIME (static_assert):
//      A1 Achsen-Namens-Bindung: die vier konstitutiven Slots tragen exakt search_algo/node_type/
//         memory_layout/allocator -- die Matrix haengt an NAMEN, nie an rohen Indizes.
//      A2 Matrix-Deckung: konstitutiv(4) + beobachtend(14) == kCompositionAxisNames (18), jeder Name
//         GENAU EINMAL. Eine neue/umbenannte Achse bricht die Wache laut statt still.
//      A3 Achsen-Concepts von T4/T5/T6 (die Store-Kette laesst nur vertragstreue Achsen zu).
//      A4 StorageOrgan/TraversalOrgan -- der T0-Vertrag ueber dem Store.
//      A5 A8S4GattungsKernAntrieb + a8s4_store_kette_kompositions_gebunden_v (die beiden im
//         abi_adapter.hpp gesetzten Kettenwachen, hier an derselben Kette gegengeprueft).
//      A6 T4 KONSTITUTIV-CT: node_type bestimmt die Chunk-Kapazitaet (Node4 != Node256).
//      A7 T5 KONSTITUTIV-CT: memory_layout bestimmt die physische Record-Breite (CLA 64 != AoS 16).
//      A8 T6 CT-Bindung: der Store fuehrt den Allokator-Namen der Komposition.
//
//   B) LAUFZEIT (durch den ECHTEN Adapter ueber IDriveableTier + an der identisch gebauten Kette):
//      B1 Gattungs-Kern funktional (insert/lookup/erase/clear/size).
//      B2 T6 KONSTITUTIV-RT literal: nach den Inserts ist chunk_alloc_count() > 0 -- der Allokator
//         wurde im Hot-Path REAL gerufen (LayoutAwareChunkedStore append_slot -> A::allocate).
//      B3 clear() gibt die Chunks REAL frei (chunk_alloc_count() == 0 -> free_chunks_ -> A::deallocate).
//      B4 Wallclock-REFERENZ des Treib-Laufs unter MESSUNG-AN -- der Vergleichs-Nullpunkt des
//         Release-Kontrasts in test_a8s4_release_pfad_neutralitaet (identische Last, identische Keys).
//
// Diese TU laeuft im MESS-Build (COMDARE_MEASUREMENT_ON/STATISTICS/EXPERIMENT_MODE) und ist damit die
// POSITIV-Seite des Paares; die Release-Seite liegt in test_a8s4_release_pfad_neutralitaet.cpp.

#include <builder/codegen/all_axes_umbrella.hpp>
#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (NUR gelesen, golden-Tabu)

#include <axes/lookup/composable/observable_composed_search.hpp>
#include <axes/lookup/composable/traversal_for_search_algo.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <axes/node/concepts/axis_04_node_type_concept.hpp>
#include <axes/layout/concepts/axis_05_memory_layout_concept.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::compositions {
// A8S4KonstitutivComposition -- store-traversierbarer Prueffall (LinearScanSearchAlgo, uint16-Keys).
// Bewusst eine EIGENE Komposition dieser TU (keine Kopplung an das Innere einer fremden Test-TU);
// gleiche Achsen-Wahl wie die Bestands-Basis test_m8_storetrav_segment.cpp, damit der Fall vergleichbar
// bleibt. Der store-traversierbare Fall ist der einzige, in dem T4/T5/T6 die FLACHE Store-Kette tragen
// (organ-gehuellte Familien tragen sie in ihrem eigenen Organ -- eigene Matrix-Zeile).
struct A8S4KonstitutivComposition {
    using search_algo      = traversal::axis_03a_search_algo::LinearScanSearchAlgo;
    using cache_traversal  = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping          = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type        = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;
    using memory_layout    = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
        memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;
    using allocator     = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch      = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency   = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization = serialization::axis_10_serialization::ObservableSerialization<
        serialization::axis_10_serialization::RawBinarySerialization>;
    using telemetry = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;
    using value_handle                         = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                                  = hardware::axis_09_isa::Amd64Isa;
    using index_organization                   = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch                          = io::axis_io::InMemoryOnly;
    using migration_policy                     = migration::axis_migration::NoMigration;
    using filter                               = filter::axis_filter::BloomFilter;
    using queuing_q1                           = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2                           = queuing::axis_q2_queuing::LazyFlush;
    using persistence_target                   = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
    static constexpr std::string_view paper_id = "A8-S4 (Konstitutiv-Ketten-Wache)";
    static constexpr std::string_view paper_title =
        "A8-S4 Konstitutiv-Matrix -- store-traversierbare Prueffall-Komposition";
    static constexpr std::string_view name = "A8S4KonstitutivComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::A8S4KonstitutivComposition",
                                        "tests/unit/test_a8s4_konstitutiv_kette.cpp");
};
} // namespace comdare::cache_engine::compositions

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace lc   = ::comdare::cache_engine::lookup::composable;
namespace nd   = ::comdare::cache_engine::node;

using Comp = comp::A8S4KonstitutivComposition;

// ---------------------------------------------------------------------------
// A1/A2 -- die Matrix haengt an ACHSEN-NAMEN, nie an rohen Indizes.
// ---------------------------------------------------------------------------
inline constexpr std::array<std::string_view, 4>  kA8S4Konstitutiv = {"search_algo", "node_type", "memory_layout",
                                                                      "allocator"};
inline constexpr std::array<std::string_view, 14> kA8S4Beobachtend = {
    "cache_traversal", "mapping",      "path_compression",   "prefetch",          "concurrency",
    "serialization",   "value_handle", "index_organization", "io_dispatch",       "migration_policy",
    "filter",          "queuing_q1",   "queuing_q2",         "persistence_target"};

// Zaehlt, wie oft `n` in den beiden Rollen-Listen vorkommt (Soll: GENAU einmal).
[[nodiscard]] constexpr std::size_t rollen_treffer(std::string_view n) noexcept {
    std::size_t c = 0;
    for (auto const& k : kA8S4Konstitutiv)
        if (k == n) ++c;
    for (auto const& b : kA8S4Beobachtend)
        if (b == n) ++c;
    return c;
}
[[nodiscard]] constexpr bool matrix_deckt_alle_achsen() noexcept {
    for (auto const& a : ex::kCompositionAxisNames)
        if (rollen_treffer(a) != 1) return false;
    return true;
}

// A1: die vier konstitutiven Slots liegen an den Achsen-Positionen, die die Matrix als T0/T4/T5/T6 fuehrt.
static_assert(ex::kCompositionAxisNames[0] == "search_algo", "A8-S4 A1: T0 ist nicht search_algo");
static_assert(ex::kCompositionAxisNames[4] == "node_type", "A8-S4 A1: T4 ist nicht node_type");
static_assert(ex::kCompositionAxisNames[5] == "memory_layout", "A8-S4 A1: T5 ist nicht memory_layout");
static_assert(ex::kCompositionAxisNames[6] == "allocator", "A8-S4 A1: T6 ist nicht allocator");
// A2: Deckung -- die Matrix laesst KEINE Achse unklassifiziert und nennt keine doppelt.
static_assert(kA8S4Konstitutiv.size() + kA8S4Beobachtend.size() == ex::kCompositionAxisNames.size(),
              "A8-S4 A2: Rollen-Listen decken nicht die volle Achsen-Zahl");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "A8-S4 A2: CSV-Achsen-Namen und Wire-Achsen-Zahl sind auseinandergelaufen");
static_assert(matrix_deckt_alle_achsen(), "A8-S4 A2: mindestens eine Achse ist unklassifiziert oder doppelt gefuehrt");

// ---------------------------------------------------------------------------
// A3-A8 -- die konstitutive Store-Kette, EXAKT wie der Adapter sie baut
// (anatomy/abi_adapter.hpp: flat_container_algorithm_t = ObservableComposedSearch<Traversal,
//  LayoutAwareChunkedStore<Composition::node_type, Composition::memory_layout, Composition::allocator>>).
// ---------------------------------------------------------------------------
using Trav  = lc::traversal_for_search_algo_t<Comp::search_algo>;
using Store = nd::LayoutAwareChunkedStore<Comp::node_type, Comp::memory_layout, Comp::allocator>;
using Chain = lc::ObservableComposedSearch<Trav, Store>;

// A3: die drei Store-Achsen erfuellen ihre Achsen-Concepts (der Store-requires-Block laesst nichts anderes zu).
static_assert(::comdare::cache_engine::node::concepts::NodeTypeStrategy<Comp::node_type>,
              "A8-S4 A3: T4 erfuellt NodeTypeStrategy nicht");
static_assert(::comdare::cache_engine::layout::concepts::MemoryLayoutStrategy<Comp::memory_layout>,
              "A8-S4 A3: T5 erfuellt MemoryLayoutStrategy nicht");
static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Comp::allocator>,
              "A8-S4 A3: T6 erfuellt AllocatorStrategy nicht");
// A4: der T0-Vertrag ueber dem Store.
static_assert(lc::StorageOrgan<Store>, "A8-S4 A4: die Store-Kette erfuellt StorageOrgan nicht");
static_assert(lc::TraversalOrgan<Trav, Store>, "A8-S4 A4: das T0-Traversal-Organ passt nicht auf den Store");
// A5: dieselben beiden Wachen, die im abi_adapter.hpp an container_algorithm_t haengen.
static_assert(an::A8S4GattungsKernAntrieb<Chain>, "A8-S4 A5: der Gattungs-Kern-Antrieb fehlt an der Kette");
static_assert(an::a8s4_store_kette_kompositions_gebunden_v<Chain, Comp>,
              "A8-S4 A5: die Store-Kette traegt nicht die Kompositions-Achsen");
// A6: T4 KONSTITUTIV-CT -- node_type bestimmt die Chunk-Kapazitaet des Hot-Path-Substrats.
using StoreNode4 =
    nd::LayoutAwareChunkedStore<::comdare::cache_engine::node::Node4NodeType, Comp::memory_layout, Comp::allocator>;
static_assert(Store::node_capacity() != StoreNode4::node_capacity(),
              "A8-S4 A6: node_type hat KEINE Compile-Time-Wirkung auf die Store-Geometrie");
// A7: T5 KONSTITUTIV-CT -- memory_layout bestimmt die physische Record-Breite (Repraesentations-Dispatch).
using StoreAoS =
    nd::LayoutAwareChunkedStore<Comp::node_type,
                                ::comdare::cache_engine::memory_layout::axis_05_memory_layout::ObservableMemoryLayout<
                                    ::comdare::cache_engine::layout::AoSStrictMemoryLayout>,
                                Comp::allocator>;
static_assert(Store::record_phys_bytes() != StoreAoS::record_phys_bytes(),
              "A8-S4 A7: memory_layout hat KEINE Compile-Time-Wirkung auf das physische Record-Layout");
static_assert(Store::record_phys_bytes() == 64, "A8-S4 A7: cache_line_aligned soll 64-B-Stride tragen");
static_assert(StoreAoS::record_phys_bytes() == 16, "A8-S4 A7: aos_strict soll 16-B-Stride tragen");
// A8: T6 CT-Bindung -- der Store fuehrt den Allokator-Namen der Komposition (keine stille Entkopplung).
static_assert(Store::allocator_name() == Comp::allocator::name(), "A8-S4 A8: der Store fuehrt einen FREMDEN Allokator");

static int  g_fail = 0;
static void chk(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Die REFERENZ-Last: identisch zu der in test_a8s4_release_pfad_neutralitaet.cpp (gleiche Zahl, gleiche
// Key-Folge) -- nur so ist der Wallclock-Kontrast MESSUNG-AN vs. Release aussagefaehig.
static constexpr std::uint64_t kN = 4096; // Keys < 65536 (LinearScanSearchAlgo key_type=uint16)

int main() {
    std::cout << "==== A8-S4: Konstitutiv-Ketten-Wache (MESSUNG-AN-Seite) ====\n";

    // ---- B1: der Gattungs-Kern durch den ECHTEN Adapter -------------------------------------
    using Anatomy = an::SearchAlgorithmAnatomy<Comp>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;
    chk("T0 routet durch den flachen Store (tier_search_routes_through_store)",
        Adapter::tier_search_routes_through_store());

    Adapter tier;
    auto*   drv = dynamic_cast<an::IDriveableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    chk("IDriveableTier vorhanden (Gattungs-Antrieb)", drv != nullptr);
    if (drv == nullptr) {
        std::cout << "==== A8-S4: ABBRUCH ====\n";
        return 1;
    }

    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);
    std::uint64_t treffer = 0;
    for (std::uint64_t i = 0; i < kN; ++i) {
        std::uint64_t v = 0;
        if (drv->tier_lookup(i, &v) && v == i * 7u + 1u) ++treffer;
    }
    auto const t1 = std::chrono::steady_clock::now();
    auto const drive_ns =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

    chk("B1 tier_size == kN nach den Inserts", drv->tier_size() == kN);
    chk("B1 alle kN Lookups treffen mit korrektem Wert", treffer == kN);
    chk("B1 tier_erase entfernt einen realen Eintrag", drv->tier_erase(7u) && drv->tier_size() == kN - 1u);
    drv->tier_clear();
    chk("B1 tier_clear leert den konstitutiven Speicher", drv->tier_size() == 0u);

    // ---- B2/B3: T6 KONSTITUTIV-RT an der identisch gebauten Kette ---------------------------
    // Der Adapter haelt container_algorithm_ privat; die Kette wird hier nach DERSELBEN Regel gebaut
    // (s. Kopf A3-A8) und literal befragt: chunk_alloc_count zaehlt die REALEN A::allocate-Aufrufe des
    // Chunk-Wachstums (axis_04_node_type_layout_aware_store.hpp append_slot -> alloc_.allocate).
    Chain kette;
    for (std::uint64_t i = 0; i < kN; ++i) (void)kette.insert(i, i * 7u + 1u);
    auto const allocs_nach_insert = kette.store().chunk_alloc_count();
    chk("B2 T6 KONSTITUTIV-RT: chunk_alloc_count > 0 (A::allocate im Hot-Path REAL gerufen)", allocs_nach_insert > 0);
    chk("B2 T4 KONSTITUTIV-CT wirkt: Chunk-Zahl == ceil(kN / node_capacity)",
        kette.store().chunk_count() == (kN + Store::node_capacity() - 1u) / Store::node_capacity());
    kette.clear();
    chk("B3 clear gibt die Chunks REAL frei (chunk_alloc_count == 0 -> A::deallocate)",
        kette.store().chunk_alloc_count() == 0 && kette.store().chunk_count() == 0);

    std::cout << "    A8-S4 REFERENZ-LAST     : " << kN << " tier_insert + " << kN << " tier_lookup\n";
    std::cout << "    A8-S4 WALLCLOCK MESSUNG-AN: " << drive_ns << " ns\n";
    std::cout << "    A8-S4 chunk_allocs (T6)   : " << allocs_nach_insert << "\n";
    std::cout << "    A8-S4 record_phys_bytes   : cache_line_aligned=" << Store::record_phys_bytes()
              << "  aos_strict=" << StoreAoS::record_phys_bytes() << "\n";
    std::cout << "    A8-S4 node_capacity (T4)  : Node256=" << Store::node_capacity()
              << "  Node4=" << StoreNode4::node_capacity() << "\n";

    std::cout << "==== A8-S4: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
