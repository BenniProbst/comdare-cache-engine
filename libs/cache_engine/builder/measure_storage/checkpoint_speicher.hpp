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
//   (a) COMPILE-HART, hier unten: die beiden Arenen-Objekte sind kCacheLineBytes-ausgerichtet, und
//       der Abstand der beiden Mitglieder ist mindestens eine ganze Cacheline. Ein Zusammenrutschen
//       faellt beim Uebersetzen auf, nicht im Betrieb.
//   (b) COMPILE-HART, in den Arenen: der heisse Zustand ist GENAU eine Cacheline gross
//       (static_assert dort) und das ERSTE Mitglied einer standard-layout-Klasse -- er beginnt damit
//       auf Offset 0 der Arena. Das ist keine Beobachtung ueber diesen Uebersetzer, sondern eine
//       Zusage der Norm; gemessen wird sie trotzdem, siehe (c).
//   (c) ZUR LAUFZEIT, in test_ms1_arenen_kein_alloc_im_fenster: die tatsaechlichen Adressen werden
//       durch kCacheLineBytes geteilt und die Zeilennummern verglichen; zusaetzlich wird die
//       Offset-0-Zusage aus (b) direkt nachgemessen (heisse_basis() gegen die Objektadresse). Eine
//       Zusicherung, die man nur uebersetzt, ist eine Zusicherung ueber den Uebersetzer -- die
//       Messung ist eine ueber die Maschine.

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
// MELDUNG PRAEZISIERT (09.08.2026). Gemessen wird der Abstand der beiden ARENEN-OBJEKTE, nicht der
// ihrer heissen Zustaende -- die Meldung sagte vorher das Zweite und mass das Erste. Vom Gemessenen
// zum Gemeinten fuehrt eine Kette, die vollstaendig aus zugesicherten Gliedern besteht: jeder heisse
// Zustand ist das ERSTE Mitglied seiner standard-layout-Arena, liegt also auf deren Offset 0 (zur
// Laufzeit nachgemessen in test_ms1, Abschnitt (8)), und er ist genau EINE Cacheline gross
// (static_assert in mess_arena.hpp bzw. stapel_arena.hpp). Der Abstand der Objekte ist damit der
// Abstand der heissen Zustaende, und die Zeile unten sagt jetzt, was sie prueft.
static_assert(offsetof(CheckpointSpeicher, stapel) - offsetof(CheckpointSpeicher, mess) >= kCacheLineBytes,
              "die beiden Arenen-Objekte muessen mindestens eine ganze Cacheline auseinander liegen");

/// Was der Aufbau geleistet hat -- JE ARENA einzeln.
///
/// BEFUND, der diesen Typ erzwingt (09.08.2026): speicher_aufbauen() gab ein nacktes `a && b`
/// zurueck. Damit war bei einem Fehlschlag nicht mehr feststellbar, WELCHE der beiden Reservierungen
/// gefehlt hat -- und die beiden haben voellig verschiedene Ursachen (eine Kapazitaet in Millionen
/// Zeilen gegen eine Verschachtelungstiefe von typisch acht). Der Aufrufer soll melden, und melden
/// kann er nur, was er weiss. Es ist dasselbe Muster wie bei AusleseErgebnis und MeldungsLaenge:
/// wer die Antwort nimmt, hat ihre Aufschluesselung in derselben Hand.
struct AufbauBefund {
    bool mess   = false; // die MESS-Arena steht
    bool stapel = false; // die STAPEL-Arena steht

    [[nodiscard]] constexpr bool steht() const noexcept { return mess && stapel; }
};

