#pragma once
// HY-A1 (Dock-Schicht) -- DIE VERTRAGS-REGISTRY der Hybrid-Pruef-Docks + der Deskriptor, der sie
// ueber die Konfigurations-Grenze traegt.
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH (Owner-Entscheid E1, 02.08.2026)
// ============================================================================================
//   "Sie haben mehrere Pruef-docks und verwenden fuer den Einsatz ihrer Pruef-Docks zur Proxy
//    Verwendung ihrer Tier-Binaries als Factory Pattern, in der Regel als Ausnahmen std::variant.
//    Aber in den plain Tier-Binaries ist das verboten. Die hybrid-Tier-Binaries fahren eine
//    Zwischenloesung zwischen statischen Pruef-Docks und austauschbaren plain Tier-Binaries je
//    Pruefdock, das ist eine XML Konfiguration auf Wunsch des anwenders in der Auswertungsphase."
//
// Und die aeltere Owner-KORREKTUR Paragraf 49 (20.07.2026), die unveraendert bindend bleibt:
// variant NUR als Traeger abweichender Unter-Pruef-Dock-TYPEN/-VERTRAEGE, gelesen und verarbeitet
// per Abstract-Factory-Methode; die Haupt-Kommunikation zu den Tier-Observern bleibt STATISCH.
//
// ZUM ANKER, weil hier eine Falle liegt: das Design-Dokument verweist fuer diese Korrektur auf
// die Ledger-Zeile 2663. Am 17.08.2026 nachgemessen zeigt sie auf eine voellig andere Stelle --
// der Wortlaut steht bei Zeile 20612 ("49-KORREKTUR (2026-07-20, User)"). Der Verweis ist nicht
// falsch GESCHRIEBEN, sondern GEWANDERT: das Dokument ist seit dem 02.08. auf ueber 30.000 Zeilen
// gewachsen und zitiert seinen eigenen stale Verweis sogar selbst weiter (bei Zeile 18012).
// Deshalb steht hier das DATUM und ein suchbares Stichwort statt eines Zeilenverweises -- Datum
// und Wortlaut wandern nicht.
// (Die Zahlen oben sind bewusst als "Zeile NNNN" geschrieben und nicht in der Verweis-Form: genau
// diese Form ist repo-weit durch die Ratsche in test_anker_marke_statt_ledgerzeile.cpp gedeckelt,
// und eine Datei, die den Drift BESCHREIBT, soll den Bestand nicht um zwei erhoehen.)
//
// ============================================================================================
// DIE UNTERSCHEIDUNG, AN DER ALLES HAENGT: VERTRAG (Daten) gegen DOCK-TYP (Code)
// ============================================================================================
// Ein VERTRAG ist eine Zeile in der Tabelle unten. Er ist ein XML-Token und ein Index -- Daten.
// Ein DOCK-TYP ist eine C++-Klasse, die diesen Vertrag erfuellt -- Code.
// Die beiden Mengen sind heute VERSCHIEDEN GROSS: vier Vertraege, EIN Dock-Typ. Das ist kein
// Zwischenstand aus Nachlaessigkeit, sondern genau der F8-Minimal-DoD (soll_design K6: "genau 1
// Standard-Dock, Standard-Vertrag, ctest-bewiesen ... vor Router-, Eviction- und XML-Vollausbau").
//
// WARUM DIE VIER TROTZDEM JETZT SCHON HIER STEHEN: die Registry ist das ANGEBOT (Registry-ist-
// ANGEBOT-Doktrin, Par.27). Ein Vertrags-Token, das im Angebot fehlt, kann in keiner XML validiert
// werden -- und die Vertrags-Namen stehen im Design fest (soll_design 3.1 (a) nennt
// "standard"/"rollback"/"scan"/"resource_control" woertlich). Ein Vertrag OHNE Dock-Typ scheitert
// deshalb BENANNT an der Factory (hybrid_status_contract_ohne_dock_typ), statt still den Standard
// zu liefern. Genau dieser Unterschied ist der Grund, dass es den Status ueberhaupt gibt: ein
// rollback-Slot, der heimlich Standard-Semantik traegt, waere ein Messfehler ohne Symptom.
//
// ============================================================================================
// WAS DIESE DATEI BEWUSST NICHT TUT
// ============================================================================================
// Kein Loader, kein dlopen, kein Proxy, kein XML-Parser. Der Proxy haelt AnatomyModuleHandle und
// das Drive-Buendel aus der BUILDER-Lib -- seine Schichten-Zuordnung ist die offene Auflage K2
// (soll_design Abschnitt 7, "Entscheid vor HY-B1"). Solange in dieser Stufe kein Loader vorkommt,
// ist sie K2-neutral baubar; der Loader-Anschluss ist HY-A2 und dort als Naht markiert.
//
// @autoritaet Owner-Entscheid E1 02.08.2026 + Owner-GO Q6 (Ein-Gattung-Hybrid) + KON28-03
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 3.1
// @muster include/cache_engine/measurement/run_methodology_registry.hpp (constexpr-Tabelle,
//         consteval-Vollstaendigkeit, Namens-Anker, FAIL-CLOSED-Lookup)

