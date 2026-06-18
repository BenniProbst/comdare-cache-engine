// test_profile_roundtrip — STRANG A KORRIGIERT, Increment 1 (2026-06-18). Die HARTE Round-Trip-Gate (S3/S8a).
//
// BEWEIST LITERAL: der profil-getriebene Pfad (base_pilot.profile.xml → parse_thesis_profile → build_axis_levels
// → StaticBinaryView) erzeugt EXAKT dieselben binary_ids wie der heutige Code-Pfad
// (build_pilot_levels<SmallPilot> → StaticBinaryView). Diff == leer ⇒ Resume #139 bricht NICHT (gleiche
// Achsen-Namen-Reihenfolge + gleiche W::name()-Werte + gleiches serialize_composition_path-Format).
//
// Klein-Ausschnitt (SmallPilot = 1·2·2·1 = 4 Binaries) — KEIN 320-Voll-Bau. Pflicht: das Profil drueckt EXAKT
// die SmallPilot-Selektion deklarativ aus (probe-verifizierte Wrapper-name()-Werte).
//
// Build (innerhalb vcvars64): cl /std:c++latest /EHsc /Od /bigobj <inc-Satz wie thesis_tiere-Harness>
//   test_profile_roundtrip.cpp <xml_config_parser.cpp>  (parse_thesis_profile ist NICHT header-only)

#include "lazy_pilot_engine.hpp"   // build_pilot_levels<SmallPilot/FullPilot> (Vergleichs-Referenz, UNVERAENDERT)
#include "profile_runner.hpp"      // build_profile_levels (die neue Naht: build_axis_levels live)

#include <builder/experiment_tree/axis_path_serialization.hpp>   // kCompositionAxisNames (19 Slot-Namen, Single-Source)

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cx  = comdare::builder::xml;
namespace fs  = std::filesystem;

static int g_fail = 0;
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

