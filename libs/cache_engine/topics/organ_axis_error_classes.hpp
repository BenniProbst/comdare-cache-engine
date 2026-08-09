#pragma once
// topics/organ_axis_error_classes.hpp -- FK-5 (A15 EBENE 4, Algorithmen-Ebene): der FEHLERRAUM der
// Organ-Haupt-Achsen, deklariert an genau der Stelle, die schon den VERSIONSRAUM erzwingt.
//
// OWNER-DIREKTIVE (17.07.2026, axis_error.hpp:2-14 verbatim): "Fehlerklassen und Behandlung sind fuer
// ALLE Achsen -> Unterachsen -> Algorithmen Pflicht." FK-0..FK-2 sind vollzogen, FK-3/FK-4 stehen in
// Phase 3 aus, FK-5 (diese Ebene) war die im M3-Fenster VERFALLENE Zusage und hat mit E-24 C9 ihren Slot.
//
// WARUM DIESER HEADER IM topics/-DACH LIEGT UND NICHT IN measurement/ (Layering, am Ist erhoben):
// KEINE einzige Datei unter axes/ oder topics/ inkludiert heute cache_engine/measurement/ (eigener grep
// 04.08.2026: axes/ zieht aus cache_engine/ ausschliesslich allocators/portable_aligned_alloc.hpp (24x)
// und concepts/cache_recommendation.hpp (1x); topics/ zieht NICHTS aus measurement/). Die 18 CRTP-Basen
// in den Fehlerraum zu heben haette diese Kante in ~110 Varianten-TUs neu gezogen -- eine
// Baseline-Layering-Aenderung im letzten ABI-Fenster vor dem Trigger, und zwar als Nebenwirkung einer
// Fehlerklassen-Deklaration. Sie ist technisch moeglich (empirisch geprobt: der Include loest auf und
// uebersetzt), aber sie ist NICHT gefragt worden. Dieser Header bleibt deshalb im Dach, das die Basen
// ohnehin schon ziehen (topics/organ_axis.hpp).
//
// WIE DIE EINE WAHRHEITSQUELLE TROTZDEM EINE BLEIBT (keine Parallelstruktur): dieser Header definiert
// KEIN eigenes Enum. Er fuehrt die STABILEN D2-ETIKETTEN als string_view -- exakt die Zeichenfolgen, die
// measurement::sample_status_label(SampleStatus) erzeugt. Dass beide Seiten deckungsgleich sind, steht
// NICHT als Zusage im Kommentar, sondern als compile-time-Wache in tests/unit/test_e24_c9_fk5_fehlerraum.cpp
// -- eine TU, die BEIDE Schichten sehen darf. Praezedenz im Haus, woertlich: test_w10_c4_zellwert_naht.cpp
// ("Die Store-Key-Naht darf den Segment-Schluessel nicht aus profile_facade inkludieren (Layer-Richtung).
// Dass beide Schreibweisen dennoch identisch sind, ist deshalb HIER verwacht -- diese TU darf beide Seiten
// sehen. Ohne diese Zeile stuende die Gleichheit nur als Zusage im Kommentar.").
//
// GEGENSTAND (D2, nicht D1): eine Organ-Achse ist ein ALGORITHMUS, der zur Laufzeit gemessen wird. Die
// A15-Realm-Zuordnung weist ihr deshalb die D2-Domaene zu ("organ-Achsen -> D2 (Failed am Pruef-Dock-Gate)",
// A15-fehlerklassen.md EBENE 2). D1 (CompilerCompilerErrorClass) ist die Planer-/Compile-Zeit-Domaene und
// gehoert NICHT hierher -- die Domaenen-Trennung dieser beiden Raeume ist der Kern von axis_error.hpp.
//
// ASCII-only.

