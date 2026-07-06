// D1 (L-SEL BuildSelection) + D2 (L-73 provision_all O(K)) — Verifikations-Test, header-only standalone.
// Beweis-Kern: aus einer 1e12-View werden 100 Binaries selektiert + gebaut → results-Vektor ist O(100),
// NICHT O(1e12) (vor dem Fix: `results(view.size())` = sofortiger OOM). Keine schweren Templates → RAM-leicht.
//
// Build: cl /std:c++latest /EHsc test_d1_d2_selection.cpp /I libs/cache_engine  (ein Include-Root, kein Boost).

#include "builder/experiment_tree/experiment_tree.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "builder/experiment_tree/coverage_selection.hpp"
#include "builder/build_orchestrator/build_orchestrator.hpp"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace comdare::cache_engine::builder::experiment;

static int  g_fail = 0;
static void check(bool cond, std::string const& msg) {
    std::cout << (cond ? "  [OK]  " : "  [FAIL] ") << msg << "\n";
    if (!cond) ++g_fail;
}

int main() {
    std::cout << "==== D1 BuildSelection (L-SEL) ====\n";

    // Pilot-View: 3 statische Ebenen 4 x 3 x 2 -> Produkt 24; max(level) = 4.
    std::vector<AxisLevel> levels = {
        {"axisA", {"a0", "a1", "a2", "a3"}, true, "", "blkA"},
        {"axisB", {"b0", "b1", "b2"}, true, "", "blkB"},
        {"axisC", {"c0", "c1"}, true, "", "blkC"},
    };
    StaticBinaryView view{levels};
    check(view.size() == 24, "Pilot-View size() == 24 (4*3*2)");
    check(view.level_count() == 3, "level_count() == 3");
    check(view.level_size(0) == 4 && view.level_size(1) == 3 && view.level_size(2) == 2, "level_size 4/3/2");

    auto sel_full = select_full(view);
    check(sel_full.size() == 24 && sel_full.provenance == "full", "select_full size() == 24");

    auto sel_ow = select_one_wise(view);
    check(sel_ow.size() == 4, "select_one_wise size() == max(4,3,2) == 4 (NICHT 24)");

    // 1-wise-Garantie: jede Variante JEDER Achse mindestens 1x abgedeckt.
    std::set<std::string> covA, covB, covC;
    for (auto idx : sel_ow.indices) {
        auto spec = view[idx];
        for (auto const& [ax, val] : spec.axes) {
            if (ax == "axisA")
                covA.insert(val);
            else if (ax == "axisB")
                covB.insert(val);
            else if (ax == "axisC")
                covC.insert(val);
        }
    }
    check(covA.size() == 4 && covB.size() == 3 && covC.size() == 2,
          "1-wise deckt JEDE Variante JEDER Achse >=1x ab (A=4,B=3,C=2)");

    // flat_index Round-Trip: operator[](flat_index(tuple)) trifft genau das Tupel.
    std::vector<std::size_t> tup     = {2, 1, 0}; // a2 / b1 / c0
    auto                     rt_spec = view[view.flat_index(tup)];
    bool rt = rt_spec.axes.size() == 3 && rt_spec.axes[0].second == "a2" && rt_spec.axes[1].second == "b1" &&
              rt_spec.axes[2].second == "c0";
    check(rt, "flat_index Round-Trip: operator[](flat_index({2,1,0})) == a2/b1/c0");

    std::cout << "\n==== D1 OOM-Sicherheit: 1e12-View, Selektion bleibt klein ====\n";
    // 6 Ebenen x 100 Werte -> Produkt = 100^6 = 1e12, max(level) = 100.
    std::vector<AxisLevel> big;
    for (int e = 0; e < 6; ++e) {
        std::vector<std::string> vals;
        vals.reserve(100);
        for (int k = 0; k < 100; ++k) vals.push_back("v" + std::to_string(k));
        big.push_back({"big" + std::to_string(e), std::move(vals), true, "", "bb"});
    }
    StaticBinaryView bigv{big};
    check(bigv.size() == static_cast<std::size_t>(1000000000000ULL), "big-View size() == 1e12 (100^6)");
    auto sel_big_ow = select_one_wise(bigv);
    check(sel_big_ow.size() == 100, "select_one_wise(big) == 100 (max level), NICHT 1e12 (OOM-sicher)");
    auto sel_big_full = select_full(bigv);
    check(sel_big_full.empty() && sel_big_full.provenance.rfind("full-REFUSED", 0) == 0,
          "select_full(big) REFUSED statt 1e12-Materialisierung (kein OOM); provenance=" + sel_big_full.provenance);

    std::cout << "\n==== D2 provision_all O(K) (L-73) ====\n";
    std::atomic<int> compile_calls{0};
    std::atomic<int> gen_calls{0};
    BuildConfig      cfg;
    cfg.source_dir      = ::comdare::test::user_tmp_dir() / "comdare_d2_src";
    cfg.output_dir      = ::comdare::test::user_tmp_dir() / "comdare_d2_out";
    cfg.cores_per_build = 1;
    cfg.total_cores     = 2;
    // ram_per_build_bytes == 0 -> RAM-Gate aus (deterministisch testbar).
    CompileFn comp = [&](BuildJob const&) -> int {
        ++compile_calls;
        return 0;
    };
    SourceGenFn gen = [&](std::string const&) -> std::string {
        ++gen_calls;
        return "// stub\n";
    };
    BuildOrchestrator orch{cfg, comp, gen};

    // KERN-BEWEIS L-73: 100 selektierte Binaries aus 1e12-View -> results O(100), NICHT O(1e12).
    compile_calls = 0;
    gen_calls     = 0;
    BuildStats stats;
    auto       res = orch.provision_all(bigv, sel_big_ow.indices, &stats);
    check(res.size() == 100, "provision_all(big, selection) results.size() == 100 (O(K), NICHT 1e12 -> kein OOM)");
    check(compile_calls.load() == 100, "CompileFn nur 100x aufgerufen (nicht 1e12)");
    check(stats.total_jobs == 100 && stats.succeeded == 100, "stats: total_jobs==100, succeeded==100");

    // Rückwärts-kompatibler Overload (alle Binaries einer handhabbaren View).
    compile_calls = 0;
    auto res2     = orch.provision_all(view, &stats);
    check(res2.size() == 24 && compile_calls.load() == 24, "provision_all(view) rueckwaerts-kompatibel: 24 Builds");

    // explizite Selektion.
    auto sel_exp  = select_explicit({0, 5, 23});
    compile_calls = 0;
    auto res3     = orch.provision_all(view, sel_exp.indices, &stats);
    check(res3.size() == 3 && compile_calls.load() == 3, "select_explicit({0,5,23}) -> genau 3 Builds");
    bool idx_ok = res3.size() == 3 && res3[0].index == 0 && res3[1].index == 5 && res3[2].index == 23;
    check(idx_ok, "BuildResult.index traegt View-Index (0/5/23), Reihenfolge selektions-relativ");

    std::cout << "\n==== " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
