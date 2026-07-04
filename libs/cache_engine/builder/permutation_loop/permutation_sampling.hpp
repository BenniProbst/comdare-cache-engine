#pragma once
// AP-5/#239 - deterministisches, toolchain-STABILES 1:sample_rate-Sampling fuer den FullSampled-Modus.
// Bewusst KEIN std::hash: dessen Ergebnis ist implementierungsdefiniert (variiert ueber Compiler/STL/OS)
// -> die gesampelte Menge waere NICHT reproduzierbar zwischen Windows-MSVC und ZIH-Linux-g++.
//
// AP-5-Follow: Stratifiziertes/gewichtetes Sampling und Coverage-Optimierung bleiben separat.

#include <cstdint>
#include <string_view>

namespace comdare::builder::loop {

inline constexpr std::uint64_t fnv1a64_offset_basis = 0xCBF29CE484222325ULL;
inline constexpr std::uint64_t fnv1a64_prime        = 0x100000001B3ULL;

inline constexpr std::uint64_t splitmix64_increment   = 0x9E3779B97F4A7C15ULL;
inline constexpr std::uint64_t splitmix64_multiplier1 = 0xBF58476D1CE4E5B9ULL;
inline constexpr std::uint64_t splitmix64_multiplier2 = 0x94D049BB133111EBULL;
inline constexpr int           splitmix64_shift1      = 30;
inline constexpr int           splitmix64_shift2      = 27;
inline constexpr int           splitmix64_shift3      = 31;

inline constexpr std::uint32_t sampling_disabled_rate     = 1u;
inline constexpr std::uint32_t sampling_min_filtered_rate = sampling_disabled_rate + 1u;

// FNV-1a-64 ueber die rohen Bytes der id - reine Integer-Arithmetik, identisch auf jeder Toolchain.
[[nodiscard]] constexpr std::uint64_t stable_id_hash(std::string_view id) noexcept {
    std::uint64_t h = fnv1a64_offset_basis;
    for (char c : id) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= fnv1a64_prime;
    }
    return h;
}

// splitmix64-Finalizer (Steele/Lea) - dekorreliert die niederwertigen Bits vor dem Modulo.
[[nodiscard]] constexpr std::uint64_t splitmix64_mix(std::uint64_t x) noexcept {
    x += splitmix64_increment;
    x = (x ^ (x >> splitmix64_shift1)) * splitmix64_multiplier1;
    x = (x ^ (x >> splitmix64_shift2)) * splitmix64_multiplier2;
    return x ^ (x >> splitmix64_shift3);
}

// Deterministischer 1:sample_rate keep-Entscheid, seed-parametrisiert, reproduzierbar ueber Laeufe UND Maschinen.
// sample_rate <= 1 behaelt ALLES (Sampling deaktiviert) -> Defined/Full unveraendert.
[[nodiscard]] constexpr bool sample_keep(std::string_view id, std::uint32_t sample_rate,
                                         std::uint64_t sample_seed) noexcept {
    if (sample_rate <= sampling_disabled_rate) return true;
    return (splitmix64_mix(stable_id_hash(id) ^ sample_seed) % sample_rate) == 0u;
}

} // namespace comdare::builder::loop
