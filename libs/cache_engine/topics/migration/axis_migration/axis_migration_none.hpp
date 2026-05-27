#pragma once
// V41.F.6.1.R7.5.g axis_migration NoMigration (Goldstandard-Update)

#include "axis_migration_strategy_base.hpp"
#include "axis_migration_subaxes_mg1_to_mg3.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include "axis_migration_flags.hpp"
#include "../concepts/topic_migration_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration::axis_migration {

/// NoMigration — Default: static placement, keine Migration (baseline).
class NoMigration : public MigrationStrategyBase<NoMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using axis_tag  = subaxes::trigger_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::none_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "migration_none"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "NoMigration (static placement, no migration baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NONE"; }
};

}  // namespace

namespace comdare::cache_engine::migration::axis_migration {
    static_assert(concepts::MigrationStrategy<NoMigration>);
    static_assert(concepts::CacheEnginePermutationStrategy<NoMigration>);
}
