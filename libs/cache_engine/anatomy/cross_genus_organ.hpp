#pragma once
// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3,
// Luecke-L7-REST) -- CROSS-GENUS-KOMPOSITION-ALS-SUB-ORGAN: die produktiven Bauformen.
//
// WAS HIER STEHT (und was NICHT): C1 hat den MECHANISMUS gebaut -- ObservableOrgan<Organ> uebersetzt die
// ORGAN-Ebene (observe_all) in die ACHSEN-Ebene (statistics/snapshot_t) und forwardet die Concept-Member
// (organ_concept.hpp). C3 hat die vier Container-Anatomien so vertieft, dass sie diese Beobachtung auch
// EINSAMMELN (observe_axes). Was fehlte, war der PRODUKTIVE Einbau: benannte, wiederverwendbare
// Sub-Organ-Bauformen statt je Test neu zusammengesteckter Typen. Genau die stehen hier.
//
// DIE VIER VOM FENSTER GEFORDERTEN VERDRAHTUNGEN (E24-DOSSIER:160 i.V.m. Bauplan 1.1-1.3):
//   node_type          <- Sequence   (Reptil-Organ als Knoten-Substrat)
//   node_type          <- Adapter    (Wirbellosen-Organ als Knoten-Substrat)
//   index_organization <- Set        (Vogel-Organ als Index-Organisation)
//   queuing            <- Adapter    (Wirbellosen-Organ als Puffer-Disziplin)
//
// WAS DAVON UNBERUEHRT BLEIBT (ausdruecklich): die Cross-Genus-JOIN-Unmoeglichkeit der ABI-Adapter
// (set_abi_adapter.hpp:18-21 "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich"). Sie sagt,
// dass ein SET-ABI-Adapter keine fremde Gattungs-Anatomie treiben darf -- eine Aussage ueber die
// ABI-GRENZE. Hier passiert etwas anderes: ein Genus-Organ wird als SUB-ORGAN in einem ACHSEN-SLOT einer
// anderen Gattung gehalten, diesseits jeder ABI-Grenze. Beide Saetze gelten gleichzeitig; die Test-TU
// pinnt das ausdruecklich, damit die Unterscheidung nicht spaeter verwischt.
//
// GOLDEN-/ABI-NEUTRAL (a-Teil): dieser Header FUEGT NICHTS in eine bestehende Achsen-Registry ein. Waere
// eine dieser Bauformen als zusaetzliche StaticAxisVariants-Variante in eine der Bestands-Registries
// gehaengt, waechst das kartesische Produkt der Permutation -- und damit braeche die golden-320-Byte-Wache
// (tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt, 4*4*5*4, verglichen von
// test_profile_roundtrip). Die OPT-IN-Registries liegen deshalb getrennt in
// builder/experiment_tree/cross_genus_composition.hpp und sind NICHT in permutation_axes.xml verdrahtet.
// Kein Byte an abi_adapter.hpp, an den Wire-PODs, an abi/*_decl.hpp oder an den Stempel-Flaechen.
//
// @doku docs/architecture/20260803-e24_container_gattungs_abi_dossier.md (S12.1, Cross-Genus)

#include "adapter_anatomy.hpp"   // AdapterAnatomy / AdapterComposition / DequeInner
#include "organ_concept.hpp"     // ObservableOrgan / OrganConcept / GenusOrgan
#include "sequence_anatomy.hpp"  // SequenceAnatomy / SequenceComposition / DoublingGrowth
#include "set_anatomy.hpp"       // SetAnatomy / SetComposition
#include "set_default_organ.hpp" // SortedArrayKeySet (das Default-Kern-Organ der Set-Gattung)

#include <type_traits>
#include <utility>

