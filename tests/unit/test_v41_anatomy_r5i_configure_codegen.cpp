// V41.F.6.1.R5.I — Configure-Time Codegen End-to-End Test
//
// Beweist:
// 1. CMake-Function comdare_run_anatomy_codegen_tool() invoked Tool zur
//    Configure-Time und produziert Snippet-File
// 2. Snippet wird via include() in CMake-Configure eingelesen, Variablen gesetzt
// 3. comdare_codegen_anatomy_module_list() mit den Variablen produziert N DLLs
// 4. AnatomyModuleLoader::load_all(dir) findet alle Tool-erzeugten DLLs
// 5. Pro Handle: composition_name match Tool-Auswahl (art/hot/wormhole)
//
// Pilot-Auswahl in CMakeLists: --names "art,hot,wormhole" → 3 SHARED-DLLs.
//
// Voraussetzung: Tool muss vor diesem Configure-Pass gebaut sein. Wenn nicht,
// wird COMDARE_R5I_PILOT_DIR nicht definiert und dieser Test wird gar nicht
// kompiliert (Optional-Block im CMakeLists.txt).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §51
// @task #713 V41.F.6.1.R5.I

#include <gtest/gtest.h>

#include <anatomy/anatomy_base.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace ee     = ::comdare::cache_engine::execution_engine;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;

#ifndef COMDARE_R5I_PILOT_DIR
  #error "COMDARE_R5I_PILOT_DIR must be defined via CMake (target_compile_definitions). "
         "Run 2-Pass-Build: 1) cmake -B build, 2) cmake --build build --target comdare_anatomy_codegen_cli, "
         "3) cmake -B build (re-configure)."
#endif

namespace {

[[nodiscard]] std::filesystem::path pilot_dir() {
    return std::filesystem::path{COMDARE_R5I_PILOT_DIR};
}

}  // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Pilot-Verzeichnis enthaelt die 3 Tool-erzeugten DLLs
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5I_ConfigureCodegen, PilotDirectoryContainsThreeDlls) {
    auto const dir = pilot_dir();
    ASSERT_TRUE(std::filesystem::is_directory(dir))
        << "Pilot-Verzeichnis: " << dir.string();

    auto const suffix = loader::AnatomyModuleLoader::platform_suffix();
    std::size_t count = 0;
    for (auto const& entry : std::filesystem::directory_iterator{dir}) {
        if (!entry.is_regular_file()) continue;
        auto const& p = entry.path();
        if (p.filename().string().find("comdare_anatomy_perm_") == 0 &&
            p.extension() == suffix) {
            ++count;
        }
    }
    EXPECT_EQ(count, 3u) << "Erwartet 3 DLLs (art/hot/wormhole) in " << dir.string();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — load_all liefert 3 Handles
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5I_ConfigureCodegen, LoadAllReturnsThreeHandles) {
    std::vector<loader::AnatomyModuleHandle> handles;
    int const status = loader::AnatomyModuleLoader::load_all(pilot_dir(), handles);
    ASSERT_EQ(status, loader::status_ok)
        << "load_all status: " << loader::status_name(status);
    EXPECT_EQ(handles.size(), 3u);
    for (auto const& h : handles) {
        EXPECT_TRUE(h.valid());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Composition-Namen matchen Tool-Selektion (art/hot/wormhole)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5I_ConfigureCodegen, LoadedCompositionsMatchToolSelection) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);
    ASSERT_EQ(handles.size(), 3u);

    std::set<std::string> seen;
    for (auto& h : handles) {
        ASSERT_NE(h.anatomy(), nullptr);
        seen.insert(std::string{h.anatomy()->composition_name()});
    }

    EXPECT_TRUE(seen.contains("ArtComposition"))      << "ArtComposition fehlt";
    EXPECT_TRUE(seen.contains("HotComposition"))      << "HotComposition fehlt";
    EXPECT_TRUE(seen.contains("WormholeComposition")) << "WormholeComposition fehlt";
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Lifecycle pro Tool-erzeugtem Modul (warm_up→run→shutdown)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5I_ConfigureCodegen, LifecyclePerModuleFromToolGeneratedSnippet) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);

    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_EQ(a->genus(), ana::AnatomyGenus::SearchAlgorithm);
        EXPECT_EQ(a->engine_kind(), ee::ExecutionEngineKind::Anatomy);

        a->warm_up();
        EXPECT_EQ(a->lifecycle_state(), ee::EngineLifecycleState::Warming);
        a->run();
        EXPECT_EQ(a->lifecycle_state(), ee::EngineLifecycleState::Running);
        a->shutdown();
        EXPECT_EQ(a->lifecycle_state(), ee::EngineLifecycleState::Shutdown);
    }
}
