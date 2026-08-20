// gen_golden_fullpilot — autoritativer Golden-Fixture-Generator (Bau-INC-2c/2h, BAUPLAN-INC2 §5).
//
// Materialisiert die binary_ids des FullSourceCatalog (NEW-GOLDEN-ALL-AXES 2026-07-18: alle 17 Achsen je 2 =
// 2^17 = 131072) über EXAKT denselben offiziellen LAZY Pfad, den die Limits-Tests fahren
// (catalog_static_levels<FullSourceCatalog> → ExperimentTree.build → StaticBinaryView, positions-getreu, O(Tiefe)
// Speicher je Dekodierung — KEINE mp_product-Materialisierung) und schreibt sie als eingefrorene Golden-Referenz. Seit Bau-INC-2c (F12iii, ABI-5) tragen die Pfade KEIN telemetry-Segment mehr
// (18 Slots; Telemetrie ist CEB-System-Achse im H-10-Sidecar). Die ABI-4-Historie bleibt additiv
// als golden_fullpilot_320_binary_ids_abi4.txt eingefroren (Messdaten nie löschen).
//
// Aufruf:
//   gen_golden_fullpilot <ausgabe.txt>   → schreibt die 131072-id-Datei (reine Inspektions-Option, NICHT ins git).
//   gen_golden_fullpilot --crc64          → druckt die CRC64-ECMA-182 der 131072 ids (Byte-Konvention: je binary_id
//                                           gefolgt von '\n', OHNE Kommentar-Header) — der committete Test-Anker
//                                           (kNewGolden131072Crc64), statt einer 62-MB-Datei (§0-GOAL CRC64).
//   gen_golden_fullpilot --golden320 <ausgabe.txt>
//                                         -> schreibt die 320-id-Byte-Wache (golden_320_catalog, 4*4*5*4) --
//                                           die committete tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt.
//
// STRUKT-R ORG-18 (2026-07-26): der --golden320-Modus wurde NACHGETRAGEN. Der Kopf-Kommentar der committeten
// 320er-Datei behauptete seit INC-2c "Regeneration NUR ueber gen_golden_fullpilot", aber dieses Werkzeug konnte
// ausschliesslich die 131072er-Form schreiben -- die Behauptung war unerfuellbar und der 320er-Re-Anker damit
// ein undokumentierter Handgriff. Jetzt ist der offizielle Weg real vorhanden (KEIN Behelfsweg mehr).

#include <profile_facade/source_catalog.hpp>

