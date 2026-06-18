#pragma once
// M3v2-SELEKTION (gate-frei, 2026-06-18, Task #156) — m3v2_select_profile: die EINE reproduzierbare, committete
// Selektions-Quelle für den m3v2-Neulauf (Design-Spec docs/sessions/20260618-M3v2-NEUMESSUNG-DESIGN-SPEC.md §3).
// KEIN Ad-hoc: der gesamte m3v2-Lebewesen-Satz (Basis-320 + Per-Achsen-Sweeps + SOTA-Reihen A/B/C) wird HIER aus
// dem statischen Achsen-Baum (StaticBinaryView) erzeugt — als getaggte BuildSelection-Pässe. Der Treiber
// (run_lazy_150) ruft je Pass die bestehende Lazy-Kette (build → load → dyn-Loop → messen) und schreibt die Tags
// (series/sweep_axis/working_set_n/platform/build_version) je Mess-Zeile in die CSV (Auswertungs-Trennung).
//
// LEITPLANKEN (Task): Achsen-Austausch IM Präfixbaum (StaticBinaryView::flat_index — nie Flach-Tupel-Enumeration);
// reproduzierbar (committete Quelle, kein Ad-hoc); Two-Phasen-Warmup unverändert (liegt im perm_runner). Dieser
// Header ist ENGINE-AGNOSTISCH (nur StaticBinaryView + BuildSelection — KEIN all_axes_umbrella) → leicht, testbar.
//
// DREI MESS-KLASSEN (jede als getaggter Selektions-Pass; die binary_id bleibt rein = Achsen-Rekombination):
//   (1) BASIS-320: voll-faktoriell search_algo × node_type × memory_layout × prefetch (= der bestehende „index"-
//       Modus, alle View-Indizes 0..N-1). Tag series="-" sweep_axis="-".
//   (2) PER-ACHSEN-SWEEP: gegen eine FESTE Baseline-Tier (alle Ebenen auf Index 0) variiert GENAU EINE Achse über
//       ihre Ausprägungen (level d: 0..level_size(d)-1, alle übrigen 0). KEIN volles Kartesisch → echte Diff-Belege
//       ohne Explosion. Tag sweep_axis=<achse>. (Die 9 vertieften Achsen, soweit im Baum vertreten; die im FullPilot
//       nicht variierten Achsen — migration/filter/value_handle/path_compression — kommen mit der SOTA-Engine-
//       Erweiterung [HELD] in den Voll-Lauf; ihre Sweep-Selektion ist hier als benannter, baubarer Achsen-Sweep
//       über die im Baum vorhandenen Ebenen vorbereitet.)
//   (3) SOTA-REIHEN A/B/C: PRT-ART + die 6 SOTA-Kompositionen unter Stufe-1 ce-only (A) / Stufe-2 Pruefling-ersetzt-
//       mit-Fallback (B) / Stufe-3 full-join (C) — die 3 Kompositionalen Joins (pruefling_merge.hpp / #8). Die
//       SOTA-/PRT-ART-Lebewesen-DLLs werden erst mit der Engine-Erweiterung gebaut (HELD); HIER liefert das Profil
//       die getaggte Selektion (series ∈ {A,B,C} + lebewesen-name) — der Tag-Apparat + die Reihen-Reihenfolge sind
//       reproduzierbar definiert und vom Treiber konsumierbar.
//
// C++23, header-only.

