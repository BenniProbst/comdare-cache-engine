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
//     'v' ("1.0.0.c"). Die GERENDERTE Form (planner_version_stamp) bleibt praefixfrei "planner@1.0.0.c" -- der
//     Renderer algo_semver_string schneidet das 'v' weg (wie "v1" -> "1.0.0"). Der CX-W5-Schritt selbst war
//     byte-identisch zum vorigen Stand; das 'c' hat A13-M3/C4 nachgezogen -- das ist ein DEKLARIERTES
//     Byte-Ereignis der --dump-plan-Zeile ("planner@1.0.0" -> "planner@1.0.0.c"), kein Q10-Verstoss.
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
/// ROH-Literal traegt das 'v' (Owner-Q10); A13-M3/C4 hat es auf "1.0.0.c" gezogen (Owner-Q3-Flag-Grammatik).
inline constexpr std::string_view kPlannerVersion = "1.0.0.c";

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

/// Die Planer-Stempel-Zeile fuer den --dump-plan-Header: "planner@X.Y.Z[.flag]* isa=<isa> os=<os>".
/// FLAG-GRAMMATIK v2: das 'v'-Praefix ist entfallen, rohe und gerenderte Form fallen damit zusammen --
/// algo_semver_string ist fuer ein wohlgeformtes Literal die Identitaet. Der Aufruf bleibt trotzdem
/// stehen: er erzwingt die kanonische Form und faengt ein Fehl-Literal als "0.0.0", statt es roh
/// durchzureichen. Die Zeile lautet "planner@1.0.0.c ...".
[[nodiscard]] inline std::string planner_version_stamp() {
    std::string s{"planner@"};
    s += ::comdare::cache_engine::measurement::algo_semver_string(kPlannerVersion);
    s += " isa=";
    s += planner_target_isa();
    s += " os=";
    s += planner_target_os();
    return s;
}

static_assert(kPlannerVersion == std::string_view{"1.0.0.c"});
// RENDER-TREUE (Byte-Wache): das Literal parst auf {1,0,0} mit CPU-Basis, und es rendert VERBATIM zurueck.
// Die Prueform ist bewusst der RENDER-VERGLEICH und nicht mehr ein von Hand aufgebauter AlgoSemVer{...}:
// mit der Flag-LISTE muesste ein Aggregat-Literal die interne Knoten-Darstellung nachbauen -- eine zweite
// Wahrheit ueber die Repraesentation, die bei jeder Deckel-Aenderung mitgepflegt werden muesste. Der
// Roundtrip-Vergleich sagt dasselbe, haengt aber an der ZEICHENFOLGE, die der Stempel wirklich traegt.
static_assert(::comdare::cache_engine::measurement::render_algo_semver(
                  ::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion))
                  .view() == kPlannerVersion);
static_assert(::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion).x == 1u);
static_assert(::comdare::cache_engine::measurement::parse_algo_semver(kPlannerVersion).has_top_level_flag("c"));
// B12-Politik (ungated, immer gebaut): der Planer-Stempel ist wohlgeformt. Die flaglose Form bliebe hier
// GRAMMATISCH wohlgeformt -- den flaglosen Fall weist erst der gated Zwilling unten ab.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kPlannerVersion),
              "kPlannerVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n punkt-getrennten "
              "Flags; traegt sie Flags, MUSS 'c' darunter sein (Owner-F-10 07.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
// SCHARFSCHALTUNG (Migrations-Naht, algo_semver.hpp Klasse (e)): der Planer-Stempel MUSS -- wie jede
// ce-Version -- die CPU-Basis tragen. Ohne diese Wache haette der Planer die Migration umgangen.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kPlannerVersion),
              "kPlannerVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version 'c' unter ihren Flags "
              "tragen (Owner-Q3 02.08.2026 / F-10 07.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif
static_assert(planner_target_isa() == std::string_view{"x86_64"});

} // namespace comdare::cache_engine::planner
