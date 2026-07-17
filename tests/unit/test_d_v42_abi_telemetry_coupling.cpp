// V42 L-74c / I1 — URSPRUENGLICH: Cross-ABI-Auto-Kopplung des telemetry-Organs in POD-Slot axis_stats[10].
// UMGEWIDMET (Bau-INC-2c / F12iii, ABI-5): telemetry ist SYSTEM-Achse, kein Kompositions-Slot und keine POD-Zeile
// mehr — dieser Test ist jetzt der NEGATIV-GUARD der Entfernung: (a) POD hat 18 Zeilen, (b) Slot [10] ist
// value_handle (nicht telemetry), (c) die Gattungs-ABI treibt weiterhin real (search-Zeile > 0). Die honest-0
// Store-Checks (#188-4c-i) bleiben unveraendert gueltig.
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
#include <string_view>

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

    // I1: die EINE konsolidierte Observer-Methode (axis_stats[18][8] + seg_ns[18]/Pfad B + Meta).
    // F12iii-NEGATIV-GUARD (compile-time): POD traegt exakt 18 Achsen-Zeilen — keine telemetry-Zeile.
    static_assert(ana::kV3AxisCount == 18, "Bau-INC-2c: telemetry ist System-Achse, POD hat 18 Zeilen");
    ana::ComdareTierObserverSnapshot u{};
    ad.tier_observe(&u);
    std::cout << "Unified-POD nach 20 insert + 20 lookup ueber die ABI:\n"
              << "  search_insert=" << u.axis_stats[0][3] << " search_lookup=" << u.axis_stats[0][0]
              << " | observable_axes=" << u.observable_axis_count << "\n";

    CE_CHECK(u.axis_stats[0][3] == 20u); // search_insert (Gattungs-ABI treibt weiterhin real)
    CE_CHECK(u.axis_stats[0][0] >= 20u); // search_lookup
    // F12iii-POSITIV-GUARD: Slot [10] IST value_handle (nicht mehr telemetry). Compile-time-Beweis über das
    // Single-Source-Schema — die erste Spalte der Zeile [10] heißt "access" (value_handle-Feld), NICHT ein
    // telemetry-Feld ("total_events"/"leaf_updates"). Das ist der eigentliche F12iii-Nachweis (die Zeile hat
    // die Identität gewechselt), stärker als der frühere runtime-!=40-Vergleich.
    static_assert(std::string_view{ana::kV3AxisSchema[10].names[0]} == std::string_view{"access"},
                  "Bau-INC-2c: Schema-Zeile [10] muss value_handle sein (erste Spalte 'access'), nicht telemetry");
    // Und die alte per-op telemetry-Kopplung (20 insert + 20 lookup → 40 Knoten-Touches in Slot [10]) existiert
    // nicht mehr: die value_handle-Zeile zählt Handle-Zugriffe, keine Knoten-Touches → != der alten 40er-Signatur.
    CE_CHECK(u.axis_stats[10][0] != 40u || u.axis_stats[10][1] != 40u);
    CE_CHECK(u.observable_axis_count >= 4u);

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

    std::cout << "OK (F12iii-Guard): telemetry ist KEINE POD-Zeile mehr (18 Achsen, [10]=value_handle); die "
                 "Gattungs-ABI treibt weiterhin real (search-Zeile > 0).\n";
    return 0;
}
