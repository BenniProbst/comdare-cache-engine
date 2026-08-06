// test_d2_mess_konsistenz_gate.cpp -- M-1/D-2 (06.08.2026): DER BISS am Vertrag CEB <-> Tier-Binary.
//
// PRUEFLING: pruef_dock/mess_konsistenz_gate.hpp -- das Gate, das beim Laden einer Tier-Binary prueft, ob
// ihre DEKLARIERTE Mess-Ausstattung (comdare_anatomy_version_lines()->measurement_line/-entries) zu der
// passt, die die CEB einkompiliert hat.
//
// ------------------------------------------------------------------------------------------------
// WARUM DIESER BISS NICHT MIT EINEM HANDGEBAUTEN POD ALLEIN AUSKOMMT
// ------------------------------------------------------------------------------------------------
// Ein Gate, das nur gegen von Hand zusammengesetzte PODs geprueft wird, beweist, dass die VERGLEICHS-
// Logik stimmt -- nicht, dass sie an dem POD stimmt, den die reale Makro-Naht erzeugt. Deshalb faehrt
// dieser Biss auf DREI Ebenen, und jede traegt etwas, das die anderen nicht koennen:
//   (A) DIE ECHTE MAKRO-EXPANSION in dieser TU (Praezedenz test_w10_system_cell_values / test_m_w12):
//       COMDARE_ANATOMY_VERSION_STAMP_M materialisiert comdare_anatomy_version_lines() genau so, wie es
//       jede emittierte Modul-Quelle tut -- inklusive des consteval-Parsers, der das Entry-Array baut.
//       Was hier gruen ist, ist an der realen Naht gruen.
//   (B) DIE ECHTE .so UEBER DIE dlopen-GRENZE (r5g-adhoc-Fixture, dieselbe wie test_pruef_only_gate):
//       sie belegt, dass der Loader die Deklaration wirklich zieht und dass eine Binary OHNE Stempel
//       real abgewiesen wird -- ueber die Modul-Grenze, nicht im selben Prozess-Bild.
//   (C) MANIPULIERTE DEKLARATIONEN: PODs, deren Mess-Zeile bzw. deren Entry-Array bewusst falsch ist.
//       Sie sind der einzige Weg, die Abweisung zu zeigen, ohne eine zweite .so zu bauen -- und sie
//       sind KEINE Attrappen: ihre Entry-Arrays entstehen aus DEMSELBEN consteval-Parser
//       (parse_stamp_entries), aus dem auch das Makro sie baut. Manipuliert ist die ZEILE, nicht der
//       Parser.
//
// ------------------------------------------------------------------------------------------------
// ANTI-VAKUOSITAET
// ------------------------------------------------------------------------------------------------
// Jede Erwartung dieses Bisses ist gegen die LEBENDEN Quellen abgeglichen, nicht gegen ein blindes Pin:
// die Vollmengen-Zeile steht als Literal da (der consteval-Weg braucht eines), wird aber zur Laufzeit
// gegen abi::measurement_stamp_line_full_set() geprueft. Wandert die Mess-Registry, sagt dieser Test es
// -- statt still die falsche Zeile weiterzuvergleichen.
//
// ASCII-only.

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>  // COMDARE_ANATOMY_VERSION_STAMP_M (die ECHTE Makro-Naht)
#include <cache_engine/abi/anatomy_stamp_entries.hpp>  // parse_stamp_entries / stamp_entries_ptr
#include <cache_engine/abi/anatomy_version_stamp.hpp>  // measurement_stamp_line_full_set / _from_combo_legend
#include <pruef_dock/mess_konsistenz_gate.hpp>         // PRUEFLING
#include <pruef_dock/pruef_only.hpp>                   // run_so_conformance_gate (Ebene B)

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace cea = ::comdare::cache_engine::abi;
namespace pd  = ::comdare::cache_engine::builder::pruef_dock;

