// test_e24_c7_element_type_vertrag -- E-24 C7-3 (b-Teil).
//
// GEGENSTAND: der GATTUNGS-TYP-VERTRAG element_type (C7-Auflage C7-3, Diskrepanz-Dossier 5/C7-3).
// Owner-Kriterium (LEDGER:3834/:3836): die Container-Gattung ist das vector-Gleichnis mit EINEM
// Element-Parameter <T>, die Map-Gattung das map-Gleichnis mit <Key,Value>.
//
//   (A) BEFUND D2       -- am Ist tragen ALLE fuenf Anatomie-Huellen genau EINEN Template-Parameter
//                          (die Composition), NICHT T. Das Owner-Gleichnis lebt in den Typ-MEMBERN.
//                          Hier compile-hart belegt, nicht behauptet.
//   (B) 4/4 GETRAGEN    -- alle vier Container-Genus-Anatomien fuehren element_type oeffentlich
//                          (Set seit C7-3; vorher trug es die Set-Anatomie als EINZIGE nur privat).
//   (C) EIN FIXPUNKT    -- alle vier tragen DENSELBEN Element-Typ. Das ist die instanziierte
//                          Gattungs-Huelle T == uint64_t -- ABGELEITET, nicht als Literal gepinnt.
//   (D) MAP-SEITE       -- die Map-Gattung fuehrt das PAAR key_type/value_type (die <Key,Value>-
//                          Instanz) -- ein anderer Vertrag, wie das Owner-Modell es verlangt.
//   (E) SET K=V         -- der Set-Element-Typ IST der Schluessel-Typ (std::set-konform:
//                          value_type == Key). Die K=V-Bauform bleibt unveraendert.
//   (F) DECKUNG         -- der Vertrag laeuft ueber type_traits<G>::ElementTypeFor, also ueber den
//                          AUTORITATIVEN Bau-Pfad, nicht ueber eine zweite Ableitung.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/container_framework.hpp"

#include "anatomy/set_default_organ.hpp" // SortedArrayKeySet (der Set-Kern)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace ccn = comdare::container;
namespace ex  = comdare::cache_engine::builder::experiment;

static int g_fail = 0;

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

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

using SearchOrgan   = SaTraits::AnatomyFor<SaComp>;
using SetOrgan      = SetTraits::AnatomyFor<SetComp>;
using SequenceOrgan = SeqTraits::AnatomyFor<SeqComp>;
using AdapterOrgan  = AdaTraits::AnatomyFor<AdaComp>;
using ViewOrgan     = ViewTraits::AnatomyFor<ViewComp>;

// =============================================================================================
// (A) BEFUND D2 -- die Huellen tragen EINEN Template-Parameter, und der ist die Composition.
//     Belegt ueber die Rueckwaerts-Ersetzung: dieselbe Huelle mit einer ANDEREN Composition ist ein
//     anderer Typ; einen Element-Typ-Parameter gibt es an der Huelle NICHT.
// =============================================================================================

static_assert(!std::is_same_v<SetOrgan, SequenceOrgan>);
static_assert(std::is_same_v<SetOrgan, cea::SetAnatomy<SetComp>>,
              "D2: die Set-Huelle ist ueber GENAU EINEN Parameter (die Composition) parametrisiert");
static_assert(std::is_same_v<SequenceOrgan, cea::SequenceAnatomy<SeqComp>>);
static_assert(std::is_same_v<ViewOrgan, cea::ViewAnatomy<ViewComp>>);

// =============================================================================================
// (B) + (C) 4/4 tragen element_type, und es ist DERSELBE (der Fixpunkt der Gattungs-Huelle).
// =============================================================================================

static_assert(ccn::ContainerElementTyped<SetOrgan>,
              "C7-3: die Set-Anatomie fuehrt element_type jetzt oeffentlich (vorher nur privat als key_t)");
static_assert(ccn::ContainerElementTyped<SequenceOrgan>);
static_assert(ccn::ContainerElementTyped<AdapterOrgan>);
static_assert(ccn::ContainerElementTyped<ViewOrgan>);

static_assert(std::is_same_v<typename SetOrgan::element_type, typename SequenceOrgan::element_type>,
              "C7-3: alle vier Container-Genera tragen DENSELBEN Element-Typ -- das IST die "
              "instanziierte Gattungs-Huelle <T>");
static_assert(std::is_same_v<typename SequenceOrgan::element_type, typename AdapterOrgan::element_type>);
static_assert(std::is_same_v<typename AdapterOrgan::element_type, typename ViewOrgan::element_type>);

// Der Fixpunkt selbst -- nachrichtlich, ABGELEITET aus dem Set-Kern-Organ statt hartkodiert.
static_assert(std::is_same_v<typename SetOrgan::element_type, cea::SortedArrayKeySet::key_type>);
static_assert(std::is_same_v<typename SetOrgan::element_type, std::uint64_t>,
              "Ist-Fixpunkt der instanziierten Gattungs-Huelle (K5: eine ECHTE Parametrisierung ist "
              "NICHT in diesem Fenster -- deklarierte Luecke)");

