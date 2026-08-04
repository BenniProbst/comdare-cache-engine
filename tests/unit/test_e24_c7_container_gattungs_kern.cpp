// test_e24_c7_container_gattungs_kern -- E-24 C7-2 (b-Teil).
//
// GEGENSTAND: der Container-GATTUNGS-KERN im comdare::container-Kopf-Framework
// (ContainerGattungsKern / ContainerClearBlock, libs/cache_engine/anatomy/container_framework.hpp).
// Owner-KERN NACHTRAG 4/5 (LEDGER:3836/:3838): "Das Genus erbt von der gemeinsamen Gattung";
// Gattung und Genus sind GESTAFFELTE Interface-Definitionen.
//
//   (A) KERN 4/4        -- alle vier Container-Genus-Anatomien erfuellen den Kern
//                          {Identitaet, observe_axes, size} -- an ECHTEN Kompositionen, nicht an
//                          Attrappen.
//   (B) OPTIONAL 3/4    -- der gestufte clear-Block traegt Set/Sequence/Adapter; die View faellt
//                          DEKLARIERT heraus (non-owning). Die Stufen-Zahl macht das messbar.
//   (C) SA AUSSERHALB   -- die Map-Gattung (Genus SearchAlgorithm) erfuellt den Container-Kern NICHT:
//                          ihr fehlt size(). Genau deshalb ist die Kern-Menge nicht groesser.
//   (D) KEIN OP-VERB    -- die Op-Verb-Schnittmenge ueber die vier Genera ist LEER: kein Genus
//                          versteht die Verben eines anderen. Ein vereinheitlichtes Op-Verb waere
//                          erfunden -- hier compile-hart belegt statt behauptet.
//   (E) ERWEITERUNG     -- jedes Genus traegt GENAU EINE der fuenf disjunkten Op-Familien (C1).
//                          Kern + genau eine Erweiterung == das Staffelungs-Modell.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/container_framework.hpp"

#include "anatomy/set_default_organ.hpp" // SortedArrayKeySet (der Set-Kern; die Anatomien kommen
                                         // ueber GenusBindingTraits, s. u.)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace ccn = comdare::container;

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

/// Ein delegierter/getragener Achsen-Platzhalter (Muster test_e24_c1_organ_concept.cpp:68-84).
struct DelegatedAxis {};
using D = DelegatedAxis;

template <class... V>
struct ProbePermTuple {};

// Die Kompositionen kommen aus den AUTORITATIVEN GenusBindingTraits -- nicht handgebaut. Damit
// prueft diese TU denselben Bau-Pfad, den der Experiment-Baum benutzt, und eine Slot-Aenderung
// schlaegt hier durch statt an einer Attrappe vorbeizulaufen.
namespace ex = comdare::cache_engine::builder::experiment;

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
// (A) KERN 4/4 -- Identitaet + observe_axes + size ueber ALLE vier Container-Genera.
// =============================================================================================

static_assert(ccn::ContainerGattungsKern<SetOrgan>);
static_assert(ccn::ContainerGattungsKern<SequenceOrgan>);
static_assert(ccn::ContainerGattungsKern<AdapterOrgan>);
static_assert(ccn::ContainerGattungsKern<ViewOrgan>);

// Und die drei Bestandteile einzeln -- damit ein Ausfall benennbar ist statt nur "Kern kaputt".
static_assert(cea::AnatomyConcept<SetOrgan> && cea::AnatomyConcept<SequenceOrgan> &&
              cea::AnatomyConcept<AdapterOrgan> && cea::AnatomyConcept<ViewOrgan>);
static_assert(ccn::ContainerObservesAxes<SetOrgan> && ccn::ContainerObservesAxes<SequenceOrgan> &&
              ccn::ContainerObservesAxes<AdapterOrgan> && ccn::ContainerObservesAxes<ViewOrgan>);
static_assert(cea::OrganSized<SetOrgan> && cea::OrganSized<SequenceOrgan> && cea::OrganSized<AdapterOrgan> &&
              cea::OrganSized<ViewOrgan>);

