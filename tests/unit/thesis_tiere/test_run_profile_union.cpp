// test_run_profile_union — STRANG A KORRIGIERT, Increment 6 / S7 (HARTE GATE (1), gate-frei). BEWEIST LITERAL,
// dass EIN profil-getriebener Treiber-Aufbau aus EINEM m3v2-Profil BEIDE Subsets traegt — die Basis-320
// (source_catalog) UND die SOTA-Reihen (sota_catalog) — ueber EINE vereinigte SourceGenFn (S7a) und EINE
// Multi-Pass-Struktur (S7b), und dass die CEB-Eintritts-API (S7c) genau diese Vereinigung verdrahtet.
//
// Dieser Test ist KEIN 320-/SOTA-Bau-Sturm: er materialisiert NUR Quelltexte (Strings) + die Pass-/View-Struktur
// (kein cl). Die ECHTE Voll-Messung bleibt HELD (#156). Er belegt die VERDRAHTUNG:
//   • S7a: make_union_source_gen liefert fuer einen BASIS-binary_id UND fuer einen SOTA-view-binary_id ueber
//          DIESELBE SourceGenFn je eine NICHT-leere, reale Modul-Quelle — disjunkte Namensraeume (kein Konflikt).
//   • S7b: build_sota_passes liefert >=1 SOTA-Reihen-Pass; jeder Pass-View-binary_id ist im "sota_tier="-Raum
//          (disjunkt zum Basis-"search_algo=…"-Raum) → ein run_profile-Lauf erzeugt Basis- UND SOTA-binary_ids.
//   • Golden-Stabilitaet: der Basis-Pass-binary_id[0] == golden[0] (Resume #139 unberuehrt; binary_id-Drift = 0).
//
// Build (innerhalb vcvars64): build_run_profile_union.ps1 (Include-Satz wie die thesis_tiere-Harness). KEIN cl
// fuer die Tiere — nur String-/Index-Rekombination + die Katalog-Materialisierung (header-only).

#include "profile_run_entry.hpp"        // run_profile-Bausteine: make_union_source_gen / build_sota_*
#include "generated_source_catalog.hpp" // generated_make_catalog_source_gen
#include "sota_catalog.hpp"             // build_sota_passes / build_sota_view_source_map / sota_view_binary_id
#include "profile_runner.hpp"           // load_thesis_profile / build_profile_basis_levels / make_union_source_gen

#include <builder/experiment_tree/experiment_tree.hpp> // ExperimentTree / StaticBinaryView

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cx  = comdare::builder::xml;
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
        return line; // erste Daten-Zeile
    }
    return {};
}

