#pragma once
// measurement/ram_identity_verdict.hpp -- die Drift-Gegenprobe der RAM-Seite (Task #7, Paket P2).
//
// WAS HIER STEHT: das Urteil darueber, ob eine ERHOBENE RAM-Frequenz und die DEKLARIERTE zueinander
// passen. Vorbild ist verify_declared_cpu (machine_identity.hpp, O-4) -- dieselbe Bauform: benannte
// Ausgaenge, kein stiller Fallback, und eine getrennte Abbildung auf die Fehler-Taxonomie, damit ein
// definierter Zustand nicht als Fehler durchgeht.
//
// -- WARUM DIE RAM-SEITE SCHWIERIGER IST ALS DIE CPU-SEITE ---------------------------------------
// Bei der CPU vergleicht O-4 zwei Angaben DERSELBEN Art: CPUID-Tupel gegen CPUID-Tupel. Bei der RAM-
// Frequenz sind es je nach Herkunft VERSCHIEDENE GROESSEN, die zufaellig dieselbe Einheit tragen:
//   - configured_measured ist der real laufende Takt (SMBIOS "Configured Memory Speed"),
//   - spd_jedec_base ist die NENNRATE der Riegel (JEDEC-Datenblatt-Wert).
// Auf prod1 stehen diese beiden belegt AUSEINANDER: SPD sagt 4800 (JEDEC-Basis), das XMP/EXPO-Profil
// bietet 5600 -- welcher Wert real laeuft, ist ohne Stufe 1 unbekannt. Ein Vergleich, der das
// ignoriert, meldet entweder ein Match, das keines ist, oder eine Abweichung, die keine ist. Auflage
// A3 verlangt deshalb eine PROVENIENZ-GLEICHHEITSREGEL, und sie steht unten als eigene Funktion.
//
// -- DIE DEKLARATION IST STUFE 3, IHR URSPRUNG IST EINE NOTIZ (A2) -------------------------------
// Der deklarierte prod2-Wert 4800 stammt nachweislich aus einem `dmidecode --type 17`-Lauf
// (machine_identity.hpp) -- er IST ein eingefrorenes Stufe-1-Ergebnis. Trotzdem wird er NICHT als
// configured_measured gefuehrt: A2 haelt fest, dass der Ursprung eine NOTIZ bleibt und keine Stufe
// wird, weil eine Stufe eine Aussage ueber die LAUFENDE Erhebung ist. Die Notiz wird hier als eigener
// Typ gefuehrt (DeclarationOrigin) und ist genau die Angabe, die A3 fuer ihr Urteil braucht.
// P3 fuellt sie aus dem Generator; bis dahin ist sie Unbekannt, und dieser Header behandelt das als
// benannten Fall, nicht als Luecke.
//
// -- GOLDEN-NEUTRALITAET -------------------------------------------------------------------------
// Ein Urteil ist eine Log-/CSV-Aussage. Es gibt hier keine Funktion, die es an eine Stempel-, Achsen-
// oder binary_id-API reicht, und keinen Weg, aus einem Verdikt eine Achsen-Auspraegung zu machen.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/ram_frequency_reading.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. DER URSPRUNG EINER DEKLARATIONS-ZAHL (A2-Notiz, KEINE Stufe)
// =================================================================================================

/// Woher die deklarierte Zahl einmal kam. Das ist eine Herkunfts-NOTIZ ueber einen eingefrorenen
/// Vorgang, keine Provenienz-Stufe der laufenden Erhebung -- deshalb ein eigener Typ und nicht ein
/// vierter Wert in RamFrequencyProvenance (dieselbe Trennung, die P1 fuer "nicht erhoben" gezogen hat).
enum class DeclarationOrigin : std::uint8_t {
    Unbekannt        = 0, ///< keine Notiz -- die Zahl ist schlicht der deklarierte Wert
    EingefrorenesDmi = 1, ///< aus einem dmidecode-T17-Lauf (Groesse: konfigurierter Ist-Takt)
    EingefrorenesSpd = 2, ///< aus einem SPD-Lesen (Groesse: JEDEC-Nennrate)
};
inline constexpr std::size_t kDeclarationOriginCount = 3;

