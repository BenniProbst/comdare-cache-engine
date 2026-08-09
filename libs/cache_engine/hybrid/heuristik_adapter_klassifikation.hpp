#pragma once
// HY-A1 -- KLASSIFIKATION der Gattung HEURISTIK-ADAPTER + die Vollstaendigkeits-Wache, die es
// bisher NICHT gab.
//
// ============================================================================================
// WARUM DIESE DATEI EXISTIERT: EIN NACHGEMESSENER NULLBEFUND
// ============================================================================================
// Das Paket-Briefing sagte, die bestehenden "Vollstaendigkeits-Tests ueber die Enums"
// (test_e24_c4_genus_pruef_docks.cpp, test_striktheit_metaprog_guard.cpp) wuerden BRECHEN, sobald
// ein Enum-Wert dazukommt -- und das sei der Beweis, dass die Wachen greifen.
//
// SIE BRECHEN NICHT. Am 09.08.2026 nachgemessen, mit Gegenprobe:
//   - AnatomyGattung::HeuristikAdapter und AnatomyGenus::FunctionInterfaceReroute angehaengt,
//   - beide Ziele neu gebaut -> 0 Fehler, 0 einschlaegige Warnungen,
//   - beide ctest -> Passed,
//   - Gegenprobe, dass nicht bloss der Cache log: die .o-Zeitstempel liegen NACH dem Header-Edit,
//     die TUs wurden also wirklich neu uebersetzt.
//
// DIE DREI GRUENDE, je am Objekt:
//   (1) test_e24_c4_genus_pruef_docks.cpp:363 haelt `constexpr std::array<AnatomyGenus, 5>
//       kAllGenera{...}` -- eine HANDGEFUEHRTE Liste IM TEST. Der Kommentar darueber behauptet
//       "VOLLSTAENDIGKEITS-WACHE ueber die ENUM-Werte, nicht ueber eine Aufzaehlung im Test".
//       Genau das Gegenteil ist der Fall; die Behauptung war nie zutreffend.
//   (2) genus_name(), gattung_of() und pruef_dock_version_for() sind switch-Anweisungen MIT
//       nachfolgendem Sammel-return. -Wswitch koennte sie fangen -- aber das Projekt uebersetzt
//       ohne -Wall und ohne -Werror (weder CMakeLists.txt noch CMakePresets.json setzen sie), die
//       Warnung entsteht also gar nicht erst, und selbst als Warnung waere sie nicht bindend.
//   (3) genus_build_admission.hpp:171 prueft `kGenusBuildSlotCounts.size() == 5` -- wieder gegen
//       ein handgefuehrtes Array, nicht gegen das Enum.
// Es gab damit repo-weit KEINE Einzelquelle "alle AnatomyGenus-Werte". Diese Datei legt sie an.
//
// ============================================================================================
// DIE ZWEI EBENEN, DIE MAN NICHT VERWECHSELN DARF (Owner-Entscheid E-1 final, 09.08.2026)
// ============================================================================================
// Der Owner hat zwei zuvor als Alternativen vorgelegte Wege KOMBINIERT. Sie widersprechen sich
// nicht, weil sie auf verschiedenen Ebenen liegen:
//
//   WEG C -- INTERFACE-Ebene: genus() einer Hybrid-Binary liefert das GEERBTE ZIEL-Genus. Der
//     Pass-through ist nach aussen transparent; wer die Binary befragt, sieht das Ziel. Die
//     Hybrid-Natur steht im STEMPEL, nicht in der genus()-Antwort.
//   WEG A -- KLASSIFIKATIONS-Ebene: eigene Enum-Werte fuer Gattung und Genus. Sie klassifizieren
//     und sortieren im Lagerbaum (Gattung -> Genus -> REST) und benennen die ART des Reroutes.
//     Owner-Begruendung woertlich: "weil sich die Art und Weise der Reroute-Genus fuer den
//     Anwendungszweck unterscheiden kann -- etwa wenn ein Graph ein anderes Reroute benoetigt als
//     SearchAlgorithm".
//
// DARAUS FOLGT DIE PARTITION, die diese Datei compile-hart macht: jeder AnatomyGenus-Wert ist
// ENTWEDER ABI-sichtbar (kann aus genus() kommen, hat ein Pruef-Dock) ODER Klassifikation (kommt
// NIE aus genus(), hat KEIN Dock). Beides zugleich geht nicht, keines von beidem auch nicht.
//
// ============================================================================================
// WAS DIESE DATEI BEWUSST NICHT TUT
// ============================================================================================
// Kein Reroute, kein Dock, kein Tier-Modul, kein Lager-Pfad. Das sind die Folgepakete HY-A2/A3.
// Hier steht ausschliesslich die Klassifikation und ihre Bewachung.
//
// @autoritaet Owner GO-3 08.08.2026 (LEDGER:2488) + Owner-Entscheid E-1 final 09.08.2026
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md