#include <builder/experiment_tree/experiment_tree.hpp>
#include <cache_engine/fingerprint/crc64.hpp>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fp  = comdare::fingerprint;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Aufruf: gen_golden_fullpilot <ausgabe.txt> | --crc64 | --golden320 <ausgabe.txt>\n";
        return 2;
    }

    // -- STRUKT-R ORG-18: --golden320 -- die 320er-Byte-Wache ueber DENSELBEN offiziellen LAZY Pfad, nur mit
    //    golden_320_catalog statt FullSourceCatalog. Eigener Frueh-Ausstieg, damit der 131072er-Baum (teuer)
    //    fuer diesen Modus gar nicht erst gebaut wird. --
    if (std::string_view{argv[1]} == "--golden320") {
        if (argc < 3) {
            std::cerr << "Aufruf: gen_golden_fullpilot --golden320 <ausgabe.txt>\n";
            return 2;
        }
        std::vector<ex::AxisLevel> const lv320 = tlz::catalog_static_levels<tlz::golden_320_catalog>();
        auto                             f320  = std::make_shared<ex::ExperimentNodeFactory>();
        ex::ExperimentTree               t320{f320};
        t320.build(lv320);
        ex::StaticBinaryView const v320 = t320.static_binary_view();
        std::ofstream              o320{argv[2], std::ios::trunc};
        if (!o320) {
            std::cerr << "FEHLER: kann '" << argv[2] << "' nicht schreiben\n";
            return 3;
        }
        o320 << "# GOLDEN-REFERENZ: " << v320.size()
             << " binary_ids des golden_320_catalog (4*4*5*4) -- ABI-7 / STRUKT-R ORG-18 (18 Slots, mit\n"
             << "# persistence_target-Segment; ohne telemetry-/isa-Segment).\n"
             << "# EINGEFROREN. test_profile_roundtrip vergleicht den PROFIL-Pfad gegen DIESE Liste (Resume "
                "#139-Schutz).\n"
             << "# Eine Zeile = ein binary_id (positions-getreue Reihenfolge des StaticBinaryView). Regeneration NUR\n"
             << "# ueber gen_golden_fullpilot --golden320 im koordinierten ABI-Fenster; die ABI-4/5/6-Historie bleibt\n"
             << "# additiv in golden_fullpilot_320_binary_ids_abi{4,5,6}.txt (Messdaten-Doktrin: nie loeschen).\n";
        for (std::size_t i = 0; i < v320.size(); ++i) o320 << v320[i].binary_id << "\n";
        std::cout << "gen_golden_fullpilot --golden320: " << v320.size() << " ids -> " << argv[2] << "\n";
        return v320.size() == 320 ? 0 : 4;
    }

    std::vector<ex::AxisLevel> const levels = tlz::catalog_static_levels<tlz::FullSourceCatalog>();

    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();

    // ── --crc64: der Test-Anker. CRC64 ueber view[0..N-1].binary_id, je gefolgt von '\n' (deckungsgleich zur Datei,
    //    OHNE Kommentar-Header). Druckt den Wert + (falls schon verankert) den Abgleich gegen kNewGolden131072Crc64. ──
    if (std::string_view{argv[1]} == "--crc64") {
        std::uint64_t crc = 0;
        for (std::size_t i = 0; i < view.size(); ++i) {
            crc = fp::crc64_ecma182_update(crc, std::string_view{view[i].binary_id});
            crc = fp::crc64_ecma182_update(crc, std::string_view{"\n"});
        }
        std::cout << "gen_golden_fullpilot: N=" << view.size() << " ids  CRC64-ECMA-182 = 0x" << std::hex
                  << std::uppercase << std::setw(16) << std::setfill('0') << crc << std::dec << "\n";
        std::cout << "  anchor kNewGolden131072Crc64 = 0x" << std::hex << std::uppercase << std::setw(16)
                  << std::setfill('0') << tlz::kNewGolden131072Crc64 << std::dec
                  << (crc == tlz::kNewGolden131072Crc64 ? "  [MATCH]" : "  [MISMATCH]") << "\n";
        bool const ok = (view.size() == 131072) && (crc == tlz::kNewGolden131072Crc64);
        return ok ? 0 : 4;
    }

    std::ofstream out{argv[1], std::ios::trunc};
    if (!out) {
        std::cerr << "FEHLER: kann '" << argv[1] << "' nicht schreiben\n";
        return 3;
    }
    out << "# GOLDEN-REFERENZ: " << view.size()
        << " binary_ids des FullSourceCatalog -- NEW-GOLDEN-ALL-AXES: 17 Kompositions-Achsen je 2 x\n"
        << "# persistence_target je 1 = 2^17 = 131072 (Ganz-System-Regressions-Detektor). ABI-7 / STRUKT-R ORG-18\n"
        << "# (18 Slots, mit persistence_target-Segment; ohne telemetry-/isa-Segment).\n"
        << "# EINGEFROREN. view.size() ist emergent (StaticBinaryView, lazy) -- KEINE Zahl hartkodiert. Regeneration "
           "NUR\n"
        << "# ueber gen_golden_fullpilot (tools/gen_golden_fullpilot) im koordinierten ABI-Fenster. Die 320-Grundlage\n"
        << "# bleibt additiv in golden_fullpilot_320_binary_ids.txt (+ _abi4/_abi5/_abi6-Historie), "
           "messdaten-erhaltend.\n";
    for (std::size_t i = 0; i < view.size(); ++i) out << view[i].binary_id << "\n";

    std::cout << "gen_golden_fullpilot: " << view.size() << " ids -> " << argv[1] << "\n";
    return view.size() == 131072 ? 0 : 4;
}
