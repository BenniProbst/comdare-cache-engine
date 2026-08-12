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
#include <cache_engine/abi/stempel_basis.hpp>       // S-1: ist_versions_literal_baustein (Wert-Baustein)
#include <cache_engine/measurement/algo_semver.hpp> // ce-Politik-Wachen + render-neutraler Semver

#include <array>
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
/// Werte, darunter Graph=2 -- AnatomyGenus (Ebene 2) hat seit HY-A1 SECHS Werte, davon FUENF ABI-sichtbare
/// und KEIN Graph. (Bis 09.08.2026 stand hier "hat FUENF"; das war ab HY-A1 falsch und hat genau den
/// Defekt gedeckt, den -Wswitch dann meldete.) Ein
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
        // HY-A1 (09.08.2026): FunctionInterfaceReroute ist ein KLASSIFIKATIONS-Genus (Gattung
        // HeuristikAdapter) und hat per Owner-Entscheid E-1 "Weg C" KEIN eigenes Pruef-Dock -- seine
        // Binaries melden ueber genus() ihr ZIEL-Genus und docken an DESSEN Dock an. Der leere
        // string_view ist hier also die RICHTIGE Antwort und kein Ausfall.
        //
        // WARUM DER FALL TROTZDEM AUSGESCHRIEBEN WIRD, statt ihn dem return unten zu ueberlassen:
        // ein ausgeschriebener case haelt -Wswitch scharf. Faellt spaeter ein SECHSTER ABI-sichtbarer
        // Wert dazu, meldet der Uebersetzer ihn -- ein stiller Durchfall auf das return unten wuerde
        // ihn mit einem LEEREN Versionsstempel quittieren, und genau das war der Defekt vom 09.08.:
        // pruef_dock_version_stamp() stempelte algo_semver_string("") statt einer Version.
        case anatomy::AnatomyGenus::FunctionInterfaceReroute: return std::string_view{};
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

// (4) Die Gattungs-Zuordnung ist fuer die FUENF ANDOCKENDEN Ebene-2-Genera belegt.
//
//     NACHGEZOGEN 09.08.2026 (Warnungs-Runde 1, Klasse MUTANT). Hier stand: "ist fuer ALLE FUENF
//     Ebene-2-Gattungen belegt. Eine kuenftige sechste faellt hier sofort auf (leerer Stempel)". Beide
//     Saetze waren zum Zeitpunkt des Lesens schon falsch: die Anatomie fuehrt seit HY-A1 SECHS Genera,
//     die sechste WAR gekommen -- und sie ist hier NICHT aufgefallen. Gefangen hat sie -Wswitch, nicht
//     diese Wache. Der Grund: der !empty()-Test unten prueft nur SearchAlgorithm, obwohl seine
//     Botschaft "jede Ebene-2-Gattung" sagt. Eine Wache, die eine Aussage ueber FUENF trifft und einen
//     einzigen Wert ansieht, ist blind -- genau die Klasse Defekt, um die es in dieser Runde geht.
//
//     Beide Richtungen stehen jetzt da, denn nur zusammen sind sie eine Partition:
//       HIN  -- die fuenf andockenden Genera tragen eine BEZIFFERTE Version (kein leerer Stempel).
//       RUECK-- FunctionInterfaceReroute bleibt LEER. Ohne diese Zeile koennte jemand dort ein Literal
//               eintragen, ohne dass etwas klappert -- und das bricht Owner-Entscheid E-1 "Weg C":
//               eine Hybrid-Binary meldet ueber genus() ihr ZIEL-Genus und dockt an DESSEN Dock an.
//               Ein eigener Stempel wuerde sie zu einer siebten Dock-Familie machen.
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::SearchAlgorithm) == kSearchAlgorithmDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Set) == kSetDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Sequence) == kSequenceDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::Adapter) == kAdapterDockVersion);
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::View) == kViewDockVersion);
static_assert(!pruef_dock_version_for(anatomy::AnatomyGenus::SearchAlgorithm).empty() &&
                  !pruef_dock_version_for(anatomy::AnatomyGenus::Set).empty() &&
                  !pruef_dock_version_for(anatomy::AnatomyGenus::Sequence).empty() &&
                  !pruef_dock_version_for(anatomy::AnatomyGenus::Adapter).empty() &&
                  !pruef_dock_version_for(anatomy::AnatomyGenus::View).empty(),
              "jedes ANDOCKENDE Ebene-2-Genus MUSS eine bezifferte Dock-Version tragen -- und diese "
              "Zusage nennt jetzt alle fuenf, statt sie an einem einzigen Wert zu behaupten");
