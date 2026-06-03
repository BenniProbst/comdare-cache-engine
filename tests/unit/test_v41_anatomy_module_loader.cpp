// V41.F.6.1.R5.E — AnatomyModuleLoader Tests
//
// Beweist:
// 1. platform_suffix() liefert .dll/.so/.dylib korrekt
// 2. load(non-existent) liefert status_file_not_found
// 3. load(valid-pilot-dll) liefert status_ok + Handle valid()
// 4. Handle->composition_name() match Pilot-Composition (WormholeComposition)
// 5. Handle->genus() == SearchAlgorithm
// 6. Lifecycle-Roundtrip via geladenem Handle (warm_up/run/reset/shutdown)
// 7. ABI-Version-Check: module_version match Host-Version
// 8. RAII: Destruktor entlaedt sauber (Pflicht: destroy ZUERST, dann FreeLibrary)
// 9. Move-Semantik (move-construct + move-assign)
// 10. Multiple-Load gleicher DLL → 2 unabhaengige Anatomy-Instanzen
//
// Pilot-DLL wird via comdare_codegen_anatomy_module(LIBRARY_TYPE SHARED)
// fuer WormholeComposition generiert. Pfad ueber COMDARE_R5E_PILOT_DLL
// Compile-Define gereicht.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §46
// @task #708 V41.F.6.1.R5.E

#include <gtest/gtest.h>

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <anatomy/anatomy_base.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace abi    = ::comdare::cache_engine::abi;
namespace ee     = ::comdare::cache_engine::execution_engine;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;

#ifndef COMDARE_R5E_PILOT_DLL
  #error "COMDARE_R5E_PILOT_DLL must be defined via CMake (target_compile_definitions)"
#endif

namespace {

[[nodiscard]] std::filesystem::path pilot_dll_path() {
    return std::filesystem::path{COMDARE_R5E_PILOT_DLL};
}

}  // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// §1 — platform_suffix
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, PlatformSuffixIsCorrect) {
    auto const suffix = loader::AnatomyModuleLoader::platform_suffix();
#if defined(_WIN32)
    EXPECT_EQ(suffix, std::string{".dll"});
#elif defined(__APPLE__)
    EXPECT_EQ(suffix, std::string{".dylib"});
#else
    EXPECT_EQ(suffix, std::string{".so"});
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Pilot-DLL Pfad existiert (Test-Setup-Sanity)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, PilotDllExists) {
    auto const path = pilot_dll_path();
    EXPECT_TRUE(std::filesystem::exists(path))
        << "Pilot-DLL nicht gefunden: " << path.string();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — load(non-existent) liefert status_file_not_found
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, LoadNonExistentReturnsNotFound) {
    loader::AnatomyModuleHandle handle;
    int const status = loader::AnatomyModuleLoader::load(
        "this_dll_does_not_exist.dll", handle);
    EXPECT_EQ(status, loader::status_file_not_found);
    EXPECT_FALSE(handle.valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — load(valid-pilot-dll) -> status_ok + Handle valid
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, LoadPilotDllSucceeds) {
    loader::AnatomyModuleHandle handle;
    int const status = loader::AnatomyModuleLoader::load(pilot_dll_path(), handle);
    ASSERT_EQ(status, loader::status_ok)
        << "Load failed: status=" << status
        << " (" << loader::status_name(status) << ")";
    EXPECT_TRUE(handle.valid());
    EXPECT_NE(handle.anatomy(), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Handle->composition_name match Pilot-Composition (WormholeComposition)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, LoadedAnatomyIsWormholeComposition) {
    loader::AnatomyModuleHandle handle;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), handle),
              loader::status_ok);

    auto* anat = handle.anatomy();
    ASSERT_NE(anat, nullptr);
    EXPECT_EQ(anat->composition_name(), std::string_view{"WormholeComposition"});
    EXPECT_EQ(anat->genus(), ana::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(anat->organ_count(), 19u);
    EXPECT_EQ(anat->engine_kind(), ee::ExecutionEngineKind::Anatomy);
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Lifecycle-Roundtrip via geladenem Handle
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, LifecycleRoundtripViaLoadedHandle) {
    loader::AnatomyModuleHandle handle;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), handle),
              loader::status_ok);

    auto* anat = handle.anatomy();
    ASSERT_NE(anat, nullptr);

    EXPECT_EQ(anat->lifecycle_state(), ee::EngineLifecycleState::Uninitialized);
    anat->warm_up();
    EXPECT_EQ(anat->lifecycle_state(), ee::EngineLifecycleState::Warming);
    anat->run();
    EXPECT_EQ(anat->lifecycle_state(), ee::EngineLifecycleState::Running);
    anat->reset();
    EXPECT_EQ(anat->lifecycle_state(), ee::EngineLifecycleState::Idle);
    anat->shutdown();
    EXPECT_EQ(anat->lifecycle_state(), ee::EngineLifecycleState::Shutdown);
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — ABI-Version match Host-Version
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, ModuleVersionMatchesHost) {
    loader::AnatomyModuleHandle handle;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), handle),
              loader::status_ok);

    auto const v = handle.module_version();
    EXPECT_EQ(v.major, abi::kHostAnatomyAbiVersion.major);
    EXPECT_LE(v.minor, abi::kHostAnatomyAbiVersion.minor);
    EXPECT_TRUE(abi::kHostAnatomyAbiVersion.host_compatible_with(v));
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — Move-Semantik (move-construct + move-assign)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, MoveConstructTransfersOwnership) {
    loader::AnatomyModuleHandle src;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), src),
              loader::status_ok);

    loader::AnatomyModuleHandle dst{std::move(src)};
    EXPECT_TRUE(dst.valid());
    EXPECT_FALSE(src.valid());  // source ist nach move leer
    EXPECT_NE(dst.anatomy(), nullptr);
}

