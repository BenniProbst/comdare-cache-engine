#pragma once
// STRANG A KORRIGIERT — Increment 1 (2026-06-18, profil-getrieben). profile_runner: die EINE fehlende NAHT.
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S3-Kern, mit Inc4/S5 vollendet).
// SOLL (Doc architektur/10_schichten_modell_M.md §2.2): die Diplomarbeit konfiguriert die Messung ueber ein
// DEKLARATIVES comdare_thesis_profile; der CacheEngineBuilder fuehrt aus. Die offizielle Kette
// (ThesisProfile → parse_thesis_profile → build_axis_levels → tree.build) ist seit Inc4/S5 der EINZIGE Lauf-Pfad
// des Treibers (die fruehere hartkodierte Code-Selektions-Schicht ist ENTFERNT).
//
// DIESER HEADER ist die Naht selbst: COMDARE_THESIS_PROFILE → parse_thesis_profile → build_axis_levels(tp, mode,
// registry) → AxisLevels (+ DynamicDims aus runtime_dynamic) → tree.build(levels) → run_lazy_static_then_dynamic.
//
// HARTE ROUND-TRIP-GATE (Plan-Risiko #1, Resume #139): die ueber build_axis_levels(Profil) erzeugten binary_ids
// MUESSEN EXAKT IDENTISCH sein zur committeten golden-Liste (golden_fullpilot_320_binary_ids.txt; gleiche 19
// Slot-Namen-Reihenfolge + gleiche W::name()-Werte + serialize_composition_path-Format). Belegt LITERAL in
// test_profile_roundtrip.cpp (StaticBinaryView-binary_id-Listen-Diff == leer). Die binary_id nutzt NUR die
// STATISCHEN Ebenen (axis=value); DynamicDims aendern sie NICHT.
//
// LEITPLANKEN: Lazy-Compile (1 DLL=1 TU) bleibt (SourceGen kommt aus dem profil-agnostischen source_catalog,
// vom Caller injiziert); Two-Phasen-Warmup intakt (liegt im perm_runner). C++23, header-only.

#include <builder/experiment_tree/experiment_tree.hpp>    // AxisLevel / DynamicDim / ExperimentTree / StaticBinaryView
#include <builder/experiment_tree/profile_to_tree.hpp>    // build_axis_levels (die offizielle Bruecke)
#include <builder/experiment_tree/coverage_selection.hpp> // BuildSelection / select_explicit
#include <builder/build_orchestrator/build_orchestrator.hpp> // ex::SourceGenFn (make_union_source_gen-Signatur; war
                                                             //   bisher nur transitiv via cache_engine_builder_iterator
                                                             //   sichtbar → fehlte im profil-only-Include-Satz, #168)
#include "xml_config_parser/xml_config_parser.hpp"           // ThesisProfile / XmlConfigParser::parse_thesis_profile

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cx = ::comdare::builder::xml;

// ════════════════════════════════════════════════════════════════════════════════════════════════════════════
// STRANG A KORRIGIERT — Increment 6 / S7a (2026-06-19): SourceGen-VEREINIGUNG (source_catalog ∪ sota_catalog).
//
// Der Treiber fährt aus EINEM Profil zwei DISJUNKTE binary_id-Namensräume: die Basis-320 ("search_algo=.../..."
// via source_catalog/make_catalog_source_gen) UND die SOTA-Reihen ("sota_tier=sota::S::name" via sota_catalog/
// build_sota_view_source_map). make_union_source_gen verbindet die beiden zu EINER SourceGenFn: je binary_id
// zuerst die Basis-Quelle (catalog_gen) abfragen; ist sie LEER (= kein Basis-id), die SOTA-map konsultieren.
// Da die Namensräume disjunkt sind, ist die Reihenfolge unkritisch — es kann nie ein Schlüssel-Konflikt geben.
// Lazy-Compile (1 DLL = 1 TU) bleibt: die Vereinigung wählt NUR die richtige Quelle je binary_id, sie ändert
// nichts an der per-Binary-Kompilierung. C++23, header-only, engine-agnostisch (keine schweren Katalog-Includes).
// ════════════════════════════════════════════════════════════════════════════════════════════════════════════

