#pragma once
// V41.F.6.1.P2.F axis_q1_queuing Interface-Functions-Liste (2026-05-26)
//
// @topic queuing @achse Q1 buffer_strategy

#include <array>
#include <string_view>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/// Pflicht-Interface-Functions der Q1 Buffer-Strategy-Achse (BufferStrategy).
/// Properties (size/is_empty) sind keine Paper-Bodies und nicht hier.
inline constexpr std::array<std::string_view, 6> kAxisInterfaceFunctions = {
    "put",
    "get",
    "emplace",
    "peek_front",
    "peek_back",
    "clear",
};

}  // namespace
