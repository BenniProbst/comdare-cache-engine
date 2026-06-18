#pragma once
// STRANG A KORRIGIERT — Increment 1 (2026-06-18, profil-getrieben). profile_runner: die EINE fehlende NAHT.
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S3-Kern).
// SOLL (Doc architektur/10_schichten_modell_M.md §2.2): die Diplomarbeit konfiguriert die Messung ueber ein
// DEKLARATIVES comdare_thesis_profile; der CacheEngineBuilder fuehrt aus. Heute ist die offizielle Kette
// (ThesisProfile → parse_thesis_profile → build_axis_levels → tree.build) VORHANDEN, aber im Lauf-Treiber
// ORPHANED — die Produktiv-Harness selektiert hartkodiert ueber build_pilot_levels<FullPilot>.
//
// DIESER HEADER zieht GENAU diese fehlende Naht — ADDITIV (PilotAxes/SelectMode bleiben unangetastet daneben):
//   COMDARE_THESIS_PROFILE → parse_thesis_profile → build_axis_levels(tp, mode, registry) → AxisLevels
//   (+ DynamicDims aus runtime_dynamic) → tree.build(levels) → run_lazy_static_then_dynamic (UNVERAENDERT).
//
// HARTE ROUND-TRIP-GATE (Plan-Risiko #1, Resume #139): die ueber build_axis_levels(Profil) erzeugten binary_ids
// MUESSEN EXAKT IDENTISCH sein zu denen aus build_pilot_levels<SmallPilot/FullPilot> (gleiche 19 Slot-Namen-
// Reihenfolge + gleiche W::name()-Werte + serialize_composition_path-Format). Belegt LITERAL in
// test_profile_roundtrip.cpp (StaticBinaryView-binary_id-Listen-Diff == leer). Die binary_id nutzt NUR die
// STATISCHEN Ebenen (axis=value); DynamicDims aendern sie NICHT.
//
// LEITPLANKEN: nichts entfernen/degradieren; Lazy-Compile (1 DLL=1 TU) bleibt (SourceGen unveraendert vom
// Caller injiziert); Two-Phasen-Warmup intakt (liegt im perm_runner). C++23, header-only.

#include <builder/experiment_tree/experiment_tree.hpp>      // AxisLevel / DynamicDim / ExperimentTree / StaticBinaryView
#include <builder/experiment_tree/profile_to_tree.hpp>       // build_axis_levels (die offizielle Bruecke)
#include "xml_config_parser/xml_config_parser.hpp"           // ThesisProfile / XmlConfigParser::parse_thesis_profile

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cx = ::comdare::builder::xml;

/// build_profile_levels(tp, mode) — der GESAMT-Level-Satz fuer ExperimentTree::build, AUSSCHLIESSLICH aus der
/// offiziellen Bruecke ex::build_axis_levels (Inc2 dyn-Dim-Konsolidierung, 2026-06-18): build_axis_levels emittiert
/// die runtime_dynamic-Dimensionen (concurrency.thread_count / prefetch.hw_prefetcher / repetition.repetition_index)
/// jetzt SELBST als is_static=false-Ebenen — die fruehere Doppelquelle (profile_runner haengt sie ein ZWEITES Mal an)
/// ist ENTFERNT. `with_dynamic=false` ⇒ es werden nur die STATISCHEN Ebenen zurueckgegeben (die dynamischen werden
/// herausgefiltert) = die reine binary_id-Quelle fuer die Round-Trip-Gate. Die AxisRegistry expandiert leere
/// <axis>-Listen (hier leer ⇒ die explizit deklarierten <value> aus dem Profil gewinnen → Pilot-Identitaet).
[[nodiscard]] inline std::vector<ex::AxisLevel> build_profile_levels(
        cx::ThesisProfile const& tp, std::string const& mode_name,
        bool with_dynamic = true, ex::AxisRegistry const& registry = {}) {
    std::vector<ex::AxisLevel> lv = ex::build_axis_levels(tp, mode_name, registry);  // EINZIGE Quelle (statisch+dyn)
    if (with_dynamic) return lv;
    std::vector<ex::AxisLevel> static_only;
    static_only.reserve(lv.size());
    for (auto& l : lv) if (l.is_static) static_only.push_back(std::move(l));   // dyn-Dims herausfiltern
    return static_only;
}

/// load_thesis_profile — kleiner Convenience-Wrapper um den offiziellen Parser (KF-1). nullopt bei Fehler.
[[nodiscard]] inline std::optional<cx::ThesisProfile> load_thesis_profile(std::filesystem::path const& xml) {
    cx::XmlConfigParser const parser;
    return parser.parse_thesis_profile(xml);
}

}  // namespace comdare::cache_engine::thesis_lazy
