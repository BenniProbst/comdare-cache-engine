#pragma once
// V41.F.6.1.R7.5.g axis_migration HotColdMigration

#include "axis_migration_strategy_base.hpp"
#include "axis_migration_subaxes_mg1_to_mg3.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include <axes/migration_policy/axis_migration_flags.hpp>
#include <topics/migration/concepts/topic_migration_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration_policy {

/// HotColdMigration — Hot/Cold-Separation via LRU + probabilistisches Counting.
/// Migriert haeufig zugegriffene (hot) Daten zu schnellem Cache, selten
/// zugegriffene (cold) zu langsamem Storage. Standard fuer LRU-K + LFU-Caches.
class HotColdMigration : public MigrationStrategyBase<HotColdMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using axis_tag  = subaxes::trigger_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::hot_cold_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "migration_hot_cold"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "HotColdMigration (LRU+probabilistic Hot/Cold separation)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "HOT_COLD"; }
};

}  // namespace

namespace comdare::cache_engine::migration_policy {
    static_assert(concepts::MigrationStrategy<HotColdMigration>);
    static_assert(concepts::CacheEnginePermutationStrategy<HotColdMigration>);
}
