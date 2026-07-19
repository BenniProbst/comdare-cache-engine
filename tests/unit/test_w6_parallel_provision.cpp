// test_w6_parallel_provision -- W6 (2026-07-19, Ledger §32-F7 "KOMPILATION darf parallel, nur MESSEN ist 1-Thread").
// Belegt den parallelen provision-/Bau-Pool (BuildConfig::build_parallelism / effective_build_workers) UND das
// KERN-GATE: der Bau parallelisiert die Kompilation, sammelt die Ergebnisse aber POSITIONS-TREU (results[j]) ein,
// sodass die Sink-/Fortschritts-Feuerung des Iterators STRENG SEQUENZIELL in Fenster-/Index-Reihenfolge bleibt --
// DETERMINISTISCH, unabhaengig von der (bewusst erzwungenen) Out-of-Order-Completion der Compile-Worker.
//
// Warum kein echter Compile: der Ordnungs-/Einsammel-Kern ist von der realen Kompilation TRENNBAR (Muster wie
// progress_delta.hpp: die Kombinatorik ist freistehend testbar). Eine Stub-CompileFn mit invertiertem Jitter
// (hoehere Index-Nummer -> kuerzerer Sleep -> frueherer Abschluss) erzwingt Out-of-Order-Completion; die
// Positions-Treue der results[] und die daraus abgeleitete Progress-Delta-Folge MUESSEN dennoch worker-zahl-
// invariant sein (= die §38-ProgressDelta-Cursor-Semantik als deterministische Mixed-Radix-Folge).
//
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_kf16).

#include "builder/build_orchestrator/build_orchestrator.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"
#include "builder/experiment_tree/progress_delta.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}
template <typename A, typename B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

// 8 statische Blaetter (1x4x2), identisch zum KF-16-Muster -> die Mixed-Radix-Ziffern variieren real ueber die Ebenen.
static ex::ExperimentTree make_tree(std::shared_ptr<ex::ExperimentNodeFactory> const& f) {
    ex::ExperimentTree t{f};
    t.build({
        ex::AxisLevel{"traversal", {"ART"}, true, ""},                        // gepinnt (Ziffer immer 0)
        ex::AxisLevel{"node", {"N4", "N16", "N48", "N256"}, true, ""},        // freigegeben (Ziffer 0..3)
        ex::AxisLevel{"node.cl_line", {"64", "128"}, true, ""},               // statische cacheline-Sub-Ebene (0..1)
        ex::AxisLevel{"concurrency", {"1", "2", "4"}, false, "thread_count"}, // dynamisch (keine Binary)
    });
    return t;
}

// Die Progress-Delta-Folge, EXAKT wie der Iterator sie im provision_only-Zweig feuert: je Binary in builds-Reihenfolge
// ein Delta (view.variant_tuple(index) mixed-radix), danach genau EIN done-Delta am Fensterende. Reine Wiederverwendung
// der getesteten Primitive compute_progress_deltas -> die Folge haengt NUR an der builds-Reihenfolge (nicht am Worker).
static std::vector<ex::ProgressDelta> delta_sequence(ex::StaticBinaryView const&         view,
                                                     std::vector<ex::BuildResult> const& builds) {
    std::vector<std::vector<std::size_t>> configs;
    configs.reserve(builds.size());
    for (ex::BuildResult const& b : builds) configs.push_back(view.variant_tuple(b.index));
    return ex::compute_progress_deltas(configs);
}

