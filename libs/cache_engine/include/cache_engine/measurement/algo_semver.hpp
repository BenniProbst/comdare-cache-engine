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
// A13-M1 (Owner-Entscheid E2 vom 02.08.2026, verbatim): "Die Markierung experimenteller Achsen-Algorithmen aus
// einem Pruefling wird in der Versionsbezifferung eines jeden Achsen-Algorithmus mit einem angehaengten 'e' fuer
// 'experimental' markiert." -> optionales TRAILING-'e' (klein, direkt angehaengt, kein Separator) an der rohen Form
// ("v1e", "v2.3.4e") UND an der gerenderten Voll-Form ("2.3.4e"). Das Flag reist im AlgoSemVer mit und geht in
// operator== ein -> ein experimenteller Stand traegt ein anderes Stempel-Segment und damit einen eigenen
// SHA-512-Fingerprint/Lager-Key.
//
// SENTINEL-WACHE (A13-Review-Auflage K-5): das NULL-TRIPEL IST der Sentinel -- auch mit 'e'. "v0e"/"0.0.0e" parsen
// exakt auf AlgoSemVer{} (experimental FALSE), damit die bestehenden CT-Wachen (parse != Sentinel) nicht ueber die
// neue Form ausgehebelt werden koennen. is_sentinel() prueft zusaetzlich NUR das x/y/z-Tripel (nicht den ganzen
// Struct) -- die Wachen bleiben auch dann dicht, wenn eine spaetere Aenderung das Flag doch durchreicht.
//
// Metaprog: reines constexpr, kein std::variant, keine vtable, keine schweren Includes.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// X.Y.Z-Tripel (X.Y = Feature, Z = Debug-Revision) + A13-M1-Experimental-Flag (Owner-E2 'e'-Suffix).
struct AlgoSemVer {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
    /// true == der Algorithmus-Stand ist EXPERIMENTELL (Trailing-'e' aus einem Pruefling, Owner-E2 02.08.2026).
    /// ce-EIGENE Registry-Varianten duerfen das NIE tragen (CT-Wache in axis_variant_version_table.hpp).
    bool experimental = false;

    /// K-5-Sentinel-Praedikat: der Sentinel ist das NULL-TRIPEL -- unabhaengig vom Flag. CT-Wachen pruefen HIERMIT,
    /// nicht per Struct-Vergleich gegen AlgoSemVer{}, damit eine experimentelle Fehlform sie nie umgeht.
    [[nodiscard]] constexpr bool is_sentinel() const noexcept { return x == 0u && y == 0u && z == 0u; }
};

[[nodiscard]] constexpr bool operator==(AlgoSemVer const& a, AlgoSemVer const& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.experimental == b.experimental;
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

/// Konsumiert ein optionales Trailing-'e' (A13-M1). NUR klein-'e'; der Aufrufer prueft danach auf Zeilen-Ende
/// (ein Rest wie "e5"/"ee" bleibt damit unparsbar -> Sentinel).
[[nodiscard]] constexpr bool take_experimental(std::string_view& s) noexcept {
    if (s.empty() || s.front() != 'e') return false;
    s.remove_prefix(1);
    return true;
}

/// K-5: baut das Ergebnis-Tripel und faltet JEDES Null-Tripel auf den reinen Sentinel AlgoSemVer{} zurueck --
/// "v0e"/"0.0.0e" sind der Sentinel, nicht ein "experimenteller Sentinel".
[[nodiscard]] constexpr AlgoSemVer make_semver(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                                               bool experimental) noexcept {
    if (x == 0u && y == 0u && z == 0u) return AlgoSemVer{};
    return AlgoSemVer{x, y, z, experimental};
}
} // namespace detail

/// Parst "vN" (-> {N,0,0}), "vN.N.N" (-> {N,N,N}) und die A13-M1-Experimental-Formen "vNe"/"vN.N.Ne"
/// (-> zusaetzlich experimental=true). Alles andere (leer, ohne 'v', "vN.N", "vN.Ne", Zusatz-Rest, Gross-'E',
/// nicht-numerisch) -> {0,0,0}. Das Null-Tripel bleibt IMMER der reine Sentinel (K-5). Rein constexpr.
[[nodiscard]] constexpr AlgoSemVer parse_algo_semver(std::string_view v) noexcept {
    if (v.empty() || v.front() != 'v') return {};
    v.remove_prefix(1);
    auto const [x, okx] = detail::take_uint(v);
    if (!okx) return {};
    if (v.empty()) return detail::make_semver(x, 0u, 0u, false); // "vN" == N.0.0
    if (v.front() == 'e') {                                      // "vNe" == N.0.0 experimentell
        v.remove_prefix(1);
        if (!v.empty()) return {}; // Rest nach dem 'e' -> Sentinel ("vNee", "vNe5")
        return detail::make_semver(x, 0u, 0u, true);
    }
    if (v.front() != '.') return {}; // Rest muss exakt ".N.N[e]" sein
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "vN.N" verboten (auch "vN.Ne")
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz) return {};
    bool const experimental = detail::take_experimental(v);
    if (!v.empty()) return {}; // Rest muss leer sein (kein "vN.N.N.extra", kein "vN.N.Ne5")
    return detail::make_semver(x, y, z, experimental);
}

