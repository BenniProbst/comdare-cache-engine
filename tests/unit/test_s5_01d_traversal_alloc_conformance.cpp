// A8-S5 Familie 01d cache_traversal (Achse 03b) -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp; DIESE TU besteht nur aus
// der Typ-Liste der Familie plus dem literalen Lauf-Ausweis (Pilot-Muster 04_execution).
//
// WAS GEPINNT WIRD: kein Organ dieser Familie fuehrt Speicher an der Allokator-Achse vorbei. Die
// Pruefung laeuft auf TYP-EBENE gegen die REALEN Kompositions-Typen -- die Liste kommt aus der
// Achsen-Registry (AllStrategies / EnabledStrategies), NICHT aus einer handgepflegten Aufzaehlung.
// Waechst die Achse um CT04/CT05, waechst die Wache mit; eine neue Strategie mit
// Default-Allokator-Container bricht diese TU sofort, statt still in den B-5-Bestand zu wachsen.
//
// -- FORM-AUSWEIS DIESER FAMILIE: B, NICHT A (und warum das eine Auflage ausloest) -----------------
// Der Pilot 04_execution kam mit Form (A) HEAP-FREI aus: die Pfad-Trajektorie hat eine COMPILE-TIME-
// Kappe (kMaxTrackedSlots), also passt der Zustand inline. Familie 01d hat diese Kappe NICHT --
// register_entry waechst unbeschraenkt (HashLookup rehasht bis 1024 Buckets und darueber hinaus,
// iterable_values() faehrt die Kapazitaeten real durch). Eine feste Inline-Kappe waere hier keine
// Schnitt-Variante, sondern eine SEMANTIK-Aenderung. Deshalb Form (B) UEBER DIE ALLOKATOR-ACHSE.
// Der Header von s5_family_alloc_conformance.hpp deklariert die Grenze der Form B ausdruecklich
// (Review-F2): sie prueft die DEKLARATION allocator_type, nicht die reale Verdrahtung. Diese TU
// erfuellt die daraus folgende Auflage "Form B braucht einen realen Verdrahtungs-Anker" auf ZWEI
// Ebenen -- compile-hart hier (Abschnitt 3: der Adapter-Typ des realen Eintrags-Containers), und zur
// LAUFZEIT in test_s5_01d_traversal_perf_sanity (traversal_allocator_statistics unter Last).
//
// -- GEPRUEFTE EBENEN ------------------------------------------------------------------------------
//   (1) die STRATEGIE-Typen selbst   -- AllStrategies + EnabledStrategies der Achse 03b
//   (2) die ORGAN-HUELLEN            -- ENTFAELLT in dieser Familie, und das ist BELEGT statt behauptet:
//       der ABI-Adapter haelt die Strategie DIREKT (abi_adapter.hpp:2206
//       "mutable typename Composition::cache_traversal ct_organ_;"). Es gibt keine
//       ObservableCacheTraversal-Huelle -- Nachweis am Ist 2026-08-04:
//         grep -rn "Observable.*[Tt]raversal" axes/cache_traversal/ topics/traversal/ --include=*.hpp
//         -> LEER (0 Treffer). Statt einer leeren Huellen-Liste (die jede Alles-Aussage wahr machte)
//       prueft Abschnitt 2 unten den REALEN Member-Typ ueber die verdrahteten Referenz-Kompositionen.
//   (3) der VERDRAHTUNGS-ANKER       -- der Adapter-Typ des realen Container-Members (Abschnitt 3)
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <axes/cache_traversal/axis_03b_cache_traversal_registry.hpp>

// Die REAL verdrahteten Kompositionen dieser Achse (nur GELESEN): sie belegen, dass die geprueften
// Strategie-Typen genau die Typen sind, die der ABI-Adapter als ct_organ_ haelt.
#include <compositions/hot_reference.hpp>
#include <compositions/wormhole_reference.hpp>

// Form-(B)-REFERENZ am realen Repo-Typ der FREMDEN Familie (nur GELESEN, Pilot-Muster): der
// B-Baum-Knoten-Pool fuehrt seinen unbounded Knoten-Speicher ueber die Allokator-Achse. Er belegt,
// dass der zweite Zweig des Praedikats an echtem Bestand traegt und nicht toter Code ist.
#include <axes/lookup/composable/btree_node_pool_store.hpp>

#include <boost/mp11.hpp>

#include <cstdio>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace mp   = boost::mp11;
namespace s5   = ::comdare::cache_engine::tests::s5;
namespace ct   = ::comdare::cache_engine::cache_traversal;
namespace comp = ::comdare::cache_engine::compositions;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

