// V41.F.6.1.R5.D — Anatomy Module ABI v1 Tests
//
// Beweist:
// 1. ABI-Version + Magic-Number sind als constexpr definiert
// 2. AnatomyAbiVersion pack/unpack Roundtrip
// 3. host_compatible_with() Pflicht-Check (Major-Match + Minor-LE)
// 4. COMDARE_DEFINE_ANATOMY_MODULE Macro expandiert zu 4 extern "C" Symbolen
// 5. Factory comdare_create_anatomy() liefert non-null IAnatomyBase*
// 6. Geladene Anatomie hat korrekte composition_name/genus/organ_count
// 7. Lifecycle-Hooks via Factory-Pointer (warm_up/run/reset/shutdown)
// 8. comdare_destroy_anatomy(ptr) gibt Resource frei (kein Leak)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §41.5 + Teil 9
// @task #706 V41.F.6.1.R5.D

// COMDARE_ANATOMY_MODULE_BUILD muss VOR dem Include definiert sein damit
// auf Windows __declspec(dllexport) statt dllimport verwendet wird.
// In einem normalen Permutations-Binary setzt die CMake-Target-Definition das.
#define COMDARE_ANATOMY_MODULE_BUILD 1

#include <gtest/gtest.h>

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <anatomy/abi_adapter.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/known_algorithms.hpp>

#include <compositions/art_reference.hpp>

#include <cstdint>
#include <type_traits>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace ce_abi = ::comdare::cache_engine::abi; // ce_abi avoids libstdc++ <cxxabi.h> global abi alias via gtest
namespace ee     = ::comdare::cache_engine::execution_engine;

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_DEFINE_ANATOMY_MODULE expandieren — Pilot mit ArtComposition
// (Das simuliert ein generiertes Permutations-Binary in-process.)
// ─────────────────────────────────────────────────────────────────────────────

// cppcheck kennt die COMDARE-Codegen-Emitter-Makros nicht (Definition via Include-Kette, kein -I im Lint-Lauf).
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_ANATOMY_MODULE(::comdare::cache_engine::compositions::ArtComposition)

// ─────────────────────────────────────────────────────────────────────────────
// §1 — ABI-Version + Magic-Number Compile-Time Konstanten
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyAbi, MacroDefinesVersionAndMagic) {
    static_assert(COMDARE_ANATOMY_ABI_MAJOR ==
                  5); // Bau-INC-2b (TABU-GO): der EINE koordinierte 4→5-Bündel-Bump (vorher #216-H2 Major 4)
    static_assert(COMDARE_ANATOMY_ABI_MINOR == 0); // Minor auf 0 zurückgesetzt beim Major-Bump
    static_assert(COMDARE_ANATOMY_ABI_MAGIC ==
                  0x434F4D444141352EULL); // "COMDA·A5·" (Magic kodiert Major, Minor-Bump ändert es nicht)
    SUCCEED();
}

TEST(R5D_AnatomyAbi, HostAbiVersionMatchesMacro) {
    static_assert(ce_abi::kHostAnatomyAbiVersion.major == 5); // Bau-INC-2b (vorher #216-H2: 4)
    static_assert(ce_abi::kHostAnatomyAbiVersion.minor == 0);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — AnatomyAbiVersion pack/unpack Roundtrip
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyAbiVersion, PackUnpackRoundtrip) {
    constexpr ce_abi::AnatomyAbiVersion v{2, 7};
    constexpr auto                      packed   = v.pack();
    constexpr auto                      unpacked = ce_abi::AnatomyAbiVersion::unpack(packed);
    static_assert(unpacked.major == 2);
    static_assert(unpacked.minor == 7);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — host_compatible_with: Major-Match + Module-Minor <= Host-Minor
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyAbiVersion, CompatibilityRules) {
    constexpr ce_abi::AnatomyAbiVersion host_1_5{1, 5};

    // Modul gleiche Version: OK
    static_assert(host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{1, 5}));
    // Modul aelter (minor lower): OK
    static_assert(host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{1, 3}));
    // Modul aelter (minor=0): OK
    static_assert(host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{1, 0}));
    // Modul neuer (minor higher): NICHT OK (Host kennt Modul-Features nicht)
    static_assert(!host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{1, 6}));
    // Major-Mismatch: NICHT OK
    static_assert(!host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{2, 0}));
    static_assert(!host_1_5.host_compatible_with(ce_abi::AnatomyAbiVersion{0, 5}));

    // Bau-INC-2b (Loader-Ablehnungs-Checkpoint): der REALE Host (ABI-5) verwirft alt-gebaute
    // Major-4-Module — alle Permutations-DLLs der ABI-4-Aera werden nicht mehr geladen.
    static_assert(!ce_abi::kHostAnatomyAbiVersion.host_compatible_with(ce_abi::AnatomyAbiVersion{4, 0}));
    static_assert(ce_abi::kHostAnatomyAbiVersion.host_compatible_with(
        ce_abi::AnatomyAbiVersion{COMDARE_ANATOMY_ABI_MAJOR, COMDARE_ANATOMY_ABI_MINOR}));
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — extern "C" Factory-Symbole sind verfuegbar (Macro-Expansion)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyFactory, AbiVersionFunctionReturnsExpected) {
    constexpr std::uint64_t expected = (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |
                                       static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);
    EXPECT_EQ(comdare_anatomy_abi_version(), expected);
}

TEST(R5D_AnatomyFactory, AbiMagicFunctionReturnsExpected) {
    EXPECT_EQ(comdare_anatomy_abi_magic(), COMDARE_ANATOMY_ABI_MAGIC);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Factory liefert non-null IAnatomyBase* mit korrekten Properties
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyFactory, CreateAnatomyReturnsValidPointer) {
    ana::IAnatomyBase* ptr = comdare_create_anatomy();
    ASSERT_NE(ptr, nullptr);
    // ArtComposition wurde im Macro hinterlegt
    EXPECT_EQ(ptr->composition_name(), std::string_view{"ArtComposition"});
    EXPECT_EQ(ptr->genus(), ana::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(ptr->organ_count(), 19u);
    EXPECT_EQ(ptr->engine_kind(), ee::ExecutionEngineKind::Anatomy);
    comdare_destroy_anatomy(ptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Lifecycle-Roundtrip via Factory-Pointer (R5.C.A4 run() ist verfuegbar)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyFactory, LifecycleRoundtripViaFactoryPointer) {
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
// §7 — Multi-Create / Multi-Destroy (Pflicht: keine Leaks, neu pro Aufruf)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyFactory, MultipleCreateProducesDistinctInstances) {
    ana::IAnatomyBase* p1 = comdare_create_anatomy();
    ana::IAnatomyBase* p2 = comdare_create_anatomy();
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_NE(p1, p2); // Heap-Allocation pro Aufruf

    // Lifecycle ist pro-Instanz unabhaengig
    p1->warm_up();
    EXPECT_EQ(p1->lifecycle_state(), ee::EngineLifecycleState::Warming);
    EXPECT_EQ(p2->lifecycle_state(), ee::EngineLifecycleState::Uninitialized);

    comdare_destroy_anatomy(p1);
    comdare_destroy_anatomy(p2);
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — DestroyAnatomy mit nullptr ist no-op (defensive)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5D_AnatomyFactory, DestroyNullptrIsNoOp) {
    // delete nullptr ist gemaess C++23 [expr.delete]/2 wohldefiniert (no-op).
    comdare_destroy_anatomy(nullptr);
    SUCCEED();
}