/// make_union_source_gen — DIE EINE vereinigte SourceGenFn (S7a). `catalog_gen` = die Basis-320-SourceGenFn
/// (make_catalog_source_gen, source_catalog.hpp); `sota_by_view_id` = die SOTA-Reihen-Quellen, GEKEYT auf den
/// VIEW-binary_id ("sota_tier=…", build_sota_view_source_map). Ein binary_id, der in KEINER der beiden Quellen
/// liegt, liefert eine leere Quelle → der BuildOrchestrator markiert die DLL als nicht baubar (ehrlich sichtbar).
[[nodiscard]] inline ex::SourceGenFn make_union_source_gen(ex::SourceGenFn                    catalog_gen,
                                                           std::map<std::string, std::string> sota_by_view_id) {
    return [catalog_gen = std::move(catalog_gen),
            sota        = std::move(sota_by_view_id)](std::string const& binary_id) -> std::string {
        std::string src = catalog_gen ? catalog_gen(binary_id) : std::string{}; // (1) Basis-320 (disjunkt)
        if (!src.empty()) return src;
        auto it = sota.find(binary_id); // (2) SOTA-Reihen (disjunkt)
        return it == sota.end() ? std::string{} : it->second;
    };
}

/// build_profile_levels(tp, mode) — der GESAMT-Level-Satz fuer ExperimentTree::build, AUSSCHLIESSLICH aus der
/// offiziellen Bruecke ex::build_axis_levels (Inc2 dyn-Dim-Konsolidierung, 2026-06-18): build_axis_levels emittiert
/// die runtime_dynamic-Dimensionen (concurrency.thread_count / prefetch.hw_prefetcher / repetition.repetition_index)
/// jetzt SELBST als is_static=false-Ebenen — die fruehere Doppelquelle (profile_runner haengt sie ein ZWEITES Mal an)
/// ist ENTFERNT. `with_dynamic=false` ⇒ es werden nur die STATISCHEN Ebenen zurueckgegeben (die dynamischen werden
/// herausgefiltert) = die reine binary_id-Quelle fuer die Round-Trip-Gate. Die AxisRegistry expandiert leere
/// <axis>-Listen (hier leer ⇒ die explizit deklarierten <value> aus dem Profil gewinnen → Pilot-Identitaet).
[[nodiscard]] inline std::vector<ex::AxisLevel> build_profile_levels(cx::ThesisProfile const& tp,
                                                                     std::string const&       mode_name,
                                                                     bool                     with_dynamic = true,
                                                                     ex::AxisRegistry const&  registry     = {}) {
    std::vector<ex::AxisLevel> lv = ex::build_axis_levels(tp, mode_name, registry); // EINZIGE Quelle (statisch+dyn)
    if (with_dynamic) return lv;
    std::vector<ex::AxisLevel> static_only;
    static_only.reserve(lv.size());
    for (auto& l : lv)
        if (l.is_static) static_only.push_back(std::move(l)); // dyn-Dims herausfiltern
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
// (run_lazy_150 geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)
// (PS-foreach + SelectMode + COMDARE_*-env): working_set_sweep ersetzt die PS-foreach + COMDARE_WORKLOAD_RECORDS;
// axis_sweeps ersetzt SelectMode "axis_sweep:<a>" + axis_to_level-Map; run_options ersetzt argv/env; die
// tier-Level-Behandlung haelt den binary_id-Raum stabil (Basis-320 == golden-Liste, Resume #139).
//
// STAND Inc4/S5: die fruehere hartkodierte Code-Selektions-Schicht ist ENTFERNT — diese Helfer SIND der einzige
// Selektions-/Lauf-Steuer-Pfad des Treibers. sota_series wird HIER NICHT konsumiert (Inc5/#162).
// ════════════════════════════════════════════════════════════════════════════════════════════════════════════

// ── tier-Level-Behandlung (Basis-320): build_axis_levels emittiert ein "tier"-Level OBEN (SOTA-Reihen-Dimension
//    aus base_tiers). Fuer den reinen 4-Achsen-Permutations-Lauf (Basis-320) zieht der Treiber diese Ebene ab —
//    so erzeugt der StaticBinaryView DENSELBEN binary_id-Raum wie die committete golden-Liste (KEIN tier-Level).
//    Round-Trip-belegt in test_profile_roundtrip.cpp (drop_tier → Mismatch 0 gegen golden). Single-Source. ──
[[nodiscard]] inline std::vector<ex::AxisLevel> drop_tier_level(std::vector<ex::AxisLevel> levels) {
    std::vector<ex::AxisLevel> out;
    out.reserve(levels.size());
    for (auto& l : levels)
        if (l.axis != "tier") out.push_back(std::move(l));
    return out;
}

/// build_profile_basis_levels — der Gesamt-Level-Satz fuer den BASIS-320-Lauf: build_axis_levels(Profil) OHNE die
/// tier-Ebene. = die EINE profil-getriebene AxisLevels-Quelle fuer den reinen Achsen-Permutations-Lauf. Die dyn.
/// Dimensionen (with_dynamic) bleiben erhalten (sie veraendern die binary_id nicht — is_static=false).
[[nodiscard]] inline std::vector<ex::AxisLevel> build_profile_basis_levels(cx::ThesisProfile const& tp,
                                                                           std::string const&       mode_name,
                                                                           bool                     with_dynamic = true,
                                                                           ex::AxisRegistry const&  registry     = {}) {
    return drop_tier_level(build_profile_levels(tp, mode_name, with_dynamic, registry));
}

// ── (run_options) — die Lauf-Steuerung aus <run_options>. argv/env darf weiterhin uebersteuern (Rueckwaerts-
//    Kompatibilitaet); diese Struct liefert die PROFIL-DEFAULTS, die der Treiber anwendet, wenn argv/env fehlt. ──
struct ProfileRunOptions {
    std::size_t cap = 0;       // max_binaries-Obergrenze (0 = cap="0"/fehlend = KEIN Cap → alle Basis-Zellen)
    std::string platform;      // CSV-Tag platform
    std::string build_version; // CSV-Tag build_version
    bool        resume = true; // Mess-Resume (#139)
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

/// profile_effective_cap — DIE EINE cap-Aufloesung der Basis-Selektion (GO-4/GO-5-Nebenbefund, 2026-07-12).
/// Semantik (gepinnt in test_profile_roundtrip (7)):
///   • override_cap > 0 (argv/env, z.B. COMDARE_E4_CAP)  ⇒ override_cap gewinnt (Rueckwaerts-Kompatibilitaet).
///   • sonst profile_cap > 0 (<run_options cap="N">)      ⇒ profile_cap.
///   • BEIDE 0 (cap="0" ODER fehlendes cap)               ⇒ KEIN Cap = alle `basis_count` Basis-Zellen.
/// VORHER interpretierte run_profile eff_cap==0 als N=0 (LEERE Basis-Selektion) — damit bekam
/// m3_golden_coverage (cap="0" = dokumentiert "KEIN kuenstliches Cap") KEINE Basis-320. Das Ergebnis ist
/// stets auf basis_count geklemmt (min), nie darueber.
[[nodiscard]] inline std::size_t profile_effective_cap(std::size_t profile_cap, std::size_t override_cap,
                                                       std::size_t basis_count) {
    std::size_t eff = override_cap;
    if (override_cap == 0 && profile_cap > 0) eff = profile_cap;
    if (eff == 0) eff = basis_count; // 0 = KEIN Cap → alle Basis-Zellen (nie mehr "leer")
    return (std::min)(eff, basis_count);
}

// ── (working_set_sweep) — die N-Liste der AEUSSEREN Lauf-Iteration (Record-Zahlen, P-MD7). Ersetzt die
//    den alten PS-foreach-Behelf (entfernt) + COMDARE_WORKLOAD_RECORDS. Leer = einmaliger Lauf. ──
[[nodiscard]] inline std::vector<std::uint64_t> profile_working_set_sweep(cx::ThesisProfile const& tp) {
    std::vector<std::uint64_t> ns;
    ns.reserve(tp.working_set_sweep.size());
    for (auto const& s : tp.working_set_sweep) {
        char*               end = nullptr;
        std::uint64_t const v   = std::strtoull(s.c_str(), &end, 10);
        if (end != s.c_str() && v > 0) ns.push_back(v);
    }
    return ns;
}

// ── (datasets, GO-5 Fork 1, 2026-07-12) — die deklarierten <dataset>-Akten-Referenzen als EINE deterministische
//    Signatur (Dokument-Reihenfolge = geparste XML-Reihenfolge). KONSUMENTEN heute: (a) der RUN_PROFILE-Lauf-Kopf
//    (Provenienz-Log) und (b) der Mess-Resume-Stamp (LazyRunConfig::profile_datasets → lazy_resume_stamp_prefix):
//    aendert sich die Dataset-Deklaration, ist ein alter per-Binary-Stand NICHT mehr resume-faehig (konservativ —
//    ehrliche Neu-Messung statt stalem Resume). Der Loader-MESS-KonsUM (load_or_generate_ycsb mit
//    dataset_source=loader, dataset_id=akte) ist der dokumentierte offene Folge-Schritt (lauf-gated; Loader-Slot
//    seit #184 hermetisch bewiesen) — bis dahin ist diese Signatur der EHRLICHE Ankunfts-Nachweis in der Spec. ──
[[nodiscard]] inline std::string profile_datasets_signature(cx::ThesisProfile const& tp) {
    std::string s;
    for (auto const& d : tp.datasets) {
        s += d.id;
        s += ':';
        s += d.akte_ref;
        s += ':';
        s += d.loader;
        s += ';';
    }
    return s;
}

// ── (axis_sweeps) — der Treiber loest jeden <axis_sweep axis=..> auf die LEVEL-INDEX-Position im tier-freien
//    Basis-StaticBinaryView auf (per Achsen-Namen-Suche in den statischen AxisLevels), NICHT ueber eine
//    hartkodierte axis_to_level-Map (run_lazy_150.cpp:214). Damit ist die Aufloesung PROFIL-GETRIEBEN. ──
//    (run_lazy_150.cpp geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)

/// profile_axis_level — der Level-Index einer Achse im (tier-freien) statischen AxisLevel-Satz. -1 wenn unbekannt.
/// Single-Source: dieselbe Reihenfolge, die der StaticBinaryView verwendet (kCompositionAxisNames-Reihenfolge).
[[nodiscard]] inline std::size_t profile_axis_level(std::vector<ex::AxisLevel> const& static_basis_levels,
                                                    std::string const&                axis_name) {
    std::size_t d = 0;
    for (auto const& l : static_basis_levels) {
        if (!l.is_static) continue; // nur statische Ebenen tragen die binary_id
        if (l.axis == axis_name) return d;
        ++d;
    }
    return static_cast<std::size_t>(-1); // unbekannt → Caller verweigert (provenance-Marker)
}

// ── Ein getaggter profil-getriebener Selektions-Pass (BuildSelection + die 2 Klassen-Tags series/sweep_axis). ──
struct ProfileTaggedSelection {
    ex::BuildSelection selection;
    std::string        series     = "-"; // SOTA-Reihe (hier immer "-" — sota_series erst Inc5)
    std::string        sweep_axis = "-"; // gesweepte Achse oder "-" (Basis)
    std::string        label;
};

/// profile_make_basis — BASIS-320: die ersten `cap` View-Indizes (voll-faktoriell, identisch zum heutigen
/// "index"/make_basis-Modus). Tag series="-" sweep_axis="-". Reine explizite Index-Liste (kein Ad-hoc).
[[nodiscard]] inline ProfileTaggedSelection profile_make_basis(ex::StaticBinaryView const& view, std::size_t cap) {
    std::size_t const        n = (std::min)(cap, view.size());
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) ids.push_back(i);
    ProfileTaggedSelection ts;
    ts.selection  = ex::select_explicit(std::move(ids));
    ts.series     = "-";
    ts.sweep_axis = "-";
    ts.label      = "basis-320";
    return ts;
}

/// profile_make_axis_sweep — PER-ACHSEN-SWEEP gegen die feste Baseline (alle Ebenen Index 0) variiert GENAU die
/// Achse `level_d` ueber ihre Ausprägungen (via StaticBinaryView::flat_index — Achsen-Austausch IM Praefixbaum).
/// Funktional identisch zu m3v2::make_axis_sweep, aber der level_d kommt aus profile_axis_level (PROFIL-getrieben).
[[nodiscard]] inline ProfileTaggedSelection profile_make_axis_sweep(ex::StaticBinaryView const& view,
                                                                    std::size_t level_d, std::string const& axis_name,
                                                                    std::size_t cap) {
    ProfileTaggedSelection ts;
    ts.series     = "-";
    ts.sweep_axis = axis_name;
    ts.label      = "sweep:" + axis_name;
    if (level_d == static_cast<std::size_t>(-1) || level_d >= view.level_count()) {
        ts.selection.provenance = "axis_sweep-REFUSED-no-such-level";
        return ts;
    }
    std::size_t const        k = view.level_size(level_d);
    std::vector<std::size_t> tuple(view.level_count(), 0); // Baseline: alle Achsen Index 0 (FESTE Baseline-Tier)
    std::size_t const        n = (std::min)(cap, k);
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t v = 0; v < n; ++v) {
        tuple[level_d] = v;                    // NUR die eine Achse austauschen
        ids.push_back(view.flat_index(tuple)); // Praefixbaum-Index (mixed-radix), kein Flach-Tupel
    }
    ts.selection            = ex::select_explicit(std::move(ids));
    ts.selection.provenance = "axis_sweep:" + axis_name;
    return ts;
}

