#pragma once
// V41.F.6.1 axis_03b_cache_traversal Subaxis-Tags CT1-CT2 (2026-05-26)
//
// @topic traversal @achse 03b

namespace comdare::cache_engine::cache_traversal::subaxes {

/// CT1 linear — linear-scan Fanout-Traversal (klein, cache-freundlich)
struct linear_access_tag {};

/// CT2 hash — Hash-basierte Lookup-Traversal (O(1) amortisiert)
struct hash_access_tag {};

} // namespace comdare::cache_engine::cache_traversal::subaxes
