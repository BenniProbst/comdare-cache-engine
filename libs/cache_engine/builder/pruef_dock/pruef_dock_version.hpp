#pragma once
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4:
// "Neue Dock-Versionen in Q3-Grammatik (ENFORCE=1 bricht sonst compile-hart)"; seit dem Owner-KERN vom
// 07.08.2026 in der FLAG-GRAMMATIK v2 "X.Y.Z[.flag]*", s. algo_semver.hpp) -- die
// Selbst-Versionen der Pruef-Docks.
//
// WAS DIESE DATEI IST: die EINE Stelle, an der die ce-eigenen Versions-Literale der Pruef-Docks stehen --
// je Gattung genau eines, jedes mit BEIDEN Politik-Wachen aus algo_semver.hpp. Muster 1:1 uebernommen von
// profile_facade/planner/planner_version.hpp (dem gelandeten Praezedenzfall einer ce-eigenen, NICHT-Achsen-
// Version): Roh-Literal in der Flag-Grammatik v2 (KEIN 'v'-Praefix mehr, Owner-KERN 07.08.2026), rohe und
// gerenderte Form fallen damit zusammen, ungated ce_owned_version_is_wellformed + gated
// ce_owned_version_satisfies_cpu_enforce.
//
// WARUM UEBERHAUPT EINE DOCK-VERSION (nicht Zierde): ein Pruef-Dock ist der Mess-UEBERGANG einer Gattung --
// aendert sich sein Antriebs-/Gate-/Serialisierungs-Verhalten, aendert sich die Bedeutung der erzeugten
// Mess-Zeilen, OHNE dass sich Achsen-Version, Kompositions-Name oder binary_id bewegen. Die Dock-Version ist
// die einzige Groesse, die diesen Unterschied tragen kann. Sie ist bewusst KEINE ABI-Groesse (sie reist nicht
// in den Stempel und nicht in die Wire-PODs), sondern eine Provenienz-Groesse der Builder-Seite.
//
// ENFORCE-CHECKLISTE (Bauplan Paragraf 4.6): CPU-only-Flotte -> jede ce-Version traegt die CPU-Basis 'c'
// (Owner-Q3, in der Flag-Grammatik v2 als F-10 praezisiert). Eine flaglose Form ("1.0.0") bricht hier
// compile-hart, sobald COMDARE_VERSION_HW_FLAG_ENFORCE scharf ist (Default 1) -- das ist die
// Klasse-I-Probe der C4-Zeile (Bauplan Paragraf 4.1), belegt durch die Negativ-Compile-Fixture
// tests/unit/test_e24_c4_dock_version_negativ.cpp.
// WAS MIT DER v2 ENTFALLEN IST: der frueher zweite Satz "'e' ist compile-VERBOTEN". Das Konzept
// experimental ist ersatzlos deprecated; 'e' bedeutet EFFICIENCY CORE und ist ein legitimes Flag.
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
// Die Roh-Literale. FLAG-GRAMMATIK v2: KEIN 'v'-Praefix -- rohe und gerenderte Form sind dieselbe
// Zeichenfolge. Die frueher hier stehende Owner-Q10-Regel ("das 'v' gehoert NUR ins Roh-Literal") ist
// damit gegenstandslos geworden.
// ================================================================================================

/// SearchAlgorithm-Dock: BESTAND, hier zum ersten Mal beziffert. Er bekommt 1.0.0.c und nicht etwa eine
/// hoehere Zahl -- es ist die erste Version, die er traegt, nicht die erste, die er hat.
inline constexpr std::string_view kSearchAlgorithmDockVersion = "1.0.0.c";

/// Die vier Container-Gattungs-Docks (E-24 C4 c/5). Alle starten bei 1.0.0.c.
inline constexpr std::string_view kSetDockVersion      = "1.0.0.c";
inline constexpr std::string_view kSequenceDockVersion = "1.0.0.c";
inline constexpr std::string_view kAdapterDockVersion  = "1.0.0.c";
inline constexpr std::string_view kViewDockVersion     = "1.0.0.c";

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

/// pruef_dock_version_stamp -- die Provenienz-Zeile eines Docks: "<DockName>@X.Y.Z[.flag]*".
/// FLAG-GRAMMATIK v2: rohe und gerenderte Form fallen zusammen; algo_semver_string ist fuer ein
/// wohlgeformtes Literal die Identitaet. Der Aufruf bleibt trotzdem stehen -- er erzwingt die kanonische
/// Form und faengt ein Fehl-Literal als "0.0.0", statt es roh durchzureichen.
[[nodiscard]] inline std::string pruef_dock_version_stamp(std::string_view dock_name, std::string_view raw_version) {
    std::string s{dock_name};
    s += '@';
    s += dock_version_detail::meas::algo_semver_string(raw_version);
    return s;
}

// ================================================================================================
// Die Politik-Wachen -- je Literal beide, exakt wie an planner_version.hpp:82-94
// ================================================================================================

