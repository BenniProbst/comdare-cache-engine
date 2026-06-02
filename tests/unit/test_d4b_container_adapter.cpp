// D4b.1 (L-75) — ContainerAbiAdapter in-process: bridge ContainerAnatomy → IAnatomyBase + IContainerTier.
// Verifiziert die ABI-Adapter-Header (container_tier + container_abi_adapter + container_module_abi_v1-Bausteine)
// OHNE DLL-Komplexität; der echte .dll-Round-Trip folgt in D4b.2 (test_d4b_container_dll via AnatomyModuleLoader).
//
// Build: cl /std:c++latest /EHsc /I libs/cache_engine /I libs/cache_engine/src /I build/generated /I <boost> ...

#include "anatomy/container_abi_adapter.hpp"
#include "anatomy/container_tier.hpp"
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace eng = comdare::cache_engine::execution_engine;
namespace q1  = comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2  = comdare::cache_engine::queuing::axis_q2_queuing;

static int g_fail = 0;
static void check(bool cond, std::string const& msg) {
    std::cout << (cond ? "  [OK]  " : "  [FAIL] ") << msg << "\n";
    if (!cond) ++g_fail;
}

int main() {
    using Comp = cea::ContainerComposition<q1::FIFOQueueBuffer, q2::WatermarkFlush>;
    using Anat = cea::ContainerAnatomy<Comp>;
    cea::ContainerAbiAdapter<Anat> adapter;   // kCapacity=16 → Watermark 75% aktiv über die Adapter-Grenze

    std::cout << "==== D4b.1 ContainerAbiAdapter über IAnatomyBase ====\n";
    cea::IAnatomyBase* base = &adapter;
    check(base->genus() == cea::AnatomyGenus::Adapter, "genus() == Adapter");
    check(base->engine_kind() == eng::ExecutionEngineKind::Anatomy, "engine_kind() == Anatomy");
    check(base->organ_count() == 2, "organ_count() == 2 (buffer + flush)");
    check(std::string{base->composition_name()} == "ContainerComposition", "composition_name == ContainerComposition");
    base->warm_up(); base->run();
    check(base->lifecycle_state() == eng::EngineLifecycleState::Running, "lifecycle: warm_up→run → Running");

    std::cout << "\n==== D4b.1 Container-Antrieb über IContainerTier (dynamic_cast, ABI-Pfad) ====\n";
    auto* ct = dynamic_cast<cea::IContainerTier*>(base);
    check(ct != nullptr, "dynamic_cast<IContainerTier*> != null (Container-Sub-Interface vorhanden)");
    if (ct != nullptr) {
        for (std::uint64_t i = 0; i < 20; ++i) ct->tier_put(i);   // cap 16 → flush bei fill>=12
        cea::ContainerObserverSnapshotV1 obs{};
        ct->tier_observe_container(&obs);
        check(obs.put_count == 20, "tier_observe_container: put_count == 20 (über Interface getrieben)");
        check(obs.organ_count == 2, "tier_observe_container: organ_count == 2");
        check(obs.flush_decisions_evaluated == 20, "tier_observe_container: flush_decisions_evaluated == 20");
        check(obs.full_flush_count > 0, "tier_observe_container: full_flush_count > 0 (Q2 Watermark spülte über ABI)");
        check(ct->tier_size() < 12, "tier_size() < 12 nach Spülungen");
        std::uint64_t out = 999;
        // nach Spülungen ist evtl. etwas im Buffer (puts 12..19 → 8 Elemente, minus Spülung bei 12...)
        bool const got = ct->tier_get(&out);
        check(got || ct->tier_size() == 0, "tier_get() konsistent mit tier_size()");
        ct->tier_clear();
        check(ct->tier_size() == 0, "tier_clear() → tier_size() == 0");
    }

    std::cout << "\n==== " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
