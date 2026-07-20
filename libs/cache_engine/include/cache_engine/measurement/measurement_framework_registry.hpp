#pragma once
// Mess-Framework-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): WELCHES
// Last-/Mess-Framework den Workload treibt. Heute EIN ehrlicher Baustein {Ycsb} (honest-1) -- der reale
// YCSB-Generator/-Konsum (load_framework_system_axis.hpp / discover_load_profiles). Eine spaetere
// Erweiterung (z.B. ein zweites Framework) faechert diese Registry additiv auf (der static_assert unten
// bricht dann bewusst, bis der Zaehler nachgezogen ist -- kein stilles Phantom).
//
// ABGRENZUNG (Section 54-T2): eine Mess-Tooling-UNTER-Achse (Planer-gesteuert, delegiert, binary_id-NEUTRAL)
// -- NICHT die HAUPT-Auffaecherung (MeasurementTooling, measurement_tooling_registry.hpp). A9.1 traegt diese
// Achse PASSIV (Feld + Parse + XSD + validate-id-Check); der Fan-out/Vollzug gehoert S5.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog measurement_tooling_registry).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// Die Mess-Framework-UNTER-Achse: WELCHES Last-/Mess-Framework den Workload treibt (Section 47/55).
enum class MeasurementFramework : std::uint8_t {
    Ycsb, ///< YCSB -- der Yahoo Cloud Serving Benchmark (der heute EINZIGE reale Last-Treiber, honest-1)
};

// Single-Source: Drift eines 2. Frameworks bricht hier compile-time (statt still 1 zu bleiben).
inline constexpr std::size_t kMeasurementFrameworkCount = 1;

struct MeasurementFrameworkInfo {
    MeasurementFramework framework;
    std::string_view     id;   ///< kanonischer XML-/Legenden-Token ("ycsb")
    std::string_view     name; ///< exakt der Enum-Name (Doku/Reporting)
};

/// Die EINE Registry der Mess-Framework-UNTER-Achse -- Index == MeasurementFramework-Wert (static_assert-gesichert).
inline constexpr std::array<MeasurementFrameworkInfo, kMeasurementFrameworkCount> kMeasurementFrameworkRegistry{{
    {MeasurementFramework::Ycsb, "ycsb", "Ycsb"},
}};

namespace detail {
[[nodiscard]] consteval bool measurement_framework_registry_is_complete() {
    for (std::size_t i = 0; i < kMeasurementFrameworkCount; ++i) {
        if (static_cast<std::size_t>(kMeasurementFrameworkRegistry[i].framework) != i) return false;
        if (kMeasurementFrameworkRegistry[i].id.empty()) return false;
        if (kMeasurementFrameworkRegistry[i].name.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(kMeasurementFrameworkRegistry.size() == kMeasurementFrameworkCount,
              "kMeasurementFrameworkRegistry: Array-Groesse == kMeasurementFrameworkCount (Anzahl-Anker).");
static_assert(detail::measurement_framework_registry_is_complete(),
              "kMeasurementFrameworkRegistry: 1 Eintrag, Index==MeasurementFramework, id/name nie leer.");
// Namen-Anker: Drift des id-Tokens (Umbenennung) bricht hier compile-time.
static_assert(kMeasurementFrameworkRegistry[static_cast<std::size_t>(MeasurementFramework::Ycsb)].id ==
                  std::string_view{"ycsb"},
              "kMeasurementFrameworkRegistry: id-Token ist {ycsb} (Namen-Anker).");

/// constexpr-Lookup (Index == MeasurementFramework-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr MeasurementFrameworkInfo const& measurement_framework_info(MeasurementFramework f) noexcept {
    return kMeasurementFrameworkRegistry[static_cast<std::size_t>(f)];
}

/// Compile-time-Iteration ueber die Mess-Framework-UNTER-Achse (Metaprogrammierungs-Interface).
template <class Visitor>
constexpr void for_each_measurement_framework(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kMeasurementFrameworkRegistry[I]), ...);
    }(std::make_index_sequence<kMeasurementFrameworkCount>{});
}

} // namespace comdare::cache_engine::measurement
