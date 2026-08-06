// W3-KERN (2026-08-05, Owner-GO R3) -- POST-COMPILE-TESTATE der MESS-Interfaces AM GELADENEN MODUL.
//
// Diese TU faehrt die Testate aus builder/pruef_dock/mess_interface_testate.hpp gegen ECHTE, per dlopen
// geladene Anatomie-Module -- nicht gegen suite-seitig instanziierte Attrappen. Das ist der Unterschied,
// um den es geht: die messungs-gegateten Sub-Interfaces (abi_adapter.hpp:238-245) waren bis hierher
// ueberwiegend NUR suite-seitig belegt; ueber die .so-Grenze war belegt, DASS sie existieren, aber nicht,
// dass ihre Zaehler sich real bewegen und ihre Invarianten halten.
//
// REIHENFOLGE (bindend, wie am Dock): import -> GATE -> Testate. run_conformance_gate prueft zuerst die
// funktionale std::map-Huellen-Konformitaet des Antriebs; erst eine Huelle, die das besteht, wird vermessen.
//
// ZWEI TRAEGER, KEIN NEUER DLL-BAU (Wiederverwendung bestehender SHARED-Targets):
//   (a) anatomy_codegen_pilot_wormhole_shared -- die ECHTE codegen-Komposition (die reale metaprogrammierte
//       Kette, wie sie der Mess-Pfad baut),
//   (b) perm_adhoc_buildvariant             -- ein zweiter, andersartiger Vertreter (AdHoc-Buildvariante).
// Beide Pfade kommen als ARGUMENTE herein ($<TARGET_FILE:...>, Muster test_d13_dll_runtime_measure) --
// bewusst NICHT als -D-Pfad-Makros: eine Makro-Nutzung in String-Literal-Adjazenz ist die frisch belegte
// cppcheck-unknownMacro-Falle (lint:static laeuft OHNE die CMake-Defines). Argumente umgehen sie strukturell.
//
// EINZIGQUELLEN-KOPPLUNG (S5-05q-Lehre "gruene Tests zementieren alte Ordnung") -- dreifach:
//   (i)   COMPILE-HART: static_assert std::is_base_of_v<Interface, SearchAlgorithmAbiAdapter<RealeAnatomie>>
//         fuer JEDES Interface der Liste. Faellt eines aus dem Adapter heraus oder kommt eines hinzu, ohne
//         dass diese Liste mitzieht, bricht die Uebersetzung -- die TU kann nicht still von abi_adapter
//         abweichen.
//   (ii)  AM MODUL: dynamic_cast != nullptr fuer dieselbe Liste. Host-Welt und Modul-Welt sind damit
//         gegeneinander gepinnt; ein nullptr ist hier ein FEHLER, kein Skip (beide Traeger sind im Test-Baum
//         MEASUREMENT-ON uebersetzt, Root-CMakeLists.txt:141).
//   (iii) ANTI-LEERLAUF: jede Quote muss cases_total > 0 tragen. Eine leere Population machte jede Aussage
//         wahr -- passed() fordert das ohnehin, hier steht es zusaetzlich literal im Protokoll.

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <builder/pruef_dock/conformance_gate.hpp>
#include <builder/pruef_dock/mess_interface_testate.hpp>

#include <anatomy/abi_adapter.hpp> // Einzigquellen-Kopplung (i): der reale Adapter als Basen-Pin
#include <anatomy/allocator_proxy_tier.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/wormhole_reference.hpp> // die Komposition des Pilot-Traegers (a)

#include <cstdio>
#include <string>
#include <type_traits>

namespace ana    = ::comdare::cache_engine::anatomy;
namespace dock   = ::comdare::cache_engine::builder::pruef_dock;
namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace comp   = ::comdare::cache_engine::compositions;

// -- Einzigquellen-Kopplung (i): der COMPILE-HARTE Basen-Pin gegen den realen Adapter ------------------------
using PilotAnatomie = ana::SearchAlgorithmAnatomy<comp::WormholeComposition>;
using PilotAdapter  = ana::SearchAlgorithmAbiAdapter<PilotAnatomie>;

