#pragma once
// V41.F.6.1.P2.F axis_06_allocator Interface-Functions-Liste (2026-05-26)
//
// @topic allocator @achse 06
//
// Achsen-Interface-Functions-Liste fuer Paper-Original-Code-Validierung
// ([[legacy-code-sha256-validation]] User-Direktive P2.A0.6).
//
// Tool apps/is_original_validator nutzt diese Liste fuer Pflicht-Validierung:
// jedes manifest.txt MUSS Mappings fuer jede dieser Functions haben (sonst
// Build-Fail mit klarer Diagnose).

#include <array>
#include <string_view>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/// Pflicht-Interface-Functions der Allocator-Achse (Schnittmenge AllocatorStrategy).
/// Sub-Concepts (calloc/realloc/usable_size/...) sind NICHT hier — sie sind optional.
inline constexpr std::array<std::string_view, 2> kAxisInterfaceFunctions = {
    "allocate",
    "deallocate",
};

}  // namespace
