#pragma once
// profile_facade/system_axes_entscheidung.hpp -- T-1: DIE EINE ENTSCHEIDUNG "deklariert dieses Profil
// System-Achsen?" -- und die Wache, die sie ueber zwei Parses hinweg zusammenhaelt.
//
// WARUM ES DIESE DATEI GIBT. Zwei Stellen beantworteten dieselbe Frage aus je EIGENEM Parse derselben
// Datei:
//   FACADE  profile_run_facade.cpp -- entscheidet, ob a.compile_for_perm BELEGT wird.
//   ENTRY   profile_run_entry.hpp  -- entscheidet, ob die opt x simd-Perm-SCHLEIFE ueberhaupt LAEUFT.
// Beide Ausdruecke waren De-Morgan-Duale und damit ueber EINEN Parse nicht trennbar (gemessen:
// tests/unit/test_t1_system_achsen_eine_entscheidung.cpp, Fall AEQUIVALENZ, 5 von 5 Profil-Formen).
// Getrennt hat sie nicht der Ausdruck, sondern die STRUKTUR: es sind ZWEI UNABHAENGIGE
// load_thesis_profile()-Aufrufe auf denselben Pfad zu ZWEI Zeitpunkten, zwischen denen die Facade
// Arbeit leistet (Lastprofil-Entdeckung, Methodik-Aufloesung, Achsen-Versionstabelle). Aendert sich die
// Datei in diesem Fenster, antworten die beiden Parses verschieden -- und nichts verglich sie.
//
// WAS DIE DRIFT KOSTET (die Richtung, die weh tut): sagt die Facade "nein" und der Entry "ja", laeuft
// die Perm-Schleife OHNE compile_for_perm. Dann ist perm_bau_je_zelle false, der per-Perm-Fingerprint-
// Provider greift NICHT und es wirkt der lauf-konstante -- jede Zelle bekommt denselben Digest, und die
// Zell-Koordinaten sind der einzige Diskriminator im Lager-Schluessel (LAG-Z1, WEG F). Die Gegenrichtung
// kostet die Provenienz: die Basis-build_version traegt dann den system_axes_version_suffix() NICHT,
// obwohl die Schleife, die ihn je Perm angehaengt haette, gar nicht laeuft.
//
// DIE LOESUNG IST EINE GEMEINSAME FUNKTION, KEIN static_assert. Die Schichtung erlaubt sie: dieser
// Header haengt NUR am Parser-Datentyp (libs/common/serialization), und BEIDE Verbraucher liegen in
// profile_facade und inkludieren ihn ABWAERTS. Der `builder` ist nicht beteiligt und inkludiert nichts
// aufwaerts. Ein static_assert schiede ohnehin aus: beide Seiten entscheiden ueber LAUFZEIT-Daten
// (geparste XML), es gibt nichts, was der Uebersetzer vergleichen koennte.
//
// DIE MITFUEHRUNG. Weil der zweite Parse nicht wegfallen kann (run_profile ist ein oeffentlicher
// Einstieg und braucht das ganze Profil), wird die EINMAL gefaellte Entscheidung als WERT mitgefuehrt
// und beim zweiten Parse GEGENGEPRUEFT. NichtGetragen ist der dritte Zustand und haelt IMMER -- ein
// Direkt-Aufrufer ohne Facade darf von dieser Wache nicht abgeschossen werden.
//
// header-only, ASCII-Kommentare, keine Bau-Abhaengigkeit ausser dem Parser-Datentyp.

#include "xml_config_parser/xml_config_parser.hpp" // ThesisProfile: compiler.opt_levels / external_utils.simd_options

#include <cstdint>

namespace comdare::cache_engine::profile_facade {

/// DIE EINE ENTSCHEIDUNG. Deklariert das Profil System-Achsen, die eine opt x simd-Permutation tragen?
/// Genau zwei Unter-Achsen zaehlen, weil genau sie die Schleife aufspannen: compiler/opt_level und
/// external_utils/simd. Andere System-Deklarationen (atomic128, target_isa) permutieren hier NICHT --
/// wer sie hinzunimmt, muss die Schleife mit hinzunehmen, nicht nur dieses Praedikat.
[[nodiscard]] inline bool profil_deklariert_system_achsen(::comdare::builder::xml::ThesisProfile const& tp) {
    return !tp.compiler.opt_levels.empty() || !tp.external_utils.simd_options.empty();
}

/// Der mitgefuehrte Wert. Drei Zustaende -- NichtGetragen ist NICHT dasselbe wie Nein: er heisst "dieser
/// Aufrufer hat die Frage nie gestellt", und dann gibt es auch nichts gegenzupruefen.
enum class SystemAchsenEntscheidung : std::uint8_t { NichtGetragen = 0, Nein = 1, Ja = 2 };

/// Die Facade faellt die Entscheidung EINMAL und verpackt sie hiermit fuer die Reise.
[[nodiscard]] inline SystemAchsenEntscheidung system_achsen_entscheidung_von(bool deklariert) {
    return deklariert ? SystemAchsenEntscheidung::Ja : SystemAchsenEntscheidung::Nein;
}

/// DIE WACHE. Haelt der mitgefuehrte Wert gegen den eigenen Parse des Empfaengers?
/// NichtGetragen haelt immer (s. oben). Sonst muss der eigene Parse dieselbe Antwort geben -- tut er es
/// nicht, hat sich das Profil zwischen den beiden Parsen bewegt, und der Aufrufer muss FAIL-CLOSED
/// abbrechen statt mit einer halben Verdrahtung weiterzulaufen.
[[nodiscard]] inline bool system_achsen_entscheidung_haelt(SystemAchsenEntscheidung                      getragen,
                                                           ::comdare::builder::xml::ThesisProfile const& eigener) {
    if (getragen == SystemAchsenEntscheidung::NichtGetragen) return true;
    return (getragen == SystemAchsenEntscheidung::Ja) == profil_deklariert_system_achsen(eigener);
}

} // namespace comdare::cache_engine::profile_facade
