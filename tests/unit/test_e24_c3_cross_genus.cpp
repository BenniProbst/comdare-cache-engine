// test_e24_c3_cross_genus -- E-24 / S12.1, Stufe C3 (produktiver Cross-Genus-Einbau, Luecke-L7-REST, 2026-08-04).
//
// GEGENSTAND (Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3 +
// Gate-Klasse III/G4 Paragraf 4.3): die VIER vom Fenster geforderten Verdrahtungen, real getrieben und mit
// echten Observer-Werten belegt -- nicht als Typ-Spielerei, sondern bis in das haltende Aggregat hinein:
//   (1) node_type          <- Sequence   -> SearchAlgorithm-Aggregat traegt echte Sequence-Werte
//   (2) node_type          <- Adapter    -> SearchAlgorithm-Aggregat traegt echte Adapter-Werte
//   (3) queuing            <- Adapter    -> SearchAlgorithm-Aggregat traegt echte Adapter-Werte (q1 UND q2)
//   (4) index_organization <- Set        -> SET-Aggregat traegt echte Set-Werte
//
// ZU (4) -- ABWEICHUNG, am Objekt erhoben und hier gepinnt: die Verdrahtung laeuft NICHT ueber die
// SearchAlgorithm-Gattung, weil deren Anatomie am Ist nur NEUN ihrer 18 Achsen als Organ-Member haelt und
// einsammelt -- index_organization ist NICHT darunter (eigener grep ueber search_algorithm_anatomy.hpp,
// Begruendung in builder/experiment_tree/cross_genus_composition.hpp). Ein SA-Sub-Organ in diesem Slot
// waere stumm. Sie laeuft deshalb ueber die SET-Gattung, wo C3 den Organ-Member gerade geschaffen hat.
// Der untenstehende Test (C) pinnt diesen Ist-Zustand, damit er nicht stillschweigend verschwindet.
//
// DIE C3-EIGENE POINTE (und der Grund, warum dieser Test erst JETZT moeglich ist): dieselbe Verdrahtung
// wird zusaetzlich in einer CONTAINER-Gattung gefahren (Set-Komposition mit fremdem Genus-Organ). Vor C3
// waere sie dort messtechnisch STUMM gewesen -- die Container-Anatomien hatten kein per-Achsen-Aggregat,
// das haette einsammeln koennen (Luecke L2). Damit schliesst diese TU L2 und L7 an derselben Stelle.
//
// WAS AUSDRUECKLICH WEITER GILT (Abgrenzung, damit sie nicht verwischt): die Cross-Genus-JOIN-
// Unmoeglichkeit der ABI-Adapter (set_abi_adapter.hpp:18-21) bleibt UNVERAENDERT bestehen. Sie betrifft die
// ABI-GRENZE (ein Set-ABI-Adapter darf keine fremde Gattungs-Anatomie treiben); hier passiert das andere:
// ein Genus-Organ als SUB-ORGAN in einem ACHSEN-SLOT, diesseits jeder ABI-Grenze. Beides wird unten gepinnt.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "builder/experiment_tree/cross_genus_composition.hpp"

#include "anatomy/observer_aggregate.hpp" // ObserverAggregate / ObservableAxis (die ACHSEN-Ebene)
#include "anatomy/set_abi_adapter.hpp"    // die JOIN-Unmoeglichkeits-Wache (bleibt bestehen)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der
// gleichnamigen Fixture-Typen ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace ex  = comdare::cache_engine::builder::experiment;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

// -- Die vier produktiven Kompositionen + ihre Anatomien --------------------------------------
using SaBinding  = ex::GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
using SetBinding = ex::GenusBindingTraits<cea::AnatomyGenus::Set>;

using SaNodeSeqComp = ex::SaCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>;
using SaNodeAdaComp = ex::SaCompositionWithNodeTypeOrgan<cea::NodeTypeFromAdapter>;
using SaQueuingComp = ex::SaCompositionWithQueuingOrgan<cea::QueuingFromAdapter, cea::QueuingFromAdapter>;

using SaNodeSeqOrgan = SaBinding::AnatomyFor<SaNodeSeqComp>;
using SaNodeAdaOrgan = SaBinding::AnatomyFor<SaNodeAdaComp>;
using SaQueuingOrgan = SaBinding::AnatomyFor<SaQueuingComp>;

