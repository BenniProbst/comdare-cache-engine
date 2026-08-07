#pragma once
// Mess-Framework-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): WELCHES
// Last-/Mess-Framework den Workload treibt. Heute EIN ehrlicher Baustein {Ycsb} (honest-1) -- der reale
// YCSB-Generator/-Konsum (load_framework_measurement_axis.hpp / discover_load_profiles). Eine spaetere
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

#include <cache_engine/measurement/algo_semver.hpp> // A13-M1b-Fixup: Flag-Grammatik-Wachen (Owner-Q3)

namespace comdare::cache_engine::measurement {

/// Die Mess-Framework-UNTER-Achse: WELCHES Last-/Mess-Framework den Workload treibt (Section 47/55).
enum class MeasurementFramework : std::uint8_t {
    Ycsb, ///< YCSB -- der Yahoo Cloud Serving Benchmark (der heute EINZIGE reale Last-Treiber, honest-1)
};

// Single-Source: Drift eines 2. Frameworks bricht hier compile-time (statt still 1 zu bleiben).
inline constexpr std::size_t kMeasurementFrameworkCount = 1;

struct MeasurementFrameworkInfo {
    MeasurementFramework framework;
    std::string_view     id;      ///< kanonischer XML-/Legenden-Token ("ycsb")
    std::string_view     name;    ///< exakt der Enum-Name (Doku/Reporting)
    std::string_view     version; ///< O-8 Schritt 0A: bump-bare Code-Version der Mess-Framework-Achse (seit
                                  ///< A13-M3/C4 "1.0.0.c"; Muster MeasurementToolingInfo.version). INERT angelegt --
    ///< der Emissions-Konsument (Segment der kMeasurementAxisVersionLine) entsteht erst mit
    ///< der Scharfschaltung in O-8 Schritt 9. Bis dahin liest sie NIEMAND: byte-neutral.
};

/// Die EINE Registry der Mess-Framework-UNTER-Achse -- Index == MeasurementFramework-Wert (static_assert-gesichert).
inline constexpr std::array<MeasurementFrameworkInfo, kMeasurementFrameworkCount> kMeasurementFrameworkRegistry{{
    {MeasurementFramework::Ycsb, "ycsb", "Ycsb", "1.0.0.c"},
}};

namespace detail {
// A13-M1b-Fixup (Review-BEFUND-1) + B12 (Codex-Review 02.08.2026): Wachen-Batterie wie an Organ-/Tooling-
// Registry, ueber die EINE Politik aus algo_semver.hpp (parsbar + nie 'e' + Flag-konform). Vorher fehlten
// hier die Parsbarkeits- und die 'e'-Pflicht -- junk waere still als @0.0.0 gestempelt worden.
[[nodiscard]] consteval bool framework_versionen_wohlgeformt() {
    for (auto const& e : kMeasurementFrameworkRegistry)
        if (!ce_owned_version_is_wellformed(e.version)) return false;
    return true;
}
[[nodiscard]] consteval bool framework_versionen_cpu_pflicht() {
    for (auto const& e : kMeasurementFrameworkRegistry)
        if (!ce_owned_version_satisfies_cpu_enforce(e.version)) return false;
    return true;
}
} // namespace detail
static_assert(detail::framework_versionen_wohlgeformt(),
              "Mess-Framework-Version verletzt die ce-Registry-Politik: (a) UNPARSBAR (und nicht der "
              "dokumentierte Sentinel \"0.0.0\") -- ein junk-Literal wuerde still als @0.0.0 stempeln; oder "
              "(b) experimentelles 'e' (AUSSCHLIESSLICH die Pruefling-Markierung, Owner-E2 02.08.2026); oder "
              "(c) FALSCHES Hardware-Flag (im CPU-only-Scope GENAU 'c' bzw. 'ce', Owner-Q3 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::framework_versionen_cpu_pflicht(),
              "Mess-Framework-Version ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version "
              "auf 'c' enden und darf NIE experimentell sein (Owner-Q3/E2 02.08.2026) -- "
              "COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif

namespace detail {
[[nodiscard]] consteval bool measurement_framework_registry_is_complete() {
    for (std::size_t i = 0; i < kMeasurementFrameworkCount; ++i) {
        if (static_cast<std::size_t>(kMeasurementFrameworkRegistry[i].framework) != i) return false;
        if (kMeasurementFrameworkRegistry[i].id.empty()) return false;
        if (kMeasurementFrameworkRegistry[i].name.empty()) return false;
        if (kMeasurementFrameworkRegistry[i].version.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(kMeasurementFrameworkRegistry.size() == kMeasurementFrameworkCount,
              "kMeasurementFrameworkRegistry: Array-Groesse == kMeasurementFrameworkCount (Anzahl-Anker).");
static_assert(detail::measurement_framework_registry_is_complete(),
              "kMeasurementFrameworkRegistry: 1 Eintrag, Index==MeasurementFramework, id/name/version nie leer.");
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
