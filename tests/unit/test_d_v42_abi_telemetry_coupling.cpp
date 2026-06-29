// V42 L-74c / I1 — Cross-ABI-Auto-Kopplung: der SearchAlgorithmAbiAdapter treibt beim tier_insert/lookup das
// telemetry-Organ AUTOMATISCH (record_node_touch), und die EINE tier_observe zieht es in den konsolidierten
// Observer-POD (axis_stats[10] = telemetry). Beweis über die Gattungs-ABI (tier_insert/lookup), nicht über explizites Organ-Treiben.
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

    assert(u.axis_stats[0][3] == 20u); // search_insert
    assert(u.axis_stats[0][0] >= 20u); // search_lookup
    // KERN-BEWEIS: telemetry wurde AUTOMATISCH ueber tier_insert/lookup gekoppelt (20+20 = 40 Knoten-Touches).
    assert(u.axis_stats[10][0] == 40u); // telemetry_total_events
    assert(u.axis_stats[10][1] == 40u); // telemetry_leaf_updates
    assert(u.axis_stats[10][2] == 0u);  // telemetry_node_updates (LeafOnlyCounter verwirft Inner-Touch)
    assert(u.observable_axis_count >= 5u);

    // scan-Achsen-Auto-Kopplung: memory_layout + serialization wurden in der Observer-Befuellung ueber das ECHTE
    // Slot-Backing des container_ getrieben (Pfad-B Zustand-Scan) → records == tier_fill_level (alle Slots).
    std::cout << "  scan-Achsen ueber Slot-Backing: layout_records=" << u.axis_stats[5][1]
              << " layout_checksum=" << u.axis_stats[5][4] << " | serialization_records=" << u.axis_stats[9][1] << "\n";
    assert(u.axis_stats[5][0] == 1u);                // layout_scan_count
    assert(u.axis_stats[5][1] == u.tier_fill_level); // layout_records_scanned == alle Tier-Slots
    assert(u.axis_stats[9][0] == 1u);                // serialization_serialize_count
    assert(u.axis_stats[9][1] == u.tier_fill_level); // serialization_records_serialized

    // node_type-Auto-Kopplung: observe_node_find ueber die ECHTEN Tier-Keys (self-query) → checksum =
    // sum der gefundenen Key-Bytes; bei keys 0..19 (alle present) = 0+1+..+19 = 190.
    std::cout << "  node_type ueber Slot-Backing: find_count=" << u.axis_stats[4][0] << " keys=" << u.axis_stats[4][1]
              << " checksum=" << u.axis_stats[4][3] << "\n";
    assert(u.axis_stats[4][0] == 1u);                // node_find_count
    assert(u.axis_stats[4][1] == u.tier_fill_level); // node_keys_stored == alle Tier-Keys
    assert(u.axis_stats[4][3] == 190u);              // node_last_checksum sum(0..19) self-query

    std::cout << "OK: abi_adapter tier_insert/lookup koppelt telemetry AUTOMATISCH -> der EINE tier_observe "
                 "(Cross-ABI konsolidierter Observer-POD ueber das Gattungs-Interface).\n";
    return 0;
}
