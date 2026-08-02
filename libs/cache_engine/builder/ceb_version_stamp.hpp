#pragma once
// builder/ceb_version_stamp.hpp -- G2-1/A5 (Lager-Gate, A8-Erfuellung fuer die CEB): der CEB-Selbst-Stempel. Die CEB
// (CacheEngineBuilder) hat KEINE Gesamt-Version (der Planer schon: planner_version.hpp). Ihre Provenienz ist ihr
// Mess-ANGEBOT: das Mess-Array [Xa.Ya.Za, Xb.Yb.Zb, Xc.Yc.Zc] je Tooling aus kMeasurementToolingRegistry,
// X.Y.Z-gerendert -- DIESELBE Form wie die Tier-Binary-measurement_stamp_line (keine zweite Wahrheit / keine Drift).
//
// O-8 Schritt 12 (Nachzug zu Schritt 9): die Zeile fuehrt jetzt ebenfalls das load_framework-Segment. Schritt 9
// hatte es nur in abi::measurement_stamp_line eingezogen und dabei DIESEN dritten Ableitungsweg uebersehen --
// die Folge war genau die Drift, die der Kopf hier ausschliesst, und der Drift-Guard in
// test_m_w12_stamp_bausteine (A5CebVersionStamp..., "EINE Wahrheit") wurde dadurch rot. Er ist die Wache, die
// den Fehler gefunden hat; abgeschwaecht wurde er nicht. Die Zeile bleibt golden-/binary_id-neutral (sie geht
// in keine binary_id ein), aendert aber ihre eigene SHA-512-Provenienz kCebFingerprint und den CEB-Log-Kopf --
// das ist die beabsichtigte Byte-Folge, nicht ein Nebeneffekt.
// Dazu eine eigene SHA-512-Provenienz-Zeile ueber diese Mess-Array-Zeile, via anatomy_fingerprint_hex mit leeren
// organ/system/merge-Anteilen ("","",mess,"") -- die EINE K7b-Primitive wiederverwendet, keine zweite.
//
// Alles consteval (registry-abgeleitet, Compile-Zeit-konstant); nur die Ausgabe-Formatierung (ceb_version_stamp())
// ist Runtime. Ausgabe im CEB-Log-Kopf (apps/cache_engine_builder). Rein additiv, golden-/binary_id-neutral.

#include <cache_engine/abi/anatomy_fingerprint.hpp>                    // anatomy_fingerprint_hex (consteval SHA-512)
#include <cache_engine/measurement/algo_semver.hpp>                    // parse_algo_semver (rohe "vX.Y.Z" -> {x,y,z})
#include <cache_engine/measurement/measurement_framework_registry.hpp> // O-8 Schritt 12: load_framework-Segment
#include <cache_engine/measurement/measurement_tooling_registry.hpp>   // kMeasurementToolingRegistry (Single-Source)

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder {

namespace detail {
/// consteval: Anzahl Dezimalstellen von n (mind. 1, auch fuer 0).
[[nodiscard]] consteval std::size_t ceb_digits(std::uint32_t n) noexcept {
    std::size_t d = 1;
    while (n >= 10) {
        n /= 10;
        ++d;
    }
    return d;
}
/// consteval: Laenge des fuehrenden load_framework-Segments "load_framework=<id>@X.Y.Z" (ohne Trenner).
/// O-8 Schritt 12: dieselbe Quelle und dieselbe Form wie in abi::measurement_stamp_line (Schritt 9).
// A13-M1b-Fixup (Review-BEFUND-2): der konsteval-Zwilling rendert den Owner-Q3-Flag-Schwanz MIT --
// heute no-op (Bestand flaglos), nach der M2/M3-Migration auf "v1.0.0c" automatisch korrekt. Laenge
// und Renderer sind symmetrisch aus denselben Registry-Werten berechnet; der Zwillingstest
// (test_m_w12, CEB-Zeile == abi::measurement_stamp_line) bleibt damit ueber die Migration gruen.
[[nodiscard]] consteval std::size_t ceb_flag_len(::comdare::cache_engine::measurement::AlgoSemVer const& v) noexcept {
    return (::comdare::cache_engine::measurement::hardware_flag_char(v.hardware) != '\0' ? 1U : 0U) +
           (v.experimental ? 1U : 0U);
}
[[nodiscard]] consteval std::size_t ceb_load_framework_segment_len() noexcept {
    using ::comdare::cache_engine::measurement::kMeasurementFrameworkRegistry;
    using ::comdare::cache_engine::measurement::parse_algo_semver;
    auto const& fw = kMeasurementFrameworkRegistry[0];
    auto const  v  = parse_algo_semver(fw.version);
    return std::string_view{"load_framework="}.size() + fw.id.size() + 1 // '@'
           + ceb_digits(v.x) + 1 + ceb_digits(v.y) + 1 + ceb_digits(v.z) + ceb_flag_len(v);
}
/// consteval: Laenge der gerenderten Mess-Array-Zeile
/// "load_framework=<id>@X.Y.Z;measurement_tooling=<id>@X.Y.Z;..." (ohne '\0').
[[nodiscard]] consteval std::size_t ceb_measurement_stamp_len() noexcept {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    using ::comdare::cache_engine::measurement::parse_algo_semver;
    std::size_t n = 0;
    // Leer-Semantik wie in Schritt 9: das Rahmen-Segment entsteht NUR, wenn ueberhaupt Tooling da ist.
    if (kMeasurementToolingCount != 0) n += ceb_load_framework_segment_len() + 1; // + ';'
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) {
        if (i != 0) ++n; // ';'
        n += std::string_view{"measurement_tooling="}.size();
        n += kMeasurementToolingRegistry[i].id.size();
        ++n; // '@'
        auto const v = parse_algo_semver(kMeasurementToolingRegistry[i].version);
        n += ceb_digits(v.x) + 1 + ceb_digits(v.y) + 1 + ceb_digits(v.z) + ceb_flag_len(v);
    }
    return n;
}
} // namespace detail