#include "anatomy/anatomy_base.hpp"
#include "heuristik_adapter_klassifikation.hpp" // ist_abi_sichtbares_genus -- das Gate der Stufe

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE STATUS-CODES DER STUFE -- errno-Stil, 0 == ok
// ------------------------------------------------------------------------------------------
// Bauart uebernommen von pruef_dock.hpp:36-66 (dock_status_*). Uebernommen ist auch die LEHRE,
// die dort im Kommentar steht: ein Status, den die Namensfunktion nicht kennt, faellt in "unknown"
// und ist im Bericht STUMM -- und ein stummer Zustand ist schlimmer als gar keiner, weil er wie
// ein bekannter Fehler aussieht. Deshalb hat jeder Code hier einen Namen, und der Test prueft die
// Totalitaet mitsamt Gegenprobe.

inline constexpr int hybrid_status_ok                      = 0;
inline constexpr int hybrid_status_unbekannter_vertrag     = 1; ///< contract_id zeigt in keine Registry-Zeile
inline constexpr int hybrid_status_contract_ohne_dock_typ  = 2; ///< Vertrag existiert, sein Dock-Typ noch nicht
inline constexpr int hybrid_status_kein_zielfaehiges_genus = 3; ///< Ziel ist ein Klassifikations-Genus (Gate S1)
inline constexpr int hybrid_status_array_voll              = 4; ///< Dock-Array am Kapazitaets-Deckel
inline constexpr int hybrid_status_slot_leer               = 5; ///< adressierter Slot traegt kein Dock
// HY-A3 -- die Parse-Seite. Sie lebt HIER und nicht in hybrid_config_xml.hpp, damit es EINE
// Status-Heimat gibt: zwei Namensfunktionen liefen auseinander, und die zweite waere die stumme.
inline constexpr int hybrid_status_xml_nicht_wohlgeformt = 6; ///< kein DOM / falscher Wurzel-Tag
inline constexpr int hybrid_status_unbekanntes_token     = 7; ///< enabled/storage/genus ausserhalb der Wire-Form
inline constexpr int hybrid_status_max_docks_ungueltig   = 8; ///< 0, nicht-numerisch oder ueber dem Programm-Deckel
inline constexpr int hybrid_status_mehr_docks_als_deckel = 9; ///< Bestueckung sprengt das selbst gesetzte max_docks
// W12 (Owner-Linie, Dock-Deckel 32 MIT XML-Pflichtangabe): wer den Hybrid-Zweig anfordert,
// deklariert seine Dock-Zahl. Eigener Code, NICHT max_docks_ungueltig -- "fehlt" und "falscher
// Wert" sind verschiedene Fehler des Anwenders und schicken ihn an verschiedene Stellen.
inline constexpr int hybrid_status_max_docks_fehlt = 10; ///< enabled="true" ohne max_docks-Angabe
// A2.5-Fix F-5: ein FEHLENDES genus-Attribut fiel bisher in unbekanntes_token -- derselbe Code
// wie ein FALSCH GESCHRIEBENES. Das ist genau die Verwechslung, die W12 fuer max_docks bereits
// aufloest (fehlt != falsch), und sie schickt den Anwender an die falsche Stelle: er sucht nach
// einem Tippfehler, wo eine Zeile fehlt.
inline constexpr int hybrid_status_genus_fehlt = 11; ///< <hybrid_tier> ohne genus-Attribut

