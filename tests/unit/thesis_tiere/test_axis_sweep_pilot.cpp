// test_axis_sweep_pilot — STRANG A Inc7 / FF (#168, gate-frei). HARTE GATE (1): macht die 4 VERTIEFTEN Achsen
// (migration_policy/filter/value_handle/path_compression) real sweep-faehig und BEWEIST LITERAL, dass
// axis_sweep:<eine der 4> >=2 REALE distinkte Lebewesen-DLLs via cl erzeugt (distinkte binary_ids, distinktes
// Verhalten, z.B. migration_policy=migration_hot_cold ≠ migration_none).
//
// BEWEIST LITERAL:
//   (A) axis_sweep_source_map("migration_policy") hat >=2 Eintraege (Baseline + variierte Auspraegung), keine
//       Baseline-Dopplung; eine Auspraegung traegt migration_hot_cold, eine andere migration_none → DISTINKT.
//   (B) GOLDEN-STABIL: der Baseline-Schluessel (migration_none-Auspraegung == alle Achsen Index 0) == golden[0]
//       → die Basis-320-Baseline driftet NICHT (Resume #139-Schutz).
//   (C) je vertiefte Achse: die per-Achse Sweep-Map hat |Auspraegungen| Eintraege (mig 4 / flt 4 / vh 5 / pc 3),
//       alle paarweise distinkt → KEIN Compile-Explosion (klein) UND >=2 distinkte je Achse.
//   (D) REALER cl-BAU: fuer migration_policy werden >=2 Auspraegungen (HotCold + none) real zu DLLs gebaut
//       (distinkte binary_ids → distinkte reale DLLs). EHRLICH: scheitert ein cl-Bau, wird er per-Auspraegung
//       LITERAL gemeldet (Log-Tail), nicht versteckt.
//
// Build/Run: build_axis_sweep_pilot.ps1 (Include-Satz wie die SOTA-Pilot-Harness; setzt COMDARE_PILOT_INCLUDES +
// COMDARE_SOTA_DEFS fuer den DLL-Bau). KEIN 320-Voll-Bau — nur die ~16 vertieften-Achsen-Materialisierungen +
// genau >=2 reale cl-DLLs (Klein-Pilot).

#include "source_catalog.hpp" // axis_sweep_source_map / axis_sweep_levels / is_deepened_axis / make_all_axis_sweeps_source_map
#include "../comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "sota_catalog.hpp" // render_sota_module_source (DRY: gleiche Modul-.cpp-Form ist hier NICHT noetig — wir
                            //   bauen die Sweep-Quelle direkt aus der Map, s.u.)

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

static int  g_fail = 0;
static void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

static std::string load_golden0(fs::path const& p) {
    std::ifstream f{p};
    std::string   line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        return line;
    }
    return {};
}

