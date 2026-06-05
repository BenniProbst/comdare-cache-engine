#pragma once
// V42 L-74c Composition-Driver — ObservableTelemetry<Strategy>: ObservableAxis-Huelle um eine telemetry-
// Strategie (axis_11). Analog observable_composed_search.hpp (#42): die Achsen-Strategie selbst (LeafOnlyCounter
// etc.) ist ein reiner Marker OHNE statistics() (test_d_v42_probe2: ObservableAxis<LeafOnlyCounter>=0). Die
// Mess-Mechanik gehoert daher in diese Huelle, parametrisiert durch die Strategie.
//
// @topic telemetry @achse 11 @saeule 2 (Per-Achsen-Observer) @task V42-L-74c
//
// **Achsen-Semantik (treu, nicht erfunden):** Die telemetry-Achse misst Mess-Overhead nach Scope (TM1:
// leaf-only / per-node, axis_11_telemetry_subaxes). Eine leaf-only-Strategie aktualisiert Zaehler NUR in
// Blatt-Knoten (vermeidet Cache-Line-Ping-Pong durch shared Inner-Node-Updates, axis_11_telemetry_leaf_only.hpp:14-15).
// Die Huelle zaehlt Knoten-Touches und verwirft Inner-Node-Touches, wenn die Strategie leaf-only ist —
// damit ist `node_updates` der literal messbare Overhead-Unterschied zwischen den Strategien.
//
// Gating exakt nach ObservableComposedSearch-Praezedenz: snapshot_t/statistics()/reset() unter
// COMDARE_CE_ENABLE_STATISTICS. Bei OFF: record_*() = no-op (0 Footprint), ObservableAxis<...> = false
// -> observe_all() faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_11_telemetry_concept.hpp"
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::telemetry_axis {

/// ABI-taugliches Telemetrie-Snapshot (standard_layout + trivially_copyable -> spaeter append-only in den
/// Cross-ABI-Observer-POD mappbar, Doc 29 §3 Schritt 4).
struct TelemetrySnapshot {
    std::uint64_t total_events = 0;   ///< alle Mess-Ereignisse (insert/lookup/erase-Knoten-Touches)
    std::uint64_t leaf_updates = 0;   ///< gezaehlte Blatt-Knoten-Zaehler-Updates
    std::uint64_t node_updates = 0;   ///< gezaehlte Inner-Knoten-Updates (== 0 bei leaf-only-Strategie)
    std::uint64_t peak_tracked = 0;   ///< Hoechststand leaf_updates+node_updates

    [[nodiscard]] bool operator==(TelemetrySnapshot const&) const noexcept = default;
};

/// ObservableAxis-Huelle: telemetry-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) -> direkt als Anatomie-Member `ObservableTelemetry<S> m;`
/// haltbar (umgeht das `{}`-Aggregat-Init-Problem des nackten Strategie-Wrappers, test_d_v42_probe2).
template <class Strategy>
    requires concepts::TelemetryStrategy<Strategy>
class ObservableTelemetry {
public:
    using strategy_type = Strategy;

    // Transparenter Decorator: die Strategie-Inspektion wird durchgereicht, damit die Huelle ueberall als
    // telemetry-Slot funktioniert (composition_registry.hpp:35 / axis_path_serialization.hpp:67 rufen
    // C::telemetry::name()). family_name/flag_suffix nur weiterreichen, wenn die Strategie sie bietet.
    [[nodiscard]] static constexpr bool             is_leaf_only() noexcept { return Strategy::is_leaf_only(); }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }
    [[nodiscard]] static constexpr std::string_view get_compiler()
        noexcept requires requires { Strategy::get_compiler(); } { return Strategy::get_compiler(); }

    /// Mess-Kopplung (der eigentliche „Driver", Doc 29 §3 Schritt 3): der Tier-insert/lookup ruft dies bei
    /// jedem Knoten-Touch. `is_leaf`=true fuer Blatt-Knoten. Leaf-only-Strategien verwerfen Inner-Touches.
    void record_node_touch(bool is_leaf) noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_events;
        if (is_leaf) {
            ++stats_.leaf_updates;
        } else if (!Strategy::is_leaf_only()) {
            ++stats_.node_updates;
        }
        std::uint64_t const tracked = stats_.leaf_updates + stats_.node_updates;
        if (tracked > stats_.peak_tracked) stats_.peak_tracked = tracked;
#else
        (void)is_leaf;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = TelemetrySnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    // reset() = Statistik-Reset (Memory-Regel), NICHT Strategie-Reset.
    void reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

}  // namespace comdare::cache_engine::telemetry_axis
