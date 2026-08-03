// test_e24_c1_organ_concept -- E-24 / S12.1, Stufe C1 (OrganConcept-Vollausbau, 2026-08-04).
//
// TRAGENDER TEST der beiden C1-Stuecke aus anatomy/organ_concept.hpp:
//   (A) OP-SCHNITTSTELLE als disjunkte Concept-FAMILIE -- die Gate-G2-Matrix "5 Genera x Concept-
//       Teilmengen" (Bauplan-Dossier Paragraf 4.2), gefuehrt gegen die PRODUKTIVEN Anatomien ueber die
//       Bau-Bruecke builder/experiment_tree/genus_binding_traits.hpp, nicht gegen Ersatz-Typen.
//   (B) FORWARDING-BEWEIS -- ObservableOrgan<Organ> forwardet die Concept-Member und uebersetzt die
//       ORGAN-Ebene (observe_all) in die ACHSEN-Ebene (statistics/snapshot_t). Der Beweis laeuft bis
//       ans Ende der Kette: ein FREMDES Genus-Organ im node_type-Slot einer SearchAlgorithm-Komposition
//       wird real getrieben, und sein Snapshot taucht im ObserverAggregate der SA-Anatomie auf.
//
// WARUM DIESE TU EXISTIERT (wie bei C0): organ_concept.hpp darf weder die fuenf Anatomien noch
// observer_aggregate.hpp inkludieren (C0-Layer-Regel, Zyklus-Vermeidung). Die Beweise GEGEN das echte
// ObservableAxis-Concept und gegen die realen Anatomien koennen deshalb nur hier stehen.
//
// FIXTURE-UNABHAENGIGER ABLEITUNGSWEG (Lehre "gruene Tests zementieren alte Ordnung"): der Kern ist die
// EINE constrained Vorlage drive_and_observe<OrganOpSurface>, die je Gattung ueber die Teilmenge
// verzweigt (if constexpr, kein Runtime-Switch) und den Snapshot ueber den Vertrag abholt. Sie prueft
// keine Strings, sondern dass Vertrag + Op-Familie generisch TRAGEN.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/organ_concept.hpp"

#include "anatomy/observer_aggregate.hpp" // ACHSEN-Ebene: ObservableAxis (der Ziel-Vertrag des Forwardings)
#include "anatomy/set_default_organ.hpp"  // SortedArrayKeySet: echtes search_algo-Organ der Set-Anatomie
#include "builder/experiment_tree/genus_binding_traits.hpp" // die 5 Genus-Spezialisierungen (Bau-Bruecke)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

namespace cea = comdare::cache_engine::anatomy;
namespace ex  = comdare::cache_engine::builder::experiment;

// ---------------------------------------------------------------------------------------------
// Pruef-Rahmen (Repo-Muster der Nachbar-TUs test_e24_c0_organ_concept / test_genus_binding)
// ---------------------------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------------------------
// Die fuenf Organe -- materialisiert ueber die PRODUKTIVEN GenusBindingTraits-Spezialisierungen
// (identisches Muster wie test_e24_c0_organ_concept.cpp / test_container_genus.cpp).
// ---------------------------------------------------------------------------------------------
struct DelegatedAxis {};
using D = DelegatedAxis;

template <class... V>
struct ProbePermTuple {};

using SaTraits   = ex::GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
using SetTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Set>;
using SeqTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Sequence>;
using AdaTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>;
using ViewTraits = ex::GenusBindingTraits<cea::AnatomyGenus::View>;

using SaComp   = SaTraits::CompositionFor<ProbePermTuple<D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D>>;
using SetComp  = SetTraits::CompositionFor<cea::SortedArrayKeySet, D, D, D, D, D, D, D, D, D, D, D, D>;
using SeqComp  = SeqTraits::CompositionFor<D, D, D, D, D, D, D, D>;
using AdaComp  = AdaTraits::CompositionFor<D, D, D, D, D, D, D, D, D, D>;
using ViewComp = ViewTraits::CompositionFor<D, D>;

using SaOrgan   = SaTraits::AnatomyFor<SaComp>;
using SetOrgan  = SetTraits::AnatomyFor<SetComp>;
using SeqOrgan  = SeqTraits::AnatomyFor<SeqComp>;
using AdaOrgan  = AdaTraits::AnatomyFor<AdaComp>;
using ViewOrgan = ViewTraits::AnatomyFor<ViewComp>;

