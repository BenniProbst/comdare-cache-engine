#pragma once
// measurement/ram_frequency_reading.hpp -- Provenienz-Vokabular + Ergebnis-Typ der RAM-Frequenz-Erhebung
// (Task #7, Paket P1; Owner-KERN 27.07.: "Hardware-Werte nie statisch, die CEB liest sie zur Laufzeit").
//
// WAS HIER STEHT UND WAS NICHT: Diese Datei traegt AUSSCHLIESSLICH Vokabular und den Ergebnis-Typ --
// kein IO, keine Erhebungs-Kette, kein Aufrufer. Die Kette (Boot-Cache -> SPD -> Deklaration) und die
// OS-Factory folgen in P2; der Byte-Parser liegt in spd_ddr5_parser.hpp. Rein additiv, golden-neutral.
//
// -- WARUM PROVENIENZ UEBERHAUPT (die eigentliche Aussage des Pakets) -----------------------------
// Ein nackter Zahlenwert "4800" beantwortet die entscheidende Frage nicht: WOHER stammt er? Der real
// laufende Takt ist unter Linux nur privilegiert lesbar (SMBIOS Type 17, gemessen 0400 root-only),
// waehrend SPD unprivilegiert nur den JEDEC-NENNWERT der Riegel liefert. Beides als dieselbe Zahl
// auszuweisen waere die teuerste Sorte Fehler: die Messreihe behauptete einen Ist-Takt, wo ein
// Datenblatt-Wert steht. Deshalb reist die Herkunft IMMER mit dem Wert.
//
// -- DIE DREI STUFEN (geordnet, absteigendes Vertrauen) -------------------------------------------
//   1 configured_measured  : der KONFIGURIERTE Ist-Takt (SMBIOS "Configured Memory Speed"). Nur ueber
//                            einen privilegierten Kanal erreichbar (Infra-Boot-Cache, P6).
//   2 spd_jedec_base       : JEDEC-Basistakt aus dem SPD-EEPROM. Unprivilegiert lesbar, aber NUR die
//                            Nennrate der Riegel -- welches Profil das BIOS aktiviert hat, steht dort
//                            nicht.
//   3 declared_not_measured: der in der Anwender-XML deklarierte Wert (heutiger Zustand).
//
// -- UND WARUM "NICHT ERHOBEN" KEINE VIERTE STUFE IST ---------------------------------------------
// Eine Vertrauens-Stufe beschreibt die HERKUNFT eines vorhandenen Wertes. "Nicht erhoben" heisst, dass
// es keinen Wert gibt -- da ist nichts, dessen Herkunft man vergleichen koennte. Als vierte Stufe
// gefuehrt wuerde es in jeden Stufen-Vergleich einsortiert ("schlechter als deklariert") und damit als
// Wert behandelt, der es nicht ist. Der Zustand steht deshalb in einem EIGENEN Enum ausserhalb des
// Rankings; trust_rank() nimmt ihn gar nicht erst an (Typ-Schnitt statt Konvention).
//
// -- EINHEITEN-NAHT (Bestands-Falle, machine_identity.hpp:59) -------------------------------------
// Das Bestandsfeld heisst ram_frequency_mhz, traegt aber MT/s (Megatransfers pro Sekunde) -- bei DDR
// ist die Transferrate doppelt so hoch wie der Bus-Takt (4800 MT/s = 2400 MHz Bus). Der Name ist
// historisch und wird hier NICHT wiederholt: die Felder dieser Datei heissen _mts. Wer zwischen beiden
// Welten kopiert, kopiert dieselbe Zahl -- nur der Name luegt im Bestand, nicht der Wert.
//
// -- GOLDEN-NEUTRALITAET (Auflage A10, strukturell) -----------------------------------------------
// RamFrequencyReading ist ein reiner Melde-Typ. Er wird von KEINER Stempel-, Achsen- oder binary_id-API
// angenommen; gemessene WERTE gehoeren nie in einen Stempel (Section-43-Invariante). Die Erhebung
// beeinflusst ausschliesslich Log- und CSV-Ausgabe.

#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

