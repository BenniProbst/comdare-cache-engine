// tests/unit/test_w10_system_cell_values.cpp -- W10-C1/C2-ORAKEL (Bauplan-Dossier 20260803, Sektion 2).
//
// == W10-C2: DER TEST-DEFINE STEHT BEWUSST GANZ OBEN ==============================================
// Die Zellwert-Naht ist eine #ifndef-PRAEZEDENZ (Muster COMDARE_OVERLAY_SOURCE_HASH,
// anatomy_fingerprint.hpp): eine Vor-Definition gewinnt gegen den Default "". Diese TU setzt sie
// deshalb VOR jedem Include und uebersetzt damit die Makro-Naht COMDARE_ANATOMY_VERSION_STAMP_M
// SCHARF -- das ist das C2-Orakel (iv), und zwar am ECHTEN Makro statt an einem nachgebauten POD.
// (Der Weg ueber target_compile_definitions waere derselbe Beweis, braeuchte aber ein
// Semikolon-Listen-Escaping in CMake; die Praezedenz im Header ist dafuer ausdruecklich gebaut.)
// WICHTIG: alle IDENTITAETS-Aussagen dieser TU laufen deshalb ueber ein EXPLIZIT leeres
// SystemCellValues{} und nie ueber den Define -- sonst behauptete die TU Identitaet, waehrend sie
// mit gesetzten Werten uebersetzt.
#define COMDARE_SYSTEM_CELL_VALUES "target_isa=x86_64;operating_system=linux;simd=avx512"

// Was diese TU beweist -- und zwar literal, nicht behauptend:
//   (1) POSITIV (CT): die vervollstaendigte System-Zeile ist byte-genau die Ziel-Zeile des Dossiers.
//   (2) NEGATIV (CT): jede Fehlform der Define-Wertform hat ihre EIGENE, benannte Diagnose -- unbekannter
//       Schluessel, Grossbuchstabe, leerer Token, RT-Unter-Achsen-Schluessel (A-15), doppelter Schluessel,
//       Teil-Belegung. Sie sind static_asserts, also compile-hart bewiesen statt zur Laufzeit gehofft.
//   (3) IDENTITAET: ein LEERES Werte-Set laesst jede Zeile BYTE-IDENTISCH -- das ist der golden-neutrale
//       Grundpfad, an dem W10a haengt. Geprueft an der LEBENDEN cea::system_stamp_line(), nicht an einem
//       eingefrorenen Literal (Lehre "gruene Tests zementieren alte Ordnung": ein gepinnter Vor-W10-
//       Stempel-String waere genau die Fixture-Zementierung, die die W10-Auflage verbietet).
//   (4) GRAMMATIK-HAERTUNG: die vervollstaendigte Zeile besteht den vollen consteval-Weg
//       (stamp_line_is_parsable, F1-F6 + Z-09), traegt DIESELBE Eintrags-Zahl, DIESELBEN Ebenen und einen
//       BYTE-UNBERUEHRTEN Versionsteil. Der Meta-Meta-Anhang bleibt am Realm-Zeilen-ENDE (Stempel-KERN).
//   (5) DRIFT-WACHE gegen den Meta-Meta-Hub: der dritte Zellwert-Schluessel IST der Stempel-Name des
//       einzigen Glieds von ExternalUtilsHub::meta_metas. Diese Haelfte der Wache sitzt HIER und nicht im
//       Header, weil der Header von JEDER emittierten Modul-Quelle inkludiert wird und deshalb keine
//       measurement-Achsen-Typen ziehen darf (Hermetik); die Achsen-Ordnungs-Haelfte steht im Header.
//
// A-15 (tragend): die TU prueft zusaetzlich am LEBENDEN Ist, dass kein erhobener RT-Wert in der Zeile
// steht -- dieselbe Beweisform wie test_os_u3_probe/test_od10_numa_page_probe.

#include <cache_engine/abi/anatomy_module_abi_v1.hpp> // W10-C2: die MAKRO-NAHT selbst (kS/kFP/kSE)
#include <cache_engine/abi/anatomy_stamp_entries.hpp>
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_order.hpp>
#include <cache_engine/abi/system_cell_values.hpp>
#include <cache_engine/measurement/external_utils_family_axis.hpp>
#include <cache_engine/measurement/operating_system_sub_axes.hpp>

#include "bestandslog/bestandslog_factory.hpp" // W10-C3: der Laufzeit-Zwilling am Lager-Key

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cea = ::comdare::cache_engine::abi;
namespace cem = ::comdare::cache_engine::measurement;

using cea::SystemCellValues;
using cea::SystemCellValuesDiagnose;

