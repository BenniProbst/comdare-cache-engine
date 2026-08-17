#pragma once
// V41.F.6.1.R7.1.c axis_02_path_compression Subaxes-Tags

namespace comdare::cache_engine::path_compression::subaxes {

// PC1: Kompressions-Granularitaet (none / bit / byte / multi-byte)
struct granularity_tag {};

// PC2: Sprung-Strategie (linear vs split vs hash)
struct skip_strategy_tag {};

// PC3: Decode-Komplexitaet (O(1) vs O(log n) vs O(n))
struct decode_complexity_tag {};

} // namespace comdare::cache_engine::path_compression::subaxes
