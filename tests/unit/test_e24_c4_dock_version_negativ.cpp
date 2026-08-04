// test_e24_c4_dock_version_negativ -- E-24 C4 (b/5): die NEGATIV-COMPILE-FIXTURE der Dock-Versionen.
//
// DIESE UEBERSETZUNGSEINHEIT MUSS BRECHEN. Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 4.1 (Orakel-Klasse I), Zeile C4:
//   "Dock-Version OHNE HW-Flag (\"v1.0.0\") = compile-hart an ENFORCE-Wache
//    (axis_variant_version_table.hpp:95-109 bzw. ce_owned_version_satisfies_cpu_enforce)"
// Und Paragraf 7, R5: "ENFORCE-Bruch spaet entdeckt -- neue Versionen ohne c-Flag brechen erst im
// Voll-Bau. GEGENMASSNAHME: ENFORCE-Preflight in V + Klasse-I-Probe in C4." Das hier IST diese Probe.
//
// SIE WIRD NIE VOM SAMMEL-BAU GEBAUT: das CMake-Ziel ist EXCLUDE_FROM_ALL und steht bewusst NICHT in
// COMDARE_TEST_TARGETS. Gebaut wird sie ausschliesslich vom ctest-Test gleichen Namens, der den Bau
// aufruft und die erwartete Diagnose per PASS_REGULAR_EXPRESSION auf den Marker
// "E24-C4-DOCK-VERSION-NEGATIV" prueft. Baut sie versehentlich DURCH, fehlt der Marker -> Test ROT.
// Muster uebernommen von test_e24_c1_organ_concept_negativ (C1, Luecke L6) -- inkl. RESOURCE_LOCK
// "comdare_nested_build", damit sich die geschachtelten Ninja-Aufrufe der Geschwister-Fixtures nicht
// gegenseitig blockieren.
//
// BEWUSST NICHT WILL_FAIL: das wuerde JEDEN Fehlschlag akzeptieren -- auch einen Tippfehler oder einen
// fehlenden Include. Gefordert ist der BESTIMMTE Bruch an der ENFORCE-Wache, nicht irgendeiner.
//
// ERWARTETE DIAGNOSEN (drei, unabhaengig voneinander -- die drei Weisen, eine Dock-Version falsch zu
// schreiben; alle drei sind am Ist real vorgekommene Fehlerbilder, siehe algo_semver.hpp:604-614):
//   E24-C4-DOCK-VERSION-NEGATIV-1  flaglose Kurzform "v1.0.0"  -> gated ENFORCE-Wache beisst.
//   E24-C4-DOCK-VERSION-NEGATIV-2  "v1.0.0e" (Pruefling-Marke) -> schon die UNGATED Politik beisst.
//   E24-C4-DOCK-VERSION-NEGATIV-3  "v1.0.0g" (fremdes HW-Flag) -> im CPU-only-Scope unzulaessig.

#include "builder/pruef_dock/pruef_dock_version.hpp"

#include <string_view>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der gleichnamigen Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation, Lint-Klasse 14398).
namespace {

namespace meas = ::comdare::cache_engine::measurement;

/// (1) Der Fehler, den der Bauplan wortwoertlich nennt: eine Dock-Version ohne Hardware-Flag. Sie ist
///     GRAMMATISCH wohlgeformt (die ungated Politik laesst sie durch) -- erst die scharfgeschaltete
///     ENFORCE-Wache weist sie ab. Genau deshalb waere sie ohne diese Fixture erst im Voll-Bau
///     aufgefallen (Risiko R5).
inline constexpr std::string_view kDockVersionOhneFlag = "v1.0.0";

/// (2) Die Pruefling-Markierung 'e' an einer ce-eigenen Version: compile-VERBOTEN (Owner-E2; 'e' ist
///     ausschliesslich der Pruefling, LEDGER:3611).
inline constexpr std::string_view kDockVersionExperimentell = "v1.0.0e";

/// (3) Ein fremdes Hardware-Flag im CPU-only-Scope (Owner-Q3: die Flotte ist CPU-only, alles endet 'c').
inline constexpr std::string_view kDockVersionFremdesFlag = "v1.0.0g";

static_assert(meas::ce_owned_version_satisfies_cpu_enforce(kDockVersionOhneFlag),
              "E24-C4-DOCK-VERSION-NEGATIV-1: eine Dock-Version OHNE Hardware-Flag muss an der "
              "ENFORCE-Wache brechen -- erwarteter Bruch");

static_assert(meas::ce_owned_version_is_wellformed(kDockVersionExperimentell),
              "E24-C4-DOCK-VERSION-NEGATIV-2: eine ce-eigene Dock-Version darf NIE die "
              "Pruefling-Markierung 'e' tragen -- erwarteter Bruch");

static_assert(meas::ce_owned_version_satisfies_cpu_enforce(kDockVersionFremdesFlag),
              "E24-C4-DOCK-VERSION-NEGATIV-3: im CPU-only-Scope ist ein fremdes Hardware-Flag "
              "unzulaessig -- erwarteter Bruch");

} // namespace

int main() {
    // Gegenprobe im selben Bau: die ECHTEN Dock-Versionen sind wohlgeformt und flag-konform. Bricht die
    // TU an DIESER Zeile statt an den dreien oben, ist der Bruch der falsche -- die Marker fehlen dann.
    static_assert(
        meas::ce_owned_version_satisfies_cpu_enforce(::comdare::cache_engine::builder::pruef_dock::kSetDockVersion));
    return 0;
}
