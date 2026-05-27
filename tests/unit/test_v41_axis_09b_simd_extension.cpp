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

TEST(R7_5_j_Axis_09b_Specific, NoSimdExtensionAllCompat) {
    // NoSimdExtension ist universell kompatibel (Baseline)
    static_assert(ax09b::NoSimdExtension::is_active()             == false);
    static_assert(ax09b::NoSimdExtension::compatible_with_x86()   == true);
    static_assert(ax09b::NoSimdExtension::compatible_with_arm()   == true);
    static_assert(ax09b::NoSimdExtension::compatible_with_riscv() == true);
    static_assert(ax09b::NoSimdExtension::compatible_with_powerpc() == true);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, X86OnlyExtensions) {
    // SSE2/AVX2/AVX-512 sind x86-only
    static_assert(ax09b::Sse2SimdExtension::compatible_with_x86()   == true);
    static_assert(ax09b::Sse2SimdExtension::compatible_with_arm()   == false);
    static_assert(ax09b::Avx2SimdExtension::compatible_with_x86()   == true);
    static_assert(ax09b::Avx2SimdExtension::compatible_with_arm()   == false);
    static_assert(ax09b::Avx512SimdExtension::compatible_with_x86() == true);
    static_assert(ax09b::Avx512SimdExtension::compatible_with_arm() == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, ArmOnlyExtensions) {
    // NEON/SVE2 sind ARM-only
    static_assert(ax09b::NeonSimdExtension::compatible_with_arm() == true);
    static_assert(ax09b::NeonSimdExtension::compatible_with_x86() == false);
    static_assert(ax09b::Sve2SimdExtension::compatible_with_arm() == true);
    static_assert(ax09b::Sve2SimdExtension::compatible_with_x86() == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, RiscVOnlyExtensions) {
    // RVV ist RISC-V-only
    static_assert(ax09b::RvvSimdExtension::compatible_with_riscv() == true);
    static_assert(ax09b::RvvSimdExtension::compatible_with_x86()   == false);
    static_assert(ax09b::RvvSimdExtension::compatible_with_arm()   == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, CudaGh200MultiCompat) {
    // CUDA-GH200 ist mit x86 (Hopper-only Host) + ARM (Grace+Hopper) + PowerPC (AC922) kompatibel
    static_assert(ax09b::CudaGh200SimdExtension::compatible_with_x86()     == true);
    static_assert(ax09b::CudaGh200SimdExtension::compatible_with_arm()     == true);
    static_assert(ax09b::CudaGh200SimdExtension::compatible_with_powerpc() == true);
    static_assert(ax09b::CudaGh200SimdExtension::compatible_with_riscv()   == false);
    SUCCEED();
}

TEST(R7_5_j_Axis_09b_Specific, VectorWidthCategorized) {
    // Vector-Width: 0 (None), 128 (Sse2/Neon), 256 (Avx2), 512 (Avx512), -1 (scalable Sve2/Rvv/GPU)
    static_assert(ax09b::NoSimdExtension::vector_width_bits()       == 0);
    static_assert(ax09b::Sse2SimdExtension::vector_width_bits()     == 128);
    static_assert(ax09b::Avx2SimdExtension::vector_width_bits()     == 256);
    static_assert(ax09b::Avx512SimdExtension::vector_width_bits()   == 512);
    static_assert(ax09b::NeonSimdExtension::vector_width_bits()     == 128);
    static_assert(ax09b::Sve2SimdExtension::vector_width_bits()     == -1);  // scalable
    static_assert(ax09b::RvvSimdExtension::vector_width_bits()      == -1);  // scalable
    static_assert(ax09b::CudaGh200SimdExtension::vector_width_bits() == -1); // massive parallel
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

// ─── R7.7.b SSE/AVX/AVX-512 Schichten-Modell (rueckwaerts-kumulativ) ───
// Doku 15 §4: SSE → SSE2 → SSE3 → SSSE3 → SSE4.1 → SSE4.2 → AVX → AVX2 → AVX-512F
// Annahme: hoehere Schicht impliziert alle Vorgaenger-Schichten = true.

TEST(R7_7_b_Sse2_Layers, OnlySseAndSse2Provided) {
    static_assert(ax09b::Sse2SimdExtension::provides_sse());
    static_assert(ax09b::Sse2SimdExtension::provides_sse2());
    static_assert(!ax09b::Sse2SimdExtension::provides_sse3());
    static_assert(!ax09b::Sse2SimdExtension::provides_ssse3());
    static_assert(!ax09b::Sse2SimdExtension::provides_sse4_1());
    static_assert(!ax09b::Sse2SimdExtension::provides_sse4_2());
    static_assert(!ax09b::Sse2SimdExtension::provides_avx());
    static_assert(!ax09b::Sse2SimdExtension::provides_avx2());
    static_assert(!ax09b::Sse2SimdExtension::provides_avx512f());
    SUCCEED();
}

TEST(R7_7_b_Avx2_Layers, AllSseLayersPlusAvxAvx2Provided) {
    // Avx2 umfasst SSE/SSE2/SSE3/SSSE3/SSE4.1/SSE4.2/AVX/AVX2 (Haswell+ 2013)
    static_assert(ax09b::Avx2SimdExtension::provides_sse());
    static_assert(ax09b::Avx2SimdExtension::provides_sse2());
    static_assert(ax09b::Avx2SimdExtension::provides_sse3());
    static_assert(ax09b::Avx2SimdExtension::provides_ssse3());
    static_assert(ax09b::Avx2SimdExtension::provides_sse4_1());
    static_assert(ax09b::Avx2SimdExtension::provides_sse4_2());
    static_assert(ax09b::Avx2SimdExtension::provides_avx());
    static_assert(ax09b::Avx2SimdExtension::provides_avx2());
    static_assert(!ax09b::Avx2SimdExtension::provides_avx512f());  // Cut-Off
    SUCCEED();
}

TEST(R7_7_b_Avx512_Layers, FullSseAvxAvx512FoundationProvided) {
    // Avx512 umfasst alle SSE/AVX/AVX2 + AVX-512F (Skylake-X+/Zen 4+)
    static_assert(ax09b::Avx512SimdExtension::provides_sse());
    static_assert(ax09b::Avx512SimdExtension::provides_sse2());
    static_assert(ax09b::Avx512SimdExtension::provides_sse3());
    static_assert(ax09b::Avx512SimdExtension::provides_ssse3());
    static_assert(ax09b::Avx512SimdExtension::provides_sse4_1());
    static_assert(ax09b::Avx512SimdExtension::provides_sse4_2());
    static_assert(ax09b::Avx512SimdExtension::provides_avx());
    static_assert(ax09b::Avx512SimdExtension::provides_avx2());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512f());  // Pflicht-Basis
    SUCCEED();
}

TEST(R7_7_b_Avx512_SubFlags, ServerDefaultSubFlagsCorrect) {
    // Server-Default: alle relevanten Sub-Flags TRUE, KNL-Only Sub-Flags FALSE.
    static_assert(ax09b::Avx512SimdExtension::provides_avx512cd());
    static_assert(!ax09b::Avx512SimdExtension::provides_avx512er());  // Xeon Phi KNL only
    static_assert(!ax09b::Avx512SimdExtension::provides_avx512pf());  // Xeon Phi KNL only
    static_assert(ax09b::Avx512SimdExtension::provides_avx512bw());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512dq());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512vl());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512ifma());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512vbmi());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512vbmi2());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512vnni());      // DBMS Vector-Indexes
    static_assert(ax09b::Avx512SimdExtension::provides_avx512bitalg());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512vpopcntdq()); // Bitmap-Indexes
    static_assert(ax09b::Avx512SimdExtension::provides_avx512bf16());
    static_assert(ax09b::Avx512SimdExtension::provides_avx512fp16());
    SUCCEED();
}

