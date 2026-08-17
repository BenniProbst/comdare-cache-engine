// tests/unit/test_flaeche3_deckung_ceb_aus_tier_an.cpp -- FLAECHE-3-DECKUNG: CEB=AUS / Tier=AN
// (KON25-02-Deckungsluecke, Task #20).
//
// ------------------------------------------------------------------------------------------------
// WAS DIESE TU IST -- und warum es sie bisher nicht gab
// ------------------------------------------------------------------------------------------------
// Flaeche 3 (der measurement-Durchstich = die Mess-Naht NAHT-1, Identitaets-Kopfblock in
// anatomy/mess_visitor_abi.hpp) ist ZWEISEITIG aktiviert: CEB UND Tier-Binary. Der Testbestand
// deckte drei der vier Gate-Kombinationen:
//   CEB=AN /Tier=AN   test_naht1_mess_visitor_biss (a)/(b)     -- die Naht traegt echte Werte
//   CEB=AN /Tier=AUS  test_naht1_mess_visitor_biss (c)/(d2)    -- keine Flaeche, kein Symbol
//   CEB=AUS ohne Modul test_a8s4_release_pfad_neutralitaet     -- eine TU, kein dlopen, kein Dock
// Der Zweig CEB=AUS/Tier=AN -- eine funktional-only uebersetzte CacheEngineBuilder VOR einem
// messfaehig gebauten Tier-Modul -- wurde von KEINEM Testziel uebersetzt (KON25-02). Der
// OFF-Zweig von SearchAlgorithmDock::measure (search_algorithm_dock.hpp, #if !COMDARE_
// MEASUREMENT_ON) war damit toter Text im Testnetz. Diese TU uebersetzt ihn und misst ihn.
//
// ------------------------------------------------------------------------------------------------
// BEIDE RICHTUNGEN, JE MIT GEGENPROBE (T-1)
// ------------------------------------------------------------------------------------------------
// RICHTUNG 1 (CEB=AUS): der OFF-Host misst NICHT -- rc == mess_deaktiviert, NULL Bytes Ausgabe,
//   obwohl der Test vorher N Ops in das Tier gegeben hat. Die GEGENPROBE, die die Aussage scharf
//   macht: DIESELBE geladene Binary TRAEGT die Mess-Flaeche (dynamic_cast != nullptr, Symbol in
//   der Tabelle). Das Schweigen kommt also von der CEB-Seite, nicht vom Tier -- "es gab nichts zu
//   messen" ist widerlegt.
//   OBJEKT-DISKRIMINATOR: rc == mess_deaktiviert BEI vorhandener Flaeche UND Legende=AN ist im
//   ON-Kompilat UNERREICHBAR (dort ergaebe diese Lage Beidseitig_an -> Messung -> dock_status_ok;
//   mess_deaktiviert verlangte dort Tier_aus = Flaeche FEHLT + Legende AUS). Der Rueckgabewert
//   beweist damit am Objekt, dass hier wirklich der OFF-Zweig uebersetzt wurde.
// RICHTUNG 2 (Tier=AN): das Tier-Modul ist messfaehig AM OBJEKT -- Flaeche per dynamic_cast, Symbol
//   tier_measure_accept per nm in der .so. GEGENPROBE: die AUS-Schwester (gleicher Quelltext, Gates
//   aus) zeigt nullptr UND kein Symbol -- die Probe unterscheidet also wirklich, ein Nichtfund ist
//   erst dadurch ein Befund.
//
// T-1-KOEDER-PROTOKOLL (rot zuerst, am Objekt gefahren, literale Ausgaben im Task-#20-Bericht):
//   R1-Koeder: dieselben Richtung-1-Assertions in einer ON-uebersetzten Wirts-TU -> ROT
//              (rc=0 "ok" statt 6 "mess_deaktiviert", CSV nicht leer).
//   R2-Koeder: dieselben Richtung-2-Assertions gegen die AUS-Schwester gerichtet -> ROT
//              (Flaeche nullptr, Symbol NEIN).
//
// PRAEPARATION: exakt der a8s4-/r3-Weg. Die Wurzel-CMakeLists setzt COMDARE_MEASUREMENT_ON=1
// GLOBAL (add_compile_definitions); ein Target kann eine Verzeichnis-Definition nicht abwaehlen.
// Der #undef VOR jedem Include ist der im Baum sanktionierte, portable Weg und stellt den real
// erreichbaren Release-Baumodus her (COMDARE_RELEASE_MODE=ON) -- kein Kunstgriff.
//
// ASCII-only.

