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
/// der Fehlerraum EXISTIERT und NICHT LEER ist. Was sie NICHT leistet und was hier ausdruecklich steht
/// statt verschwiegen zu werden: sie prueft nicht, ob der Satz fuer DIESEN Algorithmus vollstaendig ist
/// -- das ist die per-Varianten-Verfeinerung, die als deklarierte Luecke offen bleibt (s. Commit-Text).
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

} // namespace comdare::cache_engine::topics
