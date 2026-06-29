// V41.F.6.1.R7.5.f axis_io Tests (Goldstandard + TYPED_TEST_SUITE)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.f (Optional-Topics: io)

#include <gtest/gtest.h>

#include <topics/io/axis_io/axis_io_in_memory_only.hpp>
#include <topics/io/axis_io/axis_io_direct.hpp>
#include <topics/io/axis_io/axis_io_buffered.hpp>
#include <topics/io/axis_io/axis_io_mmap.hpp>
#include <topics/io/axis_io/axis_io_registry.hpp>
#include <topics/io/axis_io/axis_io_subaxes_io1_to_io3.hpp>
#include <axes/io_dispatch/axis_io_flags.hpp>
#include <topics/io/topic_io_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax_io = ::comdare::cache_engine::io::axis_io;
namespace io    = ::comdare::cache_engine::io;
namespace mp    = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern (axis_06_allocator-Goldstandard) ───
template <class... Is>
using ToGTestTypes = ::testing::Types<Is...>;

using AllIoTypes = mp::mp_apply<ToGTestTypes, ax_io::AllIos>;

template <class T>
class IoWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(IoWrapperTest, AllIoTypes);

TYPED_TEST(IoWrapperTest, ConceptConformance) {
    static_assert(ax_io::concepts::IoStrategy<TypeParam>);
    static_assert(ax_io::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(IoWrapperTest, HasTopicTagAndFamilyId) {
    using topic_tag_t = typename TypeParam::topic_tag;
    using family_id_t = typename TypeParam::family_id;
    static_assert(family_id_t::value > 0);
    static_assert(std::is_same_v<topic_tag_t, ::comdare::cache_engine::io::concepts::IoTopicTag>);
    SUCCEED();
}

TYPED_TEST(IoWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(IoWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax_io::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests

// ─── Spezifische Verhaltens-Tests (5) ───

TEST(R7_5_f_Axis_IO_Specific, IsInMemoryOnlyDifferentiated) {
    // Nur InMemoryOnly hat true; alle anderen sind persistent
    static_assert(ax_io::InMemoryOnly::is_in_memory_only() == true);
    static_assert(ax_io::DirectIo::is_in_memory_only() == false);
    static_assert(ax_io::BufferedIo::is_in_memory_only() == false);
    static_assert(ax_io::MmapIo::is_in_memory_only() == false);
    SUCCEED();
}

TEST(R7_5_f_Axis_IO_Specific, FlagSuffixUppercase) {
    static_assert(ax_io::InMemoryOnly::flag_suffix() == std::string_view{"IN_MEMORY_ONLY"});
    static_assert(ax_io::DirectIo::flag_suffix() == std::string_view{"DIRECT"});
    static_assert(ax_io::BufferedIo::flag_suffix() == std::string_view{"BUFFERED"});
    static_assert(ax_io::MmapIo::flag_suffix() == std::string_view{"MMAP"});
    SUCCEED();
}

TEST(R7_5_f_Axis_IO_Specific, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax_io::InMemoryOnly::axis_tag, ax_io::subaxes::persistence_tag>);
    static_assert(std::is_same_v<ax_io::DirectIo::axis_tag, ax_io::subaxes::caching_strategy_tag>);
    static_assert(std::is_same_v<ax_io::BufferedIo::axis_tag, ax_io::subaxes::caching_strategy_tag>);
    static_assert(std::is_same_v<ax_io::MmapIo::axis_tag, ax_io::subaxes::persistence_tag>);
    SUCCEED();
}

TEST(R7_5_f_Axis_IO_Specific, RegistryHas4Ios) {
    static_assert(mp::mp_size<ax_io::AllIos>::value == 4);
    static_assert(mp::mp_size<ax_io::EnabledIos>::value > 0);
    SUCCEED();
}

TEST(R7_5_f_IO, TopicConfigSetExposesAxisIO) {
    static_assert(mp::mp_size<io::TopicConfigSet::StaticAxisVariants_IO>::value > 0);
    static_assert(std::is_same_v<io::TopicConfigSet::StaticAxisVariants, io::TopicConfigSet::StaticAxisVariants_IO>);
    SUCCEED();
}

// Total: 4 Wrappers × 4 TYPED = 16 typed + 5 spezifisch = 21 Tests