// =============================================================================================
// (A) GATE G2 -- DIE MATRIX: 5 Gattungen x Concept-Teilmengen, compile-hart
// =============================================================================================

// A.1 Die Halte-Flaeche traegt weiterhin alle fuenf (C0-Ergebnis, hier nicht neu bewiesen, aber gepinnt).
static_assert(cea::OrganConcept<SaOrgan> && cea::OrganConcept<SetOrgan> && cea::OrganConcept<SeqOrgan> &&
              cea::OrganConcept<AdaOrgan> && cea::OrganConcept<ViewOrgan>);

// A.2 Jede Gattung faellt in GENAU EINE Op-Teilmenge -- die Diagonale ist wahr, alles daneben falsch.
//     Das ist der Disjunktheits-Beweis; er ist der eigentliche Inhalt von "gemeinsame Op-Schnittstelle".
static_assert(cea::AxisOrganAccessOps<SaOrgan>);
static_assert(!cea::KeyedOrganOps<SaOrgan> && !cea::IndexedOrganOps<SaOrgan> && !cea::AdaptedOrganOps<SaOrgan> &&
              !cea::BoundViewOrganOps<SaOrgan>);

static_assert(cea::KeyedOrganOps<SetOrgan>);
static_assert(!cea::AxisOrganAccessOps<SetOrgan> && !cea::IndexedOrganOps<SetOrgan> &&
              !cea::AdaptedOrganOps<SetOrgan> && !cea::BoundViewOrganOps<SetOrgan>);

static_assert(cea::IndexedOrganOps<SeqOrgan>);
static_assert(!cea::AxisOrganAccessOps<SeqOrgan> && !cea::KeyedOrganOps<SeqOrgan> && !cea::AdaptedOrganOps<SeqOrgan> &&
              !cea::BoundViewOrganOps<SeqOrgan>);

static_assert(cea::AdaptedOrganOps<AdaOrgan>);
static_assert(!cea::AxisOrganAccessOps<AdaOrgan> && !cea::KeyedOrganOps<AdaOrgan> && !cea::IndexedOrganOps<AdaOrgan> &&
              !cea::BoundViewOrganOps<AdaOrgan>);

static_assert(cea::BoundViewOrganOps<ViewOrgan>);
static_assert(!cea::AxisOrganAccessOps<ViewOrgan> && !cea::KeyedOrganOps<ViewOrgan> &&
              !cea::IndexedOrganOps<ViewOrgan> && !cea::AdaptedOrganOps<ViewOrgan>);

// A.3 Derselbe Befund mechanisch statt haendisch gezaehlt (fixture-unabhaengiger Ableitungsweg).
static_assert(cea::organ_op_family_count<SaOrgan>() == 1);
static_assert(cea::organ_op_family_count<SetOrgan>() == 1);
static_assert(cea::organ_op_family_count<SeqOrgan>() == 1);
static_assert(cea::organ_op_family_count<AdaOrgan>() == 1);
static_assert(cea::organ_op_family_count<ViewOrgan>() == 1);

// A.4 Der EINE gemeinsame Name traegt alle fuenf -- und NUR Organe (eine Komposition faellt durch).
static_assert(cea::OrganOpSurface<SaOrgan> && cea::OrganOpSurface<SetOrgan> && cea::OrganOpSurface<SeqOrgan> &&
              cea::OrganOpSurface<AdaOrgan> && cea::OrganOpSurface<ViewOrgan>);
static_assert(cea::OrganOpSurface<SetComp> == false, "eine Komposition traegt keine Op-Flaeche");
static_assert(cea::OrganOpSurface<D> == false, "eine Achsen-Auspraegung traegt keine Op-Flaeche");
static_assert(cea::OrganOpSurface<int> == false);

// A.5 Die beiden QUER liegenden Faehigkeiten sind ECHTE Teilmengen (und belegen zugleich, warum sie
//     kein gemeinsamer Nenner sind): size() fehlt der SA-Anatomie, clear() zusaetzlich der View.
static_assert(!cea::OrganSized<SaOrgan>, "SA-Anatomie hat kein size() (R5.B: im AnatomyExecutionContext)");
static_assert(cea::OrganSized<SetOrgan> && cea::OrganSized<SeqOrgan> && cea::OrganSized<AdaOrgan> &&
              cea::OrganSized<ViewOrgan>);
