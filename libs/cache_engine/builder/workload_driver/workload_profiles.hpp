#pragma once
// workload_profiles.hpp — Single-Source-Mapper Profil-Token → WorkloadConfig (INC-0, 2026-06-07).
//
// Hebt das bisher in apps/f15_compare/main.cpp (anonymer Namespace) dupliziert vorliegende profile_by_name in
// einen geteilten Header, damit f15_compare (CLI) UND der FullPilot-B+-Baum-Mess-Pfad (perm_runner::run_workload_perm)
// DENSELBEN Mapper nutzen — keine Profil-Drift, eine Source-of-Truth. Workload ist die dynamische Achse 2 des
// kartesischen Mess-Kreuzes (messarchitektur_v5_design.md §73 „LASTENPROFIL").
//
// Tokens: A/B/C/D/E/F (YCSB-treu, Zipfian/Latest) + IH/LH (Insert/Lookup-Heavy) + OP1..OP6 (Op-Mix-Profile, INC-6).
// @related [[project_workload_achse2_dynamic_bplus]]

#include "workload_config.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::workload_driver {

/// profile_by_name — bildet ein Lastprofil-Token (CLI/B+-Baum-Label) auf eine WorkloadConfig ab.
/// Unbekanntes Token → WorkloadConfig mit leerem name (Marker für „überspringen"). Deterministisch: gleiche
/// (Token, seed, ops) ⇒ gleiche Config ⇒ (via WorkloadGenerator) bit-identische Op-Sequenz über alle Binaries.
[[nodiscard]] inline WorkloadConfig profile_by_name(std::string_view tok,
                                                    std::uint64_t seed,
                                                    std::size_t  ops) {
    if (tok == "A"  || tok == "mixed_a")      return make_mixed_a(seed, ops);
    if (tok == "B"  || tok == "mixed_b")      return make_mixed_b(seed, ops);
    if (tok == "C"  || tok == "ycsb_c")       return make_ycsb_c(seed, ops);
    if (tok == "D"  || tok == "ycsb_d")       return make_ycsb_d(seed, ops);
    if (tok == "E"  || tok == "ycsb_e")       return make_ycsb_e(seed, ops);   // V5-#49-E (Range-Scan)
    if (tok == "F"  || tok == "ycsb_f")       return make_ycsb_f(seed, ops);   // V5-#49-F (Read-Modify-Write)
    if (tok == "IH" || tok == "insert_heavy") return make_insert_heavy(seed, ops);
    if (tok == "LH" || tok == "lookup_heavy") return make_lookup_heavy(seed, ops);
    WorkloadConfig empty; empty.name = "";  // Marker: unbekanntes Profil
    return empty;
}

}  // namespace comdare::cache_engine::builder::workload_driver
