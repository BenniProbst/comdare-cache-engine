// test_profile_roundtrip — STRANG A KORRIGIERT, Increment 1..4. Die HARTE Round-Trip-Gate (S3/S4a/S8a).
//
// BEWEIST LITERAL (Resume #139-Schutz): der PROFIL-getriebene Pfad (m3v2_study.profile.xml → parse_thesis_profile →
// build_axis_levels → StaticBinaryView) erzeugt EXAKT die committete GOLDEN-Liste der 320 binary_ids
// (golden_fullpilot_320_binary_ids.txt, S4a). Diff == leer.
//
// STRANG A Inc4/S5 (DIE ENTFERNUNG): dieser Test referenziert die ENTFERNTE Code-Selektions-Schicht NICHT mehr —
// keine geloeschten Selektions-Header. Die 320er-Referenz ist durch die committete GOLDEN-Datei ersetzt; die
// Selektion kommt aus profile_runner (profile_select), nicht aus einer Code-Selektions-Quelle. Die Golden-Datei
// wurde EINMAL aus dem Katalog-Pfad erzeugt (gen_golden_fullpilot.cpp) und ist eingefroren.
//
// Build (innerhalb vcvars64): build_profile_roundtrip.ps1 (Include-Satz wie thesis_tiere-Harness) — KEIN 320-Voll-
// Bau, nur String-/Index-Rekombination über den StaticBinaryView (kein cl/Anatomie).

#include "profile_runner.hpp" // build_profile_levels / build_profile_basis_levels / profile_select (Inc1/Inc3)

#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (17 Slot-Namen, Single-Source)
#include <builder/experiment_tree/experiment_tree.hpp>         // ExperimentTree / StaticBinaryView

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cx  = comdare::builder::xml;
namespace fs  = std::filesystem;

static int  g_fail = 0;
static void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}
template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// ExperimentTree::static_binary_view() → die geordnete binary_id-Liste. Der Baum filtert intern static_filter()
// (is_static) — dynamische Ebenen verschmutzen die binary_id NICHT. Das ist GENAU der Produktiv-Pfad.
static std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& all_levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(all_levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string>   ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}
static std::vector<ex::AxisLevel> static_only(std::vector<ex::AxisLevel> const& all_levels) {
    std::vector<ex::AxisLevel> out;
    for (auto const& l : all_levels)
        if (l.is_static) out.push_back(l);
    return out;
}
static std::vector<ex::AxisLevel> drop_tier(std::vector<ex::AxisLevel> const& levels) {
    std::vector<ex::AxisLevel> out;
    for (auto const& l : levels)
        if (l.axis != "tier") out.push_back(l);
    return out;
}
// (S4a) Die committete GOLDEN-Liste der 320 binary_ids laden (Kommentar-/Leerzeilen ignorieren).
static std::vector<std::string> load_golden(fs::path const& p) {
    std::vector<std::string> ids;
    std::ifstream            f{p};
    std::string              line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        ids.push_back(line);
    }
    return ids;
}

