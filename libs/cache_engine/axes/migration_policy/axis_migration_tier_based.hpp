#pragma once
// V41.F.6.1.R7.5.g axis_migration TierBasedMigration (RAM/SSD)

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

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "migration_tier_based"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::migration_policy::TierBasedMigration",
                                  "axes/migration_policy/axis_migration_tier_based.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "TierBasedMigration (RAM/SSD/HDD multi-tier, RocksDB-style)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "TIER_BASED"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41.F.6.1 — verhaltens-tragende Mess-Op (migration_policy F15-operativ): Entscheidungs-Scan.
    // EHRLICHKEIT: Migration ohne 2. Tier nicht ausfuehrbar -> gemessen werden ausschliesslich die
    // "Entscheidungslogik-Kosten ohne 2. Tier" (kein realer RAM->SSD->HDD-Move). KEINE konstante Zeit.
    // TierBased = Tier-Zuordnung via Modulo: jeder Feldwert wird per Modulo auf kTiers (RAM/SSD/HDD)
    // gemappt; pro Record summiert sich der Ziel-Tier-Index. Modulo-Division + datenabhaengiges Mapping
    // -> reale, strategie-spezifische Laufzeit, distinkt zu HotCold (Branch-Vote) und Adaptive (Linearkomb.).
    [[nodiscard]] static std::uint64_t migration_decide_scan(unsigned char const* buf, std::size_t n,
                                                             std::size_t record_size) noexcept {
        constexpr std::uint32_t kTiers         = 3; // RAM / SSD / HDD
        std::uint64_t           tier_index_sum = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // strided 4-Byte-Feld
            tier_index_sum += (v % kTiers);                    // Ziel-Tier per Modulo (0=RAM,1=SSD,2=HDD)
        }
        return tier_index_sum;
    }
};

} // namespace comdare::cache_engine::migration_policy

namespace comdare::cache_engine::migration_policy {
static_assert(concepts::MigrationStrategy<TierBasedMigration>);
static_assert(concepts::CacheEnginePermutationStrategy<TierBasedMigration>);
} // namespace comdare::cache_engine::migration_policy
