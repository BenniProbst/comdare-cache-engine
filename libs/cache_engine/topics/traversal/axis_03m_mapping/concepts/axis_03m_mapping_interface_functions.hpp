#pragma once
// V41.F.6.1.P2.F axis_03m_mapping Interface-Functions-Liste (2026-05-26)
//
// @topic traversal @achse 03m mapping

#include <array>
#include <string_view>

namespace comdare::cache_engine::traversal::axis_03m_mapping::concepts {

/// Pflicht-Interface-Functions der 03m Mapping-Achse (MappingVariant).
/// Properties (mapped_count) sind keine Paper-Bodies und nicht hier.
inline constexpr std::array<std::string_view, 4> kAxisInterfaceFunctions = {
    "register_slot",
    "resolve_offset",
    "reverse_lookup",
    "clear",
};

}  // namespace
