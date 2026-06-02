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
    std::size_t i = 0;
    while (i <= line.size()) {
        std::size_t const sc = line.find(';', i);
        std::size_t const end = (sc == std::string_view::npos) ? line.size() : sc;
        f.push_back(line.substr(i, end - i));
        if (sc == std::string_view::npos) break;
        i = sc + 1;
    }
    if (f.size() < 14 || f[0].empty()) return false;

    auto u64 = [](std::string_view s) -> std::uint64_t {
        std::uint64_t v = 0; std::from_chars(s.data(), s.data() + s.size(), v); return v;
    };
    NodeObserverSnapshot o{};
    o.search_lookup_count      = u64(f[1]);  o.search_hit_count          = u64(f[2]);
    o.search_miss_count        = u64(f[3]);  o.search_insert_count       = u64(f[4]);
    o.search_erase_count       = u64(f[5]);  o.search_peak_occupancy     = u64(f[6]);
    o.alloc_bytes_allocated    = u64(f[7]);  o.alloc_bytes_in_use        = u64(f[8]);
    o.alloc_allocation_count   = u64(f[9]);  o.alloc_deallocation_count  = u64(f[10]);
    o.alloc_failure_count      = u64(f[11]); o.observable_axis_count     = u64(f[12]);
    o.tier_fill_level          = u64(f[13]);

    NodeValue nv{};
    nv.has_result     = true;
    nv.observer       = o;
    nv.observer_real  = true;   // aus echtem Cluster-Lauf (perm_runner tier_observe)
    nv.measured_setting_count = 1;
    tree.set_node_value(std::string{f[0]}, nv);
    return true;
}

/// Ingest mehrerer Zeilen (z.B. eine Cluster-Ergebnis-Datei / Webhook-Batch). Liefert die Zahl eingespielter Knoten.
[[nodiscard]] inline std::size_t ingest_results(ExperimentTree& tree, std::string_view text) {
    std::size_t n = 0, i = 0;
    while (i <= text.size()) {
        std::size_t const nl = text.find('\n', i);
        std::size_t const end = (nl == std::string_view::npos) ? text.size() : nl;
        std::string_view line = text.substr(i, end - i);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (ingest_result_line(tree, line)) ++n;
        if (nl == std::string_view::npos) break;
        i = nl + 1;
    }
    return n;
}

}  // namespace comdare::cache_engine::builder::experiment
