// abi/anatomy_version_stamp.hpp -- compile-time-Ableitung der Organ-Stempel-Zeile aus einer Composition
// (Bau W12-A, Section 43, Inkrement 4-Wiring).
//
// Section 43: die Tier-Binary traegt ihre kOrganAxisVersionLine einkompiliert. Diese Zeile wird aus den 17
// Kompositions-Achsen-Typen der AdHocComposition abgeleitet (jede exponiert name() + algo_version), in
// kanonischer compose-Ordnung (Entscheid W12-A-5). NUR Haupt-Achsen (Section 42.b).
//
// BYTE-SICHER (Round-Trip-Wache): Dieser Helfer wird IM Makro COMDARE_DEFINE_ANATOMY_MODULE aus dem
// Composition-Typ aufgerufen -- der emittierte .cpp-QUELLTEXT bleibt unveraendert (weiterhin nur
// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<typen>)), die Emitter-Round-Trip-Byte-Wache (test_lazy_adhoc_source_gen)
// bleibt gruen. Der Stempel lebt im kompilierten Binary. SEPARATE Welt zur .algos-Sig (X.Y.Z-Voll-Form).

#pragma once

#include <cache_engine/measurement/axis_version_stamp.hpp> // AxisVersionEntry + build_axis_version_stamp_line

#include <array>
#include <string>

namespace comdare::cache_engine::abi {

/// organ_stamp_line<Comp>() -- die kOrganAxisVersionLine "achse=algo@X.Y.Z;..." aus den 17 benannten
/// Achsen-Aliassen einer Composition, in kanonischer compose-Ordnung (== AdHocComposition-Alias-Ordnung
/// == kCompositionAxisNames). Jede Achse muss name() + algo_version tragen.
///
/// BLOCKER (W12-A, Live-Code-Befund): die REALEN AdHocComposition-Achsen-Typen sind STRATEGIE-Typen
/// (z.B. ObservableComposedContainer<...>) und tragen KEIN name()/algo_version -- nur die Registry-WRAPPER
/// (StaticAxisVariants_*) tragen sie. Diese Funktion ist daher (noch) NICHT auf reale Module anwendbar; sie
/// beweist die Format-/Ordnungs-Logik gegen name()/algo_version-tragende Typen (Test: Mock-Composition). Die
/// Metadaten-Quelle fuer den realen Modul-Stempel ist eine offene Architektur-Frage (Registry-basiert im
/// Emitter vs. Wrapper-Typ-Liste durchs Makro) -- beide beruehren die Emitter-Round-Trip-Byte-Wache.
template <class Comp>
[[nodiscard]] inline std::string organ_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::array<AxisVersionEntry, 17> const entries{{
        {"search_algo", Comp::search_algo::name(), Comp::search_algo::algo_version},
        {"cache_traversal", Comp::cache_traversal::name(), Comp::cache_traversal::algo_version},
        {"mapping", Comp::mapping::name(), Comp::mapping::algo_version},
        {"path_compression", Comp::path_compression::name(), Comp::path_compression::algo_version},
        {"node_type", Comp::node_type::name(), Comp::node_type::algo_version},
        {"memory_layout", Comp::memory_layout::name(), Comp::memory_layout::algo_version},
        {"allocator", Comp::allocator::name(), Comp::allocator::algo_version},
        {"prefetch", Comp::prefetch::name(), Comp::prefetch::algo_version},
        {"concurrency", Comp::concurrency::name(), Comp::concurrency::algo_version},
        {"serialization", Comp::serialization::name(), Comp::serialization::algo_version},
        {"value_handle", Comp::value_handle::name(), Comp::value_handle::algo_version},
        {"index_organization", Comp::index_organization::name(), Comp::index_organization::algo_version},
        {"io_dispatch", Comp::io_dispatch::name(), Comp::io_dispatch::algo_version},
        {"migration_policy", Comp::migration_policy::name(), Comp::migration_policy::algo_version},
        {"filter", Comp::filter::name(), Comp::filter::algo_version},
        {"queuing_q1", Comp::queuing_q1::name(), Comp::queuing_q1::algo_version},
        {"queuing_q2", Comp::queuing_q2::name(), Comp::queuing_q2::algo_version},
    }};
    return build_axis_version_stamp_line(entries);
}

/// system_stamp_line() -- die kSystemAxisVersionLine (Section 43, Entscheid W12-A-1). ZWEIPHASIG dokumentiert:
/// HEUTE traegt sie die STATISCHEN System-Achsen-ALGORITHMUS-Versionen (Compiler-/SIMD-/Scheduling-Achsen-
/// Code-Version), NICHT die gewaehlten System-Zellwerte -- der Tier-Emitter ist system-blind (W4-B-Invariante),
/// und die Zellwerte existieren bereits als Provenienz im .version-Sidecar. W10-ANSCHLUSS: die CEB-Naht
/// (perm_compile kennt die Zelle) ergaenzt die Zellwerte via Compile-Define -> DANN ist die Zeile komplett,
/// ohne den Emitter zu entblinden. Format identisch zur Organ-Zeile ("achse=algo@X.Y.Z"); Algorithmus-Marker
/// "code" = die statische Code-Identitaet der System-Achse.
[[nodiscard]] inline std::string system_stamp_line() {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::array<AxisVersionEntry, 5> const entries{{
        {"compiler", "code", "v1"},
        {"extension_hardware", "code", "v1"},
        {"target_isa", "code", "v1"},
        {"scheduling", "code", "v1"},
        {"load_framework", "code", "v1"},
    }};
    return build_axis_version_stamp_line(entries);
}

/// measurement_stamp_line(tooling) -- die kMeasurementAxisVersionLine (Section 43, Section 47: Mess-Tooling == HAUPT,
/// W12-A3). Traegt GENAU die gewaehlte Mess-Tooling-HAUPT-Wahl {wallclock/macro/micro} (die collector-Achse,
/// Plan-D1: von binary_id="never" zur Fan-out-HAUPT promotet, binary_id-relevant NUR ueber diesen Version-Line-
/// Stempel) als EINEN Eintrag "measurement_tooling=<tooling>@X.Y.Z". Analog zu system_stamp_line(): dieselbe
/// AxisVersionEntry/build_axis_version_stamp_line-Welt, dieselbe X.Y.Z-Voll-Form (SEPARATE Welt zur .algos-Sig).
///
/// Section-43-INVARIANTE: NUR die Haupt-Achse. Die Ablaufmethodik (run_methodology debug/measure/release) und die
/// Workloads/Framework (ycsb_*) sind UNTER-Achsen (Laufzeit-Sweep) und NIE Stempel-Bestandteil. Der Algorithmus-
/// Marker == die gewaehlte Tooling-id; die Version == die statische Code-Identitaet der Mess-Tooling-Achse
/// ("v1" -> 1.0.0). Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert; die Makro-
/// Materialisierung legt dann measurement_line auf "" mit measurement_len==0).
[[nodiscard]] inline std::string measurement_stamp_line(std::string_view tooling) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    if (tooling.empty()) return {};
    std::array<AxisVersionEntry, 1> const entries{{
        {"measurement_tooling", tooling, "v1"},
    }};
    return build_axis_version_stamp_line(entries);
}

} // namespace comdare::cache_engine::abi
