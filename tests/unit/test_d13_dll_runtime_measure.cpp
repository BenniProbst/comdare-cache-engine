// D13-DLL / L-MEAS — der HostMeasureLoop über eine ECHTE geladene DLL (statt MockTier). Lädt eine
// SearchAlgorithm-adhoc-DLL via AnatomyModuleLoader, zieht via dynamic_cast BEIDE Sub-Interfaces (IObservableTier
// für Antrieb+Observer, IResourceControllableTier für die dyn. Steuerung) aus DEMSELBEN geladenen Tier, und fährt
// die Mess-Schleife: je dyn. Einstellung (thread_count ∈ {1,2,4}) × repeats=3 Workload + tier_observe — OHNE Reload.
// Beweist L-MEAS end-to-end über die echte .dll-Grenze. Aufruf: <searchalgo_adhoc.dll>.

#include <builder/experiment_tree/host_measure_loop.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>

#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace ex     = comdare::cache_engine::builder::experiment;
namespace ana    = comdare::cache_engine::anatomy;
namespace loader = comdare::cache_engine::builder::anatomy_loader;

// Host-Bridge: hält BEIDE dynamic_cast-Pointer auf DASSELBE geladene Tier + vereint sie für den Visitor. Sie ERBT
// IResourceControllableTier (damit `loop.run(bridge → IResourceControllableTier&)` upcastet — der RuntimeVariableLoop
// nimmt eine IResourceControllableTier&, nicht templated) + überschreibt dessen 2 Methoden (forward an rc); die
// IObservableTier-Antriebs-/Observer-Methoden sind regulär (der templated measure ruft sie duck-typed an obs).
struct DllTierBridge final : ana::IResourceControllableTier {
    ana::IObservableTier*           obs;
    ana::IResourceControllableTier* rc;
    DllTierBridge(ana::IObservableTier* o, ana::IResourceControllableTier* r) noexcept : obs(o), rc(r) {}
    // IObservableTier-Antrieb/Observer (duck-typed für den templated measure):
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept { return obs->tier_insert(k, v); }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept { return obs->tier_lookup(k, o); }
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept { return obs->tier_erase(k); }
    void               tier_clear() noexcept { obs->tier_clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept { return obs->tier_size(); }
    void                        tier_observe(ana::ComdareTierObserverSnapshot* o) const noexcept {
        obs->tier_observe(o);
    } // I1: die EINE Observer-Methode
    // IResourceControllableTier (override → forward an rc, damit loop.run den Upcast nehmen kann):
    void tier_query_resource_caps(ana::ComdareResourceControlV1* o) const noexcept override {
        rc->tier_query_resource_caps(o);
    }
    [[nodiscard]] std::uint64_t tier_apply_resource_control(ana::ComdareResourceControlV1 const* i) noexcept override {
        return rc->tier_apply_resource_control(i);
    }
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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_d13_dll_runtime_measure <searchalgo_adhoc.dll>\n";
        return 2;
    }
    std::cout << "==== D13-DLL / L-MEAS — HostMeasureLoop über die ECHTE .dll-Grenze ====\n";

    loader::AnatomyModuleHandle handle;
    int const                   st = loader::AnatomyModuleLoader::load(argv[1], handle);
    tr("load == status_ok", st == loader::status_ok);
    if (st != loader::status_ok) {
        std::cerr << "  status: " << loader::status_name(st) << "\n";
        return 1;
    }

    ana::IAnatomyBase* base = handle.anatomy();
    tr("anatomy() != null", base != nullptr);
    tr("genus() == SearchAlgorithm", base != nullptr && base->genus() == ana::AnatomyGenus::SearchAlgorithm);

    auto* obs = dynamic_cast<ana::IObservableTier*>(base);
    auto* rc  = dynamic_cast<ana::IResourceControllableTier*>(base);
    tr("dynamic_cast<IObservableTier*> != null (Antrieb+Observer über DLL)", obs != nullptr);
    tr("dynamic_cast<IResourceControllableTier*> != null (Steuer-Ebene über DLL)", rc != nullptr);
    if (obs == nullptr || rc == nullptr) {
        std::cout << "  (DLL ohne L-MEAS-Steuer-Ebene → Abbruch)\n";
        return 1;
    }

    // Caps-Quere über die DLL: die SearchAlgorithm-Komposition meldet 5 steuerbare Achsen.
    ana::ComdareResourceControlV1 caps{};
    rc->tier_query_resource_caps(&caps);
    eq("caps.controllable_axis_count == 5 (über DLL)", caps.controllable_axis_count, std::uint64_t{5});
    eq("caps.thread_count == 64 (concurrency-cap über DLL)", caps.thread_count, std::uint64_t{64});

    // Die Mess-Schleife über DIESELBE geladene Tier (kein Reload): thread_count ∈ {1,2,4} × 3 Wiederholungen.
    DllTierBridge               bridge{obs, rc};
    std::vector<ex::DynamicDim> dims = {{"concurrency", "thread_count", {"1", "2", "4"}, "blk_conc"}};
    ex::HostMeasureLoop         vis{};
    auto const                  pts = vis.measure(bridge, dims, /*n_ops=*/20, /*repeats=*/3);

    eq("Mess-Punkte == 9 (3 Einstellungen × 3 Wiederholungen)", pts.size(), std::size_t{9});
    bool                  all_measured = true, all_label_only = true;
    std::set<std::string> labels;
    for (auto const& p : pts) {
        if (p.observer.search_insert_count == 0) all_measured = false; // echt über die DLL getrieben?
        // T8-Ehrlichkeit (Achsen-Ontologie-Verifikation 2026-07-11): thread_count ist LABEL-ONLY — der
        // In-Prozess-In-Memory-Tier konsumiert es nicht (runtime_thread_count() = null Consumer), also zählt
        // apply1 es counts=false → applied_axis_count == 0. Der Sweep produziert weiter 3 distinkte, real
        // gemessene Label-Punkte; nur der Phantom-"applied" ist weg. Dieser Test ist der DLL-Ebenen-Guard.
        if (p.applied_axis_count != 0) all_label_only = false;
        labels.insert(p.setting_label);
    }
    tr("jeder Punkt: observer.search_insert_count > 0 (echt über die DLL getrieben)", all_measured);
    tr("jeder Punkt: applied_axis_count == 0 (thread_count label-only über DLL — T8-Phantom-Guard)", all_label_only);
    eq("3 distinkte dyn. Einstellungs-Labels", labels.size(), std::size_t{3});

    int rep_counts[3] = {0, 0, 0};
    for (auto const& p : pts)
        if (p.repeat_index < 3) ++rep_counts[p.repeat_index];
    tr("je 3 Punkte mit repeat_index 0/1/2 (KF-10 separat, kein Reload)",
       rep_counts[0] == 3 && rep_counts[1] == 3 && rep_counts[2] == 3);
    tr("tier_size() > 0 über DLL (Workload-Daten persistieren — kein Reload zwischen Einstellungen)",
       obs->tier_size() > 0);

    std::cout << "\n==== D13-DLL L-MEAS: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
