// test_g3_batch_planner -- G3 / #46b Lagerhaltung, Scheibe B5.
//
// Deckt ab: Paritaets-Gleichverteilung, Einzeln-Miss-Erkennung, B13-Typ-Sequenz-Wache, die Slice-
// Queue (Producer-Consumer, Start-vor-Fertig NACHGEWIESEN) und den vollen async Planer-Lauf.
//
// T2-A/F4 (2026-08-06, Owner-KERN Zaehler-Resume) ADDITIV: die PLAN-ABLAGE. Bewiesen werden hier
//   (a) PLAN-VOR-STROM: plan_batch_slices liefert exakt das, was der async Planer streamt -- der Plan
//       existiert also als GANZES, bevor ein Fach die Queue erreicht (Codex-K1 "streamt sofort"),
//   (b) die GRAMMATIK in der Resume-Stempel-Ordnung (Kopf-Glieder, dann der hereingereichte
//       Schwanz-Schluessel mit der Zeilenzahl) -- Rundlauf ueber die Platte,
//   (c) FAIL-CLOSED an jeder Naht: fremder Stempel, abgeschnittenes/angehaengtes Dokument, unparsbare
//       Zahl, Zaehler gegen einen anders grossen Plan, gemessen > kompiliert,
//   (d) das PRAEFIX-Gesetz des Resume-Punkts: ein nur TEILWEISE gedecktes Fach wird NIE uebersprungen.

#include "bestandslog/batch_planner.hpp"
#include "bestandslog/slice_queue.hpp"

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <algorithm> // T2-A/F4-NB: std::find ueber die Schreiber-Inhalte
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator> // T2-A/F4-NB: istreambuf_iterator (den publizierten Inhalt am Stueck lesen)
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

// ---------------------------------------------------------------------------
// Paritaets-Gleichverteilung.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, WindowParity) {
    EXPECT_TRUE(bl::window_belongs_to(0, 0, 2));
    EXPECT_FALSE(bl::window_belongs_to(1, 0, 2));
    EXPECT_TRUE(bl::window_belongs_to(1, 1, 2));
    EXPECT_TRUE(bl::window_belongs_to(2, 0, 2));
    EXPECT_TRUE(bl::window_belongs_to(5, 0, 1)); // eine Maschine kriegt alles
    EXPECT_TRUE(bl::window_belongs_to(9, 3, 0)); // n_machines==0 -> alle
}

// ---------------------------------------------------------------------------
// Einzeln-Miss-Erkennung.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, DetectMissingIndividually) {
    // Gerade Indizes sind vorhanden -> fehlende = ungerade.
    bl::PresencePredicate even_present = [](std::uint64_t i) { return (i % 2) == 0; };
    auto                  missing      = bl::detect_missing_in_window(0, 10, even_present);
    EXPECT_EQ(missing, (std::vector<std::uint64_t>{1, 3, 5, 7, 9}));
}

TEST(G3BatchPlanner, PresencePredicateFromKeySet) {
    std::unordered_set<std::string> present{"key1", "key3"};
    auto is_present = bl::make_presence_predicate(present, [](std::uint64_t i) { return "key" + std::to_string(i); });
    EXPECT_TRUE(is_present(1));
    EXPECT_FALSE(is_present(2));
    EXPECT_TRUE(is_present(3));
}

// ---------------------------------------------------------------------------
// B13 Batch-Typ-Sequenz-Wache.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, TypeSequenceGuard) {
    using T = bl::BatchTyp;
    std::vector<T> good{T::planer_block, T::ceb, T::tier, T::tier};
    EXPECT_TRUE(bl::is_valid_type_sequence(good));
    std::vector<T> also_good{T::tier, T::tier}; // gleiche Phase wiederholt ist ok
    EXPECT_TRUE(bl::is_valid_type_sequence(also_good));
    std::vector<T> empty{};
    EXPECT_TRUE(bl::is_valid_type_sequence(empty));
    std::vector<T> regress{T::tier, T::ceb}; // Rueckfall tier->ceb -> Verletzung
    EXPECT_FALSE(bl::is_valid_type_sequence(regress));
    std::vector<T> regress2{T::ceb, T::planer_block};
    EXPECT_FALSE(bl::is_valid_type_sequence(regress2));
}