// -- Die Herkunft eines vorhandenen Wertes (geordnet: kleinerer Wert = hoeheres Vertrauen) --------
enum class RamFrequencyProvenance : std::uint8_t {
    ConfiguredMeasured  = 0, // Stufe 1: konfigurierter Ist-Takt (privilegierte Quelle)
    SpdJedecBase        = 1, // Stufe 2: JEDEC-Nennrate aus dem SPD-EEPROM (unprivilegiert)
    DeclaredNotMeasured = 2, // Stufe 3: Deklarationswert der Anwender-XML
};
/// Single-Source der Stufenzahl (Drift-Guards unten, beide Richtungen).
inline constexpr std::size_t kRamFrequencyProvenanceCount = 3;

/// Der ZUSTAND der Erhebung -- bewusst getrennt vom Stufen-Ranking (siehe Kopf-Block).
enum class RamReadingState : std::uint8_t {
    NichtErhoben = 0, // Default: es liegt KEIN Wert vor (nie als 0 MT/s lesen)
    Erhoben      = 1, // ein Wert liegt vor; seine Herkunft steht in der Provenienz
};
inline constexpr std::size_t kRamReadingStateCount = 2;

/// Vertrauens-Rang (kleiner = vertrauenswuerdiger). NIMMT NUR eine Provenienz an: der Zustand
/// NichtErhoben ist hier strukturell nicht ausdrueckbar, weil er kein Vertrauen beschreibt.
[[nodiscard]] constexpr std::uint8_t provenance_trust_rank(RamFrequencyProvenance p) noexcept {
    return static_cast<std::uint8_t>(p);
}

/// Log-/CSV-Etikett je Stufe. Die Namen sind die des Plans und wandern unveraendert in die Auswertung.
[[nodiscard]] constexpr std::string_view provenance_token(RamFrequencyProvenance p) noexcept {
    switch (p) {
        case RamFrequencyProvenance::ConfiguredMeasured: return "configured_measured";
        case RamFrequencyProvenance::SpdJedecBase: return "spd_jedec_base";
        case RamFrequencyProvenance::DeclaredNotMeasured: return "declared_not_measured";
    }
    return "provenienz_unbekannt";
}

/// Zustands-Etikett. "nicht_erhoben" ist NICHT dasselbe wie das CSV-Zell-Token: in der Wert-Spalte
/// steht bei fehlendem Wert das etablierte "n/a" (Auflage A7 -- nie eine 0-Zahl), waehrend dieses
/// Etikett die eigene Zustands-Spalte fuellt. Zwei Spalten, zwei Aussagen.
[[nodiscard]] constexpr std::string_view reading_state_token(RamReadingState s) noexcept {
    switch (s) {
        case RamReadingState::NichtErhoben: return "nicht_erhoben";
        case RamReadingState::Erhoben: return "erhoben";
    }
    return "zustand_unbekannt";
}

// -- Der Ergebnis-Typ ------------------------------------------------------------------------------
/// Das Erhebungs-Ergebnis EINER Maschine. Default-konstruiert ist es ehrlich leer (NichtErhoben, 0),
/// nicht etwa "0 MT/s deklariert" -- dieselbe Ehrlichkeits-Regel wie die 0-Marke in kDeclaredMachines.
///
/// P2 erweitert diesen POD additiv um den Degradations-Pfad (welche Stufen fielen mit welchem
/// HardwareProbeErrorClass durch) und den Erhebungs-Zeitpunkt. Beides fuellt erst die Kette; hier
/// stehen nur Felder, die der Parser-/Deklarations-Weg selbst belegen kann.
struct RamFrequencyReading {
    std::uint32_t          mts                = 0; ///< MT/s; NUR gueltig bei state==Erhoben
    std::uint32_t          tck_ps             = 0; ///< SPD-Rohwert tCKAVGmin (ps), Ableitungs-Beleg
    std::uint32_t          xmp_expo_offer_mts = 0; ///< ANGEBOT der Riegel (XMP/EXPO), NIE der Ist-Wert
    RamFrequencyProvenance provenance         = RamFrequencyProvenance::DeclaredNotMeasured;
    RamReadingState        state              = RamReadingState::NichtErhoben;
};

/// Traegt das Ergebnis einen benutzbaren Wert? Einzige zulaessige Art, die 0 zu interpretieren.
[[nodiscard]] constexpr bool has_frequency(RamFrequencyReading const& r) noexcept {
    return r.state == RamReadingState::Erhoben && r.mts != 0U;
}

