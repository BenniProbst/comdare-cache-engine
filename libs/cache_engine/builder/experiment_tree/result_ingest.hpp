#pragma once
// D14 / L-CLUSTER (gate-frei, 2026-06-02) — result_ingest: Rückführung der Cluster-/perm_runner-Mess-Ergebnisse
// in den Experiment-Baum. Eine Ergebnis-Zeile (binary_id + 13 Observer-Felder) wird geparst und als gemessener
// NodeValue (sparse) via tree.set_node_value eingespielt — der Host-seitige Empfangs-Pfad (Doc 28 §5 „Ergebnis-
// Rückführung in den Baum-NodeValue"). LOKAL verifizierbar (kein Cluster-Gate); die echte sbatch-Submission +
// der Webhook-Receiver sind GATE-MAXIMAL (getrennt). Self-contained (kein JSON-Lib), C++23.
//
// Zeilenformat (semikolon-getrennt, = NodeObserverSnapshot-Reihenfolge): EIN Feld binary_id + 13 uint64:
//   search_lookup;hit;miss;insert;erase;peak | alloc_bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail | observable_axes;fill

#include "experiment_tree.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Zerlegt `line` in binary_id + 13 uint64-Observer-Felder (';'-getrennt) und schreibt sie als gemessenen
/// NodeValue in den Baum. Rückgabe: true bei wohlgeformter Zeile (≥14 Felder). Leerzeilen/Kommentare ('#') → false.
[[nodiscard]] inline bool ingest_result_line(ExperimentTree& tree, std::string_view line) {
    if (line.empty() || line.front() == '#') return false;
    std::vector<std::string_view> f;
    std::size_t                   i = 0;
    while (i <= line.size()) {
        std::size_t const sc  = line.find(';', i);
        std::size_t const end = (sc == std::string_view::npos) ? line.size() : sc;
        f.push_back(line.substr(i, end - i));
        if (sc == std::string_view::npos) break;
        i = sc + 1;
    }
    // KONSOLIDIERUNG (I-B.3, 2026-06-04; Bau-INC-2c 19→18): volle Matrix = binary_id + axis_stats[17][8] (136)
    // + seg_ns[17] (17) + Meta (observable_axis_count, tier_fill_level, filled_axis_count, batches_measured)
    // + P-MD3 (seg_framework_ns, seg_run_total_ns) = 159 Felder = 160 total (Bau-INC-2d: axis_stats[17][8]=136 +
    // seg_ns[17]=17 + 4 Meta + 2 P-MD3). MAJOR-MESS-09 (Audit A1): EXAKT ==160 prüfen (nicht ≥160). Ein ';'/Newline
    // in der binary_id (oder ein angehängtes/verschmolzenes Feld) erzeugt eine Zeile ≠160 → die axis_stats wären
    // gegen ihre Slots verschoben. format_perm_result lehnt verletzende IDs bereits ab; diese Schranke ist die
    // zweite, lese-seitige Verteidigung (kein stiller Slot-Versatz).
    if (f.size() != 160 || f[0].empty()) return false;

    auto u64 = [](std::string_view s) -> std::uint64_t {
        std::uint64_t v = 0;
        std::from_chars(s.data(), s.data() + s.size(), v);
        return v;
    };
    auto i64 = [](std::string_view s) -> std::int64_t {
        std::int64_t v = 0;
        std::from_chars(s.data(), s.data() + s.size(), v);
        return v;
    };
    NodeObserverSnapshot o{};
    std::size_t          idx = 1;
    for (std::size_t t = 0; t < 17; ++t) // Bau-INC-2d: 17 Achsen (isa raus)
        for (std::size_t fi = 0; fi < 8; ++fi) o.axis_stats[t][fi] = u64(f[idx++]);
    for (std::size_t t = 0; t < 17; ++t) o.seg_ns[t] = i64(f[idx++]);
    o.observable_axis_count = u64(f[idx++]);
    o.tier_fill_level       = u64(f[idx++]);
    o.filled_axis_count     = u64(f[idx++]);
    o.batches_measured      = u64(f[idx++]);
    // P-MD3 (2026-06-18): die 2 additiven Coverage-Versöhnungs-Meta-Felder (Reihenfolge = format_perm_result).
    o.seg_framework_ns = i64(f[idx++]);
    o.seg_run_total_ns = i64(f[idx++]);
    // Legacy-V1-Projektion (Übergang, entfällt I-C): 13 benannte Felder = Sicht auf axis_stats[0] (search) /
    // axis_stats[6] (allocator) → die noch nicht migrierten 13-Feld-Konsumenten (CSV-o.*, test_d14b/d14c).
    o.search_lookup_count      = o.axis_stats[0][0];
    o.search_hit_count         = o.axis_stats[0][1];
    o.search_miss_count        = o.axis_stats[0][2];
    o.search_insert_count      = o.axis_stats[0][3];
    o.search_erase_count       = o.axis_stats[0][4];
    o.search_peak_occupancy    = o.axis_stats[0][5];
    o.alloc_bytes_allocated    = o.axis_stats[6][0];
    o.alloc_bytes_in_use       = o.axis_stats[6][1];
    o.alloc_allocation_count   = o.axis_stats[6][2];
    o.alloc_deallocation_count = o.axis_stats[6][3];
    o.alloc_failure_count      = o.axis_stats[6][4];

    NodeValue nv{};
    nv.has_result             = true;
    nv.observer               = o;
    nv.observer_real          = true; // aus echtem Cluster-Lauf (perm_runner tier_observe)
    nv.measured_setting_count = 1;
    tree.set_node_value(std::string{f[0]}, nv);
    return true;
}

/// Ingest mehrerer Zeilen (z.B. eine Cluster-Ergebnis-Datei / Webhook-Batch). Liefert die Zahl eingespielter Knoten.
[[nodiscard]] inline std::size_t ingest_results(ExperimentTree& tree, std::string_view text) {
    std::size_t n = 0, i = 0;
    while (i <= text.size()) {
        std::size_t const nl   = text.find('\n', i);
        std::size_t const end  = (nl == std::string_view::npos) ? text.size() : nl;
        std::string_view  line = text.substr(i, end - i);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (ingest_result_line(tree, line)) ++n;
        if (nl == std::string_view::npos) break;
        i = nl + 1;
    }
    return n;
}

} // namespace comdare::cache_engine::builder::experiment
