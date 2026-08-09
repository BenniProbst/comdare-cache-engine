#pragma once
// mess/genus_kaskade.hpp -- DIE DURCHREICHUNG: MK durch ALLE FUENF SCHICHTEN der Owner-Schichtung.
//
// == DIE SCHICHTUNG, IN DIE HIER GEBAUT WIRD (Owner-Praezisierung 09.08.2026) ======================
//
//     GATTUNG                Kerninterface -- alle ihre Genus teilen es
//     GENUS                  erbt es + eigene SPEZIAL-Funktionen
//        |                   >>> DAS sieht das PRUEFDOCK DER CEB <<<
//     GENUS_impl             ABSTRACT FACTORY, eigene Datei
//     je Funktion EINE       jede traegt EINEN Hauptalgorithmus
//     Klasse
//     Achsen-Interfaces      der Hauptalgorithmus stuetzt sich AUSSCHLIESSLICH auf sie,
//                            rekursiv und QUER zu anderen Gattung+Genus
//
//     Gattung + Genus  ->  EIN Genus-Binary  ->  EXAKT EIN Tier-Binary  (1:1)
//
// Gattung und Genus sind KEINE Achsen, sondern Ebenen einer metaprogrammierten abstrakten
// Klassenhierarchie. Die Achsen liegen erst UNTER der _impl.
//
// == WARUM DIE MESS-NAHT AM GENUS-INTERFACE SITZT UND NICHT TIEFER =================================
// Owner: "Der Hauptalgorithmus RECHNET NICHT -- er ORCHESTRIERT Achsen; deshalb sind die Achsen
// ueberhaupt messbar, und deshalb sitzt die Mess-Naht am Genus-Interface und nicht tiefer."
//
// Das ist keine Geschmacksfrage, sondern folgt aus dem Gegenstand: waere die Naht IN den Achsen,
// mueste jede der 18 Organ-Achsen ihre eigene Messeinrichtung tragen (18 Nahtstellen, 18 Gelegenheiten
// zur Abweichung, und jede Achse traegt den Overhead ihrer eigenen Messung IN ihrem eigenen Messwert).
// Zwischen den Achsen-Aufrufen des orchestrierenden Hauptalgorithmus dagegen liegt genau EINE Stelle
// je Achse, an der sich die Achse klammern laesst, ohne dass die Klammer in ihr eigenes Ergebnis
// faellt.
//
// == WAS DIESE DATEI BEWEIST =======================================================================
// Dass MK von der Gattung bis HINTER die Kaskade in die Genus-_impl durchkommt -- die Wurzel des
// Owner-Befunds. Die Kette ist LUECKENLOS: jede Schicht traegt MK als Template-Parameter, keine
// Schicht setzt einen Default, und die unterste (der Hauptalgorithmus) baut daraus die Naht.
//
// EIN DEFAULT WAERE DER GANZE DEFEKT: `class MK = Leer` an irgendeiner Schicht liesse jeden
// Bestands-Aufrufer weiter uebersetzen -- und zwar STILL in die unmessbare Variante. Genau so
// entsteht eine Reihe voller ehrlicher Nullen. Deshalb hat KEINE Schicht hier einen Default, und der
// Bruch beim Uebersetzen ist gewollt und laut.
//
// == SELBSTCHECK ===================================================================================
//   ZUSICHERT: die luekenlose Durchreichung (static_assert kette_geschlossen unten) und die
//              Doppelform an der Genus-Flaeche (Visitor Pflicht bei an, nicht existent bei aus).
//   ZUSICHERT NICHT: dass die Achsen-Interfaces darunter unveraendert sind -- das ist Absicht des
//              Entwurfs und wird dadurch belegt, dass diese Datei keine Achse anfasst.
//
// ASCII-only.

#include <mess/konfiguration.hpp>
#include <mess/mess_naht.hpp>

#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::mess {

// ==================================================================================================
// (1) GATTUNG -- das Kerninterface. CRTP + Concept-Guard, MK ab der WURZEL.
// ==================================================================================================
//
// Die Gattung traegt, was ALLE ihre Genus teilen. Sie kennt MK bereits -- nicht, weil sie selbst
// misst, sondern weil ihre Kernfunktionen dieselbe Doppelform brauchen wie die Spezialfunktionen des
// Genus. Haette erst der Genus MK, muesste die Gattung ihre Kernfunktionen entweder ungemessen lassen
// oder sie im Genus abschreiben -- beides ist genau der Bruch, den die Kaskade vermeiden soll.
template <class GenusT, KonfigConcept MK>
class GattungKern {
public:
    using konfiguration_t = MK;

    /// Der Tag reist ab der Wurzel mit. Er ist die Kennung, die spaeter im Modul-Deskriptor steht und
    /// die das Dock gegen seine eigene vergleicht.
    static constexpr std::uint64_t mess_tag = MK::tag();

    /// Kernfunktion aller Genus dieser Gattung -- hier: die Selbstauskunft, die das Pruefdock zuerst
    /// zieht. Sie misst nicht; sie zeigt, dass MK schon auf dieser Ebene liegt.
    [[nodiscard]] static constexpr std::size_t instrument_anzahl() noexcept { return MK::anzahl; }

protected:
    // CRTP-Abstieg, ohne virtuelle Funktion. Der Genus ist statisch bekannt.
    [[nodiscard]] GenusT&       abgeleitet() noexcept { return static_cast<GenusT&>(*this); }
    [[nodiscard]] GenusT const& abgeleitet() const noexcept { return static_cast<GenusT const&>(*this); }
};

