// tests/unit/test_naht1_mess_visitor_biss.cpp -- DER KOEDER DER MESS-NAHT (NAHT-1, 09.08.2026).
// Die Naht IST Flaeche 3, der measurement-Durchstich (KON25-02-Vereinigung; Identitaets-Kopfblock
// in anatomy/mess_visitor_abi.hpp). Dieser Koeder deckt die Host-Seite CEB=AN (Tier=AN und
// Tier=AUS); die Schwester-Kombination CEB=AUS/Tier=AN deckt
// tests/unit/test_flaeche3_deckung_ceb_aus_tier_an.cpp.
//
// ------------------------------------------------------------------------------------------------
// WAS DIESER TEST NICHT PRUEFT -- und warum das der wichtigste Satz hier ist
// ------------------------------------------------------------------------------------------------
// Er prueft NIRGENDS, "dass ein Visitor uebergeben wurde". Anwesenheit ist keine Aussage (T-2): ein
// Visitor, der uebergeben und nie gerufen wird, ein Visitor, der gerufen wird und Nullen traegt, und
// ein Visitor, der die richtigen Werte traegt, sind an einem Anwesenheits-Check NICHT unterscheidbar.
// Geprueft wird ausschliesslich, ob ein ECHTER, VORHER BEKANNTER WERT durch die Naht kommt.
//
// ------------------------------------------------------------------------------------------------
// DAS ORAKEL IST FREMD (T-5)
// ------------------------------------------------------------------------------------------------
// Der Nenner entsteht NICHT im Pruefling. Der Test fuehrt selbst Buch darueber, wie viele Inserts und
// Lookups er in das Tier gegeben hat (das HOST-OP-BUCH), und vergleicht das mit dem, was das Tier
// durch die Naht BERICHTET. Zwei unabhaengige Zaehlungen derselben Sache: der Treiber zaehlt, was er
// ausgab, das Tier zaehlt, was es sah. Nur wenn beide uebereinstimmen, ist die Naht in Ordnung.
//
// ------------------------------------------------------------------------------------------------
// DER KOEDER MUSS BEISSEN (K13) -- N und M sind ZUFAELLIG, FRISCH, UND SIE STEHEN IN DER AUSGABE
// ------------------------------------------------------------------------------------------------
// N (Inserts) und M (Lookups) werden je Lauf frisch aus /dev/urandom abgeleitet und ins Protokoll
// geschrieben. Ein fest verdrahtetes Erwartungspaar koennte durch Zufall stimmen; ein je Lauf neu
// gewuerfeltes kann das nicht. Und jede Zahl in der Ausgabe traegt ihren NENNER -- nie eine nackte
// Zahl (Pruefung 1).
//
// ------------------------------------------------------------------------------------------------
// BEIDE RICHTUNGEN (die Schwesterpflicht)
// ------------------------------------------------------------------------------------------------
// Zwei .so aus EINEM Quelltext (naht1_visitor_module.cpp), identische Komposition, nur die Mess-Gates
// verschieden. Die AN-Schwester MUSS beissen; die AUS-Schwester DARF NICHT. Belegt wird die
// AUS-Seite nicht durch eine Behauptung, sondern am Objekt: die Mess-Flaeche fehlt (dynamic_cast
// nullptr) UND das Symbol tier_measure_accept steht nicht in der Symboltabelle der Binary.
//
// ASCII-only.

#include <anatomy/observable_tier.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <builder/pruef_dock/dock_error_classification.hpp>
#include <builder/pruef_dock/genus_mess_naht.hpp>
#include <builder/pruef_dock/pruef_dock.hpp>
#include <builder/pruef_dock/search_algorithm_dock.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace pd     = ::comdare::cache_engine::builder::pruef_dock;
namespace cem    = ::comdare::cache_engine::measurement;

