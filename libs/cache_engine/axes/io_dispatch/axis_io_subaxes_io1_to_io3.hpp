#pragma once
// V41.F.6.1.R7.5.f axis_io Subaxes-Tags

namespace comdare::cache_engine::io_dispatch::subaxes {

// IO1: Persistence (in-memory / disk / persistent-memory)
struct persistence_tag {};

// IO2: Caching-Strategy (none / os-cache / direct / mmap)
struct caching_strategy_tag {};

// IO3: Write-Durability (none / fsync / atomic)
struct write_durability_tag {};

} // namespace comdare::cache_engine::io_dispatch::subaxes
