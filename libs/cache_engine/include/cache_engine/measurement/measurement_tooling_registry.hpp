#pragma once
// Mess-Tooling-HAUPT-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): die AUFFAECHERUNGS-Achse
// des CEB-Typs [a,b,c]. {WallClock, Macro, Micro} = die Mess-INSTRUMENTIERUNG, die je Wahl fest in eine CEB (und
// ihre Tier-Binaries) einkompiliert wird (F7-Selektivitaet). Jede Tooling-KONFIG erzeugt eine eigene
// "ceb:build:[a,b,c]"-Strecke (N Tooling-Konfigs -> N CEB-Pipelines).
//
// ABGRENZUNG (Section 54-T2): die 16 MeasurementCategory (measurement_axis_registry.hpp) sind die Mess-Tooling-
// UNTER-Achse (Planer-gesteuert, delegiert, manifestieren sich als CSV-Spalten) -- NICHT die HAUPT-Auffaecherung.
// Die Ablaufmethodik {Debug/Messen/Release} + Workloads/Datasets + Rueckschrieb-Methoden sind weitere UNTER-Achsen.
// Section 43: NUR die Mess-Tooling-HAUPT-Wahl traegt den kMeasurementAxisVersionLine-Stempel (measurement_stamp_line
// in abi/anatomy_version_stamp.hpp konsumiert exakt diese Tooling-id).
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog measurement_axis_registry).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// Die Mess-Tooling-HAUPT-Achse: WELCHE Mess-Instrumentierung fest einkompiliert wird (Section 47/55).
enum class MeasurementTooling : std::uint8_t {
    WallClock, ///< Wall-Clock-Zeit -- die Basis-Zeitmessung (immer verfuegbar, latenzarm)
    Macro,     ///< Makro-Benchmark -- Ende-zu-Ende-Durchsatz/Latenz ueber Observer
    Micro,     ///< Micro-Benchmark -- feinkoernige PMC/Counter-Instrumentierung
};

// Single-Source: Drift eines 4. Toolings bricht hier compile-time (statt still 3 zu bleiben).
inline constexpr std::size_t kMeasurementToolingCount = 3;

struct MeasurementToolingInfo {
    MeasurementTooling tooling;
    std::string_view   id;   ///< kanonischer Legenden-/XML-/Stempel-Token ("wallclock"/"macro"/"micro")
    std::string_view   name; ///< exakt der Enum-Name (Doku/Reporting)
};

/// Die EINE Registry der Mess-Tooling-HAUPT-Achse -- Index == Tooling-Wert (static_assert-gesichert). Die `id`-Token
/// stimmen mit measurement_stamp_line (abi/anatomy_version_stamp.hpp) ueberein: measurement_tooling=<id>@X.Y.Z.
inline constexpr std::array<MeasurementToolingInfo, kMeasurementToolingCount> kMeasurementToolingRegistry{{
    {MeasurementTooling::WallClock, "wallclock", "WallClock"},
    {MeasurementTooling::Macro, "macro", "Macro"},
    {MeasurementTooling::Micro, "micro", "Micro"},
}};

namespace detail {
[[nodiscard]] consteval bool tooling_registry_is_complete() {
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) {
        if (static_cast<std::size_t>(kMeasurementToolingRegistry[i].tooling) != i) return false;
        if (kMeasurementToolingRegistry[i].id.empty()) return false;
        if (kMeasurementToolingRegistry[i].name.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(detail::tooling_registry_is_complete(),
              "kMeasurementToolingRegistry: 3 Eintraege, Index==Tooling, id/name nie leer");

/// constexpr-Lookup (Index == Tooling-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr MeasurementToolingInfo const& tooling_info(MeasurementTooling t) noexcept {
    return kMeasurementToolingRegistry[static_cast<std::size_t>(t)];
}

/// Compile-time-Iteration ueber die Mess-Tooling-HAUPT-Achse (Metaprogrammierungs-Interface fuer den Fan-out).
template <class Visitor>
constexpr void for_each_measurement_tooling(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kMeasurementToolingRegistry[I]), ...);
    }(std::make_index_sequence<kMeasurementToolingCount>{});
}

} // namespace comdare::cache_engine::measurement
