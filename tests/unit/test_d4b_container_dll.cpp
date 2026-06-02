// D4b.2 (L-75, Doc 24 §8.8 + analog BR-4 §3) — Container-DLL-Round-Trip: lädt die real gebaute Container-
// Permutations-DLL via AnatomyModuleLoader (gattungs-agnostisch), prüft die Gattungs-API (genus==Adapter,
// organ_count==2), fragt das Container-Sub-Interface via dynamic_cast<IContainerTier*> ab und treibt put/get +
// zieht tier_observe_container über die REALE .dll-Grenze. Beweist: die Container-Gattung ist DLL-baubar/-ladbar/
// -observierbar wie SearchAlgorithm (BR-4), über DENSELBEN Loader (keine Loader-Änderung — Doc 24 §8.8).
//
// Build: cl /std:c++latest /EHsc test_d4b_container_dll.cpp anatomy_module_loader.cpp → exe <perm_container.dll>

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/container_tier.hpp>   // IContainerTier + ContainerObserverSnapshotV1

#include <cstdint>
#include <iostream>
#include <string>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

static int g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail;
}
template <class A, class B> static void check_eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: test_d4b_container_dll <perm_container.dll>\n"; return 2; }
    std::cout << "D4b.2: Container-DLL → AnatomyModuleLoader (gattungs-agnostisch) → IContainerTier:\n";

    loader::AnatomyModuleHandle handle;
    int const st = loader::AnatomyModuleLoader::load(argv[1], handle);
    check_true("AnatomyModuleLoader::load == status_ok", st == loader::status_ok);
    if (st != loader::status_ok) { std::cerr << "  load-Status: " << loader::status_name(st) << "\n";
        std::cout << "\n==== D4b.2: FEHLER (load) ====\n"; return 1; }

    ana::IAnatomyBase* a = handle.anatomy();
    check_true("anatomy() != nullptr", a != nullptr);
    if (!a) { std::cout << "\n==== D4b.2: FEHLER (anatomy null) ====\n"; return 1; }

    // Gattungs-API über die DLL-Grenze: das ist eine CONTAINER-Gattung (NICHT SearchAlgorithm).
    check_true("genus == Adapter (Container über DLL)", a->genus() == ana::AnatomyGenus::Adapter);
    check_eq("organ_count == 2 (buffer + flush)", a->organ_count(), std::size_t{2});
    check_eq("composition_name == ContainerComposition", std::string{a->composition_name()}, std::string{"ContainerComposition"});

    // Container-Antrieb über die DLL-Grenze: dynamic_cast auf das Container-Sub-Interface.
    auto* ct = dynamic_cast<ana::IContainerTier*>(a);
    check_true("dynamic_cast<IContainerTier*> != nullptr (Container-Sub-Interface über DLL)", ct != nullptr);
    if (ct) {
        for (std::uint64_t i = 0; i < 20; ++i) ct->tier_put(i);   // kCapacity 16 → Watermark 75% → Flush bei ~12
        ana::ContainerObserverSnapshotV1 pod{};
        ct->tier_observe_container(&pod);
        std::cout << "    observe über DLL: put=" << pod.put_count << " full_flush=" << pod.full_flush_count
                  << " decisions=" << pod.flush_decisions_evaluated << " organs=" << pod.organ_count
                  << " size=" << ct->tier_size() << "\n";
        check_eq("tier_observe_container: put_count == 20 (über DLL getrieben)", pod.put_count, std::uint64_t{20});
        check_eq("tier_observe_container: organ_count == 2", pod.organ_count, std::uint64_t{2});
        check_true("tier_observe_container: full_flush_count > 0 (Q2 Watermark spülte über DLL-Grenze)", pod.full_flush_count > 0);
        std::uint64_t out = 0;
        bool const got = ct->tier_get(&out);
        check_true("tier_get() konsistent mit tier_size() über DLL", got || ct->tier_size() == 0);
    }

    std::cout << "\n==== D4b.2 Container-DLL-Round-Trip: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
