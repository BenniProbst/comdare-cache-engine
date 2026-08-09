#pragma once
// checkpoint_speicher.hpp -- MeasureStorage (2026-08-09): DIE ZWEI INTERNEN SYSTEME von
// checkpoint_measure, zusammengefuehrt -- und der COMPILE-HARTE BELEG, dass sie getrennt bleiben.
//
// SELBSTCHECK: diese Datei enthaelt KEINE dritte Datenstruktur. Sie haelt genau die beiden Arenen
// und stellt sicher, dass ihre heissen Zustaende nicht in derselben Cacheline landen. Wer hier eine
// gemeinsame Oberklasse, einen geteilten Puffer oder einen gemeinsamen Zaehler einbaut, hebt genau
// die Trennung auf, um derentwillen es diese Datei gibt.
//
// == DER OWNER-KERN (09.08.2026, zweimal ausdruecklich freigegeben) =================================
//     "checkpoint_measure hat also 2 interne Systeme -- Mess-Arena und Stack-Arena, custom."
//
// == WARUM ZWEI UND NICHT EINE MIT ZWEI ABSCHNITTEN -- vier Konsequenzen ============================
// 1. DIMENSIONIERUNG grundverschieden: Millionen Messpunkte gegen eine typisch einstellige
//    Verschachtelungstiefe. Ein gemeinsamer Bereich hiesse, die kleine Struktur an der grossen zu
//    bemessen -- oder umgekehrt Ueberlauf zu riskieren.
// 2. LEBENSDAUER verschieden: die Mess-Arena wird NIE zurueckgesetzt, der Stapel STAENDIG.
// 3. UEBERLAUF ist ein FEHLERKLASSEN-Unterschied: volle Mess-Arena = Datenverlust; voller Stapel =
//    zu tiefe Verschachtelung, also ein Programmierfehler. Zwei Diagnosen, die sich keine Meldung
//    teilen duerfen -- deshalb zwei Befund-Typen und zwei getrennte Meldetexte, nie ein Sammeltext.
// 4. GETRENNTE CACHELINES: der Stapel wird bei JEDEM Checkpoint gelesen UND geschrieben, die
//    Mess-Arena nur geschrieben. Beide in einer Cacheline waere selbstgemachtes False Sharing --
//    ausgerechnet in dem Modul, das solche Effekte messen soll.
//
// == WIE PUNKT 4 HIER BEWIESEN WIRD, statt behauptet zu werden =====================================
// Auf DREI Ebenen, die unabhaengig voneinander brechen:
//   (a) COMPILE-HART, hier unten: die beiden Arenen-Objekte sind kCacheLineBytes-ausgerichtet, ihre
//       Groessen sind Vielfache davon, und der Abstand der beiden Mitglieder ist mindestens eine
//       ganze Cacheline. Ein Zusammenrutschen faellt beim Uebersetzen auf, nicht im Betrieb.
//   (b) COMPILE-HART, in den Arenen: der heisse Zustand ist JEWEILS das erste Mitglied und selbst
//       cacheline-ausgerichtet -- er beginnt damit auf Offset 0 der Arena.
//   (c) ZUR LAUFZEIT, in test_ms1_arenen_kein_alloc_im_fenster: die tatsaechlichen Adressen werden
//       durch kCacheLineBytes geteilt und die Zeilennummern verglichen. Eine Zusicherung, die man
//       nur uebersetzt, ist eine Zusicherung ueber den Uebersetzer -- die Messung ueber die Maschine.

#include "mess_arena.hpp"
#include "mess_speicher_kanon.hpp"
#include "stapel_arena.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::builder::measure_storage {

/// CheckpointSpeicher -- die beiden internen Systeme, nebeneinander und getrennt.
struct CheckpointSpeicher {
    MessArena   mess{};   // nur ANHAENGEN, monoton, Auswertung zum Schluss
    StapelArena stapel{}; // LIFO, auf und ab, Auswertung waehrend des Laufs
};

static_assert(std::is_standard_layout_v<CheckpointSpeicher>,
              "CheckpointSpeicher muss standard_layout sein -- sonst ist der offsetof-Beleg unten nicht definiert");
static_assert(offsetof(CheckpointSpeicher, mess) % kCacheLineBytes == 0,
              "die Mess-Arena muss auf einer Cacheline-Grenze beginnen");
static_assert(offsetof(CheckpointSpeicher, stapel) % kCacheLineBytes == 0,
              "die Stapel-Arena muss auf einer Cacheline-Grenze beginnen");
static_assert(offsetof(CheckpointSpeicher, stapel) - offsetof(CheckpointSpeicher, mess) >= kCacheLineBytes,
              "die heissen Zustaende beider Arenen duerfen nie in derselben Cacheline liegen (False Sharing)");

/// Baut beide Arenen auf. AUFBAU, vor dem Messfenster -- hier ist Belegung erlaubt.
///
/// Zwei getrennte Reservierungen, mit Absicht: getrennte Seiten sind die staerkste Form der
/// Trennung, und sie kostet nichts, weil beide Bloecke ohnehin einzeln angefordert werden.
/// Rueckgabe false = eine der beiden nicht bekommen. Der Aufrufer meldet; abgebrochen wird nicht.
[[nodiscard]] inline bool speicher_aufbauen(CheckpointSpeicher& sp, std::uint64_t kapazitaet_zeilen,
                                            std::uint32_t tiefe_plaetze, VorabBeruehrung vorab) noexcept {
    bool const a = sp.mess.reservieren(kapazitaet_zeilen, vorab);
    bool const b = sp.stapel.reservieren(tiefe_plaetze, vorab);
    return a && b;
}

/// Die Planer-Formel fuer die Kapazitaet der MESS-Arena, an einer Stelle.
///
/// zeilen = n_ops * zeilen_je_op * sicherheitsfaktor. Die Zahl der Zeilen je Op ist 2 (das
/// Makro-Paar) plus 2 je aktiver Achse (die Mikro-Paare); sie kommt vom Planer aus dem XML und wird
/// hier NICHT erraten. Der Sicherheitsfaktor deckt die Vervielfachung durch das Drift-Gate ab:
/// run_cell_with_drift_gate (harness/drift_gated_cell.hpp) misst dieselbe Zelle bis zu
/// reps * (max_reruns + 1) mal -- mit den Vorgabewerten 3 und 5 sind das bis zu 18 Durchlaeufe je
/// Zelle, und JEDER schreibt in dieselbe monoton wachsende Arena.
///
/// SELBSTCHECK: das ist eine Rechnung, keine Politik. Woher n_ops und die Achszahl kommen, entscheidet
/// der Planer; ob der Faktor genuegt, meldet der Ueberlauf-Befund. Beides gehoert nicht hierher.
[[nodiscard]] constexpr std::uint64_t kapazitaet_zeilen_rechnen(std::uint64_t n_ops, std::uint64_t zeilen_je_op,
                                                                std::uint64_t sicherheitsfaktor) noexcept {
    return n_ops * zeilen_je_op * sicherheitsfaktor;
}

} // namespace comdare::cache_engine::builder::measure_storage
