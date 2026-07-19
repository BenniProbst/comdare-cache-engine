// test_gn_cell_filter.cpp -- W5-C+ (§36.1 Zellen-Locking, 2026-07-19). Gate fuer den GN-Zellen-Filter der
// opt×simd-Delegations-Naht (profile_facade/gn_cell_filter.hpp -- die EINE Single-Source). BEIDE offiziellen
// XML-Lauf-Pfade nutzen dieselbe Funktion: run_profile (comdare_thesis_profile) UND run_experiment_profile
// (comdare_experiment) -> dieser eine Zaehl-Test deckt beide Walks ab (siehe Fall (8)). Befund Pipeline 11453:
// ohne Filter faehrt JEDER Walk in JEDER Cluster-Zelle ALLE System-Perms (4-fach-redundanter Bau). Der Filter
// (COMDARE_GN_OPT/COMDARE_GN_SIMD) muss die Zelle auf GENAU EINE (opt,simd)-Perm einschraenken; leerer Filter =
// alle Perms (byte-neutrales Ist-Verhalten).
//
// ZAEHL-TEST OHNE DLL-BAU (der vom Auftrag vorgesehene Schnitt): gn_walk_cells liefert die (opt,simd)-Zellen, die
// der Walk real baut (GN-Filter + ISA-Gate). Bewusst umbrella-FREI (nur der Leaf-Header). Plain-main, exit 0 = OK.

#include "gn_cell_filter.hpp" // gn_cell_opt_allowed / gn_cell_simd_allowed / gn_walk_cells

#include <iostream>
#include <string>
#include <vector>

namespace tlz = ::comdare::cache_engine::thesis_lazy;

namespace {

int g_fail = 0;

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << '\n';
    if (!ok) ++g_fail;
}

void check(char const* what, bool ok) {
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << '\n';
    if (!ok) ++g_fail;
}

} // namespace