namespace {

// ------------------------------------------------------------------------------------------------
// Die Test-Senke: sie ZAEHLT und SPEICHERT, statt zu behaupten.
// ------------------------------------------------------------------------------------------------
// Sie ist bewusst NICHT die SnapshotSink des Produktivpfads: ein Koeder, der die Senke des Pruef-
// lings benutzt, teilt dessen Fehler. Diese hier fuehrt einen EIGENEN Zustandsautomaten und einen
// EIGENEN Ereignis-Zaehler -- so faellt auch ein Ordnungs-Defekt auf, den die Produktivsenke
// vielleicht verzeiht.
struct ZaehlendeSenke {
    std::uint64_t                             e2_begins           = 0;
    std::uint64_t                             e2_ends             = 0;
    std::uint64_t                             zeilen              = 0;
    std::uint64_t                             reroutes            = 0;
    std::uint64_t                             ereignisse          = 0; // ALLE visit_*-Aufrufe
    std::uint64_t                             axis_count_im_begin = 0;
    std::uint64_t                             fill_level          = 0;
    std::uint64_t                             genus_id            = 0;
    std::uint64_t                             fn_id               = 0;
    std::uint64_t                             observable_axes     = 0;
    std::uint64_t                             filled_axes         = 0;
    std::uint64_t                             batches             = 0;
    std::int64_t                              seg_framework_ns    = 0;
    std::int64_t                              seg_run_total_ns    = 0;
    bool                                      ordnung_gebrochen   = false;
    std::vector<std::array<std::uint64_t, 8>> felder{};
    std::vector<std::int64_t>                 seg_ns{};

    void e2_begin(std::uint64_t g, std::uint64_t fn, std::uint64_t fl, std::uint64_t ac) noexcept {
        if (e2_begins != e2_ends) ordnung_gebrochen = true; // begin ohne vorheriges end
        ++e2_begins;
        ++ereignisse;
        genus_id            = g;
        fn_id               = fn;
        fill_level          = fl;
        axis_count_im_begin = ac;
        felder.clear();
        seg_ns.clear();
        zeilen = 0;
    }
    void e1_axis_row(std::uint64_t idx, std::uint64_t const* f, std::int64_t s) noexcept {
        // AUFSTEIGEND, genau einmal je Achse, nur INNERHALB eines offenen Rahmens.
        if (e2_begins == e2_ends || idx != zeilen || f == nullptr) ordnung_gebrochen = true;
        ++zeilen;
        ++ereignisse;
        std::array<std::uint64_t, 8> row{};
        if (f != nullptr)
            for (std::size_t i = 0; i < 8; ++i) row[i] = f[i];
        felder.push_back(row);
        seg_ns.push_back(s);
    }
    void hybrid_reroute(std::uint64_t, std::uint64_t, std::int64_t) noexcept {
        ++reroutes;
        ++ereignisse;
    }
    void e2_end(std::uint64_t oa, std::uint64_t fa, std::uint64_t b, std::int64_t fw, std::int64_t tot) noexcept {
        if (e2_begins == e2_ends) ordnung_gebrochen = true; // end ohne begin
        ++e2_ends;
        ++ereignisse;
        observable_axes  = oa;
        filled_axes      = fa;
        batches          = b;
        seg_framework_ns = fw;
        seg_run_total_ns = tot;
    }
    [[nodiscard]] bool rahmen_vollstaendig() const noexcept {
        return e2_begins == 1 && e2_ends == 1 && zeilen == axis_count_im_begin && !ordnung_gebrochen;
    }
};
static_assert(ana::GenusMessSink<ZaehlendeSenke>, "die Test-Senke muss die Concept-Flaeche treffen");

// ------------------------------------------------------------------------------------------------
// K13: der Wuerfel. Frisch aus /dev/urandom, je Lauf neu, und der Wert steht im Protokoll.
// ------------------------------------------------------------------------------------------------
[[nodiscard]] std::uint64_t urandom_wert(std::uint64_t min, std::uint64_t spanne) {
    std::uint64_t roh = 0;
    std::ifstream f("/dev/urandom", std::ios::binary);
    // Faellt /dev/urandom aus, ist das ein WERKZEUG-Defekt und kein Mess-Ergebnis -- der Test sagt
    // das ausdruecklich, statt still auf eine Konstante zurueckzufallen (die dann immer "passt").
    EXPECT_TRUE(f.good()) << "WERKZEUG-DEFEKT: /dev/urandom nicht lesbar -- der Koeder kann nicht wuerfeln.";
    f.read(reinterpret_cast<char*>(&roh), sizeof(roh));
    return min + (roh % spanne);
}

/// Symboltabelle einer .so nach einem Namensteil absuchen (nm -D --defined-only).
/// Das ist die Zero-Cost-Aussage AM OBJEKT: nicht "wir haben es nicht kompiliert", sondern
/// "es steht nicht drin".
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

// Die vier Pfade kommen als COMPILE-DEFINITIONS vom Bau (s. tests/unit/CMakeLists.txt). Sie sind
// damit Teil des Kompilats und koennen nicht durch eine vergessene ctest-Argumentliste leer werden --
// genau der Werkzeug-Defekt, an dem die erste Fassung dieses Tests falsch rot wurde.
#if !defined(COMDARE_NAHT1_AN_SO) || !defined(COMDARE_NAHT1_AUS_SO) || !defined(COMDARE_NAHT1_AN_DIR) ||               \
    !defined(COMDARE_NAHT1_AUS_DIR)
#error "NAHT-1 BISS: die vier Schwester-Pfade fehlen -- der Test kann nicht am Objekt messen."
#endif
std::string const g_an_so{COMDARE_NAHT1_AN_SO};
std::string const g_aus_so{COMDARE_NAHT1_AUS_SO};
std::string const g_an_dir{COMDARE_NAHT1_AN_DIR};
std::string const g_aus_dir{COMDARE_NAHT1_AUS_DIR};

} // namespace

