#pragma once
// builder/ceb_version_stamp.hpp -- G2-1/A5 (Lager-Gate, A8-Erfuellung fuer die CEB): der CEB-Selbst-Stempel. Die CEB
// (CacheEngineBuilder) hat KEINE Gesamt-Version (der Planer schon: planner_version.hpp). Ihre Provenienz ist ihr
// Mess-Tooling-ANGEBOT: das Mess-Array [Xa.Ya.Za, Xb.Yb.Zb, Xc.Yc.Zc] je Tooling aus kMeasurementToolingRegistry,
// X.Y.Z-gerendert -- DIESELBE Form wie die Tier-Binary-measurement_stamp_line (keine zweite Wahrheit / keine Drift).
// Dazu eine eigene SHA-512-Provenienz-Zeile ueber diese Mess-Array-Zeile, via anatomy_fingerprint_hex mit leeren
// organ/system/merge-Anteilen ("","",mess,"") -- die EINE K7b-Primitive wiederverwendet, keine zweite.
//
// Alles consteval (registry-abgeleitet, Compile-Zeit-konstant); nur die Ausgabe-Formatierung (ceb_version_stamp())
// ist Runtime. Ausgabe im CEB-Log-Kopf (apps/cache_engine_builder). Rein additiv, golden-/binary_id-neutral.

#include <cache_engine/abi/anatomy_fingerprint.hpp>                  // anatomy_fingerprint_hex (consteval SHA-512)
#include <cache_engine/measurement/algo_semver.hpp>                  // parse_algo_semver (rohe "vX.Y.Z" -> {x,y,z})
#include <cache_engine/measurement/measurement_tooling_registry.hpp> // kMeasurementToolingRegistry (Single-Source)

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
/// consteval: Laenge der gerenderten Mess-Array-Zeile "measurement_tooling=<id>@X.Y.Z;..." (ohne '\0').
[[nodiscard]] consteval std::size_t ceb_measurement_stamp_len() noexcept {
    using ::comdare::cache_engine::measurement::kMeasurementToolingCount;
    using ::comdare::cache_engine::measurement::kMeasurementToolingRegistry;
    using ::comdare::cache_engine::measurement::parse_algo_semver;
    std::size_t n = 0;
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) {
        if (i != 0) ++n; // ';'
        n += std::string_view{"measurement_tooling="}.size();
        n += kMeasurementToolingRegistry[i].id.size();
        ++n; // '@'
        auto const v = parse_algo_semver(kMeasurementToolingRegistry[i].version);
        n += ceb_digits(v.x) + 1 + ceb_digits(v.y) + 1 + ceb_digits(v.z);
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
