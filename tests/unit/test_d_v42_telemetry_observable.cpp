// V42 L-74c — ObservableTelemetry<Strategy> Driver-Baustein-Test (2026-06-02). Belegt LITERAL (MIT
// COMDARE_CE_ENABLE_STATISTICS), dass die telemetry-Achse jetzt eine echte, getriebene ObservableAxis ist:
//   - ObservableAxis<ObservableTelemetry<...>> == true (statistics()/snapshot_t vorhanden),
//   - record_node_touch() treibt den Counter (Delta vor/nach > 0, kein Stub),
//   - die Strategie parametrisiert die Mechanik: LeafOnlyCounter (is_leaf_only) VERWIRFT Inner-Touches
//     (node_updates==0), InsertCounter (non-leaf) ZAEHLT sie (node_updates>0) — der messbare Achsen-Unterschied.
// Build: scratch_compile_test.ps1 -Test test_d_v42_telemetry_observable -Boost -Extra @("build\generated")
// (Das Define wird hier in der Quelle gesetzt -> kein Build-Script-Define noetig.)

#define COMDARE_CE_ENABLE_STATISTICS 1

#include <axes/telemetry_axis/axis_11_telemetry_observable.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_leaf_only.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_insert_counter.hpp>
#include "anatomy/observer_aggregate.hpp" // ObservableAxis

#include <cassert>
#include <iostream>
#include <type_traits>

namespace t = comdare::cache_engine::telemetry_axis;
namespace a = comdare::cache_engine::anatomy;

// M-CE-22 / NDEBUG-No-Op-Fix (2026-07-13, Muster-F): harte Checks statt assert(). assert() ist unter NDEBUG
// (Release) ein No-Op -> dieser Test degenerierte im Release-Build zu `return 0` (kein echter Beweis). CE_CHECK
// prueft NDEBUG-unabhaengig und liefert bei Verletzung `return 1` (rot). static_assert() bleibt (compile-time).
#define CE_CHECK(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "[FAIL] " #cond " @ " << __FILE__ << ":" << __LINE__ << "\n";                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

int main() {
    using LeafTel = t::ObservableTelemetry<t::LeafOnlyCounter>;
    using NodeTel = t::ObservableTelemetry<t::InsertCounter>;

    // (1) Beide Huellen sind jetzt ObservableAxis (anders als die nackten Strategie-Wrapper, Probe2: 0).
    static_assert(a::ObservableAxis<LeafTel>, "ObservableTelemetry<LeafOnlyCounter> muss ObservableAxis sein");
    static_assert(a::ObservableAxis<NodeTel>, "ObservableTelemetry<InsertCounter> muss ObservableAxis sein");
    // (2) Huelle ist KEIN Aggregat -> als Anatomie-Member direkt haltbar (umgeht das `{}`-Aggregat-Problem).
    static_assert(!std::is_aggregate_v<LeafTel>, "Huelle darf kein Aggregat sein (sonst {}-Init-Block)");
    // (3) Snapshot ist ABI-tauglich (spaeter append-only in den Cross-ABI-POD).
    static_assert(std::is_standard_layout_v<t::TelemetrySnapshot> && std::is_trivially_copyable_v<t::TelemetrySnapshot>,
                  "TelemetrySnapshot muss standard_layout + trivially_copyable sein");

    // ── leaf-only-Strategie: Inner-Touch wird VERWORFEN ───────────────────────────────────────────────
    LeafTel    leaf;
    auto const leaf_before = leaf.statistics();
    CE_CHECK(leaf_before.total_events == 0);
    leaf.record_node_touch(true);  // Blatt
    leaf.record_node_touch(false); // Inner -> leaf-only verwirft
    leaf.record_node_touch(true);  // Blatt
    auto const leaf_after = leaf.statistics();
    std::cout << "LeafOnlyCounter : total=" << leaf_after.total_events << " leaf=" << leaf_after.leaf_updates
              << " node=" << leaf_after.node_updates << " peak=" << leaf_after.peak_tracked << "\n";
    CE_CHECK(leaf_after.total_events == 3);
    CE_CHECK(leaf_after.leaf_updates == 2);
    CE_CHECK(leaf_after.node_updates == 0); // <- leaf-only verwirft Inner-Touch
    CE_CHECK(leaf_after.peak_tracked == 2);
    CE_CHECK(!(leaf_after == leaf_before)); // Delta > 0 (kein Stub)

    // ── non-leaf-Strategie: Inner-Touch wird GEZAEHLT ─────────────────────────────────────────────────
    NodeTel node;
    node.record_node_touch(true);  // Blatt
    node.record_node_touch(false); // Inner -> non-leaf zaehlt
    node.record_node_touch(true);  // Blatt
    auto const node_after = node.statistics();
    std::cout << "InsertCounter   : total=" << node_after.total_events << " leaf=" << node_after.leaf_updates
              << " node=" << node_after.node_updates << " peak=" << node_after.peak_tracked << "\n";
    CE_CHECK(node_after.total_events == 3);
    CE_CHECK(node_after.leaf_updates == 2);
    CE_CHECK(node_after.node_updates == 1); // <- non-leaf zaehlt Inner-Touch

    // (4) Der messbare Achsen-Unterschied: identische Touch-Folge, verschiedene node_updates.
    CE_CHECK(leaf_after.node_updates != node_after.node_updates);

    // (5) reset() = Statistik-Reset.
    leaf.reset();
    CE_CHECK(leaf.statistics().total_events == 0);

    std::cout << "OK: telemetry-Achse ist echte getriebene ObservableAxis; Strategie parametrisiert die Mechanik.\n";
    return 0;
}