namespace {

// Die REALE System-Zeilen-Form (A13-M3-Ist): drei Haupt-Achsen + der Meta-Meta-Klammer-Anhang am ENDE.
// Sie steht hier als Literal, weil der consteval-Weg ein Literal braucht; ihre UEBEREINSTIMMUNG mit dem
// lebenden cea::system_stamp_line() wird unten zur Laufzeit geprueft -- kein blinder Pin.
constexpr std::string_view kSystemZeileRoh =
    "target_isa=code@1.0.0c;operating_system=code@1.0.0c;external_utils=code@1.0.0c;[simd=code@1.0.0c]";

// Das Beispiel-Werte-Set (prod1-Zelle) in der Define-Wertform.
constexpr SystemCellValues kWerteProd1{"target_isa=x86_64;operating_system=linux;simd=avx512"};

// Die ZIEL-ZEILE des Dossiers (E-1-Default (a)): der Zellwert als hierarchische Namens-Erweiterung des
// Algorithmus-Markers "code" -> "code.<token>". Achsen-Namen und Versionsteil unangetastet.
constexpr std::string_view kSystemZeileZiel = "target_isa=code.x86_64@1.0.0c;operating_system=code.linux@1.0.0c;"
                                              "external_utils=code@1.0.0c;[simd=code.avx512@1.0.0c]";

constexpr auto kFertig =
    cea::complete_system_stamp_line_array<cea::complete_system_stamp_line_size(kSystemZeileRoh, kWerteProd1)>(
        kSystemZeileRoh, kWerteProd1);

// -- W10-C2: die Literale, mit denen die MAKRO-Naht unten instanziiert wird -----------------------
// Bewusst KURZE, synthetische Organ-/Mess-Zeilen (Praezedenz test_m_w12 A4-POD-Test): geprueft wird
// die Naht, nicht die Welt. Die SYSTEM-Zeile ist dagegen die reale Form -- sie IST der Prueflings-
// gegenstand und wird oben gegen das lebende cea::system_stamp_line() abgeglichen.
#define COMDARE_W10_TEST_ORGAN_LIT "search_algo=k_ary@1.0.0c;filter=bloom@2.3.4c"
#define COMDARE_W10_TEST_SYSTEM_LIT                                                                                    \
    "target_isa=code@1.0.0c;operating_system=code@1.0.0c;external_utils=code@1.0.0c;[simd=code@1.0.0c]"
#define COMDARE_W10_TEST_MEASURE_LIT "measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]"

constexpr std::string_view kOrganLit   = COMDARE_W10_TEST_ORGAN_LIT;
constexpr std::string_view kMeasureLit = COMDARE_W10_TEST_MEASURE_LIT;

} // namespace

// W10-C2-ORAKEL (iv): die ECHTE Makro-Expansion. Sie materialisiert das optionale Probe-Symbol
// comdare_anatomy_version_lines() -- genau so, wie es jede emittierte Modul-Quelle tut -- und zwar in
// dieser TU MIT gesetztem Zellwert-Define. Was hier gruen ist, ist an der realen Naht gruen.
COMDARE_ANATOMY_VERSION_STAMP_M(COMDARE_W10_TEST_ORGAN_LIT, COMDARE_W10_TEST_SYSTEM_LIT, COMDARE_W10_TEST_MEASURE_LIT)

// =================================================================================================
// (1) POSITIV -- die Vervollstaendigung trifft die Ziel-Zeile byte-genau (CT UND als ctest-Aussage)
// =================================================================================================
TEST(W10SystemCellValues, VervollstaendigteZeileIstDieZielZeile) {
    static_assert(kFertig.view() == kSystemZeileZiel,
                  "W10-C1: die vervollstaendigte System-Zeile weicht von der Ziel-Form des Bauplans ab "
                  "(target_isa=code.<isa>@X.Y.Zc;operating_system=code.<fam>@X.Y.Zc;external_utils=code@X.Y.Zc;"
                  "[simd=code.<simd>@X.Y.Zc]).");
    EXPECT_EQ(std::string{kFertig.view()}, std::string{kSystemZeileZiel});

    // Die LAUFZEIT-Form ist der Zwilling der consteval-Form -- eine Ordnung, zwei Senken.
    EXPECT_EQ(cea::complete_system_stamp_line(kSystemZeileRoh, kWerteProd1), std::string{kSystemZeileZiel});

    // external_utils bleibt WERTFREI (E-2-Default (a)): der Hub hat keine eigene Zelle.
    EXPECT_NE(std::string{kFertig.view()}.find("external_utils=code@"), std::string::npos);
    EXPECT_EQ(std::string{kFertig.view()}.find("external_utils=code."), std::string::npos);
}

