#pragma once
// BR-3-OBS-22 (2026-06-02, Doc 27 §0.1/§3) — Observer-Klassifikation je der 22 Achsen ("kein Wegschrumpfen").
//
// User-Direktive 2026-06-02: ALLE 22 Achsen tragen einen EIGENEN Observer — NICHT nur die 17 SearchAlgorithm-
// Komposition-Slots. Die Differenzierung ist GATTUNGS-KORREKT (Doc 27 §0.1, User-Entscheidung „differenziert"):
//   • SearchAlgorithmObserver : die 17 Komposition-Achsen → ObserverAggregate<17> (real für ObservableAxis,
//     R5.B: search_algo + allocator + ... operativ; Rest Default-Snapshot). Träger: NodeObserverSnapshot (BR-3).
//   • DefinitionOnly          : die Hardware-/Sub-Achsen page_type(01)/simd_extension(09b)/general_hardware(12)
//     sind Build-Zeit-KONSTANTEN (kein Laufzeit-Zustand) → sie tragen eine read-only Achsen-DEFINITION
//     (Wrapper-Identität/Properties), KEINEN Laufzeit-Observer. EHRLICH dokumentiert (nicht stillschweigend 0).
//   • ContainerObserver       : queuing q1/q2 = eigene Container-GATTUNG (Cross-Genus type-unmöglich, Doku 14 §32)
//     → eigener Container-Gattungs-Observer (eigenes Container-Prüf-Dock; Bau-Brücke = Gattungs-Generik-Folgeschritt).
//
// So ist jede der 22 Achsen klassifiziert + trägt Observer ODER Definition — keine fällt weg. C++23, header-only,
// umbrella-UNABHÄNGIG (nur Namen + Kind; die Definitionen liefert BR-1 build_all_axis_levels via reflect_names).

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// Die drei Observer-Naturen der 22 Achsen (gattungs-korrekt, Doc 27 §0.1).
enum class AxisObserverKind {
    SearchAlgorithmObserver,  // 17 Komposition-Achsen: ObserverAggregate<17> (BR-3)
    DefinitionOnly,           // page_type/09b/12: Build-Konstanten → Definition statt Laufzeit-Observer
    ContainerObserver         // queuing q1/q2: eigene Container-Gattung → eigener Observer (eigenes Dock)
};

[[nodiscard]] inline constexpr std::string_view observer_kind_name(AxisObserverKind k) noexcept {
    switch (k) {
        case AxisObserverKind::SearchAlgorithmObserver: return "SearchAlgorithmObserver";
        case AxisObserverKind::DefinitionOnly:          return "DefinitionOnly";
        case AxisObserverKind::ContainerObserver:       return "ContainerObserver";
    }
    return "?";
}

struct AxisObserverClass { std::string_view axis; AxisObserverKind kind; };

/// ALLE 22 Achsen klassifiziert (Reihenfolge = registry_to_axis_levels build_all_axis_levels: 17 Komposition, dann 5 außerhalb).
inline constexpr std::array<AxisObserverClass, 22> kAxisObserverClasses = {{
    {"search_algo",        AxisObserverKind::SearchAlgorithmObserver},
    {"cache_traversal",    AxisObserverKind::SearchAlgorithmObserver},
    {"mapping",            AxisObserverKind::SearchAlgorithmObserver},
    {"path_compression",   AxisObserverKind::SearchAlgorithmObserver},
    {"node_type",          AxisObserverKind::SearchAlgorithmObserver},
    {"memory_layout",      AxisObserverKind::SearchAlgorithmObserver},
    {"allocator",          AxisObserverKind::SearchAlgorithmObserver},
    {"prefetch",           AxisObserverKind::SearchAlgorithmObserver},
    {"concurrency",        AxisObserverKind::SearchAlgorithmObserver},
    {"serialization",      AxisObserverKind::SearchAlgorithmObserver},
    {"telemetry",          AxisObserverKind::SearchAlgorithmObserver},
    {"value_handle",       AxisObserverKind::SearchAlgorithmObserver},
    {"isa",                AxisObserverKind::SearchAlgorithmObserver},
    {"index_organization", AxisObserverKind::SearchAlgorithmObserver},
    {"io_dispatch",        AxisObserverKind::SearchAlgorithmObserver},
    {"migration_policy",   AxisObserverKind::SearchAlgorithmObserver},
    {"filter",             AxisObserverKind::SearchAlgorithmObserver},
    // ── die 5 AUSSERHALB der SearchAlgorithm-17-Komposition ──
    {"page_type",          AxisObserverKind::DefinitionOnly},      // nodes-Sub, Build-Variante
    {"simd_extension",     AxisObserverKind::DefinitionOnly},      // 09b Hardware, Build-Konstante
    {"general_hardware",   AxisObserverKind::DefinitionOnly},      // 12 Hardware, Build-Konstante
    {"queuing_q1",         AxisObserverKind::ContainerObserver},   // q1 Container-Gattung
    {"queuing_q2",         AxisObserverKind::ContainerObserver},   // q2 Container-Gattung
}};

/// Observer-Kind einer Achse (per Name). Liefert SearchAlgorithmObserver als Default (für die 17), aber der
/// Lookup deckt alle 22 ab; unbekannte Achse → false über found.
[[nodiscard]] inline constexpr bool observer_kind_of(std::string_view axis, AxisObserverKind& out) noexcept {
    for (auto const& e : kAxisObserverClasses) if (e.axis == axis) { out = e.kind; return true; }
    return false;
}

/// Anzahl Achsen je Observer-Natur (Diagnose: 17 / 3 / 2 = 22).
[[nodiscard]] inline constexpr std::size_t count_observer_kind(AxisObserverKind k) noexcept {
    std::size_t n = 0;
    for (auto const& e : kAxisObserverClasses) if (e.kind == k) ++n;
    return n;
}

}  // namespace comdare::cache_engine::builder::experiment
