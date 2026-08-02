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
//
// CX-W5 (Codex-Doppelreview 02.08.2026): der Planer-Selbst-Stempel ist ein ce-EIGENER Versionspfad und faellt
// damit unter dieselbe Owner-Grammatik wie jede Achsen-Algorithmus-Version. Zwei Owner-Direktiven greifen HIER:
//   * Owner-Q10 ("v gilt NUR fuer die Roh-Literale im Code"): das ROH-Literal kPlannerVersion traegt jetzt das
//     'v' ("v1.0.0"). Die GERENDERTE Form (planner_version_stamp) bleibt praefixfrei "planner@1.0.0" -- der
//     Renderer algo_semver_string schneidet das 'v' render-neutral weg (wie "v1" -> "1.0.0"), die
//     --dump-plan-Zeile ist byte-identisch zum vorigen Stand.
//   * Owner-Q3 (Flag-Grammatik "alle Versionen enden auf 'c'/'ce'"): die ce-Politik-Wachen aus algo_semver.hpp
//     (ce_owned_version_is_wellformed ungated + ce_owned_version_satisfies_cpu_enforce gated) sind unten genau
//     wie an jeder Registry-Variante angelegt; der Planer ist damit Teil der A13-M2/M3-Migrations-Naht
//     (algo_semver.hpp, Klasse (e)) statt sie zu umgehen.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>              // CX-W5: ce-Politik-Wachen + render-neutraler Semver
#include <cache_engine/measurement/hardware_isa_system_axis.hpp> // Section40.a: Amd64HostIsaAxis::host_isa()

#include <string>
#include <string_view>

namespace comdare::cache_engine::planner {

/// Selbst-Version des Experiment-Planers, X.Y.Z (initial 1.0.0; X.Y = Feature, Z = Debug-Revision). CX-W5: das
/// ROH-Literal traegt das 'v' (Owner-Q10) und wandert im M2/M3-Fenster auf "v1.0.0c" (Owner-Q3-Flag-Grammatik).
inline constexpr std::string_view kPlannerVersion = "v1.0.0";

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
/// CX-W5: die gerenderte Form ist praefixfrei (Owner-Q10) -- algo_semver_string schneidet das 'v' des
/// Roh-Literals render-neutral weg ("v1.0.0" -> "1.0.0"), die Zeile bleibt byte-identisch "planner@1.0.0 ...".
[[nodiscard]] inline std::string planner_version_stamp() {
    std::string s{"planner@"};
    s += ::comdare::cache_engine::measurement::algo_semver_string(kPlannerVersion);
    s += " isa=";
    s += planner_target_isa();
    s += " os=";
    s += planner_target_os();
    return s;
}

// CX-W5: das ROH-Literal traegt das 'v' (Owner-Q10).
static_assert(kPlannerVersion == std::string_view{"v1.0.0"});
// Render-Neutralitaet (Byte-Wache): das Roh-Literal parst auf {1,0,0} -> algo_semver_string rendert praefixfrei
// "1.0.0"; die --dump-plan-Zeile bleibt byte-identisch "planner@1.0.0 ...".
static_assert(::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion) ==
              ::comdare::cache_engine::measurement::AlgoSemVer{1, 0, 0});
// B12-Politik (ungated, immer gebaut): der Planer-Stempel ist wohlgeformt und traegt NIE 'e' (die
// Pruefling-Markierung, Owner-E2) -- der flaglose Uebergangs-Bestand "v1.0.0" ist bis zur M2/M3-Migration
// toleriert (deckungsgleich zu den 122 Registry-Literalen).
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kPlannerVersion),
              "kPlannerVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c' (CPU-Flag), NIE 'e' "
              "(Owner-Q3/E2 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
// A13-M2/M3-SCHARFSCHALTUNG (Migrations-Naht, algo_semver.hpp Klasse (e)): im Neuanker-Fenster geht das Define
// auf ON und der Planer-Stempel MUSS -- wie jede ce-Version -- auf 'c' enden (dann kPlannerVersion == "v1.0.0c")
// und darf nie experimentell sein. Ohne diese Wache haette der Planer die Migration planmaessig umgangen.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kPlannerVersion),
              "kPlannerVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version auf "
              "'c' enden und darf NIE experimentell sein (Owner-Q3/E2 02.08.2026) -- "
              "COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif
static_assert(planner_target_isa() == std::string_view{"x86_64"});

} // namespace comdare::cache_engine::planner
