#pragma once
// builder/pruef_dock/genus_mess_naht.hpp -- DIE CEB-SEITE DER MESS-NAHT (NAHT-1, Owner-KERN 09.08.2026).
//
// ------------------------------------------------------------------------------------------------
// DIE COMPILE-HARTE WACHE
// ------------------------------------------------------------------------------------------------
// Diese Datei traegt Mess-MASCHINERIE der CacheEngineBuilder. Sie darf in einer TU, die OHNE
// COMDARE_MEASUREMENT_ON uebersetzt wird, NICHT existieren -- sonst waere die CEB-Haelfte der
// UND-Bedingung eine Absichtserklaerung statt einer Struktur. Der Guard ist wertbasiert, exakt wie
// das Bestands-Mess-Gating in anatomy/abi_adapter.hpp:9 (-DCOMDARE_MEASUREMENT_ON=0 muss ausloesen).
//
// WARUM DER GUARD HIER STEHT UND NICHT IN anatomy/mess_visitor_abi.hpp: jene Datei traegt nur
// DEKLARATIONEN (abstrakte Klasse, Concept, Template) und wird von observable_tier.hpp gezogen, das
// die real erreichbare Release-Neutralitaets-TU (tests/unit/test_a8s4_release_pfad_neutralitaet.cpp)
// im OFF-Zustand ECHT uebersetzt. Ein #error dort braeche genau die Wache, die den OFF-Pfad belegt.
#if !defined(COMDARE_MEASUREMENT_ON) || !COMDARE_MEASUREMENT_ON
#error                                                                                                                 \
    "NAHT-1: genus_mess_naht.hpp in einer TU ohne COMDARE_MEASUREMENT_ON -- "
    "die CEB-Haelfte der Mess-Naht existiert nur im Mess-Kompilat."
#endif

#include <anatomy/observable_tier.hpp> // IObservableTier + IMessVisitor + MessEdge + SnapshotSink

#include <cstdint>

namespace comdare::cache_engine::builder::pruef_dock {

namespace anatomy = ::comdare::cache_engine::anatomy;

// ------------------------------------------------------------------------------------------------
// DIE ZWEISEITIGE AKTIVIERUNG -- ALS STRUKTUR, NICHT ALS ABFRAGE
// ------------------------------------------------------------------------------------------------
// Der KERN verlangt: "je aktiviert/deaktiviert der Messeinrichtungen in CEB UND Tier-Binary AM
// INTERFACE EIN MESS-VISITOR UEBERGEBEN WERDEN oder bei deaktiviert eben nicht."
//
// Das sind ZWEI Seiten, und keine von beiden ist eine Laufzeit-Option:
//   CEB-Seite  : dieser Header existiert nur unter COMDARE_MEASUREMENT_ON (#error oben). Eine
//                OFF-gebaute CEB kann den Visitor nicht einmal BENENNEN.
//   Tier-Seite : der Adapter erbt IObservableTier nur unter COMDARE_MEASUREMENT_ON
//                (abi_adapter.hpp). In einer OFF-Binary steht tier_measure_accept nicht in der
//                Symboltabelle -- am Objekt nachweisbar, nicht bloss zugesagt.
//
// Der einzige LAUFZEIT-Anteil ist die Probe, ob das GELADENE Modul die Flaeche mitbringt. Sie ist
// kalt (1x je Modul, nie im Hot-Loop) und darf deshalb verzweigen.

/// Das Ergebnis der zweiseitigen Gate-Pruefung -- BEVOR gemessen wird.
enum class MessGateBefund : std::uint8_t {
    Beidseitig_an       = 0, ///< CEB an (sonst waere diese Datei nicht uebersetzt) UND Tier-Flaeche da
    Tier_aus            = 1, ///< Flaeche fehlt UND die Legende sagt das auch -> ehrlich deaktiviert
    Widerspruch         = 2, ///< Legende und Flaeche widersprechen sich -> DEFEKT, kein Degrade
    Flaeche_fehlt_stumm = 3, ///< Flaeche fehlt, KEINE Legende bekannt -> Alt-DLL-Fall, unveraendert Status 3
};

/// mess_gate_befund -- die WEICHE VOR der Messung (nicht die Forensik DANACH).
///
/// R-3 hat den Erkennungsapparat fuer den Widerspruch bereits gebaut (das neunte Preimage-Glied
/// "mess-gates", abi/mess_gates_glied.hpp) -- aber als FORENSIK, also als etwas, das man hinterher
/// nachrechnen kann. Hier wird daraus eine Weiche: ein Modul, dessen Legende Messung BEHAUPTET und
/// dessen Mess-Flaeche FEHLT, wird nicht mehr "sauber degradiert", sondern als Defekt gemeldet.
///
/// GENAU DAS IST DIE TEUERSTE FEHLERKLASSE DIESES PROJEKTS: das stille Degrade lieferte einen
/// Status, der wie "alte DLL" aussah, und eine Mess-Reihe voller ehrlicher Nullen. Ein richtiges
/// Messgeraet am falschen Gegenstand faellt nie auf -- es gibt nichts, was klappern koennte. Diese
/// Funktion IST das Klappern.
///
/// legende_sagt_an: was die Bau-Seite ueber das Modul BEHAUPTET (nullptr-Aequivalent: unbekannt,
/// dann wird nichts behauptet und der Widerspruch kann nicht festgestellt werden -- eine ehrliche
/// Wissensluecke ist kein Defekt).
[[nodiscard]] inline MessGateBefund mess_gate_befund(bool flaeche_vorhanden, bool legende_bekannt,
                                                     bool legende_sagt_an) noexcept {
    if (flaeche_vorhanden) {
        // Flaeche da, Legende sagt AUS -> ebenfalls ein Widerspruch, und zwar der gefaehrlichere:
        // es WIRD gemessen, obwohl die Provenienz "nicht gemessen" behauptet. Eine Mess-Reihe mit
        // falscher Provenienz ist schlimmer als eine fehlende, weil sie in die Auswertung geht.
        if (legende_bekannt && !legende_sagt_an) return MessGateBefund::Widerspruch;
        return MessGateBefund::Beidseitig_an;
    }
    if (!legende_bekannt) return MessGateBefund::Flaeche_fehlt_stumm;
    return legende_sagt_an ? MessGateBefund::Widerspruch : MessGateBefund::Tier_aus;
}

/// genus_measure_into -- DIE UEBERGABE. Hier, und nur hier, quert der Mess-Visitor das
/// Genus-Interface.
///
/// Die Senke ist ein gewoehnlicher Typ (GenusMessSink-Concept, keine vtable, kein Erben); die
/// Typloeschung passiert an genau EINEM Punkt, dem MessEdge, und der lebt nur fuer die Dauer des
/// Aufrufs auf dem Stack. Kein std::function, keine Allokation, kein Laufzeit-Switch -- der
/// Concept-Guard erledigt die Auswahl zur Uebersetzungszeit.
template <anatomy::GenusMessSink Sink>
inline void genus_measure_into(anatomy::IObservableTier const& tier, Sink& sink) noexcept {
    anatomy::MessEdge<Sink> edge{sink};
    tier.tier_measure_accept(edge);
}

} // namespace comdare::cache_engine::builder::pruef_dock
