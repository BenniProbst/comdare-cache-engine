#pragma once
// E-24 / S12.1 -- C0 (M0-Vorstufe, 2026-08-04): OrganConcept -- die GEMEINSAME Vertrags-Flaeche der
// Gattungs-Anatomien. Zielbild des Fensters: ein Genus-Organ (SearchAlgorithm/Set/Sequence/Adapter/View)
// kann generisch als SUB-ORGAN in einem Anatomie-Slot einer ANDEREN Gattung gehalten werden
// (Cross-Genus-Komposition-als-Sub-Organ). Dieser Header ist der erste, ABI-neutrale Schritt dahin.
//
// SCHNITT DIESER STUFE (C0 -- ABI-NEUTRAL, bewusst klein):
//   * NUR der Vertrag + die am HEUTIGEN Ist beweisbaren Selbst-Beweise.
//   * KEINE bestehende Anatomie wird geaendert, KEINE ABI-Flaeche beruehrt (abi_adapter.hpp, Wire-PODs,
//     abi/*_decl.hpp, Stempel-/Fingerprint-Flaechen bleiben unangetastet).
//   * KEIN brechender Assert fuer die noch duennen Genus-Anatomien -- ihre Luecken stehen unten als
//     benannter LUECKEN-Abschnitt, nicht als Compile-Bruch.
//
// LAYER-SCHNITT (bewusst, nicht zufaellig): dieser Header inkludiert NUR anatomy_base.hpp. Er darf die
// fuenf Gattungs-Anatomien NICHT inkludieren -- sie sollen ihn spaeter selbst benutzen (sonst Include-
// Zyklus). Die Matrix-Beweise GEGEN die fuenf realen Anatomien liegen deshalb in der Test-TU
// tests/unit/test_e24_c0_organ_concept.cpp, die ueber die Bau-Bruecke
// builder/experiment_tree/genus_binding_traits.hpp (Spezialisierungen SearchAlgorithm/Adapter/Set/
// Sequence/View) und anatomy/container_framework.hpp (Slot-Pins 11/13/9/5) geht.
//
// MUSTER: C++23-Concept + CRTP-Wachen-Basis nach dem Repo-Goldstandard
// axes/io_dispatch/axis_io_strategy_base.hpp (protected CRTP-Ctor traegt die static_asserts).
// CT-statisch, zero-cost, dispatch-frei -- keine vtable, kein Runtime-Switch.
//
// @doku docs/architecture/20260803-e24_container_gattungs_abi_dossier.md (S12.1, Abschnitt 2.2/2.4)

#include "anatomy_base.hpp" // AnatomyGenus + AnatomyConcept (Identitaets-Flaeche jeder Anatomie)

#include <concepts>
#include <cstddef>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::anatomy {

// ============================================================================================
// Der Vertrag
// ============================================================================================

/// OrganObservable<A> -- die BEOBACHTUNGS-Flaeche eines Organs: eine const, nicht-werfende
/// observe_all()-Abnahme. Am Ist erfuellen das alle fuenf Gattungs-Anatomien
/// (SearchAlgorithmAnatomy/SetAnatomy/SequenceAnatomy/AdapterAnatomy/ViewAnatomy) -- jede mit einem
/// EIGENEN Snapshot-Typ. Der Vertrag fordert deshalb bewusst NUR die Existenz der Abnahme, NICHT einen
/// gemeinsamen Snapshot-Typ: ein gemeinsamer Typ existiert heute nicht und waere hier geraten.
///
/// noexcept ist Teil des Vertrags (nicht Zierde): die Beobachtung darf einen Messlauf nie abbrechen.
template <class A>
concept OrganObservable = requires(A const& a) {
    { a.observe_all() } noexcept;
};

/// organ_snapshot_t<A> -- der Beobachtungs-Typ, den A::observe_all() liefert (je Gattung ein anderer).
/// Zweck: generischer Code kann den Snapshot benennen, ohne ihn zu vereinheitlichen.
template <class A>
    requires OrganObservable<A>
using organ_snapshot_t = decltype(std::declval<A const&>().observe_all());

/// OrganConcept<A> -- die gemeinsame Organ-Vertrags-Flaeche = IDENTITAET (AnatomyConcept:
/// composition_t / composition_name() / paper_id() / organ_count() / genus()) + BEOBACHTUNG
/// (OrganObservable). Genau diese Schnittmenge ist am heutigen Ist ueber alle fuenf Gattungen beweisbar.
///
/// Was hier BEWUSST NICHT drin steht (und warum), siehe LUECKEN-Abschnitt am Dateiende.
template <class A>
concept OrganConcept = AnatomyConcept<A> && OrganObservable<A>;