// ================================================================================================
// (a) WERT-WAHRHEIT: kommt ein ECHTER, VORHER BEKANNTER WERT durch die Naht?
// ================================================================================================
TEST(Naht1MessVisitor, WertWahrheitDurchDieNahtGegenDasHostOpBuch) {
    ASSERT_FALSE(g_an_dir.empty()) << "--an-dir fehlt";
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_an_dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);
    auto* tier = dynamic_cast<ana::IObservableTier*>(handles[0].anatomy());
    ASSERT_NE(tier, nullptr) << "MESS-AN-Schwester ohne Mess-Flaeche -- die Naht fehlt, wo sie sein muesste.";

    // --- K13: N und M frisch gewuerfelt, BEIDE im Protokoll -----------------------------------
    std::uint64_t const N = urandom_wert(64, 192); // Inserts
    std::uint64_t const M = urandom_wert(32, 160); // Lookups (auf existierende Keys)
    std::printf("[NENNER] gewuerfelt aus /dev/urandom: N_inserts=%llu  M_lookups=%llu\n",
                static_cast<unsigned long long>(N), static_cast<unsigned long long>(M));

    tier->tier_clear();
    tier->tier_reset_statistics();

    // --- DAS HOST-OP-BUCH: der Test zaehlt, was ER ausgibt (fremde Quelle zum Pruefling) -------
    std::uint64_t buch_inserts = 0;
    std::uint64_t buch_lookups = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (tier->tier_insert(i, i * 7u + 1u)) ++buch_inserts;
    for (std::uint64_t i = 0; i < M; ++i) {
        std::uint64_t out = 0;
        (void)tier->tier_lookup(i % (N == 0 ? 1 : N), &out);
        ++buch_lookups;
    }

    // --- DIE UEBERGABE: die Senke geht HINEIN, das Tier berichtet ------------------------------
    ZaehlendeSenke senke;
    pd::genus_measure_into(*tier, senke);

    // --- 1. Rahmen und Nenner ------------------------------------------------------------------
    EXPECT_TRUE(senke.rahmen_vollstaendig())
        << "Ordnungsverstoss im Ereignis-Strom: begins=" << senke.e2_begins << " ends=" << senke.e2_ends
        << " zeilen=" << senke.zeilen << " erwartet=" << senke.axis_count_im_begin;
    EXPECT_EQ(senke.axis_count_im_begin, ana::kV3AxisCount);
    EXPECT_EQ(senke.zeilen, ana::kV3AxisCount);
    // 20 Ereignisse = 1 begin + 18 Zeilen + 1 end. Der NENNER steht daneben, nicht nur die Zahl.
    EXPECT_EQ(senke.ereignisse, 1u + ana::kV3AxisCount + 1u)
        << "Ereignis-Zahl gegen den Nenner (1 begin + " << ana::kV3AxisCount << " Zeilen + 1 end)";
    std::printf("[NENNER] Ereignisse=%llu von erwarteten %llu (1 begin + %zu Zeilen + 1 end)\n",
                static_cast<unsigned long long>(senke.ereignisse),
                static_cast<unsigned long long>(1u + ana::kV3AxisCount + 1u), ana::kV3AxisCount);

    // --- 2. DIE EIGENTLICHE AUSSAGE: der Wert gegen das Host-Op-Buch ---------------------------
    // T0 = search_algo. Feld 3 = total_insert_count, Feld 0 = total_lookup_count (kV3AxisSchema).
    ASSERT_GE(senke.felder.size(), 1u);
    std::uint64_t const berichtet_inserts = senke.felder[0][3];
    std::uint64_t const berichtet_lookups = senke.felder[0][0];
    std::printf("[WERT] T0.insert berichtet=%llu  Host-Op-Buch=%llu   |   T0.lookup berichtet=%llu  "
                "Host-Op-Buch=%llu\n",
                static_cast<unsigned long long>(berichtet_inserts), static_cast<unsigned long long>(buch_inserts),
                static_cast<unsigned long long>(berichtet_lookups), static_cast<unsigned long long>(buch_lookups));
    // EXAKT, nicht "> 0". Ein "> 0" wuerde jeden Mutanten ueberleben lassen, der nur EINEN Wert traegt.
    EXPECT_EQ(berichtet_inserts, buch_inserts) << "Die Naht traegt eine ANDERE Insert-Zahl als der Host ausgab.";
    EXPECT_EQ(berichtet_lookups, buch_lookups) << "Die Naht traegt eine ANDERE Lookup-Zahl als der Host ausgab.";
    EXPECT_EQ(senke.fill_level, N) << "Der Fuellstand im e2_begin muss die N eingefuegten Keys tragen.";
    EXPECT_EQ(senke.fn_id, ana::genus_fn_id::kCoreCheckpoint);

    // --- 3. Die vierte Ebene hat (noch) keinen Emitter -- GEMESSEN, nicht zugesagt -------------
    EXPECT_EQ(senke.reroutes, 0u) << "Ein SearchAlgorithm-Tier ist kein Router: der Hybrid-Slot hat bis HY-A2 "
                                     "keinen Emitter. Wenn hier etwas ankommt, ist der Vertrag verletzt.";
}