#undef COMDARE_MEASUREMENT_ON
#undef COMDARE_CE_ENABLE_STATISTICS
#undef COMDARE_EXPERIMENT_MODE_ON

#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
#error "FLAECHE-3-DECKUNG: diese TU MUSS als CEB=AUS uebersetzen (COMDARE_MEASUREMENT_ON aus)."
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
#error "FLAECHE-3-DECKUNG: COMDARE_CE_ENABLE_STATISTICS muss in der CEB=AUS-TU aus sein."
#endif

// Diese Include-Menge IST der Prueffall: search_algorithm_dock.hpp zieht genus_mess_naht.hpp NUR
// unter COMDARE_MEASUREMENT_ON -- in dieser TU existiert die CEB-Messmaschinerie nicht einmal als
// Deklaration (der #error-Riegel dort wuerde jeden Schmuggel laut machen; Text-Wache unten).
#include <anatomy/observable_tier.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <builder/pruef_dock/pruef_dock.hpp>
#include <builder/pruef_dock/search_algorithm_dock.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace pd     = ::comdare::cache_engine::builder::pruef_dock;

namespace {

// K13: der Wuerfel -- frisch aus /dev/urandom, je Lauf neu, Wert steht im Protokoll
// (Muster aus test_naht1_mess_visitor_biss.cpp; ein festes Erwartungspaar koennte zufaellig passen).
[[nodiscard]] std::uint64_t urandom_wert(std::uint64_t min, std::uint64_t spanne) {
    std::uint64_t roh = 0;
    std::ifstream f("/dev/urandom", std::ios::binary);
    EXPECT_TRUE(f.good()) << "WERKZEUG-DEFEKT: /dev/urandom nicht lesbar -- der Koeder kann nicht wuerfeln.";
    f.read(reinterpret_cast<char*>(&roh), sizeof(roh));
    return min + (roh % spanne);
}

/// Symboltabelle einer .so nach einem Namensteil absuchen (nm -D --defined-only) -- die
/// Zero-Cost-/Vorhandenseins-Aussage AM OBJEKT (Muster aus test_naht1_mess_visitor_biss.cpp).
[[nodiscard]] bool symbol_vorhanden(std::string const& so_pfad, std::string const& teilname) {
    std::string const cmd = "nm -D --defined-only '" + so_pfad + "' 2>/dev/null | grep -c '" + teilname + "'";
    FILE*             p   = popen(cmd.c_str(), "r");
    if (p == nullptr) return false;
    char        buf[64] = {};
    std::size_t n       = std::fread(buf, 1, sizeof(buf) - 1, p);
    pclose(p);
    buf[n] = '\0';
    return std::atoi(buf) > 0;
}

// Pfade als COMPILE-DEFINITIONS (Praezedenz: test_naht1_mess_visitor_biss -- ctest-Argumente koennen
// still leer werden, Compile-Definitionen nicht).
#if !defined(COMDARE_F3V_AN_SO) || !defined(COMDARE_F3V_AUS_SO) || !defined(COMDARE_F3V_AN_DIR) ||                     \
    !defined(COMDARE_F3V_AUS_DIR) || !defined(COMDARE_F3V_CEB_NAHT_SRC)
#error "FLAECHE-3-DECKUNG: die Schwester-Pfade/Quellpfade fehlen -- der Test kann nicht am Objekt messen."
#endif
std::string const g_an_so{COMDARE_F3V_AN_SO};
std::string const g_aus_so{COMDARE_F3V_AUS_SO};
std::string const g_an_dir{COMDARE_F3V_AN_DIR};
std::string const g_aus_dir{COMDARE_F3V_AUS_DIR};
std::string const g_naht_src{COMDARE_F3V_CEB_NAHT_SRC};

} // namespace

