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

#include <gtest/gtest.h>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

TEST(R5G_AdHocAutoBuilt, AllEmittedPermutationsLoadAsDllsAndMeasure) {
    std::filesystem::path const dir{COMDARE_R5G_AUTOBUILT_DLL_DIR};
    constexpr std::size_t expected = COMDARE_R5G_AUTOBUILT_COUNT;  // = Pilot-Raum-Permutations-Zahl

    std::vector<loader::AnatomyModuleHandle> handles;
    int const st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_EQ(handles.size(), expected)
        << "Erwartet " << expected << " auto-gebaute Permutations-DLLs, geladen: " << handles.size();

    std::size_t measured = 0;
    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_EQ(a->composition_name(), std::string_view{"AdHocComposition"});
        EXPECT_EQ(a->organ_count(), 17u);
        EXPECT_EQ(a->genus(), ana::AnatomyGenus::SearchAlgorithm);

        // Jede auto-gebaute DLL ist mess-fähig (Stufe B in-DLL).
        auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(a);
        ASSERT_NE(mw, nullptr);
        std::vector<std::int64_t> samples(5);
        auto const n = mw->run_workload(/*ops_per_batch=*/500, /*batches=*/5, /*seed=*/7u,
                                        samples.data(), samples.size());
        EXPECT_EQ(n, 5u);
        if (n == 5u) ++measured;
    }
    EXPECT_EQ(measured, expected) << "Nicht alle auto-gebauten DLLs waren mess-fähig.";
}