int main() {
    std::cout << "==== W5-C+ GN-Zellen-Filter Gate (§36.1 Zellen-Locking) ====\n";

    // Ein realistisches Profil-System-Achsen-Kreuz: opt {O2,O3} x simd {no_extension, avx2, avx512}.
    std::vector<std::string> const opt_perms{"O2", "O3"};
    std::vector<std::string> const simd_perms{"no_extension", "avx2", "avx512"};
    // Mock-ISA-Zulassung: no_extension + avx2 verfuegbar, avx512 NICHT (z.B. fused-off-Host / prod2).
    auto host_supports = [](std::string_view simd) { return simd == "no_extension" || simd == "avx2"; };

    // (1) Kein Filter (leer,leer): ALLE host-zulaessigen Perms -> {O2,O3} x {no_extension,avx2} = 4 (avx512 ISA-gated).
    std::cout << "== (1) leerer Filter -> alle host-zulaessigen Perms ==\n";
    auto const all = tlz::gn_walk_cells(opt_perms, simd_perms, "", "", host_supports);
    check_eq("(1) leerer Filter -> 4 Zellen (avx512 vom ISA-Gate raus)", all.size(), std::size_t{4});

    // (2) opt-Filter gesetzt -> nur die O2-Zeile: {no_extension,avx2} = 2.
    std::cout << "== (2) gn_cell_opt=O2 -> nur die O2-Zeile ==\n";
    auto const only_o2 = tlz::gn_walk_cells(opt_perms, simd_perms, "O2", "", host_supports);
    check_eq("(2) gn_cell_opt=O2 -> 2 Zellen", only_o2.size(), std::size_t{2});
    check("(2) jede Zelle hat opt==O2", only_o2.size() == 2 && only_o2[0].first == "O2" && only_o2[1].first == "O2");

    // (3) simd-Filter gesetzt -> nur die avx2-Spalte: {O2,O3} = 2.
    std::cout << "== (3) gn_cell_simd=avx2 -> nur die avx2-Spalte ==\n";
    auto const only_avx2 = tlz::gn_walk_cells(opt_perms, simd_perms, "", "avx2", host_supports);
    check_eq("(3) gn_cell_simd=avx2 -> 2 Zellen", only_avx2.size(), std::size_t{2});
    check("(3) jede Zelle hat simd==avx2",
          only_avx2.size() == 2 && only_avx2[0].second == "avx2" && only_avx2[1].second == "avx2");

    // (4) BEIDE gesetzt -> GENAU 1 Zelle (der Kalibrierungs-Fall: O2 + no_extension).
    std::cout << "== (4) gn_cell_opt=O2 + gn_cell_simd=no_extension -> GENAU 1 Zelle ==\n";
    auto const one = tlz::gn_walk_cells(opt_perms, simd_perms, "O2", "no_extension", host_supports);
    check_eq("(4) beide Filter -> GENAU 1 Zelle (§36.1 Kern-Gate)", one.size(), std::size_t{1});
    check("(4) die eine Zelle ist (O2,no_extension)",
          one.size() == 1 && one[0].first == "O2" && one[0].second == "no_extension");

    // (5) BEIDE gesetzt, andere Zelle (O3,avx2) -> GENAU 1, korrekter Inhalt.
    std::cout << "== (5) gn_cell_opt=O3 + gn_cell_simd=avx2 -> GENAU 1 Zelle (O3,avx2) ==\n";
    auto const one_b = tlz::gn_walk_cells(opt_perms, simd_perms, "O3", "avx2", host_supports);
    check("(5) genau (O3,avx2)", one_b.size() == 1 && one_b[0].first == "O3" && one_b[0].second == "avx2");

    // (6) simd-Filter auf host-UNzulaessige Zelle (avx512) -> 0 Zellen (ISA-Gate schlaegt VOR dem Bau zu).
    std::cout << "== (6) gn_cell_simd=avx512 auf fused-off-Host -> 0 Zellen (ISA-Gate) ==\n";
    auto const avx512 = tlz::gn_walk_cells(opt_perms, simd_perms, "", "avx512", host_supports);
    check_eq("(6) avx512 host-unfaehig -> 0 Zellen (ehrlich, kein Bau)", avx512.size(), std::size_t{0});
    auto const o2_avx512 = tlz::gn_walk_cells(opt_perms, simd_perms, "O2", "avx512", host_supports);
    check_eq("(6) O2+avx512 host-unfaehig -> 0 Zellen", o2_avx512.size(), std::size_t{0});

    // (7) die Praedikate direkt (Single-Source der Walk-Skip-Entscheidung).
    std::cout << "== (7) Praedikate: leerer Filter laesst alles, gesetzter nur den Match ==\n";
    check("(7) opt: leerer Filter laesst 'O2' zu", tlz::gn_cell_opt_allowed("", "O2"));
    check("(7) opt: Filter 'O2' laesst 'O2' zu", tlz::gn_cell_opt_allowed("O2", "O2"));
    check("(7) opt: Filter 'O2' verwirft 'O3'", !tlz::gn_cell_opt_allowed("O2", "O3"));
    check("(7) simd: leerer Filter laesst 'avx2' zu", tlz::gn_cell_simd_allowed("", "avx2"));
    check("(7) simd: Filter 'no_extension' verwirft 'avx2'", !tlz::gn_cell_simd_allowed("no_extension", "avx2"));

    // (8) EXPERIMENT-WALK-ABDECKUNG: run_experiment_profile (comdare_experiment) nutzt DIESELBE gn_walk_cells-
    // Single-Source wie run_profile. Ein comdare_experiment-typisches Kreuz (O3 x {no_extension,avx2}) mit
    // gesetztem Filter kollabiert IDENTISCH auf 1 Zelle -> der Spiegel-Walk baut ebenfalls nur die eine Perm.
    std::cout << "== (8) Experiment-Walk (run_experiment_profile) teilt dieselbe Single-Source ==\n";
    std::vector<std::string> const exp_opt{"O3"};
    std::vector<std::string> const exp_simd{"no_extension", "avx2"};
    auto const                     exp_all = tlz::gn_walk_cells(exp_opt, exp_simd, "", "", host_supports);
    check_eq("(8) experiment-Kreuz ohne Filter -> 2 Zellen", exp_all.size(), std::size_t{2});
    auto const exp_one = tlz::gn_walk_cells(exp_opt, exp_simd, "O3", "avx2", host_supports);
    check("(8) experiment-Kreuz + Filter -> GENAU (O3,avx2)",
          exp_one.size() == 1 && exp_one[0].first == "O3" && exp_one[0].second == "avx2");

    std::cout << (g_fail == 0 ? "\nGN_CELL_FILTER_OK\n" : "\nGN_CELL_FILTER_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