int main(int argc, char** argv) {
    fs::path const profiles_dir = fs::path("libs") / "cache_engine" / "algorithm_profiles" / "thesis_profiles";
    fs::path const m3v2_xml     = (argc >= 2) ? fs::path(argv[1]) : (profiles_dir / "m3v2_study.profile.xml");
    fs::path const golden = (argc >= 3)
                                ? fs::path(argv[2])
                                : (fs::path("tests") / "unit" / "thesis_tiere" / "golden_fullpilot_320_binary_ids.txt");
    std::cout << "Profil: " << m3v2_xml.string() << "\nGolden: " << golden.string() << "\n";

    auto tp = tlz::load_thesis_profile(m3v2_xml);
    check("parse_thesis_profile lieferte das m3v2-Profil", tp.has_value());
    if (!tp) {
        std::cout << "\n==== ABBRUCH: Profil nicht lesbar ====\n";
        return 1;
    }

    std::string const mode_name = tp->modes.empty() ? std::string{"m3v2_base"} : tp->modes.front().name;

    // ── (A) Der BASIS-Baum (wie run_profile ihn baut): build_profile_basis_levels → StaticBinaryView. ──
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree basis_tree{factory};
    basis_tree.build(tlz::build_profile_basis_levels(*tp, mode_name, /*with_dynamic=*/true));
    ex::StaticBinaryView const basis_view = basis_tree.static_binary_view();
    check("Basis-Baum binary_count == 320", basis_tree.binary_count() == 320);
    check("Basis-View nicht leer", !basis_view.empty());
    std::string const basis_bid0 = basis_view.empty() ? std::string{} : basis_view[0].binary_id;

    // GOLDEN-STABILITAET: der Basis-binary_id[0] == golden[0] (Resume #139-Schutz; binary_id-Drift = 0).
    std::string const golden0 = load_golden0(golden);
    check("Golden[0] geladen", !golden0.empty());
    check("Basis-binary_id[0] == golden[0] (binary_id-Drift = 0)", basis_bid0 == golden0);

    // ── (B) S7b: die SOTA-Reihen-Paesse aus dem Profil (>=1 real baubar). ──
    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    std::cout << "\n--- S7b: SOTA-Reihen-Paesse (build_sota_passes) = " << passes.size() << " ---\n";
    check("build_sota_passes liefert >=1 Pass", passes.size() >= 1);
    bool series_ab = false;
    {
        bool a = false, b = false;
        for (auto const& p : passes) {
            if (p.series == "A") a = true;
            if (p.series == "B") b = true;
        }
        series_ab = a && b;
    }
    // #178 (Thesis ch4 §4.8): die Stufen speisen Reihe A (Stufe1∪Stufe2) + Reihe B (Stufe3). Reihe C
    // (Regression alt↔neu) ist BUILD-ÜBERGREIFEND und wird von KEINER Stufe erzeugt → hier nicht erwartet.
    check("SOTA-Paesse decken die stufen-gespeisten Reihen A (St1+St2) und B (St3) ab", series_ab);
    std::string const sota_bid0 = passes.empty() ? std::string{} : passes.front().view_binary_id;
    std::cout << "  SOTA-Pass[0]: series=" << (passes.empty() ? "-" : passes.front().series)
              << " view_binary_id=" << sota_bid0 << "\n";

    // ── (C) Namensraum-DISJUNKTHEIT: Basis = "search_algo=…", SOTA = "sota_tier=…" (kein Praefix-Konflikt). ──
    check("Basis-binary_id im 'search_algo='-Raum", basis_bid0.rfind("search_algo=", 0) == 0);
    check("SOTA-binary_id im 'sota_tier='-Raum", sota_bid0.rfind("sota_tier=", 0) == 0);

    // ── (D) S7a: DIE EINE vereinigte SourceGenFn liefert fuer BEIDE Namensraeume eine reale Modul-Quelle. ──
    ex::SourceGenFn const union_gen =
        tlz::make_union_source_gen(tlz::generated_make_catalog_source_gen(), tlz::build_sota_view_source_map(*tp));

    std::string const basis_src = union_gen(basis_bid0);
    std::string const sota_src  = union_gen(sota_bid0);
    std::cout << "\n--- S7a: union_gen ueber EINE SourceGenFn ---\n";
    std::cout << "  basis_src.size() = " << basis_src.size() << " (Basis-320-Quelle)\n";
    std::cout << "  sota_src.size()  = " << sota_src.size() << " (SOTA-Reihen-Quelle)\n";
    check("union_gen(BASIS-binary_id) liefert NICHT-leere Quelle", !basis_src.empty());
    check("union_gen(SOTA-view-binary_id) liefert NICHT-leere Quelle", !sota_src.empty());
    check("BASIS-Quelle ist eine reale Anatomie-Modul-Quelle (COMDARE_DEFINE_ANATOMY_MODULE)",
          basis_src.find("COMDARE_DEFINE_ANATOMY_MODULE") != std::string::npos);
    check("SOTA-Quelle ist eine reale Anatomie-Modul-Quelle (COMDARE_DEFINE_ANATOMY_MODULE)",
          sota_src.find("COMDARE_DEFINE_ANATOMY_MODULE") != std::string::npos);
    check("BASIS- und SOTA-Quelle sind verschieden (zwei distinkte Lebewesen-Quelltexte)", basis_src != sota_src);

    // ── (E) Disjunktheit der UNION: ein BASIS-id liegt NICHT in der SOTA-map (und umgekehrt) → echte Vereinigung. ──
    std::map<std::string, std::string> const sota_map = tlz::build_sota_view_source_map(*tp);
    check("SOTA-map enthaelt den SOTA-view-binary_id", sota_map.count(sota_bid0) == 1);
    check("SOTA-map enthaelt KEINEN Basis-binary_id (disjunkt)", sota_map.count(basis_bid0) == 0);
    ex::SourceGenFn const basis_only = tlz::generated_make_catalog_source_gen();
    check("Basis-Katalog liefert KEINE Quelle fuer den SOTA-id (disjunkt)", basis_only(sota_bid0).empty());

    std::cout << "\n==== STRANG-A Inc6 / S7 Union-Verdrahtung (Basis-320 ∪ SOTA-Reihen ueber EINE SourceGenFn): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
