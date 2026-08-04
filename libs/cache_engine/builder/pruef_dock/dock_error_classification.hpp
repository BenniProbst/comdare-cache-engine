#pragma once
// E-24 C5 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 6.2
// FK-7) -- die CEB-SEITIGE KLASSIFIZIERUNG der Dock-/Lade-Fehlerpfade.
//
// DAS PROBLEM AM IST (erhoben 04.08., nicht angenommen): der Mess-Eintritt kennt zwei errno-Vokabulare --
//   dock_status_*   (pruef_dock.hpp:36-41: ok=0/no_anatomy=1/wrong_genus=2/subinterface_missing=3/
//                    conformance_failed=4)
//   loader status_* (anatomy_module_loader.hpp:36-44: 9 Codes von file_not_found bis null_module)
// Beide sind reine TRANSPORTFORM: ein int reist, ein Name laesst sich daraus drucken -- aber NIEMAND
// uebersetzt sie in die Fehlerklassen-Welt (measurement/axis_error.hpp). Folge am produktiven Pfad
// (apps/f15_compare/main.cpp): ein Modul mit status != ok bekam GAR KEINE Ausgabe-Zeile -- kein CSV,
// keine klassifizierte Log-Zeile, nur eine Konsolen-Zeile mit dem rohen Status-Namen. Das ist exakt die
// stille Luecke, die die Mess-Fehler-Doktrin verbietet ("Messung NIE als Nullen", Memory
// feedback_measurement_failure_visibility_csv_failed_not_null_plus_log): die Auswertung sieht ein
// FEHLEN, nicht ein FEHLSCHLAGEN.
//
// WAS DIESE DATEI TUT -- und was sie bewusst NICHT tut:
//   * TUT: Transport-Int -> measurement::DockErrorClass (D2) -> CSV-Zelle "failed" + stabiles Log-Etikett.
//   * TUT NICHT: sie fasst pruef_dock.hpp NICHT an. Die dock_status_*-Konstanten und die
//     IPruefDock::measure-Signatur stehen unter HY-D2-Freeze (Bauplan Paragraf 4.4 / E24-DOSSIER:269) --
//     FK-7 haelt sie ausdruecklich als "ABI-nahe Transportform" fest (Paragraf 6.2). Die Uebersetzung
//     sitzt deshalb HIER, beim Aufrufer, und nicht dort.
//
// DIE INT-WERTE SIND HIER COMPILE-HART GEPINNT (Abschnitt "Transport-Pin" unten). Der Grund ist die
// Lehre "gruene Tests zementieren alte Ordnung" in ihrer umgekehrten Richtung: eine Abbildung, die die
// fremden Konstanten nur BENUTZT, laeuft still falsch, wenn dort jemand eine Zahl dreht. Der Pin macht
// aus diesem stillen Auseinanderlaufen einen Uebersetzungs-Fehler.
//
// LAYER-SCHNITT: axis_error.hpp (measurement) kennt die builder-Seite NICHT und darf sie nicht kennen.
// Deshalb traegt die measurement-Seite die TAXONOMIE (DockErrorClass + Etiketten + Wachen) und diese
// builder-Seite die ABBILDUNG. Die Abhaengigkeit zeigt in genau eine Richtung.
//
// ABI-NEUTRAL (a-Teil): header-only, reine Host-Seite. Kein Byte an abi_adapter.hpp, an Wire-PODs, an
// abi/*_decl.hpp oder an einer Stempel-/Fingerprint-Flaeche.

#include "pruef_dock.hpp" // dock_status_* (READ-ONLY, HY-D2-Freeze) + dock_status_name

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // loader status_* + status_name
#include <cache_engine/measurement/axis_error.hpp>                 // DockErrorClass / SampleStatus / Etiketten

#include <optional>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

namespace cem = ::comdare::cache_engine::measurement;

// ---------------------------------------------------------------------------------------------------
// Transport-Pin: die fremden errno-Konstanten, gegen die diese Abbildung gebaut ist.
// Dreht dort jemand eine Zahl, bricht die Uebersetzung HIER -- nicht still in der Auswertung.
// ---------------------------------------------------------------------------------------------------
static_assert(dock_status_ok == 0);
static_assert(dock_status_no_anatomy == 1);
static_assert(dock_status_wrong_genus == 2);
static_assert(dock_status_subinterface_missing == 3);
static_assert(dock_status_conformance_failed == 4);
static_assert(anatomy_loader::status_ok == 0);