/// Baut beide Arenen auf. AUFBAU, vor dem Messfenster -- hier ist Belegung erlaubt.
///
/// Zwei getrennte Reservierungen, mit Absicht: getrennte Seiten sind die staerkste Form der
/// Trennung, und sie kostet nichts, weil beide Bloecke ohnehin einzeln angefordert werden.
/// Es wird IMMER beides versucht, auch wenn das erste fehlschlaegt -- sonst waere der Befund ueber
/// die zweite Arena nur "nicht geprueft" und saehe aus wie "nicht bekommen".
/// Der Aufrufer meldet; abgebrochen wird nicht.
[[nodiscard]] inline AufbauBefund speicher_aufbauen(CheckpointSpeicher& sp, std::uint64_t kapazitaet_zeilen,
                                                    std::uint32_t tiefe_plaetze, VorabBeruehrung vorab) noexcept {
    AufbauBefund b{};
    b.mess   = sp.mess.reservieren(kapazitaet_zeilen, vorab);
    b.stapel = sp.stapel.reservieren(tiefe_plaetze, vorab);
    return b;
}

/// Das Ergebnis der Planer-Formel -- die Zahl UND die Auskunft, ob sie ueberhaupt eine ist.
///
/// BEFUND, der auch diesen Typ erzwingt: die Formel rechnete in nacktem uint64 und wickelte still.
/// Gemessen hat `kapazitaet_zeilen_rechnen(2^63 + 1, 2, 1)` den Wert 2 geliefert -- und 2 ist eine
/// vollkommen plausible Kapazitaet. Aus einer masslosen Anforderung wurde damit eine winzige Arena,
/// die sofort ueberlaeuft; der Datenverlust-Befund haette den Ueberlauf gemeldet und die falsche
/// Ursache genannt. Ein Ueberlauf, der wie ein gueltiges Ergebnis aussieht, ist genau der Fall, den
/// dieses Modul nirgends durchgehen laesst.
struct KapazitaetRechnung {
    std::uint64_t zeilen      = 0;     // nur gueltig, wenn darstellbar
    bool          darstellbar = false; // false = das Produkt passt nicht in 64 Bit
};

/// Die Planer-Formel fuer die Kapazitaet der MESS-Arena, an einer Stelle.
///
/// zeilen = n_ops * zeilen_je_op * sicherheitsfaktor. Die Zahl der Zeilen je Op ist 2 (das
/// Makro-Paar) plus 2 je aktiver Achse (die Mikro-Paare); sie kommt vom Planer aus dem XML und wird
/// hier NICHT erraten. Der Sicherheitsfaktor deckt die Vervielfachung durch das Drift-Gate ab:
/// run_cell_with_drift_gate (harness/drift_gated_cell.hpp) misst dieselbe Zelle bis zu
/// reps * (max_reruns + 1) mal -- mit den Vorgabewerten 3 und 5 sind das bis zu 18 Durchlaeufe je
/// Zelle, und JEDER schreibt in dieselbe monoton wachsende Arena.
///
/// RUECKRECHNUNGS-WAECHTER, derselbe Griff wie in MessArena::reservieren: nach jeder Multiplikation
/// wird durch den einen Faktor zurueckgeteilt und gegen den anderen geprueft. Vorzeichenlose
/// Arithmetik wickelt definiert (auch in der Konstantfaltung), die Rueckrechnung ist deshalb exakt
/// und nicht bloss wahrscheinlich richtig.
///
/// SELBSTCHECK: das ist eine Rechnung, keine Politik. Woher n_ops und die Achszahl kommen, entscheidet
/// der Planer; ob der Faktor genuegt, meldet der Ueberlauf-Befund. Beides gehoert nicht hierher.
[[nodiscard]] constexpr KapazitaetRechnung kapazitaet_zeilen_rechnen(std::uint64_t n_ops, std::uint64_t zeilen_je_op,
                                                                     std::uint64_t sicherheitsfaktor) noexcept {
    std::uint64_t const erst = n_ops * zeilen_je_op;
    if (n_ops != 0u && erst / n_ops != zeilen_je_op) return KapazitaetRechnung{};
    std::uint64_t const ganz = erst * sicherheitsfaktor;
    if (erst != 0u && ganz / erst != sicherheitsfaktor) return KapazitaetRechnung{};
    return KapazitaetRechnung{ganz, true};
}

} // namespace comdare::cache_engine::builder::measure_storage
