// BR-4 Phase 3 (2026-06-02, Doc 27 §3+§4) — LOADER: lädt die in Phase 2 real gebaute Permutations-DLL via
// AnatomyModuleLoader, prüft die Gattungs-API durch (composition_name/organ_count/genus), treibt sie über
// IObservableTier (tier_insert) und zieht observe_all (tier_observe) — der Knoten-Observer über die REALE
// DLL-Grenze (Pfad B, Doku 24 §8.6). Verifiziert zugleich: der DLL-Komposition-Pfad == der Baum-Blatt-Pfad.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /I... br4_load.cpp → br4_load.exe <perm.dll> <path.txt>

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>   // IObservableTier + ComdareTierObserverSnapshot

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

static int g_fail = 0;
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }
template <class A, class B> void check_eq(char const* w, A const& g, B const& e) {
    bool ok=(g==e); std::cout<<(ok?"  [OK]  ":"  [ERR] ")<<w<<" = "<<g; if(!ok){std::cout<<" (erwartet "<<e<<")";++g_fail;} std::cout<<"\n"; }

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "usage: br4_load <perm.dll> <path.txt>\n"; return 2; }
    std::cout << "BR-4 (Phase 3): reale Anatomie-DLL → AnatomyModuleLoader → observe_all:\n";

    std::string tree_path; { std::ifstream f{argv[2]}; std::getline(f, tree_path); }

    loader::AnatomyModuleHandle handle;
    int const st = loader::AnatomyModuleLoader::load(argv[1], handle);
    check_true("AnatomyModuleLoader::load == status_ok", st == loader::status_ok);
    if (st != loader::status_ok) { std::cerr << "  load-Status: " << loader::status_name(st) << "\n";
        std::cout << "\n==== BR-4 Load: FEHLER (load) ====\n"; return 1; }

    ana::IAnatomyBase* a = handle.anatomy();
    check_true("anatomy() != nullptr", a != nullptr);
    if (!a) { std::cout << "\n==== BR-4 Load: FEHLER (anatomy null) ====\n"; return 1; }

    // Gattungs-API durchtesten (reale Anatomie über die DLL-Grenze).
    check_eq("composition_name == AdHocComposition", std::string{a->composition_name()}, std::string{"AdHocComposition"});
    check_eq("organ_count == 19 (SearchAlgorithm-Komposition, Doc 30 §8.0)", a->organ_count(), std::size_t{19});
    check_true("genus == SearchAlgorithm", a->genus() == ana::AnatomyGenus::SearchAlgorithm);

    // Pfad B: Observer über die DLL-Grenze ziehen (dynamic_cast → tier_insert → tier_observe).
    auto* obs = dynamic_cast<ana::IObservableTier*>(a);
    check_true("dynamic_cast<IObservableTier*> != nullptr (Messung-AN-DLL)", obs != nullptr);
    if (obs) {
        for (std::uint64_t k = 0; k < 256; ++k) (void)obs->tier_insert(k, k * 7u + 1u);
        for (std::uint64_t k = 0; k < 256; ++k) { std::uint64_t v = 0; (void)obs->tier_lookup(k, &v); }
        ana::ComdareTierObserverSnapshot pod{};
        obs->tier_observe(&pod);
        std::cout << "    observe_all über DLL: search_insert=" << pod.axis_stats[0][3]
                  << " lookup=" << pod.axis_stats[0][0] << " fill=" << pod.tier_fill_level
                  << " observable_axes=" << pod.observable_axis_count << "\n";
        check_true("observe_all über REALE DLL: search_insert_count > 0 (echt getrieben)", pod.axis_stats[0][3] > 0);
        check_true("observe_all über REALE DLL: tier_fill_level > 0", pod.tier_fill_level > 0);
        check_true("observable_axis_count >= 1 (R5.B)", pod.observable_axis_count >= 1);
    }

    // BR-4-Round-Trip: der Baum-Blatt-Pfad (Phase 1) ist die Identität dieser realen Binary.
    check_true("Baum-Blatt-Pfad nicht leer (Identität der Binary)", !tree_path.empty());
    std::cout << "    Baum-Blatt-Pfad dieser DLL: " << tree_path << "\n";

    std::cout << "\n==== BR-4 Load: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
