// profile_facade/planner/planner_version.hpp -- statischer Selbst-Versions-Stempel des Experiment-Planers
// (Bau W12-A, Section43.b).
//
// Section43.b (User-Direktive): "Der Experiment-Planer traegt auch einen string_view-Stempel, aber nur ueber die
// eigene statische Versionierung und unter welcher ISA/OS er passt und ausgefuehrt werden kann." Der Planer
// permutiert (er ist keine Permutation) -> KEINE Achsen-Arrays, nur die Selbst-Version X.Y.Z + eine
// Ausfuehrbarkeits-Deklaration (ISA/OS). Die ISA stammt aus der Section40.a-Signatur-Domaene (hardware_isa).
//
// Ausgegeben im --dump-plan-Header (planner_version_stamp()). Rein additiv, golden-/binary_id-neutral.
// X.Y.Z voll ausgeschrieben (neue Stempel-Welt, NICHT die .algos-Sig).

#pragma once

#include <cache_engine/measurement/hardware_isa_system_axis.hpp> // Section40.a: Amd64HostIsaAxis::host_isa()

#include <string>
#include <string_view>

namespace comdare::cache_engine::planner {

/// Selbst-Version des Experiment-Planers, X.Y.Z (initial 1.0.0; X.Y = Feature, Z = Debug-Revision).
inline constexpr std::string_view kPlannerVersion = "1.0.0";

/// Ziel-ISA, unter der der Planer ausfuehrbar ist (aus der Section40.a-Signatur-Domaene; heute x86_64).
[[nodiscard]] constexpr std::string_view planner_target_isa() noexcept {
    return ::comdare::cache_engine::measurement::Amd64HostIsaAxis::host_isa();
}

/// Ziel-OS (compile-time, kein Runtime-Sniff).
[[nodiscard]] constexpr std::string_view planner_target_os() noexcept {
#if defined(_WIN32)
    return "windows";
#elif defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "macos";
#else
    return "unknown";
#endif
}

/// Die Planer-Stempel-Zeile fuer den --dump-plan-Header: "planner@X.Y.Z isa=<isa> os=<os>".
[[nodiscard]] inline std::string planner_version_stamp() {
    std::string s{"planner@"};
    s += kPlannerVersion;
    s += " isa=";
    s += planner_target_isa();
    s += " os=";
    s += planner_target_os();
    return s;
}

static_assert(kPlannerVersion == std::string_view{"1.0.0"});
static_assert(planner_target_isa() == std::string_view{"x86_64"});

} // namespace comdare::cache_engine::planner