namespace {

// -- Die Literale der Makro-Naht (Ebene A). Organ-/System-Zeile bewusst kurz und synthetisch (Praezedenz
//    test_w10/test_m_w12: geprueft wird die MESS-Naht, nicht die Welt). Die MESS-Zeile ist dagegen die
//    reale Vollmengen-Form -- sie IST der Prueflings-Gegenstand.
#define COMDARE_D2_ORGAN_LIT   "search_algo=k_ary@1.0.0c;filter=bloom@2.3.4c"
#define COMDARE_D2_SYSTEM_LIT                                                                                  \
    "target_isa=code@1.0.0c;operating_system=code@1.0.0c;external_utils=code@1.0.0c;"                           \
    "[simd=code@1.0.0c]"
#define COMDARE_D2_MESS_VOLL                                                                                           \
    "measurement_tooling=wallclock@1.0.0c;measurement_tooling=macro@1.0.0c;"                                    \
    "measurement_tooling=micro@1.0.0c;"                                                                         \
    "[load_framework=ycsb@1.0.0c]"
// Die EINZEL-Wahl [wallclock] -- die Zeile, die eine wallclock-hart gebaute CEB stempelt.
#define COMDARE_D2_MESS_WALLCLOCK "measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]"

constexpr char kMessVoll[]      = COMDARE_D2_MESS_VOLL;
constexpr char kMessWallclock[] = COMDARE_D2_MESS_WALLCLOCK;
constexpr char kLeer[]          = "";

// Die Entry-Arrays aus DEMSELBEN consteval-Parser, den auch das Makro benutzt. Das ist der Punkt: die
// manipulierten PODs unten sind in allem echt ausser in dem einen Stueck, das manipuliert wird.
constexpr auto kEntriesVoll =
    cea::parse_stamp_entries<cea::count_stamp_entries(std::string_view{kMessVoll})>(kMessVoll);
constexpr auto kEntriesWallclock =
    cea::parse_stamp_entries<cea::count_stamp_entries(std::string_view{kMessWallclock})>(kMessWallclock);

/// Einen AnatomyVersionLines-POD von Hand bauen -- mit frei waehlbarer Layout-Version, Mess-Zeile und
/// Mess-Entry-Array. Organ-/System-Felder sind hier belanglos (das Gate liest sie nicht) und stehen als
/// ""-Sentinel da, exakt wie die ""-Doktrin des POD es verlangt (nie nullptr).
[[nodiscard]] constexpr cea::AnatomyVersionLines
mach_pod(std::uint32_t layout, char const* mess, std::uint64_t mess_len, cea::AnatomyStampEntryV1 const* eintraege,
         std::uint64_t eintrag_count) noexcept {
    return cea::AnatomyVersionLines{layout,
                                    0u,
                                    "",
                                    0u,
                                    "",
                                    0u,
                                    mess,
                                    mess_len,
                                    "",
                                    0u,
                                    cea::kAnatomyStampNoEntries,
                                    0u,
                                    cea::kAnatomyStampNoEntries,
                                    0u,
                                    eintraege,
                                    eintrag_count};
}

/// Die eine gebaute perm-.so im Fixture-Dir finden (Loader-Konvention, uebernommen aus test_pruef_only_gate).
[[nodiscard]] std::filesystem::path find_built_so(std::filesystem::path const& dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return {};
    for (auto const& e : std::filesystem::directory_iterator{dir, ec}) {
        if (!e.is_regular_file()) continue;
        std::string const name = e.path().filename().string();
        std::string const ext  = e.path().extension().string();
        if (name.rfind("comdare_anatomy_perm_", 0) == 0 && (ext == ".so" || ext == ".dll")) return e.path();
    }
    return {};
}

} // namespace

// DIE ECHTE MAKRO-EXPANSION (Ebene A). Materialisiert comdare_anatomy_version_lines() in dieser TU.
COMDARE_ANATOMY_VERSION_STAMP_M(COMDARE_D2_ORGAN_LIT, COMDARE_D2_SYSTEM_LIT, COMDARE_D2_MESS_VOLL)

// =================================================================================================
// (0) DIE ERWARTUNG IST NICHT BLIND -- das Literal deckt sich mit der lebenden Registry
// =================================================================================================
TEST(D2MessKonsistenz, VollmengenLiteralDecktSichMitDerLebendenRegistry) {
    // Waere das falsch, prueften ALLE folgenden Faelle die falsche Zeile und waeren gruen ohne Aussage.
    EXPECT_EQ(std::string{kMessVoll}, cea::measurement_stamp_line_full_set())
        << "Die Mess-Tooling-Registry ist gewandert -- das Literal dieses Bisses ist stale.";
    EXPECT_EQ(std::string{kMessWallclock}, cea::measurement_stamp_line_from_combo_legend("[wallclock]"))
        << "Der Renderer bildet [wallclock] nicht mehr auf diese Zeile ab.";
    // Und die beiden Zeilen sind unterscheidbar -- sonst koennte der Abweichungs-Fall unten nichts zeigen.
    EXPECT_NE(std::string{kMessVoll}, std::string{kMessWallclock});
}

