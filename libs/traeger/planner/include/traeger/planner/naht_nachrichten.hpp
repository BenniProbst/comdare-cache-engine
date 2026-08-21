// naht_nachrichten.hpp -- B-01 NAHT-SCHABLONE der STEUER-Naht Planer<->CEB (S-8-Kopf, T-NEU-6).
// =============================================================================================
// TRAEGER-STUFE 1 (PLANNER), Include-Wurzel <traeger/planner/...> -- Neubau nach DESIGN #29
// Par. 4.3 (super 85c1174d): KEINE Alt-Wurzeln, KEINE builder/-Includes (darum kein
// TRAEGER-BRUECKE-Marker noetig: dieser Header zieht nur std).
//
// DER VERTRAG (Ledger KON50/51/52, Sweep-B-Ergebnis Z.496):
//   Die Planer<->CEB-Naht ist die STEUER-Naht. Sie traegt Orchestrierung + Latenz-Reinheit,
//   NIE Mess-Rohdaten (KON50-01). Hinauf reisen GENAU DREI Nachrichtenklassen
//   (KON50-02 PRAEZISIERT):
//     (1) Fortschritts-DELTAS            -> FortschrittsDelta
//     (2) STATUS je Anforderung + Fehler-LOG (WARUM gescheitert) -> StatusFehlerLog
//     (3) Ergebnis-TRACE fuer die CLI (Nutzer-Sicht)             -> ErgebnisTrace
//   Hinab reisen GEFILTERTE XML-Fragmente (KON51-01)             -> GefiltertesXmlFragment
//   Ausser der Reihe (out of band) existiert GENAU EINE Nachricht: das Fertig-Signal
//   (KON52-01)                                                   -> FertigSignal
//   Waehrend der Mess-Phase gilt STUMMSCHALTUNG + Prioritaets-Queues (KON51-03): gesendet
//   wird nur an den Fenstern, nie in die laufende Messung hinein.
//
// WAS DIESE DATEI IST -- und was nicht: die SCHABLONE (Typen + Concepts), compile-hart.
// Die IPC-Technik und die Trace-Kanalform sind S-9/S-10-Design (Memory-Referenz
// "KEINE YAML -- Planer emittiert PROZESS": die Pipe ist das Flaeche-1-CONTROL-INTERFACE
// der CEB). Hier entsteht die geschlossene Klassen-Menge, gegen die S-9/S-10 verdrahten;
// die Negativprobe (Mess-Rohdaten fallen durch) fahren die Contract-Tests
// (tests/unit/test_s8kopf_planner_kopf.cpp).
//
// GESCHLOSSENHEIT ALS RIEGEL: die Concepts unten sind WHITELISTS, keine Struktur-Muster.
// Ein vierter "hinauf"-Typ -- und sei er strukturell perfekt -- faellt durch, bis er HIER
// eingetragen ist. Das ist Absicht: die Drei-Klassen-Zahl ist Owner-Wort (KON50-02), ihre
// Erweiterung ist ein Vertrags-, kein Bauvorgang.
//
// ASCII-only, Zeilen <= 120.

#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>

namespace comdare::traeger::planner {

// -- Richtungs-Tags der Naht (Flaeche 1: stream in / stream out; KON110-01: Flaeche 3 je Naht zielverschieden). --
struct NahtHinauf {}; // CEB -> Planer (Status/Fortschritt/Trace)
struct NahtHinab {};  // Planer -> CEB (erkannte Anforderungen als gefiltertes XML)

// KON51-03: zwei Prioritaets-Queues -- Status/Fehler reist vor Fortschritt, nichts reist waehrend der Messung.
enum class NahtPrioritaet : unsigned char {
    Normal = 0,
    Hoch   = 1,
};

// KON51-03 STUMMSCHALTUNG: der Zustand "Mess-Phase laeuft, die Naht schweigt". Der Typ ist bewusst ein
// eigener Zustands-Traeger und kein bool-Parameter: wer die Naht bedient, muss den Zustand BENENNEN.
struct Stummschaltung {
    bool aktiv = false; // true = Mess-Phase: kein Sendevorgang (Latenz-Reinheit, KON51-03)
};

// =====================================================================================
// DIE DREI NACHRICHTENKLASSEN HINAUF (KON50-02 PRAEZISIERT) -- geschlossene Menge.
// Jede Klasse deklariert ihre Richtung, ihre Queue-Prioritaet und -- als compile-harter
// KON50-01-Anker -- dass sie KEINE Mess-Rohdaten traegt.
// =====================================================================================

// (1) Fortschritts-DELTAS: Zaehlwerks-Deltas, nie Absolutstaende rueckgerechnet aus Messwerten.
struct FortschrittsDelta {
    using naht_richtung                                  = NahtHinauf;
    static constexpr bool           traegt_mess_rohdaten = false; // KON50-01: NIE Mess-Rohdaten zum Planer
    static constexpr bool           ausser_der_reihe     = false; // reist in der Queue, nicht OOB
    static constexpr NahtPrioritaet prioritaet           = NahtPrioritaet::Normal;