static_assert(!cea::OrganClearable<SaOrgan>);
static_assert(!cea::OrganClearable<ViewOrgan>, "View ist non-owning -- kein clear (view_anatomy.hpp:4)");
static_assert(cea::OrganClearable<SetOrgan> && cea::OrganClearable<SeqOrgan> && cea::OrganClearable<AdaOrgan>);

// =============================================================================================
// (B) FORWARDING-BEWEIS -- ObservableOrgan<Organ>
// =============================================================================================

using ObsSa   = cea::ObservableOrgan<SaOrgan>;
using ObsSet  = cea::ObservableOrgan<SetOrgan>;
using ObsSeq  = cea::ObservableOrgan<SeqOrgan>;
using ObsAda  = cea::ObservableOrgan<AdaOrgan>;
using ObsView = cea::ObservableOrgan<ViewOrgan>;

// B.1 Der Wrapper forwardet die Concept-Member: er bleibt selbst Organ und behaelt die Genus-Identitaet.
static_assert(cea::OrganConcept<ObsSa> && cea::OrganConcept<ObsSet> && cea::OrganConcept<ObsSeq> &&
              cea::OrganConcept<ObsAda> && cea::OrganConcept<ObsView>);
static_assert(cea::GenusOrgan<ObsSa, cea::AnatomyGenus::SearchAlgorithm>);
static_assert(cea::GenusOrgan<ObsSet, cea::AnatomyGenus::Set>);
static_assert(cea::GenusOrgan<ObsSeq, cea::AnatomyGenus::Sequence>);
static_assert(cea::GenusOrgan<ObsAda, cea::AnatomyGenus::Adapter>);
static_assert(cea::GenusOrgan<ObsView, cea::AnatomyGenus::View>);
static_assert(cea::GenusOrgan<ObsSeq, cea::AnatomyGenus::Adapter> == false);
static_assert(std::is_same_v<ObsSeq::composition_t, SeqComp>);
static_assert(ObsSeq::organ_count() == SeqOrgan::organ_count());

// B.2 DIE BRUECKE: der Wrapper erfuellt das ECHTE ObservableAxis-Concept der ACHSEN-Ebene -- genau das,
//     was ein haltendes Aggregat abfragt. Das gewrappte Organ selbst erfuellt es NICHT (das ist die
//     Luecke L2, die der Wrapper schliesst).
static_assert(cea::ObservableAxis<ObsSa> && cea::ObservableAxis<ObsSet> && cea::ObservableAxis<ObsSeq> &&
              cea::ObservableAxis<ObsAda> && cea::ObservableAxis<ObsView>);
static_assert(!cea::ObservableAxis<SaOrgan> && !cea::ObservableAxis<SetOrgan> && !cea::ObservableAxis<SeqOrgan> &&
              !cea::ObservableAxis<AdaOrgan> && !cea::ObservableAxis<ViewOrgan>);

// B.3 Der Snapshot-Typ wird unveraendert durchgereicht (keine Vereinheitlichung, Luecke L4 bleibt).
static_assert(std::is_same_v<ObsSa::snapshot_t, cea::ObserverAggregate<SaComp>>);
static_assert(std::is_same_v<ObsSet::snapshot_t, cea::SetObserverSnapshot>);
static_assert(std::is_same_v<ObsSeq::snapshot_t, cea::SequenceObserverSnapshot>);
static_assert(std::is_same_v<ObsAda::snapshot_t, cea::AdapterObserverSnapshot>);
static_assert(std::is_same_v<ObsView::snapshot_t, cea::ViewObserverSnapshot>);
static_assert(std::is_same_v<ObsSeq::snapshot_t, cea::organ_snapshot_t<SeqOrgan>>);
// snapshot_of_t ist der Weg, auf dem das haltende Aggregat den Typ bildet -- er trifft denselben Typ.
static_assert(std::is_same_v<cea::snapshot_of_t<ObsSeq>, cea::SequenceObserverSnapshot>);
static_assert(std::is_same_v<cea::snapshot_of_t<SeqOrgan>, cea::EmptyAxisSnapshot>,
              "ohne Huelle ist ein Sub-Organ messtechnisch stumm -- genau das ist L2");