// ---------------------------------------------------------------------------
// Slice-Queue: Start-vor-Fertig NACHGEWIESEN -- der Consumer bekommt die erste Slice, BEVOR der
// Producer die zweite ueberhaupt eingereiht hat (Producer wartet auf das Consumer-Signal).
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, QueueConsumesBeforeProducerFinishes) {
    bl::SliceQueue    q;
    std::atomic<bool> consumed_first{false};

    std::thread producer([&] {
        q.push(bl::BatchSlice{0, 4, bl::BatchTyp::tier, {0, 1, 2, 3}});
        // Warten bis der Consumer die erste Slice hat -> beweist Start-vor-Fertig.
        while (!consumed_first.load()) std::this_thread::yield();
        q.push(bl::BatchSlice{4, 4, bl::BatchTyp::tier, {4, 5, 6, 7}});
        q.close();
    });

    auto s0 = q.pop(); // muss SOFORT kommen, bevor Slice 1 existiert
    ASSERT_TRUE(s0.has_value());
    EXPECT_EQ(s0->begin, 0u);
    consumed_first.store(true); // erst jetzt darf der Producer weitermachen

    auto s1 = q.pop();
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->begin, 4u);

    auto s2 = q.pop(); // geschlossen + gedraint
    EXPECT_FALSE(s2.has_value());

    producer.join();
}

TEST(G3BatchPlanner, QueueClosedEmptyReturnsNullopt) {
    bl::SliceQueue q;
    q.close();
    EXPECT_FALSE(q.pop().has_value());
    EXPECT_TRUE(q.closed());
}

// ---------------------------------------------------------------------------
// Voller async Planer-Lauf: rank 0 von 2, Korn 4, total 16 -> Fenster 0,2 gehoeren mir; alle fehlen
// -> zwei Slices (begin 0 und 8, je 4 fehlende). Consumer draint bis nullopt.
// ---------------------------------------------------------------------------
TEST(G3BatchPlanner, PlannerProducesMyParityWindowsWithMissing) {
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(
            q, /*total*/ 16, bl::BatchTyp::tier, /*rank*/ 0, /*n_machines*/ 2,
            /*is_present*/ [](std::uint64_t) { return false; }, /*grain*/ 4);
        while (auto s = q.pop()) collected.push_back(*s);
        // planner-dtor joined
    }
    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0].begin, 0u);
    EXPECT_EQ(collected[0].count, 4u);
    EXPECT_EQ(collected[0].missing, (std::vector<std::uint64_t>{0, 1, 2, 3}));
    EXPECT_EQ(collected[1].begin, 8u);
    EXPECT_EQ(collected[1].missing, (std::vector<std::uint64_t>{8, 9, 10, 11}));
}

TEST(G3BatchPlanner, PlannerSkipsFullyPresentWindows) {
    // Alle Binaries in Fenster 0 sind vorhanden, in Fenster 2 fehlen sie -> nur EINE Slice (begin 8).
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(q, 16, bl::BatchTyp::tier, 0, 2, [](std::uint64_t i) { return i < 4; }, 4);
        while (auto s = q.pop()) collected.push_back(*s);
    }
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected[0].begin, 8u);
}

TEST(G3BatchPlanner, PlannerRankOneGetsOtherWindows) {
    // rank 1 von 2 -> Fenster 1,3 (begin 4 und 12).
    std::vector<bl::BatchSlice> collected;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(q, 16, bl::BatchTyp::tier, 1, 2, [](std::uint64_t) { return false; }, 4);
        while (auto s = q.pop()) collected.push_back(*s);
    }
    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0].begin, 4u);
    EXPECT_EQ(collected[1].begin, 12u);
}

// ===========================================================================
// T2-A/F4 -- DER PLAN ALS DOKUMENT (Owner-KERN Zaehler-Resume).
// ===========================================================================

namespace {

// Der Schwanz-Schluessel wird im Betrieb HEREINGEREICHT (kLazyResumeRowsKey aus dem Iterator). Der
// Test darf ihn nicht aus dem Iterator ziehen -- diese TU ist bewusst leicht (nur Bestandslog-Header).
// Er nagelt ihn deshalb LITERAL auf denselben Wert; test_w5_status_reader haelt die Gegenprobe, dass
// die Iterator-Konstante genau dieses "|rows=" ist (dort ist der Iterator ohnehin inkludiert).
constexpr char const* kRowsKey = "|rows=";

[[nodiscard]] std::filesystem::path plan_test_dir() {
    std::filesystem::path const d = ::comdare::test::user_tmp_dir() / "comdare_g3_batchplan";
    std::error_code             ec;
    std::filesystem::remove_all(d, ec);
    std::filesystem::create_directories(d, ec);
    return d;
}

} // namespace