/// Die X.Y.Z-VOLL-Form als String -- NUR fuer neue Stempel-Zeilen/Planer/Registry (NIE die .algos-Sig).
/// A13-M1: ein experimenteller Stand rendert mit angehaengtem 'e' ("2.3.4e"); der Sentinel rendert immer "0.0.0".
/// GOLDEN-NEUTRAL: kein Bestands-algo_version traegt heute ein 'e' -> jede reale Zeile bleibt byte-identisch.
[[nodiscard]] inline std::string algo_semver_string(std::string_view algo_version) {
    AlgoSemVer const v   = parse_algo_semver(algo_version);
    std::string      out = std::to_string(v.x) + '.' + std::to_string(v.y) + '.' + std::to_string(v.z);
    if (v.experimental) out += 'e';
    return out;
}

/// parse_dotted_semver (G2-1a / A3): parst die bereits GERENDERTE Voll-Form "X.Y.Z" (dotted, OHNE 'v') zurueck in
/// ein Tripel -- die Umkehrung von algo_semver_string. Getrennt von parse_algo_semver (das die ROHE "vN"-Form parst):
/// der consteval-Stempel-Parser (anatomy_stamp_entries.hpp) liest die "achse=algo@X.Y.Z"-Zeilen, deren Versions-Teil
/// bereits die gerenderte Form traegt. STRENG: exakt drei dotted uints + optionales Trailing-'e' (A13-M1); alles
/// andere (leer, mit 'v', Kurzform "X.Y", Zusatz-Rest, nicht-numerisch) -> {0,0,0}-Sentinel. Rein constexpr.
/// Der PUNKT ist damit ausschliesslich TRENNER der drei Zahlen -- hierarchische Namen (Owner-Q2-Muster
/// "prt-art.memory.abc@1.0.0") leben im NAMENS-Anteil VOR dem '@' und beruehren diesen Parser nie.
[[nodiscard]] constexpr AlgoSemVer parse_dotted_semver(std::string_view v) noexcept {
    auto const [x, okx] = detail::take_uint(v);
    if (!okx || v.empty() || v.front() != '.') return {};
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "X.Y" verboten
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz) return {};
    bool const experimental = detail::take_experimental(v);
    if (!v.empty()) return {}; // Rest muss leer sein (kein "X.Y.Z.extra", kein "X.Y.Ze5")
    return detail::make_semver(x, y, z, experimental);
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

// parse_dotted_semver (A3): die gerenderte "X.Y.Z"-Form; Umkehrung von algo_semver_string.
static_assert(parse_dotted_semver("1.0.0") == AlgoSemVer{1, 0, 0}); // heutiger gerenderter Stand
static_assert(parse_dotted_semver("2.3.4") == AlgoSemVer{2, 3, 4});
static_assert(parse_dotted_semver("10.0.1") == AlgoSemVer{10, 0, 1});
static_assert(parse_dotted_semver("v1.0.0") == AlgoSemVer{0, 0, 0});  // mit 'v' (rohe Form) -> Sentinel
static_assert(parse_dotted_semver("1.0") == AlgoSemVer{0, 0, 0});     // Kurzform -> Sentinel
static_assert(parse_dotted_semver("1.0.0.1") == AlgoSemVer{0, 0, 0}); // Zusatz-Rest -> Sentinel
static_assert(parse_dotted_semver("") == AlgoSemVer{0, 0, 0});        // leer -> Sentinel
static_assert(parse_dotted_semver("x.y.z") == AlgoSemVer{0, 0, 0});   // nicht-numerisch -> Sentinel
// Round-Trip: algo_semver_string rendert "vN" -> "N.0.0", parse_dotted_semver liest es exakt zurueck.
static_assert(parse_dotted_semver("1.0.0") == parse_algo_semver("v1"));

