#pragma once
// CRC64-ECMA-182 — header-only, KEINE Abhaengigkeit, kein Python (NEW-GOLDEN-ALL-AXES, 2026-07-18).
//
// ZWECK: eine kompakte, deterministische 64-bit-Pruefsumme, um grosse eingefrorene Referenz-Mengen (z.B. die
// 131.072 golden-binary_ids) als EINEN committeten uint64_t-Anker zu verankern, statt eine 62-MB-Datei ins git
// zu legen (irreversibler Repo-Bloat ueber die Mirrors + CI; widerspricht dem Feasibility-Thema, §0-GOAL nennt
// CRC64). Die Referenz-Menge wird im Test ON-DEMAND regeneriert (lazy StaticBinaryView) und ihre CRC64 gegen den
// Anker geprueft; die Vollmaterialisierung als Datei bleibt reine Inspektions-Option (gen_golden_fullpilot).
//
// ALGORITHMUS: CRC-64/ECMA-182 (ISO 3309 / ECMA-182), Standard-Katalog-Parameter:
//   Polynom (normal, NICHT reflektiert) = 0x42F0E1EBA9EA3693, Init = 0, RefIn = false, RefOut = false, XorOut = 0.
//   Check-Vektor: crc64_ecma182("123456789") == 0x6C40DF5F0B497347 (constexpr static_assert unten belegt es).
// Table-driven MSB-first (256-Eintrag constexpr-Tabelle, compile-time gebaut; O(n) je Puffer).

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::fingerprint {

inline constexpr std::uint64_t kCrc64Ecma182Poly = 0x42F0E1EBA9EA3693ULL; // normal (non-reflected) generator

namespace crc64_detail {

[[nodiscard]] inline constexpr std::array<std::uint64_t, 256> make_crc64_table() noexcept {
    std::array<std::uint64_t, 256> table{};
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint64_t crc = static_cast<std::uint64_t>(n) << 56;
        for (int k = 0; k < 8; ++k) crc = (crc & 0x8000000000000000ULL) ? ((crc << 1) ^ kCrc64Ecma182Poly) : (crc << 1);
        table[n] = crc;
    }
    return table;
}

inline constexpr std::array<std::uint64_t, 256> kCrc64Table = make_crc64_table();

} // namespace crc64_detail

/// crc64_ecma182_update(crc, bytes) — Streaming-Fortschreibung (Init=0 beim ersten Aufruf; XorOut=0 am Ende).
[[nodiscard]] inline constexpr std::uint64_t crc64_ecma182_update(std::uint64_t              crc,
                                                                  std::span<std::byte const> bytes) noexcept {
    for (std::byte const b : bytes) {
        std::uint8_t const idx =
            static_cast<std::uint8_t>((crc >> 56) ^ static_cast<std::uint64_t>(static_cast<std::uint8_t>(b)));
        crc = crc64_detail::kCrc64Table[idx] ^ (crc << 8);
    }
    return crc;
}

/// crc64_ecma182_update(crc, text) — Bequemlichkeit fuer Text (die golden-ids sind ASCII).
[[nodiscard]] inline constexpr std::uint64_t crc64_ecma182_update(std::uint64_t crc, std::string_view text) noexcept {
    for (char const c : text) {
        std::uint8_t const idx =
            static_cast<std::uint8_t>((crc >> 56) ^ static_cast<std::uint64_t>(static_cast<std::uint8_t>(c)));
        crc = crc64_detail::kCrc64Table[idx] ^ (crc << 8);
    }
    return crc;
}

/// crc64_ecma182(bytes/text) — CRC64 ueber einen ganzen Puffer (Init=0, XorOut=0).
[[nodiscard]] inline constexpr std::uint64_t crc64_ecma182(std::span<std::byte const> bytes) noexcept {
    return crc64_ecma182_update(0ULL, bytes);
}
[[nodiscard]] inline constexpr std::uint64_t crc64_ecma182(std::string_view text) noexcept {
    return crc64_ecma182_update(0ULL, text);
}

// Selbst-Beleg (compile-time): der Katalog-Check-Vektor. Faengt jede Poly-/Konventions-Drift der Impl.
static_assert(crc64_ecma182(std::string_view{"123456789"}) == 0x6C40DF5F0B497347ULL,
              "CRC-64/ECMA-182 Check-Vektor verletzt — Impl-/Poly-Drift.");

} // namespace comdare::fingerprint