// -- (1) Strategie-Typen der Familie, direkt aus der Achsen-Registry -------------------------------
using Family01dStrategies = ct::AllStrategies;
using Family01dEnabled    = ct::EnabledStrategies;

// -- Anti-Leerlauf: eine leere Liste macht jede Alles-Aussage wahr und die Wache wertlos -----------
static_assert(mp::mp_size<Family01dStrategies>::value > 0,
              "S5-01d: die Familien-Strategie-Liste ist LEER -- eine leere Liste macht jede Alles-Aussage "
              "wahr und die Wache wertlos (Registry nicht eingebunden?).");
static_assert(mp::mp_size<Family01dEnabled>::value > 0,
              "S5-01d: die ENABLED-Liste ist leer -- dann prueft die Wache keinen einzigen real gebauten Typ.");

// -- Die Wache selbst (Typ-Ebene) -----------------------------------------------------------------
static_assert(s5::family_alloc_conform_v<Family01dStrategies>,
              "S5-01d: eine Strategie der Familie 01d cache_traversal fuehrt Speicher weder heap-frei noch ueber "
              "das Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family01dEnabled>,
              "S5-01d: eine ENABLED-Strategie der Familie 01d cache_traversal fuehrt Speicher am "
              "Allokator-Achsen-Interface vorbei (F2-Schnitt-Regel, Dossier 3.4).");

// -- (2) Der REALE Member-Typ des ABI-Adapters (Ersatz fuer die entfallende Huellen-Ebene) ---------
//    Composition::cache_traversal IST der Typ von ct_organ_ (abi_adapter.hpp:2206). Waere die hier
//    gepruefte Registry-Liste von der realen Verdrahtung entkoppelt, pruefte die Wache Typen, die in
//    keiner Tier-Binary vorkommen. Die beiden Referenz-Kompositionen belegen beide Sub-Achsen der
//    Familie (CT1 linear_access via LinearFanout, CT2 hash_access via HashLookup).
using HotTraversal      = comp::HotComposition::cache_traversal;
using WormholeTraversal = comp::WormholeComposition::cache_traversal;
static_assert(mp::mp_contains<Family01dStrategies, HotTraversal>::value,
              "S5-01d: der real verdrahtete cache_traversal-Typ der Hot-Komposition steht NICHT in der "
              "geprueften Registry-Liste -- dann pruefte die Wache an der Verdrahtung vorbei.");
static_assert(mp::mp_contains<Family01dStrategies, WormholeTraversal>::value,
              "S5-01d: der real verdrahtete cache_traversal-Typ der Wormhole-Komposition steht NICHT in der "
              "geprueften Registry-Liste -- dann pruefte die Wache an der Verdrahtung vorbei.");
static_assert(!std::is_same_v<HotTraversal, WormholeTraversal>,
              "S5-01d: beide Referenz-Kompositionen zeigen auf dieselbe Strategie -- dann deckt der "
              "Verdrahtungs-Beleg nur EINE der beiden Sub-Achsen (CT1 linear / CT2 hash) ab.");

// -- (3) VERDRAHTUNGS-ANKER: der Adapter-Typ am REALEN Container, nicht nur die using-Zeile ---------
//    Die Grenze der Form B (s5_family_alloc_conformance.hpp) ist genau der Fall "allocator_type
//    deklariert, aber Default-Allokator-Container im Member". Diese Wache schliesst ihn compile-hart:
//    sie baut den Container-Typ, den die Organe real halten, aus dem DEKLARIERTEN allocator_type auf
//    und verlangt, dass er sich vom Default-Allokator-Container UNTERSCHEIDET. Ein Organ mit blosser
//    using-Zeile haette hier zwei identische Typen.
template <class T, class Elem>
inline constexpr bool axis_bound_container_differs_from_default_v =
    !std::is_same_v<std::vector<Elem, typename T::allocator_type::template StdAllocatorAdapter<Elem>>,
                    std::vector<Elem>>;

using TraversalEntry = std::pair<ct::LinearFanout::key_type, ct::LinearFanout::value_type>;
static_assert(axis_bound_container_differs_from_default_v<ct::LinearFanout, TraversalEntry>,
              "S5-01d: der ueber allocator_type gebaute Eintrags-Container ist identisch mit dem "
              "Default-Allokator-Container -- dann waere allocator_type eine folgenlose Deklaration.");
static_assert(axis_bound_container_differs_from_default_v<ct::BinarySearchFanout, TraversalEntry>,
              "S5-01d: der ueber allocator_type gebaute Eintrags-Container ist identisch mit dem "
              "Default-Allokator-Container -- dann waere allocator_type eine folgenlose Deklaration.");