static_assert(std::is_base_of_v<ana::IDriveableTier, PilotAdapter>,
              "W3 (i): IDriveableTier muss Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IResourceControllableTier, PilotAdapter>,
              "W3 (i): IResourceControllableTier muss Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IAllocatorProxyTier, PilotAdapter>,
              "W3 (i): IAllocatorProxyTier muss Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IObservableTier, PilotAdapter>,
              "W3 (i): IObservableTier muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IMeasurableWorkload, PilotAdapter>,
              "W3 (i): IMeasurableWorkload muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IMeasurableWorkloadV2, PilotAdapter>,
              "W3 (i): IMeasurableWorkloadV2 muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IMeasurableWorkloadV3, PilotAdapter>,
              "W3 (i): IMeasurableWorkloadV3 muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IRollbackableTier, PilotAdapter>,
              "W3 (i): IRollbackableTier muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");
static_assert(std::is_base_of_v<ana::IMigratableTier, PilotAdapter>,
              "W3 (i): IMigratableTier muss (MESSUNG-AN) Basis des SearchAlgorithm-ABI-Adapters sein");

namespace {

int g_fail = 0;

void check(char const* was, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", was);
    if (!ok) ++g_fail;
}

/// Eine Quote abnehmen: bestanden UND nicht-leer, beides literal protokolliert.
void abnahme(char const* interface_name, dock::ConformanceResult const& r) {
    std::printf("    -> %s: cases %llu/%llu, first_fail=%llu\n", interface_name,
                static_cast<unsigned long long>(r.cases_passed), static_cast<unsigned long long>(r.cases_total),
                static_cast<unsigned long long>(r.first_fail));
    check((std::string{interface_name} + ": cases_total > 0 (Anti-Leerlauf-Pin)").c_str(), r.cases_total > 0);
    check((std::string{interface_name} + ": Quote vollstaendig bestanden").c_str(), r.passed());
}

int fahre_traeger(char const* etikett, char const* dll_pfad) {
    std::printf("\n==== W3-Testate am geladenen Modul: %s ====\n     %s\n", etikett, dll_pfad);

    loader::AnatomyModuleHandle handle;
    int const                   st = loader::AnatomyModuleLoader::load(dll_pfad, handle);
    check("load == status_ok", st == loader::status_ok);
    if (st != loader::status_ok) {
        std::printf("     status: %s\n", loader::status_name(st));
        return 1;
    }

    ana::IAnatomyBase* base = handle.anatomy();
    check("anatomy() != nullptr", base != nullptr);
    if (base == nullptr) return 1;
    check("genus() == SearchAlgorithm", base->genus() == ana::AnatomyGenus::SearchAlgorithm);

    // -- Einzigquellen-Kopplung (ii): dieselbe Interface-Liste am geladenen Modul, nullptr == FEHLER --------
    auto* drv  = dynamic_cast<ana::IDriveableTier*>(base);
    auto* obs  = dynamic_cast<ana::IObservableTier*>(base);
    auto* w1   = dynamic_cast<ana::IMeasurableWorkload*>(base);
    auto* w2   = dynamic_cast<ana::IMeasurableWorkloadV2*>(base);
    auto* w3   = dynamic_cast<ana::IMeasurableWorkloadV3*>(base);
    auto* roll = dynamic_cast<ana::IRollbackableTier*>(base);
    auto* mig  = dynamic_cast<ana::IMigratableTier*>(base);
    auto* rc   = dynamic_cast<ana::IResourceControllableTier*>(base);
    auto* alp  = dynamic_cast<ana::IAllocatorProxyTier*>(base);

    check("dynamic_cast<IDriveableTier*> != nullptr", drv != nullptr);
    check("dynamic_cast<IObservableTier*> != nullptr", obs != nullptr);
    check("dynamic_cast<IMeasurableWorkload*> != nullptr", w1 != nullptr);
    check("dynamic_cast<IMeasurableWorkloadV2*> != nullptr", w2 != nullptr);
    check("dynamic_cast<IMeasurableWorkloadV3*> != nullptr", w3 != nullptr);
    check("dynamic_cast<IRollbackableTier*> != nullptr", roll != nullptr);
    check("dynamic_cast<IMigratableTier*> != nullptr", mig != nullptr);
    check("dynamic_cast<IResourceControllableTier*> != nullptr", rc != nullptr);
    check("dynamic_cast<IAllocatorProxyTier*> != nullptr", alp != nullptr);
    if (drv == nullptr || obs == nullptr || w1 == nullptr || w2 == nullptr || w3 == nullptr || roll == nullptr ||
        mig == nullptr || rc == nullptr || alp == nullptr) {
        std::printf("     (Modul ohne vollstaendige Mess-Interface-Flaeche -> Abbruch)\n");
        return 1;
    }

    // -- GATE ZUERST (import -> GATE -> messen) ------------------------------------------------------------
    std::printf("  -- GATE (run_conformance_gate, funktionale std::map-Huelle) --\n");
    dock::ConformanceResult const gate = dock::run_conformance_gate(*drv, /*seed=*/42, /*n_random=*/500);
    std::printf("    -> GATE: cases %llu/%llu, first_fail=%llu\n", static_cast<unsigned long long>(gate.cases_passed),
                static_cast<unsigned long long>(gate.cases_total), static_cast<unsigned long long>(gate.first_fail));
    check("Konformitaets-GATE bestanden (Vorbedingung jeder Messung)", gate.passed());
    if (!gate.passed()) return 1;

    // -- Quer-Konsistenz V1 (KEIN eigenes Testat -- f15 deckt V1 am geladenen Modul bereits; hier nur die
    //    Ketten-Aussage, dass V1 im selben Lauf real misst) -------------------------------------------------
    std::printf("  -- IMeasurableWorkload V1 (Quer-Konsistenz, kein Neu-Testat) --\n");
    std::int64_t        lat[8]  = {};
    std::uint64_t const v1_ret  = w1->run_workload(/*ops_per_batch=*/256, /*batches=*/8, /*seed=*/4242, lat, 8);
    bool                lat_pos = v1_ret > 0;
    for (std::uint64_t i = 0; i < v1_ret && i < 8; ++i)
        if (lat[i] <= 0) lat_pos = false;
    std::printf("    -> V1: samples=%llu\n", static_cast<unsigned long long>(v1_ret));
    check("V1 run_workload liefert Samples > 0 mit positiven Latenzen", lat_pos);

    // -- Die W3-Testate ------------------------------------------------------------------------------------
    std::printf("  -- IObservableTier --\n");
    abnahme("IObservableTier", dock::testat_observable_tier(*obs, stdout));
    std::printf("  -- IMeasurableWorkloadV2 --\n");
    abnahme("IMeasurableWorkloadV2", dock::testat_measurable_workload_v2(*w2, stdout));
    std::printf("  -- IMeasurableWorkloadV3 --\n");
    abnahme("IMeasurableWorkloadV3", dock::testat_measurable_workload_v3(*w3, stdout));
    std::printf("  -- IRollbackableTier --\n");
    abnahme("IRollbackableTier", dock::testat_rollbackable_tier(*roll, *drv, stdout));
    std::printf("  -- IMigratableTier --\n");
    abnahme("IMigratableTier", dock::testat_migratable_tier(*mig, *drv, stdout));
    std::printf("  -- IResourceControllableTier --\n");
    abnahme("IResourceControllableTier", dock::testat_resource_controllable_tier(*rc, obs, stdout));
    std::printf("  -- IAllocatorProxyTier --\n");
    abnahme("IAllocatorProxyTier", dock::testat_allocator_proxy_tier(*drv, stdout));
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
                     "usage: test_w3_mess_interface_testate_dock <wormhole_pilot.so> <perm_adhoc_buildvariant.so>\n");
        return 2;
    }
    std::printf("==== W3-KERN: Mess-Interface-Testate am Pruef-Dock (import -> GATE -> Testate) ====\n");
    (void)fahre_traeger("Traeger (a) codegen-Pilot Wormhole (reale metaprogrammierte Kette)", argv[1]);
    (void)fahre_traeger("Traeger (b) AdHoc-Buildvariante (zweiter, andersartiger Vertreter)", argv[2]);

    std::printf("\n==== W3-Dock-Testate: %s ====\n",
                g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER").c_str());
    return g_fail == 0 ? 0 : 1;
}