// =================================================================================================
// (2) NEGATIV -- je Fehlform eine EIGENE benannte Diagnose, compile-hart bewiesen
// =================================================================================================
TEST(W10SystemCellValues, JedeFehlformHatIhreEigeneBenannteDiagnose) {
    // Der Bestand: leer == Identitaet (kein Fehler), das volle Set == ok.
    static_assert(cea::diagnose_system_cell_values("") == SystemCellValuesDiagnose::leer,
                  "ein leeres Werte-Set ist KEIN Fehler, sondern der golden-neutrale Identitaets-Pfad.");
    static_assert(cea::diagnose_system_cell_values(kWerteProd1.value) == SystemCellValuesDiagnose::ok);

    // Unbekannter Schluessel -- die Liste ist ABSCHLIESSEND.
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64;operating_system=linux;simd=avx512;"
                                                   "compiler=gcc") == SystemCellValuesDiagnose::unbekannter_schluessel,
                  "ein Schluessel ausserhalb kSystemCellValueKeys darf nie still mitreisen.");
    static_assert(cea::diagnose_system_cell_values("external_utils=simd;target_isa=x86_64;operating_system=linux;"
                                                   "simd=avx512") == SystemCellValuesDiagnose::unbekannter_schluessel,
                  "external_utils ist der WERTFREIE Hub (E-2-Default (a)) -- ein Kunstwert dort waere eine "
                  "zweite Identitaets-Quelle neben der Glieder-Menge.");

    // A-15: RT-Unter-Achsen bekommen ihre EIGENE Diagnose, nicht bloss 'unbekannt'.
    static_assert(cea::diagnose_system_cell_values("os_version=ubuntu_24_04") ==
                      SystemCellValuesDiagnose::verbotener_rt_schluessel,
                  "A-15: os_version ist eine RT-Unter-Achse (A14/OS-U1) und steht NIE im Binary-Stempel.");
    static_assert(cea::diagnose_system_cell_values("kernel=6_17_0") ==
                  SystemCellValuesDiagnose::verbotener_rt_schluessel);
    static_assert(cea::diagnose_system_cell_values("build=35") == SystemCellValuesDiagnose::verbotener_rt_schluessel);
    static_assert(cea::diagnose_system_cell_values("os_family=linux") ==
                      SystemCellValuesDiagnose::verbotener_rt_schluessel,
                  "die Familie heisst im Stempel operating_system -- ein zweiter Name waere eine zweite "
                  "Wahrheit derselben Zelle.");
    static_assert(cea::diagnose_system_cell_values("numa_node=0") ==
                  SystemCellValuesDiagnose::verbotener_rt_schluessel);
    static_assert(cea::diagnose_system_cell_values("page=4k") == SystemCellValuesDiagnose::verbotener_rt_schluessel);
    static_assert(cea::diagnose_system_cell_values("scheduling=cfs") ==
                  SystemCellValuesDiagnose::verbotener_rt_schluessel);

    // Grossbuchstabe -- zwei Schreibweisen derselben Zelle waeren zwei SHA512 fuer denselben Bau.
    static_assert(cea::diagnose_system_cell_values("target_isa=X86_64;operating_system=linux;simd=avx512") ==
                      SystemCellValuesDiagnose::ungueltiges_token_zeichen,
                  "Kleinschreibung ist Pflicht.");
    // '.' und '@' koennen die Zeilen-Grammatik verlassen bzw. wie eine Version aussehen.
    static_assert(cea::diagnose_system_cell_values("target_isa=x86.64;operating_system=linux;simd=avx512") ==
                  SystemCellValuesDiagnose::ungueltiges_token_zeichen);
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64;operating_system=linux;simd=avx512@1") ==
                  SystemCellValuesDiagnose::ungueltiges_token_zeichen);

    // Leerer Token -- n/a reist als 'na', nie als Leere und nie als 0.
    static_assert(cea::diagnose_system_cell_values("target_isa=;operating_system=linux;simd=avx512") ==
                      SystemCellValuesDiagnose::leerer_token,
                  "n/a-statt-NULL: ein nicht bestimmbarer Wert ist 'na', kein leeres Feld.");

    // Doppelter Schluessel -- zwei Werte fuer eine Achse.
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64;target_isa=arm64;operating_system=linux;"
                                                   "simd=avx512") == SystemCellValuesDiagnose::doppelter_schluessel);

    // Teil-Belegung -- eine Stempel-BLINDSTELLE waere unsichtbar.
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64") == SystemCellValuesDiagnose::unvollstaendig,
                  "ein gesetztes Werte-Set MUSS alle drei Achsen nennen.");
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64;operating_system=linux") ==
                  SystemCellValuesDiagnose::unvollstaendig);

    // Segment ohne '='.
    static_assert(cea::diagnose_system_cell_values("target_isa") == SystemCellValuesDiagnose::segment_ohne_gleich);
    static_assert(cea::diagnose_system_cell_values("target_isa=x86_64;;operating_system=linux;simd=avx512") ==
                  SystemCellValuesDiagnose::segment_ohne_gleich);

    // Nur die beiden gutartigen Diagnosen duerfen durch das Renderer-Gate.
    static_assert(cea::system_cell_values_are_usable(""));
    static_assert(cea::system_cell_values_are_usable(kWerteProd1.value));
    static_assert(!cea::system_cell_values_are_usable("os_version=ubuntu_24_04"));
    EXPECT_FALSE(cea::system_cell_values_are_usable("target_isa=X86_64;operating_system=linux;simd=avx512"));
}

