// test_smoke_coverage_profile — #26/GO-5 (2026-07-12): Gate fuer das Smoke-/Coverage-MESS-Profil
// m3_smoke_coverage.profile.xml + den Multi-Sweep-Durchlauf (Dossier GO-5 Teil B, B.4).
//
// BEWEIST LITERAL:
//   (1) STRUKTUR: das Profil deklariert die volle Feature-Beruehrung — 7 base_tiers, 19 gepinnte
//       <permute_axes> (je EXAKT 1 Wert = Basis-Zellen-Anzahl 1), ALLE 19 <axis_sweeps>, 21 <sota_series>
//       (7 Lebewesen x 3 Stufen = #9-Messpfad), 2 Working-Sets, thread_count=1 (T8-ehrlich), 1 Wiederholung.
//   (2) BINARY-ZAEHLUNG GEPINNT (Default-Configure dieses Baums): Vereinigung der 19 deklarierten
//       Achsen-Sweeps = 1 Baseline + 71 Sweep-DLLs = 72 distinkte binary_ids; + 21 SOTA-Reihen =
//       93 Binaries gesamt (enable-abhaengig — Details am Pin kExpectedSweepUnion).
//   (3) GOLDEN BYTE-UNBERUEHRT: die gepinnte Basis erzeugt GENAU die golden-320-Baseline (golden[0]);
//       jede per-Achse-Sweep-Map enthaelt golden[0] idempotent; die Sweep-ids der 4 Basis-Achsen sind
//       unter den Default-Enables eine TEILMENGE der golden-320 (Einzel-Sweep-id-Identitaet, B.4.1-a).
//   (4) MULTI-SWEEP-REGRESSION (B.4.1-b): profile_sweep_passes ist deterministisch — explizites
//       args.sweep_axis ⇒ genau 1 Pass (Einzel-Sweep-Vorrang, byte-identisch); leer ⇒ Basis-Pass +
//       deklarierte Sweeps in Dokument-Reihenfolge; Profile OHNE <axis_sweeps> ⇒ exakt der Basis-Pass.
//   (5) VALIDAT: das Profil ist gegen die REALEN EnabledStrategies konsistent (validate_profile ok) —
//       das ctest-Gegenstueck zum treiberseitigen `messung_driver --validate`.
//
// TABU-Wache: m3v2_study.profile.xml wird hier NUR GELESEN (Regressions-Referenz), golden_fullpilot_320_
// binary_ids.txt NUR GELESEN (320 Zeilen, Fixture unveraendert).

#include "profile_runner.hpp"   // load_thesis_profile / build_profile_basis_levels / profile_sweep_passes
#include "source_catalog.hpp"   // axis_sweep_source_map / axis_sweep_levels / is_deepened_axis
#include "validate_profile.hpp" // validate_profile / axis_registry_from_levels

#include <builder/experiment_tree/experiment_tree.hpp>         // ExperimentTree / StaticBinaryView
#include <builder/experiment_tree/registry_to_axis_levels.hpp> // build_all_axis_levels (EnabledStrategies)

#include <axes/alloc/axis_06_allocator_flags.hpp> // flags::mimalloc_enabled (basis-bewusster Pin)

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#ifndef COMDARE_THESIS_PROFILES_DIR
#error "COMDARE_THESIS_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/thesis_profiles"
#endif
#ifndef COMDARE_GOLDEN_320_IDS
#error "COMDARE_GOLDEN_320_IDS must point to tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt"
#endif

namespace {

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

fs::path smoke_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "m3_smoke_coverage.profile.xml"; }
fs::path m3v2_profile_path() { return fs::path{COMDARE_THESIS_PROFILES_DIR} / "m3v2_study.profile.xml"; }

