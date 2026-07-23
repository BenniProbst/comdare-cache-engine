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

#include <cache_engine/abi/system_axis_code_versions.hpp>  // A2 (G2-4): kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/axis_version_stamp.hpp> // AxisVersionEntry + build_axis_version_stamp_line
#include <cache_engine/measurement/measurement_tooling_registry.hpp> // K7b-2: kMeasurementToolingRegistry (Vollmenge)

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
    // A2 (G2-4 Schritt 3): die 5 Achsen + Code-Versionen aus der Single-Source system_axis_code_versions.hpp (frueher
    // 5x hartkodiert {"<achse>","code","v1"}); "code" bleibt der Achsen-Marker, die Version ist je Achse bump-bar.
    // Render-neutral: "v1.0.0" -> "1.0.0" wie zuvor "v1" -> "1.0.0".
    std::array<AxisVersionEntry, kSystemAxisCodeCount> entries{};
    for (std::size_t i = 0; i < kSystemAxisCodeCount; ++i)
        entries[i] = {kSystemAxisCodeVersions[i].axis, "code", kSystemAxisCodeVersions[i].version};
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
    // A2 (G2-4 Schritt 4): die Code-Version aus der Tooling-Registry (Lookup per id) statt der "v1"-Hartkodierung;
    // bekannte id -> "v1.0.0" (render-neutral zu "1.0.0"), unbekannte id -> "v0"-Sentinel (@0.0.0, nur ungueltige ids).
    std::array<AxisVersionEntry, 1> const entries{{
        {"measurement_tooling", tooling, ::comdare::cache_engine::measurement::tooling_version_for_id(tooling)},
    }};
    return build_axis_version_stamp_line(entries);
}

/// measurement_stamp_line(toolings) -- K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form der
/// kMeasurementAxisVersionLine. Statt EINER Tooling-Wahl traegt die Zeile die MENGE der gewaehlten Mess-Tools als N
/// Eintraege "measurement_tooling=<t>@1.0.0" (';'-getrennt, Eingabe-Reihenfolge; Section-64-Vollmengen-Provenienz).
/// Leere Tokens werden uebersprungen; leere/leer-gefilterte Menge -> leere Zeile. Dieselbe X.Y.Z-Voll-Form / SEPARATE
/// Welt zur .algos-Sig wie die Einzel-Form (build_axis_version_stamp_line). binary_id-NEUTRAL (Mess-Achse
/// binary_id="never" -> der Stempel lebt nur im Version-Line/Binary, nie in der binary_id/CRC).
[[nodiscard]] inline std::string measurement_stamp_line(std::span<std::string_view const> toolings) {
    using ::comdare::cache_engine::measurement::AxisVersionEntry;
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    std::vector<AxisVersionEntry> entries;
    entries.reserve(toolings.size());
    for (std::string_view const t : toolings)
        if (!t.empty())
            // A2 (G2-4 Schritt 4): Code-Version per id-Lookup (Registry) statt "v1"-Hartkodierung; Sentinel "v0" fuer
            // unbekannte ids (render-neutral fuer die gueltigen wallclock/macro/micro).
            entries.push_back(
                {"measurement_tooling", t, ::comdare::cache_engine::measurement::tooling_version_for_id(t)});
    return build_axis_version_stamp_line(entries);
}

/// measurement_stamp_line_full_set() -- K7b-2 (Section 64-D1-B): die VOLLE Mess-Tooling-Vollmenge
/// {wallclock,macro,micro} (Single-Source kMeasurementToolingRegistry, Registry-Reihenfolge) als Mengen-Stempel. Das
/// ist der Section-64-[all]-Default: eine [all]-CEB misst mit dem vollen Tooling-Angebot -> ihre Provenienz traegt
/// ALLE Tools. Genau kMeasurementToolingCount Eintraege, immer non-empty (die Registry ist nie leer, static_assert).
[[nodiscard]] inline std::string measurement_stamp_line_full_set() {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    std::array<std::string_view, kMeasurementToolingCount> ids{};
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) ids[i] = kMeasurementToolingRegistry[i].id;
    return measurement_stamp_line(std::span<std::string_view const>{ids});
}

