// V41.F.6.1.R7.1.a + R7.1.a.2 axis_12 General-Hardware Tests
//
// R7.1.a Beweist:
// 1. Concept-Conformance fuer GenericHardwareProfile/X86_64HardwareProfile/Aarch64HardwareProfile
// 2. Pflicht-Properties (cache_line_size, memory_page_size, simd_width_bits,
//    numa_capable, huge_page_capable) sind constexpr und sinnvoll
// 3. HardwareTopicTag im topic_tag eingehalten
// 4. Unterscheidung Generic vs Plattform-Aware (SIMD 0 vs 128 vs 256)
//
// R7.1.a.2 (Goldstandard-Nachruestung) Beweist:
// 5. CacheEnginePermutationStrategy Concept-Conformance (axis_tag/family_id/
//    name/family_name/flag_suffix/enabled)
// 6. Subaxes-Tags (cpu_family/simd_capability/memory_topology/page_topology)
// 7. Registry-Filter: AllPlatforms = 3, EnabledPlatforms via mp_filter
// 8. Flags-Header constexpr-Werte (generic/x86_64/aarch64 _enabled)
// 9. TopicConfigSet hardware (axis_09 + axis_12 + CartesianIsa09xPlatform12)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §54.5 R7.1
// @memory [[axis-gold-standard-checklist]]
// @task #716 V41.F.6.1.R7.1.a + R7.1.a.2

#include <gtest/gtest.h>

#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_generic.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_aarch64.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_registry.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_subaxes_hw1_to_hw4.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_flags.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>

#include <boost/mp11.hpp>

#include <string_view>
#include <type_traits>

namespace ax12 = ::comdare::cache_engine::hardware::axis_12_general_hardware;
namespace hw   = ::comdare::cache_engine::hardware;
namespace mp   = ::boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Concept-Conformance
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, GenericConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::GenericHardwareProfile>);
    SUCCEED();
}

TEST(R7_1_Axis12, X86_64ConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::X86_64HardwareProfile>);
    SUCCEED();
}

