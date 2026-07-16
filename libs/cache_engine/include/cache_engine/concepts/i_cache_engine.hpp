#pragma once
// i_cache_engine.hpp - REV 5.2 Top-Level ICacheEngine
// Quelle: U09 (UML) - Visitor + Mediator + State-Holder + Pipeline
//
// R3-ABGRENZUNG (Doppelbelegung "ICacheEngine", F5.R3 2026-07-16): DIESER Typ,
// comdare::cache_engine::ICacheEngine, ist die U09-PIPELINE-Wurzel (Visitor + Mediator +
// State-Holder über die 12 ISubEngine-Stufen). Er ist NICHT zu verwechseln mit
// comdare::cache_engine::api::ICacheEngine (api/i_cache_engine.hpp) = der MODUL-VERMITTLUNGS-
// Fassade (Facade-Pattern über die Subsysteme). Beide liegen in DISTINKTEN Namespaces (cache_engine::
// vs. api::) → keine Sprach-Kollision; die Rollen-Doku steht autoritativ in beiden Headern
// (Umbenennung bewusst vermieden — kleinere, ripple-freie Variante, s. api/i_cache_engine.hpp).

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