// ================================================================================================
// (b) DECKUNGS-INVARIANTE: ueberlebt P-MD3 die Transport-Inversion?
// ================================================================================================
TEST(Naht1MessVisitor, DeckungsInvarianteUeberlebtDieNaht) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_an_dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);
    auto* tier = dynamic_cast<ana::IObservableTier*>(handles[0].anatomy());
    ASSERT_NE(tier, nullptr);

    std::uint64_t const N = urandom_wert(64, 192);
    std::printf("[NENNER] Deckungs-Probe mit N_inserts=%llu (aus /dev/urandom)\n", static_cast<unsigned long long>(N));
    tier->tier_clear();
    tier->tier_reset_statistics();
    for (std::uint64_t i = 0; i < N; ++i) (void)tier->tier_insert(i, i + 1u);

    ZaehlendeSenke senke;
    pd::genus_measure_into(*tier, senke);
    ASSERT_TRUE(senke.rahmen_vollstaendig());

    std::int64_t summe_seg = 0;
    for (auto s : senke.seg_ns) summe_seg += s;
    // P-MD3: Sum(seg_ns) + seg_framework_ns == seg_run_total_ns. Der Rest ist BENANNT, nicht verschwiegen.
    std::printf("[NENNER] P-MD3: Sum(seg_ns[0..%zu])=%lld + framework=%lld == run_total=%lld ?\n",
                ana::kV3AxisCount - 1, static_cast<long long>(summe_seg),
                static_cast<long long>(senke.seg_framework_ns), static_cast<long long>(senke.seg_run_total_ns));
    EXPECT_EQ(summe_seg + senke.seg_framework_ns, senke.seg_run_total_ns)
        << "Die Deckungs-Invariante ist beim Transport ueber die Naht gerissen.";

    // (e) MUTANTEN-PROBE, die genau HIER greift: ein Mutant, der EINE e1_axis_row unterschlaegt,
    // faellt nicht ueber einen Anwesenheits-Check, sondern ueber diese Summe -- weil deren seg_ns
    // dann in der Deckung fehlt. Wir rechnen die Gegenprobe explizit nach.
    ASSERT_EQ(senke.seg_ns.size(), ana::kV3AxisCount);
    std::int64_t const summe_ohne_letzte = summe_seg - senke.seg_ns.back();
    EXPECT_NE(summe_ohne_letzte + senke.seg_framework_ns, senke.seg_run_total_ns)
        << "GEGENPROBE GESCHEITERT: eine fehlende Achsen-Zeile wuerde die Deckungs-Invariante NICHT "
           "verletzen -- dann ist diese Invariante als Mutanten-Wache wertlos (seg_ns der letzten "
           "Achse ist offenbar 0).";
}