/// consteval: die gerenderte Mess-Array-Zeile als fixed char array (+ '\0'). X.Y.Z via parse_algo_semver -> keine
/// Drift zur Tier-Binary-measurement_stamp_line. Single-Source: kMeasurementToolingRegistry.
[[nodiscard]] consteval std::array<char, detail::ceb_measurement_stamp_len() + 1>
ceb_measurement_stamp_array() noexcept {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    using ::comdare::cache_engine::measurement::parse_algo_semver;
    std::array<char, detail::ceb_measurement_stamp_len() + 1> out{};
    std::size_t                                               p   = 0;
    auto                                                      put = [&](std::string_view s) {
        for (char const c : s) out[p++] = c;
    };
    auto put_num = [&](std::uint32_t v) {
        char        tmp[10]{};
        std::size_t d = 0;
        do {
            tmp[d++] = static_cast<char>('0' + (v % 10));
            v /= 10;
        } while (v != 0);
        for (std::size_t k = 0; k < d; ++k) out[p++] = tmp[d - 1 - k];
    };
    // O-8 Schritt 12: das load_framework-Segment steht VORNE -- dieselbe Ordnung wie in
    // abi::measurement_stamp_line (Schritt 9, OP-3: erstes Meta-Meta-Segment). Es entsteht nur bei
    // nicht-leerer Tooling-Menge, damit die Leer-Zeile leer bleibt.
    if (kMeasurementToolingCount != 0) {
        using ::comdare::cache_engine::measurement::kMeasurementFrameworkRegistry;
        auto const& fw = kMeasurementFrameworkRegistry[0];
        put("load_framework=");
        put(fw.id);
        out[p++]      = '@';
        auto const fv = parse_algo_semver(fw.version);
        put_num(fv.x);
        out[p++] = '.';
        put_num(fv.y);
        out[p++] = '.';
        put_num(fv.z);
        // A13-M1b-Fixup: Flag-Schwanz (heute no-op, Migration-sicher -- s. ceb_flag_len).
        if (char const hw = ::comdare::cache_engine::measurement::hardware_flag_char(fv.hardware); hw != '\0')
            out[p++] = hw;
        if (fv.experimental) out[p++] = 'e';
        out[p++] = ';';
    }
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) {
        if (i != 0) out[p++] = ';';
        put("measurement_tooling=");
        put(kMeasurementToolingRegistry[i].id);
        out[p++]     = '@';
        auto const v = parse_algo_semver(kMeasurementToolingRegistry[i].version);
        put_num(v.x);
        out[p++] = '.';
        put_num(v.y);
        out[p++] = '.';
        put_num(v.z);
        // A13-M1b-Fixup: Flag-Schwanz (heute no-op, Migration-sicher -- s. ceb_flag_len).
        if (char const hw = ::comdare::cache_engine::measurement::hardware_flag_char(v.hardware); hw != '\0')
            out[p++] = hw;
        if (v.experimental) out[p++] = 'e';
    }
    out[p] = '\0';
    return out;
}

/// Die gerenderte Mess-Array-Zeile (constexpr storage) + ihre string_view (ohne '\0').
inline constexpr auto             kCebMeasurementStampArray = ceb_measurement_stamp_array();
inline constexpr std::string_view kCebMeasurementStamp{kCebMeasurementStampArray.data(),
                                                       kCebMeasurementStampArray.size() - 1};

/// consteval SHA-512-Provenienz der CEB (A8): anatomy_fingerprint_hex ueber ("", "", Mess-Array-Zeile, "") -- die EINE
/// K7b-Primitive wiederverwendet (leere organ/system/merge). 128-hex, nullterminiert.
inline constexpr auto kCebFingerprintArray =
    ::comdare::cache_engine::abi::anatomy_fingerprint_hex("", "", kCebMeasurementStamp, "");
inline constexpr std::string_view kCebFingerprint{kCebFingerprintArray.data(), 128};

/// ceb_version_stamp() -- der CEB-Selbst-Stempel fuer den Log-Kopf/--version: die Mess-Array-Zeile + ihre SHA-512-
/// Provenienz. Runtime-String (nur Ausgabe-Formatierung); beide Bestandteile sind consteval (registry-abgeleitet).
[[nodiscard]] inline std::string ceb_version_stamp() {
    std::string s{"ceb-measurement="};
    s += kCebMeasurementStamp;
    s += ";sha512=";
    s += kCebFingerprint;
    return s;
}

} // namespace comdare::cache_engine::builder