// Die Container-Seite derselben Verdrahtung (erst durch C3 messbar).
using SetIndexSetComp  = ex::SetCompositionWithIndexOrganizationOrgan<cea::IndexOrganizationFromSet>;
using SetIndexSetOrgan = SetBinding::AnatomyFor<SetIndexSetComp>;
using SetNodeSeqComp   = ex::SetCompositionWithNodeTypeOrgan<cea::NodeTypeFromSequence>;
using SetNodeSeqOrgan  = SetBinding::AnatomyFor<SetNodeSeqComp>;

// =============================================================================================
// (A) DIE BRUECKE TRAEGT: jedes Sub-Organ erfuellt das ECHTE ObservableAxis-Concept der Achsen-Ebene
// =============================================================================================
static_assert(cea::ObservableAxis<cea::NodeTypeFromSequence>);
static_assert(cea::ObservableAxis<cea::NodeTypeFromAdapter>);
static_assert(cea::ObservableAxis<cea::IndexOrganizationFromSet>);
static_assert(cea::ObservableAxis<cea::QueuingFromAdapter>);
static_assert(!cea::ObservableAxis<cea::CarriedAxis>, "ein getragener Slot ist stumm -- und soll es sein");

// Der Snapshot-Typ im haltenden Aggregat ist der Snapshot der QUELL-Gattung (keine Vereinheitlichung).
static_assert(
    std::is_same_v<decltype(cea::ObserverAggregate<SaNodeSeqComp>::node_type), cea::SequenceObserverSnapshot>);
static_assert(std::is_same_v<decltype(cea::ObserverAggregate<SaNodeAdaComp>::node_type), cea::AdapterObserverSnapshot>);
static_assert(
    std::is_same_v<decltype(cea::ObserverAggregate<SaQueuingComp>::queuing_q1), cea::AdapterObserverSnapshot>);
// ... und in der CONTAINER-Gattung ebenso (das ist der C3-Anteil).
static_assert(
    std::is_same_v<decltype(cea::SetAxisObservation<SetIndexSetComp>::index_organization), cea::SetObserverSnapshot>);
static_assert(
    std::is_same_v<decltype(cea::SetAxisObservation<SetNodeSeqComp>::node_type), cea::SequenceObserverSnapshot>);

// =============================================================================================
// (B) ZAEHLUNG: genau die belegten Slots sind beobachtbar, kein Slot mehr, kein Slot weniger
// =============================================================================================
static_assert(SaNodeSeqOrgan::observable_axis_count() == 1);
static_assert(SaNodeAdaOrgan::observable_axis_count() == 1);
static_assert(SaQueuingOrgan::observable_axis_count() == 2, "q1 UND q2 belegt");
static_assert(SetIndexSetOrgan::observable_axis_count() == 1);

// =============================================================================================
// (C) DIE ABGRENZUNG BLEIBT: JOIN-Unmoeglichkeit der ABI-Adapter ist UNBERUEHRT
// =============================================================================================
// Der Set-ABI-Adapter nimmt eine SET-Anatomie -- auch dann, wenn diese ein fremdes Genus-Organ in einem
// ihrer Slots haelt. Was er NICHT nimmt, ist eine fremde GATTUNGS-Anatomie. Genau diese Grenze pinnen die
// beiden folgenden Zeilen: das eine geht, das andere ist type-system-mathematisch unmoeglich.
static_assert(SetIndexSetOrgan::genus() == cea::AnatomyGenus::Set,
              "ein fremdes Sub-Organ aendert die Gattung des HALTERS nicht");
static_assert(std::is_constructible_v<cea::SetAbiAdapter<SetIndexSetOrgan>>,
              "der Set-ABI-Adapter treibt eine Set-Anatomie MIT Cross-Genus-Sub-Organ unveraendert");
static_assert(cea::NodeTypeFromSequence::genus() == cea::AnatomyGenus::Sequence,
              "das Sub-Organ behaelt seine eigene Gattungs-Identitaet im fremden Slot");

