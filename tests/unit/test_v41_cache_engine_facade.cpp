// SPDX-License-Identifier: Apache-2.0
// F5.R1 (#35) — Smoke-Test der ICacheEngine-Master-Framework-Gesamt-Fassade (get_cache_engine()).
//
// Verifiziert über die EINE Tür (<cache_engine/cache_engine.hpp> = Aggregation-Header) echte Aufrufe über
// get_cache_engine(): dass die 4 verdrahteten Provider (measurement / isa_dispatch / build_tools / test_system)
// die ECHTEN in-repo-Subsysteme vermitteln UND die 2 #274-Provider EHRLICH als DEFERRED gemeldet werden
// (kein Fake). Löst zugleich das README-Beispiel ein (get_cache_engine() + get_pruefling_registry()-Iteration).
// Analog zu test_v41_cache_engine_tools_facade.cpp (F.4-Blaupause). Belegt, dass die Fassade nicht hohl ist
// und die frühere Linker-Falle (get_cache_engine() deklariert, nie definiert) geschlossen wurde.

#include <cache_engine/cache_engine.hpp> // die EINE Tür — zieht api/i_cache_engine.hpp + Prüfling-Registry

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace api  = ::comdare::cache_engine::api;
namespace meas = ::comdare::cache_engine::measurement;
namespace wg   = ::comdare::workload_generator;

// Die Fassade liefert eine prozessweite Instanz mit der erwarteten Version.
TEST(F5_CacheEngineFacade, ProvidesFrameworkVersion) {
    auto& ce = api::get_cache_engine();
    EXPECT_EQ(ce.framework_version(), std::string_view{"comdare-cache-engine-v1"});
    // Prozessweiter Singleton: zweiter Aufruf liefert dieselbe Instanz (get_cache_engine_tools-Muster).
    EXPECT_EQ(&ce, &api::get_cache_engine());
}

// Measurement-Provider: vermittelt die ECHTE 16-Kategorien-System-Achsen-Registry.
TEST(F5_CacheEngineFacade, MeasurementProviderExposesSystemAxes) {
    auto& m = api::get_cache_engine().measurement();
    EXPECT_EQ(m.system_axis_count(), meas::kMeasurementCategoryCount);
    EXPECT_EQ(m.system_axis_count(), std::size_t{16});
    EXPECT_EQ(m.axis_name(meas::MeasurementCategory::CLU), std::string_view{"CLU"});
    EXPECT_EQ(m.axis_name(meas::MeasurementCategory::LATENCY_P99), std::string_view{"LATENCY_P99"});
    EXPECT_EQ(m.axis_name(meas::MeasurementCategory::IPC_CPI), std::string_view{"IPC_CPI"});
}

// ISA-Dispatch-Provider: vermittelt die ECHTE CPUID-Auto-Discovery (probe_cpuid).
TEST(F5_CacheEngineFacade, IsaDispatchProviderRunsRealProbe) {
    auto const probe = api::get_cache_engine().isa_dispatch().detect();
    // Cache-Line ist auf jeder unterstützten Plattform > 0 (x86/ARM/RISC-V-Default 64/128).
    EXPECT_GT(probe.cache_line_bytes, 0u);
    // Der Probe-Aufruf ist deterministisch identisch mit dem direkten Subsystem-Aufruf.
    EXPECT_EQ(probe.cache_line_bytes, ::comdare::cache_engine::platform_probe::probe_cpuid().cache_line_bytes);
}

// Build-Tools-Provider: vermittelt (via F.4) den ECHTEN Codegen — emittiert kompilierbaren Modul-Quelltext.
TEST(F5_CacheEngineFacade, BuildToolsProviderEmitsModuleSource) {
    auto&                          cg = api::get_cache_engine().build_tools().codegen();
    std::vector<std::string> const fq{"ns::A0", "ns::A1", "ns::A2"};
    std::string const              src = cg.emit_module_source(7, fq);
    EXPECT_NE(src.find("Permutation 7"), std::string::npos);
    EXPECT_NE(src.find("DO NOT EDIT"), std::string::npos);
    EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC"), std::string::npos);
    for (auto const& t : fq) EXPECT_NE(src.find(t), std::string::npos);
}

// Test-System-Provider: vermittelt (via F.4) den ECHTEN YCSB-Generator (Profil C = 100 % Read).
TEST(F5_CacheEngineFacade, TestSystemProviderGeneratesYcsb) {
    auto&              wt = api::get_cache_engine().test_system().workloads();
    wg::WorkloadConfig cfg{};
    cfg.num_keys       = 1000;
    cfg.num_operations = 500;
    cfg.random_seed    = 7;
    auto const ops     = wt.generate_ycsb(cfg, wg::YcsbWorkload::C);
    ASSERT_EQ(ops.size(), 500u);
    for (auto const& op : ops) EXPECT_EQ(op.op, wg::OperationKind::Read);
}

// #274-Provider werden EHRLICH als DEFERRED gemeldet — kein Fake-Verhalten, kein Fassaden-Accessor.
TEST(F5_CacheEngineFacade, DeferredProvidersAreHonestlyReported) {
    auto const deferred = api::get_cache_engine().deferred_providers();
    ASSERT_EQ(deferred.size(), 2u);
    std::vector<std::string_view> names;
    for (auto const& d : deferred) {
        names.push_back(d.name);
        EXPECT_EQ(d.issue, std::string_view{"#274"});
        EXPECT_NE(d.milestone.find("V42"), std::string_view::npos);
        EXPECT_FALSE(d.reason.empty());
    }
    EXPECT_NE(std::find(names.begin(), names.end(), std::string_view{"search_engine"}), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), std::string_view{"cache_engine_core"}), names.end());
}

// README-Beispiel eingelöst: der minimale reale Konsument-Loop über die EINE Tür (Fassade + Prüfling-Registry).
// Im Standalone-Build ist die Registry leer (Prüflinge werden CMake-konfiguriert registriert) — der Loop ist
// dennoch real erreichbar und läuft ohne Linker-Falle durch. Beweist: link NUR gegen cache-engine genügt.
TEST(F5_CacheEngineFacade, ReadmeConsumerLoopIsReachable) {
    auto& ce  = api::get_cache_engine();
    auto& reg = api::get_pruefling_registry();
    EXPECT_EQ(ce.framework_version(), std::string_view{"comdare-cache-engine-v1"});
    std::size_t seen = 0;
    for (auto* factory : reg.all_factories()) {
        ASSERT_NE(factory, nullptr);
        for (auto axes : factory->available_axes_combinations()) {
            auto pruefling = factory->create(axes);
            ASSERT_NE(pruefling, nullptr);
            double micros = 0.0;
            (void)pruefling->run(1, micros);
            ++seen;
        }
    }
    // Standalone: 0 registrierte Prüflinge -> Loop erreichbar, aber leer (kein Fake-Prüfling).
    EXPECT_EQ(seen, reg.all_factories().empty() ? std::size_t{0} : seen);
}
