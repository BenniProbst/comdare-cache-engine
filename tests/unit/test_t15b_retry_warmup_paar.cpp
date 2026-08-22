// test_t15b_retry_warmup_paar.cpp -- T-15b + C-05 (20.08.2026): BINARY-RETRY-KLAMMER und
// WIEDERHOLUNGS-PAAR in der Mess-Schleife.
//
// SELBSTCHECK: dieser Test behauptet NICHTS ueber die Drift-Mechanik (test_chaos_drift_gate) und
// nichts ueber die T-15a-Verdrahtung (test_t15_drift_gate_messschleife). Er behauptet VIER Dinge:
//   (a) KON37-06: Build UND Messung duerfen JE bis zu 5 Mal scheitern -- die Klammer wiederholt,
//       stoppt beim Erfolg, gibt nach Erschoepfung auf (der letzte Ausgang steht).
//   (b) KON28-02: HART triggert (load_failed / SampleStatus::Failed einer EINZELNEN Einstellung),
//       SOFT (fehlende PMC-Einrichtung) triggert NIE -- der Soft-Koeder haelt das fest.
//   (c) KON47-04: je Wiederholung ein PAAR (Lauf 1 messen+VERWERFEN, Lauf 2 messen+SPEICHERN);
//       --debug misst GENAU EINMAL, kalt. Mit drift_reps=3: 6 Laeufe fuer 3 Drift-Proben.
//   (d) die KETTE steht: XML <binary_retry> -> ThesisProfile -> LazyRunConfig -> Iterator-Dispatch
//       ([T-15B-RETRY]) + BuildConfig ([T-15B-BAU-KLAMMER]); das Paar sitzt IN der [T-15-KLAMMER].
//
// KEIN ZUFALL (Owner-KERN): alle Werte sind feste Skripte; die Koeder sind VORHER benannte
// Verwechslungs-Kandidaten (Paar-Zaehlung, --debug-2-Laeufe, Lauf-1-gespeichert, Soft-triggert).
//
// T-11c-PROTOKOLL (Wegwerf-Mutationen, je ROT verlangt -- Literale in der Strang-Ergebnisdatei):
//   M1 mess_warmup_paar: Lauf 1 wird GESPEICHERT statt Lauf 2  -> Abschnitt (3) ROT
//   M2 mess_warmup_paar: --debug faehrt ZWEI Laeufe            -> Abschnitt (3) ROT
//   M3 mit_mess_retry:   stoppt nicht beim Erfolg              -> Abschnitt (1) ROT
//   M4 Praedikat:        Soft (pmc fehlt) triggert             -> Abschnitt (2) ROT
//
// Build: Standalone int main() (kein gtest), reiner Host-Pfad -- Rezept von
// test_t15_drift_gate_messschleife (derselbe Include-Satz; die Quellen-Wachen erhalten die
// Quell-Pfade per compile definition).

#include <harness/mess_retry_klammer.hpp>
#include <harness/mess_warmup_paar.hpp>
#include <harness/drift_gated_cell.hpp>
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // LazyRunConfig + Dispatch-Quelle
#include <builder/experiment_tree/measure_parallelism.hpp>           // resolve_mess_kaltlauf_of_mode
#include <serialization/xml_config_parser/xml_config_parser.hpp>     // <binary_retry .../>

#include "comdare_test_tmp.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cm = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void eq(std::string const& was, A const& ist, B const& soll) {
    bool const ok = (ist == static_cast<A>(soll));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << " = " << ist;
    if (!ok) std::cout << "  (erwartet " << soll << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// == Der Zwilling des Iterator-Outcomes (CellOutcome ist funktions-lokal; das Praedikat ist ==========
// == deshalb ein Template und wird hier gegen denselben FELD-Vertrag gefahren) ======================
enum class ZwillingStatus : std::uint8_t { Ok, Failed, SourceUnavailable };

struct ZwillingZeile {
    ZwillingStatus sample_status = ZwillingStatus::Ok;
    bool           pmc_available = true; // Deko-Feld: das Praedikat DARF es nie lesen (Soft-Koeder)
};

struct ZwillingOutcome {
    std::size_t                load_failed = 0;
    std::vector<ZwillingZeile> rows;
};

