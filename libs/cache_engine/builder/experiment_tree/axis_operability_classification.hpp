#pragma once
// D8 / L-74c (2026-06-02) — R5.B-Operabilitäts-Klassifikation der 17 SearchAlgorithm-Kern-Achsen T0..T16.
//
// EHRLICHE Transparenz statt Fake-Observer (Memory feedback_no_success_marks_without_literal_output): NICHT jede
// der 17 Kern-Achsen T0..T16 wird real getrieben. Diese constexpr-Tabelle macht literal abrufbar, welche Achse heute (a) REAL
// OPERATIV gemessen ist, (b) operativ-FÄHIG (Organ + statistics vorhanden, Verdrahtung in tier_observe = Composition-
// Driver-Folge), (c) DESKRIPTOR (passive Build-/Compile-Konstante, kein Mess-Zustand). `observable_axis_count` im
// Cross-ABI-POD (observable_tier.hpp) zählt die real getriebenen — diese Tabelle ist die ehrliche Grund-Klassifikation.
//
// Quelle: Doc 28 §1 (Observer-SOLL je Achse) + node_value_measurement.hpp R5.B-Grenze + abi_adapter.hpp tier_observe.
// Header-only, leichtgewichtig (kein Umbrella).

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// R5.B-Operabilität einer SearchAlgorithm-Kern-Achse (T0..T16).
enum class AxisOperability {
    Operative        = 0, ///< heute REAL getrieben + im Cross-ABI-POD beobachtet (search_algo, allocator)
    OperativeCapable = 1, ///< Organ + statistics vorhanden; Verdrahtung in tier_observe = Composition-Driver-Folge
    Descriptor       = 2  ///< passive Build-/Compile-Konstante bzw. V42 — kein Laufzeit-Mess-Zustand (ehrlich)
};

struct AxisOperabilityEntry {
    std::string_view axis;
    AxisOperability  operability;
    std::string_view note;
};

/// Die 17 Kern-Achsen T0..T16 (Reihenfolge = composition_factory/axis_path_serialization; queuing_q1/q2 @ T17/T18 separat)
/// mit ehrlichem R5.B-Status. 2 Operative + 4 OperativeCapable + 11 Descriptor == 17 (kein Achsen-Wegschrumpfen).
inline constexpr std::array<AxisOperabilityEntry, 17> kAxisOperability = {{
    {"search_algo", AxisOperability::Operative, "real getrieben (SearchAlgoStatistics im POD)"},
    {"cache_traversal", AxisOperability::Descriptor, "observable-faehig; nicht in tier_observe verdrahtet"},
    {"mapping", AxisOperability::Descriptor, "snapshot vorhanden; passiv"},
    {"path_compression", AxisOperability::Descriptor, "passiv (Default 0)"},
    {"node_type", AxisOperability::OperativeCapable, "KF-6 Run-Body-Divergenz; node-Observer verdrahtbar"},
    {"memory_layout", AxisOperability::OperativeCapable,
     "scan_field_sum runtime-operativ; ABER wall-clock sub-noise -> PMC (R5.D)"},
    {"allocator", AxisOperability::Operative, "real getrieben (AllocationStatistics im POD); Goldstandard"},
    {"prefetch", AxisOperability::Descriptor, "passiv"},
    {"concurrency", AxisOperability::Descriptor, "passiv"},
    {"serialization", AxisOperability::OperativeCapable, "serialize_scan runtime-operativ; verdrahtbar"},
    {"telemetry", AxisOperability::OperativeCapable, "statistics-tragend (Density/Insert/Latency); verdrahtbar"},
    {"value_handle", AxisOperability::Descriptor, "passiv"},
    {"isa", AxisOperability::Descriptor, "Build-Konstante (Definition)"},
    {"index_organization", AxisOperability::Descriptor, "passiv"},
    {"io_dispatch", AxisOperability::Descriptor, "passiv"},
    {"migration_policy", AxisOperability::Descriptor, "passiv"},
    {"filter", AxisOperability::Descriptor, "passiv"},
}};

[[nodiscard]] constexpr std::size_t count_operability(AxisOperability k) noexcept {
    std::size_t n = 0;
    for (auto const& e : kAxisOperability)
        if (e.operability == k) ++n;
    return n;
}
/// Heute real getriebene Achsen (== observable_axis_count-Untergrenze; ehrlich, kein Fake).
[[nodiscard]] constexpr std::size_t operative_axis_count() noexcept {
    return count_operability(AxisOperability::Operative);
}

} // namespace comdare::cache_engine::builder::experiment
