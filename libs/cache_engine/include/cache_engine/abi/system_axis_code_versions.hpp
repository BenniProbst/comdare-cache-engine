#pragma once
// abi/system_axis_code_versions.hpp -- Single-Source der 5 System-Achsen-CODE-Versionen (Bau W12 / G2-4 Schritt 3,
// Lager-Gate A2). Frueher waren diese Versionen in system_stamp_line() (anatomy_version_stamp.hpp) hartkodiert als
// 5x {"<achse>","code","v1"}. Jetzt: EINE Tabelle -> die Stempel-Funktion iteriert sie; ab jetzt ist jede System-
// Achsen-Code-Version einzeln BUMP-BAR (A10-X.Y.Z-Disziplin), ohne die Stempel-Funktion anzufassen.
//
// RENDER-NEUTRAL (A11): "v1.0.0" rendert ueber algo_semver_string identisch zu "1.0.0" wie zuvor "v1" -> der
// emittierte Quelltext + der gerenderte System-Stempel bleiben byte-identisch (Byte-Wache/CRC-Wache unberuehrt).
// "code" (der Algorithmus-Marker der System-Achse) lebt weiter in system_stamp_line, NICHT hier: diese Tabelle
// traegt NUR die pro-Achse bump-bare Code-Version, die Achse-zu-Marker-Zuordnung ist Sache der Stempel-Funktion.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle (analog kMeasurementToolingRegistry). header-only, C++23.

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Eine System-Haupt-Achse und ihre bump-bare Code-Version (die statische Code-Identitaet der Achse).
struct SystemAxisCodeVersion {
    std::string_view axis; ///< System-Haupt-Achsen-Name ("compiler"/"extension_hardware"/...)
    std::string_view
        version; ///< rohe Code-Version ("v1.0.0"); wird von build_axis_version_stamp_line zu X.Y.Z gerendert
};

// Single-Source: Drift einer 6. System-Achse bricht hier compile-time (statt still 5 zu bleiben).
inline constexpr std::size_t kSystemAxisCodeCount = 5;

/// Die EINE Tabelle der System-Achsen-Code-Versionen -- Reihenfolge == kanonische System-Stempel-Ordnung (Section 43,
/// W12-A-1). Init "v1.0.0" x5 (render-neutral zum frueheren "v1"); je Eintrag ab jetzt einzeln bump-bar.
inline constexpr std::array<SystemAxisCodeVersion, kSystemAxisCodeCount> kSystemAxisCodeVersions{{
    {"compiler", "v1.0.0"},
    {"extension_hardware", "v1.0.0"},
    {"target_isa", "v1.0.0"},
    {"scheduling", "v1.0.0"},
    {"load_framework", "v1.0.0"},
}};

namespace detail {
[[nodiscard]] consteval bool system_axis_code_versions_complete() {
    for (std::size_t i = 0; i < kSystemAxisCodeCount; ++i) {
        if (kSystemAxisCodeVersions[i].axis.empty()) return false;
        if (kSystemAxisCodeVersions[i].version.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(detail::system_axis_code_versions_complete(),
              "kSystemAxisCodeVersions: 5 Eintraege, axis/version nie leer");

} // namespace comdare::cache_engine::abi