[[nodiscard]] constexpr std::string_view declaration_origin_token(DeclarationOrigin o) noexcept {
    switch (o) {
        case DeclarationOrigin::Unbekannt: return "ursprung_unbekannt";
        case DeclarationOrigin::EingefrorenesDmi: return "eingefrorenes_dmi";
        case DeclarationOrigin::EingefrorenesSpd: return "eingefrorenes_spd";
    }
    return "deklarations_ursprung_unbekannt";
}

// =================================================================================================
// 2. DAS VERDIKT
// =================================================================================================

/// Ergebnis der RAM-Drift-Gegenprobe. Jeder Wert ist ein eigener, benannter Ausgang -- es gibt keinen
/// Zweig, der eine ungeprueft passende Zahl als bestaetigt ausgibt.
///
/// WARUM SIEBEN UND NICHT VIER WIE BEI DER CPU: die CPU-Seite kennt keinen Fall, in dem die beiden
/// Angaben verschiedene GROESSEN sind, und keinen, in dem ein zweites Etikett der Zahl widersprechen
/// kann. Beides gibt es hier, und beides zu einem der uebrigen Ausgaenge zusammenzuziehen hiesse, eine
/// Aussage zu treffen, die nicht gedeckt ist.
enum class RamIdentityVerdict : std::uint8_t {
    Match                 = 0, ///< erhoben und deklariert stimmen ueberein -- und waren vergleichbar
    Abweichung            = 1, ///< erhoben und deklariert widersprechen sich -> Fehlerklasse
    NurDeklariert         = 2, ///< die Kette kam nicht ueber Stufe 3 -- es gibt keine unabhaengige Erhebung
    NichtErhoben          = 3, ///< die Kette lieferte GAR nichts (auch die Deklaration nicht)
    KeineDeklaration      = 4, ///< kein Eigenschafts-Tupel getroffen oder 0-Marke -- nichts zu vergleichen
    UnvergleichbareStufen = 5, ///< A3: erhoben und deklariert sind verschiedene GROESSEN
    EtikettWiderspruch    = 6, ///< das ram_pair-Etikett widerspricht der Hardware -- benannter NICHT-Fehler
};
inline constexpr std::size_t kRamIdentityVerdictCount = 7;

[[nodiscard]] constexpr std::string_view ram_identity_verdict_label(RamIdentityVerdict v) noexcept {
    switch (v) {
        case RamIdentityVerdict::Match: return "ram_match";
        case RamIdentityVerdict::Abweichung: return "ram_abweichung";
        case RamIdentityVerdict::NurDeklariert: return "ram_nur_deklariert";
        case RamIdentityVerdict::NichtErhoben: return "ram_nicht_erhoben";
        case RamIdentityVerdict::KeineDeklaration: return "ram_keine_deklaration";
        case RamIdentityVerdict::UnvergleichbareStufen: return "ram_unvergleichbare_stufen";
        case RamIdentityVerdict::EtikettWiderspruch: return "ram_etikett_widerspruch";
    }
    return "ram_verdikt_unbekannt";
}

// =================================================================================================
// 3. DIE ZWEI REGELN, DIE DAS URTEIL TRAGEN
// =================================================================================================

/// AUFLAGE A3 -- die Provenienz-Gleichheitsregel. Darf ein erhobener Wert DIESER Stufe gegen eine
/// Deklaration DIESES Ursprungs ueberhaupt als Match/Abweichung geurteilt werden?
///
/// Die Regel ist keine Vorsichtsmassnahme, sondern eine Groessen-Aussage: eine JEDEC-Nennrate gegen
/// einen eingefrorenen Ist-Takt zu halten vergleicht Datenblatt mit Betriebszustand. Fallen sie
/// zusammen, beweist das nichts (die meisten Systeme laufen auf JEDEC-Basis); fallen sie auseinander,
/// ist das kein Fehler, sondern ein aktives XMP/EXPO-Profil.
///
/// UNBEKANNTER URSPRUNG IST VERGLEICHBAR, und das ist eine bewusste Entscheidung: ohne Notiz macht die
/// Deklaration ueberhaupt keine Groessen-Aussage -- sie ist dann schlicht die Zahl, die jemand
/// deklariert hat, und gegen die darf jede Erhebung antreten. Die Gegenposition (ohne Notiz gar nicht
/// urteilen) haette zur Folge, dass die Funktion bis P3 nie ein Urteil faellt, also nie etwas faengt.
[[nodiscard]] constexpr bool provenienz_vergleichbar(RamFrequencyProvenance erhoben,
                                                     DeclarationOrigin      ursprung) noexcept {
    switch (ursprung) {
        case DeclarationOrigin::Unbekannt: return true;
        case DeclarationOrigin::EingefrorenesDmi: return erhoben == RamFrequencyProvenance::ConfiguredMeasured;
        case DeclarationOrigin::EingefrorenesSpd: return erhoben == RamFrequencyProvenance::SpdJedecBase;
    }
    return false; // unbekannter Ursprung-Wert -> lieber nicht urteilen als falsch urteilen
}