// ==================================================================================================
// (2) GENUS -- erbt das Kerninterface, ergaenzt die SPEZIALFUNKTIONEN.
//     >>> DIESE FLAECHE SIEHT DAS PRUEFDOCK DER CEB <<<
// ==================================================================================================
//
// HIER STEHT DIE DOPPELFORM, und sie ist der Kern der ganzen Uebung:
//
//   bei aktiv<MK>   : fahren(last, MessVisitor<MK>&)   -- der Visitor ist PFLICHT. Kein Default,
//                     kein Zeiger, kein optional. Ihn wegzulassen UEBERSETZT NICHT.
//   bei !aktiv<MK>  : fahren(last)                     -- der Parameter EXISTIERT NICHT. Einen
//                     Visitor zu uebergeben UEBERSETZT NICHT (und MessVisitor<Leer> ist ohnehin
//                     nicht bildbar).
//
// Das ist die zweiseitige Aktivierung als STRUKTUR statt als Abfrage. Eine dritte Moeglichkeit --
// "Visitor optional, wird ignoriert wenn aus" -- gibt es nicht, weil sie genau die stille Degradation
// waere, die das Haus als teuerste Fehlerklasse fuehrt.
template <class ImplFabrik, KonfigConcept MK>
class GenusInterface : public GattungKern<GenusInterface<ImplFabrik, MK>, MK> {
public:
    using basis_t = GattungKern<GenusInterface<ImplFabrik, MK>, MK>;
    using impl_t  = decltype(ImplFabrik::template bauen<MK>());

    // `template <class MK2 = MK>` ist hier PFLICHT und kein Stil -- aus demselben, am Objekt
    // gemessenen Grund wie in pilot_suche_impl.hpp: `MessVisitor<MK>` im Parameter einer gewoehnlichen
    // Mitgliedsfunktion wird schon bei der Instanziierung der KLASSE gebildet und verletzt im
    // deaktivierten Bau seine eigene Bedingung -- die requires-Klausel kommt zu spaet. Nur ein
    // Parametertyp, der vom FUNKTIONS-Template abhaengt, entsteht erst beim Gebrauch.
    // `std::same_as<MK2, MK>` nagelt den Vorgabewert fest, damit niemand eine fremde Konfiguration
    // hereinreicht.

    /// SPEZIALFUNKTION des Genus, MESSENDE Form. Der Visitor ist Pflichtparameter.
    template <class Last, class MK2 = MK>
        requires(std::same_as<MK2, MK> && aktiv<MK2>)
    [[nodiscard]] auto fahren(Last const& last, MessVisitor<MK2>& visitor) const {
        auto impl = ImplFabrik::template bauen<MK2>();
        return impl.fahren_impl(last, visitor);
    }

    /// SPEZIALFUNKTION des Genus, STILLE Form. Der Visitor-Parameter existiert hier NICHT.
    template <class Last, class MK2 = MK>
        requires(std::same_as<MK2, MK> && !aktiv<MK2>)
    [[nodiscard]] auto fahren(Last const& last) const {
        auto impl = ImplFabrik::template bauen<MK2>();
        return impl.fahren_impl(last);
    }
};

// ==================================================================================================
// (3) GENUS_impl -- ABSTRACT FACTORY. Reicht MK HINTER die Kaskade.
// ==================================================================================================
//
// Owner: die variadische Variable muss "in die Genus-Implementierung HINTER die Gattung+Interface
// Kaskade GEBAUT" werden. Genau das leistet die Fabrik: sie ist die Stelle, an der aus dem
// durchgereichten Typparameter ein konkretes, monomorphisiertes Objekt wird.
//
// Als ABSTRACT FACTORY (GoF) und nicht als direkte Instanziierung, weil der Hauptalgorithmus je
// Funktion EINE eigene Klasse ist: die Fabrik ist die eine Stelle, die weiss, welche Klasse zu
// welcher Funktion gehoert. Ohne sie stuende diese Zuordnung im Genus-Interface und muesste dort fuer
// jede neue Funktion angefasst werden.
//
// Diese Vorlage ist absichtlich generisch -- die konkreten Fabriken liegen JE GENUS in EIGENEN
// DATEIEN (Owner-Schichtung: "GENUS_impl ABSTRACT FACTORY, eigene Datei"); ein Beispiel steht in
// mess/pilot_suche_impl.hpp.

// ==================================================================================================
// DIE ZUSICHERUNG: DIE KETTE IST GESCHLOSSEN
// ==================================================================================================
//
// Was hier geprueft wird, ist genau der Owner-Befund: kommt MK von der obersten Schicht bis in die
// unterste an? Die Zusicherung vergleicht den Tag, den die GATTUNG traegt, mit dem, den die
// _impl-Ebene sieht. Sind sie gleich, ist die Kette luekenlos; unterbricht eine Schicht die
// Durchreichung (etwa durch einen eigenen Default), laufen die Tags auseinander und das hier bricht.
//
// EHRLICH BENANNT: dieser static_assert kann nur pruefen, was er sieht. Er belegt die Durchreichung
// des TYPS, nicht dass unten auch gemessen WIRD -- das ist Sache der Abnahme mit dem gewuerfelten
// Koeder. Anwesenheit ist keine Aussage.
template <class GenusT>
inline constexpr bool kette_geschlossen = (GenusT::mess_tag == GenusT::konfiguration_t::tag());

} // namespace comdare::cache_engine::mess
