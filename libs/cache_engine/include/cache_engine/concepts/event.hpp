#pragma once
// Event-Hierarchie — F2 Push synchron mit Lambda-Tree-Filter
// Termin 7 / 02_uml_cache_engine §5 — 9 konkrete Event-Klassen

#include <chrono>
#include <cstdint>
#include <thread>

namespace comdare::cache_engine {

using ModuleId = uint64_t;
using NodeId   = uint64_t;
using LeafId   = uint64_t;
using PageId   = uint64_t;
using CoreId   = uint32_t;

enum class EventKind : uint8_t {
    PageRelocation,
    PageTypeChange,
    PrefetchAdjustment,
    HotPathRecognition,
    TelemetryUpdate,
    Write,
    ConsolidationBarrier,
    Sampling,
    Error,
};

struct Event {
    std::chrono::steady_clock::time_point timestamp{};
    ModuleId                              module_id = 0;
    std::thread::id                       thread_id{};
    EventKind                             kind = EventKind::Error;
};

struct PageRelocationEvent : Event {
    PageId  source_page        = 0;
    uint8_t target_layout_hint = 0;
    double  load_factor        = 0.0;
};

struct PageTypeChangeEvent : Event {
    PageId   page           = 0;
    uint16_t current_type   = 0;
    uint16_t suggested_type = 0;
    uint8_t  reason         = 0; // ResizeReason enum-as-uint
};

struct PrefetchAdjustmentEvent : Event {
    uint8_t current_distance   = 0;
    uint8_t suggested_distance = 0;
    double  measured_miss_rate = 0.0;
};

struct HotPathRecognitionEvent : Event {
    NodeId   hot_node     = 0;
    uint32_t access_count = 0;
    double   hot_score    = 0.0;
};

struct TelemetryUpdateEvent : Event {
    NodeId   node               = 0;
    uint8_t  telemetry_strategy = 0; // siehe flags::telemetry_bank
    uint64_t counter_delta      = 0;
};

struct WriteEvent : Event {
    NodeId   target                = 0;
    uint16_t node_depth            = 0;
    uint16_t num_cores_sharing     = 0;
    uint16_t cache_line_size_bytes = 64;
};

struct ConsolidationBarrierEvent : Event {
    uint64_t epoch = 0;
};

struct SamplingEvent : Event {
    uint32_t current_sampling_n = 0;
    double   cpu_load           = 0.0;
    double   cache_miss_rate    = 0.0;
};

struct ErrorEvent : Event {
    uint16_t error_code  = 0;
    uint64_t aux_payload = 0;
};

} // namespace comdare::cache_engine
