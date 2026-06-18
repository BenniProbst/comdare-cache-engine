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
#include <builder/experiment_tree/coverage_selection.hpp>    // BuildSelection / select_explicit
#include "xml_config_parser/xml_config_parser.hpp"           // ThesisProfile / XmlConfigParser::parse_thesis_profile

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

// ════════════════════════════════════════════════════════════════════════════════════════════════════════════
// STRANG A KORRIGIERT — Increment 3 (S3-complete, 2026-06-18). DER TREIBER KONSUMIERT die 4 neuen Profil-Felder.
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (Inc 3). Diese Helfer machen den
// profil-getriebenen Lauf (run_lazy_150 im PROFIL-MODUS) FUNKTIONAL identisch zum heutigen Code-Lauf
// (PS-foreach + SelectMode + COMDARE_*-env): working_set_sweep ersetzt die PS-foreach + COMDARE_WORKLOAD_RECORDS;
// axis_sweeps ersetzt SelectMode "axis_sweep:<a>" + axis_to_level-Map; run_options ersetzt argv/env; die
// tier-Level-Behandlung haelt den binary_id-Raum stabil (Basis-320 == FullPilot, Resume #139).
//
// LEITPLANKE: NICHTS aus der Code-Selektion (PilotAxes/SelectMode/m3v2_select_profile) wird ENTFERNT (das ist
// Inc4/S5); diese Helfer stehen ADDITIV daneben. sota_series wird HIER NICHT konsumiert (Inc5/#162).
// ════════════════════════════════════════════════════════════════════════════════════════════════════════════

// ── tier-Level-Behandlung (Basis-320): build_axis_levels emittiert ein "tier"-Level OBEN (SOTA-Reihen-Dimension
//    aus base_tiers). Fuer den reinen 4-Achsen-Permutations-Lauf (Basis-320) zieht der Treiber diese Ebene ab —
//    so erzeugt der StaticBinaryView DENSELBEN binary_id-Raum wie build_pilot_levels<FullPilot> (KEIN tier-Level).
//    Round-Trip-belegt in test_profile_roundtrip.cpp Abschnitt (6) (drop_tier → Mismatch 0). Single-Source. ──
[[nodiscard]] inline std::vector<ex::AxisLevel> drop_tier_level(std::vector<ex::AxisLevel> levels) {
    std::vector<ex::AxisLevel> out;
    out.reserve(levels.size());
    for (auto& l : levels) if (l.axis != "tier") out.push_back(std::move(l));
    return out;
}

/// build_profile_basis_levels — der Gesamt-Level-Satz fuer den BASIS-320-Lauf: build_axis_levels(Profil) OHNE die
/// tier-Ebene. = die EINE profil-getriebene AxisLevels-Quelle fuer den reinen Achsen-Permutations-Lauf. Die dyn.
/// Dimensionen (with_dynamic) bleiben erhalten (sie veraendern die binary_id nicht — is_static=false).
[[nodiscard]] inline std::vector<ex::AxisLevel> build_profile_basis_levels(
        cx::ThesisProfile const& tp, std::string const& mode_name,
        bool with_dynamic = true, ex::AxisRegistry const& registry = {}) {
    return drop_tier_level(build_profile_levels(tp, mode_name, with_dynamic, registry));
}

// ── (run_options) — die Lauf-Steuerung aus <run_options>. argv/env darf weiterhin uebersteuern (Rueckwaerts-
//    Kompatibilitaet); diese Struct liefert die PROFIL-DEFAULTS, die der Treiber anwendet, wenn argv/env fehlt. ──
struct ProfileRunOptions {
    std::size_t cap           = 0;      // max_binaries-Obergrenze (0 = ungesetzt → Treiber-Default)
    std::string platform;                // CSV-Tag platform
    std::string build_version;           // CSV-Tag build_version
    bool        resume        = true;    // Mess-Resume (#139)
};

/// profile_run_options — liest die <run_options> des Profils in die Treiber-Defaults (cap/platform/build_version/
/// resume). Ersetzt die argv/env-Defaults (COMDARE_PLATFORM/COMDARE_BUILD_VERSION/cap/resume) als PROFIL-Quelle.
[[nodiscard]] inline ProfileRunOptions profile_run_options(cx::ThesisProfile const& tp) {
    ProfileRunOptions ro;
    ro.cap           = (tp.run_options.cap > 0) ? static_cast<std::size_t>(tp.run_options.cap) : 0;
    ro.platform      = tp.run_options.platform;
    ro.build_version = tp.run_options.build_version;
    ro.resume        = tp.run_options.resume;
    return ro;
}

// ── (working_set_sweep) — die N-Liste der AEUSSEREN Lauf-Iteration (Record-Zahlen, P-MD7). Ersetzt die
//    PS-foreach (build_and_measure_150_tiere.ps1:166) + COMDARE_WORKLOAD_RECORDS. Leer = einmaliger Lauf. ──
[[nodiscard]] inline std::vector<std::uint64_t> profile_working_set_sweep(cx::ThesisProfile const& tp) {
    std::vector<std::uint64_t> ns;
    ns.reserve(tp.working_set_sweep.size());
    for (auto const& s : tp.working_set_sweep) {
        char* end = nullptr;
        std::uint64_t const v = std::strtoull(s.c_str(), &end, 10);
        if (end != s.c_str() && v > 0) ns.push_back(v);
    }
    return ns;
}

// ── (axis_sweeps) — der Treiber loest jeden <axis_sweep axis=..> auf die LEVEL-INDEX-Position im tier-freien
//    Basis-StaticBinaryView auf (per Achsen-Namen-Suche in den statischen AxisLevels), NICHT ueber eine
//    hartkodierte axis_to_level-Map (run_lazy_150.cpp:214). Damit ist die Aufloesung PROFIL-GETRIEBEN. ──

