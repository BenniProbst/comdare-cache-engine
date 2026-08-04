// A8-S5 Familie 04_execution (prefetch_axis + concurrency_axis) -- PERF-SANITY-HARNESS.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity (keine 0-/Phantom-Zeiten)").
// Zweite Haelfte des GATE-MUSTERS: waehrend test_s5_04_execution_alloc_conformance auf TYP-Ebene pinnt,
// dass kein Familien-Organ am Allokator-Achsen-Interface vorbei alloziert, belegt DIESE TU zur LAUFZEIT,
// dass der Scrub die Messbarkeit der Familie nicht beschaedigt hat.
//
// WARUM DIESE WACHE NOETIG IST: ein Scrub kann eine Achse still stumm schalten -- der Bau bleibt gruen,
// die Tests bleiben gruen, und die Mess-Spalte der Achse steht ab da auf 0. Genau diese Klasse war der
// A8-S1-/A8-S3-Befund (stiller Messwert-Verlust bzw. strukturell-0). Die Wache verlangt deshalb ZWEI
// Dinge ZUGLEICH, nie nur eines:
//   (1) ZEIT:    seg_ns der Familien-Achsen > 0 unter Last.
//   (2) ZAEHLER: die Observer-Zaehler DERSELBEN Op-Schleife > 0.
// Zeit ohne Zaehler waere eine Phantom-Zeit (der Timer misst eine leere Schleife); Zaehler ohne Zeit
// waere ein toter Timer. Nur beides zusammen heisst "die Achse wird real durchgemessen".
// Dazu (3) der EHRLICHE NULLPUNKT als Gegenprobe: die None-Strategie MUSS 0 reale Prefetches melden --
// eine Wache, die auch bei der 0-Overhead-Strategie ">0" saehe, misst das Framework, nicht die Achse.
//
// ACHSEN-INDIZES werden aus kCompositionAxisNames ABGELEITET (Name -> Position), nie als Literal
// geschrieben: verschiebt sich die Achsen-Ordnung, wandert die Wache mit, statt still die falsche
// Spalte zu pruefen. Die Namensliste selbst wird nur GELESEN (golden-Tabu).
//
// WIEDERVERWENDUNG durch Folge-Familien: kFamilyAxes (die Namen) + die beiden variierten
// Kompositions-Slots austauschen; der Rest -- Treiber, Kommensurabilitaets-Pruefung, Ausgabe -- bleibt.
// Der familien-SPEZIFISCHE Teil ist unten als solcher ausgewiesen (Block (2)/(3)).
//
// Standalone (plain int main, KEIN gtest), COMDARE_MEASUREMENT_ON/-STATISTICS kommen global aus dem
// Haupt-CMakeLists (COMDARE_MEASUREMENT_MODE=ON).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (NUR gelesen)
#include <compositions/hot_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <axes/concurrency_axis/axis_08_concurrency_blocking.hpp>
#include <axes/concurrency_axis/axis_08_concurrency_none.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_none.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_observable.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_path_oriented.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_real_descent.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace pfx   = ::comdare::cache_engine::prefetch_axis;
namespace ccx   = ::comdare::cache_engine::concurrency_axis;

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
constexpr std::string_view kFamilyAxes[] = {"prefetch", "concurrency"};

constexpr std::size_t kAxisPrefetch    = axis_index_of("prefetch");
constexpr std::size_t kAxisConcurrency = axis_index_of("concurrency");

static_assert(kAxisPrefetch < an::kV3AxisCount,
              "S5-04 Perf-Sanity: Achse 'prefetch' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(kAxisConcurrency < an::kV3AxisCount,
              "S5-04 Perf-Sanity: Achse 'concurrency' steht nicht mehr in kCompositionAxisNames -- die Wache "
              "wuerde eine falsche seg_ns-Spalte pruefen.");
static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
              "S5-04 Perf-Sanity: Achsen-Namensliste und Snapshot-Achsenzahl sind auseinandergelaufen.");

// -- Store-getragene Komposition: Array256 (Byte-Domaene) + Node4/CLA-Backing, prefetch/concurrency variabel.
//    Nur ueber ein REAL befuelltes Store-Backing entsteht ein echter Descent (slot_count()!=0) -- eine
//    Huellen-Komposition wuerde den T7-Treiber blockieren und die Wache um ihre Aussage bringen.
template <class PFStrategy, class CCStrategy>
using Family04Composition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::HotComposition::cache_traversal, comp::HotComposition::mapping,
    comp::HotComposition::path_compression, comp::HotComposition::node_type, comp::HotComposition::memory_layout,
    comp::HotComposition::allocator, PFStrategy, CCStrategy, comp::HotComposition::serialization,
    comp::HotComposition::value_handle, comp::HotComposition::index_organization, comp::HotComposition::io_dispatch,
    comp::HotComposition::migration_policy, comp::HotComposition::filter, comp::HotComposition::queuing_q1,
    comp::HotComposition::queuing_q2, ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

[[nodiscard]] std::uint64_t spread_key(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 17) ^ 0x9E3779B97F4A7C15ull;
}

