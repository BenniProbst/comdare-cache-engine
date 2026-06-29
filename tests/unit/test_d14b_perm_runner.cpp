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
    std::uint64_t                          inserts = 0;
    mutable std::uint64_t                  lookups = 0, hits = 0;
    // (Audit K9) std::map-konform (insert-or-assign): bei Duplikat WIRD der Wert aktualisiert; return true = NEUER Key.
    // Exakt der Vertrag des Konformitäts-Gate-Oracles (RF3), identisch zu test_v5_conformance_gate::GoodTier. Der frühere
    // emplace() ließ den Wert bei Duplikat stehen → Gate-Bruch (vom neu verdrahteten Gate korrekt aufgedeckt).
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const r = data.insert({k, v});
        if (!r.second) { r.first->second = v; }
        ++inserts;
        return r.second;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        ++lookups;
        auto it = data.find(k);
        if (it != data.end()) {
            ++hits;
            if (o) *o = it->second;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept override { return data.erase(k) > 0; }
    // (Audit K9) vertragstreu wie reale Tiere: tier_clear() nullt die Mess-Statistik je Lauf (real: search_organ_.reset(),
    // perm_runner.hpp:95-97). Ohne dies würden die Konformitäts-Gate-Ops (RF1–7 + 2000 Zufalls-Ops) die Zähler verfälschen.
    void tier_clear() noexcept override {
        data.clear();
        inserts = 0;
        lookups = 0;
        hits    = 0;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data.size(); }
    // Die EINE konsolidierte tier_observe — search-Stats nach axis_stats[0] (T0). Wird von run_observable_perm
    // für das (volle-Matrix-)Wire-Format genutzt; ingest leitet die benannten Legacy-Felder daraus ab.
    void tier_observe(ana::ComdareTierObserverSnapshot* o) const noexcept override {
        if (!o) return;
        o->axis_stats[0][0]      = lookups;
        o->axis_stats[0][1]      = hits;
        o->axis_stats[0][3]      = inserts; // T0 search lookup/hit/insert
        o->tier_fill_level       = data.size();
        o->observable_axis_count = 2;
        o->filled_axis_count     = 1;
    }
};

// (Audit K9) Absichtlich NICHT std::map-konform: tier_insert meldet IMMER „neu" (true), auch beim Update → verletzt
// die Oracle-Zusicherung RF3 (Duplikat-Insert muss false liefern). Das Konformitäts-Gate MUSS das erkennen und die
// Hülle von der Messung ausschließen (gated≠gültig). Beleg, dass das Gate im Voll-Lauf-Pfad wirklich greift.
struct BrokenTier final : ana::IObservableTier {
    std::map<std::uint64_t, std::uint64_t> data;
    [[nodiscard]] bool                     tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        data[k] = v;
        return true;
    } // BUG
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        auto it = data.find(k);
        if (it != data.end()) {
            if (o) *o = it->second;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t k) noexcept override { return data.erase(k) > 0; }
    void                        tier_clear() noexcept override { data.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data.size(); }
    void                        tier_observe(ana::ComdareTierObserverSnapshot*) const noexcept override {}
};

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "==== D14b perm_runner → result_ingest Round-Trip ====\n";
    MockObsTier          tier;
    std::string const    bid  = "search_algo=Array256/allocator=mimalloc";
    ex::PermResult const pr   = ex::run_observable_perm(tier, bid, /*n_ops=*/100); // 100 insert + 100 lookup
    std::string const    line = pr.line;
    std::cout << "    perm-Zeile: " << line << "\n";
    tr("Zeile beginnt mit binary_id", line.rfind(bid + ";", 0) == 0);
    // (Audit K9) Konformitäts-Gate wurde im Voll-Lauf-Pfad GERUFEN (import→GATE→messen) + bestanden (std::map-Mock).
    tr("Konformitäts-Gate gelaufen (cases_total>0)", pr.conformance_cases_total > 0);
    tr("Konformitäts-Gate bestanden (std::map-konformer Mock)", pr.conformance_passed);
    eq("Gate: alle Fälle bestanden", pr.conformance_cases_passed, pr.conformance_cases_total);

    // Round-Trip: die perm_runner-Zeile via result_ingest in den Baum spielen.
    ex::ExperimentTree tree{std::make_shared<ex::ExperimentNodeFactory>()};
    std::size_t const  n = ex::ingest_results(tree, line);
    eq("ingest_results == 1", n, std::size_t{1});

    auto const nv = tree.node_value(bid);
    tr("observer_real (über perm_runner gemessen + eingespielt)", nv.observer_real);
    eq("search_insert_count == 100 (Mess→Format→Ingest identisch)", nv.observer.search_insert_count,
       std::uint64_t{100});
    eq("search_lookup_count == 100", nv.observer.search_lookup_count, std::uint64_t{100});
    eq("search_hit_count == 100 (alle keys getroffen)", nv.observer.search_hit_count, std::uint64_t{100});
    eq("tier_fill_level == 100", nv.observer.tier_fill_level, std::uint64_t{100});
    eq("observable_axis_count == 2", nv.observer.observable_axis_count, std::uint64_t{2});

    // (Audit K9) Negativ-Nachweis: eine NICHT-std::map-konforme Hülle wird vom Gate erkannt → KEINE gültige Mess-Zeile.
    BrokenTier           broken;
    ex::PermResult const bpr = ex::run_observable_perm(broken, "search_algo=Broken", 100);
    tr("nicht-konform: Gate NICHT bestanden", !bpr.conformance_passed);
    tr("nicht-konform: first_fail gesetzt (erste Oracle-Verletzung)", bpr.conformance_first_fail > 0);
    tr("nicht-konform: NICHT als gültig gemessen (unified_real=false)", !bpr.unified_real);

    std::cout << "\n==== D14b perm_runner: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
