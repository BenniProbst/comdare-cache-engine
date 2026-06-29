#pragma once
// i_cache_engine.hpp - REV 5.2 Top-Level ICacheEngine
// Quelle: U09 (UML) - Visitor + Mediator + State-Holder + Pipeline

#include "cache_engine/concepts/cache_recommendation.hpp"
#include "cache_engine/concepts/i_sub_engine.hpp"
#include "cache_engine/concepts/platform_snapshot.hpp"
#include "cache_engine/concepts/request_context.hpp"

#include <memory>

namespace comdare::cache_engine {

// -----------------------------------------------------------------------------
// ICacheEngine - Wurzel-Interface fuer ISearchPageStrategy-Caller
// -----------------------------------------------------------------------------
class ICacheEngine {
public:
    virtual ~ICacheEngine() = default;

    /// Visitor-Wurzel: Caller (ISearchPageStrategy) ruft mit RequestContext,
    /// erhaelt konsolidierte CacheRecommendation aus 12-Sub-Engine-Pipeline.
    virtual CacheRecommendation advise(const RequestContext& ctx) = 0;

    /// Frischer Plattform-Snapshot - kann vom Caller weiterverarbeitet werden.
    virtual PlatformSnapshot snapshot() const = 0;

    /// Mediator-Methode: Sub-Engine signalisiert ein Event an die Engine.
    virtual void notify(const SubEngineEvent& event) = 0;

    /// Sub-Engine registrieren (typischerweise in CacheEngineBuilder)
    virtual void register_sub_engine(SubEngineSlot slot, std::unique_ptr<ISubEngine> engine) = 0;

    /// Plattform-Pressure-Transition Hook (Telemetry/Debug)
    virtual void on_pressure_transition(const state::PressureState& from, const state::PressureState& to) = 0;
};

} // namespace comdare::cache_engine