// =================================================================================================
// (3) IDENTITAET -- leeres Werte-Set laesst JEDE Zeile byte-identisch (der golden-neutrale Grundpfad)
// =================================================================================================
TEST(W10SystemCellValues, LeeresWerteSetIstDieIdentitaet) {
    static_assert(cea::complete_system_stamp_line_size(kSystemZeileRoh, SystemCellValues{}) ==
                      kSystemZeileRoh.size() + 1,
                  "ohne Werte darf sich die Laenge um kein Byte bewegen.");
    constexpr auto ident = cea::complete_system_stamp_line_array<cea::complete_system_stamp_line_size(
        kSystemZeileRoh, SystemCellValues{})>(kSystemZeileRoh, SystemCellValues{});
    static_assert(ident.view() == kSystemZeileRoh, "Identitaet: leeres Werte-Set == byte-identische Zeile.");

    // Am LEBENDEN Ist, nicht an einem eingefrorenen Literal: der Vervollstaendiger laesst die reale
    // System-Zeile unveraendert. Gleichzeitig ist das die Wache, dass das Literal oben dem Ist entspricht.
    std::string const live = cea::system_stamp_line();
    EXPECT_EQ(live, std::string{kSystemZeileRoh})
        << "das Zeilen-Literal dieser TU ist gegen die lebende system_stamp_line() gelaufen";
    EXPECT_EQ(cea::complete_system_stamp_line(live, SystemCellValues{}), live);

    // Auch die Organ-/Mess-Zeilen-FORM bleibt unberuehrt (der Vervollstaendiger kennt nur System-Achsen).
    std::string const mess = cea::measurement_stamp_line("wallclock");
    EXPECT_EQ(cea::complete_system_stamp_line(mess, SystemCellValues{}), mess);
    EXPECT_EQ(cea::complete_system_stamp_line("", SystemCellValues{}), std::string{});
}

// =================================================================================================
// (4) GRAMMATIK-HAERTUNG + Stempel-KERN: Ebenen, Eintrags-Zahl, Anhang-Position, Versionsteil
// =================================================================================================
TEST(W10SystemCellValues, VervollstaendigteZeileBleibtGrammatikKonform) {
    // Der volle consteval-Weg (Scanner F1-F6 + Entry-Strenge Z-09) traegt die vervollstaendigte Zeile.
    static_assert(cea::stamp_line_is_parsable<"target_isa=code.x86_64@1.0.0c;operating_system=code.linux@1.0.0c;"
                                              "external_utils=code@1.0.0c;[simd=code.avx512@1.0.0c]">,
                  "die vervollstaendigte Zeile MUSS die A13-M2-Klammer-Grammatik erfuellen.");
    // ... und der Owner-Q2-Namensraum traegt sie ohne jede Grammatik-Aenderung: '.' VOR dem '@' ist
    // transparenter Namens-Bestandteil, '.' NACH dem '@' bleibt reiner Zahlen-Trenner.
    static_assert(cea::stamp_line_is_parsable<"target_isa=code.na@1.0.0c;operating_system=code.na@1.0.0c;"
                                              "external_utils=code@1.0.0c;[simd=code.na@1.0.0c]">,
                  "auch die na-Sentinel-Form ist parser-legal.");

    static constexpr char kZiel[] = "target_isa=code.x86_64@1.0.0c;operating_system=code.linux@1.0.0c;"
                                    "external_utils=code@1.0.0c;[simd=code.avx512@1.0.0c]";
    constexpr auto        se      = cea::parse_stamp_entries<cea::count_stamp_entries(std::string_view{kZiel})>(kZiel);
    static constexpr char kRoh[]  = "target_isa=code@1.0.0c;operating_system=code@1.0.0c;"
                                    "external_utils=code@1.0.0c;[simd=code@1.0.0c]";
    constexpr auto        sr      = cea::parse_stamp_entries<cea::count_stamp_entries(std::string_view{kRoh})>(kRoh);

    // EINTRAGS-ZAHL unveraendert -- der Zellwert reist IN einem Eintrag, nie als zusaetzliches Segment.
    static_assert(se.size() == sr.size() && se.size() == 4);
    EXPECT_EQ(se.size(), sr.size());

    // ACHSEN-NAMEN kanonisch unangetastet (kSystemAxisOrder-Konsumenten, A14-Guards).
    static_assert(std::string_view(se[0].axis, se[0].axis_len) == cea::kSystemAxisOrder[0]);
    static_assert(std::string_view(se[1].axis, se[1].axis_len) == cea::kSystemAxisOrder[1]);
    static_assert(std::string_view(se[2].axis, se[2].axis_len) == cea::kSystemAxisOrder[2]);
    static_assert(std::string_view(se[3].axis, se[3].axis_len) == cea::kSystemCellValueKeys[2]);

    // Der ZELLWERT sitzt im ALGORITHMUS-Namen, nicht im Achsen-Namen.
    static_assert(std::string_view(se[0].algorithm, se[0].algo_len) == "code.x86_64");
    static_assert(std::string_view(se[1].algorithm, se[1].algo_len) == "code.linux");
    static_assert(std::string_view(se[2].algorithm, se[2].algo_len) == "code");
    static_assert(std::string_view(se[3].algorithm, se[3].algo_len) == "code.avx512");

    // VERSIONSTEIL BYTE-UNBERUEHRT (ENFORCE-Wache: W10 fuehrt keine neue Version ein).
    for (std::size_t i = 0; i < se.size(); ++i) {
        EXPECT_EQ(se[i].x, sr[i].x);
        EXPECT_EQ(se[i].y, sr[i].y);
        EXPECT_EQ(se[i].z, sr[i].z);
        EXPECT_EQ(se[i].reserved, sr[i].reserved) << "Flag-/Ebenen-Bits duerfen sich nicht bewegen (Index " << i << ")";
    }
    static_assert(se[0].x == 1u && se[0].y == 0u && se[0].z == 0u);
    static_assert(cea::stamp_entry_hardware_flag(se[0]) == cea::StampEntryHardwareFlag::cpu);
    static_assert(!cea::stamp_entry_is_experimental(se[0]), "'e' bleibt die Pruefling-Markierung, W10 setzt keins.");

    // STEMPEL-KERN: der Meta-Meta-Anhang bleibt am Realm-Zeilen-ENDE und behaelt seine Ebene.
    static_assert(cea::stamp_entry_meta_level(se[0]) == 0u);
    static_assert(cea::stamp_entry_meta_level(se[1]) == 0u);
    static_assert(cea::stamp_entry_meta_level(se[2]) == 0u);
    static_assert(cea::stamp_entry_meta_level(se[3]) == 1u, "der simd-Anhang bleibt Ebene 1 am Zeilen-ENDE.");
    static_assert(cea::stamp_entry_meta_level(se[3]) == cea::stamp_entry_meta_level(sr[3]));
    // KEINE merge-Zeile (Owner-E2 / Stempel-KERN): der Vervollstaendiger baut nichts an die Zeile an.
    EXPECT_EQ(std::string{kFertig.view()}.find("merge"), std::string::npos);
    EXPECT_EQ(std::string{kFertig.view()}.back(), ']') << "der Klammer-Anhang bleibt das LETZTE Zeichen der Zeile";
}