/// profile_axis_level — der Level-Index einer Achse im (tier-freien) statischen AxisLevel-Satz. -1 wenn unbekannt.
/// Single-Source: dieselbe Reihenfolge, die der StaticBinaryView verwendet (kCompositionAxisNames-Reihenfolge).
[[nodiscard]] inline std::size_t profile_axis_level(
        std::vector<ex::AxisLevel> const& static_basis_levels, std::string const& axis_name) {
    std::size_t d = 0;
    for (auto const& l : static_basis_levels) {
        if (!l.is_static) continue;          // nur statische Ebenen tragen die binary_id
        if (l.axis == axis_name) return d;
        ++d;
    }
    return static_cast<std::size_t>(-1);     // unbekannt → Caller verweigert (provenance-Marker)
}

// ── Ein getaggter profil-getriebener Selektions-Pass (BuildSelection + die 2 Klassen-Tags series/sweep_axis). ──
struct ProfileTaggedSelection {
    ex::BuildSelection selection;
    std::string        series     = "-";   // SOTA-Reihe (hier immer "-" — sota_series erst Inc5)
    std::string        sweep_axis  = "-";   // gesweepte Achse oder "-" (Basis)
    std::string        label;
};

/// profile_make_basis — BASIS-320: die ersten `cap` View-Indizes (voll-faktoriell, identisch zum heutigen
/// "index"/make_basis-Modus). Tag series="-" sweep_axis="-". Reine explizite Index-Liste (kein Ad-hoc).
[[nodiscard]] inline ProfileTaggedSelection profile_make_basis(
        ex::StaticBinaryView const& view, std::size_t cap) {
    std::size_t const n = (std::min)(cap, view.size());
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) ids.push_back(i);
    ProfileTaggedSelection ts;
    ts.selection = ex::select_explicit(std::move(ids));
    ts.series = "-"; ts.sweep_axis = "-";
    ts.label  = "basis-320";
    return ts;
}

/// profile_make_axis_sweep — PER-ACHSEN-SWEEP gegen die feste Baseline (alle Ebenen Index 0) variiert GENAU die
/// Achse `level_d` ueber ihre Ausprägungen (via StaticBinaryView::flat_index — Achsen-Austausch IM Praefixbaum).
/// Funktional identisch zu m3v2::make_axis_sweep, aber der level_d kommt aus profile_axis_level (PROFIL-getrieben).
[[nodiscard]] inline ProfileTaggedSelection profile_make_axis_sweep(
        ex::StaticBinaryView const& view, std::size_t level_d,
        std::string const& axis_name, std::size_t cap) {
    ProfileTaggedSelection ts;
    ts.series = "-"; ts.sweep_axis = axis_name;
    ts.label  = "sweep:" + axis_name;
    if (level_d == static_cast<std::size_t>(-1) || level_d >= view.level_count()) {
        ts.selection.provenance = "axis_sweep-REFUSED-no-such-level";
        return ts;
    }
    std::size_t const k = view.level_size(level_d);
    std::vector<std::size_t> tuple(view.level_count(), 0);   // Baseline: alle Achsen Index 0 (FESTE Baseline-Tier)
    std::size_t const n = (std::min)(cap, k);
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t v = 0; v < n; ++v) {
        tuple[level_d] = v;                                  // NUR die eine Achse austauschen
        ids.push_back(view.flat_index(tuple));               // Praefixbaum-Index (mixed-radix), kein Flach-Tupel
    }
    ts.selection = ex::select_explicit(std::move(ids));
    ts.selection.provenance = "axis_sweep:" + axis_name;
    return ts;
}

/// profile_select — die EINE profil-getriebene Selektions-Auswahl, gesteuert ueber `series_axis_or_empty`:
///   • ""  / "basis"        → BASIS-320 (profile_make_basis).
///   • "<achse>"            → PER-ACHSEN-SWEEP: matcht einen <axis_sweep axis="<achse>"> im Profil; der level_d
///                            wird ueber profile_axis_level aus den tier-freien Basis-Levels aufgeloest.
/// Verweigert (provenance-Marker) wenn die Achse NICHT als <axis_sweep> deklariert ist (= profil-getrieben:
/// nur deklarierte Sweeps sind waehlbar — die hartkodierte axis_to_level-Map ist ersetzt).
[[nodiscard]] inline ProfileTaggedSelection profile_select(
        cx::ThesisProfile const& tp,
        std::vector<ex::AxisLevel> const& static_basis_levels,
        ex::StaticBinaryView const& view,
        std::string const& series_axis_or_empty,
        std::size_t cap) {
    if (series_axis_or_empty.empty() || series_axis_or_empty == "basis" || series_axis_or_empty == "index")
        return profile_make_basis(view, cap);
    // Per-Achsen-Sweep: die Achse MUSS als <axis_sweep> deklariert sein (profil-getriebene Whitelist).
    bool declared = false;
    for (auto const& sw : tp.axis_sweeps) if (sw.axis == series_axis_or_empty) { declared = true; break; }
    ProfileTaggedSelection ts;
    if (!declared) {
        ts.sweep_axis = series_axis_or_empty;
        ts.label = "sweep:" + series_axis_or_empty;
        ts.selection.provenance = "axis_sweep-REFUSED-not-declared-in-profile";
        return ts;
    }
    std::size_t const level_d = profile_axis_level(static_basis_levels, series_axis_or_empty);
    return profile_make_axis_sweep(view, level_d, series_axis_or_empty, cap);
}

}  // namespace comdare::cache_engine::thesis_lazy
