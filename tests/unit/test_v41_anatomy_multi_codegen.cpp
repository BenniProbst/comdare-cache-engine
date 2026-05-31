// V41.F.6.1.R5.D.3 — Multi-Permutation-Codegen + load_all End-to-End
//
// Beweist:
// 1. comdare_codegen_anatomy_module_list generiert N SHARED-DLLs
// 2. Alle DLLs landen im gemeinsamen Output-Directory (RUNTIME_OUTPUT_DIRECTORY-Override)
// 3. AnatomyModuleLoader::load_all(dir) findet + laedt alle (Pattern: comdare_anatomy_perm_*)
// 4. Pro geladenem Handle: composition_name match erwartete Composition
// 5. Set-Vergleich (Reihenfolge irrelevant, weil load_all sortiert nach Dateiname)
// 6. Lifecycle pro Handle: warm_up -> run -> shutdown (alle 3 Compositions parallel)
//
// Pilot-Liste (V41.P2): Art / Hot / Start / Surf / Wormhole / Masstree — 6 sezierte ORGAN-Compositions
// (search_algo = Observable-Organe, direktiven-konform; Umstufung-B im Mess-Pfad).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §47
// @task #709 V41.F.6.1.R5.D.3

#include <gtest/gtest.h>

#include <anatomy/anatomy_base.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace ee     = ::comdare::cache_engine::execution_engine;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;

#ifndef COMDARE_R5D3_PILOT_DIR
  #error "COMDARE_R5D3_PILOT_DIR must be defined via CMake (target_compile_definitions)"
#endif

namespace {

[[nodiscard]] std::filesystem::path pilot_dir() {
    return std::filesystem::path{COMDARE_R5D3_PILOT_DIR};
}

}  // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Pilot-Verzeichnis existiert + enthaelt N DLLs
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, PilotDirectoryExists) {
    auto const dir = pilot_dir();
    EXPECT_TRUE(std::filesystem::is_directory(dir))
        << "Pilot-Verzeichnis nicht gefunden: " << dir.string();
}

TEST(R5D3_MultiCodegen, PilotDirectoryContainsExpectedDlls) {
    auto const dir = pilot_dir();
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
    EXPECT_EQ(count, 6u) << "Erwartet 6 anatomy-Pilot-DLLs in " << dir.string();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — load_all liefert 6 Handles (6 sezierte Organ-Kompositionen)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, LoadAllReturnsSixHandles) {
    std::vector<loader::AnatomyModuleHandle> handles;
    int const status = loader::AnatomyModuleLoader::load_all(pilot_dir(), handles);
    ASSERT_EQ(status, loader::status_ok)
        << "load_all status: " << loader::status_name(status);
    EXPECT_EQ(handles.size(), 6u);
    for (auto const& h : handles) {
        EXPECT_TRUE(h.valid());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Geladene Composition-Names matchen die 6 Pilot-Compositions (Set-Vergleich)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, LoadAllProducesCorrectCompositionSet) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);
    ASSERT_EQ(handles.size(), 6u);

    std::set<std::string> seen;
    for (auto& h : handles) {
        ASSERT_NE(h.anatomy(), nullptr);
        seen.insert(std::string{h.anatomy()->composition_name()});
    }

    // V41.P2: alle 6 sezierten Organ-Compositions (search_algo = Observable-Organe, direktiven-konform).
    EXPECT_TRUE(seen.contains("ArtComposition"))      << "ArtComposition fehlt im load_all-Set";
    EXPECT_TRUE(seen.contains("HotComposition"))      << "HotComposition fehlt im load_all-Set";
    EXPECT_TRUE(seen.contains("StartComposition"))    << "StartComposition fehlt im load_all-Set";
    EXPECT_TRUE(seen.contains("SurfComposition"))     << "SurfComposition fehlt im load_all-Set";
    EXPECT_TRUE(seen.contains("WormholeComposition")) << "WormholeComposition fehlt im load_all-Set";
    EXPECT_TRUE(seen.contains("MasstreeComposition")) << "MasstreeComposition fehlt im load_all-Set";
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Pro Handle: korrekte Pflicht-Properties (genus/organ_count/engine_kind)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, AllHandlesHaveSearchAlgorithmGenus) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);
    ASSERT_EQ(handles.size(), 6u);

    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_EQ(a->genus(),       ana::AnatomyGenus::SearchAlgorithm);
        EXPECT_EQ(a->organ_count(), 17u);
        EXPECT_EQ(a->engine_kind(), ee::ExecutionEngineKind::Anatomy);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Lifecycle pro Handle independent (warm_up->run->shutdown)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, LifecyclePerHandleIsIndependent) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);
    ASSERT_EQ(handles.size(), 6u);

    // Alle warm_up
    for (auto& h : handles) {
        h.anatomy()->warm_up();
        EXPECT_EQ(h.anatomy()->lifecycle_state(), ee::EngineLifecycleState::Warming);
    }
    // Alle run
    for (auto& h : handles) {
        h.anatomy()->run();
        EXPECT_EQ(h.anatomy()->lifecycle_state(), ee::EngineLifecycleState::Running);
    }
    // Alle shutdown
    for (auto& h : handles) {
        h.anatomy()->shutdown();
        EXPECT_EQ(h.anatomy()->lifecycle_state(), ee::EngineLifecycleState::Shutdown);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Polymorpher Mess-Loop (CacheEngineBuilder-Pattern R5.F Vorbereitung)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D3_MultiCodegen, PolymorphicMeasurementLoopOverAllHandles) {
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(pilot_dir(), handles),
              loader::status_ok);

    // Pattern: pro geladene Permutation eine Mess-Iteration (R5.F skeleton)
    std::vector<std::string_view> measured_compositions;
    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);

        a->warm_up();
        a->run();
        measured_compositions.push_back(a->composition_name());
        a->reset();
        a->shutdown();

        EXPECT_EQ(a->lifecycle_state(), ee::EngineLifecycleState::Shutdown);
    }

    EXPECT_EQ(measured_compositions.size(), 6u);
}
