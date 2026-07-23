#pragma once
// V41.F.6.1.R7.5.g axis_migration NoMigration (Goldstandard-Update)

#include "axis_migration_strategy_base.hpp"
#include "axis_migration_subaxes_mg1_to_mg3.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include <axes/migration_policy/axis_migration_flags.hpp>
#include <topics/migration/concepts/topic_migration_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::migration_policy {

/// NoMigration — Default: static placement, keine Migration (baseline).
class NoMigration : public MigrationStrategyBase<NoMigration> {
public:
    using topic_tag = ::comdare::cache_engine::migration::concepts::MigrationTopicTag;
    using axis_tag  = subaxes::trigger_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::none_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "migration_none"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::migration_policy::NoMigration",
                                  "axes/migration_policy/axis_migration_none.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "NoMigration (static placement, no migration baseline)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NONE"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41.F.6.1 — verhaltens-tragende Mess-Op (migration_policy F15-operativ): Entscheidungs-Scan.
    // EHRLICHKEIT: Migration ohne 2. Tier nicht ausfuehrbar -> gemessen werden ausschliesslich die
    // "Entscheidungslogik-Kosten ohne 2. Tier". KEINE konstante Zeit: jede Strategie traegt eine reale,
    // strategie-abhaengige Entscheidungslogik ueber denselben strided 4-Byte-Scan.
    // NoMigration = static placement: per Definition KEINE Entscheidung -> return 0 (echte Baseline,
    // bewusst leerer Pfad, nicht n/a). buf/n/record_size bleiben absichtlich ungenutzt.
    [[nodiscard]] static std::uint64_t migration_decide_scan(unsigned char const* /*buf*/, std::size_t /*n*/,
                                                             std::size_t /*record_size*/) noexcept {
        return 0;
    }
};

} // namespace comdare::cache_engine::migration_policy

namespace comdare::cache_engine::migration_policy {
static_assert(concepts::MigrationStrategy<NoMigration>);
static_assert(concepts::CacheEnginePermutationStrategy<NoMigration>);
} // namespace comdare::cache_engine::migration_policy
