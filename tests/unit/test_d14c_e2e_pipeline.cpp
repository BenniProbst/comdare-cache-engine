// D14c / L-CLUSTER-E2E — e2e_pipeline: durchgängige lokale Mess-Pipeline (lazy View → BuildSelection →
// MeasureFn → result_ingest → Baum), O(K). Beweis: 3 Binaries aus einer 1e9-View → MeasureFn genau 3× (NICHT
// 1e9), 3 Knoten im Baum, kein OOM. Mock-MeasureFn (echte DLL-Mess-Strecke separat belegt). Build: cl /I libs/cache_engine.

#include "builder/experiment_tree/e2e_pipeline.hpp"
#include "builder/experiment_tree/coverage_selection.hpp"
#include "builder/experiment_tree/perm_runner.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace ana = comdare::cache_engine::anatomy;

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D14c e2e_pipeline (lazy View → Selektion → Mess → Ingest → Baum, O(K)) ====\n";
    // 3 Ebenen × 1000 Werte → ∏ = 1e9 (lazy, nie materialisiert).
    std::vector<ex::AxisLevel> levels;
    for (int e = 0; e < 3; ++e) {
        std::vector<std::string> vals; vals.reserve(1000);
        for (int k = 0; k < 1000; ++k) vals.push_back("v" + std::to_string(k));
        levels.push_back({"ax" + std::to_string(e), std::move(vals), true, "", "blk"});
    }
    ex::StaticBinaryView view{levels};
    eq("View size() == 1e9 (1000^3)", view.size(), static_cast<std::size_t>(1000000000ULL));

    // 3 explizite Indizes — einer am Ende der 1e9-View (O(K)-Beleg, kein ∏-Durchlauf).
    auto const sel = ex::select_explicit({0, 12345, 999999999});

    std::atomic<int> measure_calls{0};
    ex::MeasureFn measure = [&](std::string const& bid) -> std::string {
        ++measure_calls;
        ana::ComdareTierObserverSnapshotV1 snap{};
        snap.search_insert_count = 42; snap.search_lookup_count = 42;
        snap.tier_fill_level = 42; snap.observable_axis_count = 2;
        return ex::format_perm_result(bid, snap);   // exakt wie perm_runner
    };

    ex::ExperimentTree tree{std::make_shared<ex::ExperimentNodeFactory>()};
    auto const st = ex::run_e2e_pipeline(tree, view, sel.indices, measure);

    eq("selected == 3", st.selected, std::size_t{3});
    eq("measured == 3 (in Baum eingespielt)", st.measured, std::size_t{3});
    eq("MeasureFn genau 3x aufgerufen (O(K), NICHT 1e9 → kein OOM)", measure_calls.load(), 3);
    eq("measured_node_count() == 3 (sparse)", tree.measured_node_count(), std::size_t{3});

    // Der gemessene NodeValue des ersten selektierten Binary trägt die Mess-Werte.
    auto const spec0 = view[sel.indices[0]];
    auto const nv = tree.node_value(spec0.binary_id);
    tr("Knoten[0] observer_real", nv.observer_real);
    eq("Knoten[0] search_insert_count == 42 (durch die ganze Pipeline)", nv.observer.search_insert_count, std::uint64_t{42});
    eq("Knoten[0] tier_fill_level == 42", nv.observer.tier_fill_level, std::uint64_t{42});

    std::cout << "\n==== D14c e2e_pipeline: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
