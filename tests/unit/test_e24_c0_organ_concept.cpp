// test_e24_c0_organ_concept -- E-24 / S12.1, Stufe C0 (M0-Vorstufe, 2026-08-04).
//
// TRAGENDER TEST des neuen Vertrags anatomy/organ_concept.hpp: er instanziiert OrganConcept gegen ALLE
// FUENF Gattungs-Anatomien -- und zwar ueber die PRODUKTIVE Bau-Bruecke
// builder/experiment_tree/genus_binding_traits.hpp (die 5 Spezialisierungen SearchAlgorithm/Adapter/
// Set/Sequence/View), nicht ueber handgeschriebene Ersatz-Typen. Die Slot-Zahlen werden gegen die
// Wachen-Klasse anatomy/container_framework.hpp (type_traits<G>::slot_count 11/13/9/5) rueckgekoppelt.
//
// WARUM DIESE TU EXISTIERT: organ_concept.hpp darf die fuenf Anatomien nicht inkludieren (Zyklus --
// sie sollen ihn spaeter selbst benutzen). Der Beweis "der Vertrag passt auf die realen Anatomien"
// kann deshalb NUR hier stehen. Ohne diese TU waere der Header eine unbelegte Behauptung.
//
// FIXTURE-UNABHAENGIGER ABLEITUNGSWEG (Lehre "gruene Tests zementieren alte Ordnung"): der Kern des
// Tests ist die EINE constrained Funktions-Vorlage genus_via_concept<OrganConcept>, die alle fuenf
// Gattungen durchlaeuft. Sie prueft nicht Strings, sondern dass der Vertrag generisch TRAEGT.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes (Liste
// COMDARE_GOALV6_BOOST_DTESTS in tests/unit/CMakeLists.txt -- Waisen-TU-Lehre: registriert).

#include "anatomy/organ_concept.hpp"

#include "anatomy/container_framework.hpp" // Wachen-Klasse: type_traits<G>::slot_count (Slot-Pins)
#include "anatomy/set_default_organ.hpp"   // SortedArrayKeySet: echtes search_algo-Organ der Set-Anatomie
#include "builder/experiment_tree/genus_binding_traits.hpp" // die 5 Genus-Spezialisierungen (Bau-Bruecke)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

namespace cea = comdare::cache_engine::anatomy;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace ctr = comdare::container;

// ---------------------------------------------------------------------------------------------
// Pruef-Rahmen (Repo-Muster der Nachbar-TUs test_container_genus / test_genus_binding)
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
// ---------------------------------------------------------------------------------------------
// Platzhalter fuer die getragenen/delegierten Achsen-Slots. Muster exakt wie test_container_genus.cpp:
// die Anatomien treiben je nur ihr spezifisches Organ real, die uebrigen Slots tragen sie als
// Kompositions-Identitaet. Fuer den ORGAN-Vertrag (Identitaet + Beobachtung) genuegt das.
struct DelegatedAxis {};
using D = DelegatedAxis;

/// ProbePermTuple -- Traeger fuer den SearchAlgorithm-Zweig: GenusBindingTraits<SearchAlgorithm>::
/// CompositionFor erwartet ein PermTuple-artiges Template und materialisiert daraus AdHocComposition.
/// So laeuft auch der SA-Zweig ueber CompositionFor und nicht an der Bau-Bruecke vorbei.
template <class... V>
struct ProbePermTuple {};

using SaTraits   = ex::GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
using SetTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Set>;
using SeqTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Sequence>;
using AdaTraits  = ex::GenusBindingTraits<cea::AnatomyGenus::Adapter>;
using ViewTraits = ex::GenusBindingTraits<cea::AnatomyGenus::View>;

using SaComp = SaTraits::CompositionFor<ProbePermTuple<D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D>>;
using SetComp =
    SetTraits::CompositionFor<cea::SortedArrayKeySet, D, D, D, D, D, D, D, D, D, D, D, D>; // T0 = echtes K-Organ
using SeqComp  = SeqTraits::CompositionFor<D, D, D, D, D, D, D, D>;       // Growth defaultet (DoublingGrowth)
using AdaComp  = AdaTraits::CompositionFor<D, D, D, D, D, D, D, D, D, D>; // Inner defaultet (DequeInner<>)
using ViewComp = ViewTraits::CompositionFor<D, D>;                        // Extent/Layout/Accessor defaulten