// GEPINNTE Binary-Zaehlung (Default-Configure DIESES Quellbaums, empirisch 2026-07-12; per-Achse-
// Aufschluesselung druckt der Test im Fehlerfall). Driftet ein Enable-Flag, MUSS dieser Pin bewusst
// nachgezogen werden. EHRLICHKEITS-BEFUND zur Dossier-Schaetzung (GO-5 B.4.3 "~115"): das Dossier zaehlte
// die CMake-ENABLE-Flags (allocator 25/26 ON); compile-time zaehlt aber USE = ENABLE && HAVE
// (Vendor-Detection, axis_06_allocator_flags.hpp) — auf dem ce-Default-Baum sind nur STD/PMR/POOL
// detektiert (allocator=3) -> Union 72 = 1 Baseline + 71 Sweeps; + 21 SOTA = 93 Binaries. Im super-Sub-
// Build (Mess-Runner) kommt MIMALLOC dazu (allocator=4 -> 73/94); mit Voll-Vendor-HAVE laege sie bei
// 94/115. Das Profil selbst ist enable-agnostisch (je Achse IMMER die volle Enabled-Liste).
// wf_1009d16f-FIX (2026-07-12): Seit der Vendor-Include-Vererbung (Root-CMakeLists) baut dieser
// Test AUCH im perms-ON-Baum (USE_MIMALLOC=1). Der Pin folgt compile-time der oben dokumentierten
// Basis (72/93 Default-Baum, 73/94 mit MIMALLOC) statt die 72 hart zu verdrahten; Drift jeder
// ANDEREN Achse/Vendor-Detection schlaegt weiterhin als Pin-Verletzung auf.
constexpr std::size_t kExpectedSweepUnion = // 1 Baseline + Sum(USE-Enabled-1)
    72 + (comdare::cache_engine::alloc::flags::mimalloc_enabled ? 1 : 0);
constexpr std::size_t kExpectedTotalBinaries = kExpectedSweepUnion + 21; // + 7 Lebewesen x 3 Stufen (SOTA) = 93

std::vector<std::string> load_golden_ids() {
    std::vector<std::string> ids;
    std::ifstream            f{fs::path{COMDARE_GOLDEN_320_IDS}};
    std::string              line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        ids.push_back(line);
    }
    return ids;
}

std::vector<std::string> tree_binary_ids(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string>   ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

} // namespace

// (1) STRUKTUR — volle deklarierte Feature-Beruehrung, Basis-Zellen-Anzahl 1, ehrliche Lauf-Dimensionen.
TEST(SmokeCoverageProfile, DeclaresFullCoverageWithPinnedBasis) {
    auto const tp = tlz::load_thesis_profile(smoke_profile_path());
    ASSERT_TRUE(tp.has_value()) << "Profil nicht lesbar: " << smoke_profile_path().string();
    EXPECT_EQ(tp->id, "m3_smoke_coverage");
    EXPECT_EQ(tp->base_tiers.size(), 7u);
    ASSERT_EQ(tp->permute_axes.size(), 19u);
    std::size_t basis_cells = 1;
    for (auto const& ax : tp->permute_axes) {
        EXPECT_EQ(ax.values.size(), 1u) << "Basis nicht gepinnt (Achse " << ax.ref << ")";
        basis_cells *= (ax.values.empty() ? 1u : ax.values.size());
    }
    EXPECT_EQ(basis_cells, 1u) << "Basis-Zellen-Anzahl muss 1 sein (KEIN Kreuzprodukt)";
    EXPECT_EQ(tp->axis_sweeps.size(), 19u) << "alle 19 Achsen-Sweeps muessen deklariert sein";
    EXPECT_EQ(tp->sota_series.size(), 21u) << "7 Lebewesen x 3 Merge-Stufen";
    EXPECT_EQ(tp->workloads.size(), 21u) << "alle 21 Lastprofile (Achse-2-Voll-Coverage)";
    EXPECT_EQ(tp->working_set_sweep.size(), 2u) << "2 Working-Sets (klein + >LLC)";
    ASSERT_EQ(tp->thread_counts.size(), 1u) << "thread_count MUSS auf 1 gepinnt sein (T8 LABEL-ONLY)";
    EXPECT_EQ(tp->thread_counts.front(), "1");
    EXPECT_EQ(tp->repetitions, 1) << "Smoke prueft Funktion, nicht Streuung (KF-10)";
    EXPECT_EQ(tp->run_options.cap, 1) << "cap=1 = exakt die eine deklarierte Basis-Zelle";
    EXPECT_TRUE(tp->run_options.resume);
    // Jede deklarierte Sweep-Achse ist sweep-katalogisiert (inkl. der 4 Basis-Achsen, #26/GO-5 B.4.1-a).
    for (auto const& sw : tp->axis_sweeps)
        EXPECT_TRUE(tlz::is_deepened_axis(sw.axis)) << "deklarierter Sweep ohne Katalog: " << sw.axis;
}