// =================================================================================================
// (1) POSITIV an der ECHTEN Makro-Naht -- SOLL == IST wird ANGENOMMEN
// =================================================================================================
TEST(D2MessKonsistenz, EchteMakroNahtBestehtGegenIhreEigeneZeile) {
    auto const* const v = comdare_anatomy_version_lines();
    ASSERT_NE(v, nullptr) << "die Makro-Naht hat das Probe-Symbol nicht materialisiert";

    pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(v, cea::measurement_stamp_line_full_set());
    std::cout << "[d2] " << pd::mess_konsistenz_meldung(e) << "\n";

    EXPECT_TRUE(e.passed()) << pd::mess_konsistenz_meldung(e);
    EXPECT_EQ(e.status, pd::MessKonsistenzStatus::ok);
    // ANTI-VAKUOSITAET: das Gate hat wirklich etwas gelesen, nicht nur nichts gefunden.
    EXPECT_EQ(e.ist, std::string{kMessVoll});
    EXPECT_EQ(e.ist_stamp_layout, cea::kAnatomyVersionLinesLayout);
    // Die Vollmengen-Zeile traegt DREI Haupt-Achsen + EINEN Meta-Meta-Anhang -> 4 Eintraege, davon 3 auf
    // Ebene 0. Beide Zahlen werden AUSGEGEBEN, nicht bloss behauptet.
    std::cout << "[d2] entries_gesamt=" << e.ist_entry_count << " davon_ebene0=" << e.ist_haupt_count
              << " soll_ebene0=" << e.soll_haupt_count << "\n";
    EXPECT_EQ(e.ist_haupt_count, 3u);
    EXPECT_EQ(e.soll_haupt_count, 3u);
    EXPECT_GT(e.ist_entry_count, e.ist_haupt_count)
        << "der Meta-Meta-Anhang [load_framework=...] muss im Gesamt-Array stecken, aber NICHT auf Ebene 0 "
           "-- sonst haette der Vergleich gegen measurement_entry_count zufaellig gestimmt";
}

// =================================================================================================
// (2) DER KERN-BISS: eine Tier-Binary mit FALSCHER Mess-Deklaration wird ABGEWIESEN
// =================================================================================================
TEST(D2MessKonsistenz, FalscheDeklarationWirdAbgewiesen) {
    // Die Binary DEKLARIERT [wallclock]; die CEB hat die Vollmenge einkompiliert. Genau der Fall, den
    // Owner-KERN F2 meint ("bei einem neuen Messsystem muessen ALLE Binaries neu gebaut werden") und den
    // bis M-1 NIEMAND bemerkt haette: alle 27 Fundstellen der Mess-Achse waren Legende, Stempel oder
    // Parse-Pfad, KEINE las die Deklaration.
    cea::AnatomyVersionLines const falsch =
        mach_pod(cea::kAnatomyVersionLinesLayout, kMessWallclock, sizeof(kMessWallclock) - 1,
                 cea::stamp_entries_ptr(kEntriesWallclock), kEntriesWallclock.size());
    pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(&falsch, cea::measurement_stamp_line_full_set());
    std::cout << "[d2] " << pd::mess_konsistenz_meldung(e) << "\n";

    EXPECT_FALSE(e.passed());
    EXPECT_EQ(e.status, pd::MessKonsistenzStatus::zeile_abweichung);
    // Die Meldung nennt BEIDE Seiten -- ohne das muesste der Bediener den Grund selbst suchen.
    std::string const m = pd::mess_konsistenz_meldung(e);
    EXPECT_NE(m.find(kMessVoll), std::string::npos);
    EXPECT_NE(m.find(kMessWallclock), std::string::npos);

    // GEGENPROBE (macht die Abweisung nicht-vakuos): DERSELBE POD gegen SEINE EIGENE Zeile besteht. Ohne
    // diese Haelfte waere ein Gate, das immer ablehnt, ebenfalls gruen.
    pd::MessKonsistenzErgebnis const g =
        pd::pruefe_mess_konsistenz(&falsch, cea::measurement_stamp_line_from_combo_legend("[wallclock]"));
    EXPECT_TRUE(g.passed()) << pd::mess_konsistenz_meldung(g);
}

