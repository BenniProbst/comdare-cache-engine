// V41.F.6.1.R7.5.d axis_14_value_handle Tests (Goldstandard-konform)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.d (Optional-Topics: value_handle)

#include <gtest/gtest.h>

#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_inline.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_external_pool.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_immutable_shared_ref.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_versioned_pointer.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_registry.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_subaxes_vh1_to_vh3.hpp>
#include <topics/value_handle/axis_14_value_handle/axis_14_value_handle_flags.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax14 = ::comdare::cache_engine::value_handle::axis_14_value_handle;
namespace vh   = ::comdare::cache_engine::value_handle;
namespace mp   = ::boost::mp11;

TEST(R7_5_d_Axis14, AllHandlesSatisfyConcepts) {
    static_assert(ax14::concepts::ValueHandleStrategy<ax14::InlineHandle>);
    static_assert(ax14::concepts::ValueHandleStrategy<ax14::ExternalPoolHandle>);
    static_assert(ax14::concepts::ValueHandleStrategy<ax14::ImmutableSharedRefHandle>);
    static_assert(ax14::concepts::ValueHandleStrategy<ax14::VersionedPointerHandle>);
    static_assert(ax14::concepts::CacheEnginePermutationStrategy<ax14::InlineHandle>);
    static_assert(ax14::concepts::CacheEnginePermutationStrategy<ax14::ExternalPoolHandle>);
    static_assert(ax14::concepts::CacheEnginePermutationStrategy<ax14::ImmutableSharedRefHandle>);
    static_assert(ax14::concepts::CacheEnginePermutationStrategy<ax14::VersionedPointerHandle>);
    SUCCEED();
}

TEST(R7_5_d_Axis14, IsInlineDifferentiated) {
    static_assert(ax14::InlineHandle::is_inline()              == true);
    static_assert(ax14::ExternalPoolHandle::is_inline()        == false);
    static_assert(ax14::ImmutableSharedRefHandle::is_inline()  == false);
    static_assert(ax14::VersionedPointerHandle::is_inline()    == false);
    SUCCEED();
}

TEST(R7_5_d_Axis14, FlagSuffixUppercase) {
    static_assert(ax14::InlineHandle::flag_suffix()              == std::string_view{"INLINE"});
    static_assert(ax14::ExternalPoolHandle::flag_suffix()        == std::string_view{"EXTERNAL_POOL"});
    static_assert(ax14::ImmutableSharedRefHandle::flag_suffix()  == std::string_view{"IMMUTABLE_SHARED_REF"});
    static_assert(ax14::VersionedPointerHandle::flag_suffix()    == std::string_view{"VERSIONED_POINTER"});
    SUCCEED();
}

TEST(R7_5_d_Axis14, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax14::InlineHandle::axis_tag,             ax14::subaxes::storage_location_tag>);
    static_assert(std::is_same_v<ax14::ExternalPoolHandle::axis_tag,       ax14::subaxes::storage_location_tag>);
    static_assert(std::is_same_v<ax14::ImmutableSharedRefHandle::axis_tag, ax14::subaxes::ownership_tag>);
    static_assert(std::is_same_v<ax14::VersionedPointerHandle::axis_tag,   ax14::subaxes::versioning_tag>);
    SUCCEED();
}

TEST(R7_5_d_Axis14, RegistryHas4Handles) {
    static_assert(mp::mp_size<ax14::AllHandles>::value == 4);
    static_assert(mp::mp_size<ax14::EnabledHandles>::value > 0);
    SUCCEED();
}

TEST(R7_5_d_Axis14, FamilyIdsDistinct) {
    static_assert(ax14::InlineHandle::family_id::value             == 1);
    static_assert(ax14::ExternalPoolHandle::family_id::value       == 2);
    static_assert(ax14::ImmutableSharedRefHandle::family_id::value == 3);
    static_assert(ax14::VersionedPointerHandle::family_id::value   == 4);
    SUCCEED();
}

TEST(R7_5_d_Axis14, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax14::flags::inline_enabled),               const bool>);
    static_assert(std::is_same_v<decltype(ax14::flags::external_pool_enabled),        const bool>);
    static_assert(std::is_same_v<decltype(ax14::flags::immutable_shared_ref_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax14::flags::versioned_pointer_enabled),    const bool>);
    SUCCEED();
}

TEST(R7_5_d_ValueHandle, TopicConfigSetExposesAxis14) {
    static_assert(mp::mp_size<vh::TopicConfigSet::StaticAxisVariants_14>::value > 0);
    static_assert(std::is_same_v<vh::TopicConfigSet::StaticAxisVariants,
                                  vh::TopicConfigSet::StaticAxisVariants_14>);
    SUCCEED();
}