/// measurement_stamp_line_from_combo_legend(legend) -- die Mess-Tooling-Stempel-Zeile aus einer Planer-Combo-Legende
/// (S6-P1b Env-Bruecke: COMDARE_MEASUREMENT_COMBO traegt die vom Planer gewaehlte [a,b,c]-Legende, z.B. "[wallclock]").
/// LEER oder "[all]" (das volle Mess-System, kein Tooling-spezifischer Fan-out) -> "" (kein Stempel; der byte-stabile
/// Default-Pfad -> emittierte Quelltexte byte-identisch). Sonst: die inneren Tooling-ids (ohne die []-Klammern) als
/// Stempel-Tooling. So reist die gewaehlte Combo bis in die emittierte DLL-Source, ohne den Emitter zu entblinden.
[[nodiscard]] inline std::string measurement_stamp_line_from_combo_legend(std::string_view legend) {
    // Leere Legende = "keine Legende gereicht" -> byte-stabil leer (der from_env-UNGESETZT-Zweig entscheidet dort
    // ueber die Vollmengen-Default-Semantik, NICHT dieser reine Legenden-Parser).
    if (legend.empty()) return {};
    std::string_view inner = legend;
    if (inner.size() >= 2 && inner.front() == '[' && inner.back() == ']') inner = inner.substr(1, inner.size() - 2);
    // Section 64-D1-B (2026-07-22): [all] / leer-innen -> die VOLLE 3-Tool-Vollmenge (Vollmengen-Provenienz), NICHT
    // mehr "" (das war die Regression: die [all]-Lane emittierte leere Mess-Provenienz).
    if (inner.empty() || inner == "all") return measurement_stamp_line_full_set();
    // Sonst: die inneren Tooling-ids als MENGE (komma-getrennt, Eingabe-Reihenfolge) -> N-Eintrags-Stempel.
    std::vector<std::string_view> toks;
    for (std::size_t start = 0; start <= inner.size();) {
        std::size_t const comma = inner.find(',', start);
        std::size_t const end   = comma == std::string_view::npos ? inner.size() : comma;
        if (end > start) toks.push_back(inner.substr(start, end - start));
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return measurement_stamp_line(std::span<std::string_view const>{toks});
}

/// merge_stamp_line(strategy, pruefling, merged_axes) -- die kMergeAxisVersionLine (Section 59, K6a): der DRITTE
/// Tier-Binary-Stempel = die MERGE-KOMBINATION. Zusaetzlich zu den zwei Section-58-Arrays (System + Organ) traegt
/// die Tier-Binary damit die Merge-Art (die MergeStrategy: Stufe1/Stufe2/Stufe3) + die Pruefling-Identitaet + die
/// Namen/Versionen der beteiligten Achsen-Algorithmen (Section 59-C: "Namen + Versionen JEDER Achsen-Algorithmen
/// IMMER im Stempel; PLUS ein dritter Tier-Binary-Stempel = die Merge-Kombination"). Format:
///   "merge=<strategy>;pruefling=<pruefling>[;<axis>=<algo>@X.Y.Z;...]"
/// R6 (§59-A(2)/A(3), Nacht-Audit 2026-07-22): der Stempel traegt die Strategie VERBATIM -- Stufe2_Hybrid (merge-
/// Modus, CE+Pruefling-Hybrid je Pruefling) erzeugt damit einen ANDEREN Stempel als Stufe3_FullJoin (fulljoin-Modus,
/// kombinierte Union); die beiden Merge-Kategorien bleiben am 3. Tier-Stempel getrennt (ihre Vermischung war die
/// Regression, merge_plan.hpp::merge_mode_to_strategy).
/// -- dieselbe X.Y.Z-Voll-Form / SEPARATE Welt zur .algos-Sig wie organ/system/measurement (build_axis_version_
/// stamp_line fuer den Achsen-Teil). NUR HAUPT-Achsen (Section 58-V; Unter-Achsen fliessen dynamisch zur Laufzeit
/// durch).
///
/// ce-only-/IDENTITAETS-Fall -> LEERE Zeile (Section 59-C-golden-Konsequenz): leere/Stufe1_CeOnly-Strategie ODER
/// leere/"CacheEngine"/"self"-Pruefling-Identitaet -> "" (kein Merge einkompiliert). So bleibt der ce-only-/
/// Katalog-Pfad byte-identisch (die Makro-Materialisierung legt merge_line auf "" mit merge_len==0), der
/// golden-CRC 0xF1C1F26A1232073B unberuehrt -- die Merges sind ein additiver id-Satz.
[[nodiscard]] inline std::string
merge_stamp_line(std::string_view strategy, std::string_view pruefling,
                 std::span<::comdare::cache_engine::measurement::AxisVersionEntry const> merged_axes = {}) {
    using ::comdare::cache_engine::measurement::build_axis_version_stamp_line;
    // ce-only (Stufe1 / keine Merge-Art) -> leer (byte-identischer golden-Pfad).
    if (strategy.empty() || strategy == "Stufe1_CeOnly") return {};
    // Identitaets-/self-Pruefling ("CacheEngine"/self / kein Pruefling) -> leer (Fork 3: identity=self ist ce).
    if (pruefling.empty() || pruefling == "CacheEngine" || pruefling == "self") return {};
    std::string out = "merge=";
    out += strategy;
    out += ";pruefling=";
    out += pruefling;
    std::string const axes = build_axis_version_stamp_line(merged_axes);
    if (!axes.empty()) {
        out += ';';
        out += axes;
    }
    return out;
}

} // namespace comdare::cache_engine::abi
