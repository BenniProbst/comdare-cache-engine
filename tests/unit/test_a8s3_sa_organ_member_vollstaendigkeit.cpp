// test_a8s3_sa_organ_member_vollstaendigkeit -- A8-S3 (2026-08-04): DAUERHAFTE WACHE dafuer, dass die
// SearchAlgorithm-Anatomie ALLE Achsen als reale Organ-Member haelt und einsammelt.
//
// BEFUND (Qualitaets-Parameter-Katalog Abschnitt 4, Arbeitsliste P1; LEDGER:3812 Punkt 1):
// SearchAlgorithmAnatomy hielt nur NEUN der 18 Achsen als Organ-Member (search_algo, cache_traversal,
// mapping, node_type, memory_layout, serialization, queuing_q1, queuing_q2, persistence_target) und
// sammelte genau diese neun in observe_all() ein. Die uebrigen NEUN Slots des ObserverAggregate blieben
// default-konstruiert -- eine STILLE 0 statt einer Aussage. Wer ein observables Organ (Observable-Huelle
// oder Cross-Genus-Sub-Organ nach E-24 C1/C3) in einen dieser Slots setzte, bekam KEINE Werte: der Slot
// war weder ueber einen Accessor treibbar noch wurde er eingesammelt. cross_genus_composition.hpp fuehrte
// das ausdruecklich als offenen Punkt ("Die SA-Seite nachzuruesten hiesse, SearchAlgorithmAnatomy um
// Organ-Member zu erweitern -- ... offener Punkt fuer den Lead").
//
// NACHRUESTUNG (A8-S3): alle 18 Slots sind reale Member mit Accessor und werden in observe_all()
// eingesammelt. KEIN Wire-Ereignis: ObserverAggregate<Composition> traegt die 18 Slots seit STRUKT-R
// ORG-18; hier entsteht nur der HALTER, der sie fuellen kann.
//
// DIE ZWEI DINGE, DIE DIESE WACHE FESTHAELT:
//   (1) VOLLSTAENDIGKEIT ohne Literal: die Zahl der gehaltenen Organ-Member wird aus den Accessoren
//       GEZAEHLT und gegen anatomy::kV3AxisCount geprueft. Kommt eine 19. Achse dazu und wird der Member
//       vergessen, bricht diese TU -- statt den Slot still auf 0 zu lassen.
//   (2) EHRLICHKEIT: ein nachgeruesteter Slot erfindet nichts. Traegt die Registry-Variante kein
//       statistics(), bleibt der Slot EmptyAxisSnapshot (Bestands-ART-Komposition, Abschnitt B). Traegt
//       er ein observables Organ, kommen ECHTE Werte an (Cross-Genus-Naht, Abschnitt C).
//
// Build: Standalone int main() (kein gtest), Include-Kette wie test_e24_c3_cross_genus.cpp.

#include <anatomy/anatomy_base.hpp>
#include <anatomy/cross_genus_organ.hpp>
#include <anatomy/observable_tier.hpp> // kV3AxisCount (die tragende Konstante)
#include <anatomy/observer_aggregate.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/cross_genus_composition.hpp>
#include <builder/experiment_tree/genus_binding_traits.hpp>

#include <compositions/art_reference.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

