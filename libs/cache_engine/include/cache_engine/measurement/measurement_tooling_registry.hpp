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

#include <cache_engine/measurement/algo_semver.hpp> // A13-M1b-Fixup: Flag-Grammatik-Wachen (Owner-Q3)

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
    std::string_view   id;      ///< kanonischer Legenden-/XML-/Stempel-Token ("wallclock"/"macro"/"micro")
    std::string_view   name;    ///< exakt der Enum-Name (Doku/Reporting)
    std::string_view   version; ///< A2 (G2-4 Schritt 4): bump-bare Code-Version der Mess-Tooling-Achse ("v1.0.0",
                                ///< render-neutral zu "v1"); measurement_stamp_line liest sie statt der Hartkodierung.
};

/// Die EINE Registry der Mess-Tooling-HAUPT-Achse -- Index == Tooling-Wert (static_assert-gesichert). Die `id`-Token
/// stimmen mit measurement_stamp_line (abi/anatomy_version_stamp.hpp) ueberein: measurement_tooling=<id>@X.Y.Z.
inline constexpr std::array<MeasurementToolingInfo, kMeasurementToolingCount> kMeasurementToolingRegistry{{
    {MeasurementTooling::WallClock, "wallclock", "WallClock", "v1.0.0"},
    {MeasurementTooling::Macro, "macro", "Macro", "v1.0.0"},
    {MeasurementTooling::Micro, "micro", "Micro", "v1.0.0"},
}};

namespace detail {
// A13-M1b-Fixup (Review-BEFUND-1): dieselbe Wachen-Doppelung wie an der Organ-Registry
// (axis_variant_version_table.hpp) auch HIER -- die Mess-Tooling-Versionen stempeln in die Mess-Zeile.
[[nodiscard]] consteval bool tooling_versionen_flag_konform() {
    for (auto const& e : kMeasurementToolingRegistry)
        if (parse_algo_semver(e.version).has_hardware_flag() && !version_satisfies_cpu_only_policy(e.version))
            return false;
    return true;
}
[[nodiscard]] consteval bool tooling_versionen_cpu_pflicht() {
    for (auto const& e : kMeasurementToolingRegistry)
        if (!version_satisfies_cpu_only_policy(e.version)) return false;
    return true;
}
} // namespace detail
static_assert(detail::tooling_versionen_flag_konform(),
              "Mess-Tooling-Version mit FALSCHEM Hardware-Flag: im CPU-only-Scope ist GENAU 'c' (bzw. 'ce') "
              "zulaessig (Owner-Q3 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::tooling_versionen_cpu_pflicht(),
              "Mess-Tooling-Version ohne CPU-Hardware-Flag: im CPU-only-Scope MUSS jede Version auf 'c' oder "
              "'ce' enden (Owner-Q3 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif

namespace detail {
[[nodiscard]] consteval bool tooling_registry_is_complete() {
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i) {
        if (static_cast<std::size_t>(kMeasurementToolingRegistry[i].tooling) != i) return false;
        if (kMeasurementToolingRegistry[i].id.empty()) return false;
        if (kMeasurementToolingRegistry[i].name.empty()) return false;
        if (kMeasurementToolingRegistry[i].version.empty()) return false; // A2: Code-Version nie leer
    }
    return true;
}
} // namespace detail
static_assert(detail::tooling_registry_is_complete(),
              "kMeasurementToolingRegistry: 3 Eintraege, Index==Tooling, id/name/version nie leer");

/// constexpr-Lookup (Index == Tooling-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr MeasurementToolingInfo const& tooling_info(MeasurementTooling t) noexcept {
    return kMeasurementToolingRegistry[static_cast<std::size_t>(t)];
}

/// A2 (G2-4 Schritt 4): die bump-bare Code-Version zu einer Tooling-`id` (Stempel-Token). Bekannte id -> ihre
/// Registry-Version ("v1.0.0"); UNBEKANNTE id -> Sentinel (dokumentierter Render-Wechsel @0.0.0 NUR fuer
/// ungueltige ids; gueltige golden-ids bleiben render-neutral bei "v1.0.0" -> "1.0.0"). measurement_stamp_line
/// (abi/anatomy_version_stamp.hpp) liest hierueber statt der frueheren "v1"-Hartkodierung.
///
/// A13-M1b (Owner-Q3 02.08.2026 "Versionierungen sind ... immer 3-Stellig"): der Sentinel-Rueckgabewert war
/// die Kurzform "v0", die mit dem Kurzform-Rueckbau nur noch ZUFAELLIG (weil unparsbar) auf den Sentinel
/// fiele. Er steht deshalb jetzt dreistellig als "v0.0.0" da -- ABSICHTLICH statt zufaellig, byte-neutral:
/// beide Formen rendern ueber algo_semver_string identisch "0.0.0".
[[nodiscard]] constexpr std::string_view tooling_version_for_id(std::string_view id) noexcept {
    for (std::size_t i = 0; i < kMeasurementToolingCount; ++i)
        if (kMeasurementToolingRegistry[i].id == id) return kMeasurementToolingRegistry[i].version;
    return "v0.0.0"; // unbekannte id -> Sentinel (dreistellig, Owner-Q3)
}

/// Compile-time-Iteration ueber die Mess-Tooling-HAUPT-Achse (Metaprogrammierungs-Interface fuer den Fan-out).
template <class Visitor>
constexpr void for_each_measurement_tooling(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kMeasurementToolingRegistry[I]), ...);
    }(std::make_index_sequence<kMeasurementToolingCount>{});
}

} // namespace comdare::cache_engine::measurement
