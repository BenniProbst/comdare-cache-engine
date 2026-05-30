#pragma once
// V41.F.6.1.R7.5.c axis_10_serialization Subaxes-Tags

namespace comdare::cache_engine::serialization_axis::subaxes {

// SR1: Byte-Order (raw / varlen / packed)
struct byte_order_tag {};

// SR2: Density (sparse / dense / succinct)
struct density_tag {};

// SR3: Compression (none / lossless)
struct compression_tag {};

}  // namespace