int main(int argc, char** argv) {
    // Repo-relative Defaults (CTest/Harness CWD = repo-root).
    fs::path const profiles_dir = fs::path("libs") / "cache_engine" / "algorithm_profiles" / "thesis_profiles";
    fs::path const m3v2_xml     = (argc >= 2) ? fs::path(argv[1]) : (profiles_dir / "m3v2_study.profile.xml");
    fs::path const golden_txt =
        (argc >= 3) ? fs::path(argv[2])
                    : (fs::path("tests") / "unit" / "thesis_tiere" / "golden_fullpilot_320_binary_ids.txt");
    std::cout << "Profil: " << m3v2_xml.string() << "\nGolden: " << golden_txt.string() << "\n";

    // ── (1) GOLDEN-REFERENZ laden (S4a) ──
    std::vector<std::string> const golden_ids = load_golden(golden_txt);
    check_eq("golden binary_count == 320 (4*4*5*4)", golden_ids.size(), std::size_t{320});
    if (golden_ids.size() != 320) {
        std::cout << "\n==== ABBRUCH: golden-Datei unbrauchbar ====\n";
        return 1;
    }

    // ── (2) PROFIL-PFAD: parse_thesis_profile → build_axis_levels (tier-frei) → binary_ids ──
    auto m3 = tlz::load_thesis_profile(m3v2_xml);
    check_true("parse_thesis_profile lieferte ein Profil", m3.has_value());
    if (!m3) {
        std::cout << "\n==== Round-Trip: ABBRUCH (Profil nicht lesbar) ====\n";
        return 1;
    }
    check_eq("Profil-id", m3->id, std::string{"m3v2_study"});

    // S1-Felder (Increment 2): die 4 deklarativen Konstrukte sind geparst.
    check_eq("m3v2: base_tiers (7 SOTA+PRT-ART)", m3->base_tiers.size(), std::size_t{7});
    check_eq("m3v2: working_set_sweep (4 N)", m3->working_set_sweep.size(), std::size_t{4});
    // FF(#168): 4 Basis-Achsen-Sweeps + 4 vertiefte Achsen-Sweeps (path_compression/migration_policy/filter/
    // value_handle) = 8 deklarierte <axis_sweep> (die vertieften laufen ueber eigene Sweep-Baeume, nicht die Basis-View).
    check_eq("m3v2: axis_sweeps (8)", m3->axis_sweeps.size(), std::size_t{8});
    check_eq("m3v2: sota_series (3x7=21)", m3->sota_series.size(), std::size_t{21});
    check_eq("m3v2: run_options.cap", m3->run_options.cap, 320);

    std::vector<ex::AxisLevel> const prof_levels_all =
        tlz::build_profile_levels(*m3, "m3v2_base", /*with_dynamic=*/false);
    std::vector<ex::AxisLevel> const prof_base   = drop_tier(prof_levels_all); // tier-Ebene (SOTA-Dim) abziehen
    std::vector<ex::AxisLevel> const prof_static = static_only(prof_base);
    std::vector<std::string> const   prof_ids    = binary_ids(prof_base);

    // ── (3) STRUKTUR: 19 statische Ebenen, kanonische Achsen-Namen-Reihenfolge ──
    check_eq("profile static_levels = 17 (tier abgezogen; INC-2d isa raus)", prof_static.size(), std::size_t{17});
    {
        bool canon_ok = (prof_static.size() == ex::kCompositionAxisNames.size());
        for (std::size_t i = 0; canon_ok && i < prof_static.size(); ++i)
            canon_ok = (prof_static[i].axis == ex::kCompositionAxisNames[i]);
        check_true("profile Achsen-Namen == kCompositionAxisNames (Single-Source-Reihenfolge)", canon_ok);
    }

    // ── (4) DIE HARTE GATE: profil-getriebene binary_ids == GOLDEN (Diff leer + positions-identisch) ──
    check_eq("profile binary_count", prof_ids.size(), golden_ids.size());
    std::vector<std::string> only_golden, only_profile;
    for (auto const& g : golden_ids) {
        bool found = false;
        for (auto const& p : prof_ids)
            if (p == g) {
                found = true;
                break;
            }
        if (!found) only_golden.push_back(g);
    }
    for (auto const& p : prof_ids) {
        bool found = false;
        for (auto const& g : golden_ids)
            if (g == p) {
                found = true;
                break;
            }
        if (!found) only_profile.push_back(p);
    }
    bool        position_identical = (prof_ids.size() == golden_ids.size());
    std::size_t mism               = 0;
    for (std::size_t i = 0; i < golden_ids.size() && i < prof_ids.size(); ++i)
        if (golden_ids[i] != prof_ids[i]) {
            ++mism;
            if (position_identical) position_identical = false;
        }

    check_eq("Diff only_in_golden (muss leer sein)", only_golden.size(), std::size_t{0});
    check_eq("Diff only_in_profile (muss leer sein)", only_profile.size(), std::size_t{0});
    check_eq("positions-identische binary_ids (Mismatch muss 0 sein)", mism, std::size_t{0});
    check_true("PROFIL-Pfad binary_ids POSITIONS-IDENTISCH zur GOLDEN-Liste (Resume #139-Schutz)", position_identical);

    std::cout << "\n--- Belege (golden vs profile, die ersten 2 + letzte) ---\n";
    for (std::size_t i : {std::size_t{0}, std::size_t{1}, golden_ids.size() - 1}) {
        bool const same = (i < prof_ids.size() && prof_ids[i] == golden_ids[i]);
        std::cout << "  [" << (same ? "==" : "!!") << "] golden[" << i << "] : " << golden_ids[i] << "\n";
        if (!same)
            std::cout << "       profile[" << i << "]: " << (i < prof_ids.size() ? prof_ids[i] : std::string{"<fehlt>"})
                      << "\n";
    }

    // ── (5) DynamicDims aendern die binary_id NICHT (with_dynamic) ──
    {
        std::vector<ex::AxisLevel> const prof_full =
            tlz::build_profile_basis_levels(*m3, "m3v2_base", /*with_dynamic=*/true);
        std::vector<std::string> const prof_full_ids = binary_ids(prof_full);
        check_true("DynamicDims aendern die binary_id NICHT (with_dynamic-ids == golden)", prof_full_ids == golden_ids);
        std::size_t n_dyn = 0;
        for (auto const& l : prof_full)
            if (!l.is_static) ++n_dyn;
        std::cout << "  profile DynamicDims (concurrency/prefetch/repetition) = " << n_dyn << "\n";
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // (6) Inc3 — DER PROFIL-GETRIEBENE TREIBER-KONSUM (profile_select), KEINE Code-Selektions-Dublette mehr.
    //     Klein-Ausschnitt (cap=12), reine Index-Rekombination über den (tier-freien) Basis-StaticBinaryView.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::cout << "\n--- (6) Inc3: profil-getriebene Selektion (profile_select) cap=12 ---\n";
        std::vector<ex::AxisLevel> const basis_levels =
            tlz::build_profile_basis_levels(*m3, "m3v2_base", /*with_dynamic=*/false);
        auto               factory7 = std::make_shared<ex::ExperimentNodeFactory>();
        ex::ExperimentTree tree7{factory7};
        tree7.build(basis_levels);
        ex::StaticBinaryView const view7 = tree7.static_binary_view();
        std::size_t const          cap   = 12;

        // (6a) BASIS-320 (profile_select "") = die ersten cap View-Indizes; ihre binary_ids == golden[0..cap-1].
        {
            tlz::ProfileTaggedSelection const pf = tlz::profile_select(*m3, basis_levels, view7, "", cap);
            check_eq("Inc3/Basis: indices count", pf.selection.indices.size(), cap);
            check_eq("Inc3/Basis: series tag", pf.series, std::string{"-"});
            check_eq("Inc3/Basis: sweep_axis tag", pf.sweep_axis, std::string{"-"});
            bool ids_ok = (pf.selection.indices.size() == cap);
            for (std::size_t i = 0; ids_ok && i < cap; ++i)
                ids_ok = (view7[pf.selection.indices[i]].binary_id == golden_ids[i]);
            check_true("Inc3/Basis: binary_ids[0..11] == golden[0..11]", ids_ok);
        }
        // (6b) PER-ACHSEN-SWEEP node_type: profile_axis_level == 4; die Sweep-binary_ids variieren NUR node_type.
        {
            std::size_t const lvl = tlz::profile_axis_level(basis_levels, "node_type");
            check_eq("Inc3/Sweep: profile_axis_level(node_type) == 4", lvl, std::size_t{4});
            tlz::ProfileTaggedSelection const pf = tlz::profile_select(*m3, basis_levels, view7, "node_type", cap);
            check_eq("Inc3/Sweep: sweep_axis tag == 'node_type'", pf.sweep_axis, std::string{"node_type"});
            check_true("Inc3/Sweep: 4 node_type-Varianten (cap>=4)", pf.selection.indices.size() == std::size_t{4});
            for (std::size_t i = 0; i < pf.selection.indices.size() && i < 4; ++i)
                std::cout << "  Sweep[node_type][" << i
                          << "] = " << view7[pf.selection.indices[i]].binary_id.substr(0, 96) << "...\n";
        }
        // (6c) PROFIL-WHITELIST: eine NICHT als <axis_sweep> deklarierte Achse wird VERWEIGERT (profil-getrieben).
        {
            tlz::ProfileTaggedSelection const pf = tlz::profile_select(*m3, basis_levels, view7, "allocator", cap);
            check_true("Inc3/Whitelist: nicht-deklarierte Achse 'allocator' verweigert (REFUSED-Provenance)",
                       pf.selection.provenance.rfind("axis_sweep-REFUSED", 0) == 0);
        }
        // (6d) run_options + working_set_sweep (profil-getrieben, ersetzt argv/env-Defaults + PS-foreach).
        {
            tlz::ProfileRunOptions const ro = tlz::profile_run_options(*m3);
            check_eq("Inc3/run_options: cap == 320", ro.cap, std::size_t{320});
            check_eq("Inc3/run_options: platform == win-x86_64", ro.platform, std::string{"win-x86_64"});
            check_eq("Inc3/run_options: build_version == m3v2", ro.build_version, std::string{"m3v2"});
            check_true("Inc3/run_options: resume == true", ro.resume);
            std::vector<std::uint64_t> const ns = tlz::profile_working_set_sweep(*m3);
            bool const exact = (ns == std::vector<std::uint64_t>{16384u, 131072u, 1048576u, 8388608u});
            check_true("Inc3/working_set_sweep == {16384,131072,1048576,8388608}", exact);
        }
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // (7) cap-SEMANTIK (GO-4/GO-5-Nebenbefund, 2026-07-12) — profile_effective_cap ist die EINE cap-Aufloesung
    //     der Basis-Selektion (profile_runner.hpp; Konsum in run_profile). GEPINNT: cap=0 → alle, cap=1 → 1,
    //     cap FEHLT (run_options-Default 0) → alle; argv/env-Override behaelt Vorrang; Klemmung auf basis_count.
    //     VORHER ergab eff_cap==0 eine LEERE Basis-Selektion — m3_golden_coverage (cap="0" = dokumentiert
    //     "KEIN kuenstliches Cap") haette damit KEINE Basis-320 selektiert.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::cout << "\n--- (7) cap-Semantik (profile_effective_cap) ---\n";
        check_eq("cap=0 → alle (320)", tlz::profile_effective_cap(0, 0, 320), std::size_t{320});
        check_eq("cap=1 → 1", tlz::profile_effective_cap(1, 0, 320), std::size_t{1});
        check_eq("cap=320 → 320", tlz::profile_effective_cap(320, 0, 320), std::size_t{320});
        check_eq("cap>basis_count → geklemmt (500→320)", tlz::profile_effective_cap(500, 0, 320), std::size_t{320});
        check_eq("Override-Vorrang (profil 0, override 150)", tlz::profile_effective_cap(0, 150, 320),
                 std::size_t{150});
        check_eq("Override-Vorrang (profil 320, override 12)", tlz::profile_effective_cap(320, 12, 320),
                 std::size_t{12});

        // cap FEHLT: ein Profil OHNE <run_options> traegt den Parser-Default cap=0 → alle Basis-Zellen.
        cx::ThesisProfile            no_cap; // run_options.cap Default = 0 (xml_config_parser.hpp)
        tlz::ProfileRunOptions const ro_no_cap = tlz::profile_run_options(no_cap);
        check_eq("cap FEHLT: run_options-Default 0", ro_no_cap.cap, std::size_t{0});
        check_eq("cap FEHLT → alle (320)", tlz::profile_effective_cap(ro_no_cap.cap, 0, 320), std::size_t{320});

        // REALES Profil m3_golden_coverage (cap="0" im XML, gleicher Ordner wie m3v2_study): dessen
        // Basis-Selektion wird durch die 0=KEIN-Cap-Semantik ERSTMALS korrekt (volle Basis-320 statt leer).
        fs::path const golden_cov_xml = m3v2_xml.parent_path() / "m3_golden_coverage.profile.xml";
        auto           gc             = tlz::load_thesis_profile(golden_cov_xml);
        check_true("m3_golden_coverage geparst", gc.has_value());
        if (gc) {
            tlz::ProfileRunOptions const ro_gc = tlz::profile_run_options(*gc);
            check_eq("m3_golden_coverage: run_options.cap == 0 (XML cap=\"0\")", ro_gc.cap, std::size_t{0});
            check_eq("m3_golden_coverage: effektiv = volle Basis-320", tlz::profile_effective_cap(ro_gc.cap, 0, 320),
                     std::size_t{320});
        }
        // m3v2_study (TABU, byte-unberuehrt): cap=320 == basis_count → Verhalten UNVERAENDERT.
        check_eq("m3v2_study: cap=320 unveraendert", tlz::profile_effective_cap(320, 0, 320), std::size_t{320});
    }

    std::cout << "\n==== STRANG-A Inc1..4 Round-Trip-Gate (gegen GOLDEN): "
              << (g_fail == 0 ? "ALLE OK (Diff leer)" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
