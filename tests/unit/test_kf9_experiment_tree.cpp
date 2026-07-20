// test_kf9_experiment_tree — KF-9 (2026-06-02)
// Experiment-B+-Baum: zusammenhängender Gesamtbaum, Filter statisch/dynamisch, statisch=Binary,
// dynamisch=virtuelle for-Schleifen (Experiment-Iterationen), Blatt = eine Experiment-Einstellung.
// + Adapter comdare_thesis_profile -> AxisLevels.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<libs/common/serialization> test_kf9_experiment_tree.cpp

#include "builder/experiment_tree/experiment_tree.hpp"
#include "builder/experiment_tree/profile_to_tree.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;
namespace cx = comdare::builder::xml;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();

    // ── Teil 1: Baum aus ZUSAMMENHÄNGENDEM Gesamt-Level-Satz + Filterung ──
    ex::ExperimentTree         tree{factory};
    std::vector<ex::AxisLevel> all_levels = {
        ex::AxisLevel{"traversal", {"ART"}, true, ""},          // statisch, gepinnt
        ex::AxisLevel{"node", {"N4", "N16", "N256"}, true, ""}, // statisch, freigegeben
        ex::AxisLevel{"node.cl_line", {"64", "128"}, true, ""}, // statisch (compile-time cacheline) -> Binary
        ex::AxisLevel{"concurrency", {"1", "2", "4"}, false, "thread_count"}, // DYNAMISCH -> for-Schleife
    };
    tree.build(all_levels);

    // Filter statisch/dynamisch (Gesamtbaum bleibt zusammenhängend)
    check_eq("static_filter (3 Ebenen)", tree.static_filter().size(), std::size_t{3});
    check_eq("dynamic_filter (1 Dimension)", tree.dynamic_filter().size(), std::size_t{1});

    // statische Rekombination = Binaries; dynamische Rekombination = Iterationen je Binary
    check_eq("binary_count (1x3x2)", tree.binary_count(), std::size_t{6});
    check_eq("experiment_setting_count (6 x 3)", tree.experiment_setting_count(), std::size_t{18});

    std::vector<ex::ExperimentSetting> settings;
    tree.for_each_experiment_setting([&](ex::ExperimentSetting const& s) { settings.push_back(s); });
    check_eq("Einstellung[0].binary_id", settings.front().binary_id,
             std::string{"traversal=ART/node=N4/node.cl_line=64"});
    check_eq("Einstellung[0].dyn-Belegung (1 Variable)", settings.front().dynamic_assignment.size(), std::size_t{1});
    check_eq("Einstellung[0].setting_id (Binary + dyn)", settings.front().setting_id,
             std::string{"traversal=ART/node=N4/node.cl_line=64/concurrency.thread_count=1"});
    check_eq("Einstellung[0].pinned_signature", settings.front().pinned_signature, std::string{"traversal=ART"});

    // je Binary 3 Iterationen (gleicher binary_id 3x in den 18 Settings)
    std::size_t art_n4_64 = 0;
    for (auto const& s : settings)
        if (s.binary_id == "traversal=ART/node=N4/node.cl_line=64") ++art_n4_64;
    check_eq("Binary 'ART/N4/64' hat 3 Iterationen", art_n4_64, std::size_t{3});

    // ── Teil 2: Adapter comdare_thesis_profile -> AxisLevels ──
    cx::ThesisProfile tp;
    tp.id             = "t";
    tp.schema_version = 1;
    tp.base_tiers     = {{"art", "../sota/art.profile.xml", "P01"}, {"hot", "../sota/hot.profile.xml", "P02"}};
    // #1-Fix-konform: eine ECHTE Organ-Kompositions-Achse (memory_layout ∈ kCompositionAxisNames) als generische
    // 2-Wert-Achse — der strukturelle Organ-only-binary_id-Guard laesst nur Organ-Achsen ins statische Level. (Frueher
    // stand hier die System-Achse isa als Stand-in; isa ist seit INC-2d KEIN Kompositions-Slot mehr → nicht binary_id.)
    cx::ThesisAxisSpec ml;
    ml.ref    = "memory_layout";
    ml.values = {"aos", "soa"};
    cx::ThesisAxisSpec cl;
    cl.ref           = "cacheline";
    cl.per_organ     = {"node"};
    cl.line_sizes    = {"64", "128"};
    cl.alignments    = {"none"};
    tp.permute_axes  = {ml, cl};
    tp.thread_counts = {"1", "2"};
    cx::ThesisMode m;
    m.name        = "ce_only";
    m.merge       = "Stufe1_CeOnly";
    m.active_axes = {"memory_layout", "cacheline"};
    tp.modes      = {m};

    auto               levels = ex::build_axis_levels(tp, "ce_only", ex::AxisRegistry{});
    ex::ExperimentTree tree2{factory};
    tree2.build(levels);
    // statisch: tier(2) x memory_layout(2) x cl_line(2) x cl_align(1) = 8 Binaries (die binary_id-Quelle).
    check_eq("Adapter: binary_count (2x2x2x1)", tree2.binary_count(), std::size_t{8});
    // dynamisch (Inc2 dyn-Dim-Konsolidierung, 2026-06-18): build_axis_levels ist jetzt die EINZIGE Quelle der
    // runtime_dynamic-Dimensionen UND emittiert die Wiederholungs-Achse (D, KF-10) SELBST. tp hat thread_counts
    // (concurrency) gesetzt + repetitions=Default 3 (repetition) → 2 dynamische Dimensionen; je 2 bzw. 3 Werte.
    check_eq("Adapter: dynamic_filter (thread_count + repetition)", tree2.dynamic_filter().size(), std::size_t{2});
    check_eq("Adapter: experiment_setting_count (8 x 2 x 3)", tree2.experiment_setting_count(), std::size_t{48});

    std::cout << "\n==== KF-9 Experiment-B+-Baum + Adapter: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
