#pragma once
// KF-9-Adapter (2026-06-02) — comdare_thesis_profile → AxisLevels für den Experiment-B+-Baum (Permutations-/Präfixbaum, kein textbook-B+-Baum — s. experiment_tree.hpp:2-10).
//
// Brückt das geparste Profil (comdare::builder::xml::ThesisProfile, KF-1) an den Baum-Kern (experiment_tree.hpp):
//   • Paper/Tier-Dimension (oben): Fanout = base_tiers (jeder = ein gepinntes Paper-Tupel).
//   • freigegebene permute_axes (mode.active_axes) → STATISCHE Ebenen (→ Binaries); cacheline → statische
//     Sub-Ebenen (line_size/alignment/sw_hint, compile-time); node_width → statische Sub-Ebene
//     (width_in_lines, compile-time; C2/FF2 2026-07-12, Muster cacheline).
//   • runtime_dynamic (thread_count/hw_prefetcher) → DYNAMISCHE Ebenen (Laufzeit-for-Schleife, Algorithm_Resource_Control).
// Werte: explizit aus dem Profil; fehlen sie, expandiert die AxisRegistry (permutation_axes.xml) die volle Liste.
// Doc architecture/26. C++23, header-only.

#include "experiment_tree.hpp"
#include "xml_config_parser/xml_config_parser.hpp"

#include <map>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Achsen-Wertebereiche aus permutation_axes.xml (axis-ref → volle Werteliste).
using AxisRegistry = std::map<std::string, std::vector<std::string>>;

/// Baut die geordneten AxisLevels eines Modus aus dem Thesis-Profil.
[[nodiscard]] inline std::vector<AxisLevel> build_axis_levels(comdare::builder::xml::ThesisProfile const& tp,
                                                              std::string const&                          mode_name,
                                                              AxisRegistry const&                         registry) {
    std::vector<AxisLevel> levels;

    // 1. Paper/Tier-Dimension (oben).
    {
        AxisLevel tier;
        tier.axis      = "tier";
        tier.is_static = true;
        for (auto const& t : tp.base_tiers) tier.values.push_back(t.id);
        if (!tier.values.empty()) levels.push_back(std::move(tier));
    }

    // Modus auflösen (active_axes = freigegebene Achsen).
    comdare::builder::xml::ThesisMode const* mode = nullptr;
    for (auto const& m : tp.modes)
        if (m.name == mode_name) mode = &m;
    auto is_active = [&](std::string const& ref) -> bool {
        if (mode == nullptr) return true; // kein Modus → alle frei
        for (auto const& a : mode->active_axes)
            if (a == ref) return true;
        return false;
    };

    // 2. Freigegebene permute_axes → statische Ebenen.
    for (auto const& ax : tp.permute_axes) {
        if (!is_active(ax.ref)) continue;
        if (ax.ref == "cacheline") { // compile-time → statische Sub-Ebenen (Binary-Identität)
            if (!ax.line_sizes.empty()) levels.push_back(AxisLevel{"cacheline.line_size", ax.line_sizes, true, ""});
            if (!ax.alignments.empty()) levels.push_back(AxisLevel{"cacheline.alignment", ax.alignments, true, ""});
            if (!ax.sw_prefetch_hints.empty())
                levels.push_back(AxisLevel{"cacheline.sw_hint", ax.sw_prefetch_hints, true, ""});
            continue;
        }
        if (ax.ref == "node_width") { // C2/FF2: Knoten-Breite in Cache-Lines, compile-time → statische Sub-Ebene.
            // Exakt das cacheline-Muster: NUR wenn ein Profil die Achse deklariert UND aktiviert, entsteht eine
            // Ebene (Binary-Identität); ohne Profil-Aktivierung ändert sich KEINE binary_id.
            if (!ax.width_in_lines.empty())
                levels.push_back(AxisLevel{"node_width.width_in_lines", ax.width_in_lines, true, ""});
            continue;
        }
        std::vector<std::string> vals = ax.values; // explizit?
        if (vals.empty()) {                        // sonst volle Liste aus dem Registry
            auto it = registry.find(ax.ref);
            if (it != registry.end()) vals = it->second;
        }
        if (!vals.empty()) levels.push_back(AxisLevel{ax.ref, std::move(vals), true, ""});
    }

    // 3. runtime_dynamic → dynamische Ebenen (Laufzeit-for-Schleife). DIE EINZIGE Quelle dieser Dimensionen
    //    (Inc2 dyn-Dim-Konsolidierung, 2026-06-18): vorher hingen profile_runner::profile_dynamic_dims dieselben
    //    Dimensionen ein ZWEITES Mal an (Doppelquelle). Jetzt emittiert build_axis_levels sie selbst — vollstaendig
    //    + mit den Mess-Pfad-Konventionen (concurrency.thread_count / prefetch.hw_prefetcher + Wiederholungs-Achse
    //    repetition.repetition_index, KF-10). Die binary_id wird davon NICHT beruehrt (is_static=false → der Baum
    //    filtert sie via static_filter() raus; Round-Trip bewiesen). variable/block_id = ComdareResourceControlV1-
    //    Feldname bzw. architektonische Ausnahme (repetition_index ist KEIN POD-Feld → set_field ignoriert es).
    if (!tp.thread_counts.empty())
        levels.push_back(AxisLevel{"concurrency", tp.thread_counts, false, "thread_count", "concurrency"});
    if (!tp.hw_prefetcher.empty())
        levels.push_back(AxisLevel{"prefetch", tp.hw_prefetcher, false, "hw_prefetcher", "prefetch"});
    if (!tp.prefetch_distances.empty())
        levels.push_back(AxisLevel{"prefetch", tp.prefetch_distances, false, "prefetch_distance", "prefetch"});
    if (!tp.pool_budgets_bytes.empty())
        levels.push_back(AxisLevel{"allocator", tp.pool_budgets_bytes, false, "pool_budget_bytes", "allocator"});
    if (!tp.batch_sizes.empty())
        levels.push_back(AxisLevel{"cache_traversal", tp.batch_sizes, false, "batch_size", "cache_traversal"});
    if (!tp.inline_thresholds_bytes.empty())
        levels.push_back(
            AxisLevel{"value_handle", tp.inline_thresholds_bytes, false, "inline_threshold_bytes", "value_handle"});
    // Wiederholungs-Achse (D, KF-10): immer vorhanden (Default 3, separat, nie interpoliert). repetitions<=0 → 1.
    {
        int const                reps = (tp.repetitions <= 0) ? 1 : tp.repetitions;
        std::vector<std::string> rep_vals;
        rep_vals.reserve(static_cast<std::size_t>(reps));
        for (int r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
        levels.push_back(AxisLevel{"repetition", std::move(rep_vals), false, "repetition_index", "repetition"});
    }

    return levels;
}

} // namespace comdare::cache_engine::builder::experiment