/// Die EINZELQUELLE aller Status-Codes. Wer einen anhaengt, traegt ihn HIER ein -- die
/// Namens-Totalitaets-Wache unten faengt sonst den Vergessenen. Handgefuehrte Listen IM TEST sind
/// genau die Bauart, an der die alten Vollstaendigkeits-Wachen gescheitert sind
/// (nachgemessen, heuristik_adapter_klassifikation.hpp:20-30).
inline constexpr std::array<int, 12> kAlleHybridStatus{
    hybrid_status_ok,
    hybrid_status_unbekannter_vertrag,
    hybrid_status_contract_ohne_dock_typ,
    hybrid_status_kein_zielfaehiges_genus,
    hybrid_status_array_voll,
    hybrid_status_slot_leer,
    hybrid_status_xml_nicht_wohlgeformt,
    hybrid_status_unbekanntes_token,
    hybrid_status_max_docks_ungueltig,
    hybrid_status_mehr_docks_als_deckel,
    hybrid_status_max_docks_fehlt,
    hybrid_status_genus_fehlt};

[[nodiscard]] constexpr std::string_view hybrid_status_name(int s) noexcept {
    switch (s) {
        case hybrid_status_ok: return "ok";
        case hybrid_status_unbekannter_vertrag: return "unbekannter_vertrag";
        case hybrid_status_contract_ohne_dock_typ: return "contract_ohne_dock_typ";
        case hybrid_status_kein_zielfaehiges_genus: return "kein_zielfaehiges_genus";
        case hybrid_status_array_voll: return "array_voll";
        case hybrid_status_slot_leer: return "slot_leer";
        case hybrid_status_xml_nicht_wohlgeformt: return "xml_nicht_wohlgeformt";
        case hybrid_status_unbekanntes_token: return "unbekanntes_token";
        case hybrid_status_max_docks_ungueltig: return "max_docks_ungueltig";
        case hybrid_status_mehr_docks_als_deckel: return "mehr_docks_als_deckel";
        case hybrid_status_max_docks_fehlt: return "max_docks_fehlt";
        case hybrid_status_genus_fehlt: return "genus_fehlt";
        default: return "unknown";
    }
}

namespace detail {
/// true gdw. JEDER gelistete Status einen echten Namen hat -- und die Namen paarweise verschieden
/// sind. Der zweite Teil ist nicht kosmetisch: zwei Codes mit demselben Namen sind im Bericht
/// ununterscheidbar, also genauso stumm wie ein namenloser.
[[nodiscard]] consteval bool alle_hybrid_status_benannt() {
    for (std::size_t i = 0; i < kAlleHybridStatus.size(); ++i) {
        if (hybrid_status_name(kAlleHybridStatus[i]) == std::string_view{"unknown"}) return false;
        for (std::size_t j = i + 1; j < kAlleHybridStatus.size(); ++j) {
            if (hybrid_status_name(kAlleHybridStatus[i]) == hybrid_status_name(kAlleHybridStatus[j])) return false;
        }
    }
    return true;
}
} // namespace detail

static_assert(detail::alle_hybrid_status_benannt(),
              "HY-A1-STATUS: ein gelisteter Status hat keinen Namen (oder zwei teilen sich einen). "
              "NAHT-1-Lehre (pruef_dock.hpp:59-63): ein stummer Zustand ist schlimmer als gar "
              "keiner -- er sieht im Bericht aus wie ein bekannter Fehler.");
static_assert(hybrid_status_name(-1) == std::string_view{"unknown"} &&
                  hybrid_status_name(9999) == std::string_view{"unknown"},
              "HY-A1-STATUS: die Namensfunktion muss fuer NICHT-Status 'unknown' liefern -- sonst "
              "ist sie auf immer-benannt degeneriert und die Wache darueber wertlos.");

// ------------------------------------------------------------------------------------------
// (2) DIE VIER VERTRAEGE
// ------------------------------------------------------------------------------------------