// (1) UNGATED, immer gebaut: wohlgeformt "X.Y.Z" mit null bis n punkt-getrennten Flags; traegt die
//     Version Flags, MUSS 'c' darunter sein (Owner-F-10 07.08.2026). Die flaglose Form bliebe hier
//     grammatisch wohlgeformt -- die weist erst (2) ab.
//     FLAG-GRAMMATIK v2: der frueher mitgefuehrte Term "NIE 'e'" ist gegenstandslos -- 'e' bedeutet
//     EFFICIENCY CORE und ist ein legitimes Flag.
//     S2 (07.08.2026): dieselbe Wache prueft seit dem Katalog-Bau ZUSAETZLICH, dass jedes Flag-Token
//     existiert und unter SEINER Basis steht (flag_grammar_catalog.hpp). Die Meldungstexte unten nennen
//     beide Gruende -- ein Bruch mit nur einem genannten Grund waere ein Fehlverweis.
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSearchAlgorithmDockVersion),
              "kSearchAlgorithmDockVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n "
              "punkt-getrennten Flags; traegt sie Flags, MUSS \'c\' darunter sein (Owner-F-10) UND jedes "
              "Flag-Token muss im Katalog stehen -- unter SEINER Basis (S2, flag_grammar_catalog.hpp)");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSetDockVersion),
              "kSetDockVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n "
              "punkt-getrennten Flags; traegt sie Flags, MUSS \'c\' darunter sein (Owner-F-10) UND jedes "
              "Flag-Token muss im Katalog stehen -- unter SEINER Basis (S2, flag_grammar_catalog.hpp)");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kSequenceDockVersion),
              "kSequenceDockVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n "
              "punkt-getrennten Flags; traegt sie Flags, MUSS \'c\' darunter sein (Owner-F-10) UND jedes "
              "Flag-Token muss im Katalog stehen -- unter SEINER Basis (S2, flag_grammar_catalog.hpp)");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kAdapterDockVersion),
              "kAdapterDockVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n "
              "punkt-getrennten Flags; traegt sie Flags, MUSS \'c\' darunter sein (Owner-F-10) UND jedes "
              "Flag-Token muss im Katalog stehen -- unter SEINER Basis (S2, flag_grammar_catalog.hpp)");
static_assert(dock_version_detail::meas::ce_owned_version_is_wellformed(kViewDockVersion),
              "kViewDockVersion nicht wohlgeformt: erlaubt ist \"X.Y.Z\" mit null bis n "
              "punkt-getrennten Flags; traegt sie Flags, MUSS \'c\' darunter sein (Owner-F-10) UND jedes "
              "Flag-Token muss im Katalog stehen -- unter SEINER Basis (S2, flag_grammar_catalog.hpp)");

#if COMDARE_VERSION_HW_FLAG_ENFORCE
// (2) GATED (Default scharf): im CPU-only-Scope MUSS jede ce-Version die CPU-Basis 'c' unter ihren Flags
//     tragen. OHNE diese Wache haetten die Dock-Versionen die Migration planmaessig umgangen -- genau der
//     Fehler, den CX-W6 an den deaktivierten Achsen-Varianten gefunden hat.
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSearchAlgorithmDockVersion),
              "kSearchAlgorithmDockVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version die "
              "CPU-Basis \'c\' tragen (Owner-Q3 / F-10) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSetDockVersion),
              "kSetDockVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version die CPU-Basis "
              "\'c\' tragen (Owner-Q3 / F-10) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kSequenceDockVersion),
              "kSequenceDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version "
              "auf 'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kAdapterDockVersion),
              "kAdapterDockVersion ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede Version "
              "auf 'c' enden (Owner-Q3/E2) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
static_assert(dock_version_detail::meas::ce_owned_version_satisfies_cpu_enforce(kViewDockVersion),
              "kViewDockVersion ohne CPU-Flag: im CPU-only-Scope MUSS jede Version die CPU-Basis "
              "\'c\' tragen (Owner-Q3 / F-10) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif

// (3) RENDER-TREUE (Byte-Wache): jedes Roh-Literal parst auf {1,0,0} mit CPU-Basis und rendert VERBATIM
//     zurueck. Damit ist gepinnt, dass die gerenderte Zeile "<Dock>@1.0.0.c" lautet -- nicht "1.0.0"
//     (Flag verloren) und nicht "1.0.0c" (die alte Q3-Schreibweise).
//     FLAG-GRAMMATIK v2: der Vergleich laeuft ueber den RENDERER statt ueber ein von Hand aufgebautes
//     AlgoSemVer{...}-Aggregat -- mit der Flag-LISTE muesste ein solches Aggregat die interne
//     Knoten-Darstellung nachbauen und waere eine zweite Wahrheit ueber die Repraesentation.
static_assert(
    dock_version_detail::meas::render_algo_semver(dock_version_detail::meas::parse_algo_semver(kSetDockVersion))
        .view() == kSetDockVersion);
static_assert(dock_version_detail::meas::parse_algo_semver(kSetDockVersion).x == 1u);
static_assert(dock_version_detail::meas::parse_algo_semver(kSetDockVersion).has_top_level_flag("c"));

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