// =============================================================================================
// (B) DER GESTUFTE OPTIONALE BLOCK -- 3/4, die View faellt DEKLARIERT heraus.
// =============================================================================================

static_assert(ccn::ContainerClearBlock<SetOrgan>);
static_assert(ccn::ContainerClearBlock<SequenceOrgan>);
static_assert(ccn::ContainerClearBlock<AdapterOrgan>);
static_assert(!ccn::ContainerClearBlock<ViewOrgan>,
              "D2-Doktrin: die View ist non-owning und kennt bewusst kein clear -- sie faellt aus dem "
              "OPTIONALEN Block, nicht aus dem Kern. Das ist Semantik, kein Mangel.");

static_assert(ccn::container_gattungs_kern_stufen<SetOrgan>() == 2);
static_assert(ccn::container_gattungs_kern_stufen<SequenceOrgan>() == 2);
static_assert(ccn::container_gattungs_kern_stufen<AdapterOrgan>() == 2);
static_assert(ccn::container_gattungs_kern_stufen<ViewOrgan>() == 1, "View: nur der Kern, kein optionaler Block");

// =============================================================================================
// (C) DIE MAP-GATTUNG STEHT AUSSERHALB -- und genau deshalb ist der Kern nicht groesser.
// =============================================================================================

static_assert(cea::AnatomyConcept<SearchOrgan>, "die Identitaet teilt die Map-Gattung sehr wohl");
static_assert(!cea::OrganSized<SearchOrgan>,
              "am Ist hat die SA-Anatomie KEIN size() -- deshalb ist size() ueber alle FUENF Genera "
              "nicht gemeinsam und der Container-Kern gilt nur fuer die vier Container-Genera");
static_assert(!ccn::ContainerGattungsKern<SearchOrgan>);
static_assert(!ccn::ContainerType<cea::AnatomyGenus::SearchAlgorithm>);

// =============================================================================================
// (D) KEIN VEREINHEITLICHTES OP-VERB -- die Verben sind paarweise fremd.
// =============================================================================================

static_assert(cea::KeyedOrganOps<SetOrgan>);
static_assert(!cea::KeyedOrganOps<SequenceOrgan> && !cea::KeyedOrganOps<AdapterOrgan> &&
              !cea::KeyedOrganOps<ViewOrgan>);
static_assert(cea::IndexedOrganOps<SequenceOrgan>);
static_assert(!cea::IndexedOrganOps<SetOrgan> && !cea::IndexedOrganOps<AdapterOrgan> &&
              !cea::IndexedOrganOps<ViewOrgan>);
static_assert(cea::AdaptedOrganOps<AdapterOrgan>);
static_assert(!cea::AdaptedOrganOps<SetOrgan> && !cea::AdaptedOrganOps<SequenceOrgan> &&
              !cea::AdaptedOrganOps<ViewOrgan>);
static_assert(cea::BoundViewOrganOps<ViewOrgan>);
static_assert(!cea::BoundViewOrganOps<SetOrgan> && !cea::BoundViewOrganOps<SequenceOrgan> &&
              !cea::BoundViewOrganOps<AdapterOrgan>);

} // namespace

