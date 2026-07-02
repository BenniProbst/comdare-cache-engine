// D13 / L-MEAS — RuntimeMeasureVisitor: die host-seitige Mess-Schleife je geladener Binary. Verifiziert: je
// dynamischer Einstellung (RuntimeVariableLoop-Kartesik) wird ein Workload gefahren + tier_observe gezogen,
// je repeats — OHNE Reload. Mock-Tier (IObservableTier + IResourceControllableTier) belegt die Mechanik
// leichtgewichtig. Build: cl /I libs/cache_engine (kein Boost).

#include "builder/experiment_tree/runtime_measure_visitor.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace ana = comdare::cache_engine::anatomy;

// Mock-Tier: erfüllt IObservableTier (Antrieb + Observer) + IResourceControllableTier (Loop-Steuerung).
// #203: anonymer Namespace — gleichnamige TU-lokale Helper in anderen Test-TUs (cppcheck-CTU-ODR).
namespace {
struct MockTier final : ana::IObservableTier, ana::IResourceControllableTier {
    std::map<std::uint64_t, std::uint64_t> data;
    std::uint64_t                          inserts        = 0;
    mutable std::uint64_t                  lookups        = 0;
    std::uint64_t                          applied_thread = 0;
    // IDriveableTier
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool n = data.emplace(k, v).second;
        ++inserts;
        return n;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        ++lookups;
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
    // IObservableTier — die EINE konsolidierte tier_observe: search → axis_stats[0].
    void tier_observe(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        if (!out) return;
        out->axis_stats[0][3]      = inserts;
        out->axis_stats[0][0]      = lookups; // T0 search insert/lookup
        out->tier_fill_level       = data.size();
        out->observable_axis_count = 2;
    }
    // IResourceControllableTier
    void tier_query_resource_caps(ana::ComdareResourceControlV1* o) const noexcept override {
        if (!o) return;
        o->thread_count            = 8;
        o->controllable_axis_count = 1;
    }
    [[nodiscard]] std::uint64_t tier_apply_resource_control(ana::ComdareResourceControlV1 const* in) noexcept override {
        if (!in) return 0;
        applied_thread = in->thread_count;
        return (in->thread_count > 0) ? 1u : 0u;
    }
};
} // namespace

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
    std::cout << "==== D13 RuntimeMeasureVisitor (dyn. Variablen × Messung je Binary, kein Reload) ====\n";
    MockTier mock;
    // 1 dynamische Dimension: concurrency.thread_count ∈ {1,2,4} → 3 Einstellungen.
    std::vector<ex::DynamicDim> dims = {{"concurrency", "thread_count", {"1", "2", "4"}, "blk_conc"}};

    ex::RuntimeMeasureVisitor vis{};
    auto const                pts = vis.measure(mock, dims, /*n_ops=*/10, /*repeats=*/3);

    eq("Punkte == 9 (3 Einstellungen × 3 Wiederholungen)", pts.size(), std::size_t{9});

    bool                  all_measured = true, all_applied = true;
    std::set<std::string> labels;
    for (auto const& p : pts) {
        if (p.observer.search_insert_count == 0) all_measured = false; // echt getrieben?
        if (p.applied_axis_count != 1) all_applied = false;            // thread_count real angewandt (cap 8)?
        labels.insert(p.setting_label);
    }
    tr("jeder Punkt: observer.search_insert_count > 0 (echt getrieben, kein Stub)", all_measured);
    tr("jeder Punkt: applied_axis_count == 1 (thread_count auf cap 8 geklammert angewandt)", all_applied);
    eq("3 distinkte dyn. Einstellungs-Labels (thread_count=1/2/4)", labels.size(), std::size_t{3});
    tr("Label enthaelt 'thread_count='",
       !pts.empty() && pts.front().setting_label.find("thread_count=") != std::string::npos);

    // KF-10: je Einstellung 3 SEPARATE Roh-Wiederholungen (repeat_index 0/1/2), nie interpoliert.
    int rep_counts[3] = {0, 0, 0};
    for (auto const& p : pts)
        if (p.repeat_index < 3) ++rep_counts[p.repeat_index];
    tr("je 3 Punkte mit repeat_index 0/1/2 (KF-10 separat)",
       rep_counts[0] == 3 && rep_counts[1] == 3 && rep_counts[2] == 3);

    // Kein Reload: derselbe Mock über alle Einstellungen (letzte angewandte Einstellung = thread_count 4).
    eq("Mock kein Reload: applied_thread == 4 (letzte Einstellung)", mock.applied_thread, std::uint64_t{4});
    tr("Mock kein Reload: tier_size() > 0 (Workload-Daten persistieren über Einstellungen)", mock.tier_size() > 0);

    std::cout << "\n==== D13 Mess-Schleife: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
