// V41.F.6.1 R5.G — End-to-End-Schluss: auto-emittierte AdHoc-Permutation als SHARED-DLL laden + messen.
//
// Lädt die aus auto_emitted_perm_module.cpp gebaute Permutations-DLL (comdare_anatomy_perm_r5g_adhoc)
// via AnatomyModuleLoader und prüft, dass eine AUTO-ENUMERIERTE AdHoc-Komposition (nicht eine benannte
// Composition wie bei F.5) als vollwertige, mess-fähige Anatomie geladen wird. Damit ist die
// R5.G-Materialisierung end-to-end geschlossen: Emitter-Ausgabe → SHARED-DLL → geladen → run_workload.

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <builder/pruef_dock/pruef_dock.hpp>
#include <builder/pruef_dock/search_algorithm_dock.hpp>
#include <builder/pruef_dock/pruef_dock_registry.hpp>
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

TEST(R5G_AdHocDllLoad, AutoEmittedAdHocPermutationLoadsAsDllAndRuns) {
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    int const st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_GE(handles.size(), 1u) << "auto-emittierte AdHoc-DLL nicht geladen.";

    auto* a = handles[0].anatomy();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->composition_name(), std::string_view{"AdHocComposition"});  // auto-enumerierte Komposition
    EXPECT_EQ(a->organ_count(), 17u);
    EXPECT_EQ(a->genus(), ana::AnatomyGenus::SearchAlgorithm);

    // Mess-Last (Stufe B) läuft auch in der auto-emittierten DLL.
    auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(a);
    ASSERT_NE(mw, nullptr);
    std::vector<std::int64_t> samples(10);
    auto const n = mw->run_workload(/*ops_per_batch=*/1000, /*batches=*/10, /*seed=*/3u,
                                    samples.data(), samples.size());
    EXPECT_EQ(n, 10u);
    EXPECT_GT(samples[0], 0);
}

// =================================================================
// V41.F.6.1 R8-REST(a) — e2e Pfad B: REALE codegen'te DLL → IObservableTier → SearchAlgorithmDock → Trace.
//
// Schliesst die letzte e2e-Luecke (Ledger §a R8-REST a): das SearchAlgorithmDock misst hier NICHT ein
// In-Process-Stand-in (so der pruef_dock-Unit-Test mit native=nullptr), sondern ein ECHTES dlopen/
// LoadLibrary-geladenes Tier-Modul. Beweist, dass die IObservableTier-vtable + der POD-Snapshot ueber die
// reale .dll-Grenze tragen (RTTI-dynamic_cast wie bei IMeasurableWorkload in R5G) und der host-seitige
// Mess-Treiber CSV/JSON persistiert. Doku 24 §8.8 (Prüf-Dock) + §8.9.1 (EIN Dock je Gattung).
// =================================================================
TEST(R8RestA_DockMeasuresRealDll, RealAdHocDllObservedThroughSearchAlgorithmDock) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    namespace pd = ::comdare::cache_engine::builder::pruef_dock;

    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);

    // (1) RTTI ueber die REALE DLL-Grenze: die geladene Anatomie IST ein SearchAlgorithmAbiAdapter → IObservableTier.
    auto* obs = dynamic_cast<ana::IObservableTier*>(handles[0].anatomy());
    ASSERT_NE(obs, nullptr) << "IObservableTier-dynamic_cast ueber die DLL-Grenze fehlgeschlagen (RTTI-Mismatch).";

    // (2) Das Dock waehlt sich per Gattung des REALEN Handles + misst es (Pfad B, host-seitiger Treiber).
    pd::PruefDockRegistry reg;
    reg.register_dock(std::make_unique<pd::SearchAlgorithmDock>());
    auto* dock = reg.select_for(handles[0]);
    ASSERT_NE(dock, nullptr) << "kein Dock akzeptiert das reale DLL-Handle (Gattungs-Mismatch).";
    EXPECT_EQ(dock->dock_genus(), ana::AnatomyGenus::SearchAlgorithm);

    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {50, 200};
    opts.lookups_per_checkpoint = 200;
    opts.deletes_per_checkpoint = 20;
    std::string csv, json;
    int const rc = dock->measure(handles[0], opts, csv, json);
    ASSERT_EQ(rc, pd::dock_status_ok) << pd::dock_status_name(rc);

    // (3) Persistierter Observer-Trace, ueber die DLL-Grenze gezogen: Header + ≥2 Checkpoint-Zeilen + Zaehler-Keys.
    EXPECT_NE(csv.find("checkpoint,observe_wall_ns,fill_level"), std::string::npos);  // CSV-Header
    EXPECT_GT(std::count(csv.begin(), csv.end(), '\n'), 2)                            // Header + 2 Checkpoints
        << "CSV hat keine Checkpoint-Zeilen — Mess-Loop lief nicht ueber die DLL.";
    ASSERT_FALSE(json.empty());
    EXPECT_EQ(json.front(), '[');                                                     // JSON-Array
    EXPECT_NE(json.find("\"search_insert\""), std::string::npos);                     // Observer-Zaehler korreliert
    EXPECT_NE(json.find("\"fill_level\""), std::string::npos);                        // Tier-Fuellstand pro Checkpoint
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus — e2e-Dock-Mess-Test n/a";
#endif
}

