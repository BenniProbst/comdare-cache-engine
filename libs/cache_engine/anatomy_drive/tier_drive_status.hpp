#pragma once
// K2 -- DIE STUFEN-NEUTRALEN STATUS-CODES des Antriebs-Zugriffs.
//
// ============================================================================================
// WARUM DIESE DATEI EXISTIERT (Owner-Entscheid 09.08.2026, K2 = Option (a))
// ============================================================================================
// Owner woertlich: "der Loader wandert in eine stufen-neutrale Bibliothek, weil das Pruefdock der
// CEB und das Pruefdock der Hybrid-Tier-Binary jeweils technisch identisch bei Konfiguration sein
// muessen."
//
// Der LOADER selbst war bereits frei -- comdare::anatomy_module_loader ist eine eigene Lib, die nur
// Boost::mp11 und libdl zieht; sie liegt lediglich physisch unter builder/. Was NICHT frei war, ist
// das ANTRIEBS-BUENDEL: SearchAlgorithmDrive und acquire_search_algorithm_drive standen in
// builder/pruef_dock/search_algorithm_dock.hpp, zusammen mit der IPruefDock-Implementierung und dem
// Konformitaets-Gate. Wer den Drive wollte, zog den Builder mit -- und genau daran haing K2.
//
// Die einzige echte Bindung des Drive an den Builder waren DIESE STATUS-KONSTANTEN. Sie stehen
// deshalb jetzt hier, in der neutralen Schicht.
//
// ============================================================================================
// EINE WANDERUNG, KEIN BRUCH
// ============================================================================================
// builder/pruef_dock/pruef_dock.hpp inkludiert diese Datei und re-exportiert JEDEN Namen per
// using. Fuer die Bestands-Konsumenten aendert sich damit NICHTS -- weder Include-Pfad noch
// Namespace-Qualifikation. Das ist Absicht: eine Extraktion, die 7 Konsumenten anfasst, waere
// dieselbe Aenderung mit sieben zusaetzlichen Bruchstellen.
//
// WAS DAS FUER DIE DOKTRIN-ZEILE BEDEUTET: pruef_dock.hpp:10-14 sagt "IPruefDock lebt nur im
// Builder-Binary". Das bleibt WAHR und unangetastet -- IPruefDock ist nicht gewandert. Gewandert
// sind nur die Status-Codes und das Antriebs-Buendel, also genau die Teile, die KEINE
// Dock-Semantik tragen, sondern die ABI-Kante beschreiben. Die Doktrin-Zeile wird dort additiv
// praezisiert, nicht ersetzt.
//
// @autoritaet Owner-Entscheid 09.08.2026 (K2 = Option (a)); Wellenplan-Korb-B-Posten B-10
//             "Loader stufen-neutral"
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 7 K2
//         (dort mit datiertem Nachtrag als BEANTWORTET markiert)

#include <string_view>

namespace comdare::cache_engine::anatomy_drive {

// errno-Stil Status-Codes (0 == ok). Werte und Reihenfolge sind UNVERAENDERT aus
// builder/pruef_dock/pruef_dock.hpp uebernommen -- eine Umnummerierung waere ein stiller
// Verhaltensbruch bei jedem Aufrufer, der Zahlen vergleicht statt Namen.
inline constexpr int dock_status_ok                   = 0;
inline constexpr int dock_status_no_anatomy           = 1; ///< Handle ohne IAnatomyBase
inline constexpr int dock_status_wrong_genus          = 2; ///< Modul-Gattung != Dock-Gattung
inline constexpr int dock_status_subinterface_missing = 3; ///< gattungs-Antrieb (dynamic_cast) == nullptr (alte DLL)
inline constexpr int dock_status_conformance_failed   = 4; ///< V5-I4: Konformitaets-Gate vor der Messung
// NAHT-1 (Owner-KERN 09.08.2026): die zweiseitige Aktivierung braucht ZWEI eigene Zustaende. Bis
// dahin fielen beide in dock_status_subinterface_missing (3) = "alte DLL, sauber degradieren" --
// und genau dieses stille Degrade ist der Grund, warum eine Reihe voller ehrlicher Nullen wie eine
// Messung aussah. Die Trennung ist der Unterschied zwischen "hier wurde bewusst nicht gemessen"
// und "hier behauptet jemand etwas, das nicht stimmt".
inline constexpr int dock_status_mess_gate_mismatch = 5; ///< Legende und Mess-Flaeche widersprechen sich -> DEFEKT
inline constexpr int dock_status_mess_deaktiviert   = 6; ///< Messeinrichtung EINER Seite bewusst aus

/// Namens-Totalitaet: ein Status, den diese Funktion nicht kennt, faellt in "unknown" und ist im
/// Bericht STUMM -- und ein stummer Zustand ist schlimmer als gar keiner, weil er wie ein bekannter
/// Fehler aussieht (NAHT-1-Lehre, im Bestands-Header ausfuehrlich begruendet).
[[nodiscard]] inline std::string_view dock_status_name(int s) noexcept {
    switch (s) {
        case dock_status_ok: return "ok";
        case dock_status_no_anatomy: return "no_anatomy";
        case dock_status_wrong_genus: return "wrong_genus";
        case dock_status_subinterface_missing: return "subinterface_missing";
        case dock_status_conformance_failed: return "conformance_failed";
        case dock_status_mess_gate_mismatch: return "mess_gate_mismatch";
        case dock_status_mess_deaktiviert: return "mess_deaktiviert";
        default: return "unknown";
    }
}

} // namespace comdare::cache_engine::anatomy_drive
