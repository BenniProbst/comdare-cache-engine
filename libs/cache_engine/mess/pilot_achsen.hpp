#pragma once
// mess/pilot_achsen.hpp -- (5) DIE ACHSEN-INTERFACES, auf die sich der Hauptalgorithmus stuetzt.
//
// == WO DIESE DATEI IM SCHICHTENBILD STEHT =========================================================
// GANZ UNTEN. Owner-Schichtung: "Achsen-Interfaces -- der Hauptalgorithmus stuetzt sich AUSSCHLIESSLICH
// auf sie, rekursiv und QUER zu anderen Gattung+Genus". Die Achsen liegen UNTER der _impl; Gattung
// und Genus sind keine Achsen, sondern Ebenen der metaprogrammierten Klassenhierarchie darueber.
//
// == WAS HIER AUSDRUECKLICH NICHT STEHT ============================================================
// KEINE MESSUNG. Keine Achse dieser Datei kennt MK, den Visitor, eine Arena oder einen Checkpoint.
// Das ist die Probe auf die Naht-Entscheidung: sitzt die Mess-Naht wirklich am Genus-Interface, dann
// muessen die Achsen von ihr NICHTS wissen -- und genau das laesst sich hier ablesen.
//
// Waere die Naht in den Achsen, stuende hier je Achse eine Messeinrichtung: 18 Nahtstellen statt
// einer, 18 Gelegenheiten zur Abweichung, und jede Achse trueg den Overhead ihrer eigenen Messung in
// ihrem eigenen Messwert. Dass diese Datei ohne jeden Mess-Bezug uebersetzt, IST der Beleg.
//
// == DIE KOEDER-ACHSE =============================================================================
// Sie ist eine gewoehnliche Achse mit einem DETERMINISTISCHEN, NACHRECHENBAREN Ergebnis. Das ist
// keine Testattrappe, sondern die Voraussetzung dafuer, dass die Abnahme eine AUSSAGE treffen kann
// statt bloss Anwesenheit festzustellen (T-2): ein Test, der seinen Sollwert nicht kennt, kann nur
// pruefen, DASS eine Zahl ankam -- nicht, dass es DIE RICHTIGE war.
//
// Warum der Messwert kein ZEITWERT ist: eine Zeit laesst sich nicht unabhaengig nachrechnen. Ein
// Orakel, das nur "groesser als null" sagen kann, ueberlebt jede Mutation der Kette -- es waere ein
// Koeder, der nicht beisst (K13).
//
// ASCII-only.

#include <cstdint>

namespace comdare::cache_engine::mess::pilot {

/// PilotKomposition -- die Achsen-Auspraegung des Piloten.
///
/// SELBSTCHECK: beide Funktionen sind rein (gleiche Eingabe, gleiche Ausgabe) und haengen an keinem
/// globalen Zustand. Nur deshalb kann die Abnahme ihren Sollwert VOR dem Lauf ausrechnen und danach
/// vergleichen -- die Zeitrichtung der Pruefung (Messung VOR dem Satz) haengt daran.
struct PilotKomposition {
    /// ACHSE 1 -- Vorbereitung. Eine gewoehnliche, deterministische Achsen-Rechnung.
    [[nodiscard]] static std::uint64_t vorbereitung(std::uint64_t saat, std::uint64_t runden) noexcept {
        std::uint64_t z = saat;
        for (std::uint64_t i = 0; i < runden; ++i) {
            z = (z * 6364136223846793005ull) + 1442695040888963407ull; // LCG, Standardkoeffizienten
        }
        return z >> 33u;
    }

    /// ACHSE 2 -- DIE KOEDER-ACHSE. f(R) = (R mod 4093) + 17.
    ///
    /// 4093 ist prim und teilt keine Zweierpotenz -- damit haengt das Ergebnis von ALLEN Bits von R
    /// ab, nicht nur von den untersten. Ein Koeder, dessen Wert nur die letzten Bits benutzt, liesse
    /// eine Kette durchgehen, die R irgendwo unterwegs maskiert.
    ///
    /// Der Summand 17 hebt den Wertebereich von der Null weg. Das ist Absicht: eine 0 ist der Wert,
    /// den eine ABGERISSENE Kette liefert (default-konstruierte Zeile), und ein Sollwert, der 0
    /// werden kann, waere von genau diesem Fehlerfall nicht zu unterscheiden.
    [[nodiscard]] static std::uint64_t koeder(std::uint64_t r) noexcept { return (r % 4093ull) + 17ull; }
};

} // namespace comdare::cache_engine::mess::pilot