// ================================================================================================
// RICHTUNG 1: CEB=AUS -- der OFF-Host misst NICHT, obwohl das Tier messen KOENNTE
// ================================================================================================
TEST(Flaeche3Deckung, CebAusMisstNichtTrotzMessfaehigemTier) {
    ASSERT_FALSE(g_an_dir.empty());
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_an_dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);
    auto* base = handles[0].anatomy();
    ASSERT_NE(base, nullptr);

    // GEGENPROBE ZUERST (macht Richtung 1 scharf): das GELADENE Tier TRAEGT die Mess-Flaeche.
    // Ohne diesen Beleg waere "0 Bytes Ausgabe" auch mit einem messunfaehigen Tier erklaerbar.
    auto* obs = dynamic_cast<ana::IObservableTier*>(base);
    ASSERT_NE(obs, nullptr) << "GEGENPROBE GESCHEITERT: die MESS-AN-Schwester traegt keine Mess-Flaeche -- "
                               "dann sagt das Schweigen der CEB nichts ueber die CEB-Seite.";
    bool const an_symbol = symbol_vorhanden(g_an_so, "tier_measure_accept");
    EXPECT_TRUE(an_symbol) << "GEGENPROBE GESCHEITERT: tier_measure_accept fehlt in der MESS-AN-.so.";

    // Der Host treibt REAL Ops in das Tier (der funktionale Antrieb lebt auch im OFF-Host) --
    // damit "0 Bytes Mess-Ausgabe" nicht an einem leeren Tier liegt, sondern an der CEB-Seite.
    std::uint64_t const N = urandom_wert(64, 192);
    std::uint64_t const M = urandom_wert(32, 160);
    std::printf("[NENNER] gewuerfelt aus /dev/urandom: N_inserts=%llu  M_lookups=%llu\n",
                static_cast<unsigned long long>(N), static_cast<unsigned long long>(M));
    auto* drive = dynamic_cast<ana::IDriveableTier*>(base);
    ASSERT_NE(drive, nullptr);
    std::uint64_t getrieben = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (drive->tier_insert(i, i * 7u + 1u)) ++getrieben;
    for (std::uint64_t i = 0; i < M; ++i) {
        std::uint64_t out = 0;
        (void)drive->tier_lookup(i % (N == 0 ? 1 : N), &out);
    }
    EXPECT_EQ(getrieben, N) << "Funktionalpfad des OFF-Hosts gestoert -- CEB=AUS heisst nicht Funktion=AUS.";

    // DIE EIGENTLICHE AUSSAGE: der offizielle CEB-Messweg (das Dock) misst NICHT.
    pd::SearchAlgorithmDock     dock;
    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {8};
    opts.lookups_per_checkpoint = 4;
    opts.deletes_per_checkpoint = 0;
    // Alle drei Legenden-Lagen: der OFF-Zweig antwortet VOR der Legenden-Weiche -- die CEB-Haelfte
    // der UND-Bedingung hat Vorrang. Jede Lage steht mit rc UND Klartext-Namen im Protokoll.
    for (int legende : {1, 0, -1}) {
        std::string csv, json;
        opts.mess_legende_erwartung = legende;
        int const rc                = dock.measure(handles[0], opts, csv, json);
        std::printf("[CEB=AUS] measure(Legende=%2d) gegen MESS-AN-Tier -> rc=%d (%s), csv=%zu B, json=%zu B\n",
                    legende, rc, std::string(pd::dock_status_name(rc)).c_str(), csv.size(), json.size());
        EXPECT_EQ(rc, pd::dock_status_mess_deaktiviert)
            << "CEB=AUS hat gemessen oder falsch klassifiziert -- die CEB-Haelfte der UND-Bedingung wirkt nicht.";
        // NULL Bytes, nicht "leer-artig": eine nicht messende CEB darf keine Mess-Ausgabe erzeugen.
        EXPECT_EQ(csv.size(), 0u) << "CEB=AUS erzeugte CSV-Bytes -- es wurde doch gemessen.";
        EXPECT_EQ(json.size(), 0u) << "CEB=AUS erzeugte JSON-Bytes -- es wurde doch gemessen.";
    }
    std::printf("[NENNER] %llu Inserts + %llu Lookups getrieben, 0 Byte Mess-Ausgabe ueber das Dock\n",
                static_cast<unsigned long long>(N), static_cast<unsigned long long>(M));

    // OBJEKT-DISKRIMINATOR (belegt: der OFF-ZWEIG wurde uebersetzt, nicht der ON-Zweig anders
    // parametriert): Flaeche VORHANDEN + Legende=AN + rc==mess_deaktiviert ist im ON-Kompilat
    // unerreichbar (dort: Beidseitig_an -> Messung -> ok; Tier_aus verlangte Flaeche FEHLT).
    // Die EXPECTs oben haben genau diese Lage festgestellt -- hier steht sie als benannter Satz.
    SUCCEED() << "OFF-Zweig am Objekt belegt: vorhandene Flaeche + Legende=AN ergab mess_deaktiviert.";
}