// ExperimentTree::static_binary_view() → die geordnete binary_id-Liste. WICHTIG: der Baum filtert intern
// static_filter() (is_static) — d.h. eventuelle dynamische Ebenen im Level-Satz verschmutzen die binary_id NICHT.
// Das ist GENAU der Produktiv-Pfad (run_lazy_150 ruft tree.static_binary_view()) → ehrlicher Vergleich.
static std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& all_levels) {
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(all_levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string> ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}
// Anzahl statischer Ebenen (= die Achsen der Binary-Identitaet) eines Level-Satzes.
static std::vector<ex::AxisLevel> static_only(std::vector<ex::AxisLevel> const& all_levels) {
    std::vector<ex::AxisLevel> out;
    for (auto const& l : all_levels) if (l.is_static) out.push_back(l);
    return out;
}
// Entfernt das (optionale) tier-Level: das m3v2-Profil deklariert base_tiers (SOTA-Reihen-Dimension) →
// build_axis_levels emittiert ein "tier"-Level oben. Der BASIS-320-Teil ist die REINE 19-Slot-Achsen-
// Rekombination (FullPilot hat KEIN tier-Level) → fuer den Round-Trip wird die tier-Ebene abgezogen.
static std::vector<ex::AxisLevel> drop_tier(std::vector<ex::AxisLevel> const& levels) {
    std::vector<ex::AxisLevel> out;
    for (auto const& l : levels) if (l.axis != "tier") out.push_back(l);
    return out;
}

int main(int argc, char** argv) {
    // Pfad zum Basis-Profil: argv[1] ODER der Repo-Default.
    fs::path profile_xml;
    if (argc >= 2) profile_xml = argv[1];
    else {
        // Repo-relativer Default (vom Harness/CTest CWD = repo-root erreichbar).
        profile_xml = fs::path("libs") / "cache_engine" / "algorithm_profiles" / "thesis_profiles" /
                      "base_pilot.profile.xml";
    }
    std::cout << "Profil: " << profile_xml.string() << "\n";

    // ── (1) CODE-PFAD: build_pilot_levels<SmallPilot> (nur statisch) → binary_ids ──
    std::vector<ex::AxisLevel> const code_levels =
        tlz::build_pilot_levels<tlz::SmallPilot>(/*with_dynamic=*/false);
    std::vector<ex::AxisLevel> const code_static = static_only(code_levels);
    std::vector<std::string> const code_ids = binary_ids(code_levels);

    // ── (2) PROFIL-PFAD: parse_thesis_profile → build_axis_levels (nur statisch) → binary_ids ──
    auto tp = tlz::load_thesis_profile(profile_xml);
    check_true("parse_thesis_profile lieferte ein Profil", tp.has_value());
    if (!tp) { std::cout << "\n==== Round-Trip: ABBRUCH (Profil nicht lesbar) ====\n"; return 1; }
    check_eq("Profil-id", tp->id, std::string{"base_pilot"});

    std::vector<ex::AxisLevel> const prof_levels =
        tlz::build_profile_levels(*tp, "pilot_base", /*with_dynamic=*/false);
    std::vector<ex::AxisLevel> const prof_static = static_only(prof_levels);
    std::vector<std::string> const prof_ids = binary_ids(prof_levels);

    // ── (3) STRUKTUR: 19 statische Ebenen, gleiche Achsen-Namen-Reihenfolge wie kCompositionAxisNames ──
    check_eq("code static_levels = 19", code_static.size(), std::size_t{19});
    check_eq("profile static_levels = 19", prof_static.size(), std::size_t{19});
    {
        bool names_ok = (code_static.size() == prof_static.size());
        for (std::size_t i = 0; names_ok && i < code_static.size(); ++i)
            names_ok = (code_static[i].axis == prof_static[i].axis);
        check_true("Achsen-Namen-Reihenfolge code == profile (19 Slots)", names_ok);

        bool canon_ok = (prof_static.size() == ex::kCompositionAxisNames.size());
        for (std::size_t i = 0; canon_ok && i < prof_static.size(); ++i)
            canon_ok = (prof_static[i].axis == ex::kCompositionAxisNames[i]);
        check_true("profile Achsen-Namen == kCompositionAxisNames (Single-Source-Reihenfolge)", canon_ok);
    }

    // ── (4) DIE HARTE GATE: binary_id-Listen-Diff == leer ──
    check_eq("binary_count code (1*2*2*1)", code_ids.size(), std::size_t{4});
    check_eq("binary_count profile", prof_ids.size(), code_ids.size());

    std::vector<std::string> only_code, only_profile;
    {
        // Symmetrische Differenz, positionsunabhaengig (Mengen-Diff) UND positions-identisch (s.u.).
        for (auto const& c : code_ids) {
            bool found = false;
            for (auto const& p : prof_ids) if (p == c) { found = true; break; }
            if (!found) only_code.push_back(c);
        }
        for (auto const& p : prof_ids) {
            bool found = false;
            for (auto const& c : code_ids) if (c == p) { found = true; break; }
            if (!found) only_profile.push_back(p);
        }
    }
    bool position_identical = (code_ids.size() == prof_ids.size());
    for (std::size_t i = 0; position_identical && i < code_ids.size(); ++i)
        position_identical = (code_ids[i] == prof_ids[i]);

    std::cout << "\n--- Die 4 binary_ids (SmallPilot-Klein-Ausschnitt) ---\n";
    for (std::size_t i = 0; i < code_ids.size(); ++i) {
        bool const same = (i < prof_ids.size() && prof_ids[i] == code_ids[i]);
        std::cout << "  [" << (same ? "==" : "!!") << "] code   : " << code_ids[i] << "\n";
        std::cout << "       profile: " << (i < prof_ids.size() ? prof_ids[i] : std::string{"<fehlt>"}) << "\n";
    }

    check_eq("Diff only_in_code (muss leer sein)", only_code.size(), std::size_t{0});
    check_eq("Diff only_in_profile (muss leer sein)", only_profile.size(), std::size_t{0});
    check_true("binary_id-Listen POSITIONS-IDENTISCH (code == profile)", position_identical);

    // ── (5) Bonus: build_profile_levels MIT dynamic → die DynamicDims aendern die binary_id NICHT ──
    {
        std::vector<ex::AxisLevel> const prof_full = tlz::build_profile_levels(*tp, "pilot_base", /*with_dynamic=*/true);
        // binary_ids() baut den ExperimentTree → static_binary_view() filtert is_static intern (Produktiv-Pfad).
        std::vector<std::string> const prof_full_ids = binary_ids(prof_full);
        bool const same = (prof_full_ids == code_ids);
        check_true("DynamicDims aendern die binary_id NICHT (with_dynamic-binary_ids == code_ids)", same);
        std::size_t n_dyn = 0; for (auto const& l : prof_full) if (!l.is_static) ++n_dyn;
        std::cout << "  profile DynamicDims (concurrency/prefetch/repetition) = " << n_dyn << "\n";
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // (6) STRANG-A Inc2 / S2 — DIE HARTE GATE fuer m3v2_study.profile.xml: sein BASIS-320-Teil (tier-frei =
    //     reine 19-Slot-Achsen-Rekombination) erzeugt via build_axis_levels EXAKT dieselben binary_ids wie
    //     build_pilot_levels<FullPilot> (4×4×5×4 = 320). KEIN 320-Voll-BAU — nur String-Rekombination ueber
    //     den StaticBinaryView (kein cl/Anatomie). Diff == leer ⇒ Resume #139 bricht NICHT.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::cout << "\n--- (6) m3v2_study.profile.xml: Basis-320 == FullPilot (tier-frei) ---\n";
        fs::path const m3v2_xml = profile_xml.parent_path() / "m3v2_study.profile.xml";
        std::cout << "Profil: " << m3v2_xml.string() << "\n";

        // FullPilot-Referenz (320 binary_ids, nur statisch).
        std::vector<ex::AxisLevel> const full_levels =
            tlz::build_pilot_levels<tlz::FullPilot>(/*with_dynamic=*/false);
        std::vector<std::string> const full_ids = binary_ids(full_levels);

        auto m3 = tlz::load_thesis_profile(m3v2_xml);
        check_true("m3v2: parse_thesis_profile lieferte ein Profil", m3.has_value());
        if (m3) {
            check_eq("m3v2: Profil-id", m3->id, std::string{"m3v2_study"});
            // S1-Felder (Increment 2): die 4 deklarativen Konstrukte sind geparst.
            check_eq("m3v2: base_tiers (7 SOTA+PRT-ART)", m3->base_tiers.size(), std::size_t{7});
            check_eq("m3v2: working_set_sweep (4 N)", m3->working_set_sweep.size(), std::size_t{4});
            check_eq("m3v2: axis_sweeps (5)", m3->axis_sweeps.size(), std::size_t{5});
            check_eq("m3v2: sota_series (3x7=21)", m3->sota_series.size(), std::size_t{21});
            check_eq("m3v2: run_options.cap", m3->run_options.cap, 320);
            check_eq("m3v2: run_options.platform", m3->run_options.platform, std::string{"win-x86_64"});
            check_eq("m3v2: run_options.build_version", m3->run_options.build_version, std::string{"m3v2"});
            check_true("m3v2: run_options.resume", m3->run_options.resume);

            // Basis-320: build_axis_levels(m3v2, m3v2_base) — tier-Ebene (SOTA-Dim) abziehen → reine 19-Slot-Grid.
            std::vector<ex::AxisLevel> const m3_levels_all =
                tlz::build_profile_levels(*m3, "m3v2_base", /*with_dynamic=*/false);
            std::vector<ex::AxisLevel> const m3_base = drop_tier(m3_levels_all);
            check_eq("m3v2: Basis static_levels = 19 (tier abgezogen)", m3_base.size(), std::size_t{19});
            {
                bool canon_ok = (m3_base.size() == ex::kCompositionAxisNames.size());
                for (std::size_t i = 0; canon_ok && i < m3_base.size(); ++i)
                    canon_ok = (m3_base[i].axis == ex::kCompositionAxisNames[i]);
                check_true("m3v2: Achsen-Namen == kCompositionAxisNames (Single-Source-Reihenfolge)", canon_ok);
            }
            std::vector<std::string> const m3_ids = binary_ids(m3_base);

            check_eq("m3v2: FullPilot binary_count (4*4*5*4)", full_ids.size(), std::size_t{320});
            check_eq("m3v2: Basis binary_count", m3_ids.size(), full_ids.size());

            bool pos_ident = (m3_ids.size() == full_ids.size());
            for (std::size_t i = 0; pos_ident && i < full_ids.size(); ++i) pos_ident = (m3_ids[i] == full_ids[i]);
            std::size_t mism = 0;
            for (std::size_t i = 0; i < full_ids.size() && i < m3_ids.size(); ++i) if (full_ids[i] != m3_ids[i]) ++mism;
            check_eq("m3v2: positions-identische binary_ids (Mismatch muss 0 sein)", mism, std::size_t{0});
            check_true("m3v2: Basis-320 binary_id-Liste POSITIONS-IDENTISCH zu FullPilot", pos_ident);
            // Belege: die ersten 2 + die letzten 1 binary_ids literal.
            for (std::size_t i : {std::size_t{0}, std::size_t{1}, full_ids.size() - 1}) {
                bool const same = (i < m3_ids.size() && m3_ids[i] == full_ids[i]);
                std::cout << "  [" << (same ? "==" : "!!") << "] full[" << i << "]: " << full_ids[i] << "\n";
                if (!same) std::cout << "       m3v2[" << i << "]: " << (i < m3_ids.size() ? m3_ids[i] : std::string{"<fehlt>"}) << "\n";
            }
        }
    }

    std::cout << "\n==== STRANG-A Inc1 Round-Trip-Gate: "
              << (g_fail == 0 ? "ALLE OK (Diff leer)" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
