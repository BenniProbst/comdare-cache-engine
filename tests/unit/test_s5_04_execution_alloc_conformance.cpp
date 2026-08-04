// A8-S5 Familie 04_execution (prefetch_axis + concurrency_axis) -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp; DIESE TU besteht nur aus
// der Typ-Liste der Familie plus dem literalen Lauf-Ausweis. Folge-Familien kopieren die Datei und
// tauschen die Registry-Includes und die drei using-Zeilen aus -- sonst nichts.
//
// WAS GEPINNT WIRD: kein Organ dieser Familie fuehrt Speicher an der Allokator-Achse vorbei. Die
// Pruefung laeuft auf TYP-EBENE gegen die REALEN Kompositions-Typen -- die Listen kommen aus den
// Achsen-Registries (AllPrefetchers / AllStrategies), NICHT aus einer handgepflegten Aufzaehlung.
// Waechst eine Achse um eine Strategie, waechst die Wache mit; eine neue Strategie mit
// Default-Allokator-Container bricht diese TU sofort, statt still in den Bestand zu wachsen.
//
// GEPRUEFTE EBENEN (alle drei, weil an jeder ein Container haengen koennte):
//   (1) die STRATEGIE-Typen selbst          -- AllPrefetchers, AllStrategies
//   (2) die ORGAN-HUELLEN der Komposition   -- ObservablePrefetch<S> / ObservableConcurrency<S>
//                                              (genau die Member-Typen pf_organ_/cc_organ_ des ABI-Adapters)
//   (3) die INNEREN Tracker-Organe          -- PathOrientedImpl (das Organ, das der Scrub angefasst hat)
//
// FAMILIE 05 (io_dispatch + persistence_target) -- NACHWEIS-FAMILIE, KEIN CODE (Beifracht dieser Scheibe):
//   grep -rlE "std::(vector|map|unordered_map)" axes/io_dispatch/ axes/persistence_target/ --include=*.hpp
//   Ergebnis am Ist 2026-08-04: LEER (0 Dateien, Exit 1). Familie 05 traegt also keinen einzigen
//   Default-Allokator-Container und braucht keinen Scrub-Commit -- der Nachweis IST die Arbeit.
//   Wird die Familie spaeter doch bebaut, ist die Wache hier das Muster: Registry-Liste eintragen, fertig.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <axes/concurrency_axis/axis_08_concurrency_observable.hpp>
#include <axes/concurrency_axis/axis_08_concurrency_registry.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_observable.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_path_oriented_impl.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_registry.hpp>

// Form-(B)-REFERENZ am realen Repo-Typ (nicht nur an einer Probe): der B-Baum-Knoten-Pool fuehrt seinen
// unbounded Knoten-Speicher ueber die Allokator-Achse (StdAllocatorAdapter-Rebind + COW-Memento). Er
// gehoert NICHT zu dieser Familie und wird hier nur GELESEN -- er belegt, dass der zweite Zweig des
// Praedikats an echtem Bestand traegt und nicht toter Code ist.
#include <axes/lookup/composable/btree_node_pool_store.hpp>

#include <boost/mp11.hpp>

#include <cstdio>
#include <string_view>

namespace mp  = boost::mp11;
namespace s5  = ::comdare::cache_engine::tests::s5;
namespace pfx = ::comdare::cache_engine::prefetch_axis;
namespace ccx = ::comdare::cache_engine::concurrency_axis;
namespace lkc = ::comdare::cache_engine::lookup::composable;

namespace {

// -- (1) Strategie-Typen der Familie, direkt aus den Achsen-Registries ----------------------------
using Family04Strategies = mp::mp_append<pfx::AllPrefetchers, ccx::AllStrategies>;

// -- (2) Organ-Huellen, wie der ABI-Adapter sie als Member haelt ----------------------------------
template <class S>
using PrefetchOrganOf = pfx::ObservablePrefetch<S>;
template <class S>
using ConcurrencyOrganOf = ccx::ObservableConcurrency<S>;
using Family04Organs     = mp::mp_append<mp::mp_transform<PrefetchOrganOf, pfx::AllPrefetchers>,
                                         mp::mp_transform<ConcurrencyOrganOf, ccx::AllStrategies>>;

// -- (3) Innere Tracker-Organe (der Scrub-Gegenstand dieser Scheibe) ------------------------------
using Family04Trackers = mp::mp_list<pfx::impl::PathOrientedImpl>;

// -- Die Wache selbst (Typ-Ebene) -----------------------------------------------------------------
static_assert(mp::mp_size<Family04Strategies>::value > 0,
              "S5-04: die Familien-Strategie-Liste ist LEER -- eine leere Liste macht jede Alles-Aussage "
              "wahr und die Wache wertlos (Registry nicht eingebunden?).");
static_assert(mp::mp_size<Family04Organs>::value == mp::mp_size<Family04Strategies>::value,
              "S5-04: zu jeder Familien-Strategie gehoert genau eine Organ-Huelle -- die Huellen-Liste ist "
              "aus der Strategie-Liste abgeleitet, nicht handgepflegt.");

static_assert(s5::family_alloc_conform_v<Family04Strategies>,
              "S5-04: eine Strategie der Familie 04_execution fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family04Organs>,
              "S5-04: eine Organ-Huelle der Familie 04_execution fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family04Trackers>,
              "S5-04: ein Tracker-Organ der Familie 04_execution fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");

// Form (B) traegt an echtem Bestand (Gegenstueck zur Form-(A)-Positiv-Probe im Header).
static_assert(s5::AxisAllocatorBoundOrgan<lkc::BTreeNodePoolStore<>>,
              "S5-Gate-Muster: der Form-(B)-Zweig findet am realen Referenz-Muster keinen allocator_type mehr -- "
              "dann pruefte die Wache nur noch Heap-Freiheit und liesse den Adapter-Weg ungedeckt.");

// -- Literaler Lauf-Ausweis (kein Erfolgs-Haken ohne Ausgabe) -------------------------------------
int g_fail = 0;

template <class T>
void report_organ(char const* stufe, std::string_view label) {
    bool const ok = s5::FamilyAllocConform<T>;
    std::printf("  [%s] %-8s %-46.*s Form: %s\n", ok ? " ok " : "FAIL", stufe, static_cast<int>(label.size()),
                label.data(), s5::family_alloc_form<T>());
    if (!ok) ++g_fail;
}

template <class List>
void report_list(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T = typename decltype(id)::type;
        if constexpr (requires { T::name(); }) {
            report_organ<T>(stufe, T::name());
        } else {
            report_organ<T>(stufe, "PathOrientedImpl (Tracker-Organ)");
        }
    });
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 04_execution -- Allokator-Achsen-Konformitaet (Typ-Ebene) ==\n");
    std::printf("-- (1) Strategie-Typen aus den Achsen-Registries --\n");
    report_list<Family04Strategies>("STRAT");
    std::printf("-- (2) Organ-Huellen (Member-Typen des ABI-Adapters) --\n");
    report_list<Family04Organs>("ORGAN");
    std::printf("-- (3) innere Tracker-Organe --\n");
    report_list<Family04Trackers>("TRACK");

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Strategien: %zu   Organ-Huellen: %zu   Tracker: %zu\n",
                static_cast<std::size_t>(mp::mp_size<Family04Strategies>::value),
                static_cast<std::size_t>(mp::mp_size<Family04Organs>::value),
                static_cast<std::size_t>(mp::mp_size<Family04Trackers>::value));
    std::printf("  Familie 05 (io_dispatch + persistence_target): grep-Nachweis LEER, kein Scrub noetig.\n");

    std::printf("== test_s5_04_execution_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
