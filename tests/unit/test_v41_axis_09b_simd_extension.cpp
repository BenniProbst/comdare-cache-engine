// V41.F.6.1.R7.5.j axis_09b_simd_extension Tests (SIMD/Accelerator Sub-Achse)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.j (NEU: Sub-Achse fuer SIMD-Extensions)

#include <gtest/gtest.h>

#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_no_extension.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_sse2.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx2.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx512.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_neon.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_sve2.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_rvv.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_cuda_gh200.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_registry.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_subaxes_se1_to_se3.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_flags.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax09b = ::comdare::cache_engine::hardware::axis_09b_simd_extension;
namespace hw    = ::comdare::cache_engine::hardware;
namespace mp    = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern ───
template <class... Es>
using ToGTestTypes = ::testing::Types<Es...>;

using AllExtensionTypes = mp::mp_apply<ToGTestTypes, ax09b::AllExtensions>;

template <class T>
class SimdExtensionWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(SimdExtensionWrapperTest, AllExtensionTypes);

TYPED_TEST(SimdExtensionWrapperTest, ConceptConformance) {
    static_assert(ax09b::concepts::SimdExtensionStrategy<TypeParam>);
    static_assert(ax09b::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(SimdExtensionWrapperTest, AtLeastOneCompat) {
    // Jede Extension muss zu mindestens einer Haupt-ISA kompatibel sein
    constexpr bool any_compat = TypeParam::compatible_with_x86()
                              || TypeParam::compatible_with_arm()
                              || TypeParam::compatible_with_riscv()
                              || TypeParam::compatible_with_powerpc();
    static_assert(any_compat);
    SUCCEED();
}

TYPED_TEST(SimdExtensionWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(SimdExtensionWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax09b::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 8 Extensions × 4 TYPED = 32 typed Tests

// ─── Spezifische Compat-Tests (User-Direktive Cross-ISA-Constraint) ───

TEST(R7_5_j_Axis_09b_Specific, NoExtensionAllCompat) {
    // NoExtension ist universell kompatibel (Baseline)
    static_assert(ax09b::NoExtension::is_active()             == false);
    static_assert(ax09b::NoExtension::compatible_with_x86()   == true);
    static_assert(ax09b::NoExtension::compatible_with_arm()   == true);
    static_assert(ax09b::NoExtension::compatible_with_riscv() == true);
    static_assert(ax09b::NoExtension::compatible_with_powerpc() == true);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, X86OnlyExtensions) {
    // SSE2/AVX2/AVX-512 sind x86-only
    static_assert(ax09b::Sse2Extension::compatible_with_x86()   == true);
    static_assert(ax09b::Sse2Extension::compatible_with_arm()   == false);
    static_assert(ax09b::Avx2Extension::compatible_with_x86()   == true);
    static_assert(ax09b::Avx2Extension::compatible_with_arm()   == false);
    static_assert(ax09b::Avx512Extension::compatible_with_x86() == true);
    static_assert(ax09b::Avx512Extension::compatible_with_arm() == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, ArmOnlyExtensions) {
    // NEON/SVE2 sind ARM-only
    static_assert(ax09b::NeonExtension::compatible_with_arm() == true);
    static_assert(ax09b::NeonExtension::compatible_with_x86() == false);
    static_assert(ax09b::Sve2Extension::compatible_with_arm() == true);
    static_assert(ax09b::Sve2Extension::compatible_with_x86() == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, RiscVOnlyExtensions) {
    // RVV ist RISC-V-only
    static_assert(ax09b::RvvExtension::compatible_with_riscv() == true);
    static_assert(ax09b::RvvExtension::compatible_with_x86()   == false);
    static_assert(ax09b::RvvExtension::compatible_with_arm()   == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, CudaGh200MultiCompat) {
    // CUDA-GH200 ist mit x86 (Hopper-only Host) + ARM (Grace+Hopper) + PowerPC (AC922) kompatibel
    static_assert(ax09b::CudaGh200Extension::compatible_with_x86()     == true);
    static_assert(ax09b::CudaGh200Extension::compatible_with_arm()     == true);
    static_assert(ax09b::CudaGh200Extension::compatible_with_powerpc() == true);
    static_assert(ax09b::CudaGh200Extension::compatible_with_riscv()   == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, VectorWidthCategorized) {
    // Vector-Width: 0 (None), 128 (Sse2/Neon), 256 (Avx2), 512 (Avx512), -1 (scalable Sve2/Rvv/GPU)
    static_assert(ax09b::NoExtension::vector_width_bits()       == 0);
    static_assert(ax09b::Sse2Extension::vector_width_bits()     == 128);
    static_assert(ax09b::Avx2Extension::vector_width_bits()     == 256);
    static_assert(ax09b::Avx512Extension::vector_width_bits()   == 512);
    static_assert(ax09b::NeonExtension::vector_width_bits()     == 128);
    static_assert(ax09b::Sve2Extension::vector_width_bits()     == -1);  // scalable
    static_assert(ax09b::RvvExtension::vector_width_bits()      == -1);  // scalable
    static_assert(ax09b::CudaGh200Extension::vector_width_bits() == -1); // massive parallel
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, RegistryHas8Extensions) {
    static_assert(mp::mp_size<ax09b::AllExtensions>::value == 8);
    static_assert(mp::mp_size<ax09b::EnabledExtensions>::value > 0);
    SUCCEED();
}

TEST(R7_5_j_Hardware, TopicConfigSetExposesAxis09bWith8Extensions) {
    static_assert(mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09b>::value == 8);
    // Cartesian-Product axis_09 x axis_09b x axis_12 = 4 x 8 x 3 = 96 unfiltered
    // (Compat-Filter ist Aufgabe der PermutationEngine via cross-constraints)
    constexpr auto isa_count  = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value;
    constexpr auto ext_count  = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09b>::value;
    constexpr auto plat_count = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_12>::value;
    constexpr auto prod_count = mp::mp_size<hw::TopicConfigSet::CartesianIsa09xExt09bxPlatform12>::value;
    static_assert(prod_count == isa_count * ext_count * plat_count);
    SUCCEED();
}
