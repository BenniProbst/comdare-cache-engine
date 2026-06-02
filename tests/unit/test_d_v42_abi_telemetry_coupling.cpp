// V42 L-74c — Cross-ABI-Auto-Kopplung: der SearchAlgorithmAbiAdapter treibt beim tier_insert/lookup das
// telemetry-Organ AUTOMATISCH (record_node_touch), und fill_observer_v2 zieht es in den flachen V2-POD.
// Beweis über die Gattungs-ABI (tier_insert/lookup), nicht über explizites Organ-Treiben.
// Build: manuell mit /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS (siehe Build-Skript).

#define COMDARE_MEASUREMENT_ON 1
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
    for (int i = 0; i < 20; ++i) (void)ad.tier_insert(static_cast<std::uint64_t>(i), static_cast<std::uint64_t>(i) * 2u);
    for (int i = 0; i < 20; ++i) { std::uint64_t out = 0; (void)ad.tier_lookup(static_cast<std::uint64_t>(i), &out); }

    ana::ComdareTierObserverSnapshotV2 v2{};
    ad.fill_observer_v2(&v2);
    std::cout << "V2 nach 20 insert + 20 lookup ueber die ABI:\n"
              << "  search_insert=" << v2.search_insert_count
              << " search_lookup=" << v2.search_lookup_count
              << " | telemetry_total=" << v2.telemetry_total_events
              << " telemetry_leaf=" << v2.telemetry_leaf_updates
              << " telemetry_node=" << v2.telemetry_node_updates
              << " | observable_axes=" << v2.observable_axis_count << "\n";

    assert(v2.search_insert_count == 20u);
    assert(v2.search_lookup_count >= 20u);
    // KERN-BEWEIS: telemetry wurde AUTOMATISCH ueber tier_insert/lookup gekoppelt (20+20 = 40 Knoten-Touches).
    assert(v2.telemetry_total_events == 40u);
    assert(v2.telemetry_leaf_updates == 40u);
    assert(v2.telemetry_node_updates == 0u);   // LeafOnlyCounter verwirft Inner-Touch
    assert(v2.observable_axis_count >= 5u);

    // KERN-BEWEIS 2: derselbe V2-POD über das EIGENSTÄNDIGE Sub-Interface IObservableTierV2 (ABI-robust —
    // wie der Host ihn über die DLL-Grenze zieht: dynamic_cast<IObservableTierV2*>; alte Module → nullptr →
    // Degrade auf V1, KEIN vtable-Append-Crash). Hier gelingt der Cast (der neue Adapter erbt IObservableTierV2).
    ana::IObservableTierV2* itier2 = dynamic_cast<ana::IObservableTierV2*>(static_cast<ana::IObservableTier*>(&ad));
    assert(itier2 != nullptr);
    ana::ComdareTierObserverSnapshotV2 v2_abi{};
    itier2->tier_observe_v2(&v2_abi);
    assert(v2_abi == v2);                       // über das Sub-Interface == direkt
    assert(v2_abi.telemetry_total_events == 40u);
    std::cout << "  via IObservableTierV2::tier_observe_v2: telemetry_total=" << v2_abi.telemetry_total_events
              << " (== fill_observer_v2, ABI-robust dynamic_cast)\n";

    std::cout << "OK: abi_adapter tier_insert/lookup koppelt telemetry AUTOMATISCH -> tier_observe_v2 (Cross-ABI V2-POD ueber Interface).\n";
    return 0;
}
