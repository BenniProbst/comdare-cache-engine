#pragma once
// V41.F.6.1.R7.5.d axis_14_value_handle Subaxes-Tags

namespace comdare::cache_engine::value_handle_axis::subaxes {

// VH1: Storage-Location (inline / external)
struct storage_location_tag {};

// VH2: Ownership (owned / shared / weak)
struct ownership_tag {};

// VH3: Versioning (none / version-tagged)
struct versioning_tag {};

} // namespace comdare::cache_engine::value_handle_axis::subaxes