// =================================================================================================
// W10-C2 (Orakel iv) -- DIE MAKRO-NAHT: vervollstaendigte kS-Zeile + DISKRIMINIERENDER kFP
// =================================================================================================
TEST(W10SystemCellValues, MakroNahtVervollstaendigtDieSystemZeileUndDenFingerprint) {
    auto const* const v = comdare_anatomy_version_lines();
    ASSERT_NE(v, nullptr);

    // (1) Die einkompilierte SYSTEM-Zeile ist die vervollstaendigte Zeile -- nicht das Literal, mit dem
    //     das Makro gerufen wurde. Genau das ist die C2-Aussage.
    EXPECT_EQ(std::string(v->system_line, v->system_len), std::string{kSystemZeileZiel});
    EXPECT_NE(std::string(v->system_line, v->system_len), std::string{kSystemZeileRoh});

    // (2) FINGERPRINT-DISKRIMINIERUNG, literal: kFP rechnet ueber die VERVOLLSTAENDIGTE Zeile. Der
    //     Digest OHNE Zellwerte ist damit ein ANDERER -- das ist der Kern der W10-Zusage (zwei Baue
    //     derselben Permutation auf verschiedenen OS-Familien/ISAs kollidieren nicht mehr).
    constexpr auto kFpOhne = cea::anatomy_fingerprint_hex(kOrganLit, kSystemZeileRoh, kMeasureLit);
    constexpr auto kFpMit  = cea::anatomy_fingerprint_hex(kOrganLit, kSystemZeileZiel, kMeasureLit);
    static_assert(std::string_view{kFpOhne.data()} != std::string_view{kFpMit.data()},
                  "W10-C2: die Zellwerte MUESSEN den Fingerprint verschieben -- taeten sie es nicht, waere "
                  "die ganze W10-Zusage (Zuordbarkeit/Wiederverwendbarkeit, Owner-E3) leer.");
    EXPECT_EQ(std::string(v->sha512_line, v->sha512_len), std::string{kFpMit.data()});
    EXPECT_NE(std::string(v->sha512_line, v->sha512_len), std::string{kFpOhne.data()});

    // (3) NEGATIV-PROBE (gleiches Werte-Set => gleicher kFP): der Digest haengt an den WERTEN, nicht am
    //     Zeitpunkt oder an einer Adresse -- zwei Baue derselben Zelle bleiben identisch.
    constexpr auto kFpMitNochmal = cea::anatomy_fingerprint_hex(kOrganLit, kSystemZeileZiel, kMeasureLit);
    static_assert(std::string_view{kFpMit.data()} == std::string_view{kFpMitNochmal.data()});
    // ... und ein FREMD-Familien-Werte-Set liefert einen ANDEREN Digest (die B1-Kollision linux==macos).
    constexpr auto kZeileMacos = cea::complete_system_stamp_line_array<cea::complete_system_stamp_line_size(
        kSystemZeileRoh, SystemCellValues{"target_isa=x86_64;operating_system=macos;simd=avx512"})>(
        kSystemZeileRoh, SystemCellValues{"target_isa=x86_64;operating_system=macos;simd=avx512"});
    constexpr auto kFpMacos = cea::anatomy_fingerprint_hex(kOrganLit, kZeileMacos.view(), kMeasureLit);
    static_assert(std::string_view{kFpMacos.data()} != std::string_view{kFpMit.data()},
                  "linux und macos MUESSEN verschiedene Fingerprints tragen -- das ist der W10-Abnahme-Kern.");
    EXPECT_NE(std::string{kFpMacos.data()}, std::string{kFpMit.data()});

    // (4) KEIN POD-/LAYOUT-ANFASSEN: die Naht bewegt Zeilen-INHALT, nie Struktur.
    EXPECT_EQ(v->stamp_layout_version, 6u);
    EXPECT_EQ(sizeof(cea::AnatomyVersionLines), 120u);
    EXPECT_TRUE(cea::stamp_pod_has_entries(*v));
    EXPECT_EQ(v->system_entry_count, 4u) << "3 Haupt-Achsen + 1 geklammerte Meta-Meta -- unveraendert";
    EXPECT_EQ(v->organ_entry_count, 2u);
    EXPECT_EQ(v->measurement_entry_count, 2u);

    // (5) Das ARRAY (kSE) parst die VERVOLLSTAENDIGTE Zeile -- Achsen-Namen kanonisch, Zellwert im
    //     Algorithmus-Namen, Ebene des Meta-Meta-Anhangs unveraendert.
    ASSERT_EQ(v->system_entry_count, 4u);
    EXPECT_EQ(std::string_view(v->system_entries[0].axis, v->system_entries[0].axis_len), cea::kSystemAxisOrder[0]);
    EXPECT_EQ(std::string_view(v->system_entries[0].algorithm, v->system_entries[0].algo_len),
              std::string_view{"code.x86_64"});
    EXPECT_EQ(std::string_view(v->system_entries[2].algorithm, v->system_entries[2].algo_len), std::string_view{"code"})
        << "external_utils bleibt der wertfreie Hub";
    EXPECT_EQ(cea::stamp_entry_meta_level(v->system_entries[3]), 1u);
    EXPECT_EQ(std::string_view(v->system_entries[3].algorithm, v->system_entries[3].algo_len),
              std::string_view{"code.avx512"});

    // (6) Die ORGAN- und MESS-Zeilen sind vom Vervollstaendiger UNBERUEHRT (er kennt nur System-Achsen).
    EXPECT_EQ(std::string(v->organ_line, v->organ_len), std::string{kOrganLit});
    EXPECT_EQ(std::string(v->measurement_line, v->measurement_len), std::string{kMeasureLit});
}