#include <builder/experiment_tree/experiment_tree.hpp>     // StaticBinaryView
#include <builder/experiment_tree/coverage_selection.hpp>  // BuildSelection

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy::m3v2 {

namespace ex = ::comdare::cache_engine::builder::experiment;

// ── Die 5 m3v2-Tags je Mess-Zeile (= die CSV-Tag-Spalten). Reine Metadaten (kein Mess-Einfluss). ──
struct RowTags {
    std::string series        = "-";            // SOTA-Reihe {A,B,C} oder "-" (Basis/Sweep)
    std::string sweep_axis    = "-";            // gesweepte Achse oder "-" (Basis/SOTA)
    std::string platform      = "win-x86_64";   // Plattform (Infra-Agent überschreibt für ZIH-Reihen)
    std::string build_version = "m3v2";         // Build-Version-Marke
    // working_set_n reist NICHT hier (es ist die N-Sweep-Achse = cfg.workload_records; der Treiber setzt es je
    // N-Wert separat). Diese RowTags decken die 4 Klassen-/Lauf-Tags ab.
};

// ── Ein getaggter Selektions-Pass: eine BuildSelection + die zugehörigen RowTags. ──
struct TaggedSelection {
    ex::BuildSelection selection;
    RowTags            tags;
    std::string        label;   // menschenlesbarer Pass-Name (Log/Diagnose), z.B. "basis-320" / "sweep:node_type"
};

// ── Die 6 SOTA-Kompositionen + PRT-ART = die 7 SOTA-Lebewesen je Reihe (Design-Spec §3c). Single-Source-Liste,
//    1:1 zu compositions/known_compositions_list.hpp (art/hot/masstree/surf/start/wormhole) + prt-art (Pruefling #8).
inline std::vector<std::string> const& sota_lebewesen_names() {
    static std::vector<std::string> const names = {
        "prt_art", "art", "hot", "masstree", "surf", "start", "wormhole"
    };
    return names;
}

// ── Die 3 SOTA-Reihen (Stufe 1/2/3 = die 3 Kompositionalen Joins). ──
inline std::vector<std::string> const& sota_series_ids() {
    static std::vector<std::string> const s = {"A", "B", "C"};  // A=Stufe-1 ce-only, B=Stufe-2 replace, C=Stufe-3 join
    return s;
}

// ── (1) BASIS-320: alle View-Indizes (voll-faktoriell). Tag series="-" sweep_axis="-". ──
/// Liefert die select_explicit(0..N-1)-Selektion über `cap` View-Indizes (kein Ad-hoc — explizite Index-Liste).
[[nodiscard]] inline TaggedSelection make_basis(ex::StaticBinaryView const& view, std::size_t cap) {
    std::size_t const n = (std::min)(cap, view.size());
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t i = 0; i < n; ++i) ids.push_back(i);
    TaggedSelection ts;
    ts.selection = ex::select_explicit(std::move(ids));
    ts.tags.series = "-"; ts.tags.sweep_axis = "-";
    ts.label = "basis-320";
    return ts;
}

// ── (2) PER-ACHSEN-SWEEP: gegen die feste Baseline (alle Ebenen Index 0) variiert NUR die Achse `level_d`. ──
/// Erzeugt die View-Indizes für level_d ∈ {0..level_size(level_d)-1}, alle übrigen Ebenen auf Index 0 — via
/// StaticBinaryView::flat_index (Achsen-Austausch IM Präfixbaum, NIE Flach-Tupel). Tag sweep_axis=<achse>.
[[nodiscard]] inline TaggedSelection make_axis_sweep(ex::StaticBinaryView const& view,
                                                     std::size_t level_d,
                                                     std::string const& axis_name,
                                                     std::size_t cap) {
    TaggedSelection ts;
    ts.tags.series = "-"; ts.tags.sweep_axis = axis_name;
    ts.label = "sweep:" + axis_name;
    if (level_d >= view.level_count()) { ts.selection.provenance = "sweep-REFUSED-no-such-level"; return ts; }
    std::size_t const k = view.level_size(level_d);
    std::vector<std::size_t> tuple(view.level_count(), 0);   // Baseline: alle Achsen auf Index 0 (FESTE Baseline-Tier)
    std::size_t const n = (std::min)(cap, k);
    std::vector<std::size_t> ids;
    ids.reserve(n);
    for (std::size_t v = 0; v < n; ++v) {
        tuple[level_d] = v;                                  // NUR die eine Achse austauschen
        ids.push_back(view.flat_index(tuple));               // Präfixbaum-Index (mixed-radix), kein Flach-Tupel
    }
    ts.selection = ex::select_explicit(std::move(ids));
    ts.selection.provenance = "axis_sweep:" + axis_name;
    return ts;
}

// ── (3) SOTA-REIHE A/B/C: getaggte Selektion je (Reihe × Lebewesen). ──
/// Die SOTA-/PRT-ART-Lebewesen-DLLs werden erst mit der Engine-Erweiterung gebaut (HELD bis der Voll-Lauf); diese
/// Funktion definiert die REPRODUZIERBARE Reihen-/Lebewesen-Zuordnung + die Tags (series ∈ {A,B,C}). Für den
/// gate-freien Pilot liefert sie KEINE View-Indizes der SOTA-Engine (die existiert noch nicht im Pilot-Baum) —
/// sie ist die Selektions-Definition, die der Treiber für die A/B/C-Tagung konsumiert. Der Pilot belegt den
/// SOTA-Tag-Apparat über die Basis-Binaries (Reihe-A-Repräsentanten, ce-only = Stufe 1).
[[nodiscard]] inline std::vector<RowTags> sota_row_tags() {
    std::vector<RowTags> out;
    for (auto const& series : sota_series_ids())
        for (auto const& name : sota_lebewesen_names()) {
            RowTags t;
            t.series     = series;
            t.sweep_axis = name;   // lebewesen-name im sweep_axis-Tag (für die A/B/C-Reihe = der Lebewesen-Identifier)
            out.push_back(std::move(t));
        }
    return out;
}

}  // namespace comdare::cache_engine::thesis_lazy::m3v2
