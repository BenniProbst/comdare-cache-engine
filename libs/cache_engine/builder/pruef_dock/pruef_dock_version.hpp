#pragma once
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4:
// "Neue Dock-Versionen in Q3-Grammatik vX.Y.Zc (ENFORCE=1 bricht sonst compile-hart)") -- die
// Selbst-Versionen der Pruef-Docks.
//
// WAS DIESE DATEI IST: die EINE Stelle, an der die ce-eigenen Versions-Literale der Pruef-Docks stehen --
// je Gattung genau eines, jedes mit BEIDEN Politik-Wachen aus algo_semver.hpp. Muster 1:1 uebernommen von
// profile_facade/planner/planner_version.hpp (dem gelandeten Praezedenzfall einer ce-eigenen, NICHT-Achsen-
// Version): Roh-Literal mit 'v' (Owner-Q10), gerenderte Form praefixfrei, ungated
// ce_owned_version_is_wellformed + gated ce_owned_version_satisfies_cpu_enforce.
//
// WARUM UEBERHAUPT EINE DOCK-VERSION (nicht Zierde): ein Pruef-Dock ist der Mess-UEBERGANG einer Gattung --
// aendert sich sein Antriebs-/Gate-/Serialisierungs-Verhalten, aendert sich die Bedeutung der erzeugten
// Mess-Zeilen, OHNE dass sich Achsen-Version, Kompositions-Name oder binary_id bewegen. Die Dock-Version ist
// die einzige Groesse, die diesen Unterschied tragen kann. Sie ist bewusst KEINE ABI-Groesse (sie reist nicht
// in den Stempel und nicht in die Wire-PODs), sondern eine Provenienz-Groesse der Builder-Seite.
//
// ENFORCE-CHECKLISTE (Bauplan Paragraf 4.6): CPU-only-Flotte -> jede ce-Version endet auf 'c'; 'e' ist fuer
// ce-Registry-/Werkzeug-Varianten compile-VERBOTEN (Owner-Q3/E2, LEDGER:3611). Eine flaglose Kurzform
// ("v1.0.0") bricht hier compile-hart, sobald COMDARE_VERSION_HW_FLAG_ENFORCE scharf ist (Default 1,
// algo_semver.hpp:172-173) -- das ist die Klasse-I-Probe der C4-Zeile (Bauplan Paragraf 4.1), belegt durch
// die Negativ-Compile-Fixture tests/unit/test_e24_c4_dock_version_negativ.cpp.
//
// BEWUSST NICHT GETAN -- IPruefDock BLEIBT UNBERUEHRT: die Version wird NICHT als neue virtuelle Methode an
// den Dock-Vertrag gehaengt. pruef_dock.hpp steht unter dem HY-D2-Freeze (Bauplan Paragraf 4.4, Klasse IV:
// "V5-Konformitaets-Gate-VERTRAG und IPruefDock::measure-Signatur UNVERAENDERT") und in der C11-Negativ-Liste.
// Die Docks tragen ihre Version deshalb als static constexpr Member; wer sie gattungs-generisch braucht,
// nimmt pruef_dock_version_for(genus) unten. Ein virtuelles dock_version() waere eine Vertrags-Erweiterung,
// die dieser Commit nicht treffen darf.
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only; keine Beruehrung von abi_adapter.hpp, Wire-PODs,
// abi/*_decl.hpp oder den Stempel-/Fingerprint-Flaechen.

#include <anatomy/anatomy_base.hpp>                 // AnatomyGenus
#include <cache_engine/measurement/algo_semver.hpp> // ce-Politik-Wachen + render-neutraler Semver

#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

namespace dock_version_detail {
namespace meas = ::comdare::cache_engine::measurement;
} // namespace dock_version_detail

// ================================================================================================
// Die Roh-Literale (Owner-Q10: das 'v' gehoert NUR ins Roh-Literal, nie in die gerenderte Form)
// ================================================================================================

/// SearchAlgorithm-Dock: BESTAND, hier zum ersten Mal beziffert. Er bekommt 1.0.0c und nicht etwa eine
/// hoehere Zahl -- es ist die erste Version, die er traegt, nicht die erste, die er hat.
inline constexpr std::string_view kSearchAlgorithmDockVersion = "v1.0.0c";

/// Die vier Container-Gattungs-Docks (E-24 C4 c/5). Alle starten bei 1.0.0c.
inline constexpr std::string_view kSetDockVersion      = "v1.0.0c";
inline constexpr std::string_view kSequenceDockVersion = "v1.0.0c";
inline constexpr std::string_view kAdapterDockVersion  = "v1.0.0c";
inline constexpr std::string_view kViewDockVersion     = "v1.0.0c";

