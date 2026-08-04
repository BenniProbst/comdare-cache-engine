#pragma once
// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3,
// Luecke-L7-REST) -- die PRODUKTIVEN Cross-Genus-Kompositionen + ihre OPT-IN-Registries.
//
// LAYER (bewusst, nicht zufaellig): die Sub-Organ-BAUFORMEN liegen eine Schicht tiefer in
// anatomy/cross_genus_organ.hpp (dort ist nur die Anatomie-Ebene sichtbar). Die KOMPOSITIONEN brauchen die
// Bau-Bruecke GenusBindingTraits -- und die inkludiert ihrerseits die Anatomien. Deshalb steht dieser Teil
// hier oben, exakt so wie anatomy/container_framework.hpp seine Slot-Pins ueber dieselbe Bruecke zieht.
//
// GOLDEN-NEUTRALITAET (die tragende Entscheidung dieser Datei, mit Beleg):
// Die Bauformen sind NICHT in eine Bestands-Achsen-Registry eingehaengt. Waeren sie es, wuechse das
// kartesische Produkt der Permutation und die golden-320-Byte-Wache braeche
// (tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt, 4*4*5*4 = 320, verglichen von
// test_profile_roundtrip; permutation_axes.xml ist TABU). Die Registries unten sind deshalb EIGENE,
// OPT-IN-TopicConfigSets: wer Cross-Genus permutieren will, setzt sie ausdruecklich als Slot-Konfiguration
// in eine PermutationEngine ein. Kein Bestands-Lauf zieht sie automatisch -- das ist kein Versehen,
// sondern der einzige Weg, produktiv zu sein OHNE die eingefrorene Referenz-Menge zu bewegen.
//
// ABI-NEUTRAL (a-Teil): kein Byte an abi_adapter.hpp, an den Wire-PODs, an abi/*_decl.hpp, an
// permutation_axes.xml oder an den Stempel-/Fingerprint-Flaechen.
//
// @doku docs/architecture/20260803-e24_container_gattungs_abi_dossier.md (S12.1, Cross-Genus)

#include "genus_binding_traits.hpp" // GenusBindingTraits<G> (die Bau-Bruecke je Gattung)

#include <anatomy/cross_genus_organ.hpp> // NodeTypeFromSequence / NodeTypeFromAdapter / ... / CarriedAxis

#include <src/permutations/permutation_engine.hpp> // PermTuple (Blatt-Form der Bau-Bruecke)

#include <boost/mp11.hpp>