namespace comdare::cache_engine::anatomy {

/// CarriedAxis -- die explizite "GETRAGEN, nicht getrieben"-Belegung eines Achsen-Slots.
///
/// Warum es diesen Typ gibt: ein Sub-Organ treibt genau die eine Achse, fuer die es eingesetzt wird
/// (Sequence: growth_policy + V-Speicher; Adapter: inner_container; Set: search_algo). Die uebrigen Slots
/// seiner Komposition sind Kompositions-IDENTITAET, kein Antrieb -- exakt die R5.B-Grenze, die die
/// Anatomien seit L-76a-c in ihren Koepfen fuehren ("die geteilten werden getragen"). Bisher wurde dafuer
/// je Test ein lokales `DelegatedAxis{}` erfunden; hier bekommt die Rolle EINEN produktiven Namen.
///
/// Bewusst LEER: ein getragener Slot hat keinen Zustand und keine statistics() -- er erscheint in der
/// per-Achsen-Sicht als EmptyAxisSnapshot, was der Wahrheit entspricht.
struct CarriedAxis {};

// ================================================================================================
// Die drei Sub-Organ-BAUFORMEN (je Quell-Gattung eine), fertig als ObservableOrgan-Huelle verpackt
// ================================================================================================

/// SequenceSubOrgan<Growth> -- ein SEQUENCE-Organ als Sub-Organ. Getrieben ist die growth_policy-Achse
/// (sie steuert ueber next_capacity das Kapazitaets-Wachstum des V-Speichers, sequence_anatomy.hpp).
template <class Growth = DoublingGrowth>
using SequenceSubOrgan =
    ObservableOrgan<SequenceAnatomy<SequenceComposition<CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis,
                                                        CarriedAxis, CarriedAxis, CarriedAxis, Growth>>>;

/// AdapterSubOrgan<Inner> -- ein ADAPTER-Organ als Sub-Organ. Getrieben ist die inner_container-Achse;
/// die Disziplin (FIFO/LIFO/Priority) bleibt API-NUTZUNG des Halters, keine Achse (Paragraf 26.4).
template <class Inner = DequeInner<>>
using AdapterSubOrgan = ObservableOrgan<
    AdapterAnatomy<AdapterComposition<CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis,
                                      CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, Inner>>>;

/// SetSubOrgan<KeySet> -- ein SET-Organ als Sub-Organ. Getrieben ist das search_algo-Kern-Organ, das die
/// Set-Anatomie als MENGE (K=V) faehrt.
template <class KeySet = SortedArrayKeySet>
using SetSubOrgan = ObservableOrgan<
    SetAnatomy<SetComposition<KeySet, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis,
                              CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis>>>;

// ================================================================================================
// Die VIER benannten Ziel-Slot-Verdrahtungen (Namensform: <Ziel-Slot>From<Quell-Gattung>)
// ================================================================================================

/// node_type <- Sequence: der Knoten-Slot traegt ein Reptil-Organ (wachsendes V-Array).
using NodeTypeFromSequence = SequenceSubOrgan<>;

/// node_type <- Adapter: der Knoten-Slot traegt ein Wirbellosen-Organ. Substrat ist der
/// Paragraf-26.4-Default DequeInner (beide Enden O(1)).
using NodeTypeFromAdapter = AdapterSubOrgan<>;

/// index_organization <- Set: die Index-Organisations-Achse traegt ein Vogel-Organ (K-only-Menge).
using IndexOrganizationFromSet = SetSubOrgan<>;

/// queuing <- Adapter: die Puffer-Achse traegt ein Wirbellosen-Organ. Substrat ist DequeInner -- das ist
/// KEINE Setzung dieses Headers, sondern der in adapter_anatomy.hpp:33-34 festgehaltene Paragraf-26.4-Satz
/// "stack/queue -> deque" (FIFO nutzt front + pop_front).
///
/// TYP-IDENTITAET (benannt, nicht versteckt): QueuingFromAdapter und NodeTypeFromAdapter sind DERSELBE Typ.
/// Die zwei Namen benennen die zwei vom Fenster geforderten ZIEL-SLOTS, nicht zwei verschiedene Organe --
/// die Adapter-Sub-Organ-Bauform ist dieselbe. Wer ein anderes Inner-Substrat braucht, instanziiert
/// AdapterSubOrgan<Inner> direkt.
using QueuingFromAdapter = AdapterSubOrgan<>;

// ================================================================================================
// Selbst-Beweise (compile-time): jede Bauform traegt BEIDE Ebenen und behaelt ihre Genus-Identitaet
// ================================================================================================

// (a) Jede Bauform ist selbst ein Organ (der Wrapper forwardet die Concept-Member, C1-Vertrag) ...
static_assert(OrganConcept<NodeTypeFromSequence>);
static_assert(OrganConcept<NodeTypeFromAdapter>);
static_assert(OrganConcept<IndexOrganizationFromSet>);

// (b) ... und behaelt die Genus-Identitaet ihrer QUELL-Gattung, auch im fremden Slot.
static_assert(GenusOrgan<NodeTypeFromSequence, AnatomyGenus::Sequence>);
static_assert(GenusOrgan<NodeTypeFromAdapter, AnatomyGenus::Adapter>);
static_assert(GenusOrgan<IndexOrganizationFromSet, AnatomyGenus::Set>);
static_assert(GenusOrgan<NodeTypeFromSequence, AnatomyGenus::SearchAlgorithm> == false,
              "ein Sequence-Sub-Organ wird durch den Einsatz in einem SA-Slot NICHT zum SA-Organ");

// (c) Die Op-Verben laufen ueber .organ() -- die Huelle forwardet sie bewusst nicht (C1-Grenze, gepinnt).
//     organ() liefert eine REFERENZ; die Faehigkeits-Concepts sind auf den Wert-Typ formuliert, deshalb
//     remove_reference_t (ohne das schlaegt die Probe fehl -- am Bau literal gesehen).
static_assert(OrganOpSurface<NodeTypeFromSequence> == false);
static_assert(IndexedOrganOps<std::remove_reference_t<decltype(std::declval<NodeTypeFromSequence&>().organ())>>);
static_assert(AdaptedOrganOps<std::remove_reference_t<decltype(std::declval<NodeTypeFromAdapter&>().organ())>>);
static_assert(KeyedOrganOps<std::remove_reference_t<decltype(std::declval<IndexOrganizationFromSet&>().organ())>>);

// (d) Ein getragener Slot ist KEIN Organ -- CarriedAxis ist eine Achsen-Belegung, keine Anatomie.
static_assert(OrganConcept<CarriedAxis> == false, "CarriedAxis ist eine getragene Achse, kein Organ");

// (e) Zero-cost: die Huelle kostet kein Byte gegenueber dem gewrappten Organ (EBO, C1-Zusage).
static_assert(
    sizeof(NodeTypeFromSequence) ==
    sizeof(SequenceAnatomy<SequenceComposition<CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis, CarriedAxis,
                                               CarriedAxis, CarriedAxis, CarriedAxis, DoublingGrowth>>));

} // namespace comdare::cache_engine::anatomy