/// Die Speicher-Generation, die ein gelungener SPD-Parse BELEGT. Der P1-Parser nimmt ausschliesslich
/// den DDR5-Key an (alles andere faellt als FormatUnbekannt heraus) -- ein Stufe-2-Ergebnis ist also
/// zugleich ein Nachweis der Generation, ohne dass ein zusaetzliches Feld noetig waere.
inline constexpr std::string_view kSpdBelegteGeneration = "ddr5";
/// Das Praefix, an dem ein ram_pair-Etikett ueberhaupt eine Generation nennt ("ddr5_2x32", "ddr4_2x32").
inline constexpr std::string_view kRamPairGenerationsPraefix = "ddr";

/// Widerspricht das ram_pair-Etikett der Deklaration dem, was die Erhebung belegt hat?
///
/// Heute pruefbar in genau EINE Richtung, und das steht hier statt einer allgemeineren Behauptung: ein
/// erfolgreicher SPD-Parse belegt DDR5. Nennt das Etikett eine ANDERE ddr-Generation, widersprechen
/// sich Deklaration und Hardware. Die Gegenrichtung (Etikett sagt ddr5, Hardware ist ddr4) laesst sich
/// so nicht zeigen, weil ein DDR4-Riegel gar nicht erst durch den Parser kaeme -- die Stufe fiele mit
/// FormatUnbekannt durch, und das ist ein Ketten-Befund, kein Etikett-Urteil.
///
/// Ein Etikett OHNE Generations-Praefix wird NICHT beurteilt: es macht keine Aussage, der man
/// widersprechen koennte. Das Schweigen als Widerspruch zu lesen waere geraten.
[[nodiscard]] constexpr bool ram_pair_etikett_widerspricht(std::string_view           ram_pair,
                                                           RamFrequencyReading const& r) noexcept {
    if (r.provenance != RamFrequencyProvenance::SpdJedecBase) return false; // nur SPD belegt die Generation
    if (!ram_pair.starts_with(kRamPairGenerationsPraefix)) return false;    // keine Aussage -> kein Widerspruch
    return !ram_pair.starts_with(kSpdBelegteGeneration);
}

// =================================================================================================
// 4. DAS URTEIL
// =================================================================================================

/// Die Drift-Gegenprobe der RAM-Seite.
///
/// DIE REIHENFOLGE DER PRUEFUNGEN IST DIE AUSSAGE:
///  (1) Ohne Erhebung gibt es nichts zu vergleichen -- und zwar unabhaengig davon, was deklariert ist.
///  (2) Ohne Deklaration ebenso. Die 0 ist dabei die NICHT-DEKLARIERT-Marke, kein Wert von 0 MT/s.
///  (3) Das ETIKETT vor der ZAHL: widersprechen sich Deklaration und Hardware in der GENERATION, ist
///      der Zahlen-Vergleich sinnlos -- zwei Zahlen aus verschiedenen Speicher-Generationen sagen
///      nichts ueber einander, auch wenn sie gleich sind.
///  (4) Kam der Wert AUS der Deklaration (Stufe 3), ist er ihr trivialerweise gleich. Das als "Match"
///      zu melden waere eine Selbstbestaetigung: die Deklaration bestaetigt sich selbst. Genau dafuer
///      gibt es NurDeklariert.
///  (5) Erst jetzt A3 -- und erst danach die Zahl.
[[nodiscard]] constexpr RamIdentityVerdict verify_declared_ram(DeclaredMachine const*     declared,
                                                               RamFrequencyReading const& reading,
                                                               DeclarationOrigin          origin) noexcept {
    if (!has_frequency(reading)) return RamIdentityVerdict::NichtErhoben;
    if (declared == nullptr || declared->ram_frequency_mhz == 0U) return RamIdentityVerdict::KeineDeklaration;
    if (ram_pair_etikett_widerspricht(declared->ram_pair, reading)) return RamIdentityVerdict::EtikettWiderspruch;
    if (reading.provenance == RamFrequencyProvenance::DeclaredNotMeasured) return RamIdentityVerdict::NurDeklariert;
    if (!provenienz_vergleichbar(reading.provenance, origin)) return RamIdentityVerdict::UnvergleichbareStufen;
    return reading.mts == declared->ram_frequency_mhz ? RamIdentityVerdict::Match : RamIdentityVerdict::Abweichung;
}