// B.4 WERT-/SCHLUESSEL-TYP-Durchreichung: konditional, exakt am Ist (Luecke L3 wird NICHT wegdefiniert).
template <class A>
concept HasValueType = requires { typename A::value_type; };
template <class A>
concept HasKeyType = requires { typename A::key_type; };
template <class A>
concept HasElementType = requires { typename A::element_type; };

static_assert(HasKeyType<ObsSa> && HasValueType<ObsSa> && !HasElementType<ObsSa>,
              "SA-Form durchgereicht: key_type + value_type");
static_assert(!HasKeyType<ObsSet> && !HasValueType<ObsSet> && !HasElementType<ObsSet>,
              "Set hat am Ist KEINEN oeffentlichen Wert-Typ -- die Huelle erfindet keinen");
static_assert(!HasValueType<ObsSeq> && HasElementType<ObsSeq>);
static_assert(!HasValueType<ObsAda> && HasElementType<ObsAda>);
static_assert(!HasValueType<ObsView> && HasElementType<ObsView>);
static_assert(std::is_same_v<ObsSa::key_type, SaOrgan::key_type>);
static_assert(std::is_same_v<ObsSa::value_type, SaOrgan::value_type>);
static_assert(std::is_same_v<ObsSeq::element_type, SeqOrgan::element_type>);

// B.5 ZERO-COST: leere Forwarder-Basen + leere CRTP-Wache kosten kein Byte (EBO), an den REALEN Organen.
static_assert(sizeof(ObsSa) == sizeof(SaOrgan));
static_assert(sizeof(ObsSet) == sizeof(SetOrgan));
static_assert(sizeof(ObsSeq) == sizeof(SeqOrgan));
static_assert(sizeof(ObsAda) == sizeof(AdaOrgan));
static_assert(sizeof(ObsView) == sizeof(ViewOrgan));

// B.6 DEKLARIERTE GRENZE (kein stiller Verlust): die Huelle forwardet die Op-Verben NICHT -- sie laufen
//     ueber organ(). Diese Zeilen pinnen die Entscheidung, damit eine spaetere Aenderung sichtbar wird.
static_assert(cea::OrganOpSurface<ObsSet> == false, "Op-Verben laufen ueber .organ(), nicht ueber die Huelle");
static_assert(cea::organ_op_family_count<ObsSet>() == 0);
static_assert(cea::KeyedOrganOps<decltype(std::declval<ObsSet&>().organ())>,
              "... und ueber .organ() ist die volle Op-Flaeche des Gewrappten erreichbar");

// =============================================================================================
// (C) DER FIXTURE-UNABHAENGIGE ABLEITUNGSWEG: EINE Vorlage, fuenf Gattungen, kein Runtime-Switch
// =============================================================================================

/// drive_and_observe -- treibt ein beliebiges Genus-Organ ueber SEINE Op-Teilmenge und gibt die
/// Fuellstands-Zahl zurueck, die dabei entsteht. `if constexpr` ueber die Faehigkeits-Concepts: statischer
/// Dispatch, keine vtable, kein Runtime-Switch. Genau dafuer existiert die Op-Familie.
template <cea::OrganOpSurface Organ>
[[nodiscard]] static std::size_t drive_and_observe(Organ& organ, std::uint64_t const* buffer, std::size_t n) {
    if constexpr (cea::KeyedOrganOps<Organ>) {
        for (std::size_t i = 0; i < n; ++i) (void)organ.insert(buffer[i]);
    } else if constexpr (cea::IndexedOrganOps<Organ>) {
        for (std::size_t i = 0; i < n; ++i) organ.push_back(static_cast<typename Organ::element_type>(buffer[i]));
    } else if constexpr (cea::AdaptedOrganOps<Organ>) {
        for (std::size_t i = 0; i < n; ++i) organ.push(static_cast<typename Organ::element_type>(buffer[i]));
    } else if constexpr (cea::BoundViewOrganOps<Organ>) {
        organ.bind(buffer, n);
        for (std::size_t i = 0; i < n; ++i) (void)organ.read(static_cast<std::uint64_t>(i));
    } else {
        // AxisOrganAccessOps (SearchAlgorithm): die Op-Flaeche sind die Organ-Accessoren; der
        // Container-Zugriff laeuft ueber den AnatomyExecutionContext und ist NICHT Teil der Anatomie.
        (void)organ.search_algo_organ();
    }
    auto const snapshot = organ.observe_all(); // Beobachtungs-Flaeche des Vertrags
    static_assert(std::is_same_v<decltype(snapshot), cea::organ_snapshot_t<Organ> const>);
    (void)snapshot;
    if constexpr (cea::OrganSized<Organ>) return organ.size();
    return 0;
}