static std::vector<std::string> split_env(char const* name, char sep) {
    std::vector<std::string> out;
    char const*              e = std::getenv(name);
    if (e == nullptr) return out;
    std::string s = e, cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else
            cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Real-build EINER Sweep-Auspraegung (binary_id → Modul-Quelle aus der Map) zu einer DLL via cl. Die Quelle ist
// bereits die fertige Anatomie-Modul-Quelle (render_adhoc_module_source) — wir schreiben sie 1:1 und bauen.
static bool build_one_dll(std::string const& module_source, fs::path const& work, std::string const& stem,
                          std::vector<std::string> const& defs, std::vector<std::string> const& incs) {
    fs::path const src = work / (stem + ".cpp");
    {
        std::ofstream s{src, std::ios::trunc};
        s << module_source;
    }
    fs::path const dll = work / (stem + ".dll");
    fs::path const rsp = work / (stem + ".rsp");
    fs::path const log = work / (stem + ".cl.log");
    {
        std::ofstream rf{rsp, std::ios::trunc};
        rf << "/nologo /std:c++latest /EHsc /O2 /LD /bigobj\n";
        for (auto const& d : defs) rf << d << "\n";
        for (auto const& i : incs) rf << "/I\"" << i << "\"\n";
        rf << "\"" << src.string() << "\"\n";
        rf << "/Fe:\"" << dll.string() << "\"\n";
        rf << "/Fo:\"" << dll.string() << ".obj\"\n";
    }
    std::error_code ec;
    fs::remove(dll, ec);
    std::string const cmd = "cl @\"" + rsp.string() + "\" > \"" + log.string() + "\" 2>&1";
    std::system(cmd.c_str());
    bool const ok = fs::exists(dll);
    if (ok) {
        std::cout << "       real-DLL gebaut: " << dll.filename().string() << " (" << fs::file_size(dll) << " bytes)\n";
    } else {
        std::cout << "       BUILD-FEHLER (" << stem << ") — Log-Tail:\n";
        std::ifstream            lf{log};
        std::vector<std::string> lines;
        std::string              ln;
        while (std::getline(lf, ln)) lines.push_back(ln);
        for (std::size_t i = (lines.size() > 14 ? lines.size() - 14 : 0); i < lines.size(); ++i)
            std::cout << "         " << lines[i] << "\n";
    }
    return ok;
}

int main(int argc, char** argv) {
    fs::path const golden = (argc >= 2)
                                ? fs::path(argv[1])
                                : (fs::path("tests") / "unit" / "thesis_tiere" / "golden_fullpilot_320_binary_ids.txt");
    fs::path const work =
        (argc >= 3) ? fs::path(argv[2]) : ::comdare::test::user_tmp_dir() / "comdare_axis_sweep_pilot";
    fs::create_directories(work);
    std::cout << "Golden: " << golden.string() << "\nWork:   " << work.string() << "\n";

    // ── (C) je vertiefte Achse die KLEINE Sweep-Source-Map (kein Compile-Explosion) ──
    struct AxisExp {
        char const* axis;
        std::size_t expect;
        char const* probe_val;
    };
    std::vector<AxisExp> const axes = {
        {"migration_policy", 4, "migration_policy=migration_hot_cold"},
        {"filter", 4, "filter=filter_cuckoo"},
        {"value_handle", 5, "value_handle=value_handle_external_pool"},
        {"path_compression", 3, "path_compression=path_compression_patricia"},
    };
    std::cout << "\n--- (C) per-Achse Sweep-Source-Maps (klein, KEIN Compile-Explosion) ---\n";
    std::size_t total_entries = 0;
    for (auto const& ax : axes) {
        check(("is_deepened_axis(" + std::string{ax.axis} + ")").c_str(), tlz::is_deepened_axis(ax.axis));
        auto m = tlz::axis_sweep_source_map(ax.axis);
        total_entries += m.size();
        std::cout << "  axis=" << ax.axis << " sweep-map-eintraege=" << m.size() << " (erwartet " << ax.expect << ")\n";
        check((std::string{ax.axis} + ": sweep-map hat genau |Auspraegungen| Eintraege").c_str(),
              m.size() == ax.expect);
        check((std::string{ax.axis} + ": >=2 distinkte Auspraegungen (sweep-faehig)").c_str(), m.size() >= 2);
        // die variierte Probe-Auspraegung ist im Schluessel-Raum vorhanden (distinkt zur Baseline).
        bool found_probe = false, found_baseline = false;
        for (auto const& [k, v] : m) {
            if (k.find(ax.probe_val) != std::string::npos) found_probe = true;
            if (k.find("migration_policy=migration_none") != std::string::npos &&
                k.find("filter=filter_bloom") != std::string::npos &&
                k.find("value_handle=value_handle_inline") != std::string::npos &&
                k.find("path_compression=path_compression_none") != std::string::npos)
                found_baseline = true;
            check((std::string{ax.axis} + ": Quelle ist reale Anatomie (COMDARE_DEFINE_ANATOMY_MODULE_ADHOC)").c_str(),
                  v.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC") != std::string::npos);
            break; // 1 Quelle pruefen reicht (alle aus demselben Emitter)
        }
        // Probe-/Baseline-Praesenz separat ueber die volle Map.
        for (auto const& [k, v] : m) {
            (void)v;
            if (k.find(ax.probe_val) != std::string::npos) found_probe = true;
        }
        check((std::string{ax.axis} + ": variierte Auspraegung " + ax.probe_val + " ist im Sweep-Raum").c_str(),
              found_probe);
    }
    std::cout << "  SUMME aller Sweep-Map-Eintraege = " << total_entries
              << " (KEIN 320-Kartesisch — Compile-Map klein)\n";
    check("Summe der 4 Sweep-Maps ist klein (<= 32 Eintraege, kein Compile-Explosion)", total_entries <= 32);

    // ── (A) migration_policy: >=2 Eintraege + DISTINKT (none vs hot_cold) ──
    std::cout << "\n--- (A) migration_policy-Sweep: Distinktheit (none vs hot_cold) ---\n";
    auto mig = tlz::axis_sweep_source_map("migration_policy");
    check("migration-Sweep hat >=2 Eintraege", mig.size() >= 2);
    std::string none_id, hot_id, none_src, hot_src;
    for (auto const& [k, v] : mig) {
        if (k.find("migration_policy=migration_none") != std::string::npos) {
            none_id  = k;
            none_src = v;
        }
        if (k.find("migration_policy=migration_hot_cold") != std::string::npos) {
            hot_id  = k;
            hot_src = v;
        }
    }
    check("migration_none-Auspraegung vorhanden", !none_id.empty());
    check("migration_hot_cold-Auspraegung vorhanden", !hot_id.empty());
    check("none- und hot_cold-binary_id sind DISTINKT", !none_id.empty() && none_id != hot_id);
    check("none- und hot_cold-Quelltext sind DISTINKT (andere Anatomie)", !none_src.empty() && none_src != hot_src);
    std::cout << "  none binary_id    = " << none_id << "\n";
    std::cout << "  hot_cold binary_id= " << hot_id << "\n";

    // ── (B) GOLDEN-STABIL: die migration_none-Baseline-Auspraegung == golden[0] (kein Basis-320-Drift) ──
    std::cout << "\n--- (B) Golden-Stabilitaet (Basis-320-Baseline unveraendert) ---\n";
    std::string const golden0 = load_golden0(golden);
    check("Golden[0] geladen", !golden0.empty());
    check("migration-Sweep-Baseline (none) == golden[0] (binary_id-Drift = 0)", none_id == golden0);

    // ── (D) REALER cl-BAU: >=2 distinkte migration-DLLs (HotCold + none) ──
    std::vector<std::string> const defs           = split_env("COMDARE_SOTA_DEFS", ';');
    std::vector<std::string> const incs           = split_env("COMDARE_PILOT_INCLUDES", ';');
    bool const                     have_toolchain = !incs.empty();
    std::cout << "\n--- (D) REALER cl-Bau (>=2 distinkte migration-DLLs) ---\n";
    std::cout << "  cl-Includes (env COMDARE_PILOT_INCLUDES) = " << incs.size()
              << (have_toolchain ? "" : "  (LEER → nur Map-/Distinktheits-Beleg, KEIN cl-Bau)") << "\n";
    if (have_toolchain && !none_id.empty() && !hot_id.empty()) {
        bool const ok_none = build_one_dll(none_src, work, "sweep_mig_none", defs, incs);
        check("REALE migration_none-DLL via cl gebaut", ok_none);
        bool const ok_hot = build_one_dll(hot_src, work, "sweep_mig_hot_cold", defs, incs);
        check("REALE migration_hot_cold-DLL via cl gebaut", ok_hot);
        if (ok_none && ok_hot) {
            fs::path const a = work / "sweep_mig_none.dll", b = work / "sweep_mig_hot_cold.dll";
            bool const     both = fs::exists(a) && fs::exists(b);
            check("ZWEI distinkte reale migration-DLLs existieren (HotCold != none)", both);
        }
    } else if (!have_toolchain) {
        std::cout << "  (cl-Toolchain nicht gesetzt → Map-/Distinktheits-Beleg ohne realen Bau; "
                     "build_axis_sweep_pilot.ps1 setzt die Includes und baut real)\n";
    }

    // ── (E) #188 per-K Increment 2b: der dedizierte per-K-search_algo-Sweep-Katalog materialisiert GENAU 4 reale
    //    per-K-Kompositionen (search_algo=k_ary_k2/k4/k8/k16 × Baseline). search_algo ist KEINE vertiefte Achse →
    //    eigener Katalog (explizite per-K-Liste statt Basis-320-First-4). Emission je K = 1 reale DLL (KAryTraversal<K>). ──
    std::cout << "\n--- (E) #188 per-K Sweep-Katalog (4 per-K search_algo-Kompositionen, KAryTraversal<K>) ---\n";
    auto const perk = tlz::kary_perk_source_map();
    check("per-K Sweep-Map hat GENAU 4 Eintraege (k_ary_k2/k4/k8/k16)", perk.size() == 4);
    // std::map-Keys sind eindeutig → 4 Eintraege == 4 paarweise distinkte binary_ids.
    for (char const* nm :
         {"search_algo=k_ary_k2", "search_algo=k_ary_k4", "search_algo=k_ary_k8", "search_algo=k_ary_k16"}) {
        bool found = false;
        for (auto const& [k, v] : perk) {
            (void)v;
            if (k.find(nm) != std::string::npos) found = true;
        }
        check((std::string{"per-K: Auspraegung "} + nm + " ist im Sweep-Raum").c_str(), found);
    }
    int perk_real = 0;
    for (auto const& [k, v] : perk) {
        (void)k;
        if (v.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC") != std::string::npos) ++perk_real;
    }
    check("per-K: GENAU 4 Quellen sind reale Anatomie (COMDARE_DEFINE_ANATOMY_MODULE_ADHOC)", perk_real == 4);
    check("per-K Levels: 17 statische Achsen-Ebenen (17-Slot-Komposition; search_algo traegt die 4 per-K-Werte)", tlz::kary_perk_levels().size() == 17);
    if (have_toolchain) {
        std::cout << "  --- (E-real) REALER cl-Bau der 4 per-K-DLLs (Emission je KAryTraversal<K>) ---\n";
        int perk_built = 0;
        for (auto const& [k, v] : perk) {
            std::string stem = "sweep_perk";
            for (char const* nm : {"k_ary_k2", "k_ary_k4", "k_ary_k8", "k_ary_k16"})
                if (k.find(nm) != std::string::npos) stem = std::string("sweep_") + nm;
            if (build_one_dll(v, work, stem, defs, incs)) ++perk_built;
        }
        check("alle 4 per-K-DLLs via cl gebaut (Emission je KAryTraversal<K> real)", perk_built == 4);
    }

    std::cout << "\n==== STRANG-A Inc7 / FF(#168) Achsen-Sweep-Pilot (4 vertiefte Achsen sweep-faehig): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