/// Abbildung auf die BESTEHENDE D1-Taxonomie -- ohne neue Klasse (Owner-Entscheid E-4).
///
/// DIE DEHNUNG, AUSDRUECKLICH BENANNT: HardwareErweiterungFehlt beschreibt woertlich eine fehlende
/// ISA-/Beschleuniger-Erweiterung. Eine RAM-Frequenz ist keine Erweiterung; die Klasse wird hier also
/// GEDEHNT auf "der Host ist nicht die Maschine, als die er deklariert ist". Das ist keine
/// Verlegenheit, sondern die dokumentierte Praezedenz: verify_declared_cpu (machine_identity.hpp)
/// nutzt fuer genau dieselbe Aussage bereits dieselbe Klasse. Eine eigene Klasse waere eine sechste in
/// einem FREMDEN Enum und zoege dessen Count-Wachen (RF-3-Kollisionslehre) -- die HardwareProbe-Domaene
/// aus P1 traegt die ERHEBUNGS-Fehler, nicht das Identitaets-Urteil.
///
/// ALLE UEBRIGEN SIND KEINE FEHLER, und das ist der Punkt der Funktion: NurDeklariert, NichtErhoben,
/// KeineDeklaration, UnvergleichbareStufen und insbesondere EtikettWiderspruch sind DEFINIERTE
/// Zustaende. Sie liefern nullopt -- was ausdruecklich nicht "wird ignoriert" heisst: der Aufrufer
/// bekommt das benannte Urteil und muss es ausweisen.
[[nodiscard]] constexpr std::optional<CompilerCompilerErrorClass>
error_class_of_ram_verdict(RamIdentityVerdict v) noexcept {
    if (v == RamIdentityVerdict::Abweichung) return CompilerCompilerErrorClass::HardwareErweiterungFehlt;
    return std::nullopt;
}