/// HybridDockContract -- welchen Vertrag ein Hybrid-Pruef-Dock gegenueber seiner plain Tier-Binary
/// faehrt. Fester Grundtyp (uint8_t), weil der Wert als contract_id ueber die Konfigurations-Grenze
/// geht und dort ein Byte ist.
enum class HybridDockContract : std::uint8_t {
    Standard        = 0, ///< der Vertrag des F8-Minimal-DoD: reines Durchstellen an EIN Ziel
    Rollback        = 1, ///< zweiphasig (soll_design 3.1: Vertrags-Parameter two_phase)
    Scan            = 2, ///< Bereichs-Durchlauf (Vertrags-Parameter range_scan)
    ResourceControl = 3, ///< Ressourcen-Steuerung am Dock
};

/// Single-Source: ein fuenfter Vertrag bricht hier compile-time, statt still vier zu bleiben.
inline constexpr std::size_t kHybridDockContractCount = 4;

/// Eine Zeile des Vertrags-ANGEBOTS.
struct HybridDockContractInfo {
    HybridDockContract contract;
    std::string_view   id;   ///< kanonischer XML-Token ("standard"/"rollback"/"scan"/"resource_control")
    std::string_view   name; ///< exakt der Enum-Name (Doku/Reporting)
    /// Traegt der Vertrag heute schon einen gebauten Dock-TYP? Heute nur der Standard (F8-Minimal).
    /// Das Feld ist die EINZELQUELLE dieser Aussage: die Factory liest sie, statt eine zweite Liste
    /// zu fuehren, und der Test liest dieselbe Zeile.
    bool dock_typ_gebaut;
    bool two_phase;  ///< Vertrags-Parameter (soll_design 3.1 (a): "additiv erweiterbar")
    bool range_scan; ///< dito
};

/// Die EINE Registry der Vertraege -- Index == HybridDockContract-Wert (consteval-gesichert).
inline constexpr std::array<HybridDockContractInfo, kHybridDockContractCount> kHybridDockContractRegistry{{
    {HybridDockContract::Standard, "standard", "Standard", true, false, false},
    {HybridDockContract::Rollback, "rollback", "Rollback", false, true, false},
    {HybridDockContract::Scan, "scan", "Scan", false, false, true},
    {HybridDockContract::ResourceControl, "resource_control", "ResourceControl", false, false, false},
}};

namespace detail {

/// Vollstaendigkeit: Index-Treue, keine leeren Tokens, keine Token-Dubletten.
/// Die Dubletten-Pruefung steht HIER und nicht nur im Test, weil sie sonst erst zur Laufzeit
/// auffiele -- und dann haette der Lookup schon monatelang immer die erste Zeile getroffen.
[[nodiscard]] consteval bool hybrid_dock_contract_registry_ist_vollstaendig() {
    for (std::size_t i = 0; i < kHybridDockContractCount; ++i) {
        if (static_cast<std::size_t>(kHybridDockContractRegistry[i].contract) != i) return false;
        if (kHybridDockContractRegistry[i].id.empty()) return false;
        if (kHybridDockContractRegistry[i].name.empty()) return false;
        for (std::size_t j = i + 1; j < kHybridDockContractCount; ++j) {
            if (kHybridDockContractRegistry[i].id == kHybridDockContractRegistry[j].id) return false;
        }
    }
    return true;
}

} // namespace detail

static_assert(kHybridDockContractRegistry.size() == kHybridDockContractCount,
              "HY-A1-VERTRAEGE: Array-Groesse == kHybridDockContractCount (Anzahl-Anker).");
static_assert(detail::hybrid_dock_contract_registry_ist_vollstaendig(),
              "HY-A1-VERTRAEGE: vier Eintraege, Index == HybridDockContract, id/name nie leer, "
              "Tokens paarweise verschieden. Eine Token-Dublette macht die zweite Zeile still "
              "unerreichbar -- der Lookup traefe immer die erste.");

// Namens-Anker: Drift eines Tokens (Umbenennung/Vertauschung) bricht compile-time. Die Tokens sind
// XML-Wire-Form -- wer sie umbenennt, macht jede Bestands-XML ungueltig und muss das WISSEN.
static_assert(kHybridDockContractRegistry[0].id == std::string_view{"standard"} &&
                  kHybridDockContractRegistry[1].id == std::string_view{"rollback"} &&
                  kHybridDockContractRegistry[2].id == std::string_view{"scan"} &&
                  kHybridDockContractRegistry[3].id == std::string_view{"resource_control"},
              "HY-A1-VERTRAEGE: die Tokens sind {standard,rollback,scan,resource_control} "
              "(soll_design Abschnitt 3.1 (a), woertlich). Sie sind XML-Wire-Form.");

