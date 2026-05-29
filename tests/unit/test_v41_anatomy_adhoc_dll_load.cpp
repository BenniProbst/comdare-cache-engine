// V41.F.6.1 R5.G — End-to-End-Schluss: auto-emittierte AdHoc-Permutation als SHARED-DLL laden + messen.
//
// Lädt die aus auto_emitted_perm_module.cpp gebaute Permutations-DLL (comdare_anatomy_perm_r5g_adhoc)
// via AnatomyModuleLoader und prüft, dass eine AUTO-ENUMERIERTE AdHoc-Komposition (nicht eine benannte
// Composition wie bei F.5) als vollwertige, mess-fähige Anatomie geladen wird. Damit ist die
// R5.G-Materialisierung end-to-end geschlossen: Emitter-Ausgabe → SHARED-DLL → geladen → run_workload.

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

TEST(R5G_AdHocDllLoad, AutoEmittedAdHocPermutationLoadsAsDllAndRuns) {
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    int const st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_GE(handles.size(), 1u) << "auto-emittierte AdHoc-DLL nicht geladen.";

    auto* a = handles[0].anatomy();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->composition_name(), std::string_view{"AdHocComposition"});  // auto-enumerierte Komposition
    EXPECT_EQ(a->organ_count(), 17u);
    EXPECT_EQ(a->genus(), ana::AnatomyGenus::SearchAlgorithm);

    // Mess-Last (Stufe B) läuft auch in der auto-emittierten DLL.
    auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(a);
    ASSERT_NE(mw, nullptr);
    std::vector<std::int64_t> samples(10);
    auto const n = mw->run_workload(/*ops_per_batch=*/1000, /*batches=*/10, /*seed=*/3u,
                                    samples.data(), samples.size());
    EXPECT_EQ(n, 10u);
    EXPECT_GT(samples[0], 0);
}
