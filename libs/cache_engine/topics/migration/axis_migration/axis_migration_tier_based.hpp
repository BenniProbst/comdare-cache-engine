#pragma once
// V41.F.6.1.R7.5.g axis_migration TierBasedMigration (RAM/SSD)

#include "axis_migration_strategy_base.hpp"
#include "axis_migration_subaxes_mg1_to_mg3.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include "axis_migration_flags.hpp"
#include "../concepts/topic_migration_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration::axis_migration {

/// TierBasedMigration — Multi-Tier-Migration: RAM → SSD → HDD.
/// Standard fuer RocksDB / LSM-Trees + Tiered-Cache-Systeme.
/// Migrations-Schwellwerte pro Tier konfigurierbar. Bidirektional
/// (Promotion bei Lookup-Hit, Demotion bei Capacity-Pressure).
class TierBasedMigration : public MigrationStrategyBase<TierBasedMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using axis_tag  = subaxes::direction_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::tier_based_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "migration_tier_based"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "TierBasedMigration (RAM/SSD/HDD multi-tier, RocksDB-style)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "TIER_BASED"; }
};

}  // namespace

namespace comdare::cache_engine::migration::axis_migration {
    static_assert(concepts::MigrationStrategy<TierBasedMigration>);
    static_assert(concepts::CacheEnginePermutationStrategy<TierBasedMigration>);
}