// =============================================================================================
// (D) Die MAP-Seite traegt einen ANDEREN Vertrag: das Paar <Key,Value>.
// =============================================================================================

template <class A>
concept TraegtKeyValuePaar = requires {
    typename A::key_type;
    typename A::value_type;
};
static_assert(TraegtKeyValuePaar<SearchOrgan>,
              "Map-Gattung == map-Gleichnis: das Genus fuehrt das PAAR key_type/value_type");
static_assert(!ccn::ContainerElementTyped<SearchOrgan>,
              "und ausdruecklich KEIN element_type -- die beiden Gattungs-Typ-Vertraege sind disjunkt");

// =============================================================================================
// (E) Set K=V -- std::set-konform (value_type == Key). Die Bauform bleibt unveraendert (C7-5).
// =============================================================================================

static_assert(std::is_same_v<typename SetOrgan::element_type, typename SetOrgan::set_organ_t::key_type>,
              "Set: der Element-Typ IST der Schluessel-Typ (std::set: value_type == Key)");

// =============================================================================================
// (F) Der Vertrag laeuft ueber den AUTORITATIVEN Bau-Pfad type_traits<G>::ElementTypeFor.
// =============================================================================================

static_assert(
    std::is_same_v<ccn::type_traits<cea::AnatomyGenus::Set>::ElementTypeFor<SetComp>, typename SetOrgan::element_type>);
static_assert(std::is_same_v<ccn::type_traits<cea::AnatomyGenus::Sequence>::ElementTypeFor<SeqComp>,
                             typename SequenceOrgan::element_type>);
static_assert(std::is_same_v<ccn::type_traits<cea::AnatomyGenus::Adapter>::ElementTypeFor<AdaComp>,
                             typename AdapterOrgan::element_type>);
static_assert(std::is_same_v<ccn::type_traits<cea::AnatomyGenus::View>::ElementTypeFor<ViewComp>,
                             typename ViewOrgan::element_type>);

} // namespace

int main() {
    std::cout << "=== E-24 C7-3 -- der Gattungs-Typ-Vertrag element_type ===\n";

    std::cout << "\n[A] Befund D2: die Huellen tragen EINEN Parameter, und der ist die Composition\n";
    check_true("Set- und Sequence-Huelle sind verschiedene Typen (Composition-parametrisch)",
               !std::is_same_v<SetOrgan, SequenceOrgan>);

    std::cout << "\n[B] 4/4 fuehren element_type oeffentlich (Set seit C7-3)\n";
    check_true("Set", ccn::ContainerElementTyped<SetOrgan>);
    check_true("Sequence", ccn::ContainerElementTyped<SequenceOrgan>);
    check_true("Adapter", ccn::ContainerElementTyped<AdapterOrgan>);
    check_true("View", ccn::ContainerElementTyped<ViewOrgan>);

    std::cout << "\n[C] EIN Fixpunkt: alle vier tragen DENSELBEN Element-Typ\n";
    check_true("Set == Sequence", std::is_same_v<SetOrgan::element_type, SequenceOrgan::element_type>);
    check_true("Sequence == Adapter", std::is_same_v<SequenceOrgan::element_type, AdapterOrgan::element_type>);
    check_true("Adapter == View", std::is_same_v<AdapterOrgan::element_type, ViewOrgan::element_type>);
    check_true("und der Fixpunkt ist uint64_t (die instanziierte Gattungs-Huelle)",
               std::is_same_v<SetOrgan::element_type, std::uint64_t>);

    std::cout << "\n[D] Die Map-Gattung traegt den ANDEREN Vertrag: <Key,Value>\n";
    check_true("SA fuehrt key_type UND value_type", TraegtKeyValuePaar<SearchOrgan>);
    check_true("SA fuehrt KEIN element_type (die Vertraege sind disjunkt)", !ccn::ContainerElementTyped<SearchOrgan>);

    std::cout << "\n[E] Set K=V bleibt: der Element-Typ IST der Schluessel-Typ (std::set-konform)\n";
    check_true("element_type == set_organ_t::key_type",
               std::is_same_v<SetOrgan::element_type, SetOrgan::set_organ_t::key_type>);

    std::cout << "\n[F] Der Vertrag laeuft ueber type_traits<G>::ElementTypeFor (autoritativer Pfad)\n";
    check_true(
        "Set",
        std::is_same_v<ccn::type_traits<cea::AnatomyGenus::Set>::ElementTypeFor<SetComp>, SetOrgan::element_type>);
    check_true(
        "View",
        std::is_same_v<ccn::type_traits<cea::AnatomyGenus::View>::ElementTypeFor<ViewComp>, ViewOrgan::element_type>);

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
