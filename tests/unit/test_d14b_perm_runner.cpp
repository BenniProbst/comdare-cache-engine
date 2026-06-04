// D14b / L-CLUSTER-E2E — perm_runner: Mess-Runner je Binary → result_ingest-Zeile → Baum-NodeValue.
// End-to-End-Round-Trip der gate-freien Cluster-Kette: run_observable_perm (Mess+Format) ↔ ingest_results
// (Format→Baum) liefern IDENTISCHE Werte. Mock-IObservableTier (DLL-Lade via AnatomyModuleLoader separat belegt,
// test_dgenus/test_d4b). Build: cl /I libs/cache_engine (kein Boost).

#include "builder/experiment_tree/perm_runner.hpp"
#include "builder/experiment_tree/result_ingest.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace ana = comdare::cache_engine::anatomy;

// Mock-IObservableTier mit search_algo-Statistik (deterministisch).
struct MockObsTier final : ana::IObservableTier {
    std::map<std::uint64_t, std::uint64_t> data;
    std::uint64_t         inserts = 0;
    mutable std::uint64_t lookups = 0, hits = 0;
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool n = data.emplace(k, v).second; ++inserts; return n; }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        ++lookups; auto it = data.find(k); if (it != data.end()) { ++hits; if (o) *o = it->second; return true; } return false; }
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept override { return data.erase(k) > 0; }
    void tier_clear() noexcept override { data.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data.size(); }
    void tier_observe(ana::ComdareTierObserverSnapshotV1* o) const noexcept override {
        if (!o) return;
        o->search_insert_count = inserts; o->search_lookup_count = lookups; o->search_hit_count = hits;
        o->tier_fill_level = data.size(); o->observable_axis_count = 2;
    }
};

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D14b perm_runner → result_ingest Round-Trip ====\n";
    MockObsTier tier;
    std::string const bid = "search_algo=Array256/allocator=mimalloc";
    std::string const line = ex::run_observable_perm(tier, bid, /*n_ops=*/100).line;  // 100 insert + 100 lookup (PermResult.line)
    std::cout << "    perm-Zeile: " << line << "\n";
    tr("Zeile beginnt mit binary_id", line.rfind(bid + ";", 0) == 0);

    // Round-Trip: die perm_runner-Zeile via result_ingest in den Baum spielen.
    ex::ExperimentTree tree{std::make_shared<ex::ExperimentNodeFactory>()};
    std::size_t const n = ex::ingest_results(tree, line);
    eq("ingest_results == 1", n, std::size_t{1});

    auto const nv = tree.node_value(bid);
    tr("observer_real (über perm_runner gemessen + eingespielt)", nv.observer_real);
    eq("search_insert_count == 100 (Mess→Format→Ingest identisch)", nv.observer.search_insert_count, std::uint64_t{100});
    eq("search_lookup_count == 100", nv.observer.search_lookup_count, std::uint64_t{100});
    eq("search_hit_count == 100 (alle keys getroffen)", nv.observer.search_hit_count, std::uint64_t{100});
    eq("tier_fill_level == 100", nv.observer.tier_fill_level, std::uint64_t{100});
    eq("observable_axis_count == 2", nv.observer.observable_axis_count, std::uint64_t{2});

    std::cout << "\n==== D14b perm_runner: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