// ================================================================================================
// (c) DIE SCHWESTERPFLICHT: die AUS-Seite darf NICHT beissen -- am Objekt, nicht per Zusage
// ================================================================================================
TEST(Naht1MessVisitor, AusSchwesterHatKeineMessFlaecheUndKeinSymbol) {
    ASSERT_FALSE(g_aus_dir.empty()) << "--aus-dir fehlt";
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_aus_dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);
    auto* base = handles[0].anatomy();
    ASSERT_NE(base, nullptr) << "Die AUS-Schwester muss FUNKTIONAL weiterhin laden -- nur die Messung faellt weg.";

    // 1. Die Mess-Flaeche ist WEG. Kein Visitor kann uebergeben werden, weil es nichts gibt, dem man
    //    ihn uebergeben koennte.
    auto* tier = dynamic_cast<ana::IObservableTier*>(base);
    EXPECT_EQ(tier, nullptr) << "MESS-AUS-Schwester traegt trotzdem IObservableTier -- die Gate-Vererbung wirkt nicht.";

    // 2. Der Antrieb lebt weiter: die Binary ist funktional-only, nicht kaputt.
    auto* drive = dynamic_cast<ana::IDriveableTier*>(base);
    ASSERT_NE(drive, nullptr) << "Die AUS-Schwester muss den funktionalen Antrieb behalten (IDriveableTier).";
    EXPECT_TRUE(drive->tier_insert(41u, 42u));
    std::uint64_t v = 0;
    EXPECT_TRUE(drive->tier_lookup(41u, &v));
    EXPECT_EQ(v, 42u) << "Funktionalpfad der AUS-Schwester gestoert -- Zero-Cost heisst nicht Zero-Funktion.";

    // 3. ZERO-COST AM OBJEKT: das Symbol steht NICHT in der Binary. Das ist die Aussage, die eine
    //    Behauptung nicht ersetzen kann -- "wir kompilieren es nicht" ist eine Absicht, "es ist nicht
    //    drin" ist ein Befund.
    ASSERT_FALSE(g_an_so.empty());
    ASSERT_FALSE(g_aus_so.empty());
    bool const an_hat  = symbol_vorhanden(g_an_so, "tier_measure_accept");
    bool const aus_hat = symbol_vorhanden(g_aus_so, "tier_measure_accept");
    std::printf("[OBJEKT] nm -D: tier_measure_accept  in MESS-AN=%s  in MESS-AUS=%s\n", an_hat ? "JA" : "NEIN",
                aus_hat ? "JA" : "NEIN");
    // Die Gegenprobe zuerst: findet das Werkzeug ueberhaupt etwas? Ein "nicht gefunden" ist erst ein
    // Befund, wenn belegt ist, dass das Verfahren findet.
    EXPECT_TRUE(an_hat) << "GEGENPROBE GESCHEITERT: nm findet das Symbol nicht einmal in der MESS-AN-Schwester "
                           "-- dann sagt sein Fehlen in der AUS-Schwester gar nichts.";
    EXPECT_FALSE(aus_hat) << "ZERO-COST VERLETZT: tier_measure_accept steht in der MESS-AUS-Binary.";
}