/// GenusOrgan<A, G> -- A ist ein Organ der Tier-Unterklasse G. Pinnt die Genus-Identitaet zur
/// Compile-Zeit; ein Slot, der ein Set-Organ erwartet, kann so kein Sequence-Organ annehmen.
/// (Die Cross-Genus-JOIN-Unmoeglichkeit der ABI-Adapter bleibt davon voellig unberuehrt.)
template <class A, AnatomyGenus G>
concept GenusOrgan = OrganConcept<A> && (A::genus() == G);

// ============================================================================================
// CRTP-Wachen-Basis (Muster axis_io_strategy_base.hpp)
// ============================================================================================

/// OrganGuard<Derived> -- CRTP-Wache: wer sie erbt, wird bei der ERSTEN Konstruktion compile-hart
/// gegen OrganConcept geprueft. Zero-cost (leere Basis, protected Ctor, keine virtuelle Funktion).
///
/// C0-Stand: die fuenf Gattungs-Anatomien erben sie NOCH NICHT -- das Anheften ist eine Aenderung AN
/// den Anatomien und gehoert damit nicht in diesen ABI-neutralen Schritt. Bewiesen wird die Wache in
/// der Test-TU an einem eigenen Traeger-Typ.
template <class Derived>
class OrganGuard {
protected:
    OrganGuard() noexcept {
        static_assert(AnatomyConcept<Derived>,
                      "OrganGuard<Derived>: Derived erfuellt AnatomyConcept nicht (composition_t / "
                      "composition_name() / paper_id() / organ_count() / genus() sind Pflicht).");
        static_assert(OrganObservable<Derived>,
                      "OrganGuard<Derived>: Derived hat keine const-noexcept observe_all()-Abnahme.");
    }
};

// ============================================================================================
// Selbst-Beweise (compile-time; kein Raten, keine Fixture aus dem Anatomie-Bestand)
// ============================================================================================

namespace organ_concept_detail {

/// OrganArchetype -- der MINIMALE Modell-Typ des Vertrags. Er belegt, dass OrganConcept erfuellbar und
/// nicht ueberzogen ist (ein Concept, das niemand erfuellen kann, ist wertlos), OHNE dass dieser Header
/// eine der fuenf Anatomien inkludieren muss. Reines Beweismittel, kein Produktions-Organ.
struct ArchetypeComposition {};

struct ArchetypeSnapshot {};

struct OrganArchetype {
    using composition_t = ArchetypeComposition;

    static constexpr std::string_view composition_name() noexcept { return "OrganArchetype"; }
    static constexpr std::string_view paper_id() noexcept { return "P00 OrganArchetype (C0-Selbstbeweis)"; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::SearchAlgorithm; }
    static constexpr std::size_t      organ_count() noexcept { return 0; }

    [[nodiscard]] ArchetypeSnapshot observe_all() const noexcept { return {}; }
};

/// GuardedArchetype -- belegt, dass OrganGuard<Derived> mit einem konformen Traeger uebersetzt.
/// Der explizit defaultete Ctor ist Absicht: als Aggregat waere der protected Basis-Ctor von aussen
/// nicht erreichbar -- genau die Kapselung, die OrganGuard herstellen soll.
struct GuardedArchetype : OrganArchetype, OrganGuard<GuardedArchetype> {
    GuardedArchetype() noexcept = default;
};

} // namespace organ_concept_detail

// (a) Der Vertrag ist erfuellbar, und OrganConcept ist echt STAERKER als AnatomyConcept allein.
static_assert(AnatomyConcept<organ_concept_detail::OrganArchetype>);
static_assert(OrganObservable<organ_concept_detail::OrganArchetype>);
static_assert(OrganConcept<organ_concept_detail::OrganArchetype>);
static_assert(AnatomyConcept<organ_concept_detail::ArchetypeComposition> == false,
              "Selbstbeweis: die Identitaets-Flaeche ist eine echte Forderung, kein Freibrief");
static_assert(OrganObservable<organ_concept_detail::ArchetypeComposition> == false,
              "Selbstbeweis: die Beobachtungs-Flaeche ist eine echte Forderung");
static_assert(OrganConcept<int> == false, "Selbstbeweis: ein beliebiger Typ ist KEIN Organ");

// (b) organ_snapshot_t benennt genau den Rueckgabetyp der Abnahme (keine Vereinheitlichung).
static_assert(
    std::same_as<organ_snapshot_t<organ_concept_detail::OrganArchetype>, organ_concept_detail::ArchetypeSnapshot>);