// (C.2) IST-PIN der SA-Organ-Reichweite (der Grund, warum index_organization<-Set ueber die Set-Gattung
//       laeuft, s. Kopf): die SearchAlgorithm-Anatomie hat KEINEN index_organization-Organ-Accessor, wohl
//       aber node_type und queuing. Waechst die SA-Anatomie spaeter um diesen Member, bricht diese Zeile --
//       und genau dann gehoert die SA-Variante der Verdrahtung nachgezogen. Kein stiller Rest.
//       (Die Proben laufen ueber Concepts statt ueber ein direktes requires-Literal: eine
//       requires-Expression mit NICHT-abhaengigem Ausdruck diagnostiziert GCC hart, statt sie zu false
//       auszuwerten -- am Bau literal gesehen.)
template <class A>
concept HasIndexOrganizationOrgan = requires(A& a) { a.index_organization_organ(); };
template <class A>
concept HasNodeTypeOrgan = requires(A& a) { a.node_type_organ(); };
template <class A>
concept HasQueuingOrgans = requires(A& a) {
    a.queuing_q1_organ();
    a.queuing_q2_organ();
};

static_assert(!HasIndexOrganizationOrgan<SaNodeSeqOrgan>,
              "IST 04.08.: SA haelt 9 von 18 Achsen als Organ-Member -- index_organization nicht");
static_assert(HasNodeTypeOrgan<SaNodeSeqOrgan>);
static_assert(HasQueuingOrgans<SaQueuingOrgan>);
static_assert(HasIndexOrganizationOrgan<SetIndexSetOrgan>,
              "die SET-Gattung hat den Member seit C3 -- deshalb laeuft die Verdrahtung dort");

// =============================================================================================
// (D) SLOT-SATZ UNVERAENDERT (Gate-Klasse IV): Cross-Genus fuegt keiner Gattung einen Slot hinzu
// =============================================================================================
static_assert(SaNodeSeqOrgan::organ_count() == SaBinding::slot_count);
static_assert(SetIndexSetOrgan::organ_count() == SetBinding::slot_count);
static_assert(ex::kCrossGenusWiringCount == 4);

} // namespace

