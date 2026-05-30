#pragma once
// V41.F.6.1.R7.5.g axis_migration AdaptiveMigration (ML-driven)

#include "axis_migration_strategy_base.hpp"
#include "axis_migration_subaxes_mg1_to_mg3.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include <axes/migration_policy/axis_migration_flags.hpp>
#include <topics/migration/concepts/topic_migration_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration_policy {

/// AdaptiveMigration — ML-driven adaptive migration mit Online-Learning.
/// Beobachtet Access-Pattern + entscheidet pro Block. Verwandt mit
/// Cachelib (Facebook 2020) + LeCaR (Vietri OSDI 2018). Trade-off:
/// hoher Overhead vs optimale Hit-Rate.
class AdaptiveMigration : public MigrationStrategyBase<AdaptiveMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using axis_tag  = subaxes::granularity_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::adaptive_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "migration_adaptive"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "AdaptiveMigration (ML-driven, Cachelib/LeCaR-style)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "ADAPTIVE"; }
};

}  // namespace

namespace comdare::cache_engine::migration_policy {
    static_assert(concepts::MigrationStrategy<AdaptiveMigration>);
    static_assert(concepts::CacheEnginePermutationStrategy<AdaptiveMigration>);
}