// (2) BINARY-ZAEHLUNG GEPINNT — Sweep-/Baseline-DLLs + 21 SOTA (Default-Enables; per-Achse-Diagnose im
// Fehlerfall, damit ein Enable-Flag-Drift sofort der Achse zuordenbar ist).
TEST(SmokeCoverageProfile, BinaryCountPinned) {
    auto const tp = tlz::load_thesis_profile(smoke_profile_path());
    ASSERT_TRUE(tp.has_value());
    std::set<std::string> union_ids;
    std::size_t           sum_minus_baseline = 0;
    std::string           per_axis_diag;
    for (auto const& sw : tp->axis_sweeps) {
        auto const m = tlz::axis_sweep_source_map(sw.axis);
        ASSERT_GE(m.size(), 2u) << "Sweep-Achse ohne >=2 Auspraegungen: " << sw.axis;
        sum_minus_baseline += m.size() - 1; // je Achse: Enabled-1 Nicht-Baseline-Auspraegungen
        for (auto const& [id, src] : m) union_ids.insert(id);
        per_axis_diag += sw.axis + "=" + std::to_string(m.size()) + " ";
    }
    // Konsistenz: Vereinigung == 1 gemeinsame Baseline + Summe der Nicht-Baseline-Auspraegungen.
    EXPECT_EQ(union_ids.size(), sum_minus_baseline + 1) << "Baseline nicht idempotent ueber die 19 Sweep-Maps";
    // GEPINNT (reale Default-Enables dieses Baums, empirisch verifiziert 2026-07-12): 1 Baseline +
    // Sum(Enabled-1) Sweep-DLLs; + 21 SOTA-Reihen = Gesamt. Per-Achse-Zaehlung s. Diagnose-String.
    EXPECT_EQ(union_ids.size(), kExpectedSweepUnion) << "Sweep-Binary-Zaehlung driftet (Enable-Flags "
                                                        "veraendert?). Per-Achse: "
                                                     << per_axis_diag;
    EXPECT_EQ(union_ids.size() + tp->sota_series.size(), kExpectedTotalBinaries)
        << "Gesamt-Binary-Zaehlung (Sweep-Union + 21 SOTA). Per-Achse: " << per_axis_diag;
}

// (3) GOLDEN BYTE-UNBERUEHRT — Basis == golden[0]; Baseline idempotent; Basis-Achsen-Sweeps ⊂ golden-320.
TEST(SmokeCoverageProfile, GoldenBaselineIdentityAndSubset) {
    std::vector<std::string> const golden = load_golden_ids();
    ASSERT_EQ(golden.size(), 320u) << "golden-Fixture veraendert (TABU!)";
    std::set<std::string> const golden_set{golden.begin(), golden.end()};

    auto const tp = tlz::load_thesis_profile(smoke_profile_path());
    ASSERT_TRUE(tp.has_value());

    // Die gepinnte Basis materialisiert GENAU die golden-Baseline (idempotente binary_id, Resume #139).
    std::vector<ex::AxisLevel> const basis =
        tlz::build_profile_basis_levels(*tp, "m3_smoke_base", /*with_dynamic=*/false);
    std::vector<std::string> const basis_ids = tree_binary_ids(basis);
    ASSERT_EQ(basis_ids.size(), 1u) << "gepinnte Basis muss genau 1 Zelle ergeben";
    EXPECT_EQ(basis_ids.front(), golden.front()) << "Smoke-Baseline driftet von golden[0]";

    // Jede per-Achse-Sweep-Map enthaelt die golden-Baseline (idempotenter Baseline-Key).
    for (auto const& sw : tp->axis_sweeps) {
        auto const m = tlz::axis_sweep_source_map(sw.axis);
        EXPECT_EQ(m.count(golden.front()), 1u) << "Baseline fehlt im Sweep-Katalog: " << sw.axis;
    }

    // Die 4 Basis-Achsen-Sweep-ids sind unter Default-Enables eine TEILMENGE der golden-320 und beginnen
    // mit golden[0] (gleiche Enabled-Reihenfolge) — das Einzel-Sweep-Verhalten bleibt id-identisch (B.4.1-a).
    for (auto const& ax : {"search_algo", "node_type", "memory_layout", "prefetch"}) {
        std::vector<std::string> const sweep_ids = tree_binary_ids(tlz::axis_sweep_levels(ax));
        ASSERT_FALSE(sweep_ids.empty());
        EXPECT_EQ(sweep_ids.front(), golden.front()) << "Basis-Achsen-Sweep beginnt nicht mit der Baseline: " << ax;
        for (auto const& id : sweep_ids)
            EXPECT_EQ(golden_set.count(id), 1u)
                << "Basis-Achsen-Sweep-id ausserhalb der golden-320 (Default-Enables): " << ax << " -> " << id;
    }
}