int main() {
    std::cout << "=== E-24 C7-2 -- der Container-Gattungs-Kern (erhoben, nicht erfunden) ===\n";

    std::cout << "\n[A] KERN 4/4: Identitaet + observe_axes + size\n";
    check_true("Set erfuellt den Kern", ccn::ContainerGattungsKern<SetOrgan>);
    check_true("Sequence erfuellt den Kern", ccn::ContainerGattungsKern<SequenceOrgan>);
    check_true("Adapter erfuellt den Kern", ccn::ContainerGattungsKern<AdapterOrgan>);
    check_true("View erfuellt den Kern", ccn::ContainerGattungsKern<ViewOrgan>);

    std::cout << "\n[B] Der GESTUFTE optionale Block: 3/4\n";
    check_eq("Stufen(Set)", ccn::container_gattungs_kern_stufen<SetOrgan>(), std::size_t{2});
    check_eq("Stufen(Sequence)", ccn::container_gattungs_kern_stufen<SequenceOrgan>(), std::size_t{2});
    check_eq("Stufen(Adapter)", ccn::container_gattungs_kern_stufen<AdapterOrgan>(), std::size_t{2});
    check_eq("Stufen(View) -- non-owning, deklarierte Ausnahme", ccn::container_gattungs_kern_stufen<ViewOrgan>(),
             std::size_t{1});

    std::cout << "\n[C] Die Map-Gattung steht ausserhalb (und begrenzt damit den Kern)\n";
    check_true("SA teilt die IDENTITAET", cea::AnatomyConcept<SearchOrgan>);
    check_true("SA hat KEIN size() -- deshalb ist der Kern nicht 5/5", !cea::OrganSized<SearchOrgan>);
    check_true("SA erfuellt den Container-Kern nicht", !ccn::ContainerGattungsKern<SearchOrgan>);

    std::cout << "\n[D] Kein vereinheitlichtes Op-Verb: die Verben sind paarweise fremd\n";
    check_true("Set kennt Mengen-Verben, sonst niemand",
               cea::KeyedOrganOps<SetOrgan> && !cea::KeyedOrganOps<SequenceOrgan>);
    check_true("Sequence kennt indizierte Verben, sonst niemand",
               cea::IndexedOrganOps<SequenceOrgan> && !cea::IndexedOrganOps<AdapterOrgan>);
    check_true("Adapter kennt Adapter-Verben, sonst niemand",
               cea::AdaptedOrganOps<AdapterOrgan> && !cea::AdaptedOrganOps<ViewOrgan>);
    check_true("View kennt Sicht-Verben, sonst niemand",
               cea::BoundViewOrganOps<ViewOrgan> && !cea::BoundViewOrganOps<SetOrgan>);

    std::cout << "\n[E] Kern + GENAU EINE Erweiterung je Genus (das Staffelungs-Modell)\n";
    check_eq("Op-Familien(Set)", cea::organ_op_family_count<SetOrgan>(), std::size_t{1});
    check_eq("Op-Familien(Sequence)", cea::organ_op_family_count<SequenceOrgan>(), std::size_t{1});
    check_eq("Op-Familien(Adapter)", cea::organ_op_family_count<AdapterOrgan>(), std::size_t{1});
    check_eq("Op-Familien(View)", cea::organ_op_family_count<ViewOrgan>(), std::size_t{1});

    std::cout << "\n[F] Der Kern ist BENUTZBAR: generischer Code ueber die Staffelung\n";
    {
        // Kern-Nutzung: Identitaet + Fuellstand + Beobachtung ohne jede Genus-Kenntnis.
        SetOrgan      s{};
        SequenceOrgan q{};
        (void)s.insert(1);
        q.push_back(7);
        auto kern_bericht = []<class A>(A const& a) {
            return static_cast<std::size_t>(a.size()) + A::observable_axis_count() + A::organ_count();
        };
        check_eq("Kern-Bericht(Set) == 1 + 0 + 13", kern_bericht(s), std::size_t{14});
        check_eq("Kern-Bericht(Sequence) == 1 + 0 + 9", kern_bericht(q), std::size_t{10});
        // Der optionale Block wird per `if constexpr` gestuft genutzt -- ohne Runtime-Switch.
        auto leeren_wenn_moeglich = []<class A>(A& a) {
            if constexpr (ccn::ContainerClearBlock<A>) {
                a.clear();
                return true;
            } else {
                return false;
            }
        };
        check_true("Set laesst sich ueber den optionalen Block leeren", leeren_wenn_moeglich(s));
        check_eq("und ist danach leer", s.size(), std::size_t{0});
        ViewOrgan vw{};
        check_true("die View NICHT -- der Block fehlt ihr deklariert", !leeren_wenn_moeglich(vw));
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