int main() {
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 1: effective_build_workers -- der harte COMDARE_BUILD_PARALLEL-Override vs die Heuristik.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // build_parallelism==0 => parallel_jobs()-Heuristik = EXAKT heute (byte-neutral): 16 Kerne / 4 = 4, auf k gekappt.
    check_eq("bp=0 => Heuristik (16 Kerne/4, k=100) == 4",
             (ex::BuildConfig{4, 16, {}, {}}).effective_build_workers(100), std::size_t{4});
    check_eq("bp=0 => Heuristik auf k gekappt (k=3)", (ex::BuildConfig{4, 16, {}, {}}).effective_build_workers(3),
             std::size_t{3});
    {
        ex::BuildConfig cfg{4, 16, {}, {}};
        cfg.build_parallelism = 8; // Override GEWINNT ueber die Heuristik (die 4 ergeben wuerde)
        check_eq("bp=8 => Override gewinnt (k=100)", cfg.effective_build_workers(100), std::size_t{8});
        check_eq("bp=8 => auf k gekappt (k=3)", cfg.effective_build_workers(3), std::size_t{3});
    }
    {
        ex::BuildConfig cfg{4, 16, {}, {}};
        cfg.build_parallelism = 1; // strikt sequenziell (= heutiges Mess-Kontext-Verhalten auf kleinem Runner)
        check_eq("bp=1 => strikt sequenziell (1 Worker)", cfg.effective_build_workers(100), std::size_t{1});
    }
    check_eq("k==0 => mind. 1 Worker (Fortschritt)", (ex::BuildConfig{4, 16, {}, {}}).effective_build_workers(0),
             std::size_t{1});
    // Byte-Neutralitaet: bei bp==0 ist effective_build_workers == das alte min(parallel_jobs(), k) fuer JEDES k.
    {
        ex::BuildConfig const cfg{2, 8, {}, {}}; // parallel_jobs() == 4
        bool                  neutral = true;
        for (std::size_t k = 0; k <= 10; ++k)
            if (cfg.effective_build_workers(k) != std::min(cfg.parallel_jobs(), k == 0 ? std::size_t{1} : k))
                neutral = false;
        check_true("bp=0 identisch zu min(parallel_jobs(), k) (byte-neutral)", neutral);
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 2: env_parallelism_value -- der reine COMDARE_BUILD_PARALLEL-Parser (Facade-Rand ruft getenv).
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    check_eq("env(nullptr) == 0 (ungesetzt => Heuristik)", ex::env_parallelism_value(nullptr), std::size_t{0});
    check_eq("env(\"\") == 0", ex::env_parallelism_value(""), std::size_t{0});
    check_eq("env(\"0\") == 0", ex::env_parallelism_value("0"), std::size_t{0});
    check_eq("env(\"1\") == 1", ex::env_parallelism_value("1"), std::size_t{1});
    check_eq("env(\"4\") == 4", ex::env_parallelism_value("4"), std::size_t{4});
    check_eq("env(\"abc\") == 0 (nicht-numerisch)", ex::env_parallelism_value("abc"), std::size_t{0});
    // End-to-End ueber die reale Env-Variable (setenv -> getenv -> Parser), wie der Facade-Rand es tut.
#ifndef _WIN32
    ::setenv("COMDARE_BUILD_PARALLEL", "4", 1);
    check_eq("getenv(COMDARE_BUILD_PARALLEL)=4 -> Parser==4",
             ex::env_parallelism_value(std::getenv("COMDARE_BUILD_PARALLEL")), std::size_t{4});
    ::unsetenv("COMDARE_BUILD_PARALLEL");
#endif

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 3: DAS KERN-GATE -- provision_all liefert POSITIONS-TREUE results[] + eine IDENTISCHE Progress-
    //         Delta-Folge, egal ob sequenziell (bp=1), heuristisch (bp=0) oder parallel (bp=4), UND selbst
    //         unter erzwungener Out-of-Order-Completion.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    auto                 factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree   tree    = make_tree(factory);
    ex::StaticBinaryView view    = tree.static_binary_view();
    check_eq("StaticBinaryView.size (1x4x2)", view.size(), std::size_t{8});

    std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_w6";
    std::error_code             ec;
    std::filesystem::remove_all(base, ec);

    // Ein Lauf mit gegebener Worker-Zahl (bp) auf einem FRISCHEN Ausgabe-Verzeichnis. Leere build_version =>
    // dll_is_current immer false => nie skippen => jeder Lauf baut alle 8 (deterministisch vergleichbar).
    auto run = [&](std::size_t bp, char const* tag) -> std::vector<ex::BuildResult> {
        std::filesystem::path const     out = base / tag;
        std::array<std::atomic<int>, 8> built{}; // je Index genau 1x?
        std::atomic<int>                spawn_order{0};
        // Invertierter Jitter: hoeherer Index -> kuerzerer Sleep -> frueherer Abschluss => Out-of-Order-Completion
        // (die niederen j-Slots werden ZULETZT fertig, obwohl sie ZUERST gegriffen wurden).
        auto compile_stub = [&](ex::BuildJob const& j) -> int {
            built[j.index].fetch_add(1);
            spawn_order.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(4 * (8 - j.index))));
            return 0;
        };
        auto gen_stub = [](std::string const&) { return std::string{"// w6-stub\n"}; };

        ex::BuildConfig cfg;
        cfg.cores_per_build    = 2;
        cfg.total_cores        = 8; // parallel_jobs() == 4 (relevant nur fuer bp==0)
        cfg.source_dir         = out / "src";
        cfg.output_dir         = out / "dll";
        cfg.per_binary_subdirs = true; // wie der reale Treiber (disjunkte per-Binary-Ordner)
        cfg.build_version.clear();     // nie skippen
        cfg.build_parallelism = bp;

        ex::BuildOrchestrator orch{cfg, compile_stub, gen_stub};
        ex::BuildStats        stats;
        auto                  results = orch.provision_all(view, &stats);

        // Grund-Invarianten je Lauf:
        bool indexed = true, once = true;
        for (std::size_t i = 0; i < results.size(); ++i) {
            if (results[i].index != i) indexed = false; // results[j] <-> view[j] (positions-treu)
            if (built[i].load() != 1) once = false;     // jede Binary GENAU 1x gebaut
        }
        check_true((std::string{tag} + ": results[i] positions-treu zu view[i]").c_str(), indexed);
        check_true((std::string{tag} + ": jede Binary GENAU 1x gebaut").c_str(), once);
        check_eq((std::string{tag} + ": stats.succeeded").c_str(), stats.succeeded, std::size_t{8});
        // Disjunkte per-Binary-Ausgabe-Verzeichnisse (Parallel-Schreib-Sicherheit): 8 distinkte Parent-Pfade.
        std::vector<std::string> parents;
        for (auto const& r : results) parents.push_back(r.output.parent_path().string());
        std::sort(parents.begin(), parents.end());
        check_eq((std::string{tag} + ": 8 DISJUNKTE per-Binary-Ordner").c_str(),
                 static_cast<std::size_t>(std::unique(parents.begin(), parents.end()) - parents.begin()),
                 std::size_t{8});
        return results;
    };

    auto const builds_seq  = run(1, "seq_bp1");  // strikt sequenziell
    auto const builds_heur = run(0, "heur_bp0"); // Heuristik (parallel_jobs()==4) = heutiges Ist
    auto const builds_par  = run(4, "par_bp4");  // harter Override = 4 parallele Worker

    // (a) gleiche DLL-Menge: identische Index-Folge in results[] ueber ALLE Worker-Zahlen.
    bool same_index_order = builds_seq.size() == builds_par.size() && builds_seq.size() == builds_heur.size();
    for (std::size_t j = 0; same_index_order && j < builds_seq.size(); ++j)
        if (builds_seq[j].index != builds_par[j].index || builds_seq[j].index != builds_heur[j].index)
            same_index_order = false;
    check_true("(a) Index-Folge results[] IDENTISCH: seq == heur == par", same_index_order);

    // (b) DAS GATE: die Progress-Delta-Folge ist IDENTISCH zwischen sequenziell und parallel (=4) -- die
    //     deterministische Fenster-/Index-Reihenfolge ist worker-zahl-invariant (trotz Out-of-Order-Completion).
    auto const deltas_seq = delta_sequence(view, builds_seq);
    auto const deltas_par = delta_sequence(view, builds_par);
    check_true("(b) Progress-Delta-Folge seq == par (KERN-GATE, deterministisch)", deltas_seq == deltas_par);
    // Struktur-Kontrolle: 8 Konfig-Deltas + genau 1 done-Delta am Fensterende (§38.b-Fertig-Signal).
    check_eq("Delta-Folge: 8 Konfig + 1 done", deltas_par.size(), std::size_t{9});
    check_true("letztes Delta = done (Fensterende)", deltas_par.back().done && deltas_par.back().changed.empty());
    check_eq("done-Cursor == Fenster-Groesse", deltas_par.back().cursor, std::size_t{8});
    // Erste Meldung = Voll-Konfiguration (alle 3 statischen Ebenen gelistet), danach mixed-radix-minimale Deltas.
    check_eq("erste Meldung = Voll-Konfiguration (3 Ebenen)", deltas_par.front().changed.size(), std::size_t{3});

    std::cout << "\n==== W6 paralleler provision-Bau (§32-F7): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
