#pragma once
// HY-A1 (Dock-Schicht) -- DAS HYBRID-PRUEF-DOCK der Rekursions-Ebene 3.
//
// ============================================================================================
// WELCHES DOCK HIER GEMEINT IST -- die Verwechslung kostet sonst eine Schicht
// ============================================================================================
// Die Kette traegt DREI Dock-Ebenen (soll_design Abschnitt 2, LEDGER:490 + :1692-1698):
//   Ebene 1  EXPERIMENT-DOCK   Planer  <-> CEB
//   Ebene 2  PRUEF-DOCK        CEB     <-> Tier-Binary   -- gebaut: builder/pruef_dock/IPruefDock
//   Ebene 3  HYBRID-PRUEF-DOCK Hybrid  <-> plain Tiers    -- DIESE DATEI
// (Die Vertrags-Namen sind Owner-gesetzt, NT 05.08.-mittag-3: "EXPERIMENT-DOCK (Planer<->CEB),
// PRUEF-DOCK (CEB<->Tier; im Arbeitsmodus ARBEITS-DOCK)".)
//
// StandardHybridDock ist deshalb KEINE IPruefDock-Implementierung. IPruefDock ist die
// BUILDER-seitige Ebene-2-Flaeche ("lebt nur im Builder-Binary", pruef_dock.hpp:10-14); ein
// Ebene-3-Dock lebt IN der Hybrid-Binary. Wer hier von IPruefDock erbte, zoege den Builder in die
// Hybrid-.so und entschiede damit die offene Auflage K2 stillschweigend.
//
// NACH OBEN bleibt die Hybrid-Binary trotzdem ein gewoehnliches Tier am Ebene-2-Dock: sie meldet
// aus genus() das GEERBTE ZIEL-Genus (Owner-E-1 final, Weg C -- belegt in
// test_hy_a1_heuristik_adapter_gattung.cpp, Fall "HybridMeldetDasGeerbteZielGenusNichtSichSelbst").
// Genau deshalb braucht sie dort KEIN sechstes Dock, und die Heuristik ist "SELBST ein Tier-Binary
// am selben Pruef-Dock -- kein dritter Dock-Typ" (NT 05.08.-mittag-7).
//
// ============================================================================================
// DIE KERN-ZUSAGE: DER HOT-PATH IST VARIANT-FREI UND CAST-FREI
// ============================================================================================
// Ein Dock haelt EINEN gecachten Zeiger auf das statische Antriebs-Sub-Interface seines Ziels
// (anatomy::IObservableTier*). Gesetzt wird er EINMAL, am Umschaltpunkt; gelesen wird er ohne
// visit und ohne dynamic_cast (soll_design 3.2 Punkt 2 + Paragraf-49-KORREKTUR: "die
// Haupt-Kommunikation zu den Tier-Binary-Observern bleibt statisch, zero-cost").
//
// Das ist zugleich die erste Zeile der Risiko-Tabelle des Designs (Abschnitt 9): "variant-Leak in
// den Hot-Path (visit pro Operation statt pro Umschaltpunkt)". Die Gegenmassnahme dort heisst
// woertlich "Attach-Zeit-Monomorphisierung + gecachter IObservableTier*; Konformitaets-Test in
// HY-B1 PFLICHT" -- der Test steht in test_hy_a1_dock_contract.cpp als static_assert ueber der
// Rueckgabe von antrieb().
//
// WAS DAS DOCK HEUTE NICHT TUT, ausdruecklich: es LAEDT nichts. Woher der Antrieb kommt, entscheidet
// der Binary-Proxy -- und der haelt AnatomyModuleHandle aus der Builder-Lib, was die offene Auflage
// K2 beruehrt (soll_design Abschnitt 7). Das Dock nimmt den Zeiger deshalb ENTGEGEN, statt ihn zu
// beschaffen. Die Naht ist damit an genau einer Stelle und heisst antrieb_binden().

#include "anatomy/anatomy_base.hpp"
#include "anatomy/observable_tier.hpp" // IObservableTier -- der statische Gattungs-Antrieb
#include "hybrid_dock_contract.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DER VERTRAG, den jeder Hybrid-Dock-TYP erfuellen muss
// ------------------------------------------------------------------------------------------
// Bewusst klein, aus demselben Grund wie RerouteStrategieVertrag in heuristik_adapter_strategy.hpp:
// die Reroute-Operationen existieren noch nicht, und erfundene Signaturen waeren beim ersten echten
// Reroute wieder falsch. Er ist additiv erweiterbar -- das ist der Punkt.
//
// Was er heute schon festhaelt, ist die Flaeche, die das Dock-ARRAY braucht: welchen Vertrag ein
// Dock faehrt (fuer den Slot-Abgleich), wie es heisst (fuer Berichte) und wie sein Antrieb gesetzt
// und gelesen wird (die Hot-Path-Zusage).

