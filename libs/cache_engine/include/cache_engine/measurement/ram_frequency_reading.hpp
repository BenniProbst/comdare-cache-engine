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

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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

// -- P2: DER DEGRADATIONS-PFAD ---------------------------------------------------------------------
// Die Erhebungs-Kette (P2, ram_probe_chain.hpp) fragt die Stufen der Reihe nach. Das Ergebnis allein
// sagt am Ende nur, WOHER der Wert kam -- nicht, was auf dem Weg dorthin geschah. Genau das ist die
// interessante Auskunft: fiel Stufe 1 durch, weil die Boot-Cache-Datei fehlt (erwartet), oder weil sie
// da war und HALB GESCHRIEBEN (Befund)? Auflage A4 verlangt, dass diese beiden nie zusammenfallen --
// also muss der Weg mitreisen, nicht nur das Ziel.
//
// Der Pfad ist ueber die PROVENIENZ-Stufen indiziert und fuehrt bewusst KEIN zweites Stufen-Enum: eine
// Kette mit eigener Stufen-Nummerierung koennte gegen die Provenienz-Ordnung driften, und dann stuende
// im Pfad Stufe 2 fuer etwas anderes als im Ergebnis. Eine Quelle, eine Ordnung.

/// Der Ausgang EINER Stufe. Die drei Werte sind bewusst unterscheidbar: "nicht gefragt" ist kein
/// Fehlschlag (eine bessere Stufe hatte schon geliefert), und ein Fehlschlag ohne Grund waere die
/// stille Degradation, die A4 verbietet.
enum class ProbeStageOutcome : std::uint8_t {
    NichtGefragt  = 0, // die Kette kam nicht bis hierher -- eine hoehere Stufe hat geliefert
    Geliefert     = 1, // diese Stufe hat den Wert geliefert
    Durchgefallen = 2, // gefragt und ohne Wert zurueck -- der Grund steht in der Fehlerklasse
};
inline constexpr std::size_t kProbeStageOutcomeCount = 3;

[[nodiscard]] constexpr std::string_view probe_stage_outcome_token(ProbeStageOutcome o) noexcept {
    switch (o) {
        case ProbeStageOutcome::NichtGefragt: return "nicht_gefragt";
        case ProbeStageOutcome::Geliefert: return "geliefert";
        case ProbeStageOutcome::Durchgefallen: return "durchgefallen";
    }
    return "stufen_ausgang_unbekannt";
}

/// Der Weg der Kette: je Provenienz-Stufe ein Ausgang und (nur bei Durchgefallen) ein Grund.
/// Fester Array statt Liste -- der Typ bleibt damit trivially_copyable und allokiert nie.
struct RamProbeTrail {
    std::array<ProbeStageOutcome, kRamFrequencyProvenanceCount>       outcome{};
    std::array<HardwareProbeErrorClass, kRamFrequencyProvenanceCount> reason{};
};

[[nodiscard]] constexpr ProbeStageOutcome stage_outcome(RamProbeTrail const& t, RamFrequencyProvenance s) noexcept {
    return t.outcome[static_cast<std::size_t>(s)];
}

/// Der Grund, warum eine Stufe durchfiel -- und NUR dann. Bei NichtGefragt/Geliefert gibt es keinen
/// Grund zu lesen, und das Ergebnis sagt das strukturell (nullopt) statt per Konvention. Ohne diesen
/// Schnitt wuerde das Default-Feld QuelleFehlt=0 als "fehlte" gelesen, wo gar nichts fehlte.
[[nodiscard]] constexpr std::optional<HardwareProbeErrorClass> stage_reason(RamProbeTrail const&   t,
                                                                            RamFrequencyProvenance s) noexcept {
    auto const i = static_cast<std::size_t>(s);
    if (t.outcome[i] != ProbeStageOutcome::Durchgefallen) return std::nullopt;
    return t.reason[i];
}

constexpr void record_stage_failure(RamProbeTrail& t, RamFrequencyProvenance s, HardwareProbeErrorClass c) noexcept {
    auto const i = static_cast<std::size_t>(s);
    t.outcome[i] = ProbeStageOutcome::Durchgefallen;
    t.reason[i]  = c;
}

constexpr void record_stage_success(RamProbeTrail& t, RamFrequencyProvenance s) noexcept {
    t.outcome[static_cast<std::size_t>(s)] = ProbeStageOutcome::Geliefert;
}

