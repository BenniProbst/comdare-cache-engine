#pragma once
// V41.F.6.1.R7.5.g axis_migration AdaptiveMigration (ML-driven)

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

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "migration_adaptive"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::migration_policy::AdaptiveMigration",
                                  "axes/migration_policy/axis_migration_adaptive.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "AdaptiveMigration (ML-driven, Cachelib/LeCaR-style)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ADAPTIVE"; }

    // V41.F.6.1 — verhaltens-tragende Mess-Op (migration_policy F15-operativ): Entscheidungs-Scan.
    // EHRLICHKEIT: Migration ohne 2. Tier nicht ausfuehrbar -> gemessen werden ausschliesslich die
    // "Entscheidungslogik-Kosten ohne 2. Tier" (kein realer Block-Move). KEINE konstante Zeit.
    // Adaptive = ML-Score via Linearkombination (Cachelib/LeCaR-Stil): pro Record bildet ein gewichteter
    // Online-Score aus Feldwert (Frequenz-Feature) UND Positions-Index (Recency-Feature) den Migrations-
    // Entscheidungswert; ein gleitender Score-Akku modelliert das Online-Learning. Mehr Rechen-Features
    // pro Record als HotCold/TierBased -> reale, hoehere strategie-spezifische Laufzeit (charakteristischer
    // Adaptive-Overhead), distinkt zu HotCold (Branch-Vote) und TierBased (Modulo).
    [[nodiscard]] static std::uint64_t migration_decide_scan(unsigned char const* buf, std::size_t n,
                                                             std::size_t record_size) noexcept {
        // Gewichte fuer Frequenz- (w_freq) und Recency-Feature (w_rec) der Linearkombination.
        constexpr std::uint64_t w_freq      = 3;
        constexpr std::uint64_t w_rec       = 1;
        std::uint64_t           score_acc   = 0; // gleitender Online-Score (Learning-Zustand)
        std::uint64_t           migrate_acc = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // strided 4-Byte-Feld (Frequenz-Feature)
            std::uint64_t const recency = static_cast<std::uint64_t>(i & 0xFFu); // Recency-Feature
            std::uint64_t const score   = w_freq * static_cast<std::uint64_t>(v) + w_rec * recency +
                                          (score_acc >> 8); // Beitrag des gleitenden Lern-Scores
            score_acc += score;
            migrate_acc += (score & 0x1u); // datenabhaengige Migrations-Entscheidung aus Score
        }
        return migrate_acc + (score_acc & 0xFFFFu);
    }
};

} // namespace comdare::cache_engine::migration_policy

namespace comdare::cache_engine::migration_policy {
static_assert(concepts::MigrationStrategy<AdaptiveMigration>);
static_assert(concepts::CacheEnginePermutationStrategy<AdaptiveMigration>);
} // namespace comdare::cache_engine::migration_policy
