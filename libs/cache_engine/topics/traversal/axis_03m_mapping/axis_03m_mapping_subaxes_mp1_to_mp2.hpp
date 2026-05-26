#pragma once
// V41.F.6.1 axis_03m_mapping Subaxis-Tags MP1-MP2 (2026-05-26)

namespace comdare::cache_engine::traversal::axis_03m_mapping::subaxes {

/// MP1 direct — direkter Slot-zu-Offset-Mapping (linear packing)
struct direct_access_tag {};

/// MP2 pool_relative — pool-relativer Offset-Mapping (CustomAlignedStructure-Pattern)
struct pool_relative_access_tag {};

}  // namespace