// F8-MINIMAL-ANKER: genau EIN Vertrag traegt heute einen gebauten Dock-Typ. Diese Zahl steigt mit
// jedem neuen Vertrags-Dock -- als BEWUSSTE Quittung, nicht als Reibung.
static_assert(kHybridDockContractRegistry[0].dock_typ_gebaut && !kHybridDockContractRegistry[1].dock_typ_gebaut &&
                  !kHybridDockContractRegistry[2].dock_typ_gebaut && !kHybridDockContractRegistry[3].dock_typ_gebaut,
              "HY-A1-F8: im Minimal-DoD traegt GENAU der Standard-Vertrag einen Dock-Typ. Wer einen "
              "Alternativ-Dock baut, dreht sein Flag hier UND haengt seinen Typ in HybridDockVariant "
              "ein -- beides zusammen, sonst meldet die Factory einen Typ, den der Slot nicht fassen kann.");

// ------------------------------------------------------------------------------------------
// (3) DER TOKEN-LOOKUP -- FAIL-CLOSED, in beiden Formen
// ------------------------------------------------------------------------------------------
// ZWEI FUNKTIONEN, ZWEI AUFGABEN, und keine ersetzt die andere:
//   hybrid_dock_contract_token_bekannt() ist der DETEKTOR. Er liefert bool und ist damit in einem
//     static_assert verwendbar -- das ist die Form, die einen Marker zuverlaessig in die Diagnose
//     bringt (die Falle dazu steht ausfuehrlich in test_hy_a1_reroute_gate_negativ.cpp:42-62).
//   hybrid_dock_contract_of_token() ist die VERWENDUNGSSTELLE. Sie ist consteval und wirft bei
//     einem unbekannten Token -- eine Ausnahme in einem consteval-Aufruf ist keine Laufzeit-
//     Ausnahme, sondern ein harter Uebersetzungsfehler. Nur diese Form beweist, dass eine ECHTE
//     Uebersetzung an einem falschen Token scheitert.
//
// WARUM FAIL-CLOSED UND NICHT "faellt halt auf standard zurueck": das Haus hat die Gegenprobe
// bereits bezahlt. run_methodology_for_ids liess bis zur Welle B/1 einen Tippfehler ("mesure")
// STILL auf measure zurueckfallen und loeste damit eine Messung aus, die niemand angefordert hatte
// (run_methodology_registry.hpp:170-173). Ein stiller Standard-Vertrag waere dieselbe Klasse Fehler:
// er misst etwas, das der Anwender nicht bestellt hat, und sieht dabei gruen aus.

/// true gdw. das Token eine Zeile der Registry benennt. Gross/Kleinschreibung ist NICHT egal --
/// die Tokens sind Wire-Form, keine Anzeigetexte.
[[nodiscard]] constexpr bool hybrid_dock_contract_token_bekannt(std::string_view token) noexcept {
    for (auto const& info : kHybridDockContractRegistry) {
        if (info.id == token) return true;
    }
    return false;
}

/// Token -> Vertrag, fuer COMPILE-TIME-Verwendungsstellen. UNBEKANNTES TOKEN = Uebersetzungsfehler
/// (consteval + throw -- eine Ausnahme im consteval-Kontext ist keine Laufzeit-Ausnahme, sondern
/// ein harter Bruch), NIE ein Default.
[[nodiscard]] consteval HybridDockContract hybrid_dock_contract_of_token(std::string_view token) {
    for (auto const& info : kHybridDockContractRegistry) {
        if (info.id == token) return info.contract;
    }
    throw "HY-A1-VERTRAEGE: unbekanntes Vertrags-Token. Gueltig sind standard/rollback/scan/"
          "resource_control. FAIL-CLOSED: ein stiller standard-Ersatz wuerde einen Dock-Vertrag "
          "fahren, den niemand angefordert hat.";
}