#include <array>
#include <concepts> // FK-6: std::convertible_to in den beiden Gate-/Namen-Concepts
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::topics {

// -- Die D2-Etiketten, die eine Organ-Achse deklarieren darf ------------------------------------
// Zeichenfolgen-identisch zu measurement::sample_status_label(); die Gleichheit ist in der
// Cross-Layer-Test-TU compile-time verwacht (s. Kopf). "mess_ok" fehlt bewusst: Ok ist der Normalfall,
// kein Fehlerraum-Mitglied -- eine Achse, die NUR "mess_ok" deklarierte, haette einen leeren Fehlerraum
// und wuerde von der Nicht-Leerheits-Wache unten gefangen.

/// D2::Failed -- der ALGORITHMISCHE Mess-Fehler (OOM, Gate-Fail, Exception). CSV-Zelle "failed", nie 0.
inline constexpr std::string_view kOrganErrMessFehler = "mess_fehler";
/// D2::NotApplicable -- die Achse ist fuer diese Binary strukturell sinnlos. Ehrliches "n/a", KEIN Fehler.
inline constexpr std::string_view kOrganErrNichtAnwendbar = "nicht_anwendbar";
/// D2::SourceUnavailable -- die MESS-QUELLE (PMC/DLL/Interface) fehlt. Ehrliches "n/a", KEIN Algo-Fehler.
inline constexpr std::string_view kOrganErrQuelleNichtVerfuegbar = "quelle_nicht_verfuegbar";

/// Der REALM-BODEN jeder Organ-Haupt-Achse (A15 EBENE 2, Realm-Zuordnung woertlich: "organ-Achsen -> D2
/// (Failed am Pruef-Dock-Gate)"). JEDE Organ-Achse kann am Dock-Gate scheitern -- das ist keine Annahme
/// ueber den einzelnen Algorithmus, sondern eine Eigenschaft des Gates, durch das alle laufen.
inline constexpr auto kOrganAxisErrorFloor = std::array<std::string_view, 1>{kOrganErrMessFehler};

/// Boden + strukturelle Nicht-Anwendbarkeit. Fuer Achsen, deren Mess-Groessen am 1-Thread-/In-Memory-/
/// decide-only-Mess-Kanon strukturell leer bleiben -- diese Leere ist ehrlich "n/a" und NICHT "failed".
inline constexpr auto kOrganAxisErrorFloorPlusNa =
    std::array<std::string_view, 2>{kOrganErrMessFehler, kOrganErrNichtAnwendbar};

// -- Die Wachen (CT-only, emittieren nichts) -------------------------------------------------

/// Ein Achsen-/Varianten-Typ traegt eine NICHT-LEERE Fehlerraum-Deklaration.
template <class T>
concept OrganAxisMitFehlerklassen = requires {
    { T::error_classes() };
} && (T::error_classes().size() >= 1);

/// assert_organ_axis_error_classes<T>() -- die fokussierte Wache, die JEDE der 18 Organ-Haupt-Achsen-
/// CRTP-Basen im Ctor traegt (K2-Muster, dieselbe Stelle wie der algo_version-Guard). Sie prueft, dass
/// der Fehlerraum EXISTIERT und NICHT LEER ist. Was sie NICHT leistet: sie prueft nicht, ob der Satz
/// fuer DIESEN Algorithmus vollstaendig ist -- diese per-Varianten-Verfeinerung war bis FK-6 die
/// "deklarierte Luecke". FK-6 (unten) schliesst sie GEZIELT statt flaechig; die Begruendung, warum
/// flaechig FALSCH waere, steht dort und ist gemessen, nicht behauptet.
template <class T>
constexpr void assert_organ_axis_error_classes() noexcept {
    static_assert(
        requires { T::error_classes(); },
        "Organ-Achse ohne 'static constexpr error_classes()' (FK-5, Owner-Direktive 17.07.2026: "
        "Fehlerklassen sind fuer ALLE Achsen -> Unterachsen -> Algorithmen Pflicht).");
    static_assert(T::error_classes().size() >= 1,
                  "Organ-Achse mit LEEREM Fehlerraum: eine leere Deklaration ist genau die stille Luecke, "
                  "die FK-5 schliessen soll -- mindestens der Realm-Boden (mess_fehler) gehoert deklariert.");
}

// == FK-6 (A15 EBENE 4, ALGORITHMEN-Ebene): die per-Varianten-Verfeinerung ====================
//
// DER BEFUND, DER DIESE WACHE FORMT (gemessen 09.08.2026, nicht behauptet). Die Owner-Direktive
// verlangt Fehlerklassen fuer "ALLE Achsen -> Unterachsen -> Algorithmen". Die naheliegende Lesart
// waere: JEDE registrierte Organ-Variante deklariert einen eigenen Satz. Die Messung sagt, dass das
// FALSCH waere, und die Zahlen stehen hier, damit der naechste sie nicht neu erheben muss:
//
//   126  registrierte Organ-Varianten gesamt (mp_size ueber AllRegisteredOrganVariantsFlat; die
//        oft zitierte "121" ist NICHT der Ist-Stand -- gegengeprueft mit 124 algo_version-Literalen
//        unter axes/+topics/, die Differenz sind Registry-Wrapper ohne eigenes Literal)
//    26  davon tragen ueberhaupt requires_specialized_hardware() (die gesamte Allokator-Achse)
//     1  davon meldet true: pim_malloc (PIM-DPU-Hardware, UPMEM/HBM-PIM)
//
// DIE KERNFRAGE WAR: welche Fehlerklasse braucht ein Algorithmus, die seine ACHSE nicht abdeckt?
// Fuer 125 der 126 lautet die Antwort KEINE -- ihr Fehlerraum ist der ihrer Achse, und 126
// Deklarationen waeren 125-mal Abschrift. Fuer GENAU EINEN lautet sie: quelle_nicht_verfuegbar.
// pim_malloc misst gegen Hardware, die auf einer CPU-Flotte strukturell FEHLT; das ist D2
// SourceUnavailable, und die Allokator-Achse fuehrt nur den Boden (mess_fehler). Ein solcher Lauf
// haette als "mess_fehler" gelesen -- also als DEFEKT DES ALGORITHMUS statt als fehlende Quelle.
// Genau diese Verwechslung ist es, gegen die axis_error.hpp gebaut wurde.
//
// ZWEITER BEFUND, der die Luecke belegt: kOrganErrQuelleNichtVerfuegbar wird von 0 der 18 Achsen
// gefuehrt (14x Boden, 4x Boden+n/a). Das Etikett existierte, war compile-time an
// sample_status_label(SourceUnavailable) gebunden -- und hatte im ganzen Organ-Raum keinen Traeger.
//
// DESHALB IST DIESE WACHE BEDINGT, NICHT FLAECHIG: sie verlangt eine eigene Deklaration NUR dort,
// wo der Algorithmus eine Klasse hervorbringen kann, die seine Achse nicht fuehrt. Eine flaechige
// Wache haette 126 Abschriften erzwungen und dabei kein einziges falsches Etikett verhindert.

/// Ein Typ, der das HW-Gate ueberhaupt fuehrt (heute: die 26 Allokator-Varianten).
template <class T>
concept AlgoMitHardwareGate = requires {
    { T::requires_specialized_hardware() } -> std::convertible_to<bool>;
};

/// Ein Typ, der einen bezifferbaren Namen fuehrt (der Schluessel der Uebergangsliste unten).
template <class T>
concept AlgoMitNamen = requires {
    { T::name() } -> std::convertible_to<std::string_view>;
};

/// Fuehrt ein Fehlerraum-Satz ein bestimmtes Etikett? (constexpr, damit die Wache CT bleibt)
template <class Satz>
[[nodiscard]] constexpr bool fehlerraum_enthaelt(Satz const& s, std::string_view etikett) noexcept {
    for (std::string_view const e : s)
        if (e == etikett) return true;
    return false;
}

/// DIE UEBERGANGSLISTE (Abnahme-Formel ##06: "erfuellt ODER Allowlist mit Begruendung").
///
/// SIE IST HEUTE LEER, UND DAS IST DAS ERGEBNIS, NICHT DER ANFANGSZUSTAND: die einzige Variante,
/// die je darauf gehoert haette (pim_malloc), ist im selben Zug migriert. Die Liste bleibt stehen,
/// weil die naechste HW-gated Variante (OD-5 nennt GPU/FPGA ausdruecklich als Nachruestung) sonst
/// entweder den Baum bricht oder die Wache aufgeweicht wird -- beides schlechter als ein Eintrag
/// mit Begruendung. Ihre LAENGE ist die Kennzahl: sie darf nur schrumpfen, nie wachsen.
///
/// SCHLUESSEL-FALLE, am Objekt gemessen und hier festgehalten: der Schluessel ist das GERENDERTE
/// W::name(). Vendor-gegatete Varianten rendern flag-abhaengig -- pim_malloc heisst "pim_malloc",
/// wenn COMDARE_AXIS_06_USE_PIM_MALLOC 1 ist, und "pim_malloc(real=std)", wenn es 0 ist (gemessen:
/// in diesem Baum ist es 0). Ein Eintrag muesste also BEIDE Schreibweisen fuehren. Wer hier je
/// einen setzt, findet diesen Satz vor, statt die Flag-Abhaengigkeit am Verhalten zu erraten.
inline constexpr auto kAlgoFehlerraumUebergangsliste = std::array<std::string_view, 0>{};

/// algo_fehlerraum_erfuellt<W>(liste) -- das PRAEDIKAT hinter der Wache, mit der Uebergangsliste als
/// PARAMETER statt als fest verdrahtetem Zugriff. Der Grund ist die Pruefbarkeit: nur so kann eine
/// Test-TU denselben Code einmal gegen die LEERE und einmal gegen eine eigene Liste fahren und damit
/// belegen, dass Listen-Mitgliedschaft wirklich durchlaesst. Waere die Liste fest verdrahtet, liesse
/// sich der Durchlass-Zweig bei leerer Liste ueberhaupt nicht ausloesen -- eine Wachen-Haelfte, die
/// niemand je beobachtet (Memory: VERDECKTE ZWEIGE sind nicht beobachtbar).
///
/// Erfuellt ist, wer EINE der drei Bedingungen traegt:
///   (1) kein HW-Gate oder Gate==false -> die Achse deckt ihn ab, nichts zu deklarieren (125 von 126),
///   (2) HW-Gate==true UND 'quelle_nicht_verfuegbar' im eigenen Satz -> migriert (pim_malloc),
///   (3) HW-Gate==true UND Name auf der Uebergangsliste -> geduldet, mit Begruendung dort.
template <class W, std::size_t N>
[[nodiscard]] constexpr bool algo_fehlerraum_erfuellt(std::array<std::string_view, N> const& uebergangsliste) noexcept {
    if constexpr (!AlgoMitHardwareGate<W>) {
        return true; // (1) kein Gate -> kein algorithmen-eigener Anspruch
    } else if constexpr (!W::requires_specialized_hardware()) {
        return true; // (1) Gate false -> die Quelle ist die CPU, und die ist da
    } else {
        if constexpr (requires { W::error_classes(); })
            if (fehlerraum_enthaelt(W::error_classes(), kOrganErrQuelleNichtVerfuegbar)) return true; // (2)
        if constexpr (AlgoMitNamen<W>)
            for (std::string_view const geduldet : uebergangsliste)
                if (geduldet == W::name()) return true; // (3)
        return false;
    }
}

/// assert_algo_error_classes<W>() -- die FK-6-Wache. Sie sitzt an derselben Population wie ihre
/// FK-5-Schwester (guard_all_registered_organ_error_classes, axis_variant_version_table.hpp) und
/// bricht compile-hart MIT dem Typ-Namen. Reine CT-Wache, emittiert nichts.
template <class W>
constexpr void assert_algo_error_classes() noexcept {
    static_assert(algo_fehlerraum_erfuellt<W>(kAlgoFehlerraumUebergangsliste),
                  "FK-6: dieser Algorithmus meldet requires_specialized_hardware()==true, fuehrt aber "
                  "'quelle_nicht_verfuegbar' NICHT in error_classes() und steht auf keiner "
                  "Uebergangsliste. Seine Mess-Quelle kann strukturell fehlen -- ohne die Klasse "
                  "laese ein solcher Lauf als 'mess_fehler', also als Defekt DES ALGORITHMUS statt "
                  "als fehlende Quelle (Owner-Direktive 17.07.2026: Fehlerklassen sind fuer ALLE "
                  "Achsen -> Unterachsen -> Algorithmen Pflicht).");
}

} // namespace comdare::cache_engine::topics
