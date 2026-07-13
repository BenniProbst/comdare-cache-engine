// V42 L-74c / I1 — Cross-ABI-Auto-Kopplung: der SearchAlgorithmAbiAdapter treibt beim tier_insert/lookup das
// telemetry-Organ AUTOMATISCH (record_node_touch), und die EINE tier_observe zieht es in den konsolidierten
// Observer-POD (axis_stats[10] = telemetry). Beweis über die Gattungs-ABI (tier_insert/lookup), nicht über explizites Organ-Treiben.
//
// HONEST-0-NACHZUG (#188-4c-i, Deep-Research 2026-07-13): ArtComposition ist eine REFERENZ-Hülle ohne store_type →
// container_is_store_backed_ = false. Die per-op-gekoppelten Observer (search axis_stats[0], telemetry axis_stats[10])
// bleiben real >0 (record_node_touch über tier_insert/lookup, KEIN Store-Scan). Die store-slot-gescannten Observer
// node_type[4] / memory_layout[5] / serialization[9] sind für die Hülle DESIGNIERT honest-0 (Re-Kopplung #234).
// Build: manuell mit /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS (siehe Build-Skript).

#define COMDARE_MEASUREMENT_ON       1
#define COMDARE_CE_ENABLE_STATISTICS 1

#include <anatomy/abi_adapter.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <compositions/art_reference.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>

namespace ana = comdare::cache_engine::anatomy;
namespace cc  = comdare::cache_engine::compositions;

// M-CE-22 / NDEBUG-No-Op-Fix (2026-07-13, Muster-F): harte Checks statt assert(). assert() ist unter NDEBUG
// (Release) ein No-Op -> dieser Test degenerierte im Release-Build zu `return 0` (kein echter Beweis). CE_CHECK
// prueft NDEBUG-unabhaengig und liefert bei Verletzung `return 1` (rot). static_assert() bleibt (compile-time).
#define CE_CHECK(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "[FAIL] " #cond " @ " << __FILE__ << ":" << __LINE__ << "\n";                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

int main() {
    ana::SearchAlgorithmAbiAdapter<ana::SearchAlgorithmAnatomy<cc::ArtComposition>> ad;

    // Treiben AUSSCHLIESSLICH über die Gattungs-ABI (kein explizites telemetry_organ-Treiben):
    for (int i = 0; i < 20; ++i)
        (void)ad.tier_insert(static_cast<std::uint64_t>(i), static_cast<std::uint64_t>(i) * 2u);
    for (int i = 0; i < 20; ++i) {
        std::uint64_t out = 0;
        (void)ad.tier_lookup(static_cast<std::uint64_t>(i), &out);
    }

    // I1: die EINE konsolidierte Observer-Methode (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta). Die früheren
    // V2-Felder sind subsumiert: telemetry→axis_stats[10], memory_layout→[5], serialization→[9], node_type→[4].
    ana::ComdareTierObserverSnapshot u{};
    ad.tier_observe(&u);
    std::cout << "Unified-POD nach 20 insert + 20 lookup ueber die ABI:\n"
              << "  search_insert=" << u.axis_stats[0][3] << " search_lookup=" << u.axis_stats[0][0]
              << " | telemetry_total=" << u.axis_stats[10][0] << " telemetry_leaf=" << u.axis_stats[10][1]
              << " telemetry_node=" << u.axis_stats[10][2] << " | observable_axes=" << u.observable_axis_count << "\n";

    CE_CHECK(u.axis_stats[0][3] == 20u); // search_insert
    CE_CHECK(u.axis_stats[0][0] >= 20u); // search_lookup
    // KERN-BEWEIS: telemetry wurde AUTOMATISCH ueber tier_insert/lookup gekoppelt (20+20 = 40 Knoten-Touches).
    CE_CHECK(u.axis_stats[10][0] == 40u); // telemetry_total_events
    CE_CHECK(u.axis_stats[10][1] == 40u); // telemetry_leaf_updates
    CE_CHECK(u.axis_stats[10][2] == 0u);  // telemetry_node_updates (LeafOnlyCounter verwirft Inner-Touch)
    CE_CHECK(u.observable_axis_count >= 5u);

    // #188-4c-i: Referenz-Hülle hat kein store_type → honest-0 (Re-Kopplung #234). Die store-slot-gescannten Achsen
    // memory_layout + serialization sind für die Hülle ehrlich 0 (der store-backed Pfad mit eigenem Store füllt sie >0).
    std::cout << "  scan-Achsen ueber Slot-Backing (Huelle honest-0): layout_records=" << u.axis_stats[5][1]
              << " layout_checksum=" << u.axis_stats[5][4] << " | serialization_records=" << u.axis_stats[9][1] << "\n";
    CE_CHECK(u.axis_stats[5][0] == 0u); // layout_scan_count (Huelle honest-0)
    CE_CHECK(u.axis_stats[5][1] == 0u); // layout_records_scanned (Huelle honest-0)
    CE_CHECK(u.axis_stats[9][0] == 0u); // serialization_serialize_count (Huelle honest-0)
    CE_CHECK(u.axis_stats[9][1] == 0u); // serialization_records_serialized (Huelle honest-0)

    // #188-4c-i: node_type-Slot-Scan (observe_node_find) läuft nur über ein echtes store_type → für die Referenz-Hülle
    // honest-0 (Re-Kopplung #234). Der store-backed Pfad liefert find_count/keys/checksum >0.
    std::cout << "  node_type ueber Slot-Backing (Huelle honest-0): find_count=" << u.axis_stats[4][0]
              << " keys=" << u.axis_stats[4][1] << " checksum=" << u.axis_stats[4][3] << "\n";
    CE_CHECK(u.axis_stats[4][0] == 0u); // node_find_count (Huelle honest-0)
    CE_CHECK(u.axis_stats[4][1] == 0u); // node_keys_stored (Huelle honest-0)
    CE_CHECK(u.axis_stats[4][3] == 0u); // node_last_checksum (Huelle honest-0)

    std::cout << "OK: abi_adapter tier_insert/lookup koppelt telemetry AUTOMATISCH -> der EINE tier_observe "
                 "(Cross-ABI konsolidierter Observer-POD ueber das Gattungs-Interface).\n";
    return 0;
}
