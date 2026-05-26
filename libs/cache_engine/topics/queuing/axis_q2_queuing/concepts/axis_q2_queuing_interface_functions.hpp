#pragma once
// V41.F.6.1.P2.F axis_q2_queuing Interface-Functions-Liste (2026-05-26)
//
// @topic queuing @achse Q2 flush_policy

#include <array>
#include <string_view>

namespace comdare::cache_engine::queuing::axis_q2_queuing::concepts {

/// Pflicht-Interface-Functions der Q2 Flush-Policy-Achse (FlushPolicy).
inline constexpr std::array<std::string_view, 2> kAxisInterfaceFunctions = {
    "should_flush",
    "on_flush_complete",
};

}  // namespace
