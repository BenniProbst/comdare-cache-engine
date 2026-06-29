#pragma once
// V41.F.6.1.R7.5.g axis_migration HotColdMigration

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

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "migration_hot_cold"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "HotColdMigration (LRU+probabilistic Hot/Cold separation)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "HOT_COLD"; }

    // V41.F.6.1 — verhaltens-tragende Mess-Op (migration_policy F15-operativ): Entscheidungs-Scan.
    // EHRLICHKEIT: Migration ohne 2. Tier nicht ausfuehrbar -> gemessen werden ausschliesslich die
    // "Entscheidungslogik-Kosten ohne 2. Tier" (kein realer Block-Move). KEINE konstante Zeit.
    // HotCold = LRU-Counter-Entscheidung: pro Record wird ein Recency-Zaehler gefuehrt; sinkt der
    // Feldwert gegenueber dem zuletzt gesehenen (cold-Tendenz), zaehlt ein Demotions-Vote, sonst ein
    // Promotions-Vote (hot). Datenabhaengiger Branch + laufender Zustand -> reale, strategie-spezifische
    // Laufzeit (Hot/Cold-Klassifikation), distinkt zu TierBased/Adaptive.
    [[nodiscard]] static std::uint64_t migration_decide_scan(unsigned char const* buf, std::size_t n,
                                                             std::size_t record_size) noexcept {
        std::uint64_t hot_votes   = 0;
        std::uint64_t cold_votes  = 0;
        std::uint32_t lru_recency = 0; // letzter gesehener Feldwert (LRU-Recency-Marker)
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // strided 4-Byte-Feld
            if (v >= lru_recency) {
                ++hot_votes; // erneuter/steigender Zugriff -> hot (Promotion-Tendenz)
            } else {
                ++cold_votes; // abfallende Recency -> cold (Demotion-Tendenz)
            }
            lru_recency = v; // LRU-Counter aktualisieren
        }
        return hot_votes + cold_votes;
    }
};

} // namespace comdare::cache_engine::migration_policy

namespace comdare::cache_engine::migration_policy {
static_assert(concepts::MigrationStrategy<HotColdMigration>);
static_assert(concepts::CacheEnginePermutationStrategy<HotColdMigration>);
} // namespace comdare::cache_engine::migration_policy
