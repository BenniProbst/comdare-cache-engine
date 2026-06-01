// V41.F.6.1.R7.5.i.2 axis_09_isa Tests (Haupt-CPU-ISA, nach User-Korrektur 2026-05-27)
//
// User-Direktive: axis_09 = Haupt-CPU-Familie (Amd64/Aarch64/RiscV/PowerPc).
// SIMD-Extensions in separater Sub-Achse axis_09b mit Compat-Constraint.
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.i.2 (Refactor nach User-Direktive)

#include <gtest/gtest.h>

#include <topics/hardware/axis_09_isa/axis_09_isa_amd64.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_aarch64.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_riscv.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_powerpc.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_subaxes_is1_to_is3.hpp>
#include <axes/simd/axis_09_isa_flags.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax09 = ::comdare::cache_engine::hardware::axis_09_isa;
namespace hw   = ::comdare::cache_engine::hardware;
namespace mp   = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern ───
template <class... Is>
using ToGTestTypes = ::testing::Types<Is...>;

using AllIsaTypes = mp::mp_apply<ToGTestTypes, ax09::AllIsas>;

template <class T>
class IsaWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(IsaWrapperTest, AllIsaTypes);

TYPED_TEST(IsaWrapperTest, ConceptConformance) {
    static_assert(ax09::concepts::IsaStrategy<TypeParam>);
    static_assert(ax09::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(IsaWrapperTest, Is64bit) {
    // Alle modernen Haupt-ISAs sind 64-bit
    static_assert(TypeParam::is_64bit() == true);
    SUCCEED();
}

TYPED_TEST(IsaWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::cpu_family().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(IsaWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax09::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests

// ─── Spezifische Verhaltens-Tests ───

TEST(R7_5_i2_Axis_09_Specific, CpuFamilyIdentifiers) {
    static_assert(ax09::Amd64Isa::cpu_family()    == std::string_view{"x86_64"});
    static_assert(ax09::Aarch64Isa::cpu_family()  == std::string_view{"aarch64"});
    static_assert(ax09::RiscVIsa::cpu_family()    == std::string_view{"riscv64"});
    static_assert(ax09::PowerPcIsa::cpu_family()  == std::string_view{"ppc64le"});
    SUCCEED();
}

TEST(R7_5_i2_Axis_09_Specific, NativeSimdAvailability) {
    // Amd64/Aarch64/PowerPC haben SIMD als Pflicht-ABI-Baseline
    // RISC-V Vector ist optional (nicht baseline)
    static_assert(ax09::Amd64Isa::supports_native_simd()   == true);
    static_assert(ax09::Aarch64Isa::supports_native_simd() == true);
    static_assert(ax09::PowerPcIsa::supports_native_simd() == true);
    static_assert(ax09::RiscVIsa::supports_native_simd()   == false);
    SUCCEED();
}

TEST(R7_5_i2_Axis_09_Specific, FlagSuffixUppercase) {
    static_assert(ax09::Amd64Isa::flag_suffix()   == std::string_view{"AMD64"});
    static_assert(ax09::Aarch64Isa::flag_suffix() == std::string_view{"AARCH64"});
    static_assert(ax09::RiscVIsa::flag_suffix()   == std::string_view{"RISCV"});
    static_assert(ax09::PowerPcIsa::flag_suffix() == std::string_view{"POWERPC"});
    SUCCEED();
}

TEST(R7_5_i2_Axis_09_Specific, RegistryHas4MainIsas) {
    static_assert(mp::mp_size<ax09::AllIsas>::value == 4);
    static_assert(mp::mp_size<ax09::EnabledIsas>::value > 0);
    SUCCEED();
}

TEST(R7_5_i2_Hardware, TopicConfigSetExposesAxis09Mit4Isas) {
    static_assert(mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value == 4);
    SUCCEED();
}
