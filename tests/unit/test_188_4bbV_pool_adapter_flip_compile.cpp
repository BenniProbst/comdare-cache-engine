// #188-4b-b-V (PHASE V, 2026-07-01) — COMPILE-VERIFIKATION des 4b-b1b container_-Flips fuer die 10 Weg-B-Pool-Familien.
//
// **Warum dieser Test (Explore+Codex konvergent 2026-07-01):** Der 4b-b1b-Flip (cache-engine cd25b9b) stellt in
// `anatomy/abi_adapter.hpp` fuer Pool-Familien-Compositions (search_algo = ROHER Wrapper mit
// organ_for_search_algo_t<S> != void, d.h. pool_family_==true) `container_t` von einem flachen SortedBinary-Spiegel
// auf `ObservableComposedContainer<organ_for_search_algo_t<S>>` (das NATIVE u64-Organ) um — via LAZY
// std::conditional_t/std::type_identity (vermeidet ObservableComposedContainer<void> fuer Nicht-Pools) — und routet
// insert/lookup/erase/T0-Observer/T0-Segment-Timing/tier_search_routes_through_store ueber `container_is_authoritative_`.
//
// BEFUND, der diesen Test noetig macht: KEIN von der GitLab-CI compiliertes Target instanziierte bisher diesen
// Pool-Zweig. `test_conformance_gate` treibt nur MapTier + KAryComposedTier (k-ary = organ==void, kein Pool);
// `test_abi_interface`/`test_config_durability`/`test_chaos_drift_gate`/`linux_perf_pmc_smoke` nutzen den ABI-Adapter
// gar nicht; und selbst das (nicht-CI) tier150-Harness komponiert search_algo = Organ-HUELLE (pool_family_==false).
// Der Flip war damit NUR Codex- + static_assert-verifiziert, NIE compiliert — genau das nur-Codex-verifizierte
// Fundament, das PHASE V (User-verbindlich, Kontext-Ende SE-17 §10) VOR 4b-c/D compile-belegt haben will.
//
// WAS dieser Test belegt: er instanziiert `SearchAlgorithmAbiAdapter<SearchAlgorithmAnatomy<PoolFlipComposition<W>>>`
// fuer ALLE 10 Pool-Familien-Wrapper W und TREIBT jeden Adapter ueber die vollen vom Flip beruehrten Routing-Pfade
// (insert/lookup/erase/size/clear + tier_observe = fill_observer_v3 + fill_segment_timing_v3). Jeder Aufruf erzwingt
// die Template-Instanziierung des jeweiligen Adapter-Members fuer die Pool-Composition → grüner literaler COMPILE-Beleg
// (faengt type_identity-Metaprogramm-/Routing-Fehler, die auf dem nur-Codex-verifizierten Fundament schlummern
// koennten — z.B. die 2 Mess-Bugs, die diese Session per Codex fielen: seg-T0-Routing + tier_clear-Reset). Der
// minimale Verhaltens-Smoke (insert→lookup-hit→erase→size→clear) ist RUN-Beleg; die tiefe std::map-Weit-Key-
// Konformitaet der 10 Organe deckt bereits test_188_4bb0 (organ-direkt) ab — hier geht es um den ADAPTER-Zweig.
//
// Standalone (plain int main, KEIN gtest) — konsistent mit den anderen contract-Stufen (test_conformance_gate).
// COMDARE_MEASUREMENT_ON=1 ist ZWINGEND: der Adapter vererbt IObservableTier NUR bei Messung-AN (abi_adapter.hpp:977)
// → sonst wuerde der tier_observe-/Segment-Timing-Pfad (fill_observer_v3 :~1010 + fill_segment_timing_v3 :~1326) gar
// nicht compiliert. Voller ADHOC-Include-Satz (abi_adapter zieht die 19 Organe + NodeChunkedStore + Boost::mp11 +
// generierte Achsen-Flags), vgl. m3v2_pmc_smoke. CI-erzwungen ueber einen eigenen contract:pool_flip-Job (.gitlab-ci.yml).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <axes/lookup/composable/organ_for_search_algo.hpp>         // organ_for_search_algo_t (pool_family_-Typ-Beleg)
#include <axes/lookup/composable/observable_composed_container.hpp> // #188-4b-DEG1: Organ-Huelle fuer den Ernte-Beleg
#include <builder/codegen/all_axes_umbrella.hpp> // volle Definitionen ALLER Achsen-Wrapper (die 9 rohen search_algo)
#include <compositions/art_reference.hpp>        // ArtComposition = Basis fuer die 18 Nicht-search-Achsen

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>
#include <type_traits>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace lk =
    ::comdare::cache_engine::lookup; // kanonischer Registry-Namespace der 10 Pool-Wrapper (wie test_188_4bb0)
namespace lkc = ::comdare::cache_engine::lookup::composable; // organ_for_search_algo_t