using SaOrgan   = SaTraits::AnatomyFor<SaComp>;
using SetOrgan  = SetTraits::AnatomyFor<SetComp>;
using SeqOrgan  = SeqTraits::AnatomyFor<SeqComp>;
using AdaOrgan  = AdaTraits::AnatomyFor<AdaComp>;
using ViewOrgan = ViewTraits::AnatomyFor<ViewComp>;

// ---------------------------------------------------------------------------------------------
// (1) DIE MATRIX: alle 5 Gattungen erfuellen OrganConcept -- compile-hart
// ---------------------------------------------------------------------------------------------
static_assert(cea::OrganConcept<SaOrgan>, "SearchAlgorithm-Anatomie erfuellt OrganConcept nicht");
static_assert(cea::OrganConcept<SetOrgan>, "Set-Anatomie erfuellt OrganConcept nicht");
static_assert(cea::OrganConcept<SeqOrgan>, "Sequence-Anatomie erfuellt OrganConcept nicht");
static_assert(cea::OrganConcept<AdaOrgan>, "Adapter-Anatomie erfuellt OrganConcept nicht");
static_assert(cea::OrganConcept<ViewOrgan>, "View-Anatomie erfuellt OrganConcept nicht");

// Genus-Identitaet gepinnt (positiv je Gattung, negativ ueber Kreuz -- ein Slot fuer Genus X nimmt
// kein Organ von Genus Y an).
static_assert(cea::GenusOrgan<SaOrgan, cea::AnatomyGenus::SearchAlgorithm>);
static_assert(cea::GenusOrgan<SetOrgan, cea::AnatomyGenus::Set>);
static_assert(cea::GenusOrgan<SeqOrgan, cea::AnatomyGenus::Sequence>);
static_assert(cea::GenusOrgan<AdaOrgan, cea::AnatomyGenus::Adapter>);
static_assert(cea::GenusOrgan<ViewOrgan, cea::AnatomyGenus::View>);
static_assert(cea::GenusOrgan<SetOrgan, cea::AnatomyGenus::Sequence> == false);
static_assert(cea::GenusOrgan<SeqOrgan, cea::AnatomyGenus::Adapter> == false);
static_assert(cea::GenusOrgan<AdaOrgan, cea::AnatomyGenus::View> == false);
static_assert(cea::GenusOrgan<ViewOrgan, cea::AnatomyGenus::SearchAlgorithm> == false);
static_assert(cea::GenusOrgan<SaOrgan, cea::AnatomyGenus::Set> == false);

// Negativ gegen PRODUKTIVE Typen (nicht gegen erfundene): eine Komposition ist kein Organ, eine
// Achsen-Auspraegung erst recht nicht.
static_assert(cea::OrganConcept<SetComp> == false, "eine Komposition ist kein Organ");
static_assert(cea::OrganConcept<SaComp> == false, "eine Komposition ist kein Organ");
static_assert(cea::OrganConcept<D> == false, "eine Achsen-Auspraegung ist kein Organ");

// ---------------------------------------------------------------------------------------------
// (2) SLOT-PIN-RUECKKOPPLUNG: organ_count() der Anatomie == slot_count der Bau-Bruecke ==
//     Slot-Pin der Wachen-Klasse container_framework.hpp. Bricht, sobald irgendwo eine Slot-Zahl
//     einseitig wandert.
// ---------------------------------------------------------------------------------------------
static_assert(SaOrgan::organ_count() == SaTraits::slot_count);
static_assert(SetOrgan::organ_count() == SetTraits::slot_count);
static_assert(SeqOrgan::organ_count() == SeqTraits::slot_count);
static_assert(AdaOrgan::organ_count() == AdaTraits::slot_count);
static_assert(ViewOrgan::organ_count() == ViewTraits::slot_count);

static_assert(SaTraits::slot_count == 18);
static_assert(AdaTraits::slot_count == ctr::type_traits<cea::AnatomyGenus::Adapter>::slot_count);
static_assert(SetTraits::slot_count == ctr::type_traits<cea::AnatomyGenus::Set>::slot_count);
static_assert(SeqTraits::slot_count == ctr::type_traits<cea::AnatomyGenus::Sequence>::slot_count);
static_assert(ViewTraits::slot_count == ctr::type_traits<cea::AnatomyGenus::View>::slot_count);
static_assert(ctr::type_count == 4, "C0 aendert die Container-Typ-Menge nicht");