// =============================================================================================
// (D) CROSS-GENUS-SUB-ORGAN AM PRODUKTIVEN SA-PFAD (Luecke L7, Mechanismus)
//     Der node_type-Slot (T4) einer SearchAlgorithm-Komposition wird mit einem SEQUENCE-Organ belegt --
//     ueber die Huelle. SearchAlgorithmAnatomy bleibt UNVERAENDERT; ihr observe_all() sammelt den
//     Sub-Organ-Snapshot allein deshalb ein, weil die Huelle ObservableAxis erfuellt.
// =============================================================================================
using SaCompSub  = SaTraits::CompositionFor<ProbePermTuple<D, D, D, D, ObsSeq, D, D, D, D, D, D, D, D, D, D, D, D, D>>;
using SaOrganSub = SaTraits::AnatomyFor<SaCompSub>;

static_assert(std::is_same_v<SaCompSub::node_type, ObsSeq>, "T4 == node_type (composition_factory.hpp:30)");
static_assert(cea::OrganConcept<SaOrganSub>);
static_assert(SaOrganSub::observable_axis_count() == 1,
              "genau EIN beobachtbarer Slot: das eingesetzte fremde Genus-Organ");
static_assert(SaOrgan::observable_axis_count() == 0, "Gegenprobe ohne Huelle: stumm (das war L2)");
static_assert(std::is_same_v<decltype(cea::ObserverAggregate<SaCompSub>::node_type), cea::SequenceObserverSnapshot>);

// =============================================================================================
// (E) DIE WACHE HAENGT (Luecke L5): alle fuenf Anatomien erben OrganGuard nach dem Repo-Goldstandard
//     axis_io_strategy_base.hpp:15-21. Die static_asserts der Wache feuern bei der ERSTEN Konstruktion
//     -- die im main() unten real stattfindet. Hier zusaetzlich die Struktur-Wache, damit ein stilles
//     Abhaengen der Vererbung auffliegt.
// =============================================================================================
static_assert(std::is_base_of_v<cea::OrganGuard<SaOrgan>, SaOrgan>);
static_assert(std::is_base_of_v<cea::OrganGuard<SetOrgan>, SetOrgan>);
static_assert(std::is_base_of_v<cea::OrganGuard<SeqOrgan>, SeqOrgan>);
static_assert(std::is_base_of_v<cea::OrganGuard<AdaOrgan>, AdaOrgan>);
static_assert(std::is_base_of_v<cea::OrganGuard<ViewOrgan>, ViewOrgan>);
// Zero-cost: die Wache ist eine leere Basis (EBO) -- kein Byte, keine vtable, kein Layout-Ereignis.
static_assert(std::is_empty_v<cea::OrganGuard<SaOrgan>> && std::is_empty_v<cea::OrganGuard<SetOrgan>> &&
              std::is_empty_v<cea::OrganGuard<SeqOrgan>> && std::is_empty_v<cea::OrganGuard<AdaOrgan>> &&
              std::is_empty_v<cea::OrganGuard<ViewOrgan>>);
static_assert(!std::is_polymorphic_v<SaOrgan> && !std::is_polymorphic_v<SetOrgan> && !std::is_polymorphic_v<SeqOrgan> &&
              !std::is_polymorphic_v<AdaOrgan> && !std::is_polymorphic_v<ViewOrgan>);
// Die Wache ist NICHT von aussen konstruierbar (protected Ctor) -- sie ist eine Wache, kein Bauteil.
static_assert(!std::is_default_constructible_v<cea::OrganGuard<SetOrgan>>);