/// HybridDockVertrag<D> -- die Pflicht-Flaeche eines Hybrid-Pruef-Docks.
template <class D>
concept HybridDockVertrag = requires(D& d, D const& cd, anatomy::IObservableTier* p) {
    { D::contract() } -> std::convertible_to<HybridDockContract>;
    { D::dock_name() } -> std::convertible_to<std::string_view>;
    { d.antrieb_binden(p) } -> std::same_as<void>;
    { cd.antrieb() } -> std::same_as<anatomy::IObservableTier*>;
    { cd.ist_bereit() } -> std::same_as<bool>;
} && std::is_nothrow_default_constructible_v<D>;
// Der noexcept-Default-Ctor ist keine Kosmetik: das Dock-Array legt seine Slots an, BEVOR ein Ziel
// gebunden ist (die Belegung ist dynamisch, die Kapazitaet statisch -- soll_design 3.1 (c)). Ein
// werfender Default-Ctor machte schon das Anlegen des Arrays zu einem Fehlerpfad.

// ------------------------------------------------------------------------------------------
// (2) DAS EINE DOCK DES F8-MINIMAL-DoD
// ------------------------------------------------------------------------------------------

/// StandardHybridDock -- das Dock des Standard-Vertrags: reines Durchstellen an EIN Ziel.
///
/// Es ist der einzige heute gebaute Dock-Typ (soll_design K6: "genau 1 Standard-Dock"). Die drei
/// uebrigen Vertraege stehen im Angebot (hybrid_dock_contract.hpp), tragen aber noch keinen Typ --
/// die Factory meldet das benannt, statt still den Standard zu liefern.
class StandardHybridDock {
public:
    StandardHybridDock() noexcept = default;

    [[nodiscard]] static constexpr HybridDockContract contract() noexcept { return HybridDockContract::Standard; }
    [[nodiscard]] static constexpr std::string_view   dock_name() noexcept { return "StandardHybridDock"; }

    /// Der UMSCHALTPUNKT. Hier -- und nur hier -- wird der Antrieb gesetzt.
    /// nullptr ist ein zulaessiges Argument und heisst "geloest", nicht "Fehler": beim detach muss
    /// der Slot seinen Zeiger verlieren, BEVOR das Modul entladen wird (soll_design 3.3,
    /// destroy-vor-dlclose). Ein Dock, das einen Zeiger in ein entladenes Modul behielte, waere ein
    /// use-after-dlclose mit voll plausiblem Verhalten bis zum ersten Zugriff.
    void antrieb_binden(anatomy::IObservableTier* p) noexcept { antrieb_ = p; }

    /// Der OP-PFAD. Ein Zeiger-Read, kein visit, kein Cast.
    [[nodiscard]] anatomy::IObservableTier* antrieb() const noexcept { return antrieb_; }

    /// Traegt das Dock ein gebundenes Ziel? Ehrlich getrennt von "der Slot ist belegt": ein Slot
    /// kann angelegt sein und trotzdem (noch) kein Ziel tragen -- genau der Zustand zwischen
    /// attach und der Bindung durch den Proxy (HY-A2).
    [[nodiscard]] bool ist_bereit() const noexcept { return antrieb_ != nullptr; }

private:
    anatomy::IObservableTier* antrieb_ = nullptr;
};

static_assert(HybridDockVertrag<StandardHybridDock>,
              "HY-A1-DOCK: StandardHybridDock muss den HybridDockVertrag erfuellen -- sonst kann das "
              "Dock-Array ihn nicht einheitlich ansprechen.");

// HOT-PATH-ANKER, an der Quelle statt nur im Test: die Rueckgabe von antrieb() IST der statische
// Antrieb. Wird daraus je ein variant, ein optional oder ein Wrapper, laeuft die Aufloesung im
// Op-Pfad statt am Umschaltpunkt -- das ist die Risiko-Zeile 1 der soll_design-Tabelle Abschnitt 9.
static_assert(std::is_same_v<decltype(std::declval<StandardHybridDock const&>().antrieb()), anatomy::IObservableTier*>,
              "HY-A1-HOTPATH: antrieb() liefert den STATISCHEN IObservableTier-Zeiger, nichts anderes.");

// GEGENPROBE, damit der Vertrag oben nicht auf immer-wahr degeneriert: ein Typ, dem genau eine
// Vertrags-Flaeche fehlt, erfuellt ihn NICHT. Ohne diese Zeile koennte jemand HybridDockVertrag
// entkernen, ohne dass eine einzige der Zusicherungen bricht.
namespace detail {
/// NUR fuer die Gegenprobe: ein Dock-Attrappe ohne antrieb().
struct DockAttrappeOhneAntrieb {
    [[nodiscard]] static constexpr HybridDockContract contract() noexcept { return HybridDockContract::Standard; }
    [[nodiscard]] static constexpr std::string_view   dock_name() noexcept { return "DockAttrappeOhneAntrieb"; }
    void                                             antrieb_binden(anatomy::IObservableTier*) noexcept {}
    [[nodiscard]] bool                               ist_bereit() const noexcept { return false; }
};
} // namespace detail
static_assert(!HybridDockVertrag<detail::DockAttrappeOhneAntrieb>,
              "HY-A1-DOCK: der Dock-Vertrag unterscheidet nicht mehr -- er ist auf immer-true "
              "degeneriert und wuerde ein Dock ohne antrieb() durchlassen.");

} // namespace comdare::cache_engine::hybrid
