// V41.F.6.1.R7.5.g axis_migration Tests (Goldstandard + TYPED_TEST_SUITE)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.g (Optional-Topics: migration)

#include <gtest/gtest.h>

#include <topics/migration/axis_migration/axis_migration_none.hpp>
#include <topics/migration/axis_migration/axis_migration_hot_cold.hpp>
#include <topics/migration/axis_migration/axis_migration_tier_based.hpp>
#include <topics/migration/axis_migration/axis_migration_adaptive.hpp>
#include <topics/migration/axis_migration/axis_migration_registry.hpp>
#include <topics/migration/axis_migration/axis_migration_subaxes_mg1_to_mg3.hpp>
#include <topics/migration/axis_migration/axis_migration_flags.hpp>
#include <topics/migration/topic_migration_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax_mig = ::comdare::cache_engine::migration::axis_migration;
namespace mig    = ::comdare::cache_engine::migration;
namespace mp     = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern (axis_06_allocator-Goldstandard) ───
template <class... Ms>
using ToGTestTypes = ::testing::Types<Ms...>;

using AllMigrationTypes = mp::mp_apply<ToGTestTypes, ax_mig::AllMigrations>;

template <class T>
class MigrationWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(MigrationWrapperTest, AllMigrationTypes);

TYPED_TEST(MigrationWrapperTest, ConceptConformance) {
    static_assert(ax_mig::concepts::MigrationStrategy<TypeParam>);
    static_assert(ax_mig::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(MigrationWrapperTest, HasTopicTagAndFamilyId) {
    using topic_tag_t = typename TypeParam::topic_tag;
    static_assert(std::is_same_v<topic_tag_t,
                                 ::comdare::cache_engine::migration::concepts::MigrationTopicTag>);
    SUCCEED();
}

TYPED_TEST(MigrationWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(MigrationWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax_mig::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests

// ─── Spezifische Verhaltens-Tests (5) ───

TEST(R7_5_g_Axis_Migration_Specific, IsActiveDifferentiated) {
    // Nur NoMigration ist inaktiv; alle anderen sind aktiv
    static_assert(ax_mig::NoMigration::is_active()        == false);
    static_assert(ax_mig::HotColdMigration::is_active()   == true);
    static_assert(ax_mig::TierBasedMigration::is_active() == true);
    static_assert(ax_mig::AdaptiveMigration::is_active()  == true);
    SUCCEED();
}

TEST(R7_5_g_Axis_Migration_Specific, FlagSuffixUppercase) {
    static_assert(ax_mig::NoMigration::flag_suffix()        == std::string_view{"NONE"});
    static_assert(ax_mig::HotColdMigration::flag_suffix()   == std::string_view{"HOT_COLD"});
    static_assert(ax_mig::TierBasedMigration::flag_suffix() == std::string_view{"TIER_BASED"});
    static_assert(ax_mig::AdaptiveMigration::flag_suffix()  == std::string_view{"ADAPTIVE"});
    SUCCEED();
}

TEST(R7_5_g_Axis_Migration_Specific, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax_mig::NoMigration::axis_tag,        ax_mig::subaxes::trigger_tag>);
    static_assert(std::is_same_v<ax_mig::HotColdMigration::axis_tag,   ax_mig::subaxes::trigger_tag>);
    static_assert(std::is_same_v<ax_mig::TierBasedMigration::axis_tag, ax_mig::subaxes::direction_tag>);
    static_assert(std::is_same_v<ax_mig::AdaptiveMigration::axis_tag,  ax_mig::subaxes::granularity_tag>);
    SUCCEED();
}

TEST(R7_5_g_Axis_Migration_Specific, RegistryHas4Migrations) {
    static_assert(mp::mp_size<ax_mig::AllMigrations>::value == 4);
    static_assert(mp::mp_size<ax_mig::EnabledMigrations>::value > 0);
    SUCCEED();
}

TEST(R7_5_g_Migration, TopicConfigSetExposesAxisMigration) {
    static_assert(mp::mp_size<mig::TopicConfigSet::StaticAxisVariants_MG>::value > 0);
    static_assert(std::is_same_v<mig::TopicConfigSet::StaticAxisVariants,
                                  mig::TopicConfigSet::StaticAxisVariants_MG>);
    SUCCEED();
}

// Total: 4 Wrappers × 4 TYPED = 16 typed + 5 spezifisch = 21 Tests