[[nodiscard]] std::string trimme(std::string const& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

} // namespace

int main() {
    std::cout << "== T-15b/C-05: Binary-Retry-Klammer + Wiederholungs-Paar ==\n";

    // == (1) DIE RETRY-KLAMMER: je 5, Stop beim Erfolg, aufgeben nach Erschoepfung ==================
    {
        std::cout << "-- (1) mit_mess_retry: Stop beim Erfolg / Erschoepfung --\n";
        ex::MessRetryKonfig k5; // Owner-Default 5

        // Erfolg beim ERSTEN Versuch: genau ein Aufruf, kein Log.
        std::size_t        laeufe = 0;
        std::ostringstream warn;
        auto const         sofort = ex::mit_mess_retry(
            k5,
            [&] {
                ++laeufe;
                return int{7};
            },
            [](int) { return false; }, &warn, "sofort");
        eq("Erfolg sofort: Versuche", sofort.versuche, std::size_t{1});
        eq("Erfolg sofort: Aufrufe", laeufe, std::size_t{1});
        tr("Erfolg sofort: nicht erschoepft", !sofort.erschoepft);
        tr("Erfolg sofort: kein Warn-Log", warn.str().empty());

        // Hart bis Versuch 2, Erfolg beim 3.: der Ausgang traegt den WERT DES LETZTEN Versuchs
        // (jeder Wiederholungs-Versuch verwirft den vorigen Ausgang vollstaendig).
        laeufe = 0;
        std::ostringstream warn3;
        auto const         dritter = ex::mit_mess_retry(
            k5,
            [&] {
                ++laeufe;
                return static_cast<int>(laeufe);
            },
            [](int v) { return v < 3; }, &warn3, "dritter");
        eq("Erfolg beim 3.: Versuche", dritter.versuche, std::size_t{3});
        eq("Erfolg beim 3.: der LETZTE Ausgang steht", dritter.outcome, int{3});
        tr("Erfolg beim 3.: nicht erschoepft", !dritter.erschoepft);
        tr("Erfolg beim 3.: jeder Wiederholungs-Anlauf steht im Log ([t15b-retry] x2)",
           warn3.str().find("[t15b-retry]") != std::string::npos &&
               warn3.str().find("Versuch 1/5") != std::string::npos &&
               warn3.str().find("Versuch 2/5") != std::string::npos &&
               warn3.str().find("Versuch 3/5") == std::string::npos);

        // Dauerhaft hart: 5 Versuche, dann AUFGEBEN -- der letzte Ausgang steht, laut.
        laeufe = 0;
        std::ostringstream warn5;
        auto const         zaeh = ex::mit_mess_retry(
            k5,
            [&] {
                ++laeufe;
                return static_cast<int>(laeufe);
            },
            [](int) { return true; }, &warn5, "zaeh");
        eq("Erschoepfung: Versuche == Owner-5", zaeh.versuche, std::size_t{5});
        eq("Erschoepfung: Aufrufe", laeufe, std::size_t{5});
        tr("Erschoepfung: erschoepft == true", zaeh.erschoepft);
        eq("Erschoepfung: der LETZTE Ausgang steht ('failed', nie null)", zaeh.outcome, int{5});
        tr("Erschoepfung: die Aufgabe steht im Log", warn5.str().find("Budget erschoepft") != std::string::npos);

        // max_versuche == 0 ist eine Fehlkonfiguration -> GENAU EIN Versuch, nicht null.
        laeufe = 0;
        ex::MessRetryKonfig k0;
        k0.max_versuche = 0;
        auto const kein = ex::mit_mess_retry(
            k0,
            [&] {
                ++laeufe;
                return int{1};
            },
            [](int) { return true; }, nullptr, "null");
        eq("max_versuche=0: genau EIN Versuch", laeufe, std::size_t{1});
        tr("max_versuche=0: erschoepft (Budget 1 verbraucht)", kein.erschoepft);
    }

    // == (2) DER TRIGGER (KON28-02): hart JA, soft NIE ==============================================
    {
        std::cout << "-- (2) mess_durchlauf_hart_gescheitert: hart/soft --\n";
        // load_failed (die drei SourceUnavailable-Pfade) -> HART.
        ZwillingOutcome lf;
        lf.load_failed = 1;
        tr("load_failed=1 -> hart", ex::mess_durchlauf_hart_gescheitert(lf, ZwillingStatus::Failed));

        // SampleStatus::Failed EINER einzelnen Einstellung -> HART (die KON26-04-OFFEN-Frage ist
        // durch KON28-02 mit JA geschlossen).
        ZwillingOutcome eine;
        eine.rows = {{ZwillingStatus::Ok, true}, {ZwillingStatus::Failed, true}, {ZwillingStatus::Ok, true}};
        tr("EINE Failed-Einstellung -> hart", ex::mess_durchlauf_hart_gescheitert(eine, ZwillingStatus::Failed));

        // SOFT-KOEDER: fehlende PMC-Einrichtung (pmc_available=false) bei Ok-Zeilen -> NIE hart.
        // Wuerde das Praedikat ein Verfuegbarkeits-Flag lesen, wuerde dieser Koeder beissen.
        ZwillingOutcome soft;
        soft.rows = {{ZwillingStatus::Ok, false}, {ZwillingStatus::Ok, false}};
        tr("SOFT-Koeder: PMC fehlt -> KEIN Trigger (kein Retry)",
           !ex::mess_durchlauf_hart_gescheitert(soft, ZwillingStatus::Failed));

        // Alles ok -> kein Trigger. Und SourceUnavailable-ZEILEN allein (ohne load_failed-Zaehler)
        // sind nicht das Hart-Kriterium der Zeilen-Seite -- die Quelle-fehlt-Lage meldet der
        // Zaehler; die Zeilen-Seite urteilt ueber Failed.
        ZwillingOutcome gut;
        gut.rows = {{ZwillingStatus::Ok, true}};
        tr("alles ok -> kein Trigger", !ex::mess_durchlauf_hart_gescheitert(gut, ZwillingStatus::Failed));
        ZwillingOutcome nur_zeile;
        nur_zeile.rows = {{ZwillingStatus::SourceUnavailable, true}};
        tr("SourceUnavailable-Zeile ohne load_failed -> kein Zeilen-Trigger",
           !ex::mess_durchlauf_hart_gescheitert(nur_zeile, ZwillingStatus::Failed));
    }

    // == (3) DAS PAAR (KON47-04): 2 Laeufe/1 Wert -- --debug 1 Lauf -- Verwerf-Beweis ===============
    {
        std::cout << "-- (3) mess_warmup_paar: Zaehlung + Verwerf-Beweis + --debug --\n";
        // Das Skript macht Lauf 1 und Lauf 2 UNTERSCHEIDBAR: wer Lauf 1 speichert, faellt hier um.
        std::size_t laeufe = 0;
        auto        skript = [&]() -> std::int64_t {
            ++laeufe;
            return laeufe == 1 ? std::int64_t{111} : std::int64_t{222};
        };

        auto const paar = ex::mess_warmup_paar(false, skript);
        eq("PAAR: genau ZWEI Laeufe", paar.laeufe, std::size_t{2});
        eq("PAAR: davon EINER verworfen", paar.verworfen, std::size_t{1});
        eq("PAAR: gespeichert ist LAUF 2 (Verwerf-Beweis)", paar.payload, std::int64_t{222});
        eq("PAAR: das Skript lief wirklich zweimal", laeufe, std::size_t{2});

        // --debug: GENAU EIN Lauf, kalt -- und der EINE Wert ist der Kaltwert.
        laeufe           = 0;
        auto const debug = ex::mess_warmup_paar(true, skript);
        eq("--debug: genau EIN Lauf", debug.laeufe, std::size_t{1});
        eq("--debug: nichts verworfen", debug.verworfen, std::size_t{0});
        eq("--debug: der Kaltwert steht", debug.payload, std::int64_t{111});
        eq("--debug: das Skript lief wirklich nur einmal", laeufe, std::size_t{1});
    }

    // == (3b) PAAR x DRIFT: die KON47-Zahl "6 Laeufe / 3 Werte" ====================================
    // Das Paar sitzt IN der [T-15-KLAMMER], also unter dem Drift-Gate: je Drift-Probe ein Paar.
    // reps=3, stabil => 1 Gruppe => 3 Proben => 6 rohe Laeufe. Gate aus => 1 Probe => 2 Laeufe.
    {
        std::cout << "-- (3b) Paar unter dem Drift-Gate: 6 Laeufe / 3 Werte --\n";
        std::size_t rohe = 0;
        auto        mess = [&]() -> std::int64_t {
            ++rohe;
            return 100; // stabil: Drift 0 %
        };
        ex::DriftGateConfig g;
        g.reps       = 3;
        g.max_reruns = 2;

        auto const zelle = ex::run_cell_with_drift_gate(
            g, [&]() -> std::int64_t { return ex::mess_warmup_paar(false, mess).payload; },
            [](std::int64_t v) { return v; }, nullptr, "paar_x_drift");
        eq("3 Drift-Proben (Werte) je Gruppe", zelle.messungen, std::size_t{3});
        eq("6 rohe Laeufe fuer 3 Werte (KON47-04)", rohe, std::size_t{6});
        tr("stabil (das Paar aendert das Urteil nicht)", zelle.stable);

        // Gate aus: 1 Wert, 2 rohe Laeufe. --debug + Gate aus: 1 Wert, 1 roher Lauf.
        rohe = 0;
        ex::DriftGateConfig aus;
        aus.reps        = 1;
        auto const solo = ex::run_cell_with_drift_gate(
            aus, [&]() -> std::int64_t { return ex::mess_warmup_paar(false, mess).payload; },
            [](std::int64_t v) { return v; }, nullptr, "paar_solo");
        eq("Gate aus: 1 Wert", solo.messungen, std::size_t{1});
        eq("Gate aus: 2 rohe Laeufe (das Paar bleibt Pflicht)", rohe, std::size_t{2});
        rohe             = 0;
        auto const debug = ex::run_cell_with_drift_gate(
            aus, [&]() -> std::int64_t { return ex::mess_warmup_paar(true, mess).payload; },
            [](std::int64_t v) { return v; }, nullptr, "paar_debug");
        eq("--debug + Gate aus: genau 1 roher Lauf (kalt)", rohe, std::size_t{1});
        eq("--debug: der Wert steht trotzdem", debug.payload, std::int64_t{100});
    }

    // == (4) RETRY x PAAR: die Klammer wiederholt den GESAMTEN Durchlauf ============================
    // 2 Versuche x (3 Proben x 2 Laeufe) = 12 rohe Laeufe -- die Multiplikation, die die Arena-
    // Planer-Zeile arena_gesamt_faktor deckt (drift * paar * retry; planner_mengen_types.hpp).
    {
        std::cout << "-- (4) Retry x Paar x Drift: die Faktoren multiplizieren --\n";
        std::size_t         rohe     = 0;
        std::size_t         versuche = 0;
        ex::DriftGateConfig g;
        g.reps       = 3;
        g.max_reruns = 2;
        ex::MessRetryKonfig k;
        k.max_versuche = 5;

        auto const geklammert = ex::mit_mess_retry(
            k,
            [&] {
                ++versuche;
                auto const zelle = ex::run_cell_with_drift_gate(
                    g,
                    [&]() -> std::int64_t {
                        ++rohe;
                        (void)rohe; // Lauf 1 (Warmup) -- verworfen
                        ++rohe;
                        return 100; // Lauf 2 -- gespeichert (inline statt mess_warmup_paar: die
                                    // Zaehlung bleibt dieselbe, s. Abschnitt 3b fuer die Paar-Form)
                    },
                    [](std::int64_t v) { return v; }, nullptr, "retry_x_paar");
                return zelle.payload;
            },
            [&](std::int64_t) { return versuche < 2; }, nullptr, "retry_x_paar");
        eq("2 Versuche gefahren (1 hart + 1 Erfolg)", geklammert.versuche, std::size_t{2});
        eq("12 rohe Laeufe == 2 Versuche x 3 Proben x 2 Paar-Laeufe", rohe, std::size_t{12});
    }

    // == (5) DIE KETTE: XML <binary_retry> -> ThesisProfile (+ Umzugs-Beweis am drift_gate) =========
    {
        std::cout << "-- (5) <binary_retry/> aus dem Profil-XML + Attr-Default-Umzug --\n";
        namespace fs = std::filesystem;
        namespace cx = comdare::builder::xml;

        auto const      dir = comdare::test::user_tmp_dir() / "t15b_retry";
        std::error_code ec;
        fs::create_directories(dir, ec);
        auto schreibe = [&](std::string const& name, std::string const& inhalt) {
            std::ofstream f{dir / name, std::ios::trunc};
            f << inhalt;
            f.close();
            return dir / name;
        };

        // max_versuche=4 ist bewusst NICHT der Default: nur so ist belegt, dass das XML wirkt.
        auto const mit =
            schreibe("mit_binary_retry.profile.xml", "<comdare_thesis_profile id=\"t15b\" schema_version=\"1\">\n"
                                                     "  <repetitions count=\"3\"/>\n"
                                                     "  <binary_retry max_versuche=\"4\"/>\n"
                                                     "</comdare_thesis_profile>\n");
        auto const ohne =
            schreibe("ohne_binary_retry.profile.xml", "<comdare_thesis_profile id=\"t15b\" schema_version=\"1\">\n"
                                                      "  <repetitions count=\"3\"/>\n"
                                                      "</comdare_thesis_profile>\n");
        auto const kaputt =
            schreibe("kaputtes_binary_retry.profile.xml", "<comdare_thesis_profile id=\"t15b\" schema_version=\"1\">\n"
                                                          "  <binary_retry max_versuche=\"0\"/>\n"
                                                          "</comdare_thesis_profile>\n");
        // Umzugs-Beweis: <drift_gate> OHNE max_reruns-Attribut traegt den NEUEN Attr-Default 3.
        auto const umzug =
            schreibe("drift_ohne_max_reruns.profile.xml", "<comdare_thesis_profile id=\"t15b\" schema_version=\"1\">\n"
                                                          "  <drift_gate reps=\"4\" threshold_permille=\"37\"/>\n"
                                                          "</comdare_thesis_profile>\n");

        cx::XmlConfigParser const p;
        auto const                tp_mit = p.parse_thesis_profile(mit);
        tr("Profil MIT <binary_retry> ist lesbar", tp_mit.has_value());
        if (tp_mit.has_value()) {
            tr("declared", tp_mit->binary_retry_declared);
            eq("max_versuche aus dem XML", tp_mit->binary_retry_max_versuche, 4);
        }
        auto const tp_ohne = p.parse_thesis_profile(ohne);
        tr("Profil OHNE <binary_retry> ist lesbar", tp_ohne.has_value());
        if (tp_ohne.has_value())
            tr("declared == false -> Owner-Default 5 der cache_engine-Schicht bleibt", !tp_ohne->binary_retry_declared);
        tr("max_versuche=0 macht das Profil UNLESBAR (kein einziger Versuch ist keine Messlage)",
           !p.parse_thesis_profile(kaputt).has_value());
        auto const tp_umzug = p.parse_thesis_profile(umzug);
        tr("Umzugs-Profil lesbar", tp_umzug.has_value());
        if (tp_umzug.has_value())
            eq("drift_gate OHNE max_reruns-Attr -> NEUER Default 3 (Umzug, KON26-04)", tp_umzug->drift_gate_max_reruns,
               3);
    }

    // == (6) DEFAULTS + --debug-RESOLVER ===========================================================
    {
        std::cout << "-- (6) Owner-Defaults + Kaltlauf-Resolver --\n";
        eq("MessRetryKonfig::max_versuche (Owner-5)", ex::MessRetryKonfig{}.max_versuche, std::uint32_t{5});
        eq("BuildConfig::bau_max_versuche (dieselbe Groesse, 'je 5')", ex::BuildConfig{}.bau_max_versuche,
           std::uint32_t{5});
        ex::LazyRunConfig const lauf;
        eq("LazyRunConfig::mess_retry.max_versuche", lauf.mess_retry.max_versuche, std::uint32_t{5});
        tr("LazyRunConfig::mess_kaltlauf_debug default false (PAAR-Pflicht)", !lauf.mess_kaltlauf_debug);

        // Der Resolver am WorkModeInfo-Zustand: KEIN Registry-Modus ist heute "misst UND parallel".
        for (auto const& m : cm::kWorkModeRegistry)
            tr(std::string{"Registry '"} + std::string{m.id} + "' -> Kaltlauf false (Paar-Pflicht)",
               !ex::resolve_mess_kaltlauf_of_mode(m));
        // Der kuenftige --debug-Zustand (S-8/W2) schaltet den Kaltlauf -- ueber die Injektion, ohne
        // Registry-Zeile: measurement_on && !single_thread.
        cm::WorkModeInfo debug{cm::WorkMode::Measure, "debug_injektion", "DebugInjektion", "Debug", true, false};
        tr("Zustands-Injektion 'misst UND parallel' -> Kaltlauf true", ex::resolve_mess_kaltlauf_of_mode(debug));
        cm::WorkModeInfo messen{cm::WorkMode::Measure, "m", "M", "Release", true, true};
        tr("misst, aber 1-Thread -> Kaltlauf false", !ex::resolve_mess_kaltlauf_of_mode(messen));
    }

    // == (7) QUELLEN-WACHEN (dieselbe ehrliche Einordnung wie T-15 Abschnitt 8: Struktur-, keine =====
    // == Verhaltens-Wache -- aber sie faengt exakt den Defekt 'Mechanismus ohne Aufrufer') ==========
    {
        std::cout << "-- (7) Quellen-Wachen: Dispatch-Klammer, Paar in der T-15-Klammer, Bau-Klammer --\n";

        // (7a) Iterator: [T-15B-RETRY]-Marker existieren genau einmal; measure_one_binary wird im
        // Dispatch NUR innerhalb der Marker gerufen; mess_warmup_paar( wird genau einmal gerufen.
        std::ifstream it{COMDARE_T15B_ITERATOR_QUELLE};
        tr("Iterator-Quelle lesbar", it.good());
        std::string zeile;
        std::size_t marker_auf = 0, marker_zu = 0, dispatch_in = 0, dispatch_aussen = 0;
        std::size_t paar_aufrufe = 0, retry_aufrufe = 0;
        bool        in_marker = false;
        while (std::getline(it, zeile)) {
            std::string const t         = trimme(zeile);
            bool const        kommentar = (t.rfind("//", 0) == 0) || (t.rfind("*", 0) == 0);
            if (t.find("// [T-15B-RETRY]") != std::string::npos) {
                in_marker = true;
                ++marker_auf;
                continue;
            }
            if (t.find("// [T-15B-RETRY-ENDE]") != std::string::npos) {
                in_marker = false;
                ++marker_zu;
                continue;
            }
            if (kommentar) continue;
            if (t.find("mess_warmup_paar(") != std::string::npos) ++paar_aufrufe;
            if (t.find("mit_mess_retry(") != std::string::npos) ++retry_aufrufe;
            // Das AUFRUF-Muster "measure_one_binary(" trifft die Lambda-DEFINITION nicht (dort
            // steht "= [&](" dazwischen) -- gezaehlt werden ausschliesslich echte Aufrufe.
            if (t.find("measure_one_binary(") != std::string::npos) {
                if (in_marker)
                    ++dispatch_in;
                else
                    ++dispatch_aussen;
            }
        }
        eq("genau eine oeffnende [T-15B-RETRY]-Marke", marker_auf, std::size_t{1});
        eq("genau eine schliessende Marke", marker_zu, std::size_t{1});
        eq("measure_one_binary-Dispatch IN der Retry-Klammer", dispatch_in, std::size_t{1});
        eq("KEIN measure_one_binary-Aufruf ausserhalb der Klammer", dispatch_aussen, std::size_t{0});
        eq("mit_mess_retry wird genau einmal gerufen", retry_aufrufe, std::size_t{1});
        eq("mess_warmup_paar wird genau einmal gerufen (in der T-15-KLAMMER)", paar_aufrufe, std::size_t{1});

        // (7b) Entry-Kette: beide Zuweisungen existieren (Thesis- UND Experiment-Kanal).
        auto zaehle = [&](char const* pfad, std::string_view nadel) {
            std::ifstream f{pfad};
            std::size_t   n = 0;
            std::string   z;
            while (std::getline(f, z)) {
                std::string const t = trimme(z);
                if (t.rfind("//", 0) == 0) continue;
                if (t.find(nadel) != std::string::npos) ++n;
            }
            return n;
        };
        tr("profile_run_entry belegt cfg.mess_retry.max_versuche",
           zaehle(COMDARE_T15B_ENTRY_QUELLE, "cfg.mess_retry.max_versuche") >= 1);
        tr("profile_run_entry belegt cfg.mess_kaltlauf_debug",
           zaehle(COMDARE_T15B_ENTRY_QUELLE, "cfg.mess_kaltlauf_debug") >= 1);
        tr("experiment_run_entry belegt cfg.mess_kaltlauf_debug",
           zaehle(COMDARE_T15B_EXPENTRY_QUELLE, "cfg.mess_kaltlauf_debug") >= 1);
        tr("der Iterator speist bcfg.bau_max_versuche aus DERSELBEN Quelle",
           zaehle(COMDARE_T15B_ITERATOR_QUELLE, "bcfg.bau_max_versuche = cfg.mess_retry.max_versuche") >= 1);
        // C-11-Kette (KON28-02 SOFT): der Iterator ERHEBT die PMC-Warnung in den Traeger, der Entry
        // REICHT sie in die Mappe (INFO-Blatt der xlsx) -- beide Glieder muessen existieren.
        tr("der Iterator erhebt die C-11-Warnung in LazyRunResult::mess_warnungen",
           zaehle(COMDARE_T15B_ITERATOR_QUELLE, "result.mess_warnungen.push_back") >= 1);
        tr("profile_run_entry reicht die Warnungen als KonstantenMeta in die Mappe",
           zaehle(COMDARE_T15B_ENTRY_QUELLE, "mappe.schliessen(sysinfo, {}, warn_meta)") >= 1);

        // (7c) Bau-Klammer: Marker-Paar im Orchestrator + Budget-Nutzung. Die Marker LEBEN in
        // Kommentaren -- fuer sie zaehlt eine Roh-Zaehlung OHNE Kommentar-Filter (zaehle() wuerde
        // sie als Kommentarzeilen verwerfen -- der erste Lauf dieses Tests hat genau das gezeigt).
        auto zaehle_roh = [&](char const* pfad, std::string_view nadel) {
            std::ifstream f{pfad};
            std::size_t   n = 0;
            std::string   z;
            while (std::getline(f, z))
                if (z.find(nadel) != std::string::npos) ++n;
            return n;
        };
        tr("[T-15B-BAU-KLAMMER] existiert", zaehle_roh(COMDARE_T15B_ORCH_QUELLE, "[T-15B-BAU-KLAMMER]") >= 1);
        tr("[T-15B-BAU-KLAMMER-ENDE] existiert", zaehle_roh(COMDARE_T15B_ORCH_QUELLE, "[T-15B-BAU-KLAMMER-ENDE]") >= 1);
        tr("provision_core liest cfg_.bau_max_versuche",
           zaehle(COMDARE_T15B_ORCH_QUELLE, "cfg_.bau_max_versuche") >= 1);

        // (7d) Kaltpfad-Zensus (KON47-04 'anpassen ODER ausbuchen' -> ANPASSUNG): perm_runner.hpp
        // traegt run_observable_perm( genau ZWEIMAL ausserhalb von Kommentaren -- die Definition und
        // der Unbekannt-Profil-Fallback in run_workload_perm (der selbst nur durch die T-15-Klammer
        // erreicht wird). Ein DRITTES Vorkommen waere ein neuer Kaltpfad und macht diese Zeile rot.
        eq("perm_runner.hpp: run_observable_perm( genau 2x (Definition + Fallback)",
           zaehle(COMDARE_T15B_PERMRUNNER_QUELLE, "run_observable_perm("), std::size_t{2});
    }

    std::cout << (g_fail == 0 ? "== T-15b GRUEN ==\n" : "== T-15b ROT ==\n");
    return g_fail == 0 ? 0 : 1;
}
