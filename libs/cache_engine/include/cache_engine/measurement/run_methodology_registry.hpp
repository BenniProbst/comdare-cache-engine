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
    // S5-P1 (P-VOLLZUG, Section 47/55, 2026-07-20): die Build-Semantik jeder Ablauf-Methode. Der Planer speist
    // daraus den CMAKE_BUILD_TYPE + die Mess-/Thread-Politik der emittierten Bau-/Mess-Jobs (Single-Source statt
    // Magic-String im Emitter). GOLDEN/binary_id-NEUTRAL: reine Bau-/Mess-Matrix, KEIN Stempel-Feld -- opt/simd/
    // build_type sind system_config und fliessen NIE in binary_id (Q2 Option C).
    std::string_view cmake_build_type; ///< CMAKE_BUILD_TYPE dieses Laufs ("Debug"/"Release")
    bool             measurement_on;   ///< misst der Lauf (golden-Zahlen) oder baut/referenziert er nur
    bool             single_thread;    ///< 1-Thread-deterministischer Mess-Vollzug (Section 38.b)
};

/// Die EINE Registry der Run-Methodik-UNTER-Achse -- Index == RunMethodology-Wert (static_assert-gesichert).
/// S5-P1-Build-Semantik: measure = deterministischer 1-Thread-Messlauf (Release, misst); debug = paralleler
/// Verdrahtungs-Check (Debug, misst, KEINE 1-Thread-Determinismus-Garantie); release = Referenz-Durchsatz (Release,
/// misst NICHT). Der Emitter waehlt fuer die S5-Mess-Strecke die measure-Zeile (der Methodik-Fanout ist S6).
inline constexpr std::array<RunMethodologyInfo, kRunMethodologyCount> kRunMethodologyRegistry{{
    {RunMethodology::Debug, "debug", "Debug", "Debug", true, false},
    {RunMethodology::Measure, "measure", "Measure", "Release", true, true},
    {RunMethodology::Release, "release", "Release", "Release", false, false},
}};

namespace detail {
[[nodiscard]] consteval bool run_methodology_registry_is_complete() {
    for (std::size_t i = 0; i < kRunMethodologyCount; ++i) {
        if (static_cast<std::size_t>(kRunMethodologyRegistry[i].methodology) != i) return false;
        if (kRunMethodologyRegistry[i].id.empty()) return false;
        if (kRunMethodologyRegistry[i].name.empty()) return false;
        if (kRunMethodologyRegistry[i].cmake_build_type.empty()) return false; // S5-P1: Build-Typ nie leer
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
// S5-P1 Build-Semantik-Anker: Drift der Build-/Mess-/Thread-Politik einer Methode bricht hier compile-time. Der
// Emitter verlaesst sich auf measure == {Release, misst, 1-Thread} (die S5-Mess-Strecke); direkter Index-Zugriff,
// weil run_methodology_info() erst weiter unten deklariert ist.
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].cmake_build_type ==
                      std::string_view{"Release"} &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].measurement_on &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].single_thread,
              "kRunMethodologyRegistry: measure = {Release, misst, 1-Thread-deterministisch} (S5-Mess-Strecke).");
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].cmake_build_type ==
                      std::string_view{"Debug"} &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].measurement_on &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].single_thread,
              "kRunMethodologyRegistry: debug = {Debug, misst, parallel/kein 1-Thread-Garantie}.");
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].cmake_build_type ==
                      std::string_view{"Release"} &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].measurement_on &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].single_thread,
              "kRunMethodologyRegistry: release = {Release, misst NICHT, parallel} (Referenz-Durchsatz).");

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