// ================================================================================================
// (d) GEGENEINGANG (T-4): der Widerspruch muss hart auffallen, nie gruen degradieren
// ================================================================================================
TEST(Naht1MessVisitor, GateWiderspruchIstEinDefektUndKeinDegrade) {
    // Die vier Zustaende der zweiseitigen Aktivierung, als Tabelle durchgefahren.
    using B = pd::MessGateBefund;
    // Flaeche da + Legende sagt AN  -> messen
    EXPECT_EQ(pd::mess_gate_befund(true, true, true), B::Beidseitig_an);
    // Flaeche da + KEINE Legende    -> messen (Wissensluecke ist kein Defekt)
    EXPECT_EQ(pd::mess_gate_befund(true, false, false), B::Beidseitig_an);
    // Flaeche da + Legende sagt AUS -> WIDERSPRUCH (es wird gemessen, die Provenienz sagt Nein)
    EXPECT_EQ(pd::mess_gate_befund(true, true, false), B::Widerspruch);
    // Flaeche weg + Legende sagt AN -> WIDERSPRUCH (die teuerste Klasse: gefaelschte Erwartung)
    EXPECT_EQ(pd::mess_gate_befund(false, true, true), B::Widerspruch);
    // Flaeche weg + Legende sagt AUS-> ehrlich deaktiviert
    EXPECT_EQ(pd::mess_gate_befund(false, true, false), B::Tier_aus);
    // Flaeche weg + KEINE Legende   -> Alt-DLL-Fall, unveraendert Status 3
    EXPECT_EQ(pd::mess_gate_befund(false, false, false), B::Flaeche_fehlt_stumm);

    // T-7: REGISTRIERUNG IST TEIL DES TESTS. Ein Status, den dock_status_name nicht kennt, liest als
    // "unknown" und ist im Bericht stumm -- schlimmer als gar keiner, weil er wie ein bekannter
    // Fehler aussieht.
    EXPECT_EQ(pd::dock_status_name(pd::dock_status_mess_gate_mismatch), std::string_view{"mess_gate_mismatch"});
    EXPECT_EQ(pd::dock_status_name(pd::dock_status_mess_deaktiviert), std::string_view{"mess_deaktiviert"});
    EXPECT_EQ(pd::classify_dock_status(pd::dock_status_mess_gate_mismatch), cem::DockErrorClass::MessGateWiderspruch);
    EXPECT_EQ(pd::classify_dock_status(pd::dock_status_mess_deaktiviert), cem::DockErrorClass::MessDeaktiviert);
    // Und die Klassen duerfen NICHT als "n/a" lesen -- sonst taerne sich ein Defekt als Planmaessigkeit.
    EXPECT_EQ(cem::sample_status_token(cem::dock_error_sample_status(cem::DockErrorClass::MessGateWiderspruch)),
              std::string_view{"failed"});
    std::printf("[T-7] registriert: %s / %s (Klassen %u / %u von %zu)\n",
                std::string(pd::dock_status_name(pd::dock_status_mess_gate_mismatch)).c_str(),
                std::string(pd::dock_status_name(pd::dock_status_mess_deaktiviert)).c_str(),
                static_cast<unsigned>(cem::DockErrorClass::MessGateWiderspruch),
                static_cast<unsigned>(cem::DockErrorClass::MessDeaktiviert), cem::kDockErrorClassCount);
}