// -- A13-M1: 'e'-Suffix (Owner-E2 02.08.2026) -- vollstaendige CT-Batterie -------------------------
// (a) Bestand bleibt nicht-experimentell (Golden-Neutralitaet: kein Bestands-String traegt ein 'e').
static_assert(!parse_algo_semver("v1").experimental);
static_assert(!parse_algo_semver("v2.3.4").experimental);
static_assert(!parse_dotted_semver("1.0.0").experimental);
// (b) Rohe Form MIT 'e': Kurzform "vNe" und Voll-Form "vN.N.Ne".
static_assert(parse_algo_semver("v1e") == AlgoSemVer{1, 0, 0, true});
static_assert(parse_algo_semver("v2.3.4e") == AlgoSemVer{2, 3, 4, true});
static_assert(parse_algo_semver("v10.0.1e") == AlgoSemVer{10, 0, 1, true});
static_assert(parse_algo_semver("v1e").experimental);
// (c) Gerenderte Form MIT 'e' (die Stempel-Zeilen-Ruecklesung).
static_assert(parse_dotted_semver("2.3.4e") == AlgoSemVer{2, 3, 4, true});
static_assert(parse_dotted_semver("1.0.0e") == AlgoSemVer{1, 0, 0, true});
// (d) K-5-SENTINEL-WACHE: "v0e"/"v0.0.0e"/"0.0.0e" sind der REINE Sentinel (Flag faellt weg) -- die bestehenden
//     CT-Wachen (parse != AlgoSemVer{}) bleiben damit auch gegen die neue Form dicht.
static_assert(parse_algo_semver("v0e") == AlgoSemVer{});
static_assert(parse_algo_semver("v0.0.0e") == AlgoSemVer{});
static_assert(parse_dotted_semver("0.0.0e") == AlgoSemVer{});
static_assert(!parse_algo_semver("v0e").experimental); // KEIN "experimenteller Sentinel"
static_assert(!parse_dotted_semver("0.0.0e").experimental);
static_assert(parse_algo_semver("v0e").is_sentinel()); // x/y/z-Praedikat (statt Struct-Vergleich)
static_assert(parse_algo_semver("v0").is_sentinel());
static_assert(parse_dotted_semver("0.0.0e").is_sentinel());
static_assert(!parse_algo_semver("v1e").is_sentinel()); // ein echter experimenteller Stand ist KEIN Sentinel
static_assert(!parse_algo_semver("v2.3.4e").is_sentinel());
// (e) Fehlformen mit 'e' -> Sentinel (nur klein-'e', nur genau eines, nur ganz am Ende, nie an der Kurzform "vN.N").
static_assert(parse_algo_semver("v1.2e") == AlgoSemVer{}); // Kurzform bleibt verboten
static_assert(parse_algo_semver("v1E") == AlgoSemVer{});   // Gross-'E' ist kein Marker
static_assert(parse_algo_semver("v1ee") == AlgoSemVer{});  // doppeltes 'e'
static_assert(parse_algo_semver("v1e5") == AlgoSemVer{});  // Rest nach dem 'e'
static_assert(parse_algo_semver("v2.3.4e5") == AlgoSemVer{});
static_assert(parse_algo_semver("ve") == AlgoSemVer{});        // ohne Zahl
static_assert(parse_algo_semver("1.0.0e") == AlgoSemVer{});    // gerenderte Form in der rohen Wache -> Sentinel
static_assert(parse_dotted_semver("1.0e") == AlgoSemVer{});    // Kurzform
static_assert(parse_dotted_semver("v2.3.4e") == AlgoSemVer{}); // rohe Form in der gerenderten Wache
static_assert(parse_dotted_semver("2.3.4E") == AlgoSemVer{});
static_assert(parse_dotted_semver("2.3.4ee") == AlgoSemVer{});
// (f) UNTERSCHEIDBARKEIT (Fingerprint-/Lager-Wirkung): experimentell != stabil bei sonst gleichem Tripel.
static_assert(!(parse_algo_semver("v1e") == parse_algo_semver("v1")));
static_assert(!(parse_dotted_semver("2.3.4e") == parse_dotted_semver("2.3.4")));
// (g) Round-Trip rohe <-> gerenderte Form INKLUSIVE Flag.
static_assert(parse_dotted_semver("1.0.0e") == parse_algo_semver("v1e"));
static_assert(parse_dotted_semver("2.3.4e") == parse_algo_semver("v2.3.4e"));

} // namespace comdare::cache_engine::measurement