// (4) MULTI-SWEEP-REGRESSION — deterministische Pass-Liste; Einzel-Sweep-Vorrang; Basis-only unveraendert.
TEST(SmokeCoverageProfile, SweepPassPlanIsDeterministic) {
    auto const tp = tlz::load_thesis_profile(smoke_profile_path());
    ASSERT_TRUE(tp.has_value());

    // Leer ⇒ Basis-Pass ("") + die 19 deklarierten Sweeps in Dokument-Reihenfolge.
    std::vector<std::string> const passes = tlz::profile_sweep_passes(*tp, "");
    ASSERT_EQ(passes.size(), 20u);
    EXPECT_TRUE(passes.front().empty()) << "der Basis-Pass muss IMMER zuerst laufen";
    for (std::size_t i = 0; i < tp->axis_sweeps.size(); ++i)
        EXPECT_EQ(passes[i + 1], tp->axis_sweeps[i].axis) << "Pass-Reihenfolge != Dokument-Reihenfolge (i=" << i << ")";

    // Explizites args.sweep_axis behaelt Vorrang: GENAU ein Pass (Einzel-Sweep byte-identisch).
    std::vector<std::string> const single = tlz::profile_sweep_passes(*tp, "allocator");
    ASSERT_EQ(single.size(), 1u);
    EXPECT_EQ(single.front(), "allocator");

    // Profil OHNE <axis_sweeps> ⇒ exakt der Basis-Pass (bisheriges Verhalten byte-identisch).
    comdare::builder::xml::ThesisProfile empty_tp;
    std::vector<std::string> const       basis_only = tlz::profile_sweep_passes(empty_tp, "");
    ASSERT_EQ(basis_only.size(), 1u);
    EXPECT_TRUE(basis_only.front().empty());

    // m3v2_study (TABU, NUR GELESEN): Basis + seine 8 deklarierten Sweeps; explizit ⇒ 1 Pass.
    auto const m3v2 = tlz::load_thesis_profile(m3v2_profile_path());
    ASSERT_TRUE(m3v2.has_value());
    EXPECT_EQ(tlz::profile_sweep_passes(*m3v2, "").size(), 1u + m3v2->axis_sweeps.size());
    EXPECT_EQ(tlz::profile_sweep_passes(*m3v2, "node_type"), (std::vector<std::string>{"node_type"}));
}

// (5) VALIDAT — das Profil ist gegen die REALEN EnabledStrategies konsistent (ctest-Gegenstueck
// zu `messung_driver --validate`).
TEST(SmokeCoverageProfile, ValidatesAgainstEnabledStrategies) {
    auto const tp = tlz::load_thesis_profile(smoke_profile_path());
    ASSERT_TRUE(tp.has_value());
    ex::AxisRegistry const             registry = tlz::axis_registry_from_levels(ex::build_all_axis_levels());
    tlz::ProfileValidationResult const vr       = tlz::validate_profile(*tp, registry);
    for (auto const& e : vr.errors) ADD_FAILURE() << "[validate] " << e;
    EXPECT_TRUE(vr.ok);
    EXPECT_EQ(vr.axes_checked, 19u);
    EXPECT_EQ(vr.sweeps_checked, 19u);
    EXPECT_EQ(vr.series_checked, 21u);
}