// ================================================================================================
// (d2) DAS DOCK LIEFERT DEN WIDERSPRUCH AUCH WIRKLICH -- gegen die AUS-Schwester mit GEFAELSCHTER
//      Legende. Das ist der Gegeneingang am echten Objekt, nicht an der Tabelle.
// ================================================================================================
TEST(Naht1MessVisitor, DockMeldetWiderspruchBeiGefaelschterLegende) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(g_aus_dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);

    pd::SearchAlgorithmDock     dock;
    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {8};
    opts.lookups_per_checkpoint = 4;
    opts.deletes_per_checkpoint = 0;
    std::string csv, json;

    // Die Legende BEHAUPTET Messung (1) -- die Flaeche fehlt. Das ist der Defekt.
    opts.mess_legende_erwartung = 1;
    int const rc_widerspruch    = dock.measure(handles[0], opts, csv, json);
    std::printf("[GEGENEINGANG] Legende=AN gegen MESS-AUS-Binary -> rc=%d (%s)\n", rc_widerspruch,
                std::string(pd::dock_status_name(rc_widerspruch)).c_str());
    EXPECT_EQ(rc_widerspruch, pd::dock_status_mess_gate_mismatch);
    EXPECT_EQ(pd::dock_status_name(rc_widerspruch), std::string_view{"mess_gate_mismatch"});

    // Dieselbe Binary, EHRLICHE Legende (0) -> kein Defekt, aber auch keine Messung.
    opts.mess_legende_erwartung = 0;
    int const rc_ehrlich        = dock.measure(handles[0], opts, csv, json);
    std::printf("[GEGENEINGANG] Legende=AUS gegen MESS-AUS-Binary -> rc=%d (%s)\n", rc_ehrlich,
                std::string(pd::dock_status_name(rc_ehrlich)).c_str());
    EXPECT_EQ(rc_ehrlich, pd::dock_status_mess_deaktiviert);

    // Und ohne Legende bleibt es beim Alt-Verhalten -- keine stille Verhaltensaenderung fuer Bestand.
    opts.mess_legende_erwartung = -1;
    EXPECT_EQ(dock.measure(handles[0], opts, csv, json), pd::dock_status_subinterface_missing);

    // Die AUS-Schwester hat NICHTS gemessen: kein Byte Ausgabe.
    EXPECT_TRUE(csv.empty()) << "Eine nicht gemessene Binary darf keine Mess-Ausgabe erzeugen.";
}

