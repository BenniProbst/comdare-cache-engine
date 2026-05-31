#pragma once
// V32.EE.1 (2026-05-18 spaet) - Workload-Definition fuer ExecuteEngineCommand
//
// @subsystem CEB
// @phase_owner CEB

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief WorkloadKind - Typ der Workload-Operation (YCSB-Style)
 * @subsystem CEB
 */
enum class WorkloadKind : std::uint8_t {
    YCSB_A_Read50Write50 = 0,
    YCSB_B_Read95Write5  = 1,
    YCSB_C_ReadOnly      = 2,
    YCSB_D_ReadLatest    = 3,
    YCSB_E_Scan95        = 4,
    YCSB_F_ReadModifyWrite = 5,
    Custom_HotKey        = 100,
    Custom_RangeDelete   = 101,
    Custom_BulkInsert    = 102   // V41.P5 (G11): OP-2 Bulk-Insert (datasets OP_1_to_6_SPECIFICATIONS)
};

/**
 * @brief Workload - Eingabe fuer ExecuteEngineCommand
 * @subsystem CEB
 */
struct Workload {
    WorkloadKind kind {WorkloadKind::YCSB_C_ReadOnly};
    std::size_t record_count {1'000'000};
    std::size_t operation_count {100'000};
    std::uint64_t seed {42};  ///< Determinismus
    std::string_view name {"unnamed-workload"};
};

}  // namespace comdare::cache_engine::builder::commands