// =================================================================================================
// 5. WACHEN (compile-time)
// =================================================================================================
static_assert(kDeclarationOriginCount == static_cast<std::size_t>(DeclarationOrigin::EingefrorenesSpd) + 1);
static_assert(declaration_origin_token(static_cast<DeclarationOrigin>(kDeclarationOriginCount)) ==
                  std::string_view{"deklarations_ursprung_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettierter Deklarations-Ursprung");
static_assert(kRamIdentityVerdictCount == static_cast<std::size_t>(RamIdentityVerdict::EtikettWiderspruch) + 1);
static_assert(ram_identity_verdict_label(static_cast<RamIdentityVerdict>(kRamIdentityVerdictCount)) ==
                  std::string_view{"ram_verdikt_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettiertes Verdikt");
// Die Etiketten sind gegen die bestehenden Vokabeln disjunkt UND gegen die der CPU-Seite: "match" ist
// dort bereits vergeben, und zwei Spalten nebeneinander mit demselben Wort waeren nicht lesbar.
static_assert(probe_label_ist_disjunkt(ram_identity_verdict_label(RamIdentityVerdict::Match)));
static_assert(probe_label_ist_disjunkt(ram_identity_verdict_label(RamIdentityVerdict::Abweichung)));
static_assert(probe_label_ist_disjunkt(declaration_origin_token(DeclarationOrigin::EingefrorenesDmi)));
static_assert(ram_identity_verdict_label(RamIdentityVerdict::Match) !=
                  machine_identity_verdict_label(MachineIdentityVerdict::Match),
              "RAM- und CPU-Verdikt stehen als eigene Spalten nebeneinander -- gleiche Worte waeren "
              "in der Auswertung nicht auseinanderzuhalten.");
static_assert(ram_identity_verdict_label(RamIdentityVerdict::Abweichung) !=
              machine_identity_verdict_label(MachineIdentityVerdict::Abweichung));

// -- A3 als Wache, nicht als Kommentar --
// Die tragende Zeile des ganzen Headers: eine SPD-Nennrate gegen einen eingefrorenen dmidecode-Wert
// ist KEIN vergleichbares Paar, auch wenn beide "4800" lauten.
static_assert(!provenienz_vergleichbar(RamFrequencyProvenance::SpdJedecBase, DeclarationOrigin::EingefrorenesDmi),
              "A3: spd_jedec_base darf NIE gegen einen configured-Ursprung als Match/Abweichung "
              "geurteilt werden -- das sind verschiedene Groessen.");
static_assert(provenienz_vergleichbar(RamFrequencyProvenance::ConfiguredMeasured, DeclarationOrigin::EingefrorenesDmi),
              "Gleiche Groesse gegen gleiche Groesse ist der Fall, fuer den A3 den Vergleich freigibt.");
static_assert(provenienz_vergleichbar(RamFrequencyProvenance::SpdJedecBase, DeclarationOrigin::EingefrorenesSpd));
static_assert(provenienz_vergleichbar(RamFrequencyProvenance::SpdJedecBase, DeclarationOrigin::Unbekannt) &&
                  provenienz_vergleichbar(RamFrequencyProvenance::ConfiguredMeasured, DeclarationOrigin::Unbekannt),
              "Ohne Notiz macht die Deklaration keine Groessen-Aussage -- dann darf jede Erhebung "
              "antreten (bewusste Wahl, siehe provenienz_vergleichbar).");

// -- Die Ausgaenge, jeder einmal belegt (ein unerreichbarer Ausgang waere toter Code) --
// Der prod2-Fall ist der einzige mit einer deklarierten Zahl (4800) und damit die Referenz-Zeile.
inline constexpr DeclaredMachine const* kVerdictProbeProd2 = &kDeclaredMachines[1];
inline constexpr DeclaredMachine const* kVerdictProbeProd1 = &kDeclaredMachines[0];

static_assert(verify_declared_ram(kVerdictProbeProd2, RamFrequencyReading{}, DeclarationOrigin::Unbekannt) ==
                  RamIdentityVerdict::NichtErhoben,
              "Ohne Erhebung wird nicht geurteilt -- auch nicht gegen eine vorhandene Deklaration.");
static_assert(verify_declared_ram(nullptr,
                                  RamFrequencyReading{.mts        = 4800,
                                                      .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::Unbekannt) == RamIdentityVerdict::KeineDeklaration);
// prod1 traegt die 0-Marke: sie ist NICHT deklariert, nicht etwa 0 MT/s. Ohne diese Zeile koennte
// jemand die Marke als Zahl vergleichen und bekaeme fuer jede echte Erhebung eine "Abweichung".
static_assert(verify_declared_ram(kVerdictProbeProd1,
                                  RamFrequencyReading{.mts        = 4800,
                                                      .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::Unbekannt) == RamIdentityVerdict::KeineDeklaration,
              "Die 0 in kDeclaredMachines ist die NICHT-DEKLARIERT-Marke und darf nie als Wert in "
              "einen Vergleich geraten.");
// Stufe 3 gegen die Deklaration: der Wert KAM von dort. Ein Match waere eine Selbstbestaetigung.
static_assert(verify_declared_ram(kVerdictProbeProd2,
                                  RamFrequencyReading{.mts        = 4800,
                                                      .provenance = RamFrequencyProvenance::DeclaredNotMeasured,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::Unbekannt) == RamIdentityVerdict::NurDeklariert,
              "Die Deklaration darf sich nicht selbst bestaetigen.");
// Echter Vergleich, echte Zahlen: Match und Abweichung sind beide erreichbar.
static_assert(verify_declared_ram(kVerdictProbeProd2,
                                  RamFrequencyReading{.mts        = 4800,
                                                      .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::Unbekannt) == RamIdentityVerdict::Match);
static_assert(verify_declared_ram(kVerdictProbeProd2,
                                  RamFrequencyReading{.mts        = 5600,
                                                      .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::Unbekannt) == RamIdentityVerdict::Abweichung);
// A3 im vollen Urteil: DIESELBEN Zahlen wie das Match darueber, nur mit der Ursprungs-Notiz -- und
// das Urteil kippt von Match auf UnvergleichbareStufen. Das ist der Beleg, dass die Regel wirkt und
// nicht bloss danebensteht.
static_assert(verify_declared_ram(kVerdictProbeProd2,
                                  RamFrequencyReading{.mts        = 4800,
                                                      .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                      .state      = RamReadingState::Erhoben},
                                  DeclarationOrigin::EingefrorenesDmi) == RamIdentityVerdict::UnvergleichbareStufen,
              "A3 im Urteil: dieselben Zahlen, aber ungleiche Groessen -- kein Match.");
// EtikettWiderspruch: ein SPD-Beleg (= DDR5) gegen ein ddr4-Etikett. Der Fall ist seit dem
// E-2-Schluesselfix auf prod2 nicht mehr auslesbar, der ZUSTAND bleibt aber noetig -- er ist der
// benannte Nicht-Fehler, in dem die halbe Identitaet der Hardware widerspricht.
static_assert(ram_pair_etikett_widerspricht("ddr4_2x32",
                                            RamFrequencyReading{.mts        = 4800,
                                                                .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                                .state      = RamReadingState::Erhoben}));
static_assert(!ram_pair_etikett_widerspricht("ddr5_2x32",
                                             RamFrequencyReading{.mts        = 4800,
                                                                 .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                                 .state      = RamReadingState::Erhoben}),
              "Der heutige prod2-Schluessel (nach E-2) widerspricht NICHT.");
// Ein Etikett ohne Generations-Angabe schweigt -- Schweigen ist kein Widerspruch.
static_assert(!ram_pair_etikett_widerspricht("unbenannt_2x32",
                                             RamFrequencyReading{.mts        = 4800,
                                                                 .provenance = RamFrequencyProvenance::SpdJedecBase,
                                                                 .state      = RamReadingState::Erhoben}));
// Und nur ein SPD-Beleg begruendet ueberhaupt eine Generations-Aussage: ein Deklarations-Wert belegt
// nichts ueber die Hardware und darf dem Etikett deshalb nicht widersprechen.
static_assert(!ram_pair_etikett_widerspricht("ddr4_2x32", RamFrequencyReading{
                                                              .mts        = 4800,
                                                              .provenance = RamFrequencyProvenance::DeclaredNotMeasured,
                                                              .state      = RamReadingState::Erhoben}));

// -- Fehlerklassen-Abbildung: genau EIN Fehler, alles andere definierter Zustand --
static_assert(error_class_of_ram_verdict(RamIdentityVerdict::Abweichung) ==
              CompilerCompilerErrorClass::HardwareErweiterungFehlt);
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::Match).has_value());
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::NurDeklariert).has_value());
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::NichtErhoben).has_value());
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::KeineDeklaration).has_value());
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::UnvergleichbareStufen).has_value());
static_assert(!error_class_of_ram_verdict(RamIdentityVerdict::EtikettWiderspruch).has_value(),
              "EtikettWiderspruch ist ein benannter NICHT-Fehler-Zustand (Owner-Entscheid E-2): er "
              "wird ausgewiesen, hat aber keine Dispatch-Wirkung.");
// Die D1-Taxonomie wird NICHT erweitert -- dieselbe Wache wie in machine_identity.hpp, damit ein
// spaeterer Bump auch hier vorbeikommt.
static_assert(kCompilerCompilerErrorClassCount == 5,
              "Fehlerklassen-Zahl driftet. P2 erweitert die D1-Taxonomie NICHT: die RAM-Drift dehnt "
              "die bestehende Klasse (Owner-Entscheid E-4), sie fuegt keine hinzu.");

} // namespace comdare::cache_engine::measurement