#include "anatomy/anatomy_base.hpp" // AnatomyGattung / AnatomyGenus / gattung_of / *_name

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::hybrid {

namespace cea = ::comdare::cache_engine::anatomy;

// ------------------------------------------------------------------------------------------
// (1) DIE EINZELQUELLE: alle Enum-Werte, je Ebene genau einmal aufgeschrieben
// ------------------------------------------------------------------------------------------
// Diese beiden Arrays sind ab jetzt DIE Liste. Wer ueber "alle Gattungen" oder "alle Genera"
// iterieren will, nimmt sie -- und nicht eine eigene Kopie. Die Wache unter (2) sorgt dafuer,
// dass sie nicht hinter dem Enum zurueckbleiben koennen.

/// Alle Ebene-1-Kategorien (Stand HY-A1: vier).
inline constexpr std::array<cea::AnatomyGattung, 4> kAlleGattungen{
    cea::AnatomyGattung::Map, cea::AnatomyGattung::Container, cea::AnatomyGattung::Graph,
    cea::AnatomyGattung::HeuristikAdapter};

/// Alle Ebene-2-Tier-Unterklassen (Stand HY-A1: sechs -- fuenf ABI-sichtbare + ein Reroute-Genus).
inline constexpr std::array<cea::AnatomyGenus, 6> kAlleGenera{
    cea::AnatomyGenus::SearchAlgorithm, cea::AnatomyGenus::Set,  cea::AnatomyGenus::Sequence,
    cea::AnatomyGenus::Adapter,         cea::AnatomyGenus::View, cea::AnatomyGenus::FunctionInterfaceReroute};

namespace detail {

/// Lineare constexpr-Suche -- die Listen sind einstellig lang, ein Sortier-/Hash-Aufwand waere Zierde.
template <class E, std::size_t N>
[[nodiscard]] constexpr bool enthaelt(std::array<E, N> const& liste, E const gesucht) noexcept {
    for (auto const e : liste) {
        if (e == gesucht) return true;
    }
    return false;
}

} // namespace detail

// ------------------------------------------------------------------------------------------
// (2) DIE WACHE, DIE WIRKLICH BEISST
// ------------------------------------------------------------------------------------------
// MECHANIK, und warum ausgerechnet so: beide Enums haben einen FESTGELEGTEN Grundtyp
// (std::uint8_t). Damit sind alle 256 Bitmuster gueltige Werte des Enum-Typs -- die Schleife
// unten ist wohldefiniert, kein UB. Fuer jeden dieser 256 Werte wird zweierlei verglichen:
//   BENANNT  == liefert *_name() etwas anderes als "Unknown"? (d.h. das switch kennt den Wert)
//   GELISTET == steht der Wert in der Einzelquelle aus (1)?
// Die Wache verlangt BENANNT == GELISTET fuer JEDEN der 256 Werte.
//
// WARUM DAS DIE BEIDEN REALEN DRIFT-RICHTUNGEN FAENGT -- und warum eine simple Groessen-Prueffung
// (`size() == <letzter Wert> + 1`) sie NICHT gefangen haette:
//   - Jemand haengt einen Enum-Wert an und pflegt sein *_name()-case: BENANNT=true, GELISTET=false
//     -> Bruch. (Die Groessen-Pruefung haette hier geschwiegen, sobald der neue Wert nicht mehr
//     der letzte ist -- genau die Falle, an der die bestehenden Wachen gescheitert sind.)
//   - Jemand haengt einen Wert an, vergisst das *_name()-case UND die Liste: dann bleibt der Wert
//     "Unknown", und die Namens-Totalitaets-Wache unter (3) bricht.
//   - Jemand traegt etwas in die Liste ein, das es im switch nicht gibt: GELISTET ohne BENANNT
//     -> Bruch.
// Der Preis sind 2 x 256 constexpr-Schleifendurchlaeufe je uebersetzender TU. Gemessen im Rahmen
// dieses Pakets nicht auffaellig; die Datei wird ohnehin nur von der Hybrid-Stufe und den
// Gattungs-Tests gezogen, nicht von anatomy_base.hpp selbst (dort waere sie repo-weite Last).

/// true gdw. Namens-Kenntnis und Einzelquelle fuer JEDEN der 256 Ebene-1-Werte uebereinstimmen.
[[nodiscard]] constexpr bool gattungs_liste_ist_vollstaendig() noexcept {
    for (unsigned v = 0; v < 256U; ++v) {
        auto const g        = static_cast<cea::AnatomyGattung>(static_cast<std::uint8_t>(v));
        bool const benannt  = (cea::gattung_name(g) != std::string_view{"Unknown"});
        bool const gelistet = detail::enthaelt(kAlleGattungen, g);
        if (benannt != gelistet) return false;
    }
    return true;
}

/// true gdw. Namens-Kenntnis und Einzelquelle fuer JEDEN der 256 Ebene-2-Werte uebereinstimmen.
[[nodiscard]] constexpr bool genus_liste_ist_vollstaendig() noexcept {
    for (unsigned v = 0; v < 256U; ++v) {
        auto const g        = static_cast<cea::AnatomyGenus>(static_cast<std::uint8_t>(v));
        bool const benannt  = (cea::genus_name(g) != std::string_view{"Unknown"});
        bool const gelistet = detail::enthaelt(kAlleGenera, g);
        if (benannt != gelistet) return false;
    }
    return true;
}

static_assert(gattungs_liste_ist_vollstaendig(),
              "HY-A1-WACHE: kAlleGattungen und gattung_name() sind auseinandergelaufen. Wer einen "
              "AnatomyGattung-Wert anhaengt, pflegt BEIDE Stellen -- die Liste hier und das switch "
              "in anatomy_base.hpp. Genau dieses Auseinanderlaufen blieb bis HY-A1 unbemerkt.");

static_assert(genus_liste_ist_vollstaendig(),
              "HY-A1-WACHE: kAlleGenera und genus_name() sind auseinandergelaufen. Wer einen "
              "AnatomyGenus-Wert anhaengt, pflegt BEIDE Stellen -- und entscheidet zusaetzlich, ob "
              "der neue Wert ABI-sichtbar (dann braucht er ein Pruef-Dock) oder Klassifikation ist.");

// ------------------------------------------------------------------------------------------
// (3) NAMENS-TOTALITAET: kein gelisteter Wert darf "Unknown" heissen
// ------------------------------------------------------------------------------------------
// Ohne diese zweite Wache koennte man (2) truebe stellen, indem man einen Wert WEDER benennt NOCH
// listet -- dann waere BENANNT == GELISTET == false und (2) bliebe still. Diese Wache schliesst
// das Loch von der anderen Seite: alles, was gelistet ist, MUSS einen echten Namen haben.

[[nodiscard]] constexpr bool alle_gattungen_benannt() noexcept {
    for (auto const g : kAlleGattungen) {
        if (cea::gattung_name(g) == std::string_view{"Unknown"}) return false;
    }
    return true;
}

[[nodiscard]] constexpr bool alle_genera_benannt() noexcept {
    for (auto const g : kAlleGenera) {
        if (cea::genus_name(g) == std::string_view{"Unknown"}) return false;
    }
    return true;
}

static_assert(alle_gattungen_benannt(), "HY-A1-WACHE: eine gelistete Gattung hat kein gattung_name()-case.");
static_assert(alle_genera_benannt(), "HY-A1-WACHE: ein gelistetes Genus hat kein genus_name()-case.");

// ------------------------------------------------------------------------------------------
// (4) DIE PARTITION: Klassifikation gegen ABI-Sichtbarkeit
// ------------------------------------------------------------------------------------------
// ABGELEITET aus gattung_of(), NICHT als zweite Aufzaehlung. Das ist Absicht: eine zweite Liste
// waere eine zweite Wahrheit, die driften kann -- exakt der Fehler, den (2) fuer die Namen behebt.
// Ein Reroute-Genus erkennt man daran, dass seine Gattung HeuristikAdapter ist. Wer spaeter einen
// ZWEITEN Reroute-Genus anhaengt, traegt ihn in kAlleGenera ein und bildet ihn in gattung_of() auf
// HeuristikAdapter ab -- und ist damit automatisch hier richtig einsortiert, ohne eine Zeile
// dieser Datei zu aendern. Genau das verlangt der Owner-Auftrag "die Struktur muss MEHRERE
// Reroute-Genera tragen, ein zweiter baut nichts um".

/// true gdw. G ein KLASSIFIKATIONS-Genus ist: es kommt NIE aus genus() und hat KEIN Pruef-Dock.
[[nodiscard]] constexpr bool ist_klassifikations_genus(cea::AnatomyGenus g) noexcept {
    return cea::gattung_of(g) == cea::AnatomyGattung::HeuristikAdapter;
}

/// true gdw. G ABI-sichtbar ist: es kann aus genus() kommen und traegt ein Pruef-Dock.
[[nodiscard]] constexpr bool ist_abi_sichtbares_genus(cea::AnatomyGenus g) noexcept {
    return !ist_klassifikations_genus(g);
}

/// Zahl der ABI-sichtbaren Genera -- MUSS der Zahl der Pruef-Docks entsprechen.
[[nodiscard]] constexpr std::size_t abi_sichtbare_genus_anzahl() noexcept {
    std::size_t n = 0;
    for (auto const g : kAlleGenera) {
        if (ist_abi_sichtbares_genus(g)) ++n;
    }
    return n;
}

/// Zahl der Klassifikations-Genera (Reroute-Genera). Heute genau eins; mehrere sind vorgesehen.
[[nodiscard]] constexpr std::size_t klassifikations_genus_anzahl() noexcept {
    return kAlleGenera.size() - abi_sichtbare_genus_anzahl();
}

/// kPruefDockPflichtigeGenusAnzahl -- die Zahl, an der die Dock-Registry haengt.
/// SIE IST KEIN LITERAL: sie faellt aus der Partition heraus. Kaeme ein sechstes ABI-sichtbares
/// Genus dazu, stiege sie auf 6 und die Registry-Wache im Test brechen -- was richtig ist, denn ein
/// ABI-sichtbares Genus OHNE Dock waere unmessbar.
inline constexpr std::size_t kPruefDockPflichtigeGenusAnzahl = abi_sichtbare_genus_anzahl();

static_assert(kPruefDockPflichtigeGenusAnzahl == 5,
              "HY-A1-PARTITION: es gibt genau fuenf ABI-sichtbare Genera (SearchAlgorithm/Set/"
              "Sequence/Adapter/View) und damit genau fuenf Pruef-Docks. Bricht das hier, ist ein "
              "ABI-sichtbares Genus dazugekommen -- dann fehlt ein Dock, und register_all_genus_docks "
              "muss mitgezogen werden. Ein REROUTE-Genus loest das NICHT aus (es hat kein Dock).");

static_assert(klassifikations_genus_anzahl() == 1,
              "HY-A1-PARTITION: heute gibt es genau einen Reroute-Genus (FunctionInterfaceReroute). "
              "Diese Zahl DARF steigen -- der Owner hat mehrere angekuendigt. Wer einen zweiten "
              "anhaengt, zieht diese Zahl mit; das ist eine bewusste Quittung, kein Defekt.");

// Die Hybrid-Gattung selbst traegt KEIN Pruef-Dock -- Gegenstueck zur Zahl oben, als eigene Aussage.
static_assert(ist_klassifikations_genus(cea::AnatomyGenus::FunctionInterfaceReroute),
              "HY-A1: FunctionInterfaceReroute MUSS ueber gattung_of() bei HeuristikAdapter landen.");
static_assert(ist_abi_sichtbares_genus(cea::AnatomyGenus::SearchAlgorithm) &&
                  ist_abi_sichtbares_genus(cea::AnatomyGenus::Set) &&
                  ist_abi_sichtbares_genus(cea::AnatomyGenus::Sequence) &&
                  ist_abi_sichtbares_genus(cea::AnatomyGenus::Adapter) &&
                  ist_abi_sichtbares_genus(cea::AnatomyGenus::View),
              "HY-A1: die fuenf Bestands-Genera bleiben ABI-sichtbar -- der Hybrid-Anbau darf an "
              "ihrer Sichtbarkeit nichts aendern.");

} // namespace comdare::cache_engine::hybrid