// ---------------------------------------------------------------------------------------------
// (3) LUECKE L4 MECHANISCH BELEGT: es gibt KEINEN gemeinsamen Snapshot-Typ. organ_snapshot_t
//     benennt je Gattung einen ANDEREN Typ -- deshalb fordert OrganConcept ihn nicht.
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<cea::organ_snapshot_t<SaOrgan>, cea::ObserverAggregate<SaComp>>);
static_assert(std::is_same_v<cea::organ_snapshot_t<SetOrgan>, cea::SetObserverSnapshot>);
static_assert(std::is_same_v<cea::organ_snapshot_t<SeqOrgan>, cea::SequenceObserverSnapshot>);
static_assert(std::is_same_v<cea::organ_snapshot_t<AdaOrgan>, cea::AdapterObserverSnapshot>);
static_assert(std::is_same_v<cea::organ_snapshot_t<ViewOrgan>, cea::ViewObserverSnapshot>);
static_assert(std::is_same_v<cea::organ_snapshot_t<SetOrgan>, cea::organ_snapshot_t<SeqOrgan>> == false);
static_assert(std::is_same_v<cea::organ_snapshot_t<SeqOrgan>, cea::organ_snapshot_t<AdaOrgan>> == false);
static_assert(std::is_same_v<cea::organ_snapshot_t<AdaOrgan>, cea::organ_snapshot_t<ViewOrgan>> == false);

// ---------------------------------------------------------------------------------------------
// (4) C1-ABSICHTS-WACHE fuer LUECKE L3 (kein gemeinsamer Wert-Typ). Diese Zeilen pinnen den IST-Stand
//     vom 04.08. Sie MUESSEN gedreht werden, sobald C1 den Vertrag um einen Wert-Typ erweitert --
//     genau dafuer stehen sie hier: damit die Erweiterung nicht STILL passiert.
//     (Sie stehen bewusst in der Test-TU und NICHT im Produktions-Header: dort waeren sie ein
//     Compile-Bruch fuer jeden Verwender, hier sind sie ein sichtbares, revertierbares Signal.)
// ---------------------------------------------------------------------------------------------
template <class A>
concept HasValueType = requires { typename A::value_type; };
template <class A>
concept HasElementType = requires { typename A::element_type; };

static_assert(HasValueType<SaOrgan> && !HasElementType<SaOrgan>, "SA traegt key_type/value_type");
static_assert(!HasValueType<SetOrgan> && !HasElementType<SetOrgan>,
              "Set traegt am Ist GAR KEINEN oeffentlichen Wert-Typ (key_t/value_t sind privat)");
static_assert(!HasValueType<SeqOrgan> && HasElementType<SeqOrgan>, "Sequence traegt element_type");
static_assert(!HasValueType<AdaOrgan> && HasElementType<AdaOrgan>, "Adapter traegt element_type");
static_assert(!HasValueType<ViewOrgan> && HasElementType<ViewOrgan>, "View traegt element_type");

// ---------------------------------------------------------------------------------------------
// (5) DER FIXTURE-UNABHAENGIGE ABLEITUNGSWEG: EINE constrained Vorlage ueber ALLE fuenf Gattungen.
//     Sie ist der eigentliche Zweck des Vertrags -- generischer, dispatch-freier Zugriff.
// ---------------------------------------------------------------------------------------------
template <cea::OrganConcept Organ>
[[nodiscard]] static cea::AnatomyGenus genus_via_concept(Organ const& organ) noexcept {
    auto const snapshot = organ.observe_all(); // Beobachtungs-Flaeche des Vertrags
    (void)snapshot;
    static_assert(std::is_same_v<decltype(snapshot), cea::organ_snapshot_t<Organ> const>);
    return Organ::genus();
}