/// classify_dock_status(s) -- die D2-Klasse eines Dock-Ergebnisses. LEER genau dann, wenn s == ok.
///
/// Ein unbekannter Int wird NICHT verschwiegen und NICHT auf eine benachbarte Klasse geraten, sondern
/// traegt die eigene Klasse UnbekannterDockStatus. Ein neuer dock_status-Wert faellt damit sichtbar auf,
/// statt sich als "irgendein Mess-Fehler" zu tarnen.
///
/// GRENZE (bewusst, und der Grund fuer classify_dock_selection unten): dock_status_no_anatomy traegt am
/// Ist ZWEI Bedeutungen. pruef_dock_sequencer.hpp setzt ihn sowohl fuer "das Modul hat keine
/// IAnatomyBase" als auch fuer "die Registry hat kein Dock fuer diese Gattung". Aus dem Int allein ist
/// das nicht trennbar -- also raet diese Funktion auch nicht, sondern liefert die Modul-Seite und
/// ueberlaesst die Trennung der Stelle, die den Handle sieht.
[[nodiscard]] constexpr std::optional<cem::DockErrorClass> classify_dock_status(int s) noexcept {
    switch (s) {
        case dock_status_ok: return std::nullopt;
        case dock_status_no_anatomy: return cem::DockErrorClass::ModulOhneAnatomie;
        case dock_status_wrong_genus: return cem::DockErrorClass::FremdeGattung;
        case dock_status_subinterface_missing: return cem::DockErrorClass::AntriebsInterfaceFehlt;
        case dock_status_conformance_failed: return cem::DockErrorClass::KonformitaetGescheitert;
        default: return cem::DockErrorClass::UnbekannterDockStatus;
    }
}

/// classify_dock_selection(hat_anatomie) -- die Trennung, die der Transport-Int nicht hergibt: WARUM kam
/// fuer dieses Handle kein Dock zustande? Ohne Anatomie ist es die MODUL-Seite (die .so exportiert keine
/// IAnatomyBase), mit Anatomie die REGISTRY-Seite (die Gattung ist deklariert, aber kein Dock registriert).
/// Beide Befunde haben verschiedene Adressaten -- Modul-Autor vs. Registry-Belegung
/// (pruef_dock_registry_default.hpp) -- und muessen deshalb im Log unterscheidbar bleiben.
[[nodiscard]] constexpr cem::DockErrorClass classify_dock_selection(bool handle_hat_anatomie) noexcept {
    return handle_hat_anatomie ? cem::DockErrorClass::KeinDockFuerGattung : cem::DockErrorClass::ModulOhneAnatomie;
}

/// classify_loader_status(s) -- die D2-Klasse eines LADE-Ergebnisses. LEER genau dann, wenn s == status_ok.
///
/// Alle neun Loader-Codes fallen in EINE Klasse (ModulLadeFehler) -- und das ist Absicht, keine
/// Grobheit: die Domaenen-Aussage ist "die Messung kam nicht bis zum Dock". Die neun Gruende sind damit
/// nicht verloren, denn der Loader traegt sie bereits als stabilen Namen (status_name), und
/// dock_failure_log_line haengt genau diesen Namen als Transport-Detail an. Eine zweite, parallele
/// Neun-Klassen-Taxonomie waere eine Verdopplung des Loader-Vokabulars ohne neue Aussage.
[[nodiscard]] constexpr std::optional<cem::DockErrorClass> classify_loader_status(int s) noexcept {
    if (s == anatomy_loader::status_ok) return std::nullopt;
    return cem::DockErrorClass::ModulLadeFehler;
}

/// dock_failure_cell() -- der CSV-Zell-Token eines klassifizierten Dock-Fehlers. Immer "failed", nie eine
/// Null und nie ein "n/a" (Begruendung an dock_error_sample_status in axis_error.hpp).
[[nodiscard]] constexpr std::string_view dock_failure_cell(cem::DockErrorClass c) noexcept {
    return cem::sample_status_token(cem::dock_error_sample_status(c));
}