/// Treibt ein echtes Tier unter Last (insert + lookup) und zieht EINEN Observer-Snapshot.
template <class PFStrategy, class CCStrategy>
[[nodiscard]] an::ComdareTierObserverSnapshot drive_and_observe(std::uint64_t n_ops) {
    using Anatomy = an::SearchAlgorithmAnatomy<Family04Composition<PFStrategy, CCStrategy>>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    an::ComdareTierObserverSnapshot        snap{};
    if (drv == nullptr || obs == nullptr) {
        tr("IDriveableTier + IObservableTier vorhanden (Messung-AN kompiliert)", false);
        return snap;
    }
    // Byte-Domaene: Array256 ist DirectAddress-treu; ungeklemmte u64-Keys wuerden abgelehnt und das
    // Store-Backing bliebe leer (slot_count()==0 -> kein Descent -> die Wache saehe eine leere Achse).
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_insert(spread_key(i) & 0xFFull, i * 11u + 5u);
    std::uint64_t v = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)drv->tier_lookup(spread_key(i) & 0xFFull, &v);
    obs->tier_observe(&snap);
    return snap;
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 04_execution -- Perf-Sanity (keine 0-/Phantom-Zeiten) ==\n");
    constexpr std::uint64_t kLoad = 2000;

    // Familien-Besetzung unter Last: die geSCRUBbte Pfad-Trajektorie + ein echtes Sync-Primitiv-Paar.
    auto const path_blocking = drive_and_observe<pfx::PathOrientedPrefetch, ccx::BlockingConcurrency>(kLoad);
    // Gegenprobe fuer den ehrlichen Nullpunkt der prefetch-Achse.
    auto const none_none = drive_and_observe<pfx::NonePrefetch, ccx::NoneConcurrency>(kLoad);

    std::printf("-- (1) ZEIT: seg_ns der Familien-Achsen unter Last (n_ops=%llu je Phase) --\n",
                static_cast<unsigned long long>(kLoad));
    for (auto const axis : kFamilyAxes) {
        std::size_t const  idx = axis_index_of(axis);
        std::int64_t const ns  = path_blocking.seg_ns[idx];
        std::printf("     seg_ns[T%zu %-12.*s] = %lld ns\n", idx, static_cast<int>(axis.size()), axis.data(),
                    static_cast<long long>(ns));
    }
    std::printf("     seg_run_total_ns = %lld ns   batches_measured = %llu\n",
                static_cast<long long>(path_blocking.seg_run_total_ns),
                static_cast<unsigned long long>(path_blocking.batches_measured));

    tr("(1) der Segment-Lauf hat ueberhaupt stattgefunden (batches_measured > 0)", path_blocking.batches_measured > 0);
    for (auto const axis : kFamilyAxes) {
        std::size_t const idx = axis_index_of(axis);
        if (axis == std::string_view{"prefetch"})
            tr("(1) seg_ns[prefetch] > 0 unter Last (keine 0-Zeit)", path_blocking.seg_ns[idx] > 0);
        else
            tr("(1) seg_ns[concurrency] > 0 unter Last (keine 0-Zeit)", path_blocking.seg_ns[idx] > 0);
    }
    tr("(1) Kommensurabel: Summe der Familien-Segmente <= seg_run_total_ns (kein Zeit-Ueberlauf)",
       path_blocking.seg_ns[kAxisPrefetch] + path_blocking.seg_ns[kAxisConcurrency] <= path_blocking.seg_run_total_ns);

    // -- (2) FAMILIEN-SPEZIFISCH: die Zaehler DERSELBEN Op-Schleife (Anti-Phantom) -------------------
    //    T7-Feld 5 = real_prefetches_issued, T8-Feld 0/1 = acquire/release (abi_adapter fill_observer_v3;
    //    Schema-Zeilen kV3AxisSchema[T7]/[T8]).
    std::uint64_t const pf_issued  = path_blocking.axis_stats[kAxisPrefetch][5];
    std::uint64_t const cc_acquire = path_blocking.axis_stats[kAxisConcurrency][0];
    std::uint64_t const cc_release = path_blocking.axis_stats[kAxisConcurrency][1];
    std::printf("-- (2) ZAEHLER derselben Op-Schleife (Anti-Phantom) --\n");
    std::printf("     T7 real_prefetches_issued = %llu\n", static_cast<unsigned long long>(pf_issued));
    std::printf("     T8 acquire = %llu   release = %llu\n", static_cast<unsigned long long>(cc_acquire),
                static_cast<unsigned long long>(cc_release));
    tr("(2) T7: die gemessene Zeit ist durch reale Prefetch-Ops gedeckt (issued > 0)", pf_issued > 0);
    tr("(2) T8: die gemessene Zeit ist durch reale, gepaarte Sync-Primitive gedeckt (acquire>0, acquire==release)",
       cc_acquire > 0 && cc_acquire == cc_release);

    // -- (3) FAMILIEN-SPEZIFISCH: der ehrliche Nullpunkt als Gegenprobe -----------------------------
    std::uint64_t const none_issued = none_none.axis_stats[kAxisPrefetch][5];
    std::printf("-- (3) ehrlicher Nullpunkt (Gegenprobe) --\n");
    std::printf("     None-Prefetch real_prefetches_issued = %llu   (PathOriented: %llu)\n",
                static_cast<unsigned long long>(none_issued), static_cast<unsigned long long>(pf_issued));
    tr("(3) NonePrefetch meldet 0 reale Prefetches (deklarierter Nullpunkt, kein erfundener Wert)", none_issued == 0);
    tr("(3) Strategie-Kontrast: PathOriented > None -- die Zeit haengt an der Achse, nicht am Framework",
       pf_issued > none_issued);

    std::printf("== test_s5_04_execution_perf_sanity: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