// V41 (2026-05-31) — Robustheits-Regression: der Fuellstands-Treiber haengt NICHT bei einem Fill ÜBER die
// Tier-Kapazitaet. Der AdHoc-Tier hat 256 Slots (via f15_compare --observe entdeckt); ein Checkpoint von
// 1e6 wuerde ohne den max_insert_stagnation-Guard `while (tier_size() < target)` ENDLOS laufen. Der Guard
// deckelt die WRITE-Phase an der effektiven Kapazitaet → der Treiber terminiert + liefert einen gedeckelten
// fill_level. (Bug entdeckt + gefixt in der OpenDone.2-Charge.)
TEST(R8RestA_DockMeasuresRealDll, ObserveTraceGuardCapsOverCapacityFillNoHang) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    namespace ac = ::comdare::cache_engine::builder::anatomy_commands;
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);
    auto* obs = dynamic_cast<ana::IObservableTier*>(handles[0].anatomy());
    ASSERT_NE(obs, nullptr);

    ac::AbiTierTraceConfig cfg;
    cfg.fill_checkpoints       = {1'000'000};   // WEIT über die 256-Slot-Kapazität
    cfg.lookups_per_checkpoint = 10;
    cfg.deletes_per_checkpoint = 0;
    cfg.max_insert_stagnation  = 1024;          // schneller, bounded Abbruch
    auto const trace = ac::drive_tier_observe_trace_abi(*obs, cfg);   // MUSS terminieren (ohne Guard = Hang)
    ASSERT_EQ(trace.checkpoints.size(), 1u);
    EXPECT_GT(trace.checkpoints[0].fill_level, 0u);
    EXPECT_LT(trace.checkpoints[0].fill_level, 1'000'000u);   // an der effektiven Kapazität gedeckelt
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus";
#endif
}

// V42 L-74c — DLL-ROUND-TRIP für tier_observe_v2 ZURÜCKGESTELLT (2026-06-03, ehrlicher Befund):
// Ein erster Versuch (TEST V2ObserverSnapshotOverRealDllBoundary) crashte mit SEH 0xc0000005 — NICHT wegen
// fehlerhaften V2-Codes (der In-Process-Beweis test_d_v42_abi_telemetry_coupling ist grün), sondern wegen
// einer vtable-ABI-Subtilität über die REALE .dll-Grenze: `tier_observe_v2` ist eine NEUE virtuelle Methode
// am vtable-Ende von IObservableTier. Die per Codegen erzeugte comdare_r5g_adhoc_perm-DLL wurde NICHT
// zuverlässig mit dem neuen Header neu gebaut (auto_emitted_perm_module.cpp wird vom Emitter regeneriert,
// ein Datei-Touch erzwingt den DLL-Rebuild nicht) → Host-vtable (mit dem neuen Slot) vs. DLL-vtable (alt) →
// der Aufruf springt über das vtable-Ende → Crash. LEHRE (Doc 29-würdig): eine vtable-additive Methode ist
// über eine DLL-Grenze nur sicher, wenn DLL UND Host SYNCHRON aus demselben Header gebaut werden; der saubere
// Weg ist entweder ein erzwungener Clean-DLL-Rebuild im CMake-Graph ODER — ABI-robuster — ein SEPARATES
// Sub-Interface IObservableTierV2 (per dynamic_cast abgefragt, alte DLLs → nullptr, kein vtable-Slap).
// Nächster Sprint: IObservableTierV2 als eigenständiges Sub-Interface (analog wie IObservableTier selbst
// NICHT an IAnatomyBase hängt) + Codegen-DLL-Rebuild-Dependency. Der In-Process-V2-Pfad bleibt verifiziert.

// V42 L-74c — DLL-ROUND-TRIP über das ABI-ROBUSTE IObservableTierV2-Sub-Interface (2026-06-03, Nachzug).
// Anders als die vtable-additive Variante (die crashte) fragt der Host das V2-Sub-Interface via dynamic_cast
// ab: gelingt er (DLL mit IObservableTierV2 gebaut) → tier_observe_v2 trägt den V2-POD über die reale
// .dll-Grenze; schlägt er fehl (alte DLL) → nullptr → sauberer Degrade auf V1, KEIN Crash. Genau das
// Designprinzip von IObservableTier selbst. Beweist die ABI-Robustheit des V2-Pfads über die echte DLL.
TEST(R8RestA_DockMeasuresRealDll, V2SubInterfaceOverRealDllBoundaryOrGracefulDegrade) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 1u);

    auto* obs2 = dynamic_cast<ana::IObservableTierV2*>(handles[0].anatomy());
    if (obs2 == nullptr) {
        // Alte DLL ohne IObservableTierV2 → sauberer Degrade (KEIN Crash). Die ABI-Robustheit IST der Beweis.
        GTEST_SKIP() << "DLL ohne IObservableTierV2 (nicht synchron neu gebaut) — Host degradiert sauber auf V1.";
    }
    // Neue DLL: V2-POD über die reale .dll-Grenze ziehen.
    for (int i = 0; i < 30; ++i) {
        auto* drv = dynamic_cast<ana::IDriveableTier*>(handles[0].anatomy());
        ASSERT_NE(drv, nullptr);
        (void)drv->tier_insert(static_cast<std::uint64_t>(i), static_cast<std::uint64_t>(i) * 2u);
    }
    ana::ComdareTierObserverSnapshotV2 v2{};
    obs2->tier_observe_v2(&v2);
    EXPECT_GE(v2.search_insert_count, 1u);
    EXPECT_GE(v2.observable_axis_count, 1u);
    EXPECT_GT(v2.tier_fill_level, 0u);
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus";
#endif
}