/// profile_sweep_passes — #26/GO-5 (B.4.1-b, 2026-07-12): die DETERMINISTISCHE Pass-Liste der Selektions-Phase
/// von run_profile. Vorher fuhr run_profile GENAU EINEN Selektions-Pass (Basis ODER die eine args.sweep_axis) —
/// und der E4-Treiber setzt sweep_axis nie ⇒ die im Profil deklarierten <axis_sweeps> blieben im offiziellen
/// XML-Weg UNGEFAHREN (Eigenbefund Dossier GO-5 B.1; betraf auch den #18-Coverage-Voll-Lauf).
/// Semantik:
///   • `requested_axis` nicht leer (explizites args.sweep_axis) ⇒ genau {requested_axis} — das bisherige
///     Einzel-Sweep-Verhalten behaelt Vorrang und bleibt unveraendert (rueckwaerts-kompatibel).
///   • `requested_axis` leer ⇒ {""} (= Basis-Pass, immer zuerst) + JE deklariertem <axis_sweep> ein Pass in
///     Dokument-Reihenfolge (tp.axis_sweeps ist die geparste XML-Reihenfolge ⇒ deterministisch).
///     Profile OHNE <axis_sweeps> liefern exakt {""} = das bisherige Basis-only-Verhalten (byte-identisch).
[[nodiscard]] inline std::vector<std::string> profile_sweep_passes(cx::ThesisProfile const& tp,
                                                                   std::string const&       requested_axis) {
    if (!requested_axis.empty()) return {requested_axis};
    std::vector<std::string> passes;
    passes.reserve(tp.axis_sweeps.size() + 1);
    passes.emplace_back(); // "" = Basis-Pass (immer zuerst)
    for (auto const& sw : tp.axis_sweeps) passes.push_back(sw.axis);
    return passes;
}

