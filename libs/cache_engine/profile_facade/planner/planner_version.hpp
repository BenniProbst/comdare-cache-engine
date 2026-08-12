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

#include <cache_engine/abi/stempel_basis.hpp>                    // S-1: StempelBasis/StempelVertrag (Erbin PLANER)
#include <cache_engine/measurement/algo_semver.hpp>              // CX-W5: ce-Politik-Wachen + render-neutraler Semver
#include <cache_engine/measurement/hardware_isa_system_axis.hpp> // Section40.a: Amd64HostIsaAxis::host_isa()

#include <array>
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

// -- S-1 (P4): DIE PLANER-ERBIN DES STEMPEL-VERTRAGS ---------------------------------------------------

/// Die Teile des gesamt_stempel-Kompositums in ERBIN-genannter Reihenfolge (S-6-Sperre: die Basis kennt
/// keine Ordnung). BYTE-IDENTISCH zur bisherigen planner_version_stamp()-Konkatenation: dort lief
/// kPlannerVersion durch algo_semver_string, und Render==Identitaet ist fuer dieses Literal durch die
/// Roundtrip-Wache unten (render_algo_semver(parse_algo_semver(kPlannerVersion)).view() == kPlannerVersion)
/// BEWIESEN -- das Kompositum traegt deshalb das Roh-Literal selbst.
[[nodiscard]] constexpr std::array<std::string_view, 6> planer_gesamt_stempel_teile() noexcept {
    return {"planner@", kPlannerVersion, " isa=", planner_target_isa(), " os=", planner_target_os()};
}
inline constexpr auto kPlanerGesamtStempelKompositum =
    ::comdare::cache_engine::abi::stempel_kompositum<&planer_gesamt_stempel_teile>();

/// PlanerStempel -- die Erbin des CRTP-Stempel-Vertrags fuer den Traeger PLANER (S-1). Sie DELEGIERT auf
/// die Werte, die diese Datei schon traegt (S-1 aendert KEINEN Stempel-WERT, kein X.Y.Z-Bump).
struct PlanerStempel
    : ::comdare::cache_engine::abi::StempelBasis<PlanerStempel, ::comdare::cache_engine::abi::StempelTraeger::Planer> {
    /// version_xyz (KON7-04: NUR der Planer): das EINE Roh-Literal.
    [[nodiscard]] static constexpr std::string_view version_xyz() noexcept { return kPlannerVersion; }

    /// fingerprint_sha: DEKLARIERTE LUECKE -- die KON6-03-IST-Luecke "Planer-SHA fehlt" wird hiermit LAUT
    /// statt still. Die Fuellung ist ein EIGENER Posten; der Preimage-/Glied-Entscheid faellt NICHT in S-1.
    [[nodiscard]] static constexpr std::string_view fingerprint_sha() noexcept { return {}; }
    static constexpr bool                           kFingerprintShaBewusstLeer = true;
    static constexpr std::string_view               kFingerprintShaBewusstLeerGrund =
        "KON6-03-IST-Luecke, laut deklariert: der Planer traegt noch keinen SHA-512-Fingerprint; "
        "Fuellung = eigener Posten (Preimage-/Glied-Entscheid nicht in S-1)";

    /// gesamt_stempel: das consteval-Kompositum "planner@<X.Y.Z[.flag]*> isa=<isa> os=<os>".
    [[nodiscard]] static constexpr std::string_view gesamt_stempel() noexcept {
        return kPlanerGesamtStempelKompositum.view();
    }
};
/// DER ZWANG (S-1/P1.2): die Erbin schliesst direkt unter ihrer Definition mit dem Vertrag. Die
/// bestehenden Wachen unten bleiben VOLLSTAENDIG stehen -- die Erbin verschaerft, sie ersetzt nichts.
static_assert(
    ::comdare::cache_engine::abi::StempelVertrag<PlanerStempel, ::comdare::cache_engine::abi::StempelTraeger::Planer>,
    "S-1: PlanerStempel verletzt den Stempel-Vertrag des Traegers PLANER (Zulassungsmatrix "
    "kStempelZulassungsMatrix, stempel_basis.hpp)");

/// Die Planer-Stempel-Zeile fuer den --dump-plan-Header: "planner@X.Y.Z[.flag]* isa=<isa> os=<os>".
/// FLAG-GRAMMATIK v2: das 'v'-Praefix ist entfallen, rohe und gerenderte Form fallen damit zusammen --
/// algo_semver_string ist fuer ein wohlgeformtes Literal die Identitaet. S-1 (P4): die Funktion
/// DELEGIERT auf das consteval-Kompositum der Erbin, byte-identisch zur frueheren Laufzeit-Konkatenation
/// (Beweis: die Roundtrip-Wache unten haelt Render==Identitaet fest, und der Vertragstest friert das
/// Byte-Orakel mit dem Praefix-Anker ein). Der Fehl-Literal-Fang der frueheren algo_semver_string-Stufe
/// ist dabei NICHT entfallen, sondern VORVERLEGT: die static_asserts unten brechen ein Fehl-Literal
/// compile-hart, bevor irgendein Aufrufer eine Zeile sieht.
[[nodiscard]] inline std::string planner_version_stamp() { return std::string{PlanerStempel::gesamt_stempel()}; }

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
              "Flags; traegt sie Flags, MUSS 'c' darunter sein (Owner-F-10 07.08.2026) UND jedes Flag-Token "
              "muss im Katalog stehen -- unter SEINER Basis (S2-Katalog-Wache, flag_grammar_catalog.hpp)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
// SCHARFSCHALTUNG (Migrations-Naht, algo_semver.hpp Klasse (e)): der Planer-Stempel MUSS -- wie jede
// ce-Version -- die CPU-Basis tragen. Ohne diese Wache haette der Planer die Migration umgangen.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kPlannerVersion),
              "kPlannerVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version 'c' unter ihren Flags "
              "tragen (Owner-Q3 02.08.2026 / F-10 07.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif
static_assert(planner_target_isa() == std::string_view{"x86_64"});

} // namespace comdare::cache_engine::planner