TEST(R5E_AnatomyLoader, MoveAssignReleasesPreviousAndTakesNew) {
    loader::AnatomyModuleHandle h1;
    loader::AnatomyModuleHandle h2;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), h1),
              loader::status_ok);
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), h2),
              loader::status_ok);

    // h1 ist gueltig, ueberschreibe mit h2 (move)
    h1 = std::move(h2);
    EXPECT_TRUE(h1.valid());
    EXPECT_FALSE(h2.valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// §9 — Multiple-Load gleicher DLL liefert 2 unabhaengige Anatomy-Instanzen
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, MultipleLoadsProduceDistinctInstances) {
    loader::AnatomyModuleHandle h1;
    loader::AnatomyModuleHandle h2;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), h1),
              loader::status_ok);
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), h2),
              loader::status_ok);

    EXPECT_NE(h1.anatomy(), h2.anatomy());  // distinkte Heap-Allokationen

    // Lifecycle pro Instanz unabhaengig
    h1.anatomy()->warm_up();
    EXPECT_EQ(h1.anatomy()->lifecycle_state(), ee::EngineLifecycleState::Warming);
    EXPECT_EQ(h2.anatomy()->lifecycle_state(), ee::EngineLifecycleState::Uninitialized);
}

// ─────────────────────────────────────────────────────────────────────────────
// §10 — explizites unload() macht Handle wieder leer
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5E_AnatomyLoader, ExplicitUnloadInvalidatesHandle) {
    loader::AnatomyModuleHandle handle;
    ASSERT_EQ(loader::AnatomyModuleLoader::load(pilot_dll_path(), handle),
              loader::status_ok);
    EXPECT_TRUE(handle.valid());

    handle.unload();
    EXPECT_FALSE(handle.valid());
    EXPECT_EQ(handle.anatomy(), nullptr);

    // Zweites unload() ist no-op (defensive)
    handle.unload();
    EXPECT_FALSE(handle.valid());
}
