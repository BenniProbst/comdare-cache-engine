// D4b.1 (L-75) — AdapterAbiAdapter in-process: bridge AdapterAnatomy → IAnatomyBase + IAdapterTier.
// Verifiziert die ABI-Adapter-Header (container_tier + container_abi_adapter + container_module_abi_v1-Bausteine)
// OHNE DLL-Komplexität; der echte .dll-Round-Trip folgt in D4b.2 (test_d4b_container_dll via AnatomyModuleLoader).
//
// #87+#90 (2026-06-03, Doku 14 §28): die Adapter-Tier-Unterklasse hat 13 Achsen (12 geteilt/delegiert +
// inner_container), KEINE „ordering"-Achse. Hier: DequeInner (queue-Default, FIFO via get/pop_front); die 12
// geteilten sind im in-process-Test Platzhalter (DelegatedAxis). Ein Adapter ist unbeschränkt (kein Capacity/Flush).
//
// Build: cl /std:c++latest /EHsc /I libs/cache_engine /I libs/cache_engine/src /I build/generated /I <boost> ...

#include "anatomy/adapter_abi_adapter.hpp"
#include "anatomy/adapter_tier.hpp"
#include "anatomy/adapter_anatomy.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace eng = comdare::cache_engine::execution_engine;

// Platzhalter für die 12 geteilten/delegierten §28-Achsen (analog test_container_genus.cpp); die Anatomie
// treibt NUR die spezifische Achse inner_container real, die geteilten werden im Komposition-Typ getragen.
struct DelegatedAxis {};

static int  g_fail = 0;
static void check(bool cond, std::string const& msg) {
    std::cout << (cond ? "  [OK]  " : "  [FAIL] ") << msg << "\n";
    if (!cond) ++g_fail;
}

int main() {
    using D    = DelegatedAxis;
    using Comp = cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, cea::DequeInner<>>; // 11 + inner (INC-2c)
    using Anat = cea::AdapterAnatomy<Comp>;
    cea::AdapterAbiAdapter<Anat> adapter; // unbeschränkter Container-Adapter (13 Achsen)

    std::cout << "==== D4b.1 AdapterAbiAdapter über IAnatomyBase ====\n";
    cea::IAnatomyBase* base = &adapter;
    check(base->genus() == cea::AnatomyGenus::Adapter, "genus() == Adapter");
    check(base->engine_kind() == eng::ExecutionEngineKind::Anatomy, "engine_kind() == Anatomy");
    check(base->organ_count() == 11, "organ_count() == 11 (§28 + INC-2c: 11 geteilt/delegiert + inner_container)");
    check(std::string{base->composition_name()} == "AdapterComposition", "composition_name == AdapterComposition");
    base->warm_up();
    base->run();
    check(base->lifecycle_state() == eng::EngineLifecycleState::Running, "lifecycle: warm_up→run → Running");

    std::cout << "\n==== D4b.1 Container-Antrieb über IAdapterTier (dynamic_cast, ABI-Pfad) ====\n";
    auto* ct = dynamic_cast<cea::IAdapterTier*>(base);
    check(ct != nullptr, "dynamic_cast<IAdapterTier*> != null (Container-Sub-Interface vorhanden)");
    if (ct != nullptr) {
        for (std::uint64_t i = 0; i < 20; ++i) ct->tier_put(i); // unbeschränkt → alle 20 verbleiben
        cea::AdapterObserverSnapshotV1 obs{};
        ct->tier_observe_container(&obs);
        check(obs.push_count == 20, "tier_observe_container: push_count == 20 (über Interface getrieben)");
        check(obs.organ_count == 11, "tier_observe_container: organ_count == 11 (INC-2c)");
        check(obs.peak_occupancy == 20, "tier_observe_container: peak_occupancy == 20 (unbeschränkter Adapter)");
        check(ct->tier_size() == 20, "tier_size() == 20 (kein Flush — Container-Adapter unbeschränkt)");
        std::uint64_t out = 999;
        bool const    got = ct->tier_get(&out);
        check(got, "tier_get() liefert ein Element (FIFO)");
        check(out == 0, "FIFO: tier_get() liefert das vorderste Element (0)");
        ct->tier_observe_container(&obs);
        check(obs.pop_count == 1, "tier_observe_container: pop_count == 1 nach einem get");
        check(obs.front_reads == 1, "tier_observe_container: front_reads == 1 (ein get == pop_front)");
        check(obs.back_reads == 0, "tier_observe_container: back_reads == 0 (kein pop_back/top)");
        ct->tier_clear();
        check(ct->tier_size() == 0, "tier_clear() → tier_size() == 0");
    }

    std::cout << "\n==== " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