TEST(R7_1_Axis12, Aarch64ConformsToConcept) {
    static_assert(ax12::concepts::GeneralHardwareStrategy<ax12::Aarch64HardwareProfile>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Pflicht-Properties: GenericHardwareProfile (conservative defaults)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, GenericHasConservativeDefaults) {
    static_assert(ax12::GenericHardwareProfile::cache_line_size() == 64);
    static_assert(ax12::GenericHardwareProfile::memory_page_size() == 4096);
    static_assert(ax12::GenericHardwareProfile::simd_width_bits() == 0); // Scalar-Baseline
    static_assert(ax12::GenericHardwareProfile::numa_capable() == false);
    static_assert(ax12::GenericHardwareProfile::huge_page_capable() == false);
    static_assert(ax12::GenericHardwareProfile::name() == std::string_view{"general_hardware_generic"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — X86_64HardwareProfile (AVX2 + NUMA + huge-pages)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, X86_64HasAvx2DefaultsAndNumaAware) {
    static_assert(ax12::X86_64HardwareProfile::cache_line_size() == 64);
    static_assert(ax12::X86_64HardwareProfile::memory_page_size() == 4096);
    static_assert(ax12::X86_64HardwareProfile::simd_width_bits() == 256); // AVX2 Conservative
    static_assert(ax12::X86_64HardwareProfile::numa_capable() == true);
    static_assert(ax12::X86_64HardwareProfile::huge_page_capable() == true);
    static_assert(ax12::X86_64HardwareProfile::name() == std::string_view{"general_hardware_x86_64"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Aarch64HardwareProfile (NEON 128-bit)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, Aarch64HasNeonDefaults) {
    static_assert(ax12::Aarch64HardwareProfile::cache_line_size() == 64);
    static_assert(ax12::Aarch64HardwareProfile::memory_page_size() == 4096);
    static_assert(ax12::Aarch64HardwareProfile::simd_width_bits() == 128); // NEON
    static_assert(ax12::Aarch64HardwareProfile::numa_capable() == true);
    static_assert(ax12::Aarch64HardwareProfile::huge_page_capable() == true);
    static_assert(ax12::Aarch64HardwareProfile::name() == std::string_view{"general_hardware_aarch64"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Plattform-Differenzierung: SIMD-Width distinguishes the 3 Wrappers
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, ThreeWrappersDistinguishedBySimdWidth) {
    static_assert(ax12::GenericHardwareProfile::simd_width_bits() == 0);
    static_assert(ax12::Aarch64HardwareProfile::simd_width_bits() == 128);
    static_assert(ax12::X86_64HardwareProfile::simd_width_bits() == 256);
    // Familien-IDs sind ebenfalls distinkt (0, 1, 2)
    static_assert(ax12::GenericHardwareProfile::family_id::value == 0);
    static_assert(ax12::X86_64HardwareProfile::family_id::value == 1);
    static_assert(ax12::Aarch64HardwareProfile::family_id::value == 2);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Instanziierbar (Default-Konstruktor + CRTP-Base)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_Axis12, AllWrappersInstantiable) {
    [[maybe_unused]] ax12::GenericHardwareProfile g;
    [[maybe_unused]] ax12::X86_64HardwareProfile  x;
    [[maybe_unused]] ax12::Aarch64HardwareProfile a;
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — R7.1.a.2 Goldstandard: CacheEnginePermutationStrategy Concept
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_a_2_Axis12, AllWrappersSatisfyCacheEnginePermutationConcept) {
    static_assert(ax12::concepts::CacheEnginePermutationStrategy<ax12::GenericHardwareProfile>);
    static_assert(ax12::concepts::CacheEnginePermutationStrategy<ax12::X86_64HardwareProfile>);
    static_assert(ax12::concepts::CacheEnginePermutationStrategy<ax12::Aarch64HardwareProfile>);
    SUCCEED();
}

TEST(R7_1_a_2_Axis12, AllWrappersHaveSubaxisTagCpuFamily) {
    static_assert(std::is_same_v<ax12::GenericHardwareProfile::axis_tag, ax12::subaxes::cpu_family_tag>);
    static_assert(std::is_same_v<ax12::X86_64HardwareProfile::axis_tag, ax12::subaxes::cpu_family_tag>);
    static_assert(std::is_same_v<ax12::Aarch64HardwareProfile::axis_tag, ax12::subaxes::cpu_family_tag>);
    SUCCEED();
}

TEST(R7_1_a_2_Axis12, AllWrappersHaveFlagSuffix) {
    static_assert(ax12::GenericHardwareProfile::flag_suffix() == std::string_view{"GENERIC"});
    static_assert(ax12::X86_64HardwareProfile::flag_suffix() == std::string_view{"X86_64"});
    static_assert(ax12::Aarch64HardwareProfile::flag_suffix() == std::string_view{"AARCH64"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — R7.1.a.2 Goldstandard: Registry (AllPlatforms + EnabledPlatforms)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_a_2_Axis12, AllPlatformsContainsThreeWrappers) {
    static_assert(mp::mp_size<ax12::AllPlatforms>::value == 3);
    SUCCEED();
}

TEST(R7_1_a_2_Axis12, EnabledPlatformsIsNonEmpty) {
    // Default: alle 3 ENABLE-Optionen ON -> alle in EnabledPlatforms
    static_assert(mp::mp_size<ax12::EnabledPlatforms>::value > 0);
    SUCCEED();
}

TEST(R7_1_a_2_Axis12, IsEnabledPredicateMatchesWrapperFlag) {
    static_assert(ax12::is_enabled<ax12::GenericHardwareProfile>::value == ax12::GenericHardwareProfile::enabled);
    static_assert(ax12::is_enabled<ax12::X86_64HardwareProfile>::value == ax12::X86_64HardwareProfile::enabled);
    static_assert(ax12::is_enabled<ax12::Aarch64HardwareProfile>::value == ax12::Aarch64HardwareProfile::enabled);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §9 — R7.1.a.2 Goldstandard: Flags-Header constexpr-Werte
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_a_2_Axis12, FlagsHeaderProvidesAllThreePlatforms) {
    static_assert(std::is_same_v<decltype(ax12::flags::generic_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax12::flags::x86_64_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax12::flags::aarch64_enabled), const bool>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §10 — R7.1.a.2 Goldstandard: TopicConfigSet (axis_09 + axis_12 + Cartesian)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_a_2_Hardware, TopicConfigSetExposesBothAxes) {
    // axis_09 (R7.5.i: jetzt 4 ISAs Scalar/Sse2/Avx2/Neon via eigene Registry)
    static_assert(mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value == 4);
    // axis_12 (3 Wrappers per default ENABLE)
    static_assert(mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_12>::value > 0);
    // Default-StaticAxisVariants = axis_12 (Plattform-Konfiguration ist Haupt-Achse)
    static_assert(std::is_same_v<hw::TopicConfigSet::StaticAxisVariants, hw::TopicConfigSet::StaticAxisVariants_12>);
    SUCCEED();
}

TEST(R7_1_a_2_Hardware, TopicConfigSetCartesianIsa09xPlatform12IsProductOfBoth) {
    constexpr auto isa_count  = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value;
    constexpr auto plat_count = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_12>::value;
    constexpr auto prod_count = mp::mp_size<hw::TopicConfigSet::CartesianIsa09xPlatform12>::value;
    static_assert(prod_count == isa_count * plat_count);
    SUCCEED();
}
