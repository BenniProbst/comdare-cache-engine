// SPDX-License-Identifier: Apache-2.0
// V41.E11 Skeleton (2026-05-25) — Facade-Interface fuer cache-engine als
// Master-Framework. Vermittelt die 6 ausgelagerten Submodule (search-engine,
// cache-engine-core, measurement, isa-dispatch, build-tools, test-system)
// an Konsumenten (Diplomarbeit) ohne dass diese die Module direkt linken.
//
// Status: API-Skelett. Implementation in src/facade/cache_engine_facade.cpp
// (Phase 7+). Vorerst nur Pure-Interfaces + Dokumentation der erwarteten
// Vermittlungs-Operationen.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::api {

// Forward Declarations (echte Klassen kommen aus den Submodulen)
class IMeasurementProvider;
class IIsaDispatchProvider;
class ISearchEngineProvider;
class ICacheEngineCoreProvider;
class IBuildToolsProvider;
class ITestSystemProvider;

// Master-Framework-Facade.
// Konsumenten linken NUR gegen cache-engine + benutzen ICacheEngine — die
// Submodule-Implementierungen werden intern vermittelt.
class ICacheEngine {
public:
    virtual ~ICacheEngine() = default;

    // Vermittlungs-Accessor je Submodul. Owner bleibt das Modul selbst.
    [[nodiscard]] virtual IMeasurementProvider&    measurement()    = 0;
    [[nodiscard]] virtual IIsaDispatchProvider&    isa_dispatch()   = 0;
    [[nodiscard]] virtual ISearchEngineProvider&   search_engine()  = 0;
    [[nodiscard]] virtual ICacheEngineCoreProvider& core()          = 0;
    [[nodiscard]] virtual IBuildToolsProvider&     build_tools()    = 0;
    [[nodiscard]] virtual ITestSystemProvider&     test_system()    = 0;

    [[nodiscard]] virtual std::string_view framework_version() const = 0;
};

// Factory-Function — eine Implementation pro Build (Singleton-artig).
// Implementation in src/facade/cache_engine_facade.cpp (Phase 7+).
[[nodiscard]] ICacheEngine& get_cache_engine();

}  // namespace comdare::cache_engine::api