// ================================================================================================
// (e) MUTANTEN-PROBE an der Ordnung: der Vertrag ist die REIHENFOLGE, nicht die Anwesenheit
// ================================================================================================
TEST(Naht1MessVisitor, OrdnungsVerstoesseFallenAuf) {
    std::array<std::uint64_t, 8> f{};

    // Mutant 1: Zeile VOR dem begin.
    {
        ZaehlendeSenke s;
        s.e1_axis_row(0, f.data(), 1);
        EXPECT_TRUE(s.ordnung_gebrochen) << "Eine Zeile vor dem begin muss auffallen.";
    }
    // Mutant 2: Zeile uebersprungen (0, dann 2).
    {
        ZaehlendeSenke s;
        s.e2_begin(0, 0, 0, 3);
        s.e1_axis_row(0, f.data(), 1);
        s.e1_axis_row(2, f.data(), 1);
        EXPECT_TRUE(s.ordnung_gebrochen) << "Eine uebersprungene Achse muss auffallen.";
    }
    // Mutant 3: end ohne begin.
    {
        ZaehlendeSenke s;
        s.e2_end(0, 0, 0, 0, 0);
        EXPECT_TRUE(s.ordnung_gebrochen) << "Ein end ohne begin muss auffallen.";
    }
    // Mutant 4: eine Zeile zu wenig -> Rahmen unvollstaendig.
    {
        ZaehlendeSenke s;
        s.e2_begin(0, 0, 0, 2);
        s.e1_axis_row(0, f.data(), 1);
        s.e2_end(0, 0, 0, 0, 0);
        EXPECT_FALSE(s.rahmen_vollstaendig()) << "Ein Rahmen mit fehlender Zeile darf nicht als vollstaendig gelten.";
    }
    // GEGENPROBE: der ordnungsgemaesse Fall muss GRUEN sein -- sonst prueft die Wache nur sich selbst.
    {
        ZaehlendeSenke s;
        s.e2_begin(0, 0, 0, 2);
        s.e1_axis_row(0, f.data(), 1);
        s.e1_axis_row(1, f.data(), 1);
        s.e2_end(0, 0, 0, 0, 0);
        EXPECT_TRUE(s.rahmen_vollstaendig()) << "GEGENPROBE GESCHEITERT: die Ordnungs-Wache schlaegt auch bei "
                                                "korrekter Reihenfolge an -- dann sagt ihr Anschlagen nichts.";
    }
}

// ================================================================================================
// (f) DIE INVERSE: SnapshotSink und push_snapshot_to_visitor muessen sich exakt aufheben.
//     Das belegt, dass der Ereignis-Strom ALLES traegt, was der POD trug.
// ================================================================================================
TEST(Naht1MessVisitor, StromTraegtDenVollenPodVerlustfrei) {
    ana::ComdareTierObserverSnapshot original{};
    std::uint64_t                    z = 1;
    for (std::size_t t = 0; t < ana::kV3AxisCount; ++t) {
        for (std::size_t k = 0; k < ana::kV3FieldCount; ++k)
            original.axis_stats[t][k] = (z = z * 6364136223846793005ull + 1442695040888963407ull) >> 33;
        original.seg_ns[t] = static_cast<std::int64_t>(t) * 97 + 3;
    }
    original.observable_axis_count = 17;
    original.tier_fill_level       = 4242;
    original.filled_axis_count     = 18;
    original.batches_measured      = 8;
    original.seg_framework_ns      = 12345;
    original.seg_run_total_ns      = 98765;

    ana::ComdareTierObserverSnapshot rekonstruiert{};
    ana::SnapshotSink                sink{&rekonstruiert};
    ana::MessEdge<ana::SnapshotSink> kante{sink};
    ana::push_snapshot_to_visitor(original, kante, 0);

    EXPECT_TRUE(sink.order_ok());
    EXPECT_EQ(sink.rows_seen(), ana::kV3AxisCount);
    EXPECT_EQ(sink.reroutes_seen(), 0u);
    std::printf("[INVERSE] %zu Achsen x %zu Felder + 6 Meta durch den Strom und zurueck\n", ana::kV3AxisCount,
                ana::kV3FieldCount);
    // Bitweise Gleichheit. Faellt EIN Feld aus dem Vertrag, bricht genau hier.
    EXPECT_EQ(rekonstruiert, original) << "Der Ereignis-Strom verliert Felder -- die Naht traegt weniger als der POD.";
}
