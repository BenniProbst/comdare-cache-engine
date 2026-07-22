#pragma once
// abi/anatomy_fingerprint.hpp -- K7b-3 (Section 62-B / Section 64): der SHA-512-Fingerprint der vier einkompilierten
// Anatomy-Stempel-Zeilen (organ/system/measurement/merge), consteval berechnet aus der K7b-1-Primitive.
//
// D3 (User-GO 2026-07-22): Preimage == concat(organ + system + measurement + merge) in DIESER fixen Reihenfolge.
// D2: der Fingerprint reist als 128-hex nullterminierte Zeile ({char const*, uint64}) im AnatomyVersionLines-POD.
// GOLDEN-NEUTRAL: die Berechnung passiert INNEN im COMDARE_ANATOMY_VERSION_STAMP*-Makro -> der emittierte Quelltext
// (der Makro-Call, 2/3/4-arg) bleibt byte-identisch; der Fingerprint materialisiert erst in der Makro-Expansion,
// nicht im emittierten .cpp -> golden-CRC 0xF1C1F26A1232073B unberuehrt. Saat fuer den #46b-std::map-Lookup (ein
// kompakter, stabiler Provenienz-Schluessel je Tier-Binary).

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Obergrenze des Preimage-Puffers: die laengste Stempel-Zeile ist die 17-Achsen-Organ-Zeile; die Summe der vier
/// Zeilen bleibt weit darunter. Wird die Grenze je ueberschritten, bricht die consteval-Auswertung sichtbar (der
/// Puffer-Zugriff ist dann kein konstanter Ausdruck mehr).
inline constexpr std::size_t kAnatomyFingerprintPreimageMax = 4096;

/// anatomy_fingerprint_hex(organ, system, measurement, merge) -- 128-hex SHA-512 (nullterminiert, array<char, 129>)
/// von concat(organ + system + measurement + merge). consteval: reine Compile-Zeit-Ableitung der vier Stempel-Literale
/// (leere Zeilen -> leerer Anteil; der ce-only-/Katalog-Pfad {measurement="", merge=""} ergibt den Fingerprint von
/// concat(organ + system), deterministisch und ohne den emittierten Quelltext zu beruehren).
[[nodiscard]] consteval std::array<char, 129> anatomy_fingerprint_hex(std::string_view organ, std::string_view system,
                                                                      std::string_view measurement,
                                                                      std::string_view merge) noexcept {
    std::array<std::uint8_t, kAnatomyFingerprintPreimageMax> preimage{};
    std::size_t                                              n      = 0;
    auto                                                     append = [&](std::string_view s) {
        for (char c : s) preimage[n++] = static_cast<std::uint8_t>(c);
    };
    append(organ);
    append(system);
    append(measurement);
    append(merge);
    auto const digest = ::comdare::cache_engine::sha512::sha512(std::span<const std::uint8_t>{preimage.data(), n});
    auto const hex    = ::comdare::cache_engine::sha512::to_hex(digest); // array<char, 128>
    std::array<char, 129> out{};
    for (std::size_t i = 0; i < 128; ++i) out[i] = hex[i];
    out[128] = '\0';
    return out;
}

} // namespace comdare::cache_engine::abi