// (c) Die Genus-Identitaet ist gepinnt: derselbe Typ ist NICHT Organ einer fremden Tier-Unterklasse.
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::SearchAlgorithm>);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Set> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Sequence> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Adapter> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::View> == false);

// (d) Die CRTP-Wache uebersetzt am konformen Traeger (Beweis, dass sie kein toter Buchstabe ist).
static_assert(OrganConcept<organ_concept_detail::GuardedArchetype>);

// ============================================================================================
// LUECKEN -- was dieser Header BEWUSST NOCH NICHT leistet (Arbeitsvorrat der naechsten Stufe C1)
// ============================================================================================
//
// Diese Liste ist am Ist erhoben (04.08.), nicht aus dem Plan abgeschrieben. Sie beschreibt WAS fehlt,
// nicht WIE es zu bauen ist -- die Loesung ist Gegenstand der naechsten Stufe, nicht dieser.
//
// L1  GEMEINSAME OP-SCHNITTSTELLE FEHLT. OrganConcept fordert heute Identitaet + Beobachtung, aber
//     KEINE Operation. Die fuenf Gattungen tragen bewusst unterschiedliche Verben (SearchAlgorithm:
//     Organ-Accessoren, der Zugriff laeuft ueber den ExecutionContext; Set: insert/contains/erase;
//     Sequence: push_back/at; Adapter: push/pop_front/pop_back/front/back/top; View: bind/read).
//     Ein gemeinsamer Op-Nenner existiert am Ist NICHT und wird hier NICHT erfunden.
//
// L2  OBSERVABLEAXIS-/STATISTICS-FORWARDING FEHLT. ObservableAxis (observer_aggregate.hpp) ist die
//     ACHSEN-Ebene (a.statistics() -> A::snapshot_t); OrganConcept ist die ORGAN-Ebene
//     (a.observe_all()). Die Verbindung -- ein Organ, das als Sub-Organ gehalten wird, muss seine
//     Beobachtung an das haltende Aggregat durchreichen, und ein Observable-Wrapper muss die
//     Concept-Member forwarden -- ist NICHT gebaut. Ohne sie ist ein Sub-Organ zwar typ-korrekt
//     einsetzbar, aber messtechnisch stumm.
//
// L3  KEIN GEMEINSAMER WERT-TYP. Am Ist: SearchAlgorithmAnatomy traegt key_type/value_type,
//     SequenceAnatomy/AdapterAnatomy/ViewAnatomy tragen element_type, SetAnatomy traegt gar keinen
//     oeffentlichen Wert-Typ (key_t/value_t sind dort privat). Ein gemeinsamer Wert-Typ-Alias ist
//     deshalb HEUTE NICHT beweisbar und ist bewusst KEIN Teil von OrganConcept. (Vermerk fuer die
//     naechste Stufe: der Bauplan-Schnitt erwartete hier eine "value_type-Teilmenge" -- die Erhebung am
//     Objekt widerlegt das fuer Set.)
//
// L4  KEIN GEMEINSAMER SNAPSHOT-TYP. observe_all() liefert je Gattung einen eigenen Typ
//     (ObserverAggregate<Composition> / SetObserverSnapshot / SequenceObserverSnapshot /
//     AdapterObserverSnapshot / ViewObserverSnapshot). organ_snapshot_t benennt ihn nur.
//
// L5  DIE FUENF ANATOMIEN ERBEN OrganGuard NOCH NICHT. Das Anheften aendert die Anatomien und ist damit
//     ausserhalb des ABI-neutralen C0-Schnitts.
//
// L6  KEINE NEGATIV-COMPILE-FIXTURE. Das Repo kennt am Ist KEIN Muster fuer erwartete Uebersetzungs-
//     Fehler (Erhebung 04.08.: 0 Treffer fuer try_compile / WILL_FAIL in tests/ und cmake/). Die
//     Negativ-Richtung ist hier deshalb ueber ERFUELLBARE Negativ-Asserts gefuehrt (Abschnitte a/c
//     oben: "Nicht-Organ erfuellt OrganConcept NICHT"). Eine echte Compile-Fehler-Probe verlangt zuerst
//     ein Repo-Muster dafuer -- diese Luecke ist hiermit DEKLARIERT, nicht stillschweigend uebergangen.
//
// L7  CROSS-GENUS-KOMPOSITION-ALS-SUB-ORGAN IST UNGEBAUT. OrganConcept ist die Voraussetzung dafuer,
//     nicht die Sache selbst: kein Anatomie-Slot nimmt heute ein fremdes Genus-Organ auf.

} // namespace comdare::cache_engine::anatomy
