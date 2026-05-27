// V41.F.6.1.R7.5.i axis_09_isa Tests (Goldstandard + TYPED_TEST_SUITE)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.i (Optional-Topics: isa — letzte R7.5-Achse)

#include <gtest/gtest.h>

#include <topics/hardware/axis_09_isa/axis_09_isa_scalar.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_sse2.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_avx2.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_neon.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_subaxes_is1_to_is3.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_flags.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax09 = ::comdare::cache_engine::hardware::axis_09_isa;
namespace hw   = ::comdare::cache_engine::hardware;
namespace mp   = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern (axis_06_allocator-Goldstandard) ───
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

TYPED_TEST(IsaWrapperTest, HasTopicTagAndFamilyId) {
    using topic_tag_t = typename TypeParam::topic_tag;
    static_assert(std::is_same_v<topic_tag_t,
                                 ::comdare::cache_engine::hardware::concepts::HardwareTopicTag>);
    SUCCEED();
}

TYPED_TEST(IsaWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(IsaWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax09::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests

// ─── Spezifische Verhaltens-Tests (5) ───

TEST(R7_5_i_Axis_09_Specific, SupportsSimdDifferentiated) {
    // Nur Scalar hat keine SIMD; alle anderen sind SIMD-faehig
    static_assert(ax09::IsaScalar::supports_simd() == false);
    static_assert(ax09::IsaSse2::supports_simd()   == true);
    static_assert(ax09::IsaAvx2::supports_simd()   == true);
    static_assert(ax09::IsaNeon::supports_simd()   == true);
    SUCCEED();
}

TEST(R7_5_i_Axis_09_Specific, FlagSuffixUppercase) {
    static_assert(ax09::IsaScalar::flag_suffix() == std::string_view{"SCALAR"});
    static_assert(ax09::IsaSse2::flag_suffix()   == std::string_view{"SSE2"});
    static_assert(ax09::IsaAvx2::flag_suffix()   == std::string_view{"AVX2"});
    static_assert(ax09::IsaNeon::flag_suffix()   == std::string_view{"NEON"});
    SUCCEED();
}

TEST(R7_5_i_Axis_09_Specific, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax09::IsaScalar::axis_tag, ax09::subaxes::simd_width_tag>);
    static_assert(std::is_same_v<ax09::IsaSse2::axis_tag,   ax09::subaxes::vector_arch_tag>);
    static_assert(std::is_same_v<ax09::IsaAvx2::axis_tag,   ax09::subaxes::simd_width_tag>);
    static_assert(std::is_same_v<ax09::IsaNeon::axis_tag,   ax09::subaxes::vector_arch_tag>);
    SUCCEED();
}

TEST(R7_5_i_Axis_09_Specific, RegistryHas4Isas) {
    static_assert(mp::mp_size<ax09::AllIsas>::value == 4);
    static_assert(mp::mp_size<ax09::EnabledIsas>::value > 0);
    SUCCEED();
}

TEST(R7_5_i_Hardware, TopicConfigSetExposesAxis09NowWithRegistry) {
    // R7.5.i migrated StaticAxisVariants_09 von lokaler mp_list<IsaScalar>
    // zur axis_09_isa::EnabledIsas (echte Registry mit 4 Wrappers)
    static_assert(mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value == 4);
    // Cartesian-Product axis_09 × axis_12 (4 × 3 = 12)
    constexpr auto isa_count  = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_09>::value;
    constexpr auto plat_count = mp::mp_size<hw::TopicConfigSet::StaticAxisVariants_12>::value;
    constexpr auto prod_count = mp::mp_size<hw::TopicConfigSet::CartesianIsa09xPlatform12>::value;
    static_assert(prod_count == isa_count * plat_count);
    SUCCEED();
}

// Total: 4 Wrappers × 4 TYPED = 16 typed + 5 spezifisch = 21 Tests