int main() {
    std::cout << "E-24 C0 (M0-Vorstufe): OrganConcept gegen alle 5 Gattungs-Anatomien:\n";

    SaOrgan   sa{};
    SetOrgan  set{};
    SeqOrgan  seq{};
    AdaOrgan  ada{};
    ViewOrgan view{};

    // (5a) EIN generischer Weg, fuenf Gattungen -- ohne Laufzeit-Verzweigung, ohne vtable.
    check_true("genus_via_concept(SearchAlgorithm)", genus_via_concept(sa) == cea::AnatomyGenus::SearchAlgorithm);
    check_true("genus_via_concept(Set)", genus_via_concept(set) == cea::AnatomyGenus::Set);
    check_true("genus_via_concept(Sequence)", genus_via_concept(seq) == cea::AnatomyGenus::Sequence);
    check_true("genus_via_concept(Adapter)", genus_via_concept(ada) == cea::AnatomyGenus::Adapter);
    check_true("genus_via_concept(View)", genus_via_concept(view) == cea::AnatomyGenus::View);

    // (5b) Slot-Zahlen literal (die static_asserts oben koppeln sie; hier die sichtbaren Werte).
    check_eq("SearchAlgorithm organ_count", sa.organ_count(), std::size_t{18});
    check_eq("Set organ_count", set.organ_count(), std::size_t{13});
    check_eq("Sequence organ_count", seq.organ_count(), std::size_t{9});
    check_eq("Adapter organ_count", ada.organ_count(), std::size_t{11});
    check_eq("View organ_count", view.organ_count(), std::size_t{5});

    // (5c) Die Beobachtungs-Flaeche traegt ECHTE Werte -- der Vertrag ist keine leere Huelle.
    check_true("Set: insert(7) ist neu", set.insert(7));
    check_true("Set: insert(7) erneut ist NICHT neu", !set.insert(7));
    check_true("Set: contains(7)", set.contains(7));
    check_true("Set: contains(8) nicht", !set.contains(8));
    cea::SetObserverSnapshot const set_obs = set.observe_all();
    check_eq("Set-Observer: insert_count", set_obs.insert_count, std::uint64_t{1});
    check_eq("Set-Observer: contains_count", set_obs.contains_count, std::uint64_t{2});
    check_eq("Set-Observer: contains_hit_count", set_obs.contains_hit_count, std::uint64_t{1});

    seq.push_back(11);
    seq.push_back(12);
    cea::SequenceObserverSnapshot const seq_obs = seq.observe_all();
    check_eq("Sequence-Observer: push_count", seq_obs.push_count, std::uint64_t{2});
    check_eq("Sequence-Observer: current_size", seq_obs.current_size, std::uint64_t{2});

    ada.push(21);
    ada.push(22);
    auto const popped = ada.pop_front();
    check_true("Adapter: pop_front liefert 21", popped.has_value() && *popped == 21u);
    cea::AdapterObserverSnapshot const ada_obs = ada.observe_all();
    check_eq("Adapter-Observer: push_count", ada_obs.push_count, std::uint64_t{2});
    check_eq("Adapter-Observer: pop_count", ada_obs.pop_count, std::uint64_t{1});

    std::uint64_t const buffer[3] = {31, 32, 33};
    view.bind(buffer, 3);
    auto const read0 = view.read(0);
    check_true("View: read(0) liefert 31", read0.has_value() && *read0 == 31u);
    cea::ViewObserverSnapshot const view_obs = view.observe_all();
    check_eq("View-Observer: bind_count", view_obs.bind_count, std::uint64_t{1});
    check_eq("View-Observer: read_count", view_obs.read_count, std::uint64_t{1});

    // (5d) Die CRTP-Wache OrganGuard ist kein toter Buchstabe: erst die KONSTRUKTION instanziiert
    //      ihren Ctor-Rumpf und damit die static_asserts. Genau das passiert hier.
    cea::organ_concept_detail::GuardedArchetype guarded{};
    check_true("OrganGuard: konformer Traeger konstruiert (Ctor-Wache uebersetzt)",
               guarded.genus() == cea::AnatomyGenus::SearchAlgorithm);

    // (5e) LUECKE L2 sichtbar gemacht (nicht behauptet): die SA-Anatomie traegt hier NULL beobachtbare
    //      Achsen, weil die Platzhalter-Achsen kein statistics() haben. Das ist genau die fehlende
    //      Achsen-nach-Organ-Weiterreichung, die die naechste Stufe zu bauen hat.
    check_eq("SA: observable_axis_count der Platzhalter-Komposition", SaOrgan::observable_axis_count(), std::size_t{0});

    std::cout << "\n==== E-24 C0 OrganConcept (5/5 Gattungen): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