/// Traegt das Ergebnis ein Riegel-ANGEBOT (XMP/EXPO)? Getrennt abfragbar, damit niemand es versehentlich
/// als Frequenz ausliest -- es gibt bewusst keine Funktion, die das Angebot als mts zurueckgibt (A7).
[[nodiscard]] constexpr bool has_profile_offer(RamFrequencyReading const& r) noexcept {
    return r.xmp_expo_offer_mts != 0U;
}

// -- Wachen: POD-Form, beide Drift-Richtungen, Ordnung, Trennung von Wert und Zustand --------------
static_assert(std::is_same_v<std::underlying_type_t<RamFrequencyProvenance>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<RamReadingState>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<RamFrequencyReading>);
static_assert(std::is_aggregate_v<RamFrequencyReading>);
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt (RF-3).
static_assert(kRamFrequencyProvenanceCount ==
              static_cast<std::size_t>(RamFrequencyProvenance::DeclaredNotMeasured) + 1);
static_assert(provenance_token(static_cast<RamFrequencyProvenance>(kRamFrequencyProvenanceCount)) ==
                  std::string_view{"provenienz_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Provenienz-Stufe");
static_assert(kRamReadingStateCount == static_cast<std::size_t>(RamReadingState::Erhoben) + 1);
static_assert(reading_state_token(static_cast<RamReadingState>(kRamReadingStateCount)) ==
                  std::string_view{"zustand_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettierter Zustand");
// Die Ordnung IST die Aussage: gemessen schlaegt Nennwert schlaegt Deklaration. Dreht jemand die
// Reihenfolge um, waehlt die Kette in P2 stillschweigend die schlechtere Quelle.
static_assert(provenance_trust_rank(RamFrequencyProvenance::ConfiguredMeasured) <
              provenance_trust_rank(RamFrequencyProvenance::SpdJedecBase));
static_assert(provenance_trust_rank(RamFrequencyProvenance::SpdJedecBase) <
              provenance_trust_rank(RamFrequencyProvenance::DeclaredNotMeasured));
// Der Default ist der ehrliche Leerzustand, nicht "0 MT/s gemessen".
static_assert(!has_frequency(RamFrequencyReading{}));
static_assert(RamFrequencyReading{}.state == RamReadingState::NichtErhoben);
static_assert(static_cast<std::uint8_t>(RamReadingState::NichtErhoben) == 0,
              "NichtErhoben MUSS 0 sein: ein default-konstruiertes Ergebnis darf nie einen Wert behaupten.");
// Wert und Zustand sind unabhaengig: eine Zahl ohne Erhoben-Zustand bleibt ungueltig (kein stiller Wert).
static_assert(!has_frequency(RamFrequencyReading{.mts = 4800}));
static_assert(has_frequency(RamFrequencyReading{.mts = 4800, .state = RamReadingState::Erhoben}));
// Das Angebot ist NIE die Frequenz -- ein Reading mit Angebot, aber ohne erhobenen Wert bleibt leer.
static_assert(!has_frequency(RamFrequencyReading{.xmp_expo_offer_mts = 5600}));
static_assert(has_profile_offer(RamFrequencyReading{.xmp_expo_offer_mts = 5600}));
// Provenienz-Etiketten sind gegen die bestehenden Fehler-/Zell-Vokabeln disjunkt (dieselbe Wache wie
// in axis_error.hpp -- die Auswertung liest alle diese Spalten nebeneinander).
static_assert(probe_label_ist_disjunkt(provenance_token(RamFrequencyProvenance::ConfiguredMeasured)));
static_assert(probe_label_ist_disjunkt(provenance_token(RamFrequencyProvenance::SpdJedecBase)));
static_assert(probe_label_ist_disjunkt(provenance_token(RamFrequencyProvenance::DeclaredNotMeasured)));
static_assert(probe_label_ist_disjunkt(reading_state_token(RamReadingState::NichtErhoben)));
static_assert(probe_label_ist_disjunkt(reading_state_token(RamReadingState::Erhoben)));

} // namespace comdare::cache_engine::measurement
