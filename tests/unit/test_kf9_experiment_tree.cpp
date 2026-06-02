// test_kf9_experiment_tree — KF-9 (2026-06-02)
// Standalone-Test für den Experiment-B+-Baum: statische Knoten = Binaries, dynamische = Laufzeit-for-Schleife.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf9_experiment_tree.cpp

#include "builder/experiment_tree/experiment_tree.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();

    // Abstract Factory: zwei Knotenarten + Ausführungssemantik
    auto sn = factory->make_static("traversal", "ART", true);
    auto dn = factory->make_dynamic("concurrency", "thread_count", "2");
    check_true("static = NICHT Laufzeit-Schleife (laedt Binary)", !sn->is_runtime_loop());
    check_true("static traegt Binary-Signatur bei", sn->contributes_to_signature());
    check_true("dynamic = Laufzeit-Schleife (auf geladener Binary)", dn->is_runtime_loop());
    check_true("dynamic traegt KEINE Binary-Signatur bei", !dn->contributes_to_signature());
    check_eq("dynamic.serialize", dn->serialize(), std::string{"concurrency.thread_count=2"});

    // Baum: STATISCH traversal(1,pinned) x node(3) x node.cl_line(2) = 6 Binaries;
    //       DYNAMISCH concurrency.thread_count(3) = Laufzeit-Schleife je Binary -> 18 Mess-Blaetter.
    ex::ExperimentTree tree{factory};
    std::vector<ex::AxisLevel> levels = {
        ex::AxisLevel{"traversal", {"ART"}, true, ""},
        ex::AxisLevel{"node", {"N4", "N16", "N256"}, true, ""},
        ex::AxisLevel{"node.cl_line", {"64", "128"}, true, ""},          // compile-time cacheline -> Binary
        ex::AxisLevel{"concurrency", {"1", "2", "4"}, false, "thread_count"},  // Laufzeit-Schleife
    };
    tree.build(levels);

    check_eq("Mess-Blaetter (1x3x2 x 3)", tree.leaf_count(), std::size_t{18});
    check_eq("distinkte Tier-Binaries (1x3x2)", tree.binary_count(), std::size_t{6});

    std::vector<ex::LeafView> leaves;
    tree.for_each_leaf([&](ex::LeafView const& lv) { leaves.push_back(lv); });
    check_eq("Blatt[0].binary_id (nur statisch)", leaves.front().binary_id,
             std::string{"traversal=ART/node=N4/node.cl_line=64"});
    check_eq("Blatt[0].runtime_setting (nur dynamisch)", leaves.front().runtime_setting,
             std::string{"concurrency.thread_count=1"});
    check_eq("Blatt[0].pinned_signature", leaves.front().pinned_signature, std::string{"traversal=ART"});
    check_eq("Blatt[0].path (voll)", leaves.front().path,
             std::string{"traversal=ART/node=N4/node.cl_line=64/concurrency.thread_count=1"});

    // Prüf-Dock-Modell: je Binary einmal laden, 3 Laufzeit-Einstellungen durchschleifen
    std::size_t bins = 0; bool all_three = true;
    tree.for_each_binary([&](std::string const& bin, std::vector<std::string> const& settings) {
        ++bins; if (settings.size() != 3) all_three = false; (void)bin;
    });
    check_eq("for_each_binary: Binaries", bins, std::size_t{6});
    check_true("je Binary 3 Laufzeit-Einstellungen", all_three);

    // KF-15: alle Binaries teilen pinned-Signatur "traversal=ART"
    auto idx = tree.pinned_signature_index();
    check_eq("multimap unter 'traversal=ART' (18 Mess-Blaetter)", idx.count("traversal=ART"), std::size_t{18});

    std::cout << "\n==== KF-9 Experiment-B+-Baum: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
