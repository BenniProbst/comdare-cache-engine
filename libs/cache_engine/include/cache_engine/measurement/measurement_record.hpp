#pragma once
// MeasurementRecord — 32-Byte gepacktes Binary-Format
// Termin 7 / 06_uml_persistence §3

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::measurement {

#pragma pack(push, 1)
struct alignas(32) MeasurementRecord {
    std::uint8_t  category;    // MeasurementCategory enum-as-uint
    std::uint16_t algo_detail; // AlgoDetail enum-as-uint
    std::uint16_t thread_id;
    std::uint8_t  cpu_class;          // 0=core, 1=atom (Block AO Hybrid-CPU)
    std::uint8_t  telemetry_strategy; // Achse 11 Kuehn (siehe TelemetryStrategyKind)
    std::uint32_t sampling_n;         // N fuer LeafOnlySampledCounter<N>; 0 wenn unzutreffend
    std::uint8_t  reserved[3];
    std::uint64_t timestamp_ns;
    double        metric_value;
};
#pragma pack(pop)

static_assert(sizeof(MeasurementRecord) == 32, "MeasurementRecord muss exakt 32 Byte sein (Disk-Dump-Format)");
static_assert(alignof(MeasurementRecord) == 32, "MeasurementRecord muss 32-Byte aligned sein");
static_assert(std::is_trivially_copyable_v<MeasurementRecord>,
              "MeasurementRecord muss trivially-copyable sein (Binary-Dump)");

} // namespace comdare::cache_engine::measurement