/// profile_select — die EINE profil-getriebene Selektions-Auswahl, gesteuert ueber `series_axis_or_empty`:
///   • ""  / "basis"        → BASIS-320 (profile_make_basis).
///   • "<achse>"            → PER-ACHSEN-SWEEP: matcht einen <axis_sweep axis="<achse>"> im Profil; der level_d
///                            wird ueber profile_axis_level aus den tier-freien Basis-Levels aufgeloest.
/// Verweigert (provenance-Marker) wenn die Achse NICHT als <axis_sweep> deklariert ist (= profil-getrieben:
/// nur deklarierte Sweeps sind waehlbar — die hartkodierte axis_to_level-Map ist ersetzt).
[[nodiscard]] inline ProfileTaggedSelection profile_select(cx::ThesisProfile const&          tp,
                                                           std::vector<ex::AxisLevel> const& static_basis_levels,
                                                           ex::StaticBinaryView const&       view,
                                                           std::string const& series_axis_or_empty, std::size_t cap) {
    if (series_axis_or_empty.empty() || series_axis_or_empty == "basis" || series_axis_or_empty == "index")
        return profile_make_basis(view, cap);
    // Per-Achsen-Sweep: die Achse MUSS als <axis_sweep> deklariert sein (profil-getriebene Whitelist).
    bool declared = false;
    for (auto const& sw : tp.axis_sweeps)
        if (sw.axis == series_axis_or_empty) {
            declared = true;
            break;
        }
    ProfileTaggedSelection ts;
    if (!declared) {
        ts.sweep_axis           = series_axis_or_empty;
        ts.label                = "sweep:" + series_axis_or_empty;
        ts.selection.provenance = "axis_sweep-REFUSED-not-declared-in-profile";
        return ts;
    }
    std::size_t const level_d = profile_axis_level(static_basis_levels, series_axis_or_empty);
    return profile_make_axis_sweep(view, level_d, series_axis_or_empty, cap);
}

} // namespace comdare::cache_engine::thesis_lazy