// -- Der Ergebnis-Typ ------------------------------------------------------------------------------
/// Das Erhebungs-Ergebnis EINER Maschine. Default-konstruiert ist es ehrlich leer (NichtErhoben, 0),
/// nicht etwa "0 MT/s deklariert" -- dieselbe Ehrlichkeits-Regel wie die 0-Marke in kDeclaredMachines.
///
/// P2-ZUWACHS (additiv ans Ende, damit die bestehenden Aggregat-Initialisierungen gueltig bleiben):
/// der Degradations-Pfad und der Erhebungs-Zeitpunkt. Beide fuellt die Kette; der Parser-Weg laesst
/// sie leer, weil er weder eine Kette noch eine Uhr kennt.
struct RamFrequencyReading {
    std::uint32_t          mts                = 0; ///< MT/s; NUR gueltig bei state==Erhoben
    std::uint32_t          tck_ps             = 0; ///< SPD-Rohwert tCKAVGmin (ps), Ableitungs-Beleg
    std::uint32_t          xmp_expo_offer_mts = 0; ///< ANGEBOT der Riegel (XMP/EXPO), NIE der Ist-Wert
    RamFrequencyProvenance provenance         = RamFrequencyProvenance::DeclaredNotMeasured;
    RamReadingState        state              = RamReadingState::NichtErhoben;
    RamProbeTrail          trail{}; ///< welche Stufe lieferte, welche fiel warum durch
    /// Unix-Sekunden der Erhebung; 0 = nicht erhoben. GOLDEN-REGEL (A10): dieser Wert ist der einzige
    /// im Typ, der sich zwischen zwei identischen Laeufen aendert -- er gehoert deshalb NIE in ein
    /// byte-gegatetes Artefakt (Stempel, binary_id, Registry-XML), sondern ausschliesslich in Log/CSV.
    std::uint64_t collected_at_unix_s = 0;
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

// -- P2-Wachen: Degradations-Pfad ------------------------------------------------------------------
// Der Zuwachs darf die POD-Form nicht kippen (die Kette gibt das Ergebnis by value zurueck) und die
// Ehrlichkeits-Aussagen oben nicht verschieben.
static_assert(std::is_trivially_copyable_v<RamProbeTrail>);
static_assert(std::is_trivially_copyable_v<RamFrequencyReading>, "P2-Zuwachs haelt die POD-Form.");
static_assert(std::is_aggregate_v<RamFrequencyReading>, "P2-Zuwachs haelt die Aggregat-Form.");
static_assert(!has_frequency(RamFrequencyReading{}), "Der Zuwachs aendert den Leer-Default NICHT.");
static_assert(kProbeStageOutcomeCount == static_cast<std::size_t>(ProbeStageOutcome::Durchgefallen) + 1);
static_assert(probe_stage_outcome_token(static_cast<ProbeStageOutcome>(kProbeStageOutcomeCount)) ==
                  std::string_view{"stufen_ausgang_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettierter Stufen-Ausgang");
static_assert(probe_label_ist_disjunkt(probe_stage_outcome_token(ProbeStageOutcome::NichtGefragt)));
static_assert(probe_label_ist_disjunkt(probe_stage_outcome_token(ProbeStageOutcome::Geliefert)));
static_assert(probe_label_ist_disjunkt(probe_stage_outcome_token(ProbeStageOutcome::Durchgefallen)));
// Der Pfad ist ueber die PROVENIENZ indiziert -- waechst das Stufen-Ranking, waechst er mit. Diese
// Zeile ist die Naht zwischen beiden: faellt sie, zeigt ein Pfad-Eintrag auf die falsche Stufe.
static_assert(RamProbeTrail{}.outcome.size() == kRamFrequencyProvenanceCount);
static_assert(RamProbeTrail{}.reason.size() == kRamFrequencyProvenanceCount);
// Der Leer-Pfad behauptet nichts: keine Stufe gefragt, kein Grund lesbar.
static_assert(stage_outcome(RamProbeTrail{}, RamFrequencyProvenance::ConfiguredMeasured) ==
              ProbeStageOutcome::NichtGefragt);
static_assert(!stage_reason(RamProbeTrail{}, RamFrequencyProvenance::ConfiguredMeasured).has_value());
// A4 als Wache: ein durchgefallener Grund ist lesbar UND unterscheidet fehlend von korrupt; eine
// gelieferte Stufe hat ueberhaupt keinen Grund (nicht etwa den Default QuelleFehlt).
static_assert(
    [] {
        RamProbeTrail t{};
        record_stage_failure(t, RamFrequencyProvenance::ConfiguredMeasured, HardwareProbeErrorClass::QuelleFehlt);
        record_stage_failure(t, RamFrequencyProvenance::SpdJedecBase, HardwareProbeErrorClass::QuelleKorrupt);
        record_stage_success(t, RamFrequencyProvenance::DeclaredNotMeasured);
        return stage_reason(t, RamFrequencyProvenance::ConfiguredMeasured) == HardwareProbeErrorClass::QuelleFehlt &&
               stage_reason(t, RamFrequencyProvenance::SpdJedecBase) == HardwareProbeErrorClass::QuelleKorrupt &&
               stage_outcome(t, RamFrequencyProvenance::DeclaredNotMeasured) == ProbeStageOutcome::Geliefert &&
               !stage_reason(t, RamFrequencyProvenance::DeclaredNotMeasured).has_value();
    }(),
    "Der Degradations-Pfad muss fehlend von korrupt trennen und darf einer gelieferten Stufe keinen "
    "Grund andichten (Auflage A4).");
// Der Erhebungs-Zeitpunkt ist im Leer-Ergebnis 0 -- ein default-konstruiertes Reading behauptet also
// auch keinen Zeitpunkt. Damit bleibt der Typ in constexpr-Kontexten vollstaendig zeit-frei.
static_assert(RamFrequencyReading{}.collected_at_unix_s == 0);

} // namespace comdare::cache_engine::measurement