namespace {

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// ── Pool-Familien-Composition: ArtComposition-Basis (18 Nicht-search-Achsen IDENTISCH) mit AUSGETAUSCHTEM search_algo
//    = ROHER Pool-Wrapper (BTreeSearchAlgo etc.). Genau die Struktur der 320 generierten Permutations-Binaries (roher
//    Wrapper als T0) → organ_for_search_algo_t<W> != void → pool_family_==true → container_t nimmt den Flip-Zweig
//    ObservableComposedContainer<organ> an. Analog GridComposition (tier150_axis_grid), nur search_algo variiert. ──
//    (tier150_axis_grid entfernt 2026-07-11; Mess-System heute Code/02_messung_driver, E4-XML)
template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct PoolFlipComposition {
    using search_algo                          = SearchAlgoWrapper; // <-- ROHER Pool-Wrapper (der Flip-Ausloeser)
    using cache_traversal                      = comp::ArtComposition::cache_traversal;
    using mapping                              = comp::ArtComposition::mapping;
    using path_compression                     = comp::ArtComposition::path_compression;
    using node_type                            = comp::ArtComposition::node_type;
    using memory_layout                        = comp::ArtComposition::memory_layout;
    using allocator                            = comp::ArtComposition::allocator;
    using prefetch                             = comp::ArtComposition::prefetch;
    using concurrency                          = comp::ArtComposition::concurrency;
    using serialization                        = comp::ArtComposition::serialization;
    using value_handle                         = comp::ArtComposition::value_handle;
    using isa                                  = comp::ArtComposition::isa;
    using index_organization                   = comp::ArtComposition::index_organization;
    using io_dispatch                          = comp::ArtComposition::io_dispatch;
    using migration_policy                     = comp::ArtComposition::migration_policy;
    using filter                               = comp::ArtComposition::filter;
    using queuing_q1                           = comp::ArtComposition::queuing_q1;
    using queuing_q2                           = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "188-4b-b-V pool-flip compile probe";
    static constexpr std::string_view name     = "PoolFlipComposition";
};

// Instanziiert den ABI-Adapter fuer EINE Pool-Familie und treibt ALLE vom Flip beruehrten Routing-Pfade →
// erzwingt die Compile-Instanziierung des Pool-Zweigs + minimaler Verhaltens-Smoke.
template <class SearchAlgoWrapper>
void verify_pool_adapter_flip(char const* name) {
    using C       = PoolFlipComposition<SearchAlgoWrapper>;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    // (Typ-Beleg) der Flip TRIFFT diese Familie nur, wenn ein natives Organ gemappt ist (organ != void = pool_family_).
    static_assert(
        !std::is_same_v<lkc::organ_for_search_algo_t<SearchAlgoWrapper>, void>,
        "4b-b-V: verify_pool_adapter_flip nur fuer Pool-Familien (organ != void) — sonst greift der Flip nicht");

    std::printf("-- %s --\n", name);

    // Default-Konstruktion → das Daten-Member `container_t container_{}` erzwingt die type_identity-/conditional_t-
    // Instanziierung von container_t = ObservableComposedContainer<organ_for_search_algo_t<Wrapper>> (der Flip-Kern).
    // Heap (make_unique) wie im realen Host-Pfad (tier150): das Adapter-Objekt ist gross (19 Organe + Store).
    auto  tier = std::make_unique<Adapter>();
    auto* drv  = static_cast<an::IDriveableTier*>(tier.get());
    auto* obs  = dynamic_cast<an::IObservableTier*>(drv); // MEASUREMENT_ON → IObservableTier vererbt
    check("IObservableTier vorhanden (MEASUREMENT_ON → Observer-/Segment-Timing-Pfad compiliert)", obs != nullptr);
    if (obs == nullptr) return; // ohne Observer kein sinnvoller Weiter-Treib (sollte nie eintreten)

    auto const val_for = [](std::uint64_t k) -> std::uint64_t { return k ^ 0x9E3779B97F4A7C15ull; };

    // insert (abi_adapter.hpp:~753 Weg-B → container_) — 5 distinkte Keys, neu-Flag pruefen.
    for (std::uint64_t k = 1; k <= 5; ++k) check("insert neuer Key == true", drv->tier_insert(k, val_for(k)));
    check("tier_size == 5 nach 5 insert", drv->tier_size() == 5u);
    // insert bestehenden Key (update) → NICHT neu.
    check("re-insert (update) == false", !drv->tier_insert(3u, val_for(3u) ^ 1u));

    // lookup (abi_adapter.hpp:~837 Weg-B → container_) — Treffer + exakter Wert + Miss.
    {
        std::uint64_t out = 0;
        check("lookup Treffer fuer eingefuegten Key", drv->tier_lookup(2u, &out));
        check("lookup liefert exakten Wert", out == val_for(2u));
        check("lookup Miss fuer nie eingefuegten Key", !drv->tier_lookup(9999u, &out));
    }

    // erase (abi_adapter.hpp:~888 Weg-B → container_) — bool-Rueckgabe + Groesse.
    check("erase eingefuegter Key == true", drv->tier_erase(4u));
    check("erase bereits geloeschter Key == false", !drv->tier_erase(4u));
    check("tier_size == 4 nach einem erase", drv->tier_size() == 4u);

    // Observer + Segment-Timing (abi_adapter.hpp fill_observer_v3:~1010 + fill_segment_timing_v3:~1326 → beide auf
    // container_is_authoritative_ umgestellt; die 2 Mess-Bugs dieser Session lagen hier). tier_observe darf nicht
    // crashen; die T0-Observer-Zaehler kommen aus container_.statistics(); #188-4b-DEG1 erntet die Pool-Keys
    // im nativen container_-Organ und wird hier mit-instanziiert.
    {
        an::ComdareTierObserverSnapshot snap{};
        obs->tier_observe(&snap);
        // #188-4b-DEG1: tier_observe instanziiert fill_segment_timing_v3 und damit den Pool-Harvest via
        // container_.for_each_record; Compile-Gate reicht, kein separater Laufzeit-Check.
        check("tier_observe lief ohne Crash (Observer + Segment-Timing-Pfad compiliert + laeuft)", true);
    }

    // clear (+ container_.reset()-Pfad, 4b-b1b) — leert vollstaendig.
    drv->tier_clear();
    check("tier_size == 0 nach clear", drv->tier_size() == 0u);
    {
        std::uint64_t out = 0;
        check("lookup Miss nach clear", !drv->tier_lookup(2u, &out));
    }

    // #188-4b-DEG1 (CI-Verhaltens-Beleg): der for_each_record-Walk des NATIVEN Organs — dieselbe Huelle, die der
    // Adapter als container_ haelt — liefert die eingefuegten Records EXACTLY-ONCE (Wert inklusive). Direkt an der
    // Huelle geprueft, weil der Adapter die geernteten Keys nicht exponiert; contract:pool_flip macht den Walk damit
    // je Familie LAUFZEIT-sichtbar (die tiefe Weit-Key-Konformitaet deckt test_188_4bb0 in der Haupt-Suite).
    {
        lkc::ObservableComposedContainer<lkc::organ_for_search_algo_t<SearchAlgoWrapper>> hull;
        std::uint64_t const probe[4] = {1u, 7u, 65536u, 0xDEADBEEFull};
        for (auto k : probe) hull.insert(k, val_for(k));
        std::uint64_t     seen_mask = 0, dups = 0, bad = 0;
        std::size_t const visited = hull.for_each_record([&](std::uint64_t k, std::uint64_t v) {
            bool matched = false;
            for (int i = 0; i < 4; ++i)
                if (k == probe[i]) {
                    matched = true;
                    if (v != val_for(k)) ++bad;          // Wert-Drift = Fehler
                    if (seen_mask & (1ull << i)) ++dups; // Duplikat = Fehler
                    seen_mask |= (1ull << i);
                }
            if (!matched) ++bad; // Fremd-Record = Fehler
        });
        check("DEG-1 for_each_record: 4/4 Records exactly-once (keine Duplikate/Fremd-Records/Wert-Drift)",
              visited == 4 && seen_mask == 0xFull && dups == 0 && bad == 0);
        check("DEG-1 for_each_record: Rueckgabe == occupied_count", visited == hull.occupied_count());
    }
}

} // namespace

