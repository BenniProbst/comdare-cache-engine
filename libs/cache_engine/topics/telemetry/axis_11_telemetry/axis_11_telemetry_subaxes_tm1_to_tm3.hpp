#pragma once
// V41.F.6.1.R7.5.b axis_11_telemetry Subaxes-Tags

namespace comdare::cache_engine::telemetry::axis_11_telemetry::subaxes {

// TM1: Scope (leaf-only / per-node / global)
struct scope_tag {};

// TM2: Metric-Type (counter / histogram / density)
struct metric_type_tag {};

// TM3: Collection-Overhead-Level (low / medium / high)
struct overhead_level_tag {};

}  // namespace
