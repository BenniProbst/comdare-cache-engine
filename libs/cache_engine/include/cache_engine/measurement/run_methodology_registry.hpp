#pragma once
// Run-Methodik-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): die 3 Ablauf-
// Methoden {Debug, Measure, Release} des Mess-Vollzugs. Sie TYPISIERT den offenen TODO in
// measurement_axis_registry.xml:55-56 ("die 3 Mess-Modi Debug/Mess/Release existieren NICHT als Typen ->
// nicht emittiert; erst nach ihrer Typisierung als Mess-Unter-Achse reflektierbar").
//
// ABGRENZUNG (Section 54-T2): dies ist eine Mess-Tooling-UNTER-Achse (Planer-gesteuert, delegiert,
// binary_id-NEUTRAL) -- NICHT die HAUPT-Auffaecherung (das ist MeasurementTooling {WallClock/Macro/Micro},
// measurement_tooling_registry.hpp, die ALLEIN den kMeasurementAxisVersionLine-Stempel traegt, Section 43).
// A9.1 traegt diese Achse PASSIV (Feld + Parse + XSD + validate-id-Check); der Fan-out/Vollzug gehoert S5.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog measurement_tooling_registry).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// Die Run-Methodik-UNTER-Achse: WELCHE Ablauf-Methode der Mess-Vollzug faehrt (Section 47/55; Section 32-F1/F7).
enum class RunMethodology : std::uint8_t {
    Debug,   ///< Debug-Lauf -- parallel/schnell, KEINE Mess-golden-Zahlen (Verifikation der Verdrahtung)
    Measure, ///< Mess-Lauf -- 1-Thread/deterministisch, die golden-Messung (Run-to-Run-stabil)
    Release, ///< Release-Lauf -- Voll-Optimierung ohne Mess-Instrumentierung (Referenz-Durchsatz)
};

// Single-Source: Drift einer 4. Methode bricht hier compile-time (statt still 3 zu bleiben).
inline constexpr std::size_t kRunMethodologyCount = 3;

struct RunMethodologyInfo {
    RunMethodology   methodology;
    std::string_view id;   ///< kanonischer XML-/Legenden-Token ("debug"/"measure"/"release")
    std::string_view name; ///< exakt der Enum-Name (Doku/Reporting)
};

/// Die EINE Registry der Run-Methodik-UNTER-Achse -- Index == RunMethodology-Wert (static_assert-gesichert).
inline constexpr std::array<RunMethodologyInfo, kRunMethodologyCount> kRunMethodologyRegistry{{
    {RunMethodology::Debug, "debug", "Debug"},
    {RunMethodology::Measure, "measure", "Measure"},
    {RunMethodology::Release, "release", "Release"},
}};

namespace detail {
[[nodiscard]] consteval bool run_methodology_registry_is_complete() {
    for (std::size_t i = 0; i < kRunMethodologyCount; ++i) {
        if (static_cast<std::size_t>(kRunMethodologyRegistry[i].methodology) != i) return false;
        if (kRunMethodologyRegistry[i].id.empty()) return false;
        if (kRunMethodologyRegistry[i].name.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(kRunMethodologyRegistry.size() == kRunMethodologyCount,
              "kRunMethodologyRegistry: Array-Groesse == kRunMethodologyCount (Anzahl-Anker).");
static_assert(detail::run_methodology_registry_is_complete(),
              "kRunMethodologyRegistry: 3 Eintraege, Index==RunMethodology, id/name nie leer.");
// Namen-Anker: Drift eines id-Tokens (Umbenennung/Vertauschung) bricht hier compile-time.
static_assert(
    kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].id == std::string_view{"debug"} &&
        kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].id == std::string_view{"measure"} &&
        kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].id == std::string_view{"release"},
    "kRunMethodologyRegistry: id-Tokens sind {debug,measure,release} (Namen-Anker).");

/// constexpr-Lookup (Index == RunMethodology-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr RunMethodologyInfo const& run_methodology_info(RunMethodology m) noexcept {
    return kRunMethodologyRegistry[static_cast<std::size_t>(m)];
}

/// Compile-time-Iteration ueber die Run-Methodik-UNTER-Achse (Metaprogrammierungs-Interface).
template <class Visitor>
constexpr void for_each_run_methodology(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kRunMethodologyRegistry[I]), ...);
    }(std::make_index_sequence<kRunMethodologyCount>{});
}

} // namespace comdare::cache_engine::measurement
