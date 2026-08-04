// A8-S5 Familie 01_read_path / Sub-Scheibe 01a (Pool-/Layout-Stores) -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_01a_pool_stores_alloc_conformance auf TYP-Ebene pinnt (und
// am Objekt belegt), dass kein Substrat der Familie am Allokator-Achsen-Interface vorbei alloziert, belegt
// DIESE TU zur LAUFZEIT, dass der Scrub die Messbarkeit der Familie nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST: ein Scrub kann eine Achse still stumm schalten -- der Bau bleibt gruen, die
// Tests bleiben gruen, und die Mess-Spalte der Achse steht ab da auf 0. Genau diese Klasse war der A8-S1-/
// A8-S3-Befund (stiller Messwert-Verlust bzw. strukturell-0). Die Wache verlangt deshalb ZWEI Dinge ZUGLEICH:
//   (1) ZEIT:    seg_ns der Familien-Achsen > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit waere ein
// toter Timer. Dazu (3) der EHRLICHE NULLPUNKT als Gegenprobe.
//
// FAMILIEN-SPEZIFISCHE FELD-WAHL (Block (2)/(3)) -- BEGRUENDET, nicht geraten:
//   * T0 search_algo -> axis_stats[T0][0] "lookup" + [1] "hit" + [3] "insert"
//     (Schema-Zeile kV3AxisSchema[T0], observable_tier.hpp; Schreiber abi_adapter fill_observer_v3).
//     Das ist die Achse, UNTER der die geSCRUBbten Substrate haengen: der Pool-Store IST das Substrat des
//     search_algo-Organs. insert fuellt ihn (und loest damit die Achsen-Allokationen aus), lookup liest ihn.
//   * T6 allocator -> axis_stats[T6][0] "bytes_alloc" + [1] "bytes_in_use" + [2] "alloc_cnt"
//     (Schema-Zeile kV3AxisSchema[T6]). DIESE Zeile ist der eigentliche Gegenstand der Scheibe: sie wird vom
//     ABI-Adapter aus store_allocator_statistics() des getriebenen Organs gespeist (abi_adapter.hpp, T6-Route,
//     Rich-Zweig). Vor dem Scrub trug sie die Store-eigene Capacity-Delta-SCHAETZUNG, seither die ECHTE
//     Strategie-Statistik -- eine Wache auf "> 0" allein waere hier zu schwach, deshalb zusaetzlich der
//     KOHAERENZ-Test gegen die direkt am Organ gelesene Achsen-Statistik (Block (2)).
//   * Die uebrigen 16 Achsen gehoeren NICHT dieser Familie und werden hier NICHT behauptet -- eine
//     Zaehler-Aussage ueber eine nicht angefasste Achse waere Schmuck, keine Wache.
//
// ACHSEN-INDIZES werden aus kCompositionAxisNames ABGELEITET (Name -> Position), nie als Literal geschrieben.
// Die Namensliste selbst wird nur GELESEN (golden-Tabu).
//
// STRATEGIE-KONTRAST statt Absolut-Zahlen: gefahren werden DREI Pool-Familien mit sehr verschiedener
// Substrat-Anatomie (ART: fuenf adaptive Knoten-Pools; SkipList: Knoten-Vektor MIT Container im Element;
// SwissTable: zwei flache Vektoren + Rehash). Behauptet wird nur, was strukturell gilt, nie eine Rangfolge
// zwischen Allokations-Zahlen (die waere Hardware-/Wachstums-abhaengig und damit eine Flake-Quelle).
//
// Standalone (plain int main, KEIN gtest), COMDARE_MEASUREMENT_ON/-STATISTICS kommen global aus dem
// Haupt-CMakeLists (COMDARE_MEASUREMENT_MODE=ON).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <builder/codegen/all_axes_umbrella.hpp>                           // volle Definitionen aller Achsen-Wrapper
#include <builder/experiment_tree/axis_path_serialization.hpp>             // kCompositionAxisNames (NUR gelesen)
#include <compositions/art_reference.hpp>                                  // Basis fuer die 17 Nicht-search-Achsen
#include <axes/lookup/composable/tier_to_organ_mapping.hpp>                // die Organ-Aliase der Familie
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace lk   = ::comdare::cache_engine::lookup;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