// =================================================================================================
// (3) FAIL-CLOSED -- jeder unklare Zustand ist ein FEHLER, keiner ist ein Freifahrtschein
// =================================================================================================
TEST(D2MessKonsistenz, FailClosedInAllenUnklarenZustaenden) {
    std::string const soll = cea::measurement_stamp_line_full_set();

    // (1) ERWARTUNG LEER -- die CEB benennt ihre einkompilierte Mess-Achse nicht.
    {
        auto const* const           v = comdare_anatomy_version_lines();
        pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(v, "");
        EXPECT_FALSE(e.passed());
        EXPECT_EQ(e.status, pd::MessKonsistenzStatus::erwartung_leer)
            << "eine leere Erwartung darf NIE durchgewunken werden -- auch nicht an einer tadellosen Binary";
    }
    // (2) KEIN STEMPEL-SYMBOL.
    {
        pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(nullptr, soll);
        EXPECT_FALSE(e.passed());
        EXPECT_EQ(e.status, pd::MessKonsistenzStatus::stempel_symbol_fehlt);
    }
    // (3) FREMDES POD-LAYOUT. Layout 5 traegt merge_line/merge_len und hat damit ANDERE Offsets (A13-M3/K-4)
    //     -- es MUSS abgewiesen werden, BEVOR ein Feld gelesen wird.
    {
        cea::AnatomyVersionLines const alt = mach_pod(5u, kMessVoll, sizeof(kMessVoll) - 1,
                                                      cea::stamp_entries_ptr(kEntriesVoll), kEntriesVoll.size());
        pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(&alt, soll);
        EXPECT_FALSE(e.passed());
        EXPECT_EQ(e.status, pd::MessKonsistenzStatus::stempel_layout_fremd)
            << "ein fremdes Layout haette mit v6-Offsets gelesen Zeiger-Muell geliefert -- schlimmer als "
               "gar kein Ergebnis";
        // Und zwar auch dann, wenn die Zeile ZUFAELLIG stimmt: das Layout-Gate steht VOR dem Zeilen-Gate.
        EXPECT_EQ(e.ist, std::string{}) << "bei fremdem Layout darf KEIN Feld ausgelesen worden sein";
    }
    // (4) LEERE DEKLARATION -- "kein Mess-Tooling einkompiliert". Ehrliche Aussage, und genau der Zustand,
    //     in dem die Binary nicht messen darf. Das ist die Form, die der 2-arg-Emissionsweg erzeugt.
    {
        cea::AnatomyVersionLines const leer =
            mach_pod(cea::kAnatomyVersionLinesLayout, kLeer, 0u, cea::kAnatomyStampNoEntries, 0u);
        pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(&leer, soll);
        EXPECT_FALSE(e.passed());
        EXPECT_EQ(e.status, pd::MessKonsistenzStatus::deklaration_leer);
    }
    // (5) ZWEITER ABLEITUNGSWEG: die ZEILE stimmt byte-genau, das ENTRY-ARRAY widerspricht ihr. Das kann
    //     nur entstehen, wenn Zeile und Array im Modul auseinandergelaufen sind -- die
    //     O-8-Schritt-12-Fehlerklasse. Hier bewusst herbeigefuehrt, indem die richtige Zeile mit dem
    //     wallclock-Array kombiniert wird.
    {
        cea::AnatomyVersionLines const wider =
            mach_pod(cea::kAnatomyVersionLinesLayout, kMessVoll, sizeof(kMessVoll) - 1,
                     cea::stamp_entries_ptr(kEntriesWallclock), kEntriesWallclock.size());
        pd::MessKonsistenzErgebnis const e = pd::pruefe_mess_konsistenz(&wider, soll);
        std::cout << "[d2] " << pd::mess_konsistenz_meldung(e) << "\n";
        EXPECT_FALSE(e.passed());
        EXPECT_EQ(e.status, pd::MessKonsistenzStatus::entries_widerspruch)
            << "Zeile und Entry-Array entstehen im Makro aus DEMSELBEN Literal, aber ueber ZWEI consteval-Wege "
               "-- diese Wache ist der Grund, warum das billig zu pruefen ist und trotzdem geprueft wird";
        EXPECT_EQ(e.ist_haupt_count, 1u);
        EXPECT_EQ(e.soll_haupt_count, 3u);
    }
}