/// pruef_dock_version_for -- die Dock-Version einer Gattung, gattungs-generisch abrufbar (der Ersatz fuer
/// ein virtuelles dock_version(), siehe Kopf).
///
/// EBENEN-TRENNUNG, die hier zaehlt (anatomy_base.hpp:39-45 vs. :78-84): AnatomyGattung (Ebene 1) hat DREI
/// Werte, darunter Graph=2 -- AnatomyGenus (Ebene 2, die Tier-Unterklassen) hat FUENF und KEIN Graph. Ein
/// Pruef-Dock haengt an der Ebene-2-Gattung (pruef_dock.hpp:61-62 dock_genus() -> AnatomyGenus), also gibt
/// es hier nichts "fuer Graph" zu entscheiden. Der Graph-Enumerator bleibt unangetastet (E24-DOSSIER:163-164,
/// Q5: nach Abgabe); ein unbekannter Wert liefert einen LEEREN string_view statt eines erfundenen Literals.
[[nodiscard]] constexpr std::string_view pruef_dock_version_for(anatomy::AnatomyGenus g) noexcept {
    switch (g) {
        case anatomy::AnatomyGenus::SearchAlgorithm: return kSearchAlgorithmDockVersion;
        case anatomy::AnatomyGenus::Set: return kSetDockVersion;
        case anatomy::AnatomyGenus::Sequence: return kSequenceDockVersion;
        case anatomy::AnatomyGenus::Adapter: return kAdapterDockVersion;
        case anatomy::AnatomyGenus::View: return kViewDockVersion;
    }
    return std::string_view{};
}

/// pruef_dock_version_stamp -- die Provenienz-Zeile eines Docks: "<DockName>@X.Y.Zc".
/// PRAEFIXFREI (Owner-Q10): algo_semver_string schneidet das 'v' des Roh-Literals weg; das HARDWARE-FLAG
/// gehoert zur Version und BLEIBT stehen (A13-M3/C4).
[[nodiscard]] inline std::string pruef_dock_version_stamp(std::string_view dock_name, std::string_view raw_version) {
    std::string s{dock_name};
    s += '@';
    s += dock_version_detail::meas::algo_semver_string(raw_version);
    return s;
}

// ================================================================================================
// Die Politik-Wachen -- je Literal beide, exakt wie an planner_version.hpp:82-94
// ================================================================================================

// (1) UNGATED, immer gebaut: wohlgeformt "vX.Y.Z" mit optionalem 'c', NIE 'e' (die Pruefling-Markierung,
//     Owner-E2). Die flaglose Form bliebe hier grammatisch wohlgeformt -- die weist erst (2) ab.
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSearchAlgorithmDockVersion),
              "kSearchAlgorithmDockVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c', NIE 'e'");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSetDockVersion),
              "kSetDockVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c', NIE 'e'");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSequenceDockVersion),
              "kSequenceDockVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c', NIE 'e'");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kAdapterDockVersion),
              "kAdapterDockVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c', NIE 'e'");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kViewDockVersion),
              "kViewDockVersion nicht wohlgeformt: erlaubt ist \"vX.Y.Z\" mit optional 'c', NIE 'e'");

#if COMDARE_VERSION_HW_FLAG_ENFORCE
// (2) GATED (Default scharf seit A13-M3/C4): im CPU-only-Scope MUSS jede ce-Version auf 'c' enden und darf
//     nie experimentell sein. OHNE diese Wache haetten die Dock-Versionen die Migration planmaessig umgangen
//     -- genau der Fehler, den CX-W6 an den deaktivierten Achsen-Varianten gefunden hat.
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSearchAlgorithmDockVersion),
              "kSearchAlgorithmDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede "
              "Version auf 'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSetDockVersion),
              "kSetDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version auf "
              "'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSequenceDockVersion),
              "kSequenceDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version "
              "auf 'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kAdapterDockVersion),
              "kAdapterDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version "
              "auf 'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kViewDockVersion),
              "kViewDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version auf "
              "'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif

// (3) Render-Neutralitaet (Byte-Wache): jedes Roh-Literal parst auf {1,0,0} mit CPU-Flag und ohne
//     Pruefling-Markierung. Damit ist gepinnt, dass die gerenderte Zeile "<Dock>@1.0.0c" lautet -- nicht
//     "1.0.0" (Flag verloren) und nicht "v1.0.0c" (Praefix durchgereicht).
static_assert(dock_version_detail::meas::parse_algo_semver(kSetDockVersion) ==
              dock_version_detail::meas::AlgoSemVer{1, 0, 0, dock_version_detail::meas::HardwareFlag::cpu, false});

// (4) Die Gattungs-Zuordnung ist fuer ALLE FUENF Ebene-2-Gattungen belegt. Eine kuenftige sechste faellt
//     hier sofort auf (leerer Stempel), statt still eine unbezifferte Mess-Provenienz zu erzeugen.
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::SearchAlgorithm) == kSearchAlgorithmDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Set) == kSetDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Sequence) == kSequenceDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Adapter) == kAdapterDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::View) == kViewDockVersion);
static_assert(!pruef_dock_version_for(anatomy::AnatomyGenus::SearchAlgorithm).empty(),
              "jede Ebene-2-Gattung MUSS eine bezifferte Dock-Version tragen");

} // namespace comdare::cache_engine::builder::pruef_dock