// ---------------------------------------------------------------------------
// (a) PLAN VOR STROM: die pure Konsolidierung == das, was der async Planer streamt.
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, PureKonsolidierungIstDerGestreamteePlan) {
    bl::PresencePredicate keiner_da = [](std::uint64_t) { return false; };
    auto const            plan      = bl::plan_batch_slices(16, bl::BatchTyp::tier, 0, 2, keiner_da, 4);

    std::vector<bl::BatchSlice> gestreamt;
    {
        bl::SliceQueue   q;
        bl::BatchPlanner planner(q, 16, bl::BatchTyp::tier, 0, 2, keiner_da, 4);
        while (auto s = q.pop()) gestreamt.push_back(*s);
    }
    EXPECT_EQ(plan, gestreamt) << "der Strom muss den Plan ABBILDEN, nicht ihn ersetzen";

    // Und die Dokument-Sicht traegt Fenster + Miss-Zahl, ohne die Miss-Liste.
    auto const faecher = bl::plan_faecher_sicht(plan);
    ASSERT_EQ(faecher.size(), 2u);
    EXPECT_EQ(faecher[0], (bl::PlanFach{0, 4, 4}));
    EXPECT_EQ(faecher[1], (bl::PlanFach{8, 4, 4}));
    EXPECT_EQ(bl::plan_atome(faecher), 8u);
}

// ---------------------------------------------------------------------------
// (b) GRAMMATIK + RUNDLAUF ueber die Platte (atomar geschrieben).
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, GrammatikUndDateiRundlauf) {
    std::string const               stamp = std::string{bl::kBatchPlanFormat} + "|art=test|indizes=9|korn=4";
    std::vector<bl::PlanFach> const faecher{{0, 4, 4}, {4, 4, 1}, {8, 1, 0}};

    std::string const text = bl::render_batch_plan(stamp, faecher, kRowsKey);
    EXPECT_EQ(text.rfind(stamp + "|rows=3\n", 0), 0u) << text; // Kopf-Glieder, dann DER Schwanz-Schluessel
    EXPECT_NE(text.find("\n4;4;1\n"), std::string::npos) << text;

    auto const zurueck = bl::parse_batch_plan(text, stamp, kRowsKey);
    ASSERT_TRUE(zurueck.has_value());
    EXPECT_EQ(*zurueck, faecher);

    auto const            dir = plan_test_dir();
    std::filesystem::path p   = dir / "plan.txt";
    ASSERT_TRUE(bl::write_batch_plan(p, stamp, faecher, kRowsKey));
    // T2-A/F4-NB: der tmp-Name ist prozess-eindeutig, ein fester Name waere hier nicht mehr
    // aussagekraeftig. Geprueft wird deshalb das ORDNER-IST: nach dem Schreiben liegt GENAU die
    // Zieldatei da und kein einziger .tmp-Rest -- egal wie er heisst.
    std::error_code ec;
    for (auto const& e : std::filesystem::directory_iterator{dir, ec})
        EXPECT_EQ(e.path().extension().string() == ".tmp", false)
            << "die atomare Ablage darf keine .tmp-Leiche hinterlassen: " << e.path().string();
    auto const gelesen = bl::read_batch_plan(p, stamp, kRowsKey);
    ASSERT_TRUE(gelesen.has_value());
    EXPECT_EQ(*gelesen, faecher);

    // Leerer Plan: gueltiges Dokument mit 0 Faechern (nicht "kaputt").
    ASSERT_TRUE(bl::write_batch_plan(dir / "leer.txt", stamp, {}, kRowsKey));
    auto const leer = bl::read_batch_plan(dir / "leer.txt", stamp, kRowsKey);
    ASSERT_TRUE(leer.has_value());
    EXPECT_TRUE(leer->empty());
}