/// Token -> Vertrag, fuer LAUFZEIT-Verwendungsstellen (der XML-Parser liest sein Token zur Laufzeit).
///
/// WARUM ZWEI FORMEN UND NICHT EINE: die consteval-Form oben kann kein Laufzeit-Argument annehmen --
/// sie ist die Verwendungsstelle, an der ein falsches Literal die Uebersetzung reisst. Die Form hier
/// ist die, die ein Parser braucht. Beide sind FAIL-CLOSED, nur auf verschiedene Art: die eine
/// bricht, die andere liefert "kein Wert". Was KEINE von beiden tut, ist auf standard zurueckfallen.
/// std::optional statt eines Ausgangs-Parameters, weil "kein Vertrag" hier ein normaler, erwarteter
/// Zustand ist (jede Validierung eines Anwender-Tokens laeuft hier durch) und kein Fehlerpfad.
[[nodiscard]] constexpr std::optional<HybridDockContract>
hybrid_dock_contract_lookup(std::string_view token) noexcept {
    for (auto const& info : kHybridDockContractRegistry) {
        if (info.id == token) return info.contract;
    }
    return std::nullopt;
}

/// constexpr-Lookup ueber den Enum-Wert (Index == Wert, per consteval garantiert).
[[nodiscard]] constexpr HybridDockContractInfo const& hybrid_dock_contract_info(HybridDockContract c) noexcept {
    return kHybridDockContractRegistry[static_cast<std::size_t>(c)];
}

/// true gdw. die (uint8_t-)Wire-Form eine gueltige Registry-Zeile adressiert.
/// Die Wire-Form kann jeden Byte-Wert tragen -- deshalb ist diese Pruefung kein Zierrat.
[[nodiscard]] constexpr bool hybrid_dock_contract_id_gueltig(std::uint8_t contract_id) noexcept {
    return contract_id < kHybridDockContractCount;
}

// ------------------------------------------------------------------------------------------
// (4) DER DESKRIPTOR -- die POD-Wire-Form ueber die Konfigurations-Grenze
// ------------------------------------------------------------------------------------------

/// DockContractDescriptor -- was die Konfiguration (spaeter: die XML) je Dock-Slot liefert.
/// POD und trivially_copyable, weil er spaeter durch ein Sidecar-Manifest geht (soll_design 3.1 (a)
/// + 6.4: die Dock-Bestueckung ist RUNTIME-Konfiguration und gehoert NIE in die binary_id).
struct DockContractDescriptor {
    std::uint8_t          contract_id; ///< Index in kHybridDockContractRegistry
    anatomy::AnatomyGenus genus;       ///< Genus der andockbaren plain Tier-Binaries
};

static_assert(std::is_trivially_copyable_v<DockContractDescriptor>,
              "HY-A1-DESKRIPTOR: er geht als POD ueber die Konfigurations-Grenze -- kein "
              "nicht-triviales Member.");
static_assert(std::is_standard_layout_v<DockContractDescriptor>);

/// Pruefung eines Deskriptors gegen Registry UND Gate. Liefert hybrid_status_ok oder den GRUND.
/// Die Reihenfolge ist bindend: erst Registry (existiert der Vertrag ueberhaupt?), dann Gate
/// (ist das Ziel zielfaehig?), dann Dock-Typ (ist er gebaut?). Anders herum bekaeme ein
/// unbekannter Vertrag mit gutem Ziel die Meldung "Dock-Typ fehlt" -- richtig rot, falsche Ursache.
[[nodiscard]] constexpr int pruefe_dock_deskriptor(DockContractDescriptor const& desc) noexcept {
    if (!hybrid_dock_contract_id_gueltig(desc.contract_id)) return hybrid_status_unbekannter_vertrag;
    // Das Gate der Stufe, hier an der Dock-Kante durchgesetzt: hinter einem Klassifikations-Genus
    // steht keine plain Tier-Binary, sondern wieder nur eine Klassifikation
    // (heuristik_adapter_gate.hpp, Schranke S1 -- "allein nicht ansprechbar").
    if (!ist_abi_sichtbares_genus(desc.genus)) return hybrid_status_kein_zielfaehiges_genus;
    if (!kHybridDockContractRegistry[desc.contract_id].dock_typ_gebaut) return hybrid_status_contract_ohne_dock_typ;
    return hybrid_status_ok;
}

} // namespace comdare::cache_engine::hybrid