// ================================================================================================
// RICHTUNG 2: Tier=AN -- die Mess-Flaeche ist AM OBJEKT da; die AUS-Schwester unterscheidet
// ================================================================================================
TEST(Flaeche3Deckung, TierAnTraegtDieFlaecheAmObjektUndDieProbeUnterscheidet) {
    // AN-Schwester: Flaeche + Symbol JA.
    std::vector<loader::AnatomyModuleHandle> an_handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_an_dir, an_handles), loader::status_ok);
    ASSERT_GE(an_handles.size(), 1u);
    auto* an_obs = dynamic_cast<ana::IObservableTier*>(an_handles[0].anatomy());
    EXPECT_NE(an_obs, nullptr) << "Tier=AN ohne Mess-Flaeche -- der Durchstich fehlt, wo er sein muesste.";
    bool const an_hat = symbol_vorhanden(g_an_so, "tier_measure_accept");
    // GEGENPROBE des Werkzeugs: erst wenn nm in der AN-.so findet, sagt ein Fehlen anderswo etwas.
    EXPECT_TRUE(an_hat) << "GEGENPROBE GESCHEITERT: nm findet tier_measure_accept nicht einmal in der "
                           "MESS-AN-Schwester -- dann ist jede Abwesenheits-Aussage wertlos.";

    // AUS-Schwester (der Distinguisher): Flaeche + Symbol NEIN, Funktion JA.
    std::vector<loader::AnatomyModuleHandle> aus_handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_aus_dir, aus_handles), loader::status_ok);
    ASSERT_GE(aus_handles.size(), 1u);
    auto* aus_base = aus_handles[0].anatomy();
    ASSERT_NE(aus_base, nullptr);
    EXPECT_EQ(dynamic_cast<ana::IObservableTier*>(aus_base), nullptr)
        << "Die AUS-Schwester traegt eine Mess-Flaeche -- die Probe unterscheidet nicht mehr.";
    bool const aus_hat = symbol_vorhanden(g_aus_so, "tier_measure_accept");
    EXPECT_FALSE(aus_hat) << "tier_measure_accept steht in der MESS-AUS-Binary -- Zero-Cost verletzt.";
    std::printf("[OBJEKT] nm -D tier_measure_accept: MESS-AN=%s  MESS-AUS=%s\n", an_hat ? "JA" : "NEIN",
                aus_hat ? "JA" : "NEIN");
}

// ================================================================================================
// TEXT-WACHE (a8s4-Praezedenz): der compile-harte Riegel der CEB-Messmaschinerie steht im Quelltext
// ================================================================================================
// Warum eine Text-Wache und kein Include-Versuch: genus_mess_naht.hpp in DIESER TU zu includieren
// waere ein Kompilierfehler (genau das ist der Riegel) -- ein Test kann seinen eigenen Nichtbau
// nicht ausfuehren. Also wird der Riegel dort geprueft, wo er lebt: im Quelltext, wertbasiert.
TEST(Flaeche3Deckung, CebMessmaschinerieTraegtDenCompileHartenRiegel) {
    std::ifstream f(g_naht_src);
    ASSERT_TRUE(f.good()) << "Quelltext nicht lesbar: " << g_naht_src;
    std::stringstream ss;
    ss << f.rdbuf();
    std::string const quelle = ss.str();
    ASSERT_FALSE(quelle.empty());

    // Der wertbasierte Guard (-DCOMDARE_MEASUREMENT_ON=0 muss ausloesen) UND der #error dahinter.
    EXPECT_NE(quelle.find("#if !defined(COMDARE_MEASUREMENT_ON) || !COMDARE_MEASUREMENT_ON"), std::string::npos)
        << "Der wertbasierte OFF-Guard fehlt in " << g_naht_src;
    EXPECT_NE(quelle.find("#error"), std::string::npos)
        << "Der compile-harte Riegel (#error) fehlt in " << g_naht_src;
    // Und die Vereinigungs-Identitaet ist dort angekommen (Flaeche-3-Vokabular an der gebauten Naht).
    EXPECT_NE(quelle.find("FLAECHE 3"), std::string::npos)
        << "Die KON25-02-Vereinigung (Flaeche-3-Vokabular) fehlt in der CEB-Seite der Naht.";
    std::printf("[TEXT-WACHE] OFF-Guard + #error + FLAECHE-3-Anker in %s belegt (%zu Zeichen gelesen)\n",
                g_naht_src.c_str(), quelle.size());
}
