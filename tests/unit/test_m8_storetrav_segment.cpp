// M8-Basis (Befund-2-Weg-A) — store-traversierbare LinearScan-Komposition IN-PROCESS durch den abi_adapter.
// Ergänzt test_pathb_segment_timer (das NUR Art/Hot/Masstree = Trie/Weg-B deckt) um den STORE-TRAVERSIERBAREN Fall
// (LinearScan): belegt A2.5/Befund-2 für store-geroutete Suche literal + ist die Verifikations-Basis für die spätere
// search_organ_-Voll-Entfernung (M8). Prüft: (1) tier_search_routes_through_store()==true; (2) Basis-Ops korrekt
// (Keys < 65536, LinearScanSearchAlgo key_type=uint16); (3) alle 19 seg_ns > 0 (deckt den Befund auf, dass die
// seg_ns-Key-Ernte für NICHT-MementoAxis-store-traversierbare Tiere wie LinearScan ohne den container_-Harvest-Fix
// auf nk=1 degeneriert → seg_ns einzelner Achsen = 0; mit dem Fix [abi_adapter fill_segment_timing_v3, Key-Ernte aus
// container_.save_state() für store-traversierbare] alle > 0).
// Build (scratch, wie test_pathb): cl /std:c++latest /EHsc /bigobj /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1
//   + voller ADHOC-Include-Satz (libs/cache_engine[+include,+src], libs/common, build/msvc-release/generated/**, boost_mp11).
#include <builder/codegen/all_axes_umbrella.hpp>          // LinearScanSearchAlgo (traversal::axis_03a_search_algo::)
#include <compositions/art_reference.hpp>                 // ArtComposition (19-Achsen-Vorlage)
#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::compositions {
// LinearScanComposition = ArtComposition-Spiegel, NUR search_algo getauscht auf das STORE-TRAVERSIERBARE
// LinearScanSearchAlgo (axis_03a_store_traversable=true). Alle übrigen 18 Achsen identisch zu ArtComposition.
struct LinearScanComposition {
    using search_algo        = traversal::axis_03a_search_algo::LinearScanSearchAlgo;   // <-- store-traversierbar
    using cache_traversal    = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping            = traversal::axis_03m_mapping::DirectPlacement;
    using path_compression   = nodes::axis_02_path_compression::PathCompressionNone;
    using node_type          = nodes::axis_04_node_type::ObservableNodeType<nodes::axis_04_node_type::Node256NodeType>;
    using memory_layout      = memory_layout::axis_05_memory_layout::ObservableMemoryLayout<memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout>;
    using allocator          = allocator::axis_06_allocator::MimallocAllocator;
    using prefetch           = prefetch::axis_07_prefetch::NonePrefetch;
    using concurrency        = concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
    using serialization      = serialization::axis_10_serialization::ObservableSerialization<serialization::axis_10_serialization::RawBinarySerialization>;
    using telemetry          = telemetry::axis_11_telemetry::ObservableTelemetry<telemetry::axis_11_telemetry::LeafOnlyCounter>;
    using value_handle       = value_handle::axis_14_value_handle::InlineValueHandle;
    using isa                = hardware::axis_09_isa::Amd64Isa;
    using index_organization = search_engine::axis_01_index_organization::IotIndexOrganization;
    using io_dispatch        = io::axis_io::InMemoryOnly;
    using migration_policy   = migration::axis_migration::NoMigration;
    using filter             = filter::axis_filter::BloomFilter;
    using queuing_q1         = queuing::axis_q1_queuing::NoBuffer;
    using queuing_q2         = queuing::axis_q2_queuing::LazyFlush;
    static constexpr std::string_view paper_id    = "M8-test (LinearScan store-traversierbar)";
    static constexpr std::string_view paper_title = "LinearScan (ART Node4-Baseline, Leis ICDE 2013) - store-traversierbare M8-Basis";
    static constexpr std::string_view name        = "LinearScanComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::LinearScanComposition", "tests/unit/test_m8_storetrav_segment.cpp");
};
}  // namespace

namespace an = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
static int g_fail = 0;
static void chk(std::string const& w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== M8-Basis: store-traversierbare LinearScan-Komposition durch abi_adapter ====\n";
    using Anatomy = an::SearchAlgorithmAnatomy<comp::LinearScanComposition>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    chk("tier_search_routes_through_store()==true (LinearScan store-traversierbar)", Adapter::tier_search_routes_through_store());

    Adapter tier;
    auto* drv = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    chk("IObservableTier via dynamic_cast vorhanden", drv != nullptr);
    if (!drv) { std::cout << "==== M8-Basis: ABBRUCH ====\n"; return 1; }

    constexpr std::uint64_t kN = 4096;   // Keys < 65536 (LinearScanSearchAlgo key_type=uint16, K9-(d)-LIMIT)
    for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);
    chk("tier_size == kN", drv->tier_size() == kN);
    std::uint64_t v = 0; bool hit = drv->tier_lookup(1234, &v);
    chk("lookup(1234) hit + Wert", hit && v == 1234u * 7u + 1u);
    chk("lookup(99999) miss (nie geladen)", !drv->tier_lookup(99999, &v));

    an::ComdareTierObserverSnapshot u{};
    drv->tier_observe(&u);
    bool all_pos = true; int zero_t = -1;
    for (int t = 0; t < 19; ++t) if (u.seg_ns[t] <= 0) { all_pos = false; if (zero_t < 0) zero_t = t; }
    chk(std::string("alle 19 seg_ns > 0 (container_-Key-Ernte, KEINE nk=1-Degeneration)") + (all_pos ? "" : (" (erste 0 bei T" + std::to_string(zero_t) + ")")), all_pos);
    chk("T0 search seg_ns > 0 (Such-Pfad store-geroutet)", u.seg_ns[0] > 0);
    chk("tier_fill_level == kN", u.tier_fill_level == kN);
    std::cout << "    seg_ns[T0,T4,T5,T6] = " << u.seg_ns[0] << "," << u.seg_ns[4] << "," << u.seg_ns[5] << "," << u.seg_ns[6] << "\n";

    std::cout << "==== M8-Basis: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