// =================================================================================================
// (4) DAS TESTAT -- die Quoten-Form, mit der das Gate im Pruefstand-Batch mitlaeuft
// =================================================================================================
TEST(D2MessKonsistenz, TestatFormLiefertEineSprechendeQuote) {
    auto const* const v = comdare_anatomy_version_lines();
    ASSERT_NE(v, nullptr);

    pd::ConformanceResult const ok = pd::testat_mess_deklarations_konsistenz(v, cea::measurement_stamp_line_full_set());
    std::cout << "[d2] testat ok: " << ok.cases_passed << "/" << ok.cases_total << "\n";
    EXPECT_GT(ok.cases_total, 0u) << "ANTI-LEERLAUF: eine leere Population macht jede Aussage wahr";
    EXPECT_TRUE(ok.passed());

    // Ein Aufruf OHNE Erwartung erzeugt eine FEHLGESCHLAGENE Quote, nicht eine leere -- sonst waere das
    // Weglassen der Erwartung der bequemste Weg an diesem Gate vorbei.
    pd::ConformanceResult const leer = pd::testat_mess_deklarations_konsistenz(v, "");
    std::cout << "[d2] testat ohne Erwartung: " << leer.cases_passed << "/" << leer.cases_total
              << " first_fail=" << leer.first_fail << "\n";
    EXPECT_GT(leer.cases_total, 0u);
    EXPECT_FALSE(leer.passed());
    EXPECT_EQ(leer.first_fail, 1u) << "die erste Zusicherung IST die Existenz einer SOLL-Zeile";

    // Und die Quote sagt, WORAN es lag: bei blosser Zeilen-Abweichung fallen die ersten vier Zusicherungen
    // NICHT, nur die fuenfte.
    cea::AnatomyVersionLines const falsch =
        mach_pod(cea::kAnatomyVersionLinesLayout, kMessWallclock, sizeof(kMessWallclock) - 1,
                 cea::stamp_entries_ptr(kEntriesWallclock), kEntriesWallclock.size());
    pd::ConformanceResult const ab =
        pd::testat_mess_deklarations_konsistenz(&falsch, cea::measurement_stamp_line_full_set());
    EXPECT_FALSE(ab.passed());
    EXPECT_EQ(ab.first_fail, 5u) << "erst die Identitaets-Zusicherung faellt, nicht die Existenz-Zusicherungen";
    EXPECT_EQ(ab.cases_passed, ab.cases_total - 1u);
}

// =================================================================================================
// (5) UEBER DIE dlopen-GRENZE -- eine REALE .so ohne Mess-Deklaration wird abgewiesen
// =================================================================================================
//
// Die r5g-adhoc-Fixture ist eine ECHTE, gebaute Tier-.so, die den Loader besteht und deren std::map-Huelle
// das Funktions-Gate besteht -- sie traegt aber KEIN COMDARE_ANATOMY_VERSION_STAMP und exportiert das
// Probe-Symbol darum gar nicht. Genau daran zeigt sich, dass die beiden Gatter UNABHAENGIG sind: funktional
// tadellos UND als Mess-Quelle dieses Laufs unzulaessig.
TEST(D2MessKonsistenz, RealeSoOhneDeklarationWirdAbgewiesenObwohlDasFunktionsGateBesteht) {
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::filesystem::path const so = find_built_so(dir);
    ASSERT_FALSE(so.empty()) << "keine gebaute perm-.so im Fixture-Dir gefunden: " << dir;

    pd::PruefOutcome const oc = pd::run_so_conformance_gate(so, cea::measurement_stamp_line_full_set());
    std::cout << "[d2] so=" << so.filename().string() << " loaded=" << (oc.loaded ? 1 : 0) << " funktions_gate="
              << oc.gate.cases_passed << "/" << oc.gate.cases_total << " mess=" << pd::mess_konsistenz_meldung(oc.mess)
              << "\n";

    // Das FUNKTIONS-Gate besteht -- die Binary ist keine kaputte Huelle.
    EXPECT_TRUE(oc.loaded);
    EXPECT_GT(oc.gate.cases_total, 0u);
    EXPECT_TRUE(oc.gate.passed()) << "Vorbedingung dieses Bisses: die Fixture ist funktional in Ordnung";

    // Der MESS-Vertrag bricht -- und deshalb bricht das Gesamt-Ergebnis.
    EXPECT_FALSE(oc.mess.passed());
    EXPECT_EQ(oc.mess.status, pd::MessKonsistenzStatus::stempel_symbol_fehlt)
        << "diese Fixture traegt kein COMDARE_ANATOMY_VERSION_STAMP -- sie DEKLARIERT nichts";
    EXPECT_FALSE(oc.passed()) << "eine Binary ohne Mess-Deklaration darf NICHT als pruef-bestanden gelten, "
                                 "auch wenn ihre std::map-Huelle tadellos ist";
}

// Die nicht-existente .so bleibt, was sie war: nicht ladbar, nicht bestanden. Der neue Mess-Vertrag
// aendert daran nichts (er kommt erst NACH dem erfolgreichen Laden) -- festgehalten, damit ein spaeterer
// Umbau der Reihenfolge auffaellt.
TEST(D2MessKonsistenz, NichtLadbareSoBleibtNichtBestanden) {
    pd::PruefOutcome const oc = pd::run_so_conformance_gate("/nonexistent/perm.dll",
                                                            cea::measurement_stamp_line_full_set());
    EXPECT_FALSE(oc.loaded);
    EXPECT_FALSE(oc.passed());
}
