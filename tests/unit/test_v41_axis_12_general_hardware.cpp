// V41.F.6.1.R7.1 axis_12 General-Hardware Tests
//
// Beweist:
// 1. Concept-Conformance fuer GenericHardware/X86_64Hardware/Aarch64Hardware
// 2. Pflicht-Properties (cache_line_size, memory_page_size, simd_width_bits,
//    numa_capable, huge_page_capable) sind constexpr und sinnvoll
// 3. HardwareTopicTag im topic_tag eingehalten
// 4. Unterscheidung Generic vs Plattform-Aware (SIMD 0 vs 128 vs 256)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §54.5 R7.1
// @task #716 V41.F.6.1.R7.1

#include <gtest/gtest.h>

#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_generic.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_aarch64.hpp>

#include <string_view>

namespace ax12 = ::comdare::cache_engine::hardware::axis_12_general_hardware;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Concept-Conformance
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, GenericConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::GenericHardware>);
    SUCCEED();
}

TEST(R7_1_Axis12, X86_64ConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::X86_64Hardware>);
    SUCCEED();
}

TEST(R7_1_Axis12, Aarch64ConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::Aarch64Hardware>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Pflicht-Properties: GenericHardware (conservative defaults)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, GenericHasConservativeDefaults) {
    static_assert(ax12::GenericHardware::cache_line_size()   == 64);
    static_assert(ax12::GenericHardware::memory_page_size()  == 4096);
    static_assert(ax12::GenericHardware::simd_width_bits()   == 0);  // Scalar-Baseline
    static_assert(ax12::GenericHardware::numa_capable()      == false);
    static_assert(ax12::GenericHardware::huge_page_capable() == false);
    static_assert(ax12::GenericHardware::name() == std::string_view{"general_hardware_generic"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — X86_64Hardware (AVX2 + NUMA + huge-pages)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, X86_64HasAvx2DefaultsAndNumaAware) {
    static_assert(ax12::X86_64Hardware::cache_line_size()   == 64);
    static_assert(ax12::X86_64Hardware::memory_page_size()  == 4096);
    static_assert(ax12::X86_64Hardware::simd_width_bits()   == 256);  // AVX2 Conservative
    static_assert(ax12::X86_64Hardware::numa_capable()      == true);
    static_assert(ax12::X86_64Hardware::huge_page_capable() == true);
    static_assert(ax12::X86_64Hardware::name() == std::string_view{"general_hardware_x86_64"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Aarch64Hardware (NEON 128-bit)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, Aarch64HasNeonDefaults) {
    static_assert(ax12::Aarch64Hardware::cache_line_size()   == 64);
    static_assert(ax12::Aarch64Hardware::memory_page_size()  == 4096);
    static_assert(ax12::Aarch64Hardware::simd_width_bits()   == 128);  // NEON
    static_assert(ax12::Aarch64Hardware::numa_capable()      == true);
    static_assert(ax12::Aarch64Hardware::huge_page_capable() == true);
    static_assert(ax12::Aarch64Hardware::name() == std::string_view{"general_hardware_aarch64"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Plattform-Differenzierung: SIMD-Width distinguishes the 3 Wrappers
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, ThreeWrappersDistinguishedBySimdWidth) {
    static_assert(ax12::GenericHardware::simd_width_bits() == 0);
    static_assert(ax12::Aarch64Hardware::simd_width_bits() == 128);
    static_assert(ax12::X86_64Hardware::simd_width_bits()  == 256);
    // Familien-IDs sind ebenfalls distinkt (0, 1, 2)
    static_assert(ax12::GenericHardware::family_id::value == 0);
    static_assert(ax12::X86_64Hardware::family_id::value  == 1);
    static_assert(ax12::Aarch64Hardware::family_id::value == 2);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Instanziierbar (Default-Konstruktor + CRTP-Base)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, AllWrappersInstantiable) {
    [[maybe_unused]] ax12::GenericHardware g;
    [[maybe_unused]] ax12::X86_64Hardware  x;
    [[maybe_unused]] ax12::Aarch64Hardware a;
    SUCCEED();
}
