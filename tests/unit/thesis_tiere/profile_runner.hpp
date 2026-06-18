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

/// Die DYNAMISCHEN Dimensionen aus runtime_dynamic des Profils (= virtuelle for-Schleifen auf der GELADENEN DLL,
/// NICHT teil der binary_id). thread_count → concurrency.thread_count; hw_prefetcher → prefetch.hw_prefetcher.
/// PLUS die Wiederholungs-Achse (D, KF-10): repetition.repetition_index ∈ {0..repetitions-1}. KEIN POD-Feld →
/// set_field ignoriert es (architektonische Ausnahme), es multipliziert nur die Mess-Wiederholungen.
[[nodiscard]] inline std::vector<ex::DynamicDim> profile_dynamic_dims(cx::ThesisProfile const& tp) {
    std::vector<ex::DynamicDim> dims;
    if (!tp.thread_counts.empty())
        dims.push_back(ex::DynamicDim{"concurrency", "thread_count", tp.thread_counts, "concurrency"});
    if (!tp.hw_prefetcher.empty())
        dims.push_back(ex::DynamicDim{"prefetch", "hw_prefetcher", tp.hw_prefetcher, "prefetch"});
    std::uint32_t const reps = (tp.repetitions <= 0) ? 1u : static_cast<std::uint32_t>(tp.repetitions);
    std::vector<std::string> rep_vals;
    rep_vals.reserve(reps);
    for (std::uint32_t r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
    dims.push_back(ex::DynamicDim{"repetition", "repetition_index", std::move(rep_vals), "repetition"});
    return dims;
}

/// build_profile_levels(tp, mode) — der GESAMT-Level-Satz (statische Ebenen via build_axis_levels + dynamische
/// Dimensionen) fuer ExperimentTree::build. `with_dynamic=false` ⇒ nur die statischen Ebenen (= die reine
/// binary_id-Quelle fuer die Round-Trip-Gate). Die AxisRegistry expandiert leere <axis>-Listen (hier leer ⇒ die
/// explizit deklarierten <value> aus dem Basis-Profil gewinnen, was die Pilot-Identitaet garantiert).
[[nodiscard]] inline std::vector<ex::AxisLevel> build_profile_levels(
        cx::ThesisProfile const& tp, std::string const& mode_name,
        bool with_dynamic = true, ex::AxisRegistry const& registry = {}) {
    std::vector<ex::AxisLevel> lv = ex::build_axis_levels(tp, mode_name, registry);  // OFFIZIELLE Bruecke
    if (with_dynamic)
        for (auto const& d : profile_dynamic_dims(tp))
            lv.push_back(ex::AxisLevel{d.axis, d.values, /*is_static=*/false, d.variable, d.block_id});
    return lv;
}

/// load_thesis_profile — kleiner Convenience-Wrapper um den offiziellen Parser (KF-1). nullopt bei Fehler.
[[nodiscard]] inline std::optional<cx::ThesisProfile> load_thesis_profile(std::filesystem::path const& xml) {
    cx::XmlConfigParser const parser;
    return parser.parse_thesis_profile(xml);
}

}  // namespace comdare::cache_engine::thesis_lazy