TEST(R7_7_b_NonX86_Layers, NoSseAvxAvx512Provided) {
    // NEON/SVE2/RVV/CUDA haben KEINE SSE/AVX/AVX-512 (eigene ARM/RISC-V/GPU-ISA).
    static_assert(!ax09b::NeonSimdExtension::provides_sse2());
    static_assert(!ax09b::NeonSimdExtension::provides_avx2());
    static_assert(!ax09b::Sve2SimdExtension::provides_avx2());
    static_assert(!ax09b::Sve2SimdExtension::provides_avx512f());
    static_assert(!ax09b::RvvSimdExtension::provides_avx2());
    static_assert(!ax09b::CudaGh200SimdExtension::provides_avx2());
    static_assert(!ax09b::NoSimdExtension::provides_sse());
    static_assert(!ax09b::NoSimdExtension::provides_avx2());
    SUCCEED();
}

// ─── R7.7.c CPU-Sockel-Count + P/E-Cores Topologie (User-Direktive) ───
// Doku 15 §5: units_per_socket, shared_among_cores, accessible_from_efficiency_cores

TEST(R7_7_c_Topology_X86, Sse2Avx2DualUnitAllCores) {
    // SSE/AVX2: typisch 2 Units/Sockel, alle Cores haben Zugriff.
    static_assert(ax09b::Sse2SimdExtension::units_per_socket() == 2);
    static_assert(ax09b::Sse2SimdExtension::accessible_from_efficiency_cores());
    static_assert(ax09b::Avx2SimdExtension::units_per_socket() == 2);
    static_assert(ax09b::Avx2SimdExtension::accessible_from_efficiency_cores());
    SUCCEED();
}

TEST(R7_7_c_Topology_Avx512, SingleUnitPCoresOnly) {
    // AVX-512: 1 Unit/Sockel, Intel Alder/Raptor Lake P-Core-only (E-Cores disabled).
    static_assert(ax09b::Avx512SimdExtension::units_per_socket() == 1);
    static_assert(!ax09b::Avx512SimdExtension::accessible_from_efficiency_cores());
    SUCCEED();
}

TEST(R7_7_c_Topology_Arm_RiscV, AllCoresAccessible) {
    // ARM big.LITTLE / RISC-V: alle Cores haben NEON/SVE2/RVV.
    static_assert(ax09b::NeonSimdExtension::units_per_socket() == 1);
    static_assert(ax09b::NeonSimdExtension::accessible_from_efficiency_cores());
    static_assert(ax09b::Sve2SimdExtension::units_per_socket() == 1);
    static_assert(ax09b::Sve2SimdExtension::accessible_from_efficiency_cores());
    static_assert(ax09b::RvvSimdExtension::units_per_socket() == 1);
    SUCCEED();
}

TEST(R7_7_c_Topology_Gpu, MassiveParallelNotSocketBound) {
    // CUDA GPU: units_per_socket=-1 (massive parallel, GPU-Bus statt CPU-intern),
    // shared_among_cores=false (GPU ist separate Device).
    static_assert(ax09b::CudaGh200SimdExtension::units_per_socket() == -1);
    static_assert(!ax09b::CudaGh200SimdExtension::shared_among_cores());
    SUCCEED();
}

TEST(R7_7_c_Topology_NoSimd, ZeroUnitsButAllCores) {
    // NoSimd: keine SIMD-Units (0), aber Default "alle Cores koennen 'kein SIMD'".
    static_assert(ax09b::NoSimdExtension::units_per_socket() == 0);
    static_assert(ax09b::NoSimdExtension::accessible_from_efficiency_cores());
    SUCCEED();
}