// =================================================================================================
// (5) DRIFT-WACHE gegen den System-Meta-Meta-Hub + na-Sentinel
// =================================================================================================
TEST(W10SystemCellValues, SchluesselMengeIstGegenAchsenOrdnungUndHubGewacht) {
    // Achsen-Ordnungs-Haelfte (die zweite Haelfte steht als static_assert im Header selbst).
    static_assert(cea::kSystemCellValueKeys[0] == cea::kSystemAxisOrder[0]);
    static_assert(cea::kSystemCellValueKeys[1] == cea::kSystemAxisOrder[1]);
    static_assert(!cea::is_system_cell_value_key(cea::kSystemAxisOrder[2]),
                  "external_utils ist der wertfreie Hub -- er hat keine eigene Zelle.");

    // HUB-Haelfte: der dritte Schluessel IST der Stempel-Name des einzigen Meta-Meta-Glieds. Wer ein
    // zweites Glied (gpu/fpga/npu) einhaengt, muss HIER entscheiden, ob es einen Zellwert traegt.
    static_assert(cem::ExternalUtilsHub::meta_metas::size == 1,
                  "W10-C1: der System-Meta-Meta-Hub hat ein zweites Glied bekommen -- kSystemCellValueKeys "
                  "deckt dann nicht mehr alle Meta-Meta-Zellen ab (E-2 neu entscheiden).");
    static_assert(cea::kSystemCellValueKeys[2] == cem::SimdExternalUtilsFamily::sub_axis_label(),
                  "W10-C1: der simd-Zellwert-Schluessel ist aus dem Stempel-Namen des Hub-Glieds gelaufen.");
    static_assert(cea::kSystemCellValueKeys[2] == cem::SimdExternalUtilsFamily::family_id());

    // na-Sentinel: grammatisch zulaessig, als AUSSAGE erkennbar (Fail-Closed-Bedingung fuer C4).
    static_assert(cea::system_cell_value_is_na(cea::kSystemCellValueNa));
    static_assert(cea::system_cell_values_contain_na("target_isa=na;operating_system=linux;simd=avx512"));
    static_assert(!cea::system_cell_values_contain_na(kWerteProd1.value));
    static_assert(cea::diagnose_system_cell_values("target_isa=na;operating_system=na;simd=na") ==
                      SystemCellValuesDiagnose::ok,
                  "'na' ist eine AUSSAGE (nicht bestimmbar), kein Formfehler -- die Fail-Closed-Folge "
                  "(kein Lager-Rueckschrieb) haengt am eigenen Praedikat, nicht an der Grammatik.");
    EXPECT_EQ(
        cea::complete_system_stamp_line(kSystemZeileRoh, SystemCellValues{"target_isa=na;operating_system=na;simd=na"}),
        std::string{"target_isa=code.na@1.0.0c;operating_system=code.na@1.0.0c;external_utils=code@1.0.0c;"
                    "[simd=code.na@1.0.0c]"});

    // Der Wert-Lookup ist die EINE Nachschlage-Stelle (keine zweite Ableitung im Renderer).
    static_assert(cea::system_cell_value_for(kWerteProd1.value, "target_isa") == "x86_64");
    static_assert(cea::system_cell_value_for(kWerteProd1.value, "operating_system") == "linux");
    static_assert(cea::system_cell_value_for(kWerteProd1.value, "simd") == "avx512");
    static_assert(cea::system_cell_value_for(kWerteProd1.value, "external_utils").empty());
}

