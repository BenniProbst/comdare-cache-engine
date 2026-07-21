// test_kf16_build_orchestrator — KF-16 (2026-06-02)
// StaticBinaryView (indizierter Iterator über den statischen Teilbaum) + BuildOrchestrator (multithreaded
// DLL-Bereitstellung, Default 2 Kerne/Build, alle CPU-Kerne). Orchestrierung deterministisch via Stub-CompileFn.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf16_build_orchestrator.cpp

#include "builder/build_orchestrator/build_orchestrator.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "builder/experiment_tree/ceb_generator.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
void       check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static ex::ExperimentTree make_tree(std::shared_ptr<ex::ExperimentNodeFactory> const& f) {
    ex::ExperimentTree t{f};
    t.build({
        ex::AxisLevel{"traversal", {"ART"}, true, ""},                        // gepinnt
        ex::AxisLevel{"node", {"N4", "N16", "N48", "N256"}, true, ""},        // freigegeben
        ex::AxisLevel{"node.cl_line", {"64", "128"}, true, ""},               // statische cacheline-Sub-Ebene
        ex::AxisLevel{"concurrency", {"1", "2", "4"}, false, "thread_count"}, // dynamisch (keine Binary)
    });
    return t;
}

int main() {
    // ── Teil 1: parallel_jobs-Mathematik (2 Kerne/Build, alle Kerne) ──
    check_eq("parallel_jobs(2 je Build, 16 Kerne)", (ex::BuildConfig{2, 16, {}, {}}).parallel_jobs(), std::size_t{8});
    check_eq("parallel_jobs(4 je Build, 16 Kerne)", (ex::BuildConfig{4, 16, {}, {}}).parallel_jobs(), std::size_t{4});
    check_eq("parallel_jobs(2 je Build, 1 Kern → min 1)", (ex::BuildConfig{2, 1, {}, {}}).parallel_jobs(),
             std::size_t{1});
    check_eq("parallel_jobs(0 je Build → wie 1)", (ex::BuildConfig{0, 8, {}, {}}).parallel_jobs(), std::size_t{8});
    check_true("effective_total(0) > 0 (Hardware-Concurrency)", (ex::BuildConfig{2, 0, {}, {}}).effective_total() > 0);

    // ── Teil 2: StaticBinaryView (indiziert + iterierbar) ──
    auto                 factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree   tree    = make_tree(factory);
    ex::StaticBinaryView view    = tree.static_binary_view();
    check_eq("StaticBinaryView.size (1x4x2)", view.size(), std::size_t{8});
    check_eq("view[0].index", view[0].index, std::size_t{0});
    check_eq("view[0].binary_id", view[0].binary_id, std::string{"traversal=ART/node=N4/node.cl_line=64"});
    check_eq("view[0].axes (3 statische Ebenen)", view[0].axes.size(), std::size_t{3});
    check_true("view[0].axes[0] == (traversal, ART)",
               view[0].axes[0] == std::make_pair(std::string{"traversal"}, std::string{"ART"}));
    check_true("view[0].axes[2] == (node.cl_line, 64)",
               view[0].axes[2] == std::make_pair(std::string{"node.cl_line"}, std::string{"64"}));
    // Indizes 0..7 lückenlos + Iteration
    {
        std::size_t expect     = 0;
        bool        contiguous = true;
        for (auto const& s : view) {
            if (s.index != expect++) contiguous = false;
        }
        check_true("Iterator: Indizes 0..7 lückenlos", contiguous && expect == 8);
    }

    std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_kf16";
    std::error_code             ec;
    std::filesystem::remove_all(base, ec);

    // ── Teil 3: provision_all mit Stub-CompileFn — deterministische Orchestrierung ──
    {
        std::array<std::atomic<int>, 8> built{}; // je Index genau 1×?
        auto                            compile_stub = [&](ex::BuildJob const& j) -> int {
            built[j.index].fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(15)); // Überlappung erzwingen (Parallelität)
            return 0;
        };
        ex::BuildConfig       cfg{2, 8, base / "src", base / "dll"};              // 8 Kerne / 2 = 4 parallele Jobs
        ex::BuildOrchestrator orch{cfg, compile_stub, &ex::generate_perm_source}; // echte KF-8-Quellen
        ex::BuildStats        stats;
        auto                  results = orch.provision_all(view, &stats);

        check_eq("provision_all: Ergebnis-Zahl", results.size(), std::size_t{8});
        bool all_ok = true, indexed = true, once = true;
        for (std::size_t i = 0; i < results.size(); ++i) {
            if (!results[i].ok()) all_ok = false;
            if (results[i].index != i) indexed = false;
            if (built[i].load() != 1) once = false;
        }
        check_true("alle 8 Builds ok", all_ok);
        check_true("results[i] indiziert zu view[i]", indexed);
        check_true("jede Binary GENAU 1× gebaut", once);
        check_eq("stats.succeeded", stats.succeeded, std::size_t{8});
        check_eq("stats.failed", stats.failed, std::size_t{0});
        check_true("Peak-Parallelität ≤ parallel_jobs (Cap=4)", stats.peak_concurrency <= 4);
        check_true("Peak-Parallelität ≥ 2 (echtes Multithreading)", stats.peak_concurrency >= 2);

        // KF-8-Quellen real geschrieben (perm_<id>.cpp) + gültiger Inhalt
        std::size_t cpp = 0;
        for (auto const& e : std::filesystem::directory_iterator(base / "src"))
            if (e.path().extension() == ".cpp") ++cpp;
        check_eq("perm_*.cpp auf Disk (je Binary)", cpp, std::size_t{8});
        std::filesystem::path const sample = (base / "src") / ("perm_" + ex::orch_sanitize(view[0].binary_id) + ".cpp");
        check_true("Quelle = echter KF-8-Source (comdare_perm_descriptor)", [&] {
            std::ifstream f{sample};
            std::string   s((std::istreambuf_iterator<char>(f)), {});
            return s.find("comdare_perm_descriptor") != std::string::npos;
        }());
    }

    // ── Teil 4: Fehler-Propagierung (CompileFn-Exit ≠ 0) ──
    {
        auto                  compile_fail3 = [](ex::BuildJob const& j) -> int { return (j.index == 3) ? 7 : 0; };
        ex::BuildConfig       cfg{2, 4, base / "src2", base / "dll2"};
        ex::BuildOrchestrator orch{cfg, compile_fail3, [](std::string const&) { return std::string{"// stub\n"}; }};
        ex::BuildStats        stats;
        auto                  results = orch.provision_all(view, &stats);
        check_eq("Fehler-Binary status", results[3].status, 7);
        check_true("Fehler-Binary !ok", !results[3].ok());
        check_eq("stats.failed", stats.failed, std::size_t{1});
        check_eq("stats.succeeded", stats.succeeded, std::size_t{7});
    }

    // ── Teil 5 (Bau-INC-2c.opt-b): opt_flag-Kanal in make_gpp_compile_fn — .rsp-Byte-Identität + Durchfluss ──
    // Durabler Guard: der Signatur-Default "-O2" MUSS byte-identisch zum vormals hartkodierten -O2 bleiben
    // (Verhaltensneutralität des Refactors), und opt_flag MUSS bis in die .rsp durchfliessen. Byte-Identität
    // wird am .rsp geprueft, NICHT an der .so (deren Bytes tragen abs. Pfade + build-id/timestamps und
    // variieren lauf-zu-lauf unabhaengig von opt_flag). g++-Exit egal — die .rsp wird VOR dem Spawn geschrieben.
    {
        auto const   tmp = comdare::test::user_tmp_dir();
        ex::BuildJob job;
        job.source = tmp / "optb_probe.cpp";
        job.output = tmp / "optb_probe.so";
        {
            std::ofstream s{job.source};
            s << "int main(){return 0;}\n";
        }
        auto rsp_of = [&](ex::CompileFn const& fn) -> std::string {
            std::error_code ec;
            std::filesystem::remove(job.output.string() + ".rsp", ec);
            (void)fn(job); // schreibt .rsp, dann g++ (Exit ignoriert — nur die .rsp interessiert)
            std::ifstream f{job.output.string() + ".rsp"};
            return std::string((std::istreambuf_iterator<char>(f)), {});
        };
        std::string const rsp_default = rsp_of(ex::make_gpp_compile_fn()); // Default opt_flag="-O2"
        std::string const rsp_o2      = rsp_of(ex::make_gpp_compile_fn({}, {}, "g++-16", {}, "-O2")); // explizit -O2
        std::string const rsp_o3      = rsp_of(ex::make_gpp_compile_fn({}, {}, "g++-16", {}, "-O3")); // -O3
        check_true("opt-b: Default-.rsp traegt -O2", rsp_default.find("-O2\n") != std::string::npos);
        check_true("opt-b: Default-.rsp == explizit-O2 (Byte-Identitaet des Refactors)", rsp_default == rsp_o2);
        check_true("opt-b: -O3-.rsp traegt -O3", rsp_o3.find("-O3\n") != std::string::npos);
        check_true("opt-b: -O3-.rsp traegt NICHT -O2", rsp_o3.find("-O2\n") == std::string::npos);
        check_true("opt-b: opt_flag fliesst durch (default != O3)", rsp_default != rsp_o3);
    }

    // ── Teil 6 (Scheibe 2b, Ledger 61/62 + §62-G (4)): Build-Typ-Debug-Flags-Naht + .rsp-Byte-Stabilitaet ──
    // Die toolchain-abstrahierte Naht debug_flags_for_toolchain() liefert die Debug-Flags (-O0 -g). Die
    // Facade reicht sie als opt_flag-WERT herunter (achsen-blinder Builder). Byte-Stabilitaets-Wache: ein
    // normaler opt_flag ergibt eine .rsp OHNE -g; der Debug-Flag-Wert ergibt eine .rsp MIT -O0 UND -g und
    // OHNE -O3. Prueft am .rsp-String (Kommando-Vergleich), nicht an der .so (deren Bytes variieren).
    {
        check_eq("2b: debug_flags_for_toolchain()", ex::debug_flags_for_toolchain(), std::string{"-O0 -g"});

        auto const   tmp = comdare::test::user_tmp_dir();
        ex::BuildJob job;
        job.source = tmp / "dbg_probe.cpp";
        job.output = tmp / "dbg_probe.so";
        {
            std::ofstream s{job.source};
            s << "int main(){return 0;}\n";
        }
        auto rsp_of = [&](ex::CompileFn const& fn) -> std::string {
            std::error_code ec;
            std::filesystem::remove(job.output.string() + ".rsp", ec);
            (void)fn(job);
            std::ifstream f{job.output.string() + ".rsp"};
            return std::string((std::istreambuf_iterator<char>(f)), {});
        };
        // Facade-Debug-Kanal simuliert: opt_flag == debug_flags_for_toolchain() (so montiert die Facade bei Debug).
        std::string const rsp_dbg =
            rsp_of(ex::make_gpp_compile_fn({}, {}, "g++-16", {}, ex::debug_flags_for_toolchain()));
        std::string const rsp_o3 = rsp_of(ex::make_gpp_compile_fn({}, {}, "g++-16", {}, "-O3"));
        check_true("2b: Debug-.rsp traegt -O0 -g", rsp_dbg.find("-O0 -g\n") != std::string::npos);
        check_true("2b: Debug-.rsp traegt Debug-Info -g", rsp_dbg.find("-g") != std::string::npos);
        check_true("2b: Debug-.rsp traegt NICHT -O3", rsp_dbg.find("-O3\n") == std::string::npos);
        check_true("2b: Release-.rsp (-O3) traegt KEIN -g (byte-stabil, kein Debug-Leck)",
                   rsp_o3.find("-g\n") == std::string::npos && rsp_o3.find("-O0") == std::string::npos);
        check_true("2b: Debug- != Release-.rsp (echter Compile-Unterschied)", rsp_dbg != rsp_o3);
    }

    std::cout << "\n==== KF-16 StaticBinaryView + BuildOrchestrator: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