// ---------------------------------------------------------------------------
// (c) FAIL-CLOSED: jede Abweichung ist "kein Anspruch", nie eine Teil-Uebernahme.
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, JedeAbweichungIstKeinAnspruch) {
    std::string const               stamp = std::string{bl::kBatchPlanFormat} + "|art=test|indizes=8|korn=4";
    std::vector<bl::PlanFach> const faecher{{0, 4, 4}, {4, 4, 4}};
    std::string const               text = bl::render_batch_plan(stamp, faecher, kRowsKey);

    EXPECT_FALSE(bl::parse_batch_plan(text, stamp + "x", kRowsKey).has_value()) << "fremder Stempel";
    EXPECT_FALSE(bl::parse_batch_plan(text, stamp, "|zeilen=").has_value()) << "fremder Schwanz-Schluessel";
    EXPECT_FALSE(bl::parse_batch_plan(text + "12;4;0\n", stamp, kRowsKey).has_value()) << "angehaengte Zeile";
    EXPECT_FALSE(bl::parse_batch_plan(text.substr(0, text.rfind("4;4;4")), stamp, kRowsKey).has_value())
        << "abgeschnittenes Dokument";
    EXPECT_FALSE(bl::parse_batch_plan(stamp + "|rows=1\n0;4\n", stamp, kRowsKey).has_value()) << "Feld fehlt";
    EXPECT_FALSE(bl::parse_batch_plan(stamp + "|rows=1\n0;4;xx\n", stamp, kRowsKey).has_value()) << "keine Zahl";
    EXPECT_FALSE(bl::parse_batch_plan(stamp + "|rows=1\n0;4;9\n", stamp, kRowsKey).has_value())
        << "mehr offen als gross -- das Dokument widerspricht sich";
    EXPECT_FALSE(bl::parse_batch_plan(stamp + "|rows=x\n", stamp, kRowsKey).has_value()) << "Zeilenzahl keine Zahl";

    // Eine fehlende Datei ist kein Anspruch (und kein Wurf).
    auto const dir = plan_test_dir();
    EXPECT_FALSE(bl::read_batch_plan(dir / "gibt_es_nicht.txt", stamp, kRowsKey).has_value());
    EXPECT_FALSE(bl::read_phasen_zaehler(dir / "gibt_es_nicht.zaehler", stamp, faecher, kRowsKey).has_value());
}

// ---------------------------------------------------------------------------
// (c2) DIE ZAEHLER: Ordnung, Bindung an den Plan, Invarianten.
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, ZaehlerStehenGegenGenauDiesenPlan) {
    std::string const               stamp = std::string{bl::kBatchPlanFormat} + "|art=test|indizes=8|korn=4";
    std::vector<bl::PlanFach> const faecher{{0, 4, 4}, {4, 4, 4}};

    std::string const zeile = bl::render_phasen_zaehler(stamp, bl::PhasenZaehler{4, 4}, faecher.size(), kRowsKey);
    EXPECT_EQ(zeile, stamp + "|kompiliert=4|gemessen=4|rows=2\n") << zeile;

    auto const gelesen = bl::parse_phasen_zaehler(zeile, stamp, faecher, kRowsKey);
    ASSERT_TRUE(gelesen.has_value());
    EXPECT_EQ(*gelesen, (bl::PhasenZaehler{4, 4}));

    // Gegen einen ANDERS GROSSEN Plan gilt derselbe Zaehler nicht mehr.
    std::vector<bl::PlanFach> const anderer{{0, 4, 4}};
    EXPECT_FALSE(bl::parse_phasen_zaehler(zeile, stamp, anderer, kRowsKey).has_value());

    // Invarianten: nicht mehr als geplant, und gemessen nie vor gebaut.
    EXPECT_FALSE(
        bl::parse_phasen_zaehler(stamp + "|kompiliert=99|gemessen=0|rows=2\n", stamp, faecher, kRowsKey).has_value());
    EXPECT_FALSE(
        bl::parse_phasen_zaehler(stamp + "|kompiliert=2|gemessen=4|rows=2\n", stamp, faecher, kRowsKey).has_value());
    // Feld-ORDNUNG ist Teil des Formats.
    EXPECT_FALSE(
        bl::parse_phasen_zaehler(stamp + "|gemessen=0|kompiliert=2|rows=2\n", stamp, faecher, kRowsKey).has_value());

    auto const dir = plan_test_dir();
    ASSERT_TRUE(bl::write_phasen_zaehler(dir / "p.zaehler", stamp, bl::PhasenZaehler{8, 3}, faecher.size(), kRowsKey));
    auto const von_platte = bl::read_phasen_zaehler(dir / "p.zaehler", stamp, faecher, kRowsKey);
    ASSERT_TRUE(von_platte.has_value());
    EXPECT_EQ(*von_platte, (bl::PhasenZaehler{8, 3}));
}

// ---------------------------------------------------------------------------
// (d) DAS PRAEFIX-GESETZ: ein halb gedecktes Fach wird NIE uebersprungen.
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, ResumePunktUeberspringtNurGanzGedeckteFaecher) {
    std::vector<bl::PlanFach> const faecher{{0, 4, 4}, {4, 4, 4}, {8, 2, 2}};
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 0), 0u);
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 3), 0u) << "3 von 4 Atomen: das Fach ist NICHT fertig";
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 4), 1u);
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 7), 1u);
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 8), 2u);
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 10), 3u);
    EXPECT_EQ(bl::plan_resume_faecher(faecher, 99), 3u) << "nie mehr als es Faecher gibt";
}