static_assert(axis_bound_container_differs_from_default_v<ct::HashLookup, TraversalEntry>,
              "S5-01d: der ueber allocator_type gebaute Bucket-Container ist identisch mit dem "
              "Default-Allokator-Container -- dann waere allocator_type eine folgenlose Deklaration.");

//    Der LAUFZEIT-Teil desselben Ankers lebt in test_s5_01d_traversal_perf_sanity
//    (traversal_allocator_statistics: 0 vor der ersten Allokation, > 0 unter Last). Hier compile-hart
//    nur, dass die Lese-Route ueberhaupt existiert -- sonst koennte die Laufzeit-Wache still entfallen.
#ifdef COMDARE_CE_ENABLE_STATISTICS
static_assert(
    requires(ct::LinearFanout const& t) { t.traversal_allocator_statistics(); },
    "S5-01d: der Laufzeit-Verdrahtungs-Anker fehlt -- ohne ihn bliebe Form B eine Deklaration.");
static_assert(
    requires(ct::BinarySearchFanout const& t) { t.traversal_allocator_statistics(); },
    "S5-01d: der Laufzeit-Verdrahtungs-Anker fehlt -- ohne ihn bliebe Form B eine Deklaration.");
static_assert(
    requires(ct::HashLookup const& t) { t.traversal_allocator_statistics(); },
    "S5-01d: der Laufzeit-Verdrahtungs-Anker fehlt -- ohne ihn bliebe Form B eine Deklaration.");
#endif

// Form (B) traegt an echtem Bestand ausserhalb dieser Familie (Gegenstueck zur Form-(A)-Positiv-Probe
// im Header) -- Pilot-Muster, nur GELESEN.
static_assert(s5::AxisAllocatorBoundOrgan<lkc::BTreeNodePoolStore<>>,
              "S5-Gate-Muster: der Form-(B)-Zweig findet am realen Referenz-Muster keinen allocator_type mehr -- "
              "dann pruefte die Wache nur noch Heap-Freiheit und liesse den Adapter-Weg ungedeckt.");

// -- Literaler Lauf-Ausweis (kein Erfolgs-Haken ohne Ausgabe) -------------------------------------
int g_fail = 0;

template <class T>
void report_organ(char const* stufe, std::string_view label) {
    bool const ok = s5::FamilyAllocConform<T>;
    std::printf("  [%s] %-8s %-24.*s Form: %s\n", ok ? " ok " : "FAIL", stufe, static_cast<int>(label.size()),
                label.data(), s5::family_alloc_form<T>());
    if (!ok) ++g_fail;
}

template <class List>
void report_list(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T = typename decltype(id)::type;
        report_organ<T>(stufe, T::name());
    });
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01d cache_traversal -- Allokator-Achsen-Konformitaet (Typ-Ebene) ==\n");
    std::printf("-- (1) Strategie-Typen aus der Achsen-Registry (AllStrategies) --\n");
    report_list<Family01dStrategies>("STRAT");
    std::printf("-- (1b) dieselbe Pruefung auf der ENABLED-Liste (real gebaute Typen) --\n");
    report_list<Family01dEnabled>("ENABL");

    std::printf("-- (2) Verdrahtung: die realen ct_organ_-Typen der Referenz-Kompositionen --\n");
    std::printf("     HotComposition::cache_traversal      = %.*s\n", static_cast<int>(HotTraversal::name().size()),
                HotTraversal::name().data());
    std::printf("     WormholeComposition::cache_traversal = %.*s\n",
                static_cast<int>(WormholeTraversal::name().size()), WormholeTraversal::name().data());
    std::printf("     (Organ-Huellen-Ebene entfaellt: der ABI-Adapter haelt die Strategie DIREKT,\n");
    std::printf("      abi_adapter.hpp:2206 -- keine ObservableCacheTraversal-Huelle im Repo.)\n");

    std::printf("-- (3) Verdrahtungs-Anker (compile-hart; Laufzeit-Haelfte in der Perf-Sanity) --\n");
    std::printf("     Eintrags-/Bucket-Container != Default-Allokator-Container: belegt (static_assert)\n");

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Strategien (All): %zu   davon ENABLED: %zu   Form der Familie: B (Allokator-Achse)\n",
                static_cast<std::size_t>(mp::mp_size<Family01dStrategies>::value),
                static_cast<std::size_t>(mp::mp_size<Family01dEnabled>::value));
    std::printf("  Form A (heap-frei) ist hier NICHT zulaessig: unbounded register_entry/rehash, keine CT-Kappe.\n");

    std::printf("== test_s5_01d_traversal_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