// =================================================================================================
// (6) A-15 am LEBENDEN Ist -- kein erhobener RT-Wert steht in der Zeile
// =================================================================================================
TEST(W10SystemCellValues, KeineRtUnterAchsenInDerVervollstaendigtenZeile) {
    std::string const fertig{kFertig.view()};
    for (std::string_view const verboten :
         {"os_version", "kernel", "build", "os_family", "numa_node", "page", "scheduling", "core_class"})
        EXPECT_EQ(fertig.find(verboten), std::string::npos)
            << "A-15: '" << verboten << "' darf in keiner Stempel-Zeile auftauchen";

    // Und die drei OS-Unter-Achsen-Labels ihrerseits sind keine zulaessigen Zellwert-Schluessel.
    for (auto const& label : cem::kOperatingSystemSubAxisLabels) {
        EXPECT_FALSE(cea::is_system_cell_value_key(label));
        EXPECT_EQ(cea::diagnose_system_cell_values(std::string{label} + "=x"),
                  SystemCellValuesDiagnose::verbotener_rt_schluessel);
    }
}

// =================================================================================================
// W10-C3 -- DAS ZWILLINGS-GLEICHHEITS-ORAKEL: consteval-Makro-kFP == Laufzeit-Lager-Key
// =================================================================================================
// Die EINE-WAHRHEIT-Doktrin (Drift-Guard-Praezedenz A5CebVersionStamp): der einkompilierte Fingerprint
// und der Lager-Key MUESSEN fuer dasselbe Werte-Set denselben 128-hex liefern. Vor W10 galt das, weil
// beide dieselbe Glied-Folge hashten; nach W10 gilt es nur weiter, wenn BEIDE ueber DIESELBE
// vervollstaendigte System-Zeile rechnen. Genau das wird hier literal geprueft -- nicht behauptet.
TEST(W10SystemCellValues, ZwillingsGleichheitConstevalMakroGegenLaufzeitLagerKey) {
    namespace bl = ::comdare::cache_engine::builder::bestandslog;

    // Die kanonische Glied-Folge mit der ROHEN System-Zeile -- exakt so, wie der Lager-Weg sie baut.
    auto const glieder_roh = cea::anatomy_fingerprint_glieder(kOrganLit, kSystemZeileRoh, kMeasureLit);
    std::span<std::string_view const> const roh{glieder_roh.data(), glieder_roh.size()};

    // (1) MIT Zellwerten: der Laufzeit-Lager-Key == der consteval-Fingerprint der Makro-Naht (die TU
    //     uebersetzt mit gesetztem Define, comdare_anatomy_version_lines() traegt also die Zellwerte).
    auto const* const v = comdare_anatomy_version_lines();
    ASSERT_NE(v, nullptr);
    std::string const lager_key_mit = bl::to_hex(bl::BinaryKeyPolicy::derive_key(roh, kWerteProd1));
    EXPECT_EQ(lager_key_mit, std::string(v->sha512_line, v->sha512_len))
        << "Lager-Key und einkompilierter Fingerprint MUESSEN fuer dieselbe Zelle identisch sein "
           "(EINE-Wahrheit-Doktrin) -- sonst findet das Skip-Gate seine eigenen Binaries nicht wieder";

    // (2) LEERES Werte-Set => byte-identisch zum Weg ohne Zellwerte. Zeuge ist der eingefrorene
    //     Testvektor: dieselben drei Zeilen wie in test_m_w12/test_g3 ergeben denselben Hex.
    //     O-2/C-2: im Format-3-Commit NEU eingefroren (Bump 2 -> 3, zwei zusaetzliche Glieder) -- die
    //     AUSSAGE ist unveraendert, nur ihr Zeuge ist der heutige. Alle drei Fundstellen wurden im
    //     SELBEN Commit gedreht (Lehre "gruene Tests zementieren alte Ordnung").
    constexpr std::string_view kFrozenOrgan = "search_algo=k_ary@1.0.0c;path_compression=path_compression_none@1.0.0c";
    constexpr std::string_view kFrozenMeasure = "measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]";
    // NB/CX-4 END-FORM (06.08.2026, der ZWEITE und LETZTE Neuanker-Dreh des Buendels): der Vektor rechnet
    // ab hier ueber BELEGTE Glieder [5]/[6] -- Literale, byte-gleich zu test_g3_sha512_index.cpp und
    // test_m_w12_stamp_bausteine.cpp. Vorgaenger f8f811a9...9137fb0c, in der git-Historie. Grund: mit der
    // Live-Naht tragen beide Glieder produktiv Werte; ein Anker ueber die leeren Defaults deckte genau den
    // neuen Teil des Preimage nicht ab. LITERALE statt Live-Werte, weil die Live-Werte an Toolchain und
    // Enable-Menge der Maschine haengen (8er-Docker-Matrix).
    constexpr std::string_view kFrozenToolchain =
        "tc=1;cxx=gcc-16.2.0@1.0.0c;opt=O3{-O3}@1.0.0c;ext=avx512;ceb=8.0;gate=avx512;atomic128=cx16{-mcx16}@1.0.0c";
    constexpr std::string_view kFrozenBvset = "bvset=1;bv=2;page_type[{bplus;hw_cache_line=64;hw_numa_capable=0}];"
                                              "simd_extension[{avx512}];"
                                              "general_hardware[{x86_64;hw_cache_line=64;hw_numa_capable=0}]";
    constexpr std::string_view kFrozenFingerprintV1 =
        "17148e5a4d0f4a2d96e1f5ad97dc4c727b99fce6e38bd6e337fb6dbf0e4461f9"
        "b7fd37fbba76414be4718ad2180deecbb14387293935a8eff1469cef8ce89374";
    auto const frozen_glieder =
        cea::anatomy_fingerprint_glieder(kFrozenOrgan, kSystemZeileRoh, kFrozenMeasure,
                                         cea::ToolchainGlied{kFrozenToolchain}, cea::BvsetGlied{kFrozenBvset});
    std::span<std::string_view const> const frozen{frozen_glieder.data(), frozen_glieder.size()};
    EXPECT_EQ(bl::to_hex(bl::BinaryKeyPolicy::derive_key(frozen)), std::string{kFrozenFingerprintV1})
        << "der Default-Pfad (leeres Werte-Set) MUSS byte-identisch zum Vor-W10-Stand bleiben";
    EXPECT_EQ(bl::to_hex(bl::BinaryKeyPolicy::derive_key(frozen, SystemCellValues{})),
              std::string{kFrozenFingerprintV1})
        << "explizit leere Zellwerte sind dieselbe Identitaet wie der Default";

    // (3) DISKRIMINIERUNG am Lager-Key: ein anderes Werte-Set => anderer Key; dasselbe => derselbe Key.
    std::string const lager_key_macos = bl::to_hex(
        bl::BinaryKeyPolicy::derive_key(roh, SystemCellValues{"target_isa=x86_64;operating_system=macos;simd=avx512"}));
    EXPECT_NE(lager_key_macos, lager_key_mit);
    EXPECT_EQ(bl::to_hex(bl::BinaryKeyPolicy::derive_key(roh, kWerteProd1)), lager_key_mit);
    EXPECT_NE(bl::to_hex(bl::BinaryKeyPolicy::derive_key(roh)), lager_key_mit)
        << "ohne Zellwerte MUSS ein anderer Key entstehen -- sonst waere die Naht wirkungslos";

    // (4) Die ZELL-Klammer des LagerKey bleibt DREI-feldig (Manager-Entscheid W10-M1): die Zellwerte
    //     wandern in den SHA512, NICHT in die ZellKoordinaten.
    bl::ZellKoordinaten const zelle{.combo = "default", .opt = "O2", .simd = "avx512"};
    bl::LagerKey const        lk = bl::BinaryKeyPolicy::derive_lager_key(roh, zelle, kWerteProd1);
    EXPECT_EQ(bl::to_hex(lk.sha), lager_key_mit);
    EXPECT_EQ(lk.zelle.combo, std::string{"default"});
    EXPECT_EQ(lk.zelle.opt, std::string{"O2"});
    EXPECT_EQ(lk.zelle.simd, std::string{"avx512"});

    // (5) FAIL-CLOSED-Wache statt stiller Fehl-Ableitung: eine FREMDE Komponenten-Liste (ohne
    //     System-Glied) mit gesetzten Zellwerten ist ein Fehler, kein still an Index 2 vervollstaendigtes
    //     Fremd-Glied.
    std::array<std::string_view, 2> const fremd{{"mess_a", "mess_b"}};
    EXPECT_THROW((void)bl::BinaryKeyPolicy::derive_key(std::span<std::string_view const>{fremd}, kWerteProd1),
                 std::invalid_argument);
    EXPECT_NO_THROW((void)bl::BinaryKeyPolicy::derive_key(std::span<std::string_view const>{fremd}));
}
