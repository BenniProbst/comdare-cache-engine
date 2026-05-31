#pragma once
// V41.F.6.1.P2.F axis_03a_search_algo Interface-Functions-Liste (2026-05-26)
//
// @topic traversal @achse 03a search_algo

#include <array>
#include <string_view>

namespace comdare::cache_engine::lookup::concepts {

/// Pflicht-Interface-Functions der 03a Search-Algo-Achse (SearchAlgoVariant).
/// Properties (occupied_count/density_percent) sind keine Paper-Bodies und nicht hier.
inline constexpr std::array<std::string_view, 4> kAxisInterfaceFunctions = {
    "insert",
    "lookup",
    "erase",
    "clear",
};

}  // namespace