int main() {
    std::printf(
        "== test_188_4bbV_pool_adapter_flip_compile (PHASE V — der container_-Flip fuer 10 Pool-Familien) ==\n");

    // Alle 10 registrierten Weg-B-Pool-Familien (organ_for_search_algo-gemappt) — exakt die Menge aus test_188_4bb0
    // + axis_03a_search_algo_registry.hpp. Jede traegt ein ANDERES Organ → jede kann einen anderen type_identity-/
    // Routing-Compile-Fehler ausloesen → alle 10 durch den Adapter instanziieren.
    verify_pool_adapter_flip<lk::BinarySearchTreeSearchAlgo>("BST");
    verify_pool_adapter_flip<lk::BTreeSearchAlgo>("BTree");
    verify_pool_adapter_flip<lk::SkipListSearchAlgo>("SkipList");
    verify_pool_adapter_flip<lk::HashSearchAlgo>("Hash");
    verify_pool_adapter_flip<lk::OriginalArtSearchAlgo>("ART");
    verify_pool_adapter_flip<lk::OriginalHotSearchAlgo>("HOT");
    verify_pool_adapter_flip<lk::OriginalStartSearchAlgo>("START");
    verify_pool_adapter_flip<lk::OriginalWormholeSearchAlgo>("Wormhole");
    verify_pool_adapter_flip<lk::OriginalSurfSearchAlgo>("SuRF");
    verify_pool_adapter_flip<lk::SwissTableSearchAlgo>("SwissTable");

    std::printf("== test_188_4bbV: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
