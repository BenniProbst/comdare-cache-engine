#pragma once
// V41.F.6.1.P1 Phase B — COMDARE_IS_ORIGINAL Macro fuer Paper-Original-Code-Validierung
//
// @stand V41.F.6.1.P1 Phase B Pilot
// @reference [[legacy-code-sha256-validation]] Memory
// @reference [[consteval-sha256-function-validation]] Memory
//
// **Macro-Stringification-Workaround** (kein C++26 Reflection vor 2027 LTS):
// Pro Function-Body wird COMDARE_IS_ORIGINAL_FROM_STRING manuell aufgerufen.
// Macro generiert:
//   - static constexpr Source-Storage (raw-string-literal)
//   - static_assert auf 50 KB Compile-Time-Budget
//   - static constexpr bool is_original_<function_name>() noexcept Pflicht-API
//
// Verwendung im Wrapper:
//
//   class MyWrapper {
//   public:
//       COMDARE_IS_ORIGINAL_FROM_STRING(
//           allocate,
//           R"COMDARE(
// void* allocate(std::size_t bytes, std::size_t align) {
//     return std::malloc(bytes);
// }
// )COMDARE",
//           "5f8b3a..." /* expected sha256 hex string, 64 chars */
//       )
//   };
//
// **Phase B.2 (TODO):** ersetze raw-string-literal durch xxd-generated .inc-File
// (#include "function_body.cpp.inc") fuer Compile-Time-Embed echter Source-Datei.

#include "ctsha.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::sha256 {

/**
 * @brief from_hex — consteval Konvertierung 64-char hex string → Digest
 *
 * Erlaubt SHA256-Erwartungswert als String-Literal im Macro-Aufruf.
 * Macht das Pattern besser lesbar als Array-Literal mit 32 hex bytes.
 *
 * @example consteval Digest d = from_hex("5f8b3a8e...");  // 64 hex chars
 */
consteval Digest from_hex(std::string_view hex) noexcept {
    Digest d{};
    if (hex.size() != 64) return d; // Defekt-Marker: bleibt all-zero, Test schlaegt fehl
    auto to_nibble = [](char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
        return 0;
    };
    for (std::size_t i = 0; i < 32; ++i) {
        d[i] = static_cast<std::uint8_t>((to_nibble(hex[2 * i]) << 4) | to_nibble(hex[2 * i + 1]));
    }
    return d;
}

/**
 * @brief to_hex — runtime + constexpr Konvertierung Digest → 64-char hex string
 *
 * Nuetzlich fuer Diagnose-Ausgaben + Tests (z.B. fehlgeschlagene Validierung).
 */
constexpr std::array<char, 64> to_hex(Digest const& d) noexcept {
    std::array<char, 64> out{};
    constexpr char       kHex[] = "0123456789abcdef";
    for (std::size_t i = 0; i < 32; ++i) {
        out[2 * i + 0] = kHex[(d[i] >> 4) & 0x0f];
        out[2 * i + 1] = kHex[d[i] & 0x0f];
    }
    return out;
}

} // namespace comdare::cache_engine::sha256

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_IS_ORIGINAL_FROM_GENERATED — Pflicht-Macro mit Pre-Build-Generated-Header
// ─────────────────────────────────────────────────────────────────────────────
//
// **User-Direktive [[compile-time-only-no-runtime]]:**
// is_original-Validierung MUSS zur Build-Time geschehen, NIE zur Compile-Time
// (Cache-Engine-Compile) oder Runtime.
//
// Workflow:
//   1. apps/is_original_validator Pre-Build-Tool (siehe [[paper-original-code-pattern]])
//      berechnet SHA256 zur BUILD-TIME und schreibt:
//        ${BUILD}/generated/<topic>/<wrapper>_is_original.hpp
//      mit `inline constexpr bool kIsOriginal_<fn> = true|false;`
//
//   2. Wrapper includiert generierten Header + nutzt COMDARE_IS_ORIGINAL_FROM_GENERATED
//      um is_original_<fn>() als statische Funktion zu exposen:
//        #include "<wrapper>_is_original.hpp"
//        class MyWrapper {
//            COMDARE_IS_ORIGINAL_FROM_GENERATED(allocate, kIsOriginal_allocate)
//        };
//
//   3. Im Cache-Engine-Compile + Hot-Path: is_original_allocate() returnt
//      direkten constexpr bool literal → ZERO Runtime-Cost, kein consteval-SHA.

#define COMDARE_IS_ORIGINAL_FROM_GENERATED(fn, generated_constant)                                                     \
    [[nodiscard]] static constexpr bool is_original_##fn() noexcept {                                                  \
        return generated_constant; /* aus Pre-Build-Tool-Header (literal) */                                           \
    }

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_IS_ORIGINAL_NOT_APPLICABLE — Re-Implementation-Marker
// ─────────────────────────────────────────────────────────────────────────────
//
// Fuer Pseudocode-Papers / Non-C/C++/Go-Originale [[pseudocode-papers-fallback]]:
// is_original_##fn() = false HART, ohne SHA-Validierung.

#define COMDARE_IS_ORIGINAL_NOT_APPLICABLE(fn)                                                                         \
    [[nodiscard]] static constexpr bool is_original_##fn() noexcept {                                                  \
        return false; /* Re-Implementation, kein Original-Code-Linking */                                              \
    }
