// V41.F.6.1 R5.G — Skalierungs-Schluss: ALLE auto-emittierten Permutations-DLLs eines Raums laden.
//
// Während test_v41_anatomy_adhoc_dll_load EINE auto-emittierte AdHoc-Permutation lädt, prüft dieser
// Test die SKALIERUNG: der C++-Auto-Emitter (apps/adhoc_emitter) hat zur Configure-Time den GANZEN
// Pilot-Raum enumeriert (for_each_composition_type), pro Permutation ein Modul-.cpp geschrieben, und
// CMake (comdare_build_adhoc_modules) hat JEDES als eigene SHARED-DLL gebaut. Hier wird das ganze
// Verzeichnis via load_all geladen + jede DLL als vollwertige, mess-fähige Anatomie verifiziert.
//
// Damit ist R5.G end-to-end UND skaliert: Raum → N Modul-.cpp → N Permutations-DLLs → N Anatomien.

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/measurable_workload.hpp>
#include <builder/commands/multi_compare.hpp>     // R5.D: volle F15-Auswertung über reale DLLs
#include <builder/commands/result_aggregator.hpp> // make_execution_result + Export

#include <gtest/gtest.h>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;
namespace cmd    = ::comdare::cache_engine::builder::commands;
namespace stats  = ::comdare::cache_engine::builder::commands::stats;

TEST(R5G_AdHocAutoBuilt, AllEmittedPermutationsLoadAsDllsAndMeasure) {
    std::filesystem::path const dir{COMDARE_R5G_AUTOBUILT_DLL_DIR};
    constexpr std::size_t       expected = COMDARE_R5G_AUTOBUILT_COUNT; // = Pilot-Raum-Permutations-Zahl

    std::vector<loader::AnatomyModuleHandle> handles;
    int const                                st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_EQ(handles.size(), expected) << "Erwartet " << expected
                                        << " auto-gebaute Permutations-DLLs, geladen: " << handles.size();

    std::size_t measured = 0;
    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_EQ(a->composition_name(), std::string_view{"AdHocComposition"});
        EXPECT_EQ(a->organ_count(), 19u);
        EXPECT_EQ(a->genus(), ana::AnatomyGenus::SearchAlgorithm);

        // Jede auto-gebaute DLL ist mess-fähig (Stufe B in-DLL).
        auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(a);
        ASSERT_NE(mw, nullptr);
        std::vector<std::int64_t> samples(5);
        auto const                n =
            mw->run_workload(/*ops_per_batch=*/500, /*batches=*/5, /*seed=*/7u, samples.data(), samples.size());
        EXPECT_EQ(n, 5u);
        if (n == 5u) ++measured;
    }
    EXPECT_EQ(measured, expected) << "Nicht alle auto-gebauten DLLs waren mess-fähig.";
}

// R5.D — DURCHGAENGIGER F15-MESSLAUF-TREIBER ueber REALE materialisierte Permutations-DLLs:
// load_all → run_workload je DLL → make_execution_result → multi_compare vs Baseline → summarize →
// Export. Demonstriert die ganze F15-Pipeline auf ECHTEN geladenen Kompositionen (nicht synthetisch).
// Die R5.G-Pilot-DLLs variieren search_algo (Array256/VectorU8U8/VectorU16U16) → echt vergleichbar.
TEST(R5G_AdHocAutoBuilt, FullF15DriverOverRealDlls) {
    std::filesystem::path const              dir{COMDARE_R5G_AUTOBUILT_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 2u) << "fuer einen Vergleich werden >= 2 DLLs gebraucht.";

    // 1) Jede geladene DLL messen (Stufe B) → ExecutionResult. Namen stabil halten (string_view!).
    std::vector<std::string> names;
    names.reserve(handles.size());
    std::vector<cmd::ExecutionResult> results;
    results.reserve(handles.size());
    for (std::size_t i = 0; i < handles.size(); ++i) {
        auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(handles[i].anatomy());
        ASSERT_NE(mw, nullptr);
        std::vector<std::int64_t> samples(64);
        auto const                n =
            mw->run_workload(/*ops_per_batch=*/2000, /*batches=*/64, /*seed=*/11u, samples.data(), samples.size());
        ASSERT_GE(n, 2u);
        samples.resize(static_cast<std::size_t>(n));
        names.push_back("perm_" + std::to_string(i));
        results.push_back(cmd::make_execution_result(names.back(), std::move(samples)));
    }

    // 2) Baseline = results[0]; Kandidaten = Rest. Multi-Compare (Welch + Holm-FWER) + Summary.
    std::vector<cmd::ExecutionResult> candidates(results.begin() + 1, results.end());
    auto                              rep =
        stats::multi_compare_against_baseline(results.front(), std::span<const cmd::ExecutionResult>{candidates}, 0.05);
    auto sum = stats::summarize(rep);

    // 3) Report ist WOHL-GEFORMT (kein bestimmter Verdict — der ist hardware-/last-abhaengig).
    EXPECT_EQ(rep.comparisons.size(), handles.size() - 1);
    EXPECT_EQ(sum.total, handles.size() - 1);
    EXPECT_EQ(sum.significant_faster + sum.significant_slower + sum.not_significant, sum.total);
    EXPECT_GE(sum.win_rate, 0.0);
    EXPECT_LE(sum.win_rate, 1.0);
    for (auto const& c : rep.comparisons) {
        EXPECT_TRUE(c.welch.valid) << c.name; // >= 64 Samples pro Gruppe → gueltig
        EXPECT_GE(c.adjusted_p, 0.0);
        EXPECT_LE(c.adjusted_p, 1.0);
    }
    // 4) Export der realen Mess-Kette ist nicht-leer + maschinenlesbar.
    EXPECT_FALSE(stats::report_to_csv(rep).empty());
    auto json = stats::report_to_json(rep);
    EXPECT_EQ(json.front(), '{');
    EXPECT_NE(json.find("\"comparisons\":["), std::string::npos);
}
