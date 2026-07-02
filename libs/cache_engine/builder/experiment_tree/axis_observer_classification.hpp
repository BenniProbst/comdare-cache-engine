#pragma once
// BR-3-OBS-22 (2026-06-02, Doc 27 §0.1/§3) — Observer-Klassifikation je der 26 Achsen ("kein Wegschrumpfen").
//
// User-Direktive 2026-06-02: ALLE 26 Achsen tragen einen EIGENEN Observer — NICHT nur die 19 SearchAlgorithm-
// Komposition-Slots. Die Differenzierung ist GATTUNGS-KORREKT (Doc 27 §0.1, User-Entscheidung „differenziert"):
//   • SearchAlgorithmObserver : die 19 Komposition-Achsen → ObserverAggregate<19> (real für ObservableAxis,
//     R5.B: search_algo + allocator + ... operativ; Rest Default-Snapshot). Träger: NodeObserverSnapshot (BR-3).
//     korr. 2026-06-03 (Doc 30 §8.0): umfasst jetzt AUCH queuing q1/q2 (Slots T17/T18) — reguläre SA-Tier-Unterklasse-Achsen.
//   • DefinitionOnly          : die Hardware-/Sub-Achsen page_type(01)/simd_extension(09b)/general_hardware(12)
//     sind Build-Zeit-KONSTANTEN (kein Laufzeit-Zustand) → sie tragen eine read-only Achsen-DEFINITION
//     (Wrapper-Identität/Properties), KEINEN Laufzeit-Observer. EHRLICH dokumentiert (nicht stillschweigend 0).
//   • ContainerObserver       : RESERVIERT für die ECHTE Container-Gattung (std::queue/stack/priority_queue =
//     Adapter-Tier-Unterklasse, 13 Achsen inkl. inner_container, §28, #87+#90) — NICHT für queuing (das war der
//     korrigierte Kategorienfehler, Doc 30 §8.0). Aktuell 0 Einträge.
//
// So ist jede der 26 Achsen klassifiziert + trägt Observer ODER Definition — keine fällt weg. C++23, header-only,
// umbrella-UNABHÄNGIG (nur Namen + Kind; die Definitionen liefert BR-1 build_all_axis_levels via reflect_names).

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// Die drei Observer-Naturen der 26 Achsen (gattungs-korrekt, Doc 27 §0.1).
enum class AxisObserverKind {
    SearchAlgorithmObserver, // 19 Komposition-Achsen (inkl. queuing q1/q2 T17/T18): ObserverAggregate<19> (BR-3)
    DefinitionOnly,   // page_type/09b/12 + 4 node-shape (#234-K): Build-Konstanten → Definition statt Laufzeit-Observer
    ContainerObserver // RESERVIERT: echte Container-Gattung (Adapter, 13 Achsen inkl. inner_container, §28, #87+#90) — NICHT queuing (korr. 2026-06-03)
};

[[nodiscard]] inline constexpr std::string_view observer_kind_name(AxisObserverKind k) noexcept {
    switch (k) {
        case AxisObserverKind::SearchAlgorithmObserver: return "SearchAlgorithmObserver";
        case AxisObserverKind::DefinitionOnly: return "DefinitionOnly";
        case AxisObserverKind::ContainerObserver: return "ContainerObserver";
    }
    return "?";
}

struct AxisObserverClass {
    std::string_view axis;
    AxisObserverKind kind;
};

/// ALLE 26 Achsen klassifiziert (Reihenfolge = registry_to_axis_levels build_all_axis_levels: 17 Kern-Achsen, dann 3 build-only + q1/q2 = 19 Komposition, dann 4 node-shape (#234-K) → 7 DefinitionOnly gesamt).
inline constexpr std::array<AxisObserverClass, 26> kAxisObserverClasses = {{
    {"search_algo", AxisObserverKind::SearchAlgorithmObserver},
    {"cache_traversal", AxisObserverKind::SearchAlgorithmObserver},
    {"mapping", AxisObserverKind::SearchAlgorithmObserver},
    {"path_compression", AxisObserverKind::SearchAlgorithmObserver},
    {"node_type", AxisObserverKind::SearchAlgorithmObserver},
    {"memory_layout", AxisObserverKind::SearchAlgorithmObserver},
    {"allocator", AxisObserverKind::SearchAlgorithmObserver},
    {"prefetch", AxisObserverKind::SearchAlgorithmObserver},
    {"concurrency", AxisObserverKind::SearchAlgorithmObserver},
    {"serialization", AxisObserverKind::SearchAlgorithmObserver},
    {"telemetry", AxisObserverKind::SearchAlgorithmObserver},
    {"value_handle", AxisObserverKind::SearchAlgorithmObserver},
    {"isa", AxisObserverKind::SearchAlgorithmObserver},
    {"index_organization", AxisObserverKind::SearchAlgorithmObserver},
    {"io_dispatch", AxisObserverKind::SearchAlgorithmObserver},
    {"migration_policy", AxisObserverKind::SearchAlgorithmObserver},
    {"filter", AxisObserverKind::SearchAlgorithmObserver},
    // ── die 3 Build-Achsen (DefinitionOnly) + queuing q1/q2 (korr. 2026-06-03: jetzt Komposition-Slots T17/T18) ──
    {"page_type", AxisObserverKind::DefinitionOnly},        // nodes-Sub, Build-Variante
    {"simd_extension", AxisObserverKind::DefinitionOnly},   // 09b Hardware, Build-Konstante
    {"general_hardware", AxisObserverKind::DefinitionOnly}, // 12 Hardware, Build-Konstante
    {"queuing_q1",
     AxisObserverKind::
         SearchAlgorithmObserver}, // korr. 2026-06-03 (Doc 30 §8.0): SA-Tier-Unterklasse-Achse T17 (buffer_strategy) — KEINE Gattung
    {"queuing_q2",
     AxisObserverKind::
         SearchAlgorithmObserver}, // korr. 2026-06-03 (Doc 30 §8.0): SA-Tier-Unterklasse-Achse T18 (flush_policy) — KEINE Gattung
    {"btree_order", AxisObserverKind::DefinitionOnly},
    {"skip_list_shape", AxisObserverKind::DefinitionOnly},
    {"bst_shape", AxisObserverKind::DefinitionOnly},
    {"hash_probe_shape", AxisObserverKind::DefinitionOnly},
}};

/// Observer-Kind einer Achse (per Name). Liefert SearchAlgorithmObserver als Default (für die 17), aber der
/// Lookup deckt alle 26 ab; unbekannte Achse → false über found.
[[nodiscard]] inline constexpr bool observer_kind_of(std::string_view axis, AxisObserverKind& out) noexcept {
    for (auto const& e : kAxisObserverClasses)
        if (e.axis == axis) {
            out = e.kind;
            return true;
        }
    return false;
}

/// Anzahl Achsen je Observer-Natur (Diagnose: 19 / 7 / 0 = 26 — test-belegt br3_obs22/d7b).
[[nodiscard]] inline constexpr std::size_t count_observer_kind(AxisObserverKind k) noexcept {
    std::size_t n = 0;
    for (auto const& e : kAxisObserverClasses)
        if (e.kind == k) ++n;
    return n;
}

} // namespace comdare::cache_engine::builder::experiment