// ---------------------------------------------------------------------------
// (e) T2-A/F4-NB (Codex-Scope-F4, ECHT) -- DIE ABLAGE IST MULTI-WRITER-SICHER.
//
// Der Bau-Pool laeuft PARALLEL (die Ein-CEB-Exklusivitaet gilt nur fuers MESSEN). Mit einem FESTEN
// tmp-Namen oeffnen zwei gleichzeitige Schreiber DIESELBE Datei mit trunc, schreiben ineinander, und
// der erste rename publiziert den verwobenen Inhalt als "atomar geschriebenen" Plan.
//
// ROT VORGEFUEHRT (nicht committet, literal im Bericht): mit dem festen Namen "<ziel>.tmp" liefert
// genau dieser Test zerrissene Inhalte (Praefix des einen + Rest des anderen). Committet ist die
// GRUENE Form -- ein zerrissener Inhalt bricht sie sofort.
// ---------------------------------------------------------------------------
TEST(G3BatchPlan, AblageIstMultiWriterSicher) {
    auto const                  dir = plan_test_dir();
    std::filesystem::path const ziel = dir / "geteilt.txt";

    // Acht Schreiber, acht UNTERSCHEIDBARE Inhalte unterschiedlicher LAENGE (gleiche Laenge wuerde
    // ein Zerreissen verschleiern) -- gross genug, dass der Schreibvorgang nicht in einem Rutsch
    // durchlaeuft.
    constexpr int            kSchreiber = 8;
    std::vector<std::string> inhalte;
    for (int i = 0; i < kSchreiber; ++i)
        inhalte.push_back(std::string(static_cast<std::size_t>(200000 + i * 40000), static_cast<char>('a' + i)));

    std::atomic<int>         fehlschlaege{0};
    std::vector<std::thread> pool;
    for (int i = 0; i < kSchreiber; ++i)
        pool.emplace_back([&, i] {
            for (int runde = 0; runde < 8; ++runde)
                if (!bl::schreibe_atomar(ziel, inhalte[static_cast<std::size_t>(i)])) ++fehlschlaege;
        });
    for (auto& t : pool) t.join();
    EXPECT_EQ(fehlschlaege.load(), 0);

    // Der publizierte Inhalt ist IMMER genau einer der ganzen Inhalte -- nie eine Mischung.
    std::ifstream     is{ziel, std::ios::binary};
    std::string const gelesen{std::istreambuf_iterator<char>{is}, std::istreambuf_iterator<char>{}};
    EXPECT_NE(std::find(inhalte.begin(), inhalte.end(), gelesen), inhalte.end())
        << "publiziert wurde ein Inhalt der Laenge " << gelesen.size() << " -- kein ganzer Schreibvorgang";

    // Und kein einziger tmp-Rest bleibt liegen (Raeumung auf allen Wegen).
    std::error_code ec;
    for (auto const& e : std::filesystem::directory_iterator{dir, ec})
        EXPECT_NE(e.path().extension().string(), ".tmp") << e.path().string();
}

// (e2) Die tmp-Marke selbst: eindeutig ueber Aufrufe UND ueber Threads.
TEST(G3BatchPlan, TmpMarkeIstProzessEindeutig) {
    constexpr int                            kThreads = 8;
    constexpr int                            kProThread = 500;
    std::vector<std::vector<std::string>>    je_thread(kThreads);
    std::vector<std::thread>                 pool;
    for (int t = 0; t < kThreads; ++t)
        pool.emplace_back([&, t] {
            auto& meine = je_thread[static_cast<std::size_t>(t)];
            for (int i = 0; i < kProThread; ++i) meine.push_back(bl::detail::tmp_marke());
        });
    for (auto& th : pool) th.join();

    std::unordered_set<std::string> alle;
    for (auto const& v : je_thread)
        for (auto const& m : v) alle.insert(m);
    EXPECT_EQ(alle.size(), static_cast<std::size_t>(kThreads * kProThread))
        << "zwei gleiche Marken heissen zwei Schreiber auf derselben tmp-Datei";
}

// (e3) FEHLERPFAD: laesst sich der Ordner nicht anlegen (eine DATEI steht an seiner Stelle), ist das
// ein ehrliches false -- und es bleibt kein eigener Rest liegen.
TEST(G3BatchPlan, FehlerpfadHinterlaesstKeinenRest) {
    auto const                  dir = plan_test_dir();
    std::filesystem::path const sperre = dir / "keinordner";
    { std::ofstream{sperre, std::ios::trunc} << "ich bin eine Datei\n"; }
    EXPECT_FALSE(bl::schreibe_atomar(sperre / "plan.txt", "egal"));
    std::error_code ec;
    for (auto const& e : std::filesystem::directory_iterator{dir, ec})
        EXPECT_NE(e.path().extension().string(), ".tmp") << e.path().string();
}