namespace {

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Die Bestands-Anatomie mit einer REALEN Registry-Komposition (nackte Strategien in den neun
/// nachgeruesteten Slots) -- der Ehrlichkeits-Fall.
using ArtAnatomy = an::SearchAlgorithmAnatomy<comp::ArtComposition>;

// Accessor-Proben als Concepts (nicht als requires-Literal): eine requires-Expression ueber einen
// NICHT-abhaengigen Ausdruck diagnostiziert GCC hart, statt sie zu false auszuwerten (Befund der
// C3-TU, hier uebernommen).
// clang-format off
template <class A> concept HasSearchAlgo        = requires(A& a) { a.search_algo_organ(); };
template <class A> concept HasCacheTraversal    = requires(A& a) { a.cache_traversal_organ(); };
template <class A> concept HasMapping           = requires(A& a) { a.mapping_organ(); };
template <class A> concept HasPathCompression   = requires(A& a) { a.path_compression_organ(); };
template <class A> concept HasNodeType          = requires(A& a) { a.node_type_organ(); };
template <class A> concept HasMemoryLayout      = requires(A& a) { a.memory_layout_organ(); };
template <class A> concept HasAllocator         = requires(A& a) { a.allocator_organ(); };
template <class A> concept HasPrefetch          = requires(A& a) { a.prefetch_organ(); };
template <class A> concept HasConcurrency       = requires(A& a) { a.concurrency_organ(); };
template <class A> concept HasSerialization     = requires(A& a) { a.serialization_organ(); };
template <class A> concept HasValueHandle       = requires(A& a) { a.value_handle_organ(); };
template <class A> concept HasIndexOrganization = requires(A& a) { a.index_organization_organ(); };
template <class A> concept HasIoDispatch        = requires(A& a) { a.io_dispatch_organ(); };
template <class A> concept HasMigrationPolicy   = requires(A& a) { a.migration_policy_organ(); };
template <class A> concept HasFilter            = requires(A& a) { a.filter_organ(); };
template <class A> concept HasQueuingQ1         = requires(A& a) { a.queuing_q1_organ(); };
template <class A> concept HasQueuingQ2         = requires(A& a) { a.queuing_q2_organ(); };
template <class A> concept HasPersistenceTarget = requires(A& a) { a.persistence_target_organ(); };
// clang-format on

/// Zaehlt die Achsen-Slots, die als reales Organ-Member greifbar sind. GEZAEHLT, nie als Zahl gepflegt --
/// der Vergleich unten laeuft gegen kV3AxisCount (die tragende Konstante des Observer-Schemas).
template <class A>
[[nodiscard]] constexpr std::size_t gehaltene_organ_member() noexcept {
    std::size_t n = 0;
    if constexpr (HasSearchAlgo<A>) ++n;
    if constexpr (HasCacheTraversal<A>) ++n;
    if constexpr (HasMapping<A>) ++n;
    if constexpr (HasPathCompression<A>) ++n;
    if constexpr (HasNodeType<A>) ++n;
    if constexpr (HasMemoryLayout<A>) ++n;
    if constexpr (HasAllocator<A>) ++n;
    if constexpr (HasPrefetch<A>) ++n;
    if constexpr (HasConcurrency<A>) ++n;
    if constexpr (HasSerialization<A>) ++n;
    if constexpr (HasValueHandle<A>) ++n;
    if constexpr (HasIndexOrganization<A>) ++n;
    if constexpr (HasIoDispatch<A>) ++n;
    if constexpr (HasMigrationPolicy<A>) ++n;
    if constexpr (HasFilter<A>) ++n;
    if constexpr (HasQueuingQ1<A>) ++n;
    if constexpr (HasQueuingQ2<A>) ++n;
    if constexpr (HasPersistenceTarget<A>) ++n;
    return n;
}

// =============================================================================================
// (A) VOLLSTAENDIGKEIT -- gegen die tragende Konstante, nie gegen ein Literal
// =============================================================================================
static_assert(gehaltene_organ_member<ArtAnatomy>() == an::kV3AxisCount,
              "A8-S3: SearchAlgorithmAnatomy haelt nicht ALLE Achsen als Organ-Member. Kam eine Achse dazu, "
              "gehoert ihr Member + Accessor + observe_all()-Slot nachgezogen -- sonst bleibt sie stumm.");

// =============================================================================================
// (B) EHRLICHKEIT -- eine nackte Bestands-Strategie bleibt EmptyAxisSnapshot (keine erfundene Zahl)
// =============================================================================================
// Die ART-Registry-Komposition traegt in den nachgeruesteten Slots nackte Strategien ohne statistics().
// Der ObservableAxis-Guard in observe_all() faellt daher auf EmptyAxisSnapshot zurueck -- das IST die
// Wahrheit ueber diese Auspraegung, kein Defekt.
static_assert(
    std::is_same_v<decltype(an::ObserverAggregate<comp::ArtComposition>::index_organization), an::EmptyAxisSnapshot>);
static_assert(std::is_same_v<decltype(an::ObserverAggregate<comp::ArtComposition>::filter), an::EmptyAxisSnapshot>);
static_assert(
    std::is_same_v<decltype(an::ObserverAggregate<comp::ArtComposition>::migration_policy), an::EmptyAxisSnapshot>);

// =============================================================================================
// (C) DIE CROSS-GENUS-SA-NAHT -- erst durch die Nachruestung messbar
// =============================================================================================
using SaBinding      = ex::GenusBindingTraits<an::AnatomyGenus::SearchAlgorithm>;
using SaIndexSetComp = ex::SaCompositionWithIndexOrganizationOrgan<an::IndexOrganizationFromSet>;
using SaIndexSet     = SaBinding::AnatomyFor<SaIndexSetComp>;

static_assert(an::ObservableAxis<an::IndexOrganizationFromSet>);
static_assert(
    std::is_same_v<decltype(an::ObserverAggregate<SaIndexSetComp>::index_organization), an::SetObserverSnapshot>,
    "der SA-Slot traegt den Snapshot der QUELL-Gattung -- keine Vereinheitlichung");

// =============================================================================================
// (D) WIRE-NEUTRALITAET -- die Nachruestung bewegt kein Byte ueber die ABI-Grenze
// =============================================================================================
static_assert(sizeof(an::ComdareTierObserverSnapshot) == 1344,
              "A8-S3 ist in-process: der Wire-POD bleibt unberuehrt (G8-Negativ-Liste 1.2)");

} // namespace

int main() {
    std::cout << "==== A8-S3: SA-Organ-Member-Vollstaendigkeit (kV3AxisCount = " << an::kV3AxisCount << ") ====\n";
    std::cout << "    gehaltene Organ-Member der SearchAlgorithmAnatomy = " << gehaltene_organ_member<ArtAnatomy>()
              << "\n";
    tr("A Vollstaendigkeit: gehaltene Organ-Member == kV3AxisCount (static_assert, kein Literal)",
       gehaltene_organ_member<ArtAnatomy>() == an::kV3AxisCount);

    // -- B: die Bestands-Komposition bleibt ehrlich stumm, wo nichts zu berichten ist.
    ArtAnatomy                                        art{};
    an::ObserverAggregate<comp::ArtComposition> const agg_art = art.observe_all();
    (void)agg_art; // die Aussage ist der TYP (EmptyAxisSnapshot), nicht ein Wert
    tr("B Ehrlichkeit: nackte ART-Strategien liefern in den neuen Slots EmptyAxisSnapshot (static_assert)", true);
    std::cout << "    ART observable_axis_count = " << ArtAnatomy::observable_axis_count() << "\n";

    // -- C: derselbe Slot mit einem Cross-Genus-Sub-Organ traegt ECHTE Werte.
    SaIndexSet sa_index{};
    sa_index.index_organization_organ().organ().insert(901);
    sa_index.index_organization_organ().organ().insert(902);
    sa_index.index_organization_organ().organ().insert(903);
    an::ObserverAggregate<SaIndexSetComp> const agg_x = sa_index.observe_all();
    std::cout << "    Cross-Genus index_organization<-Set: insert_count = " << agg_x.index_organization.insert_count
              << "\n";
    tr("C Cross-Genus-SA-Naht: observe_all().index_organization.insert_count == 3 (vor A8-S3 unerreichbar)",
       agg_x.index_organization.insert_count == 3u);
    tr("C das Sub-Organ behaelt seine eigene Gattung im SA-Slot",
       SaIndexSetComp::index_organization::genus() == an::AnatomyGenus::Set);
    tr("C genau EIN beobachtbarer Slot in dieser Komposition (kein Phantom-Zuwachs)",
       SaIndexSet::observable_axis_count() == 1u);

    if (g_fail == 0)
        std::cout << "==== A8-S3 SA-Member-Wache: ALLE OK (18/18 gehalten, ehrlich leer wo leer) ====\n";
    else
        std::cout << "==== A8-S3 SA-Member-Wache: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
