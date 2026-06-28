// test_sota_series_pilot — STRANG A KORRIGIERT, Increment 5 / S6 (Klein-Pilot). Die HARTE GATE (1):
// baut je Stufe 1/2/3 >=1 REALE SOTA/PRT-ART-Lebewesen-DLL (echter cl-Bau, distinkte binary_id). #178: die Reihe
// (A/B) wird aus der Stufe abgeleitet (Stufe1∪Stufe2→A, Stufe3→B); Reihe C ist build-übergreifend (keine Stufe).
//
// BEWEIST LITERAL:
//   • S6a: die benannten SOTA + PRT-ART sind im Katalog materialisierbar (binary_id → reale Modul-Quelle).
//   • S6b: die 3 Stufen (pruefling_merge: Stufe1/Stufe2/Stufe3) erzeugen je eine reale, distinkte Lebewesen-DLL.
//   • Distinktheit: die 3 Stufen-binary_ids sind paarweise verschieden (distinkte Kompositionen, nicht nur Tag).
//   • EHRLICH: falls ein cl-Bau scheitert, wird das per Stufe LITERAL gemeldet (Log-Tail), nicht versteckt.
//
// Dieser Test RUFT cl SELBST auf (innerhalb vcvars64): er rendert je Reihe das Modul-.cpp aus dem Katalog
// (render_sota_module_source) und baut es real zu einer DLL — kein „nur Tag". KEIN 320/voller SOTA-Sturm:
// genau 1 Lebewesen je Reihe (Klein-Pilot). Include-Satz wie die 150er-Harness (build_sota_pilot.ps1).

#include "sota_catalog.hpp"     // build_sota_source_map / sota_module_for / render_sota_module_source (S6)
#include "profile_runner.hpp"   // load_thesis_profile

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cx  = comdare::builder::xml;
namespace fs  = std::filesystem;