static_assert(pruef_dock_version_for(anatomy::AnatomyGenus::FunctionInterfaceReroute).empty(),
              "E-1 'Weg C': FunctionInterfaceReroute ist ein KLASSIFIKATIONS-Genus und hat KEIN eigenes "
              "Pruef-Dock. Wer hier eine Version eintraegt, macht die Hybrid-Gattung zu einer eigenen "
              "Dock-Familie und bricht den transparenten Pass-through -- der leere Stempel ist Absicht.");

// ================================================================================================
// (5) S-1 (P6): die fuenf Dock-Literale als VERSIONS-LITERAL-BAUSTEINE der Stempel-Strecke
// ================================================================================================
// Die Literale sind WERTE, keine Typen -- ihre Baustein-Anbindung laeuft deshalb ueber das
// consteval-Praedikat abi::ist_versions_literal_baustein (dieselbe EINE ce-Politik wie die Wachen (1)
// und (2) oben; beide BLEIBEN vollstaendig stehen -- die Anbindung verschaerft, sie ersetzt nichts).
//
// AUSDRUECKLICH NICHT S-1: die Genus-ZUSAMMENSETZUNG (SOLL "Genus ZUSAMMENGESETZT" der
// Versionierungs-Uebersicht) ist ein EIGENER Posten -- diese Liste beziffert die fuenf Docks einzeln
// und setzt nichts zusammen.
//
// KON47-02-ANKER: der angeschlossene()-DATENBESTAND lebt am Dock (Init-Cache der angeschlossenen
// Stempel; Hybrid-RT-Hook der Stempel-Basis). S-1 definiert dafuer NUR den Vertrag
// (abi::AngeschlosseneVertrag, stempel_basis.hpp); Datenbestand und Bau sind HY-A2/W-D.
inline constexpr std::array<std::string_view, 5> kPruefDockVersionsLiteralBausteine{
    kSearchAlgorithmDockVersion, kSetDockVersion, kSequenceDockVersion, kAdapterDockVersion, kViewDockVersion};

/// NENNER-Wache: GENAU die fuenf andockenden Genus-Docks (KON6-03). Ein sechstes Literal gehoert erst
/// dann in diese Liste, wenn sein Genus WIRKLICH andockt -- und dann bricht der Nenner hier und im
/// Vertragstest (fremdquelliger ASSERT ==5) LAUT, statt still mitzulaufen.
static_assert(kPruefDockVersionsLiteralBausteine.size() == 5,
              "S-1/P6: der Dock-Nenner ist FUENF (die fuenf andockenden Ebene-2-Genera, KON6-03) -- wer "
              "ihn bewegt, zieht den Vertragstest test_s1_stempel_basis_vertrag im selben Commit nach.");

/// Jedes Dock-Literal ist ein wohlgeformter Versions-Baustein (ungated Politik + gated CPU-Basis --
/// exakt die Wachen (1)/(2), hier als EIN Praedikat der Stempel-Basis gebuendelt).
static_assert(
    [] {
        for (std::string_view const v : kPruefDockVersionsLiteralBausteine)
            if (!::comdare::cache_engine::abi::ist_versions_literal_baustein(v)) return false;
        return true;
    }(),
    "S-1/P6: ein Dock-Literal besteht die ce-Versions-Politik nicht (ist_versions_literal_baustein, "
    "stempel_basis.hpp) -- zulaessig ist X.Y.Z[.flag]* mit Katalog-Flags und (scharf) der CPU-Basis 'c'.");

} // namespace comdare::cache_engine::builder::pruef_dock