int main() {
    std::cout << "E-24 C1: OrganConcept-Vollausbau (Op-Familie + ObservableAxis-Forwarding):\n";

    SaOrgan   sa{};
    SetOrgan  set{};
    SeqOrgan  seq{};
    AdaOrgan  ada{};
    ViewOrgan view{};

    std::uint64_t const buffer[4] = {41, 42, 43, 44};

    // (C-Beleg) EIN generischer Treiber, fuenf Gattungen, statischer Dispatch.
    check_eq("drive_and_observe(Set)", drive_and_observe(set, buffer, 4), std::size_t{4});
    check_eq("drive_and_observe(Sequence)", drive_and_observe(seq, buffer, 4), std::size_t{4});
    check_eq("drive_and_observe(Adapter)", drive_and_observe(ada, buffer, 4), std::size_t{4});
    check_eq("drive_and_observe(View)", drive_and_observe(view, buffer, 4), std::size_t{4});
    check_eq("drive_and_observe(SearchAlgorithm)", drive_and_observe(sa, buffer, 4), std::size_t{0});

    // (B-Beleg) Die Huelle liefert DIESELBEN Werte auf beiden Ebenen -- statistics() == observe_all().
    ObsSeq wrapped{};
    wrapped.organ().push_back(11);
    wrapped.organ().push_back(12);
    wrapped.organ().push_back(13);
    cea::SequenceObserverSnapshot const via_axis  = wrapped.statistics();  // ACHSEN-Ebene
    cea::SequenceObserverSnapshot const via_organ = wrapped.observe_all(); // ORGAN-Ebene
    check_eq("Huelle ACHSEN-Ebene statistics().push_count", via_axis.push_count, std::uint64_t{3});
    check_eq("Huelle ORGAN-Ebene observe_all().push_count", via_organ.push_count, std::uint64_t{3});
    check_eq("Huelle: beide Ebenen identisch (current_size)", via_axis.current_size, via_organ.current_size);
    check_true("Huelle forwardet die Identitaet (genus)", wrapped.genus() == cea::AnatomyGenus::Sequence);
    check_true("Huelle forwardet die Identitaet (composition_name)",
               wrapped.composition_name() == SeqOrgan::composition_name());

    // (B-Beleg) Der Ctor-Weg mit vorbereitetem Organ forwardet ebenfalls.
    SeqOrgan preloaded{};
    preloaded.push_back(99);
    ObsSeq const from_organ{preloaded};
    check_eq("Huelle aus vorbereitetem Organ: push_count", from_organ.statistics().push_count, std::uint64_t{1});

    // (D-Beleg) DIE DURCHREICHUNG: fremdes Genus-Organ im node_type-Slot der SA-Komposition. Getrieben
    //           wird das Sub-Organ ueber den Organ-Accessor der SA-Anatomie; abgeholt wird es ueber
    //           deren observe_all() -- ohne EINE Zeile Aenderung an SearchAlgorithmAnatomy.
    SaOrganSub sa_sub{};
    sa_sub.node_type_organ().organ().push_back(7);
    sa_sub.node_type_organ().organ().push_back(8);
    (void)sa_sub.node_type_organ().organ().at(0);
    cea::ObserverAggregate<SaCompSub> const agg = sa_sub.observe_all();
    check_eq("Sub-Organ im SA-node_type-Slot: push_count im SA-Aggregat", agg.node_type.push_count, std::uint64_t{2});
    check_eq("Sub-Organ im SA-node_type-Slot: at_count im SA-Aggregat", agg.node_type.at_count, std::uint64_t{1});
    check_eq("Sub-Organ im SA-node_type-Slot: current_size im SA-Aggregat", agg.node_type.current_size,
             std::uint64_t{2});
    check_eq("SA-Anatomie mit Sub-Organ: observable_axis_count", SaOrganSub::observable_axis_count(), std::size_t{1});
    check_true("Sub-Organ behaelt seine Genus-Identitaet im fremden Slot",
               SaCompSub::node_type::genus() == cea::AnatomyGenus::Sequence);
    check_true("haltende Anatomie behaelt ihre eigene Genus-Identitaet",
               SaOrganSub::genus() == cea::AnatomyGenus::SearchAlgorithm);

    std::cout << "\n==== E-24 C1 OrganConcept-Vollausbau: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