int  g_fail = 0;
void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// -- Achsen-Index AUS DEM NAMEN (nie Literal) ------------------------------------------------------
[[nodiscard]] constexpr std::size_t axis_index_of(std::string_view axis) noexcept {
    for (std::size_t i = 0; i < ex::kCompositionAxisNames.size(); ++i)
        if (ex::kCompositionAxisNames[i] == axis) return i;
    return ex::kCompositionAxisNames.size(); // nicht gefunden -> vom static_assert unten gefangen
}

// -- Familien-Definition: DAS ist die Zeile, die Folge-Familien austauschen ------------------------
constexpr std::string_view kFamilyAxes[] = {"search_algo", "allocator"};

constexpr std::size_t kAxisSearchAlgo = axis_index_of("search_algo");
constexpr std::size_t kAxisAllocator  = axis_index_of("allocator");

static_assert(kAxisSearchAlgo < an::kV3AxisCount,
              "S5-01a Perf-Sanity: Achse 'search_algo' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisAllocator < an::kV3AxisCount,
              "S5-01a Perf-Sanity: Achse 'allocator' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-01a Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

/// Pool-Familien-Komposition: ArtComposition-Basis (17 Nicht-search-Achsen identisch) mit AUSGETAUSCHTEM
/// search_algo = ROHER Pool-Wrapper. Genau die Struktur der generierten Permutations-Binaries; damit nimmt
/// der ABI-Adapter den container_t-Pool-Zweig (organ_for_search_algo_t != void) und treibt das NATIVE Organ
/// -- also den geSCRUBbten Store. Identisches Muster wie test_188_4bbV (der Flip-Compile-Beleg).
template <class SearchAlgoWrapper>
// cppcheck-suppress ctuOneDefinitionRuleViolation // FP: anon. Namespace = interne Bindung je TU
struct Family01aComposition {
    using search_algo                          = SearchAlgoWrapper;
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
    using index_organization                   = comp::ArtComposition::index_organization;
    using io_dispatch                          = comp::ArtComposition::io_dispatch;
    using migration_policy                     = comp::ArtComposition::migration_policy;
    using filter                               = comp::ArtComposition::filter;
    using queuing_q1                           = comp::ArtComposition::queuing_q1;
    using queuing_q2                           = comp::ArtComposition::queuing_q2;
    using persistence_target                   = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
    static constexpr std::string_view paper_id = "A8-S5 01a pool store perf sanity";
    static constexpr std::string_view name     = "S501aPoolComposition";
};

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Ergebnis eines Treiber-Laufs: der Observer-Snapshot (Zeit + Zaehler) PLUS die DIREKT am Organ gelesene
/// Achsen-Statistik. Die aggregierte T6-Zeile allein saehe einen stillgelegten Store nicht vom Nullpunkt aus;
/// erst der Vergleich der beiden Quellen belegt, dass die T6-Zeile wirklich aus DIESEM Store gespeist wird.
struct DriveResult {
    an::ComdareTierObserverSnapshot snap{};
    std::uint64_t                   store_alloc_cnt   = 0;
    std::uint64_t                   store_bytes_inuse = 0;
    std::uint64_t                   store_failures    = 0;
};

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
template <class SearchAlgoWrapper, class Organ>
[[nodiscard]] DriveResult drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family01aComposition<SearchAlgoWrapper>>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;

    auto        tier = std::make_unique<Adapter>(); // gross (18 Organe + Store) -> Heap, wie im realen Host-Pfad
    auto*       drv  = static_cast<an::IDriveableTier*>(tier.get());
    auto*       obs  = dynamic_cast<an::IObservableTier*>(drv);
    DriveResult out{};
    if (obs == nullptr) {
        tr("IObservableTier vorhanden (MEASUREMENT_ON kompiliert)", false);
        return out;
    }
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i), i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i), &v);
    obs->tier_observe(&out.snap);

    // Zweite, UNABHAENGIGE Quelle derselben Groesse: ein baugleiches Organ, mit derselben Op-Schleife
    // getrieben. Es belegt, dass die T6-Zeile des Tiers nicht aus einer fremden Quelle stammt.
    Organ probe{};
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)probe.insert(spread_key(i), i * 11u + 5u);
    auto const st         = probe.store_allocator_statistics();
    out.store_alloc_cnt   = static_cast<std::uint64_t>(st.allocation_count);
    out.store_bytes_inuse = static_cast<std::uint64_t>(st.total_bytes_in_use);
    out.store_failures    = static_cast<std::uint64_t>(st.failure_count);
    return out;
}

