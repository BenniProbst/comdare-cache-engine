#pragma once
// Per-Achsen-Observer Phase B (2026-06-04) — ObservableMigration<Strategy>: ObservableAxis-Huelle um eine
// migration_policy-Strategie (T15, axis_migration). EXAKT analog axis_05_memory_layout_observable.hpp: die
// Strategie selbst (NoMigration/HotCold/TierBased/Adaptive) traegt zwar die verhaltens-tragende static Methode
// migration_decide_scan(), hat aber KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die Mess-Mechanik
// gehoert daher in diese Huelle, die migration_decide_scan static durchreicht (seg19-Aufrufer bleiben heil) UND
// einen Instanz-Driver observe_decide() ergaenzt, der beim Treiben die Entscheidungs-Statistik trackt.
//
// @topic migration @achse 15 @saeule 2 (Per-Achsen-Observer) @task Phase-B
//
// **Achsen-Semantik (EHRLICH deklariert):** Die migration_policy-Achse misst die ENTSCHEIDUNGSLOGIK-Kosten OHNE
// 2. Tier (Hauptagent-Entscheid: Min decide-only, KEIN 2-Tier-Store) — es gibt keinen realen Block-Move, daher
// bleibt `tier_moves` HONEST 0 (kein 2. Tier vorhanden, nicht n/a). Die uebrigen Zaehler folgen der echten
// Entscheidungs-Op: total_decisions = ueber alle Runden geprueffte Records (Σ n je Scan); migrations_triggered =
// nur bei aktiver Strategie (is_active()==true) zaehlende potenzielle Migrations-Entscheidungen (NoMigration =
// static placement → 0, ehrliche Baseline); hot_votes/cold_votes = die strategie-charakteristische Hot/Cold-
// Klassifikation des kanonischen strided 4-Byte-Recency-Scans (identische Vote-Regel wie HotColdMigration::
// migration_decide_scan — KEIN erfundener Wert, die Votes spiegeln die echte Demotions-/Promotions-Tendenz der
// Daten). hot_votes+cold_votes == total_decisions bei aktiver Strategie.
//
// Gating exakt nach Praezedenz (axis_05): snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: migration_decide_scan = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false → observe_all()
// faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_migration_concept.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::migration_policy {

/// ABI-taugliches Migration-Snapshot (NUR uint64 → standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct MigrationSnapshot {
    std::uint64_t total_decisions      = 0;   ///< ueber alle Runden geprueffte Records (Σ n)
    std::uint64_t migrations_triggered = 0;   ///< potenzielle Migrations-Entscheidungen (nur is_active()-Strategien)
    std::uint64_t hot_votes            = 0;   ///< Promotions-Tendenz-Votes (steigende/gleiche Recency)
    std::uint64_t cold_votes           = 0;   ///< Demotions-Tendenz-Votes (fallende Recency)
    std::uint64_t tier_moves           = 0;   ///< HONEST 0 — kein 2. Tier (decide-only, kein realer Block-Move)

    [[nodiscard]] bool operator==(MigrationSnapshot const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<MigrationSnapshot>);
static_assert(std::is_trivially_copyable_v<MigrationSnapshot>);

/// ObservableAxis-Huelle: migration_policy-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) → direkt als Anatomie-Member haltbar.
template <class Strategy>
    requires concepts::MigrationStrategy<Strategy>
class ObservableMigration {
public:
    using strategy_type = Strategy;
    // topic_tag durchgereicht → die Huelle erfuellt MigrationComponent/MigrationStrategy und ist als
    // migration_policy-Slot einsetzbar (composition_registry / axis_path_serialization rufen C::migration_policy::name()).
    using topic_tag = typename Strategy::topic_tag;

    // Transparenter Decorator: Strategie-Inspektion durchgereicht.
    [[nodiscard]] static constexpr bool             is_active()   noexcept { return Strategy::is_active(); }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); } { return Strategy::get_compiler(); }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode unveraendert durchgereicht, damit die
    /// Huelle als migration_policy-Slot die bestehenden seg19-Aufrufer NICHT bricht (abi_adapter.hpp T15
    /// `Migration::migration_decide_scan`). Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t migration_decide_scan(unsigned char const* buf, std::size_t n,
                                                             std::size_t record_size) noexcept {
        return Strategy::migration_decide_scan(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): treibt den Entscheidungs-Scan + trackt. Der Observer-
    /// Treiber (abi_adapter::fill_observer_v3 / tier_insert-Kopplung) ruft dies → die Entscheidungs-Aktivitaet wird
    /// observable. hot/cold-Votes ueber den KANONISCHEN strided 4-Byte-Recency-Scan (identische Regel wie
    /// HotColdMigration::migration_decide_scan; bei aktiver Strategie). Getrennt von der static-Variante, weil die
    /// bestehenden seg19-Aufrufer static bleiben muessen.
    void observe_decide(unsigned char const* buf, std::size_t n, std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::migration_decide_scan(buf, n, record_size);
        (void)checksum;   // Treibe-Op real exerziert (Wegoptimierungs-Schutz erfolgt im seg19-Pfad via sink)
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_.total_decisions += static_cast<std::uint64_t>(n);
        // migrations_triggered nur fuer aktive Strategien (NoMigration = static placement → 0, ehrliche Baseline).
        if (Strategy::is_active()) {
            stats_.migrations_triggered += static_cast<std::uint64_t>(n);
            // Hot/Cold-Klassifikation: kanonischer Recency-Scan (gleiche Vote-Regel wie HotColdMigration).
            std::uint32_t lru_recency = 0;
            for (std::size_t i = 0; i < n; ++i) {
                std::uint32_t v{};
                std::memcpy(&v, buf + i * record_size, sizeof(v));   // strided 4-Byte-Recency-Feld
                if (v >= lru_recency) ++stats_.hot_votes;            // steigende/gleiche Recency → hot (Promotion)
                else                  ++stats_.cold_votes;           // fallende Recency → cold (Demotion)
                lru_recency = v;
            }
        }
        // stats_.tier_moves bleibt HONEST 0 — kein 2. Tier (decide-only).
#else
        (void)buf; (void)n; (void)record_size;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = MigrationSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

}  // namespace comdare::cache_engine::migration_policy