/// dock_failure_log_line() -- die klassifizierte Log-Zeile eines Dock-/Lade-Fehlers.
///
/// FORM (bewusst stabil und grep-bar, Muster der bestehenden Politik-Praefixe in axis_error.hpp):
///   "Mess-Fehler[<klassen_etikett>] modul=<name> transport=<roher_status_name> zelle=failed"
/// Das Praefix kommt aus FailedCellD2Policy::log_prefix() -- dieselbe Single-Source, die die D2-Politik
/// schon fuehrt; das Klassen-Etikett aus dock_error_label (disjunkt gegen JEDES andere Vokabular, per
/// static_assert in axis_error.hpp); der Transport-Name bleibt woertlich erhalten, damit die neun
/// Loader-Gruende und die fuenf Dock-Codes im Log nachlesbar sind und nicht in der Klasse verschwinden.
[[nodiscard]] inline std::string dock_failure_log_line(cem::DockErrorClass c, std::string_view modul_name,
                                                       std::string_view transport_name) {
    std::string out;
    out.reserve(96);
    out += cem::FailedCellD2Policy::log_prefix();
    out += '[';
    out += cem::dock_error_label(c);
    out += "] modul=";
    out += modul_name.empty() ? std::string_view{"<unbenannt>"} : modul_name;
    out += " transport=";
    out += transport_name.empty() ? std::string_view{"<keiner>"} : transport_name;
    out += " zelle=";
    out += dock_failure_cell(c);
    return out;
}

/// dock_failure_csv() -- die MESS-CSV eines gescheiterten Moduls: eine Kopfzeile + GENAU EINE Datenzeile,
/// deren Wert-Zelle "failed" traegt. Sie tritt an die Stelle der Trace-CSV, die es fuer dieses Modul nicht
/// gibt -- damit die Auswertung ein FEHLSCHLAGEN sieht statt einer fehlenden Datei (D2-Doktrin).
/// Die Spalten sind bewusst schmal und selbsterklaerend: dies ist ein Fehler-Datensatz, keine Messreihe.
[[nodiscard]] inline std::string dock_failure_csv(cem::DockErrorClass c, std::string_view modul_name,
                                                  std::string_view transport_name) {
    std::string out;
    out.reserve(160);
    out += "modul,dock_transport,fehlerklasse,domaene,wert\n";
    out += modul_name.empty() ? std::string_view{"<unbenannt>"} : modul_name;
    out += ',';
    out += transport_name.empty() ? std::string_view{"<keiner>"} : transport_name;
    out += ',';
    out += cem::dock_error_label(c);
    out += ",sample,";
    out += dock_failure_cell(c);
    out += '\n';
    return out;
}

// ---------------------------------------------------------------------------------------------------
// Selbst-Beweise (compile-time): die Abbildung ist total, die Zelle ist nie still.
// ---------------------------------------------------------------------------------------------------
static_assert(!classify_dock_status(dock_status_ok).has_value(), "ok ist KEIN Fehlerpfad.");
static_assert(classify_dock_status(dock_status_wrong_genus) == cem::DockErrorClass::FremdeGattung);
static_assert(classify_dock_status(dock_status_subinterface_missing) == cem::DockErrorClass::AntriebsInterfaceFehlt);
static_assert(classify_dock_status(dock_status_conformance_failed) == cem::DockErrorClass::KonformitaetGescheitert);
static_assert(classify_dock_status(4242) == cem::DockErrorClass::UnbekannterDockStatus,
              "Ein unbekannter Transport-Int wird SICHTBAR, nicht auf eine Nachbar-Klasse geraten.");
static_assert(classify_dock_selection(true) == cem::DockErrorClass::KeinDockFuerGattung);
static_assert(classify_dock_selection(false) == cem::DockErrorClass::ModulOhneAnatomie);
static_assert(!classify_loader_status(anatomy_loader::status_ok).has_value());
static_assert(classify_loader_status(anatomy_loader::status_abi_major_mismatch) == cem::DockErrorClass::ModulLadeFehler,
              "Ein abgelehntes Alt-Modul ist ein Lade-Fehler -- G5-relevant, und nie eine leere Zelle.");
static_assert(dock_failure_cell(cem::DockErrorClass::ModulLadeFehler) == std::string_view{"failed"});

} // namespace comdare::cache_engine::builder::pruef_dock