namespace comdare::cache_engine::builder::experiment {

namespace cea = ::comdare::cache_engine::anatomy;
namespace pe  = ::comdare::cache_engine::permutations;
namespace mp  = boost::mp11;

using SaBinding  = GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
using SetBinding = GenusBindingTraits<cea::AnatomyGenus::Set>;

// ================================================================================================
// (1) SearchAlgorithm-Kompositionen mit einem FREMDEN Genus-Organ in einem ihrer 18 Slots
// ================================================================================================
//
// Slot-Reihenfolge T0..T17 nach anatomy/composition_factory.hpp:24-44 (+ STRUKT-R ORG-18):
//   T4 = node_type | T11 = index_organization | T15 = queuing_q1 | T16 = queuing_q2 | T17 = persistence_target
// Alle nicht benannten Slots sind CarriedAxis -- GETRAGEN, nicht getrieben (R5.B-Grenze, ehrlich).
//
// WELCHE SA-SLOTS UEBERHAUPT IN FRAGE KOMMEN -- ABWEICHUNG, am Objekt erhoben (nicht geraten):
// Die SearchAlgorithm-Anatomie haelt am Ist NUR NEUN ihrer 18 Achsen als reale Organ-Member und sammelt
// genau diese neun in observe_all() ein (eigener grep 04.08. ueber search_algorithm_anatomy.hpp:
//   agg.search_algo, agg.cache_traversal, agg.mapping, agg.node_type, agg.memory_layout, agg.serialization,
//   agg.queuing_q1, agg.queuing_q2, agg.persistence_target).
// `index_organization` ist NICHT darunter. Ein Cross-Genus-Sub-Organ im SA-index_organization-Slot waere
// deshalb STUMM -- weder ueber einen Organ-Accessor treibbar noch von observe_all() eingesammelt, also
// genau der Zustand, den dieses Fenster beseitigt. Es gibt hier folglich BEWUSST KEIN
// SaCompositionWithIndexOrganizationOrgan: die Verdrahtung `index_organization <- Set` wird in der
// SET-Gattung produktiv gemacht (Abschnitt 2), wo C3 den Organ-Member samt Accessor gerade geschaffen hat.
// Die SA-Seite nachzuruesten hiesse, SearchAlgorithmAnatomy um Organ-Member zu erweitern -- das ist eine
// Aenderung am gemessenen SA-Pfad und liegt ausserhalb des C3-Schnitts (offener Punkt fuer den Lead).

/// SaCompositionWithNodeTypeOrgan<Organ> -- node_type (T4) traegt ein fremdes Genus-Organ.
template <class Organ>
using SaCompositionWithNodeTypeOrgan = SaBinding::CompositionFor<pe::PermTuple<
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, Organ, cea::CarriedAxis, cea::CarriedAxis,
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis>>;

/// SaCompositionWithQueuingOrgan<Q1, Q2> -- queuing_q1 (T15) + queuing_q2 (T16) tragen fremde Organe.
template <class Q1, class Q2>
using SaCompositionWithQueuingOrgan = SaBinding::CompositionFor<pe::PermTuple<
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
    cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, Q1, Q2, cea::CarriedAxis>>;

// ================================================================================================
// (2) Set-Komposition mit einem FREMDEN Genus-Organ -- die Container-Seite derselben Verdrahtung
// ================================================================================================
//
// Der Beweis waere halb, blieben Cross-Genus-Sub-Organe eine reine SearchAlgorithm-Angelegenheit: erst mit
// der C3-Einsammlung (SetAnatomy::observe_axes) kann auch eine CONTAINER-Gattung ein fremdes Genus-Organ
// messtechnisch tragen. Slot-Reihenfolge nach set_composition.hpp:21-33; T9 = index_organization.

/// SetCompositionWithIndexOrganizationOrgan<Organ> -- index_organization (T9) traegt ein fremdes Organ;
/// Slot 0 bleibt das echte Mengen-Kern-Organ, damit die Gattung real getrieben werden kann.
template <class Organ, class KeySet = cea::SortedArrayKeySet>
using SetCompositionWithIndexOrganizationOrgan =
    SetBinding::CompositionFor<KeySet, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
                               cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, Organ,
                               cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis>;

/// SetCompositionWithNodeTypeOrgan<Organ> -- node_type (T3) traegt ein fremdes Organ.
template <class Organ, class KeySet = cea::SortedArrayKeySet>
using SetCompositionWithNodeTypeOrgan =
    SetBinding::CompositionFor<KeySet, cea::CarriedAxis, cea::CarriedAxis, Organ, cea::CarriedAxis, cea::CarriedAxis,
                               cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
                               cea::CarriedAxis, cea::CarriedAxis>;

// ================================================================================================
// (3) OPT-IN-REGISTRIES -- als TopicConfigSet-Slots einer PermutationEngine einsetzbar
// ================================================================================================
//
// Form exakt wie die Bestands-Registries (StaticAxisVariants = mp_list<...>, z. B.
// topics/sequence/axis_growth/axis_growth_registry.hpp:25). Der UNTERSCHIED ist die Verdrahtung: diese
// hier haengen an KEINER Bestands-Achse und damit an keinem golden-Lauf (s. Kopf-Klausel).

/// Cross-Genus-Varianten fuer einen node_type-Slot: Sequence- ODER Adapter-Organ.
struct CrossGenusNodeTypeVariants {
    using StaticAxisVariants = mp::mp_list<cea::NodeTypeFromSequence, cea::NodeTypeFromAdapter>;
};

/// Cross-Genus-Varianten fuer einen index_organization-Slot: Set-Organ.
struct CrossGenusIndexOrganizationVariants {
    using StaticAxisVariants = mp::mp_list<cea::IndexOrganizationFromSet>;
};

/// Cross-Genus-Varianten fuer einen queuing-Slot: Adapter-Organ (FIFO-Substrat, Paragraf 26.4).
struct CrossGenusQueuingVariants {
    using StaticAxisVariants = mp::mp_list<cea::QueuingFromAdapter>;
};

/// kCrossGenusWiringCount -- wie viele Ziel-Slot-Verdrahtungen dieses Fenster produktiv macht (4:
/// node_type<-Sequence, node_type<-Adapter, index_organization<-Set, queuing<-Adapter). GERECHNET aus den
/// Registry-Listen, nicht als Literal gepflegt -- kommt eine Variante dazu, waechst die Zahl mit.
inline constexpr std::size_t kCrossGenusWiringCount =
    mp::mp_size<CrossGenusNodeTypeVariants::StaticAxisVariants>::value +
    mp::mp_size<CrossGenusIndexOrganizationVariants::StaticAxisVariants>::value +
    mp::mp_size<CrossGenusQueuingVariants::StaticAxisVariants>::value;

// ================================================================================================
// Selbst-Beweise (compile-time)
// ================================================================================================

// (a) Die Kompositionen sind echte Kompositionen ihrer Gattung -- die Cross-Genus-Belegung bricht die
//     Gattungs-Concepts nicht.
static_assert(cea::IsComposition<SaCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>>);
static_assert(cea::IsComposition<SaCompositionWithQueuingOrgan<cea::QueuingFromAdapter, cea::QueuingFromAdapter>>);
static_assert(cea::IsSetComposition<SetCompositionWithIndexOrganizationOrgan<cea::IndexOrganizationFromSet>>);
static_assert(cea::IsSetComposition<SetCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>>);

// (b) Der Slot traegt WIRKLICH das fremde Organ (und nicht versehentlich einen Nachbar-Slot).
static_assert(std::is_same_v<typename SaCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>::node_type,
                             cea::NodeTypeFromSequence>);
static_assert(
    std::is_same_v<typename SaCompositionWithQueuingOrgan<cea::QueuingFromAdapter, cea::QueuingFromAdapter>::queuing_q1,
                   cea::QueuingFromAdapter>);
static_assert(
    std::is_same_v<typename SetCompositionWithIndexOrganizationOrgan<cea::IndexOrganizationFromSet>::index_organization,
                   cea::IndexOrganizationFromSet>);

// (c) Der Slot-SATZ der Gattung bleibt unveraendert -- Cross-Genus fuegt keinen Slot hinzu. Die SA-Seite
//     zaehlt ueber composition_organ_count: AdHocComposition traegt KEIN slot_count-Member (am Bau literal
//     gesehen -- "'slot_count' is not a member of ... AdHocComposition<...>").
static_assert(cea::composition_organ_count<SaCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>>::value ==
              SaBinding::slot_count);
static_assert(SetCompositionWithIndexOrganizationOrgan<cea::IndexOrganizationFromSet>::slot_count ==
              SetBinding::slot_count);

// (d) Die vier geforderten Verdrahtungen sind vollzaehlig registriert.
static_assert(kCrossGenusWiringCount == 4,
              "E-24 C3 / L7: node_type<-Sequence, node_type<-Adapter, index_organization<-Set, queuing<-Adapter");

} // namespace comdare::cache_engine::builder::experiment