    std::uint64_t schritte_delta  = 0; // NEU fertige Schritte seit der letzten Nachricht (Delta, kein Stand)
    std::uint64_t schritte_gesamt = 0; // der Plan-Nenner (aus dem Director-Walk, nicht aus Messung)
    std::string   gegenstand;          // welcher Plan-Schritt (perm/step-Bezeichner der Plan-Sprache)
};

// (2) STATUS je Anforderung + Fehler-LOG: erfolgreich/nicht + log-Message WARUM (Flaeche-1 stream out).
struct StatusFehlerLog {
    using naht_richtung                                  = NahtHinauf;
    static constexpr bool           traegt_mess_rohdaten = false;
    static constexpr bool           ausser_der_reihe     = false;
    static constexpr NahtPrioritaet prioritaet           = NahtPrioritaet::Hoch; // Fehler reisen vor Fortschritt

    bool        erfolgreich = false;
    std::string anforderung; // WELCHE Anforderung (hinab gereichtes XML-Fragment-Kennzeichen)
    std::string log_warum;   // WARUM gescheitert -- die log-Message der Flaeche 1
};

// (3) Ergebnis-TRACE: die Nutzer-Sicht fuer die Planer-CLI. Trace-ZEILEN, nie Messwerte --
//     "an den Planer geht NUR der Ergebnis-Trace fuer die Kommandozeile" (KON50, Memory-Referenz).
struct ErgebnisTrace {
    using naht_richtung                                  = NahtHinauf;
    static constexpr bool           traegt_mess_rohdaten = false;
    static constexpr bool           ausser_der_reihe     = false;
    static constexpr NahtPrioritaet prioritaet           = NahtPrioritaet::Normal;

    std::string trace_zeile; // eine CLI-Zeile Nutzer-Sicht (Format = S-10-Design, hier nur der Traeger)
};

// -- HINAB (KON51-01): die Steuer-Naht spricht GEFILTERTES XML. Kein Voll-Profil, kein Rohtext. --
struct GefiltertesXmlFragment {
    using naht_richtung                        = NahtHinab;
    static constexpr bool traegt_mess_rohdaten = false;
    static constexpr bool ausser_der_reihe     = false;

    std::string fragment;    // das gefilterte XML-Fragment (die erkannte Anforderung)
    std::string anforderung; // Kennzeichen, unter dem StatusFehlerLog::anforderung antwortet
};

// -- GENAU EINE OOB-NACHRICHT (KON52-01): das Fertig-Signal. Nichts anderes reist ausser der Reihe. --
struct FertigSignal {
    using naht_richtung                        = NahtHinauf;
    static constexpr bool traegt_mess_rohdaten = false;
    static constexpr bool ausser_der_reihe     = true; // OOB -- und zwar als EINZIGER Typ (Concept unten)
};

// =====================================================================================
// DIE SCHABLONE ALS COMPILE-HARTE WHITELIST (Concepts).
// =====================================================================================

// Die geschlossene Drei-Klassen-Menge hinauf -- als Typliste exportiert, damit Registry-/Queue-Bauten
// (S-9/S-10) DARUEBER iterieren statt eigene Listen zu pflegen. Der Contract-Test zaehlt gegen den
// FREMDEN Nenner (KON50-02-Wortlaut "drei Nachrichtenklassen"), nicht gegen diese Liste selbst.
using SteuerNahtHinaufKlassen = std::tuple<FortschrittsDelta, StatusFehlerLog, ErgebnisTrace>;

namespace detail {
template <class T, class Tuple>
struct ist_in_tupel;
template <class T, class... Ks>
struct ist_in_tupel<T, std::tuple<Ks...>> : std::bool_constant<(std::same_as<T, Ks> || ...)> {};
} // namespace detail

// WHITELIST-Kern: T ist eine der drei Klassen. Struktur-Gleichheit genuegt NICHT (Geschlossenheit).
template <class T>
inline constexpr bool ist_steuer_naht_hinauf_klasse = detail::ist_in_tupel<T, SteuerNahtHinaufKlassen>::value;

// DER VERTRAGS-CONCEPT HINAUF: Whitelist UND die drei Marker-Anker. Die Marker-Bedingungen sind bewusst
// redundant zur Whitelist -- sie machen den KON50-01-Bruch (traegt_mess_rohdaten == true) als EIGENE
// Verletzung sichtbar, statt ihn in "nicht gelistet" zu verstecken.
template <class T>
concept SteuerNahtHinauf = ist_steuer_naht_hinauf_klasse<T>                       // geschlossene Menge (KON50-02)
                           && std::same_as<typename T::naht_richtung, NahtHinauf> // Richtung erklaert
                           && (!T::traegt_mess_rohdaten)                          // KON50-01: NIE Mess-Rohdaten
                           && (!T::ausser_der_reihe); // OOB ist dem FertigSignal vorbehalten

// DER HINAB-CONCEPT: genau das gefilterte XML-Fragment (KON51-01).
template <class T>
concept SteuerNahtHinab = std::same_as<T, GefiltertesXmlFragment> &&
                          std::same_as<typename T::naht_richtung, NahtHinab> && (!T::traegt_mess_rohdaten);

// DER OOB-CONCEPT (KON52-01): GENAU EINE Nachricht ausser der Reihe -- das Fertig-Signal. Auch hier
// Whitelist: ein zweiter OOB-Typ faellt durch, selbst wenn er ausser_der_reihe = true traegt.
template <class T>
concept SteuerNahtAusserDerReihe = std::same_as<T, FertigSignal> && T::ausser_der_reihe && (!T::traegt_mess_rohdaten);

} // namespace comdare::traeger::planner
