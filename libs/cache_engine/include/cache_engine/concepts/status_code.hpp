#pragma once
// status_code.hpp — errno-style Status-Codes fuer Schreib-/IO-Operationen (BASIS).
//
// V41.F.6.1 Migration (Doku 19, Phase A): kanonische Heimat der hybriden
// SearchEngine-API-Status-Codes (REV 7.1, 2026-05-13). Frueher prt-art-lokal
// (comdare::prt_art::status_t) — gehoert als cross-cutting BASIS in die cache-engine;
// der Pruefling prt-art KONSUMIERT diese Codes (Richtung: prt-art -> cache-engine).
//
// Vertrag (siehe [[hybrid-search-engine-interface]]):
//   Alle Schreib-/IO-Operatoren (insert, erase, push_back, pop_back, clear, resize,
//   reserve, assign, ...) returnen IMMER `int` (= status_t):
//     0   = success
//     > 0 = spezifischer Fehlercode (Konstanten unten)
//     < 0 = reserviert fuer Extension (nicht verwendet)
//   Leseoperatoren behalten ihren natuerlichen Returntyp (optional/T&/size_t).

namespace comdare::cache_engine {

using status_t = int;

// Status-Code-Konstanten (errno-Style)
inline constexpr status_t status_ok                      = 0;
inline constexpr status_t status_key_already_exists      = 1;
inline constexpr status_t status_key_not_found           = 2;
inline constexpr status_t status_out_of_memory           = 3;
inline constexpr status_t status_invalid_argument        = 4;
inline constexpr status_t status_capacity_exceeded       = 5;
inline constexpr status_t status_locked                  = 6;
inline constexpr status_t status_out_of_range            = 7;
inline constexpr status_t status_empty_container         = 8;
inline constexpr status_t status_concurrent_modification = 9;
inline constexpr status_t status_io_error                = 10;

[[nodiscard]] inline constexpr bool status_is_ok(status_t s)    noexcept { return s == 0; }
[[nodiscard]] inline constexpr bool status_is_error(status_t s) noexcept { return s != 0; }

}  // namespace comdare::cache_engine