int main() {
    std::cout << "E-24 C3: produktiver Cross-Genus-Einbau (Luecke-L7-Rest), real getrieben:\n";

    // -- (1) node_type <- Sequence, im SearchAlgorithm-Aggregat --------------------------------
    SaNodeSeqOrgan sa_seq{};
    sa_seq.node_type_organ().organ().push_back(11);
    sa_seq.node_type_organ().organ().push_back(12);
    sa_seq.node_type_organ().organ().push_back(13);
    auto const                                  seq_at  = sa_seq.node_type_organ().organ().at(1);
    cea::ObserverAggregate<SaNodeSeqComp> const agg_seq = sa_seq.observe_all();
    check_true("(1) node_type<-Sequence: at(1) == 12", seq_at.has_value() && *seq_at == 12u);
    check_eq("(1) node_type<-Sequence: push_count im SA-Aggregat", agg_seq.node_type.push_count, std::uint64_t{3});
    check_eq("(1) node_type<-Sequence: current_size im SA-Aggregat", agg_seq.node_type.current_size, std::uint64_t{3});
    check_eq("(1) node_type<-Sequence: at_count im SA-Aggregat", agg_seq.node_type.at_count, std::uint64_t{1});

    // -- (2) node_type <- Adapter, im SearchAlgorithm-Aggregat ---------------------------------
    SaNodeAdaOrgan sa_ada{};
    sa_ada.node_type_organ().organ().push(21);
    sa_ada.node_type_organ().organ().push(22);
    auto const                                  ada_pop = sa_ada.node_type_organ().organ().pop_front(); // FIFO
    cea::ObserverAggregate<SaNodeAdaComp> const agg_ada = sa_ada.observe_all();
    check_true("(2) node_type<-Adapter: pop_front() == 21", ada_pop.has_value() && *ada_pop == 21u);
    check_eq("(2) node_type<-Adapter: push_count im SA-Aggregat", agg_ada.node_type.push_count, std::uint64_t{2});
    check_eq("(2) node_type<-Adapter: pop_count im SA-Aggregat", agg_ada.node_type.pop_count, std::uint64_t{1});
    check_eq("(2) node_type<-Adapter: peak_occupancy im SA-Aggregat", agg_ada.node_type.peak_occupancy,
             std::uint64_t{2});

    // -- (3) queuing <- Adapter (q1 UND q2), im SearchAlgorithm-Aggregat -----------------------
    SaQueuingOrgan sa_q{};
    sa_q.queuing_q1_organ().organ().push(41);
    sa_q.queuing_q1_organ().organ().push(42);
    sa_q.queuing_q2_organ().organ().push(43);
    auto const                                  q1_front = sa_q.queuing_q1_organ().organ().front();
    cea::ObserverAggregate<SaQueuingComp> const agg_q    = sa_q.observe_all();
    check_true("(3) queuing<-Adapter: q1.front() == 41 (FIFO-Ende)", q1_front.has_value() && *q1_front == 41u);
    check_eq("(3) queuing<-Adapter: q1 push_count im SA-Aggregat", agg_q.queuing_q1.push_count, std::uint64_t{2});
    check_eq("(3) queuing<-Adapter: q2 push_count im SA-Aggregat", agg_q.queuing_q2.push_count, std::uint64_t{1});
    check_true("(3) queuing<-Adapter: die beiden Slots sind GETRENNTE Organe",
               agg_q.queuing_q1.push_count != agg_q.queuing_q2.push_count);
    check_eq("(3) queuing<-Adapter: observable_axis_count == 2", SaQueuingOrgan::observable_axis_count(),
             std::size_t{2});

    // -- (4) index_organization <- Set UND die C3-Pointe: beides in der CONTAINER-Gattung ------
    std::cout << "\ndie Container-Seite (vor C3 messtechnisch stumm -- Luecke L2):\n";
    SetIndexSetOrgan set_holder{};
    check_true("(4) Halter treibt seine EIGENE Mengen-Semantik: insert(51)", set_holder.insert(51));
    check_true("(4) Halter treibt seine EIGENE Mengen-Semantik: insert(52)", set_holder.insert(52));
    set_holder.index_organization_organ().organ().insert(501);
    set_holder.index_organization_organ().organ().insert(502);
    set_holder.index_organization_organ().organ().insert(503);
    cea::SetObserverSnapshot const             holder_flat = set_holder.observe_all();
    SetIndexSetOrgan::axis_observation_t const holder_axes = set_holder.observe_axes();
    check_eq("(4) flache Halter-Sicht: eigene insert_count", holder_flat.insert_count, std::uint64_t{2});
    check_eq("(4) per-Achsen-Sicht: Sub-Organ insert_count", holder_axes.index_organization.insert_count,
             std::uint64_t{3});
    check_true("(4) die beiden Ebenen sind GETRENNT (Halter 2 vs. Sub-Organ 3)",
               holder_flat.insert_count != holder_axes.index_organization.insert_count);
    check_eq("(4) Halter: observable_axis_count", SetIndexSetOrgan::observable_axis_count(), std::size_t{1});
    check_true("(4) Halter behaelt seine Gattung", SetIndexSetOrgan::genus() == cea::AnatomyGenus::Set);
    check_true("(4) Sub-Organ behaelt seine Gattung",
               SetIndexSetComp::index_organization::genus() == cea::AnatomyGenus::Set);

    SetNodeSeqOrgan set_node_seq{};
    set_node_seq.node_type_organ().organ().push_back(61);
    SetNodeSeqOrgan::axis_observation_t const node_axes = set_node_seq.observe_axes();
    check_eq("(4b) Set-Gattung mit node_type<-Sequence: push_count in der per-Achsen-Sicht",
             node_axes.node_type.push_count, std::uint64_t{1});
    check_true("(4b) das Sequence-Sub-Organ behaelt seine Gattung im Set-Slot",
               SetNodeSeqComp::node_type::genus() == cea::AnatomyGenus::Sequence);

    // -- (5) Die getragenen Slots bleiben stumm -- kein erfundener Wert -------------------------
    std::cout << "\nGegenprobe (getragene Slots):\n";
    check_true("(5) ein getragener Slot liefert EmptyAxisSnapshot",
               (std::is_same_v<decltype(cea::ObserverAggregate<SaNodeSeqComp>::filter), cea::EmptyAxisSnapshot>));
    check_eq("(5) SA-Komposition mit EINEM Sub-Organ: genau 1 beobachtbarer Slot",
             SaNodeSeqOrgan::observable_axis_count(), std::size_t{1});

    std::cout << "\n==== E-24 C3 Cross-Genus (L7-Rest): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
