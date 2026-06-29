// V41.F.6.1.R5.D.2 — anatomy_codegen Smoke-Test
//
// Beweist:
// 1. CMake-Function comdare_codegen_anatomy_module() generiert kompilierbare .cpp
// 2. STATIC-Lib hot_codegen_pilot exportiert die 4 Pflicht-extern-"C" Symbole
// 3. comdare_create_anatomy() liefert HotComposition-Anatomie (NICHT Art)
// 4. Lifecycle-Roundtrip funktioniert via Factory-Pointer
// 5. Kollision-frei zu test_v41_anatomy_module_abi (das ArtComposition nutzt)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §45
// @task #707 V41.F.6.1.R5.D.2

#include <gtest/gtest.h>

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <anatomy/anatomy_base.hpp>

#include <cstdint>
#include <string_view>

// NOTE: Wir #include die ABI-Header OHNE COMDARE_ANATOMY_MODULE_BUILD zu setzen
// — die STATIC-Pilot-Lib setzt COMDARE_ANATOMY_ABI_STATIC=1 als PUBLIC
// compile-definition (siehe cmake/anatomy_codegen.cmake), das macht die
// __declspec-Markierungen auf Windows zu no-ops.

namespace ana = ::comdare::cache_engine::anatomy;
namespace abi = ::comdare::cache_engine::abi;
namespace ee  = ::comdare::cache_engine::execution_engine;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Codegen-Pilot exportiert die 4 ABI-Symbole
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D2_Codegen, AbiVersionFunctionExported) {
    constexpr std::uint64_t expected = (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |
                                       static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);
    EXPECT_EQ(comdare_anatomy_abi_version(), expected);
}

TEST(R5D2_Codegen, AbiMagicFunctionExported) { EXPECT_EQ(comdare_anatomy_abi_magic(), COMDARE_ANATOMY_ABI_MAGIC); }

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Factory liefert HotComposition (vs ArtComposition in test_v41_anatomy_module_abi)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D2_Codegen, CreateAnatomyReturnsHotComposition) {
    ana::IAnatomyBase* ptr = comdare_create_anatomy();
    ASSERT_NE(ptr, nullptr);
    // Codegen-Pilot wurde mit HotComposition-Type aufgerufen
    EXPECT_EQ(ptr->composition_name(), std::string_view{"HotComposition"});
    EXPECT_EQ(ptr->genus(), ana::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(ptr->organ_count(), 19u);
    EXPECT_EQ(ptr->engine_kind(), ee::ExecutionEngineKind::Anatomy);
    comdare_destroy_anatomy(ptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Lifecycle-Roundtrip via codegen-erzeugte Factory
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D2_Codegen, LifecycleRoundtripViaCodegenFactory) {
    ana::IAnatomyBase* ptr = comdare_create_anatomy();
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(ptr->lifecycle_state(), ee::EngineLifecycleState::Uninitialized);
    ptr->warm_up();
    EXPECT_EQ(ptr->lifecycle_state(), ee::EngineLifecycleState::Warming);
    ptr->run();
    EXPECT_EQ(ptr->lifecycle_state(), ee::EngineLifecycleState::Running);
    ptr->reset();
    EXPECT_EQ(ptr->lifecycle_state(), ee::EngineLifecycleState::Idle);
    ptr->shutdown();
    EXPECT_EQ(ptr->lifecycle_state(), ee::EngineLifecycleState::Shutdown);

    comdare_destroy_anatomy(ptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Compat-Check via AnatomyAbiVersion-Helper
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D2_Codegen, ModuleVersionIsCompatibleWithHost) {
    auto const module_version = abi::AnatomyAbiVersion::unpack(comdare_anatomy_abi_version());
    EXPECT_TRUE(abi::kHostAnatomyAbiVersion.host_compatible_with(module_version));
}
