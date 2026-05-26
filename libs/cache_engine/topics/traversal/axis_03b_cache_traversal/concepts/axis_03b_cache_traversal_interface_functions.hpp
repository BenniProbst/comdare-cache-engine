#pragma once
// V41.F.6.1.P2.F axis_03b_cache_traversal Interface-Functions-Liste (2026-05-26)
//
// @topic traversal @achse 03b cache_traversal

#include <array>
#include <string_view>

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal::concepts {

/// Pflicht-Interface-Functions der 03b Cache-Traversal-Achse (CacheTraversalVariant).
/// Properties (tracked_count) sind keine Paper-Bodies und nicht hier.
inline constexpr std::array<std::string_view, 4> kAxisInterfaceFunctions = {
    "register_entry",
    "resolve",
    "unregister",
    "clear",
};

}  // namespace
