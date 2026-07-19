#pragma once
// gn_cell_filter.hpp -- W5-C+ (§36.1 Zellen-Locking-Doktrin, 2026-07-19). Der GN-Zellen-Filter der
// opt×simd-Delegations-Naht (profile_run_entry.hpp run_profile-Walk), herausgeloest als SCHLANKER Leaf-Header.
//
// BEFUND (Pipeline 11453): die CI-Matrix exportiert je Cluster-Zelle COMDARE_GN_OPT/COMDARE_GN_SIMD (z.B. O2 +
// no_extension), aber ohne Konsum faehrt run_profile in JEDER Zelle ALLE Profil-System-Perms (4-fach-redundanter
// Bau; die §36.1-Locking-Doktrin -- eine System-Permutation genau EINMAL im Cluster -- wird wirkungslos).
//
// Die zwei Praedikate sind die EINE Single-Source der Zellen-Zulassung: der run_profile-Walk (Feuerung + Skip-Log)
// UND der Zaehl-Test nutzen dieselben Funktionen -> driftfrei. Leerer Filter = kein Filter = alle Zellen
// (byte-neutrales Ist-Verhalten). Bewusst umbrella-FREI (nur stdlib) -> direkt testbar ohne Katalog/DLL-Bau.
// Header-only, C++23, ASCII-only.

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

/// true, wenn die opt-Zelle vom GN-Filter zugelassen ist (leerer Filter = alle opt-Zellen).
[[nodiscard]] inline bool gn_cell_opt_allowed(std::string const& gn_cell_opt, std::string_view opt_id) noexcept {
    return gn_cell_opt.empty() || gn_cell_opt == opt_id;
}
/// true, wenn die simd-Zelle vom GN-Filter zugelassen ist (leerer Filter = alle simd-Zellen).
[[nodiscard]] inline bool gn_cell_simd_allowed(std::string const& gn_cell_simd, std::string_view simd_id) noexcept {
    return gn_cell_simd.empty() || gn_cell_simd == simd_id;
}

/// gn_walk_cells -- die (opt,simd)-Zellen, die der opt×simd-Walk real baut: GN-Zellen-Filter UND ISA-Gate
/// (host_supports) angewandt, in Walk-Reihenfolge (opt aussen, simd innen). Freistehend + testbar (KEIN DLL-Bau)
/// -- dieselben Praedikate wie der Walk, daher driftfrei. host_supports(simd_id) = die ISA-Zulassung
/// (system_axis_host_supports_simd; im Test ein Mock-Praedikat).
template <class HostSupports>
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
gn_walk_cells(std::vector<std::string> const& opt_perms, std::vector<std::string> const& simd_perms,
              std::string const& gn_cell_opt, std::string const& gn_cell_simd, HostSupports host_supports) {
    std::vector<std::pair<std::string, std::string>> cells;
    for (auto const& opt_id : opt_perms) {
        if (!gn_cell_opt_allowed(gn_cell_opt, opt_id)) continue;
        for (auto const& simd_id : simd_perms) {
            if (!gn_cell_simd_allowed(gn_cell_simd, simd_id)) continue;
            if (!host_supports(simd_id)) continue; // ISA-Gate: eine host-unfaehige Zelle baut nichts (ehrlich)
            cells.emplace_back(opt_id, simd_id);
        }
    }
    return cells;
}

} // namespace comdare::cache_engine::thesis_lazy