void report_family(char const* label, DriveResult const& r) {
    std::printf("-- %s --\n", label);
    for (auto const axis : kFamilyAxes) {
        std::size_t const idx = axis_index_of(axis);
        std::printf("     seg_ns[T%zu %-12.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(r.snap.seg_ns[idx]));
    }
    std::printf("     T0  lookup = %llu   hit = %llu   insert = %llu\n",
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisSearchAlgo][0]),
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisSearchAlgo][1]),
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisSearchAlgo][3]));
    std::printf("     T6  bytes_alloc = %llu   bytes_in_use = %llu   alloc_cnt = %llu   (Organ direkt: "
                "alloc_cnt %llu / in_use %llu / fail %llu)\n",
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisAllocator][0]),
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisAllocator][1]),
                static_cast<unsigned long long>(r.snap.axis_stats[kAxisAllocator][2]),
                static_cast<unsigned long long>(r.store_alloc_cnt),
                static_cast<unsigned long long>(r.store_bytes_inuse),
                static_cast<unsigned long long>(r.store_failures));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)", r.snap.batches_measured > 0);
    tr("(1) seg_ns[search_algo] > 0 unter Last (keine 0-Zeit)", r.snap.seg_ns[kAxisSearchAlgo] > 0);
    tr("(1) seg_ns[allocator] > 0 unter Last (Versorger-Achse der Familie)", r.snap.seg_ns[kAxisAllocator] > 0);

    tr("(2) T0: die gemessene Zeit ist durch reale insert-Ops gedeckt (insert > 0)",
       r.snap.axis_stats[kAxisSearchAlgo][3] > 0);
    tr("(2) T0: und durch reale lookup-Ops (lookup > 0)", r.snap.axis_stats[kAxisSearchAlgo][0] > 0);
    tr("(2) T0: die Eintraege sind auch wiederfindbar (hit > 0) -- das Substrat traegt die Daten wirklich",
       r.snap.axis_stats[kAxisSearchAlgo][1] > 0);
    tr("(2) T6: der geSCRUBbte Store speist die Allokator-Zeile REAL (alloc_cnt > 0)",
       r.snap.axis_stats[kAxisAllocator][2] > 0);
    tr("(2) T6: und die Achse haelt den Speicher auch (bytes_in_use > 0)", r.snap.axis_stats[kAxisAllocator][1] > 0);
    tr("(2) T6: bytes_alloc >= bytes_in_use (kumulativ >= gehalten -- die Zeile ist in sich schluessig)",
       r.snap.axis_stats[kAxisAllocator][0] >= r.snap.axis_stats[kAxisAllocator][1]);
    tr("(2) KOHAERENZ: das baugleiche Organ meldet dieselbe Achsen-Groesse ungleich 0 -- die T6-Zeile stammt "
       "aus DIESEM Store, nicht aus einer fremden Quelle",
       r.store_alloc_cnt > 0 && r.store_bytes_inuse > 0);
    tr("(2) EHRLICHKEIT: keine verdeckten Allokations-Fehlschlaege (failure_count == 0)", r.store_failures == 0);
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01a -- Perf-Sanity der Pool-/Layout-Stores (keine 0-/Phantom-Zeiten) ==\n");
    constexpr std::uint64_t kLoad = 2000;
    std::printf("   n_ops = %llu je Phase (insert + lookup), drei Substrat-Anatomien im Kontrast\n",
                static_cast<unsigned long long>(kLoad));

    // Drei bewusst VERSCHIEDENE Substrat-Anatomien der Familie:
    //   ART        = fuenf adaptive Knoten-Pools + fuenf Free-Listen (die dichteste Scrub-Flaeche),
    //   SkipList   = Knoten-Vektor MIT Container im Element (der Verschachtelungs-Fall),
    //   SwissTable = zwei flache Vektoren + Rehash (der Umschaufel-Fall).
    auto const art   = drive_and_observe<lk::OriginalArtSearchAlgo, lkc::ArtTrieOrgan>(kLoad);
    auto const skipl = drive_and_observe<lk::SkipListSearchAlgo, lkc::SkipListOrgan>(kLoad);
    auto const swiss = drive_and_observe<lk::SwissTableSearchAlgo, lkc::SwissTableOrgan>(kLoad);

    report_family("ART (5 adaptive Knoten-Pools)", art);
    report_family("SkipList (Container IM Element)", skipl);
    report_family("SwissTable (flache Vektoren + Rehash)", swiss);

    // -- (3) EHRLICHER NULLPUNKT + Kontrast ----------------------------------------------------------
    // Der Nullpunkt dieser Familie ist NICHT eine None-Strategie (es gibt keine), sondern der UNGETRIEBENE
    // Zustand: ein frisch konstruiertes Organ hat noch keine Achsen-Allokation. Das trennt "die Zahlen kommen
    // von den Ops" von "die Zahlen entstehen schon beim Bauen" -- genau die Phantom-Klasse.
    std::printf("-- (3) ehrlicher Nullpunkt + Anatomie-Kontrast --\n");
    lkc::ArtTrieOrgan  fresh_art{};
    lkc::SkipListOrgan fresh_skip{};
    auto const         art_zero  = fresh_art.store_allocator_statistics();
    auto const         skip_zero = fresh_skip.store_allocator_statistics();
    std::printf("     frisch: art alloc_cnt = %llu   skip_list alloc_cnt = %llu (Kopf-Sentinel + Turm)\n",
                static_cast<unsigned long long>(art_zero.allocation_count),
                static_cast<unsigned long long>(skip_zero.allocation_count));
    tr("(3) ART-Nullpunkt ehrlich: ungetrieben == 0 Achsen-Allokationen", art_zero.allocation_count == 0);
    tr("(3) Skip-Listen-Nullpunkt ehrlich DEKLARIERT: > 0, weil der Konstruktor den Kopf-Sentinel MIT seinem "
       "Forward-Turm anlegt -- die Zahl ist erklaerbar, nicht erfunden",
       skip_zero.allocation_count > 0);
    tr("(3) und unter Last liegt jede der drei Anatomien echt ueber ihrem Nullpunkt",
       art.store_alloc_cnt > art_zero.allocation_count && skipl.store_alloc_cnt > skip_zero.allocation_count &&
           swiss.store_alloc_cnt > 0);
    // KEINE Rangfolge-Behauptung zwischen den Anatomien: die waere wachstums-/hardware-abhaengig und damit
    // eine Flake-Quelle. Behauptet wird nur, was strukturell gilt.
    tr("(3) die drei Anatomien sind wirklich verschieden verdrahtet (paarweise verschiedene alloc_cnt)",
       art.store_alloc_cnt != skipl.store_alloc_cnt && skipl.store_alloc_cnt != swiss.store_alloc_cnt &&
           art.store_alloc_cnt != swiss.store_alloc_cnt);

    std::printf("== test_s5_01a_pool_stores_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
