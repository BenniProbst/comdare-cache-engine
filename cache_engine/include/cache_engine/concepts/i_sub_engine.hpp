#pragma once
// i_sub_engine.hpp - REV 5.2 ISubEngine + 12 Sub-Engine-Slots C1-C12
// Quelle: U09 + K15

#include "cache_engine/concepts/cache_recommendation.hpp"
#include "cache_engine/concepts/platform_snapshot.hpp"
#include "cache_engine/concepts/request_context.hpp"

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine {

/// SubEngineSlot - die 12 Sub-Engine-Familien aus K15
enum class SubEngineSlot : std::uint8_t {
    C01_Layout      = 1,
    C02_Pinning     = 2,
    C03_Prefetch    = 3,
    C04_Coherence   = 4,
    C05_Telemetry   = 5,
    C06_Allocation  = 6,
    C07_Migration   = 7,
    C08_Encoding    = 8,
    C09_Heuristik   = 9,
    C10_Topologie   = 10,
    C11_Scheduler   = 11,
    C12_Filter      = 12
};

/// Deterministische Pipeline-Reihenfolge (U09 §3.4):
/// Information first -> Structure second -> Allocation third -> Behavior last
inline constexpr SubEngineSlot kPipelineOrder[12] = {
    SubEngineSlot::C05_Telemetry,
    SubEngineSlot::C10_Topologie,
    SubEngineSlot::C09_Heuristik,
    SubEngineSlot::C01_Layout,
    SubEngineSlot::C08_Encoding,
    SubEngineSlot::C06_Allocation,
    SubEngineSlot::C02_Pinning,
    SubEngineSlot::C07_Migration,
    SubEngineSlot::C03_Prefetch,
    SubEngineSlot::C04_Coherence,
    SubEngineSlot::C12_Filter,
    SubEngineSlot::C11_Scheduler
};

/// SubEngineEvent - Mediator-Callback fuer Cross-Sub-Engine-Kommunikation
struct SubEngineEvent {
    SubEngineSlot source;
    SubEngineSlot target;
    std::uint32_t event_kind = 0;
    const void* payload = nullptr;
};

// -----------------------------------------------------------------------------
// ISubEngine - eine Pipeline-Stufe (1 von 12)
// -----------------------------------------------------------------------------
class ISubEngine {
public:
    virtual ~ISubEngine() = default;

    /// Lesbarer Name fuer Telemetry/Logging
    virtual std::string_view name() const noexcept = 0;

    /// Welche Sub-Engine-Familie (C01-C12)
    virtual SubEngineSlot family_kind() const noexcept = 0;

    /// Haupt-Methode: lese running-Empfehlung + Snapshot + Context, liefere Delta
    virtual CacheRecommendation advise(
        const PlatformSnapshot& snap,
        const RequestContext& ctx,
        const CacheRecommendation& running) = 0;

    /// Mediator-Callback (z.B. Layout-Engine signalisiert "neue Geometrie")
    virtual void on_event(const SubEngineEvent& event) = 0;
};

}  // namespace comdare::cache_engine