static int g_fail = 0;
static void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// Render + real-build EINES SOTA/PRT-ART-Moduls zu einer DLL via cl (Response-File, wie run_lazy_150).
// Gibt true zurück, wenn die DLL real entstanden ist. defs/incs kommen aus dem env (von der Harness gesetzt).
static bool build_one_dll(std::string const& fq_type, std::string const& header,
                          fs::path const& work, std::string const& stem,
                          std::vector<std::string> const& defs, std::vector<std::string> const& incs) {
    fs::path const src = work / (stem + ".cpp");
    {
        std::ofstream s{src, std::ios::trunc};
        s << tlz::render_sota_module_source(fq_type, header);
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
    std::error_code ec; fs::remove(dll, ec);
    std::string const cmd = "cl @\"" + rsp.string() + "\" > \"" + log.string() + "\" 2>&1";
    std::system(cmd.c_str());
    bool const ok = fs::exists(dll);
    if (ok) {
        std::cout << "       real-DLL gebaut: " << dll.filename().string()
                  << " (" << fs::file_size(dll) << " bytes)\n";
    } else {
        std::cout << "       BUILD-FEHLER (" << stem << ") — Log-Tail:\n";
        std::ifstream lf{log};
        std::vector<std::string> lines; std::string ln;
        while (std::getline(lf, ln)) lines.push_back(ln);
        for (std::size_t i = (lines.size() > 12 ? lines.size() - 12 : 0); i < lines.size(); ++i)
            std::cout << "         " << lines[i] << "\n";
    }
    return ok;
}

static std::vector<std::string> split_env(char const* name, char sep) {
    std::vector<std::string> out;
    char const* e = std::getenv(name);
    if (e == nullptr) return out;
    std::string s = e, cur;
    for (char c : s) { if (c == sep) { if (!cur.empty()) out.push_back(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

int main(int argc, char** argv) {
    fs::path const profiles_dir = fs::path("libs") / "cache_engine" / "algorithm_profiles" / "thesis_profiles";
    fs::path const m3v2_xml = (argc >= 2) ? fs::path(argv[1]) : (profiles_dir / "m3v2_study.profile.xml");
    fs::path const work     = (argc >= 3) ? fs::path(argv[2]) : fs::temp_directory_path() / "comdare_sota_pilot";
    fs::create_directories(work);
    std::cout << "Profil: " << m3v2_xml.string() << "\nWork:   " << work.string() << "\n";

    auto tp = tlz::load_thesis_profile(m3v2_xml);
    check("parse_thesis_profile lieferte das m3v2-Profil", tp.has_value());
    if (!tp) { std::cout << "\n==== ABBRUCH: Profil nicht lesbar ====\n"; return 1; }
    check("Profil deklariert <sota_series> (3x7=21)", tp->sota_series.size() == 21);

    // ── S6a: der Katalog materialisiert die deklarierten (Reihe,Lebewesen) → reale Modul-Quelle ──
    std::map<std::string, std::string> const src_map = tlz::build_sota_source_map(*tp);
    std::cout << "\n--- S6a: SOTA-Katalog-Quellen (binary_id → reale Modul-Quelle) = " << src_map.size() << " distinkt ---\n";
    check("S6a: Katalog hat >=3 distinkte SOTA/PRT-ART-Quellen", src_map.size() >= 3);

    // ── Klein-Pilot: 1 Lebewesen je Reihe (echter cl-Bau). defs/incs aus env (Harness). ──
    std::vector<std::string> const defs = split_env("COMDARE_SOTA_DEFS", ';');
    std::vector<std::string> const incs = split_env("COMDARE_PILOT_INCLUDES", ';');
    bool const have_toolchain = !incs.empty();
    std::cout << "  cl-Includes (env COMDARE_PILOT_INCLUDES) = " << incs.size()
              << (have_toolchain ? "" : "  (LEER → nur Katalog-Beleg, KEIN cl-Bau)") << "\n";

    // #178: je STUFE (merge) das ERSTE im Profil deklarierte Lebewesen (profil-getrieben). Die Stufe ist die
    // mechanische Wahrheit; die Reihe wird daraus abgeleitet (Thesis ch4 §4.8: Stufe1∪Stufe2→A, Stufe3→B;
    // Reihe C ist build-übergreifend und an KEINE Stufe gebunden → hier nicht vertreten).
    struct StufeCase { char const* merge; char const* expect_reihe; };
    StufeCase const stufen[] = {
        {"Stufe1_CeOnly",           "A"},
        {"Stufe2_PrueflingReplace", "A"},
        {"Stufe3_FullJoin",         "B"},
    };
    std::map<std::string, std::string> first_of_merge;      // merge → lebewesen
    for (auto const& s : tp->sota_series)
        if (!first_of_merge.count(s.merge)) first_of_merge[s.merge] = s.lebewesen;
    check("Profil deklariert die Stufen 1, 2 und 3",
          first_of_merge.count("Stufe1_CeOnly") && first_of_merge.count("Stufe2_PrueflingReplace")
              && first_of_merge.count("Stufe3_FullJoin"));

    std::map<std::string, std::string> built_binary_id;     // merge → binary_id (für Distinktheits-Check)
    int real_built = 0;
    for (auto const& sc : stufen) {
        auto it = first_of_merge.find(sc.merge);
        if (it == first_of_merge.end()) continue;
        std::string const reihe = tlz::stufe_to_reihe(sc.merge);
        auto mod = tlz::sota_module_for(sc.merge, it->second);   // #178: dispatch auf die Stufe (merge)
        std::cout << "\n--- Stufe " << sc.merge << " → Reihe " << reihe
                  << " (Lebewesen=" << it->second << "): ";
        check((std::string{"Stufe "} + sc.merge + ": Katalog liefert ein reales Lebewesen-Modul").c_str(),
              mod.has_value());
        check((std::string{"Stufe "} + sc.merge + " → Reihe " + sc.expect_reihe + " (stufe_to_reihe)").c_str(),
              reihe == sc.expect_reihe);
        if (!mod) continue;
        std::cout << "       binary_id = " << mod->binary_id.substr(0, 110) << "...\n";
        std::cout << "       type      = " << mod->composition_type << "\n";
        built_binary_id[sc.merge] = mod->binary_id;
        if (have_toolchain) {
            bool const ok = build_one_dll(mod->composition_type, mod->header, work,
                                          std::string{"sota_"} + reihe + "_" + it->second, defs, incs);
            check((std::string{"Stufe "} + sc.merge + ": REALE Lebewesen-DLL via cl gebaut (echter Bau)").c_str(), ok);
            if (ok) ++real_built;
        }
    }

    // ── HARTE GATE (1) Teil 2: die 3 Stufen-binary_ids sind paarweise DISTINKT (distinkte Kompositionen) ──
    std::cout << "\n--- Distinktheit der 3 Stufen-binary_ids ---\n";
    bool distinct = built_binary_id.size() == 3;
    if (distinct) {
        distinct = (built_binary_id["Stufe1_CeOnly"]           != built_binary_id["Stufe2_PrueflingReplace"]) &&
                   (built_binary_id["Stufe1_CeOnly"]           != built_binary_id["Stufe3_FullJoin"]) &&
                   (built_binary_id["Stufe2_PrueflingReplace"] != built_binary_id["Stufe3_FullJoin"]);
    }
    check("Stufe1/2/3 binary_ids paarweise distinkt (distinkte Kompositionen, nicht nur Tag)", distinct);

    if (have_toolchain) check("Klein-Pilot: je Stufe 1/2/3 >=1 reale DLL gebaut (3 real)", real_built == 3);
    else std::cout << "  (cl-Toolchain nicht gesetzt → Katalog-/Distinktheits-Beleg ohne realen Bau; die "
                      "build_sota_pilot.ps1-Harness setzt die Includes und baut real)\n";

    std::cout << "\n==== STRANG-A Inc5 / S6 SOTA-Reihen-Pilot: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
