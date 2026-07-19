// measurement/algo_semver.hpp -- X.Y.Z-Interpretation des algo_version-Strings (Bau W12-A, Section43.b).
//
// Section43.b (User-Direktive): Versionen sind X.Y.Z mit X.Y = Feature-Version, Z = Debug-Revision im selben
// Feature-Stand -- fuer jeden Achsen-Algorithmus (alle Typen) einzeln + den Planer statisch.
//
// KRITISCH (Byte-Wache, Section43): Dieser Helfer PARST nur den bereits existierenden algo_version-String
// (`W::algo_version`, heute ueberall "v1") in ein X.Y.Z-Tripel. Er wird AUSSCHLIESSLICH von den NEUEN
// Stempel-Zeilen / dem Planer-Stempel / der neuen Registry-Emission genutzt -- NIE vom bestehenden
// compose_algo_signature-/.algos-Serialisierungspfad (der bleibt byte-identisch: "v1" serialisiert weiter
// als "v1", nie "1.0.0"). Kein gemeinsamer Formatter mit der Alt-Welt.
//
// Format (Architekt-Entscheid W12-A-2): NUR "vN" (== N.0.0) oder "vN.N.N". Kurzformen wie "v1.2" sind
// VERBOTEN (Mehrdeutigkeit). Unparsbar/Sentinel "v0" -> {0,0,0} (deckungsgleich zum @v0-Sentinel des
// compose-Pfads; der Aufrufer raet nie).
//
// Metaprog: reines constexpr, kein std::variant, keine vtable, keine schweren Includes.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// X.Y.Z-Tripel (X.Y = Feature, Z = Debug-Revision).
struct AlgoSemVer {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
};

[[nodiscard]] constexpr bool operator==(AlgoSemVer const& a, AlgoSemVer const& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

namespace detail {
/// Konsumiert eine Dezimalzahl am Anfang von s (modifiziert s). Rueckgabe {wert, gefunden}.
[[nodiscard]] constexpr std::pair<std::uint32_t, bool> take_uint(std::string_view& s) noexcept {
    if (s.empty() || s.front() < '0' || s.front() > '9') return {0u, false};
    std::uint32_t n = 0;
    while (!s.empty() && s.front() >= '0' && s.front() <= '9') {
        n = n * 10u + static_cast<std::uint32_t>(s.front() - '0');
        s.remove_prefix(1);
    }
    return {n, true};
}
} // namespace detail

/// Parst "vN" (-> {N,0,0}) oder "vN.N.N" (-> {N,N,N}). Alles andere (leer, ohne 'v', "vN.N", Zusatz-Rest,
/// nicht-numerisch) -> {0,0,0}. Rein constexpr.
[[nodiscard]] constexpr AlgoSemVer parse_algo_semver(std::string_view v) noexcept {
    if (v.empty() || v.front() != 'v') return {};
    v.remove_prefix(1);
    auto const [x, okx] = detail::take_uint(v);
    if (!okx) return {};
    if (v.empty()) return {x, 0u, 0u}; // "vN" == N.0.0
    if (v.front() != '.') return {};   // Rest muss exakt ".N.N" sein
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "vN.N" verboten
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz || !v.empty()) return {}; // Rest muss leer sein (kein "vN.N.N.extra")
    return {x, y, z};
}

/// Die X.Y.Z-VOLL-Form als String -- NUR fuer neue Stempel-Zeilen/Planer/Registry (NIE die .algos-Sig).
[[nodiscard]] inline std::string algo_semver_string(std::string_view algo_version) {
    AlgoSemVer const v = parse_algo_semver(algo_version);
    return std::to_string(v.x) + '.' + std::to_string(v.y) + '.' + std::to_string(v.z);
}

// -- Wohlgeformtheit (alles compile-time) --------------------------------------------------------
static_assert(parse_algo_semver("v1") == AlgoSemVer{1, 0, 0}); // heutiger Stand ALLER Algos
static_assert(parse_algo_semver("v0") == AlgoSemVer{0, 0, 0}); // Sentinel
static_assert(parse_algo_semver("v2.3.4") == AlgoSemVer{2, 3, 4});
static_assert(parse_algo_semver("v10.0.1") == AlgoSemVer{10, 0, 1});
static_assert(parse_algo_semver("v1.2") == AlgoSemVer{0, 0, 0});     // Kurzform verboten -> Sentinel
static_assert(parse_algo_semver("1.0.0") == AlgoSemVer{0, 0, 0});    // ohne 'v' -> Sentinel
static_assert(parse_algo_semver("") == AlgoSemVer{0, 0, 0});         // leer -> Sentinel
static_assert(parse_algo_semver("vx") == AlgoSemVer{0, 0, 0});       // nicht-numerisch -> Sentinel
static_assert(parse_algo_semver("v1.2.3.4") == AlgoSemVer{0, 0, 0}); // Zusatz-Rest -> Sentinel

} // namespace comdare::cache_engine::measurement
