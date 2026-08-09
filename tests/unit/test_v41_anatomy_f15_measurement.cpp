// V41.F.6.1 F15 — Mess-Treiber (Stufe A): lädt generierte Permutations-DLLs + misst+vergleicht.
//
// KONZEPT (Anatomie-Metapher, User-Direktive 2026-05-29 — siehe [[std-map-unified-interface]]):
// Die Anatomie-Metapher (Tiere) DEFINIERT + ZERLEGT Algorithmen, damit sie überhaupt VERGLEICHBAR
// werden. Dafür wird jeder Such-Algorithmus auf EIN einheitliches Interface zusammengeschnitten —
// bei (key,value) ist das IMMER `std::map<key,value>`. Die 17 ACHSEN (Organe) beschreiben, WIE sich
// das INNERE dieser `std::map<key,value>` verhält (dense-Array / sortierter Vektor / ART / B+ / …) —
// und genau das wirkt sich auf die Performance aus.
//
// => Zwei `std::map<key,value>` zu vergleichen ist NICHT hohl, sondern das KERNZIEL der Arbeit (F15):
//    identisches Vergleichs-Interface, unterschiedliches Innen-Verhalten durch Achsen-Wahl →
//    MESSBAR unterschiedliche Performance. Bringt die CacheEngine messbaren Wert = bringt die
//    Achsen-Konfiguration des `std::map`-Innenlebens messbaren Wert.
//
// Die Messung ist ZWEIDIMENSIONAL (Join, User-Direktive 2026-05-29):
//   - DIMENSION 1 (ACHSEN-EBENE): einzelne Achsen-Werte gegeneinander — z.B. zwei search_algo-Werte
//     (Array256 dense O(1) vs VectorU8U8 sortiert O(log n)). „Wie verhält sich das Innere bei
//     gleicher Achse, anderem Wert?"
//   - DIMENSION 2 (ALGORITHMUS-EBENE): ganze Algorithmen (volle Kompositionen) gegeneinander — z.B.
//     ArtComposition vs HotComposition, jeweils über ihr `C::search_algo` (die dominante
//     `std::map`-Lookup-Struktur der Komposition). „Welcher GESAMTE Algorithmus ist schneller?"
//   Beide Dimensionen vergleichen Implementierungen DESSELBEN `std::map<key,value>`-Interface mit
//   unterschiedlichem Innen-Verhalten — via echtem Welch-T-Test auf der Lookup-Latenz.
//
// Dieser Treiber demonstriert die Mess-Pipeline end-to-end an realem Code:
//   (0) PIPELINE: lädt die per Codegen erzeugten Permutations-DLLs (load_all, R5.I-Pattern), liest
//       Identität (composition_name) + fährt den Lifecycle (warm_up/run/reset/shutdown).
//   (1) DIM 1 ACHSEN-EBENE: Array256SearchAlgo vs VectorU8U8SearchAlgo (search_algo-Achsen-Werte).
//   (2) DIM 2 ALGORITHMUS-EBENE: ArtComposition vs HotComposition (volle Algorithmen via C::search_algo).
//
// EHRLICHE technische Grenzen (code-belegt, KEIN Overclaim):
//   - Die geladene DLL exponiert via IAnatomyBase NUR Metadaten + Lifecycle (warm_up/run/reset/shutdown
//     = state_-Flips, abi_adapter.hpp), KEINE CRUD-ABI durch die DLL-Grenze → die gemessene Last läuft
//     host-seitig über die (compile-time-bekannten) Achsen-Implementierungen. Last DURCH die geladene
//     DLL erfordert eine additive ABI-Methode (Stufe B: virtuelle IAnatomyBase::run_workload, R6).
//   - Latenz = steady_clock-Wall-Time (keine Hardware-Counter/PMC).

#include <gtest/gtest.h>

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/measurable_workload.hpp> // Stufe B: Mess-Last DURCH die geladene DLL
#include <anatomy/abi_adapter.hpp>         // R6/Pfad B: SearchAlgorithmAbiAdapter + IObservableTier
#include <anatomy/composition_factory.hpp> // #188-4c-i-T: store-backed AdHoc-Kontrollkomposition
#include <anatomy/observable_tier.hpp>     // R6: ABI-stabiler Observer-Snapshot-POD
#include <anatomy/search_algorithm_anatomy.hpp>
#include <cstring> // memcpy (ABI-Stabilitaets-Roundtrip)
#include <builder/commands/welch_t_test.hpp>
#include <builder/commands/multiple_comparison.hpp> // R6: FWER-Korrektur bei vielen Vergleichen
#include <builder/commands/result_aggregator.hpp>   // R5.E: Mess-Ergebnisse → CSV/JSON
#include <builder/commands/latency_stats.hpp>       // R5.E: Latenz-Perzentile (geteilt, non-mutierend)
#include <builder/commands/multi_compare.hpp>       // R6: N Kompositionen vs Baseline, FWER-kontrolliert
#include <builder/commands/lade_bilanz.hpp>         // D4e: Bilanz des Mess-Laufs (Nenner + Summenprobe)

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp>
// Dim 2 (Algorithmus-Ebene): volle Kompositionen, gemessen über C::search_algo
#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>                    // 3. Komposition (echtes Masstree-Organ)
#include <builder/anatomy_commands/anatomy_execution_context.hpp> // Saeule-2: Per-Achsen-observe_all-Statistik
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>    // R6 Inkrement 2: ABI-Fuellstands-Treiber

#include <chrono>
#include <cmath>  // D4a: std::isfinite / std::nan im Degenerations-Praedikat-Test
#include <cstdint>
#include <filesystem>
#include <limits> // D4a: numeric_limits<double>::infinity() als zweiter nicht-endlicher Eingang
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace stats  = ::comdare::cache_engine::builder::commands::stats;
namespace ce03a  = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace comp   = ::comdare::cache_engine::compositions;

using StoreBackedAdHocComposition = ::comdare::cache_engine::anatomy::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
    comp::ArtComposition::queuing_q1, comp::ArtComposition::queuing_q2,
    /* STRUKT-R ORG-18: T17 persistence_target, expliziter Durchreich-Wert */
    ::comdare::cache_engine::persistence_target::MemoryOnlyTarget>;

namespace {

/// Misst Batch-Lookup-Latenzen (ns) einer Such-Datenstruktur (= Innenleben einer std::map<key,value>)
/// mit gleichem 256-Key-Workload. Generisch über `Algo::key_type` (uint8/uint16). Batch-Granularität
/// (ops_per_batch Lookups/Sample) statt Einzel-Op, weil ein direkter Lookup unter der steady_clock-
/// Auflösung liegt. Liefert `batches` Latenz-Samples.
template <class Algo>
std::vector<std::int64_t> measure_lookup_batches(int batches, int ops_per_batch, std::uint64_t seed) {
    using K = typename Algo::key_type;
    Algo algo;
    for (int k = 0; k < 256; ++k) { algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u); }
    std::mt19937_64 rng{seed};
    auto            do_batch = [&]() -> std::int64_t {
        std::uint64_t sink = 0;
        auto const    t0   = std::chrono::steady_clock::now();
        for (int i = 0; i < ops_per_batch; ++i) {
            auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
            if (v) sink += *v;
        }
        auto const t1 = std::chrono::steady_clock::now();
        auto       ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        // sink-Nutzung verhindert Wegoptimierung der Lookups.
        return (sink == ~0ull) ? (ns ^ 1) : ns;
    };
    do_batch(); // Warmup (verworfen)
    std::vector<std::int64_t> samples;
    samples.reserve(static_cast<std::size_t>(batches));
    for (int b = 0; b < batches; ++b) samples.push_back(do_batch());
    return samples;
}

/// Welch-Vergleich zweier Mess-Reihen + Verdikt-Log. label_a/label_b für die F15-Ausgabe.
stats::WelchResult welch_compare(std::vector<std::int64_t> const& a, std::vector<std::int64_t> const& b,
                                 char const* dim, char const* label_a, char const* label_b) {
    auto const  w       = stats::welch_t_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    bool const  sig     = w.valid && w.p_value < 0.05;
    char const* verdict = !sig ? "Tie/Inconclusive" : (w.mean_a < w.mean_b ? label_a : label_b);
    std::cout << "[F15 " << dim << "] " << label_a << " mean=" << w.mean_a << "ns vs " << label_b
              << " mean=" << w.mean_b << "ns  t=" << w.t_statistic << " p=" << w.p_value << "  => " << verdict
              << " schneller\n";
    return w;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// (1) Pipeline: generierte Permutations-DLLs laden + Lifecycle fahren (R5.I-Pattern)
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, LoadsGeneratedPermutationDllsAndRunsLifecycle) {
    std::filesystem::path const              dir{COMDARE_F15_MEASUREMENT_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    int const                                st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_GE(handles.size(), 1u) << "Keine Permutations-DLLs geladen.";

    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_FALSE(a->composition_name().empty()); // Identität aus der DLL
        EXPECT_EQ(a->organ_count(), 18u);            // volle Anatomie (18 Achsen, STRUKT-R ORG-18)
        // Lifecycle (state-Flips; keine Daten — s. Datei-Kopf): muss ohne Crash durchlaufen.
        a->warm_up();
        a->run();
        a->reset();
        a->shutdown();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// (1) DIMENSION 1 — ACHSEN-EBENE: einzelne search_algo-Achsen-Werte gegeneinander
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, Dim1_AxisLevel_SearchAlgoValues) {
    constexpr int kBatches     = 200;
    constexpr int kOpsPerBatch = 4000;
    auto          dense        = measure_lookup_batches<ce03a::Array256SearchAlgo>(kBatches, kOpsPerBatch, 0xF15u);
    auto          sorted       = measure_lookup_batches<ce03a::VectorU8U8SearchAlgo>(kBatches, kOpsPerBatch, 0xF15u);
    ASSERT_EQ(dense.size(), static_cast<std::size_t>(kBatches));
    ASSERT_EQ(sorted.size(), static_cast<std::size_t>(kBatches));

    auto const w = welch_compare(dense, sorted, "DIM1-Achse", "Array256(dense)", "VectorU8U8(sorted)");
    // F15-Nachweis = gültiges statistisches Verdikt (NICHT eine fixe, HW-abhängige Reihenfolge).
    ASSERT_TRUE(w.valid);
    EXPECT_GT(w.mean_a, 0.0);
    EXPECT_GT(w.mean_b, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// (2) DIMENSION 2 — ALGORITHMUS-EBENE: ganze Kompositionen gegeneinander
//     (jeweils über ihr C::search_algo = dominante std::map-Lookup-Struktur)
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, Dim2_AlgorithmLevel_WholeCompositions) {
    constexpr int kBatches     = 200;
    constexpr int kOpsPerBatch = 4000;
    // Volle Algorithmen: ArtComposition vs HotComposition, gemessen über ihr search_algo-Innenleben.
    auto art = measure_lookup_batches<typename comp::ArtComposition::search_algo>(kBatches, kOpsPerBatch, 0xA27u);
    auto hot = measure_lookup_batches<typename comp::HotComposition::search_algo>(kBatches, kOpsPerBatch, 0xA27u);
    ASSERT_EQ(art.size(), static_cast<std::size_t>(kBatches));
    ASSERT_EQ(hot.size(), static_cast<std::size_t>(kBatches));

    auto const w = welch_compare(art, hot, "DIM2-Algorithmus", "ArtComposition", "HotComposition");
    ASSERT_TRUE(w.valid);
    EXPECT_GT(w.mean_a, 0.0);
    EXPECT_GT(w.mean_b, 0.0);
    // HINWEIS: Dim 2 misst aktuell die dominante search_algo-Struktur der Komposition. Sobald weitere
    // Achsen ins std::map-Innenleben routen (R5.B), erfasst dieselbe Mess-Stelle den vollen Algorithmus.
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) STUFE B — Mess-Last DURCH die geladene DLL (IMeasurableWorkload-Sub-Interface)
//     Die Last läuft jetzt IN der DLL (auf deren eigener Komposition), nicht host-seitig.
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, StufeB_InDllWorkloadThroughLoadedComposition) {
    namespace an = ::comdare::cache_engine::anatomy;
    std::filesystem::path const              dir{COMDARE_F15_MEASUREMENT_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    ASSERT_EQ(loader::AnatomyModuleLoader::load_all(dir, handles), loader::status_ok);
    ASSERT_GE(handles.size(), 2u);

    constexpr std::uint64_t kBatches = 200, kOps = 4000;
    struct Run {
        std::string               name;
        std::vector<std::int64_t> samples;
    };
    std::vector<Run> runs;
    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        // Sub-Interface via dynamic_cast abfragen (ABI-sicher: alte DLLs → nullptr).
        auto* mw = dynamic_cast<an::IMeasurableWorkload*>(a);
        ASSERT_NE(mw, nullptr) << "Geladene DLL implementiert IMeasurableWorkload nicht.";
        std::vector<std::int64_t> s(static_cast<std::size_t>(kBatches));
        auto const                n = mw->run_workload(kOps, kBatches, 0xB15u, s.data(), s.size());
        ASSERT_EQ(n, kBatches); // Last lief IN der DLL
        runs.push_back(Run{std::string{a->composition_name()}, std::move(s)});
    }

    // Welch-Vergleich der ersten beiden Kompositionen — in-DLL gemessen (volle Stufe B).
    auto const w =
        welch_compare(runs[0].samples, runs[1].samples, "DIM2-inDLL", runs[0].name.c_str(), runs[1].name.c_str());
    ASSERT_TRUE(w.valid);
    EXPECT_GT(w.mean_a, 0.0);
    EXPECT_GT(w.mean_b, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// (4) SAEULE 2 — PER-ACHSEN-observe_all-STATISTIK (komplementaer zur Saeule-1-Wall-Clock oben)
//     Zweite Mess-Dimension im f15-Treiber: jede Komposition treibt + misst ihr ECHTES seziertes
//     search_algo-Organ (ART/HOT/Masstree) ueber AnatomyExecutionContext::observe_all() — seit der
//     #42-Umstufung + Saeule-2-Mess-Treue NICHT mehr generisch SortedBinary (Doku 24 §5.2).
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, Saeule2_PerAxisStatisticsTraceRealOrgan) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    namespace ac           = ::comdare::cache_engine::builder::anatomy_commands;
    auto drive_and_observe = []<class C>() {
        ac::AnatomyExecutionContext<C> ctx;
        constexpr std::uint64_t        N = 5000;
        for (std::uint64_t i = 0; i < N; ++i) {
            std::uint64_t const k = (i * 2654435761u) % 100000u;
            ctx.insert(k, k + 1);
        }
        for (std::uint64_t i = 0; i < N; ++i) {
            std::uint64_t const k = (i * 2654435761u) % 100000u;
            (void)ctx.lookup(k);
        }
        return ctx.observe_all();
    };
    auto const art_agg  = drive_and_observe.template operator()<comp::ArtComposition>();
    auto const hot_agg  = drive_and_observe.template operator()<comp::HotComposition>();
    auto const mass_agg = drive_and_observe.template operator()<comp::MasstreeComposition>();

    // Per-Achsen-Statistik (Saeule 2) ist ECHT/nicht-leer: jede Komposition hat ihr eigenes Organ getrieben.
    EXPECT_GT(art_agg.search_algo.total_insert_count, 0u);
    EXPECT_GT(art_agg.search_algo.total_lookup_count, 0u);
    EXPECT_GT(art_agg.search_algo.peak_occupancy, 0u);
    EXPECT_GT(hot_agg.search_algo.total_lookup_count, 0u);
    EXPECT_GT(mass_agg.search_algo.total_lookup_count, 0u);
    // Lookup-Zaehler == Anzahl Lookups (Workload-getrieben, organ-unabhaengig deterministisch).
    EXPECT_EQ(art_agg.search_algo.total_lookup_count, 5000u);
    EXPECT_EQ(mass_agg.search_algo.total_lookup_count, 5000u);
    SUCCEED();
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus — Saeule-2-Trace n/a";
#endif
}

// R6 / Pfad B (Doku 24 §8.6, HYBRID-Modell): host-seitiger Observer-Zugriff über das ABI-stabile
// IObservableTier. Belegt den vollständigen Mess-Ablauf, den die CacheEngineBuilder über die Modul-Binary-
// Grenze fährt (hier in-process Stand-in; über die .dll-Grenze identische vtable/POD-Layout):
//   (1) Gattungs-API DURCHTESTEN (tier_insert/lookup/erase über uint64),
//   (2) die IM Tier eingebauten Observer als flachen POD durch die Schnittstelle ZIEHEN (tier_observe),
//   (3) ABI-Stabilität des Snapshots (trivially_copyable → memcpy-Roundtrip).
TEST(F15Measurement, R6_HostSideObserverPullViaAbiInterface) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    namespace an  = ::comdare::cache_engine::anatomy;
    using Anatomy = an::SearchAlgorithmAnatomy<comp::ArtComposition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier; // das gebaute composite-Tier-Modul (in-process Stand-in)

    // Host greift NUR über das ABI-Sub-Interface zu (genau wie nach dynamic_cast eines geladenen Moduls):
    auto* obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    ASSERT_NE(obs, nullptr) << "Tier-Modul muss IObservableTier exponieren (Pfad B)";

    // (1) Gattungs-API DURCHTESTEN: insert N (alle neu) → lookup (1 Hit, 1 Miss, 500 weitere) → 1 erase.
    constexpr std::uint64_t N        = 2000;
    std::uint64_t           new_keys = 0;
    for (std::uint64_t i = 0; i < N; ++i)
        if (obs->tier_insert(i, i * 7u + 1u)) ++new_keys;
    EXPECT_EQ(new_keys, N);
    EXPECT_EQ(obs->tier_size(), N);
    std::uint64_t out = 0;
    EXPECT_TRUE(obs->tier_lookup(123, &out));
    EXPECT_EQ(out, 123u * 7u + 1u);
    EXPECT_FALSE(obs->tier_lookup(999999, &out)); // Miss
    for (std::uint64_t i = 0; i < 500; ++i) (void)obs->tier_lookup(i, nullptr);
    EXPECT_TRUE(obs->tier_erase(0));
    EXPECT_FALSE(obs->tier_erase(0)); // schon weg
    EXPECT_EQ(obs->tier_size(), N - 1u);

    // (2) Observer durch die ABI-Grenze ZIEHEN (flacher POD).
    an::ComdareTierObserverSnapshot snap{};
    obs->tier_observe(&snap);
    EXPECT_EQ(snap.axis_stats[0][3], N);    // alle Inserts beim getriebenen ECHTEN Organ
    EXPECT_GE(snap.axis_stats[0][0], 502u); // 1 Hit + 1 Miss + 500 explizite Lookups
    EXPECT_GT(snap.axis_stats[0][5], 0u);
    EXPECT_EQ(snap.tier_fill_level, obs->tier_size()); // korrelierter Fuellstand (§8.7)
    EXPECT_GT(snap.observable_axis_count, 0u);         // mind. die search_algo-Achse observable
    // #188-4c-i sagte: Huellen-Komposition container_-authoritativ -> store-Achsen honest-0 (bis observe-Hooks
    // #234). Dieser Vorbehalt ist durch CMD-1 (b) (#267, Ein-Speicher-Umbau + nicht-konstitutives T6-Mess-Organ
    // allocator_meter_) eingeloest: die allocator-Achse wird jetzt AUCH auf der Huellen-Komposition echt gemessen.
    EXPECT_GT(snap.axis_stats[6][0], 0u); // bytes_alloc — echte Messung statt honest-0
    EXPECT_GT(snap.axis_stats[6][2], 0u); // alloc_cnt   — dito

    // R6 Inkrement 2b: die allocator-Achse wird weiter über die ABI-Grenze gemessen, aber der Echtheits-Beweis
    // lebt seit #188-4c-i-T auf der store-backed AdHoc-Flachgruppe statt auf der Reference-Huelle.
    using StoreBackedAnatomy = an::SearchAlgorithmAnatomy<StoreBackedAdHocComposition>;
    an::SearchAlgorithmAbiAdapter<StoreBackedAnatomy> store_backed_tier;
    auto* store_backed_obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&store_backed_tier));
    ASSERT_NE(store_backed_obs, nullptr);
    for (std::uint64_t i = 0; i < N; ++i) (void)store_backed_obs->tier_insert(i, i * 7u + 1u);
    an::ComdareTierObserverSnapshot store_backed_snap{};
    store_backed_obs->tier_observe(&store_backed_snap);
    EXPECT_GT(store_backed_snap.axis_stats[6][0], 0u);
    EXPECT_GT(store_backed_snap.axis_stats[6][2], 0u);

    // (3) ABI-Stabilität: der Snapshot ist memcpy-fähig (Cross-Boundary-Pflicht).
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);
    an::ComdareTierObserverSnapshot copy{};
    std::memcpy(&copy, &snap, sizeof(snap));
    EXPECT_EQ(copy, snap);
    SUCCEED();
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus — R6-Observer-Trace n/a";
#endif
}

// R6 Inkrement 2 (Doku 24 §8.7): host-seitiger Füllstands-Treiber über IObservableTier — korrelierte
// (Wall-Clock ↔ Observer)-Erhebung über das ABI-Interface, OHNE den Composition-Typ zu kennen (nur Gattung).
TEST(F15Measurement, R6_AbiTierObserveTraceCorrelatesWallClockAndObservers) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    namespace an2 = ::comdare::cache_engine::anatomy;
    namespace ac2 = ::comdare::cache_engine::builder::anatomy_commands;
    using Anatomy = an2::SearchAlgorithmAnatomy<comp::ArtComposition>;
    an2::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto* obs = dynamic_cast<an2::IObservableTier*>(static_cast<an2::IAnatomyBase*>(&tier));
    ASSERT_NE(obs, nullptr);

    ac2::AbiTierTraceConfig cfg;
    cfg.fill_checkpoints       = {10, 100, 1000};
    cfg.lookups_per_checkpoint = 500;
    cfg.deletes_per_checkpoint = 50;
    auto const trace           = ac2::drive_tier_observe_trace_abi(*obs, cfg);

    ASSERT_EQ(trace.checkpoints.size(), 3u);
    std::uint64_t prev_inserts = 0;
    std::int64_t  prev_wall_ns = -1;
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        EXPECT_EQ(cp.fill_level, cfg.fill_checkpoints[i]);
        EXPECT_EQ(cp.observer.tier_fill_level, cfg.fill_checkpoints[i]); // Observer korreliert zum Füllstand
        EXPECT_FALSE(cp.write_ns.empty());                               // Tier-Wall-Clock erhoben (write)
        EXPECT_EQ(cp.read_ns.size(), 500u);                              // r/w/d getrennt (read-Kurve)
        EXPECT_GT(cp.observer.axis_stats[0][3], prev_inserts);           // Inserts wachsen monoton
        EXPECT_GT(cp.observer.axis_stats[0][0], 0u);
        EXPECT_GT(cp.observe_wall_ns, prev_wall_ns); // §8.7: Wall-Clock-Stempel monoton
        prev_inserts = cp.observer.axis_stats[0][3];
        prev_wall_ns = cp.observe_wall_ns;
    }
    EXPECT_EQ(trace.checkpoints.back().observer.tier_fill_level, 1000u);

    // §8.6 Schritt 6: PERSISTIERUNG — der Builder serialisiert die korrelierten (Wall-Clock ↔ Observer)-
    // Ergebnisse als CSV (eine Zeile je Checkpoint).
    std::string const csv = ac2::serialize_abi_tier_trace_csv(trace);
    EXPECT_NE(csv.find("checkpoint,observe_wall_ns,fill_level"), std::string::npos); // Header
    std::size_t nl = 0;
    for (char const c : csv)
        if (c == '\n') ++nl;
    EXPECT_EQ(nl, 4u); // 1 Header + 3 Datenzeilen

    // JSON-Persistierung mit p50/p99 der r/w/d-Kurven (Tier-Wall-Clock-Detail-Auswertung §2.1).
    std::string const json = ac2::serialize_abi_tier_trace_json(trace);
    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"write_p50_ns\""), std::string::npos);
    EXPECT_NE(json.find("\"read_p99_ns\""), std::string::npos);
    EXPECT_NE(json.find("\"observe_wall_ns\""), std::string::npos);
    std::size_t obj_seps = 0, pos = 0;
    while ((pos = json.find("},{", pos)) != std::string::npos) {
        ++obj_seps;
        pos += 3;
    }
    EXPECT_EQ(obj_seps, 2u); // 3 Checkpoint-Objekte
    SUCCEED();
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus — R6-Trace n/a";
#endif
}

// Welch-Validität-Randfall: <2 Samples → valid=false (dokumentierter Fallback).
TEST(F15Measurement, WelchInvalidOnTooFewSamples) {
    std::vector<std::int64_t> one{42};
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{one}, std::span<const std::int64_t>{one});
    EXPECT_FALSE(w.valid);
}

// ─────────────────────────────────────────────────────────────────────────────
// R6 — Multiple-Comparison-Korrektur (Bonferroni + Holm-Bonferroni). Bei F15 werden VIELE
// Achsen-Kompositionen paarweise verglichen → FWER-Kontrolle noetig, sonst Zufalls-"Signifikanzen".
// ─────────────────────────────────────────────────────────────────────────────

TEST(F15MultipleComparison, BonferroniScalesByCount) {
    std::vector<double> p{0.01, 0.02, 0.04};
    auto                adj = stats::bonferroni_adjust(std::span<const double>{p});
    ASSERT_EQ(adj.size(), 3u);
    EXPECT_NEAR(adj[0], 0.03, 1e-12);
    EXPECT_NEAR(adj[1], 0.06, 1e-12);
    EXPECT_NEAR(adj[2], 0.12, 1e-12);
    // Bei alpha=0.05 ueberlebt nur der erste.
    EXPECT_EQ(stats::count_significant(std::span<const double>{adj}, 0.05), 1u);
}

TEST(F15MultipleComparison, BonferroniClampsAtOne) {
    std::vector<double> p{0.5, 0.9};
    auto                adj = stats::bonferroni_adjust(std::span<const double>{p});
    EXPECT_NEAR(adj[0], 1.0, 1e-12); // 0.5*2=1.0
    EXPECT_NEAR(adj[1], 1.0, 1e-12); // 0.9*2=1.8 -> clamp 1.0
}

TEST(F15MultipleComparison, HolmIsMorePowerfulThanBonferroni) {
    std::vector<double> p{0.01, 0.02, 0.04}; // m=3
    auto                holm = stats::holm_bonferroni_adjust(std::span<const double>{p});
    // sortiert: 3*0.01=0.03, 2*0.02=0.04, 1*0.04=0.04 (monoton) → [0.03, 0.04, 0.04]
    EXPECT_NEAR(holm[0], 0.03, 1e-12);
    EXPECT_NEAR(holm[1], 0.04, 1e-12);
    EXPECT_NEAR(holm[2], 0.04, 1e-12);
    // Holm: alle 3 signifikant @0.05; Bonferroni nur 1 → Holm strikt maechtiger bei gleicher FWER.
    EXPECT_EQ(stats::count_significant(std::span<const double>{holm}, 0.05), 3u);
}

TEST(F15MultipleComparison, HolmReorderEnforcesMonotonicity) {
    // Original-Reihenfolge unsortiert; Monotonie wird in p-Sortier-Reihenfolge erzwungen + zurueckgestreut.
    std::vector<double> p{0.6, 0.04, 0.5}; // m=3
    auto                holm = stats::holm_bonferroni_adjust(std::span<const double>{p});
    ASSERT_EQ(holm.size(), 3u);
    // sortiert [0.04,0.5,0.6]: 3*0.04=0.12, 2*0.5=1.0, max(1*0.6,1.0)=1.0 → [0.12,1.0,1.0]
    // zurueck auf Original-Indizes: p[1]=0.04→0.12, p[2]=0.5→1.0, p[0]=0.6→1.0
    EXPECT_NEAR(holm[1], 0.12, 1e-12);
    EXPECT_NEAR(holm[0], 1.0, 1e-12);
    EXPECT_NEAR(holm[2], 1.0, 1e-12);
    // korrigierte p-Werte nie kleiner als das Roh-p
    for (std::size_t i = 0; i < p.size(); ++i) EXPECT_GE(holm[i], p[i] - 1e-12);
}

TEST(F15MultipleComparison, EmptyInputSafe) {
    std::vector<double> p{};
    EXPECT_TRUE(stats::bonferroni_adjust(std::span<const double>{p}).empty());
    EXPECT_TRUE(stats::holm_bonferroni_adjust(std::span<const double>{p}).empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// R5.E — ResultAggregator: ExecutionResult-Sammlung → CSV/JSON (maschinenlesbarer F15-Export).
// ─────────────────────────────────────────────────────────────────────────────

namespace cmd = ::comdare::cache_engine::builder::commands;

TEST(F15ResultAggregator, CsvHeaderAndRows) {
    cmd::ExecutionResult a{};
    a.engine_name            = "art";
    a.throughput_ops_per_sec = 1000000.0;
    a.latency_p50            = std::chrono::nanoseconds{50};
    a.latency_p99            = std::chrono::nanoseconds{120};
    a.memory_footprint_bytes = 4096;
    a.latency_samples_ns     = {1, 2, 3};
    a.success                = true;
    cmd::ExecutionResult b{};
    b.engine_name = "hot";
    b.success     = false;

    std::vector<cmd::ExecutionResult> rs{a, b};
    auto                              csv = cmd::to_csv(std::span<const cmd::ExecutionResult>{rs});
    // Header + 2 Daten-Zeilen + finaler newline → 3 newlines.
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 3);
    EXPECT_NE(csv.find("engine_name,workload_kind"), std::string::npos); // Header
    EXPECT_NE(csv.find("\"art\""), std::string::npos);
    EXPECT_NE(csv.find("\"hot\""), std::string::npos);
    EXPECT_NE(csv.find(",50,120,"), std::string::npos); // p50/p99
    // success als 1/0, n_latency_samples=3 fuer art
    auto row_a = cmd::to_csv_row(a);
    EXPECT_NE(row_a.find(",3,1"), std::string::npos); // n_samples=3, success=1 am Ende
}

TEST(F15ResultAggregator, JsonArrayOfObjects) {
    cmd::ExecutionResult a{};
    a.engine_name            = "art";
    a.throughput_ops_per_sec = 5.0;
    a.success                = true;
    std::vector<cmd::ExecutionResult> rs{a};
    auto                              json = cmd::to_json(std::span<const cmd::ExecutionResult>{rs});
    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"engine_name\":\"art\""), std::string::npos);
    EXPECT_NE(json.find("\"success\":true"), std::string::npos);
    EXPECT_NE(json.find("\"n_latency_samples\":0"), std::string::npos);
}

TEST(F15ResultAggregator, JsonEscapesSpecialChars) {
    cmd::ExecutionResult a{};
    a.engine_name = "ev\"il\\name"; // " und \ muessen escaped werden
    std::vector<cmd::ExecutionResult> rs{a};
    auto                              json = cmd::to_json(std::span<const cmd::ExecutionResult>{rs});
    EXPECT_NE(json.find("ev\\\"il\\\\name"), std::string::npos);
}

TEST(F15ResultAggregator, EmptyCollection) {
    std::vector<cmd::ExecutionResult> rs{};
    auto                              csv = cmd::to_csv(std::span<const cmd::ExecutionResult>{rs});
    EXPECT_NE(csv.find("engine_name"), std::string::npos); // nur Header
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 1);
    EXPECT_EQ(cmd::to_json(std::span<const cmd::ExecutionResult>{rs}), "[]");
}

// ─────────────────────────────────────────────────────────────────────────────
// R5.E — latency_stats: geteilte, non-mutierende Perzentil/Statistik-Helfer.
// ─────────────────────────────────────────────────────────────────────────────

TEST(F15LatencyStats, NearestRankPercentiles) {
    // SELBSTCHECK DIESER AENDERUNG (D5-1, 2026-08-09)
    //   ZUSICHERT: latency_p50/p99 rechnen die Lehrbuch-Formel k = ceil(q*n)-1.
    //              1..100 -> p50: k = ceil(50)-1 = 49 -> sortiert[49] = 50.
    //              1..100 -> p99: k = ceil(99)-1 = 98 -> sortiert[98] = 99.
    //   ZUSICHERT NICHT: dass die frueheren Pins (51 / 100) falsch GEMESSEN waren -- sie waren
    //              korrekt fuer die alte Formel k = min(n-1, floor(q*n)). Die Formel hat gewechselt,
    //              nicht die Messung. FOLGE: alle vor dem 2026-08-09 erhobenen p50/p95/p99 sind
    //              nach einer anderen Definition gerechnet und mit den heutigen nicht vergleichbar.
    std::vector<std::int64_t> s(100);
    for (int i = 0; i < 100; ++i) s[static_cast<std::size_t>(i)] = i + 1;
    EXPECT_EQ(stats::latency_p50_ns(std::span<const std::int64_t>{s}).count(), 50); // D5-1: war 51
    EXPECT_EQ(stats::latency_p95_ns(std::span<const std::int64_t>{s}).count(), 95); // D5-1: war 96
    EXPECT_EQ(stats::latency_p99_ns(std::span<const std::int64_t>{s}).count(), 99); // D5-1: war 100
    EXPECT_EQ(stats::percentile_ns(std::span<const std::int64_t>{s}, 0.0).count(), 1);
    EXPECT_EQ(stats::percentile_ns(std::span<const std::int64_t>{s}, 1.0).count(), 100); // q geklemmt, k=n-1
}

TEST(F15LatencyStats, NonMutatingInput) {
    std::vector<std::int64_t> s{5, 1, 4, 2, 3};
    auto const                before = s; // Kopie
    (void)stats::latency_p50_ns(std::span<const std::int64_t>{s});
    (void)stats::latency_p99_ns(std::span<const std::int64_t>{s});
    EXPECT_EQ(s, before) << "percentile_ns darf die Eingabe NICHT umsortieren (Welch-Samples bleiben)";
}

TEST(F15LatencyStats, MinMaxMeanAndEmpty) {
    std::vector<std::int64_t> s{10, 20, 30};
    EXPECT_EQ(stats::latency_min_ns(std::span<const std::int64_t>{s}), 10);
    EXPECT_EQ(stats::latency_max_ns(std::span<const std::int64_t>{s}), 30);
    EXPECT_NEAR(stats::latency_mean_ns(std::span<const std::int64_t>{s}), 20.0, 1e-9);
    std::vector<std::int64_t> e{};
    EXPECT_EQ(stats::percentile_ns(std::span<const std::int64_t>{e}, 0.5).count(), 0);
    EXPECT_EQ(stats::latency_min_ns(std::span<const std::int64_t>{e}), 0);
    EXPECT_EQ(stats::latency_mean_ns(std::span<const std::int64_t>{e}), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// R6 — multi_compare_against_baseline: der vollstaendige F15-Workflow (N Kompositionen vs Baseline,
// Welch pro Paar + Holm-FWER-Korrektur + Signifikanz/Schneller-Flag).
// ─────────────────────────────────────────────────────────────────────────────

namespace {
// Samples um center mit kleinem Jitter (±2) → nicht-null Varianz, deterministisch.
std::vector<std::int64_t> make_samples(std::int64_t center, std::size_t n) {
    std::vector<std::int64_t> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) v.push_back(center + static_cast<std::int64_t>(i % 5) - 2);
    return v;
}
} // namespace

TEST(F15MultiCompare, FasterSlowerSimilarVsBaseline) {
    cmd::ExecutionResult base{};
    base.engine_name        = "baseline";
    base.latency_samples_ns = make_samples(100, 40);
    cmd::ExecutionResult fast{};
    fast.engine_name        = "fast";
    fast.latency_samples_ns = make_samples(50, 40);
    cmd::ExecutionResult slow{};
    slow.engine_name        = "slow";
    slow.latency_samples_ns = make_samples(150, 40);
    cmd::ExecutionResult sim{};
    sim.engine_name        = "similar";
    sim.latency_samples_ns = make_samples(100, 40);

    std::vector<cmd::ExecutionResult> cands{fast, slow, sim};
    auto rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{cands}, 0.05);

    ASSERT_EQ(rep.comparisons.size(), 3u);
    EXPECT_EQ(rep.comparisons[0].name, std::string_view{"fast"}); // Reihenfolge erhalten
    EXPECT_EQ(rep.comparisons[1].name, std::string_view{"slow"});
    EXPECT_EQ(rep.comparisons[2].name, std::string_view{"similar"});

    EXPECT_TRUE(rep.comparisons[0].significant); // fast klar schneller
    EXPECT_TRUE(rep.comparisons[0].faster_than_baseline);
    EXPECT_TRUE(rep.comparisons[1].significant); // slow klar langsamer
    EXPECT_FALSE(rep.comparisons[1].faster_than_baseline);
    EXPECT_FALSE(rep.comparisons[2].significant); // similar == baseline → nicht signifikant

    EXPECT_EQ(rep.significant_count, 2u);
    EXPECT_DOUBLE_EQ(rep.alpha, 0.05);
    // korrigierte p-Werte >= Roh-p (Holm macht nie kleiner)
    for (auto const& c : rep.comparisons) EXPECT_GE(c.adjusted_p, c.raw_p - 1e-12);
}

TEST(F15MultiCompare, InvalidCandidateNotSignificant) {
    cmd::ExecutionResult base{};
    base.engine_name        = "baseline";
    base.latency_samples_ns = make_samples(100, 40);
    cmd::ExecutionResult tiny{};
    tiny.engine_name        = "tiny";
    tiny.latency_samples_ns = {42}; // <2 → Welch ungueltig
    std::vector<cmd::ExecutionResult> cands{tiny};
    auto rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{cands});
    ASSERT_EQ(rep.comparisons.size(), 1u);
    EXPECT_FALSE(rep.comparisons[0].welch.valid);
    EXPECT_DOUBLE_EQ(rep.comparisons[0].raw_p, 1.0);
    EXPECT_FALSE(rep.comparisons[0].significant);
    EXPECT_EQ(rep.significant_count, 0u);
}

TEST(F15MultiCompare, EmptyCandidates) {
    cmd::ExecutionResult base{};
    base.latency_samples_ns = make_samples(100, 10);
    std::vector<cmd::ExecutionResult> none{};
    auto rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{none});
    EXPECT_TRUE(rep.comparisons.empty());
    EXPECT_EQ(rep.significant_count, 0u);
}

TEST(F15MultiCompare, ReportCsvAndJsonExport) {
    cmd::ExecutionResult base{};
    base.engine_name        = "baseline";
    base.latency_samples_ns = make_samples(100, 40);
    cmd::ExecutionResult fast{};
    fast.engine_name        = "fast";
    fast.latency_samples_ns = make_samples(50, 40);
    cmd::ExecutionResult sim{};
    sim.engine_name        = "similar";
    sim.latency_samples_ns = make_samples(100, 40);
    std::vector<cmd::ExecutionResult> cands{fast, sim};
    auto rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{cands}, 0.05);

    auto csv = stats::report_to_csv(rep);
    EXPECT_NE(csv.find("name,raw_p,adjusted_p,significant,faster_than_baseline"), std::string::npos);
    EXPECT_NE(csv.find("\"fast\""), std::string::npos);
    EXPECT_NE(csv.find("\"similar\""), std::string::npos);
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 3); // Header + 2 Zeilen

    auto json = stats::report_to_json(rep);
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');
    EXPECT_NE(json.find("\"significant_count\":1"), std::string::npos); // nur fast signifikant
    EXPECT_NE(json.find("\"comparisons\":["), std::string::npos);
    EXPECT_NE(json.find("\"name\":\"fast\""), std::string::npos);
    EXPECT_NE(json.find("\"faster_than_baseline\":true"), std::string::npos);
}

TEST(F15MultiCompare, SummarizeWinRate) {
    cmd::ExecutionResult base{};
    base.engine_name        = "baseline";
    base.latency_samples_ns = make_samples(100, 40);
    cmd::ExecutionResult fast{};
    fast.engine_name        = "fast";
    fast.latency_samples_ns = make_samples(50, 40);
    cmd::ExecutionResult slow{};
    slow.engine_name        = "slow";
    slow.latency_samples_ns = make_samples(150, 40);
    cmd::ExecutionResult sim{};
    sim.engine_name        = "similar";
    sim.latency_samples_ns = make_samples(100, 40);
    std::vector<cmd::ExecutionResult> cands{fast, slow, sim};
    auto rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{cands}, 0.05);

    auto sum = stats::summarize(rep);
    EXPECT_EQ(sum.total, 3u);
    EXPECT_EQ(sum.significant_faster, 1u);       // fast
    EXPECT_EQ(sum.significant_slower, 1u);       // slow
    EXPECT_EQ(sum.not_significant, 1u);          // similar
    EXPECT_NEAR(sum.win_rate, 1.0 / 3.0, 1e-12); // F15-Headline: 1/3 schlagen Baseline signifikant
    // Konsistenz: Kategorien summieren zu total.
    EXPECT_EQ(sum.significant_faster + sum.significant_slower + sum.not_significant, sum.total);
}

TEST(F15MultiCompare, SummarizeEmptyZeroWinRate) {
    stats::MultiCompareReport empty{};
    auto                      sum = stats::summarize(empty);
    EXPECT_EQ(sum.total, 0u);
    EXPECT_DOUBLE_EQ(sum.win_rate, 0.0); // kein Division-durch-0
}

// ─────────────────────────────────────────────────────────────────────────────
// R5.D — Konnektor make_execution_result + die vollstaendige Driver-Kette in Miniatur:
// Samples → ExecutionResult → multi_compare → summarize → export.
// ─────────────────────────────────────────────────────────────────────────────

TEST(F15ResultBuilder, MakeExecutionResultFillsPercentiles) {
    std::vector<std::int64_t> s(100);
    for (int i = 0; i < 100; ++i) s[static_cast<std::size_t>(i)] = i + 1;
    auto r = cmd::make_execution_result("art", s, 1234.0);
    EXPECT_EQ(r.engine_name, std::string_view{"art"});
    EXPECT_DOUBLE_EQ(r.throughput_ops_per_sec, 1234.0);
    // D5-1 (2026-08-09): Kanon k = ceil(q*n)-1. p50 von 1..100 = 50 (war 51), p99 = 99 (war 100).
    // Der Konnektor make_execution_result rechnet damit nachweislich ueber stats::percentile_ns
    // und nicht ueber eine eigene Formel -- das ist die Aussage dieses Pins.
    EXPECT_EQ(r.latency_p50.count(), 50);
    EXPECT_EQ(r.latency_p99.count(), 99);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.latency_samples_ns.size(), 100u);
    // leere Samples → success=false
    auto empty = cmd::make_execution_result("none", {});
    EXPECT_FALSE(empty.success);
    // D4d (2026-08-09): dieser Test hat DECKUNG VORGETAEUSCHT. Seine beiden Faelle (100 positive
    // Werte / leer) verhalten sich unter der alten Definition success=!empty() und der neuen
    // "mindestens eine Probe > 0" IDENTISCH -- die Luecke dazwischen (Proben da, alle 0) war fuer
    // ihn unsichtbar. Sie steht jetzt in F15ProbenSindTot; hier wird nur noch mitgesichert, dass
    // beide Felder gesetzt sind und sich in diesen zwei Faellen entsprechen.
    EXPECT_FALSE(r.degeneriert);
    EXPECT_TRUE(empty.degeneriert);
}

// Vollstaendige F15-Driver-Kette (in-process): drei "gemessene" Kompositionen vs Baseline.
TEST(F15ResultBuilder, FullDriverChainSamplesToSummary) {
    auto                              baseline = cmd::make_execution_result("baseline", make_samples(100, 40));
    std::vector<cmd::ExecutionResult> cands{
        cmd::make_execution_result("fast", make_samples(50, 40)),
        cmd::make_execution_result("slow", make_samples(150, 40)),
        cmd::make_execution_result("similar", make_samples(100, 40)),
    };
    auto rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    auto sum = stats::summarize(rep);
    EXPECT_EQ(sum.total, 3u);
    EXPECT_EQ(sum.significant_faster, 1u); // nur "fast" schlaegt die Baseline signifikant
    EXPECT_NEAR(sum.win_rate, 1.0 / 3.0, 1e-12);
    // Export der ganzen Kette ist nicht-leer + maschinenlesbar.
    EXPECT_FALSE(stats::report_to_csv(rep).empty());
    EXPECT_NE(stats::report_to_json(rep).find("\"significant_count\":2"), std::string::npos);
}

// V41.F.6.1 R5.D — Mann-Whitney-U (robuster Rang-Test). Klar getrennte Gruppen → signifikant + a<b;
// identische Gruppen → n.s.; und Robustheit: ein einzelner extremer Ausreisser in a kippt das
// Rang-Urteil NICHT (waehrend er einen mittelwert-basierten Test verzerren wuerde).
TEST(F15RobustStats, MannWhitneyDistinctIdenticalAndOutlierRobust) {
    // (1) a klar kleiner als b (keine Ueberlappung) → hochsignifikant, a stochastisch kleiner.
    std::vector<std::int64_t> a(40, 0), b(40, 0);
    for (int i = 0; i < 40; ++i) {
        a[i] = 100 + i;
        b[i] = 1000 + i;
    }
    auto const w1 = stats::mann_whitney_u_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    EXPECT_TRUE(w1.valid);
    EXPECT_TRUE(w1.a_stochastically_less);
    EXPECT_LT(w1.p_value, 0.001);
    EXPECT_NEAR(w1.cliff_delta, 1.0, 1e-9); // a durchweg kleiner → delta=+1
    EXPECT_EQ(stats::cliff_delta_magnitude(w1.cliff_delta), std::string_view{"large"});

    // (2) identische Gruppen → kein Unterschied (p ~ 1, delta ~ 0).
    std::vector<std::int64_t> same(40, 500);
    auto const                w2 =
        stats::mann_whitney_u_test(std::span<const std::int64_t>{same}, std::span<const std::int64_t>{same});
    EXPECT_TRUE(w2.valid);
    EXPECT_GT(w2.p_value, 0.5);
    EXPECT_NEAR(w2.cliff_delta, 0.0, 1e-9);
    EXPECT_EQ(stats::cliff_delta_magnitude(w2.cliff_delta), std::string_view{"negligible"});

    // (3) Ausreisser-Robustheit: a = lauter 100er + EIN 10^9-Spike; b = lauter 200er.
    // Median/Rang von a liegt klar unter b → MWU sagt weiterhin a<b, trotz Ausreisser.
    std::vector<std::int64_t> ao(40, 100);
    ao[39] = 1000000000; // ein extremer Spike
    std::vector<std::int64_t> bo(40, 200);
    auto const w3 = stats::mann_whitney_u_test(std::span<const std::int64_t>{ao}, std::span<const std::int64_t>{bo});
    EXPECT_TRUE(w3.valid);
    EXPECT_TRUE(w3.a_stochastically_less) << "Rang-Test bleibt vom Einzel-Ausreisser unbeeinflusst";
    EXPECT_LT(w3.p_value, 0.05);
    EXPECT_NEAR(w3.cliff_delta, 0.95, 1e-9); // 1 - 2*40/1600; trotz Ausreisser klar grosses Effektmass
    EXPECT_EQ(stats::cliff_delta_magnitude(w3.cliff_delta), std::string_view{"large"});
    // invalid bei leerer Gruppe:
    EXPECT_FALSE(stats::mann_whitney_u_test(std::span<const std::int64_t>{}, std::span<const std::int64_t>{bo}).valid);
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// D4a (2026-08-09) — WELCH: DIE NULL ALS DATEN-AUSSAGE, NICHT NUR ALS DIVISIONS-GEFAHR
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
// DER BEFUND, den diese Tests festnageln: welch_t_test rettete bei se<=0 die Division, gab
// t=0, p=1.0 zurueck UND setzte valid=true. Der Aufrufer bekam damit ein Ergebnis, das von
// einem ECHT gemessenen "kein Unterschied" nicht zu unterscheiden war -- obwohl der Test in
// diesem Zweig gar nicht gerechnet hat: bei var_a==0 UND var_b==0 ist die t-Statistik 0/0,
// also UNDEFINIERT, und zwar auch dann, wenn die beiden Gruppen weit auseinanderliegen.
//
// KOEDER (K13, frisch aus /dev/urandom gewuerfelt am 2026-08-09T18:31:31Z, NICHT aus einer Doku
// abgeschrieben): zwei KONSTANTE Gruppen, 929 (5x) gegen 1031 (4x) -- 11 % auseinander. Vorher
// meldete Welch dafuer p=1.0 bei valid=true, also "kein Unterschied". Der Gegeneingang (T-4)
// steht direkt daneben: dieselben Gruppen MIT Streuung duerfen NICHT degeneriert heissen.

TEST(F15WelchDegeneriert, KonstanteGruppenMitVerschiedenenMittelnSindDegeneriert) {
    std::vector<std::int64_t> const a(5, 929);  // KOEDER, gewuerfelt: Wert 929, n=5, Varianz 0
    std::vector<std::int64_t> const b(4, 1031); // KOEDER, gewuerfelt: Wert 1031, n=4, Varianz 0
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});

    // Die beiden Gruppen liegen 11 % auseinander -- das ist die PRAEMISSE des Koeders, und sie
    // wird hier gepinnt statt vorausgesetzt (sonst koennte der Koeder unbemerkt stumpf werden).
    ASSERT_DOUBLE_EQ(w.mean_a, 929.0);
    ASSERT_DOUBLE_EQ(w.mean_b, 1031.0);
    ASSERT_GT(w.mean_b - w.mean_a, 0.10 * w.mean_a) << "Koeder stumpf: die Gruppen liegen zu nah";
    ASSERT_DOUBLE_EQ(w.var_a, 0.0);
    ASSERT_DOUBLE_EQ(w.var_b, 0.0);

    EXPECT_TRUE(w.valid) << "valid bleibt 'gerechnet' -- die Aussage steckt in degeneriert";
    EXPECT_TRUE(w.degeneriert) << "0/0 ist keine Messung von 'kein Unterschied'";
    EXPECT_DOUBLE_EQ(w.p_value, 1.0);
}

TEST(F15WelchDegeneriert, StreuendeGruppenSindNichtDegeneriert) {
    // GEGENEINGANG (T-4 / K13 beide Richtungen): dieselben Mittelwerte, aber mit echter Streuung.
    // Faellt diese Zusicherung, war der neue Guard konstant-rot und haette nichts bewiesen.
    std::vector<std::int64_t> a, b;
    for (int i = 0; i < 5; ++i) a.push_back(929 + i);
    for (int i = 0; i < 4; ++i) b.push_back(1031 + i);
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    EXPECT_TRUE(w.valid);
    EXPECT_FALSE(w.degeneriert);
    EXPECT_LT(w.p_value, 0.05) << "mit Streuung wird der Unterschied SICHTBAR -- vorher war er es nicht";
}

TEST(F15WelchDegeneriert, ZuWenigSamplesIstDegeneriertUndNichtGerechnet) {
    // n<2: gar nichts gerechnet. valid=false (wie bisher) UND degeneriert=true (neu) -- die
    // beiden Felder sagen zwei verschiedene Dinge und werden hier getrennt gepinnt.
    std::vector<std::int64_t> const one{42};
    std::vector<std::int64_t>       many(8, 100);
    for (std::size_t i = 0; i < many.size(); ++i) many[i] = 100 + static_cast<std::int64_t>(i);
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{one}, std::span<const std::int64_t>{many});
    EXPECT_FALSE(w.valid);
    EXPECT_TRUE(w.degeneriert);
}

TEST(F15WelchDegeneriert, DegenerationsPraedikatHatBeideRichtungen) {
    // Das Praedikat ist die EINE Stelle, an der "degeneriert" entschieden wird; es wird hier
    // direkt befragt, damit auch der zweite Ausloeser (nicht-endlicher p-Wert) eine Zusicherung
    // hat und nicht nur als toter Zweig im Header steht (T-2: Aussage statt Anwesenheit).
    EXPECT_TRUE(stats::welch_ergebnis_ist_degeneriert(0.0, 1.0));  // se == 0
    EXPECT_TRUE(stats::welch_ergebnis_ist_degeneriert(-1.0, 0.01)); // se < 0 (fail-closed)
    EXPECT_TRUE(stats::welch_ergebnis_ist_degeneriert(2.5, std::nan("")));
    EXPECT_TRUE(stats::welch_ergebnis_ist_degeneriert(2.5, std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(stats::welch_ergebnis_ist_degeneriert(2.5, 0.01)); // GEGENEINGANG: hier faellt es
}

TEST(F15WelchDegeneriert, NichtEndlichesPFeuertUeberDemGanzenNennerNie) {
    // NENNER IN DER AUSGABE: der zweite Ausloeser (nicht-endliches p) ist aus std::int64_t-Eingaben
    // heute NICHT erreichbar -- denom>0 folgt aus se>0. Das ist eine AUSSAGE ueber eine
    // Grundgesamtheit, keine Vermutung, deshalb wird sie gezaehlt und die Zahl ausgegeben.
    std::mt19937_64 rng{20260809u}; // fester Seed: reproduzierbare Zaehlung, kein Koeder
    std::size_t     geprueft = 0, nicht_endlich = 0, se_null = 0;
    for (int lauf = 0; lauf < 2000; ++lauf) {
        std::size_t const         na = 2 + static_cast<std::size_t>(rng() % 60);
        std::size_t const         nb = 2 + static_cast<std::size_t>(rng() % 60);
        std::vector<std::int64_t> a(na), b(nb);
        for (auto& v : a) v = static_cast<std::int64_t>(rng() % 1000000000ULL);
        for (auto& v : b) v = static_cast<std::int64_t>(rng() % 1000000000ULL);
        auto const w = stats::welch_t_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
        ++geprueft;
        if (!std::isfinite(w.p_value)) ++nicht_endlich;
        if (w.degeneriert) ++se_null;
    }
    EXPECT_EQ(geprueft, 2000u);
    EXPECT_EQ(nicht_endlich, 0u) << nicht_endlich << " von " << geprueft << " Laeufen mit nicht-endlichem p";
    EXPECT_EQ(se_null, 0u) << se_null << " von " << geprueft << " zufaelligen Laeufen degeneriert";
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// D4b (2026-08-09) — MANN-WHITNEY: valid HINTER den Guard, cliff_delta nicht "gross" ueber Nichts
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
// DER BEFUND: r.valid=true und r.cliff_delta standen VOR jedem Degenerations-Check. Der einzige
// Ausschluss war na==0||nb==0. Bei na=1, nb=1 mit verschiedenen Werten ist
// var_U = (1*1/12)*((3) - 0) = 0.25 > 0 -- der bestehende var_U-Guard greift also NICHT, und
// cliff_delta = 1 - 2*U_a/(1*1) ist bei zwei Einzelmessungen ZWANGSLAEUFIG +-1, also "large",
// voellig unabhaengig davon, wie nah die beiden Werte beieinanderliegen.
//
// KOEDER (K13, gewuerfelt 2026-08-09T18:31:31Z): 3160 gegen 3165 -- 5 ns Abstand, je EINE Probe.
// Vorher: valid=true, cliff_delta=+1.0, magnitude "large" ("a ist durchweg schneller").

TEST(F15MwuDegeneriert, EinzelprobenLiefernKeinGrossesEffektmass) {
    std::vector<std::int64_t> const a{3160}; // KOEDER, gewuerfelt
    std::vector<std::int64_t> const b{3165}; // KOEDER, gewuerfelt: 5 ns daneben
    auto const m = stats::mann_whitney_u_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    EXPECT_TRUE(m.degeneriert) << "eine Probe je Seite ist keine Rangverteilung";
    EXPECT_FALSE(m.valid) << "valid darf nicht VOR dem Guard gesetzt werden";
    EXPECT_DOUBLE_EQ(m.cliff_delta, 0.0) << "kein Extremwert ueber Nichts";
    EXPECT_EQ(stats::cliff_delta_magnitude(m.cliff_delta), std::string_view{"negligible"});
}

TEST(F15MwuDegeneriert, ZweiProbenJeSeiteSindNichtDegeneriert) {
    // GEGENEINGANG (T-4): eine Probe MEHR je Seite, und der Rang-Test ist wieder zustaendig.
    // Ohne diese Zusicherung koennte der neue Guard zu breit sein und alles ablehnen.
    std::vector<std::int64_t> const a{3160, 3161};
    std::vector<std::int64_t> const b{3165, 3166};
    auto const m = stats::mann_whitney_u_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    EXPECT_FALSE(m.degeneriert);
    EXPECT_TRUE(m.valid);
    EXPECT_NEAR(m.cliff_delta, 1.0, 1e-12) << "hier ist +1 eine ECHTE Rang-Aussage: a liegt komplett unter b";
}

TEST(F15MwuDegeneriert, LeereGruppeBleibtUngueltigUndDegeneriert) {
    std::vector<std::int64_t> const b{100, 200, 300};
    auto const m = stats::mann_whitney_u_test(std::span<const std::int64_t>{}, std::span<const std::int64_t>{b});
    EXPECT_FALSE(m.valid);
    EXPECT_TRUE(m.degeneriert);
}

TEST(F15MwuDegeneriert, DurchgehendIdentischeWerteSindGueltigUndNichtDegeneriert) {
    // BEWUSSTE ASYMMETRIE ZU WELCH, und sie ist begruendet: var_U<=0 tritt (algebraisch) GENAU
    // dann ein, wenn ALLE N Werte identisch sind -- dann sagen die Daten wirklich "kein
    // Unterschied", und der Rang-Test kann das auch sagen. Welchs se<=0 dagegen tritt schon ein,
    // wenn jede Gruppe FUER SICH konstant ist, die beiden Gruppen aber weit auseinanderliegen.
    // Genau diesen Fall behandelt der Rang-Test korrekt -- siehe die naechste Zusicherung.
    std::vector<std::int64_t> const same(40, 500);
    auto const m = stats::mann_whitney_u_test(std::span<const std::int64_t>{same}, std::span<const std::int64_t>{same});
    EXPECT_TRUE(m.valid);
    EXPECT_FALSE(m.degeneriert);
    EXPECT_NEAR(m.cliff_delta, 0.0, 1e-12);
}

TEST(F15MwuDegeneriert, KonstanteVerschiedeneGruppenSindFuerDenRangTestSEHRWOHLTestbar) {
    // Der Welch-Koeder aus D4a (929 vs 1031, je konstant), hier durch den Rang-Test geschickt.
    // ORAKEL UNABHAENGIG (T-5): die Erwartung kommt NICHT aus welch_t_test, sondern aus der
    // Rang-Konstruktion selbst -- alle 5 a-Werte belegen die Raenge 1..5, alle 4 b-Werte die
    // Raenge 6..9, also U_a = 0 und cliff_delta = +1. Das ist per Hand nachrechenbar.
    std::vector<std::int64_t> const a(5, 929);
    std::vector<std::int64_t> const b(4, 1031);
    auto const m = stats::mann_whitney_u_test(std::span<const std::int64_t>{a}, std::span<const std::int64_t>{b});
    EXPECT_TRUE(m.valid);
    EXPECT_FALSE(m.degeneriert) << "der Rang-Test kann diesen Fall -- nur Welch kann ihn nicht";
    EXPECT_DOUBLE_EQ(m.u_statistic, 0.0);
    EXPECT_NEAR(m.cliff_delta, 1.0, 1e-12);
    EXPECT_TRUE(m.a_stochastically_less);
}

// D4b-Schwesterstelle (T-6) in multi_compare: die Diskrepanz-Warnung "Welch und MWU sind uneinig"
// war frueher schon dann wahr, wenn EINER der beiden gar nicht gerechnet hatte. Der Koeder ist der
// D4a-Koeder (929 vs 1031, je konstant): Welch degeneriert dort, der Rang-Test nicht -- vorher
// ergab das eine "ausreisser-verdaechtige" Meldung, wo es gar keinen Ausreisser gibt.
TEST(F15MwuDegeneriert, DiskrepanzWarntNurWennBeideTestsGesprochenHaben) {
    cmd::ExecutionResult base{};
    base.engine_name        = "baseline";
    base.latency_samples_ns = std::vector<std::int64_t>(4, 1031); // konstant -> Welch degeneriert
    cmd::ExecutionResult cand{};
    cand.engine_name        = "kandidat";
    cand.latency_samples_ns = std::vector<std::int64_t>(5, 929); // konstant, 11 % darunter
    std::vector<cmd::ExecutionResult> cands{cand};

    auto const rep = stats::multi_compare_against_baseline(base, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    ASSERT_EQ(rep.comparisons.size(), 1u);
    EXPECT_TRUE(rep.comparisons[0].welch.degeneriert);
    EXPECT_FALSE(rep.comparisons[0].mwu.degeneriert);
    EXPECT_FALSE(rep.comparisons[0].significance_discrepancy)
        << "ein degenerierter Welch ist mit dem Rang-Test nicht 'uneinig' -- er hat nichts gesagt";
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// D4c (2026-08-09) — BONFERRONI: m IST DIE ZAHL DER GETESTETEN HYPOTHESEN, NICHT DER KANDIDATEN
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
// DER BEFUND: holm_bonferroni_adjust bekam IMMER alle Kandidaten, auch die, fuer die gar kein
// Test gerechnet wurde (raw_p = 1.0 als Platzhalter). Die Korrektur skaliert mit (m-k) --
// jeder untestbare Kandidat verschaerft also die Schwelle fuer die ECHTEN. Und win_rate teilte
// durch die Gesamtzahl statt durch die getestete Menge.
//
// Der Algorithmus in multiple_comparison.hpp ist dabei KORREKT; falsch war ausschliesslich,
// was hineingereicht wurde. Deshalb aendert D4c die Aufrufstelle und nicht den Algorithmus.

namespace {
// Fein aufloesende Sample-Konstruktion: center + 100*i. Die Streuung ist gross genug, dass ein
// Versatz von 1 ns die t-Statistik nur um ~0.004 bewegt -- nur so ist ein p-Wert im schmalen
// Kipp-Fenster [0.05/9 .. 0.05/7] ueberhaupt TREFFBAR.
std::vector<std::int64_t> fein_samples(std::int64_t center, std::size_t n) {
    std::vector<std::int64_t> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) v.push_back(center + 100 * static_cast<std::int64_t>(i));
    return v;
}
// Die sieben ECHTEN Kandidaten. Der erste ist der KIPP-KANDIDAT: sein Roh-p liegt zwischen
// 0.05/9 und 0.05/7, er ist also bei m=9 nicht signifikant und bei m=7 signifikant. Genau daran
// haengt die Headline-Zahl der Arbeit.
constexpr std::int64_t kKippVersatz         = 734;
constexpr std::int64_t kEchteVersaetze[7]   = {734, 700, 640, 560, 470, 360, 230};
constexpr std::size_t  kSamplesJeKandidat   = 40;
constexpr std::int64_t kBasisZentrum        = 100000;
} // namespace

TEST(F15BonferroniNenner, DegenerierteKandidatenVerduennenDieFamilieNichtMehr) {
    // ── Aufbau: 7 echte Kandidaten + 2 degenerierte, ZWEI VERSCHIEDENER Degenerations-Klassen.
    auto const baseline_samples = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "baseline";
    baseline.latency_samples_ns = baseline_samples;

    std::vector<std::string> namen;
    namen.reserve(9);
    for (int i = 0; i < 7; ++i) namen.push_back("echt_" + std::to_string(i));
    namen.emplace_back("degeneriert_eine_probe");
    namen.emplace_back("degeneriert_tote_proben");

    std::vector<cmd::ExecutionResult> nur_echte;
    for (std::size_t i = 0; i < 7; ++i) {
        cmd::ExecutionResult c{};
        c.engine_name        = namen[i];
        c.latency_samples_ns = fein_samples(kBasisZentrum - kEchteVersaetze[i], kSamplesJeKandidat);
        nur_echte.push_back(c);
    }
    std::vector<cmd::ExecutionResult> mit_degenerierten = nur_echte;
    {
        cmd::ExecutionResult eine{}; // Klasse 1: n<2 -> Welch rechnet gar nicht
        eine.engine_name        = namen[7];
        eine.latency_samples_ns = {123456};
        mit_degenerierten.push_back(eine);
        cmd::ExecutionResult tot{}; // Klasse 2: Proben da, aber alle 0 -> DATEN-Degeneration
        tot.engine_name        = namen[8];
        tot.latency_samples_ns = std::vector<std::int64_t>(35, 0); // KOEDER, gewuerfelt: n=35
        mit_degenerierten.push_back(tot);
    }

    auto const rep9 =
        stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{mit_degenerierten}, 0.05);
    auto const rep7 =
        stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{nur_echte}, 0.05);
    ASSERT_EQ(rep9.comparisons.size(), 9u);
    ASSERT_EQ(rep7.comparisons.size(), 7u);

    // ── PRAEMISSE des Koeders, gepinnt statt vorausgesetzt: der Kipp-Kandidat liegt WIRKLICH im
    //    Fenster. Wird der Koeder je stumpf (andere Formel, andere Samples), faellt genau hier auf.
    double const roh = rep9.comparisons[0].raw_p;
    ASSERT_GT(roh, 0.05 / 9.0) << "Kipp-Koeder stumpf: p_roh zu klein, er waere auch mit m=9 signifikant";
    ASSERT_LT(roh, 0.05 / 7.0) << "Kipp-Koeder stumpf: p_roh zu gross, er wird auch mit m=7 nicht signifikant";

    // ── DER INVARIANZ-BEWEIS: derselbe Datensatz MIT und OHNE die degenerierten Kandidaten muss
    //    fuer die ECHTEN identische korrigierte p-Werte liefern. Das Orakel ist der zweite Lauf,
    //    also eine unabhaengige Rechnung ueber eine andere Eingabemenge -- nicht die Funktion,
    //    die geprueft wird, und nicht aus einer Doku abgeschrieben.
    for (std::size_t i = 0; i < 7; ++i) {
        EXPECT_DOUBLE_EQ(rep9.comparisons[i].adjusted_p, rep7.comparisons[i].adjusted_p)
            << "Kandidat " << i << ": zwei nie gemessene Zellen verschieben sein Urteil";
        EXPECT_EQ(rep9.comparisons[i].significant, rep7.comparisons[i].significant);
        EXPECT_DOUBLE_EQ(rep9.comparisons[i].robust_adjusted_p, rep7.comparisons[i].robust_adjusted_p);
    }

    // ── DER KIPP-BEWEIS: mit m=7 ist der Kandidat signifikant. Mit dem alten m=9 waere er es
    //    nicht -- und das wird hier nicht behauptet, sondern nachgerechnet: (m-k)*p bei k=0.
    EXPECT_TRUE(rep9.comparisons[0].significant) << "adjusted_p=" << rep9.comparisons[0].adjusted_p;
    EXPECT_DOUBLE_EQ(rep9.comparisons[0].adjusted_p, 7.0 * roh);
    EXPECT_GT(9.0 * roh, 0.05) << "so sah es vorher aus: 9 * " << roh << " = " << (9.0 * roh);

    // ── Die degenerierten selbst: nicht getestet, nie signifikant, neutraler korrigierter Wert.
    for (std::size_t i = 7; i < 9; ++i) {
        EXPECT_FALSE(rep9.comparisons[i].getestet_welch) << "Kandidat " << namen[i];
        EXPECT_FALSE(rep9.comparisons[i].significant);
        EXPECT_DOUBLE_EQ(rep9.comparisons[i].adjusted_p, 1.0);
    }
}

TEST(F15BonferroniNenner, WinRateTeiltDurchDieGetesteteMengeUndNennntSie) {
    // Nenner in der AUSGABE (Pruefung 1): "X (Y von Z getestet, D degeneriert)" statt einer
    // nackten Quote. Aufbau: 1 klar schnellerer echter Kandidat, 1 gleich schneller echter,
    // 2 degenerierte -> heute waere win_rate 1/4 = 0.25, richtig ist 1/2 = 0.5.
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "baseline";
    baseline.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);

    cmd::ExecutionResult schnell{};
    schnell.engine_name        = "schnell";
    schnell.latency_samples_ns = fein_samples(kBasisZentrum - 5000, kSamplesJeKandidat);
    cmd::ExecutionResult gleich{};
    gleich.engine_name        = "gleich";
    gleich.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    cmd::ExecutionResult tot{};
    tot.engine_name        = "tot";
    tot.latency_samples_ns = std::vector<std::int64_t>(35, 0);
    cmd::ExecutionResult eine{};
    eine.engine_name        = "eine_probe";
    eine.latency_samples_ns = {4711};

    std::vector<cmd::ExecutionResult> cands{schnell, gleich, tot, eine};
    auto const rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    auto const sum = stats::summarize(rep);

    EXPECT_EQ(sum.total, 4u);
    EXPECT_EQ(sum.getestet, 2u);
    EXPECT_EQ(sum.degeneriert, 2u);
    EXPECT_EQ(sum.significant_faster, 1u);
    EXPECT_NEAR(sum.win_rate, 0.5, 1e-12) << "1 von 2 GETESTETEN, nicht 1 von 4 Kandidaten";
    // Die vier Kategorien decken die Grundgesamtheit vollstaendig ab -- ohne diese Zusicherung
    // koennte ein Kandidat still in keiner Kategorie landen.
    EXPECT_EQ(sum.significant_faster + sum.significant_slower + sum.not_significant + sum.degeneriert, sum.total);

    // Die GANZE Zeile wird gepinnt, nicht ein Teilstueck: ein Fragment-Treffer waere auch dann
    // noch gruen, wenn Zaehler und Nenner vertauscht sind. Die Erwartung ist aus dem Formatvertrag
    // "<quote> (<schneller> von <getestet> getestet, <degeneriert> degeneriert, Grundgesamtheit
    // <total>)" von Hand abgeleitet, nicht aus der Ausgabe abgeschrieben.
    EXPECT_EQ(stats::win_rate_zeile(sum), "0.5 (1 von 2 getestet, 2 degeneriert, Grundgesamtheit 4)");
}

TEST(F15BonferroniNenner, ToteProbenGewinnenNichtDenVergleich) {
    // DIE ASYMMETRISCHE DEGENERATION, die weder D4a noch D4b faengt: EIN Kandidat mit lauter
    // Nullen gegen eine normal streuende Baseline. se>0 (die Baseline traegt die Varianz), Welch
    // rechnet durch, mean_a=0 -> "unschlagbar schnell", p winzig, faster_than_baseline=true.
    // Das ist eine DATEN-Eigenschaft, keine Test-Eigenschaft -- deshalb prueft sie multi_compare
    // an der Eingabe und nicht der t-Test an seinem Ergebnis.
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "baseline";
    baseline.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    cmd::ExecutionResult tot{};
    tot.engine_name        = "praeparierte_null_dll";
    tot.latency_samples_ns = std::vector<std::int64_t>(35, 0); // KOEDER, gewuerfelt: n=35
    std::vector<cmd::ExecutionResult> cands{tot};

    auto const rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    ASSERT_EQ(rep.comparisons.size(), 1u);
    EXPECT_TRUE(rep.comparisons[0].proben_tot);
    EXPECT_FALSE(rep.comparisons[0].getestet_welch);
    EXPECT_FALSE(rep.comparisons[0].significant) << "eine tote Probe gewinnt keinen Vergleich";
    EXPECT_FALSE(rep.comparisons[0].faster_than_baseline);
    // GEGENPROBE (T-4): der Welch-Rohbefund WAERE hochsignifikant gewesen -- der Ausschluss
    // passiert also wirklich in der Eingangspruefung und nicht, weil der Test ohnehin nichts fand.
    EXPECT_TRUE(rep.comparisons[0].welch.valid);
    EXPECT_LT(rep.comparisons[0].welch.p_value, 0.001);
}

TEST(F15BonferroniNenner, ToteBaselineMachtJedenVergleichUngetestet) {
    // Die Baseline ist die Schwesterstelle des Kandidaten (T-6): ist SIE tot, ist kein einziger
    // Vergleich mehr aussagefaehig -- und dann darf auch keine win_rate ueber 0 entstehen.
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "tote_baseline";
    baseline.latency_samples_ns = std::vector<std::int64_t>(35, 0);
    cmd::ExecutionResult a{};
    a.engine_name        = "a";
    a.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    cmd::ExecutionResult b{};
    b.engine_name        = "b";
    b.latency_samples_ns = fein_samples(kBasisZentrum - 5000, kSamplesJeKandidat);
    std::vector<cmd::ExecutionResult> cands{a, b};

    auto const rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    auto const sum = stats::summarize(rep);
    EXPECT_EQ(sum.getestet, 0u);
    EXPECT_EQ(sum.degeneriert, 2u);
    EXPECT_DOUBLE_EQ(sum.win_rate, 0.0);
    // "0 von 0 getestet" ist etwas voellig anderes als "0 von 2 haben gewonnen" -- und genau das
    // war vorher nicht unterscheidbar, weil die Quote durch die Gesamtzahl teilte.
    EXPECT_EQ(stats::win_rate_zeile(sum), "0 (0 von 0 getestet, 2 degeneriert, Grundgesamtheit 2)");
}

TEST(F15BonferroniNenner, GesundeFamilieBleibtUnveraendert) {
    // GEGENEINGANG / K13 beide Richtungen: ohne einen einzigen degenerierten Kandidaten muss sich
    // NICHTS aendern. Faellt das, war die Heilung eine pauschale Abwertung statt einer Trennung.
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "baseline";
    baseline.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    std::vector<cmd::ExecutionResult> cands;
    for (std::size_t i = 0; i < 7; ++i) {
        cmd::ExecutionResult c{};
        c.engine_name        = "echt";
        c.latency_samples_ns = fein_samples(kBasisZentrum - kEchteVersaetze[i], kSamplesJeKandidat);
        cands.push_back(c);
    }
    auto const rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);
    auto const sum = stats::summarize(rep);
    EXPECT_EQ(sum.total, 7u);
    EXPECT_EQ(sum.getestet, 7u);
    EXPECT_EQ(sum.degeneriert, 0u);
    for (auto const& c : rep.comparisons) {
        EXPECT_TRUE(c.getestet_welch);
        EXPECT_DOUBLE_EQ(c.raw_p, c.welch.p_value);
    }
    // Der Kipp-Kandidat ist auch hier signifikant: m=7 in beiden Faellen.
    EXPECT_TRUE(rep.comparisons[0].significant);
    EXPECT_DOUBLE_EQ(rep.comparisons[0].adjusted_p, 7.0 * rep.comparisons[0].raw_p);
    static_assert(kEchteVersaetze[0] == kKippVersatz, "der erste Kandidat ist der Kipp-Kandidat");
}

TEST(F15ExportRobust, CsvUndJsonTragenDieDegenerationsFelderUndDenNenner) {
    cmd::ExecutionResult baseline{};
    baseline.engine_name        = "baseline";
    baseline.latency_samples_ns = fein_samples(kBasisZentrum, kSamplesJeKandidat);
    cmd::ExecutionResult schnell{};
    schnell.engine_name        = "schnell";
    schnell.latency_samples_ns = fein_samples(kBasisZentrum - 5000, kSamplesJeKandidat);
    cmd::ExecutionResult tot{};
    tot.engine_name        = "tot";
    tot.latency_samples_ns = std::vector<std::int64_t>(35, 0);
    std::vector<cmd::ExecutionResult> cands{schnell, tot};
    auto const rep = stats::multi_compare_against_baseline(baseline, std::span<const cmd::ExecutionResult>{cands}, 0.05);

    auto const csv = stats::report_to_csv(rep);
    EXPECT_NE(csv.find("welch_degeneriert,mwu_degeneriert,proben_tot,getestet_welch,getestet_mwu"), std::string::npos)
        << csv;
    EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), 3); // Header + 2 Zeilen

    auto const json = stats::report_to_json(rep);
    // Der JSON-Kopf traegt die Grundgesamtheit, nicht nur den Zaehler.
    EXPECT_NE(json.find("\"total\":2"), std::string::npos) << json;
    EXPECT_NE(json.find("\"getestet_welch\":1"), std::string::npos) << json;
    EXPECT_NE(json.find("\"degeneriert\":1"), std::string::npos) << json;
    EXPECT_NE(json.find("\"proben_tot\":true"), std::string::npos) << json;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// D4d (2026-08-09) — success HEISST "es gab eine echte Probe", nicht "der Vektor ist nicht leer"
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
// DER BEFUND: make_execution_result setzte success = !latency_samples_ns.empty(). 35 Proben von
// exakt 0 ns galten damit als Erfolg. Der bestehende Test MakeExecutionResultFillsPercentiles
// prueft nur "100 positive Werte -> true" und "leer -> false" -- beide Faelle sind unter alter
// wie neuer Definition gleich, die Luecke war fuer ihn UNSICHTBAR. Er hat Deckung vorgetaeuscht.
//
// KOEDER (K13, gewuerfelt 2026-08-09T18:31:31Z): n = 35 Proben, alle exakt 0.

TEST(F15ProbenSindTot, LauterNullenIstKeinErfolgSondernDegeneration) {
    std::vector<std::int64_t> const nullen(35, 0); // KOEDER, gewuerfelt
    auto const r = cmd::make_execution_result("tote_dll", nullen);
    ASSERT_EQ(r.latency_samples_ns.size(), 35u) << "die Proben SIND da -- nur ohne Aussage";
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.degeneriert);
}

TEST(F15ProbenSindTot, EineEinzigeEchteProbeGenuegt) {
    // GEGENEINGANG (T-4): 34 Nullen und EINE echte Probe -> success. Die Schwelle liegt bei
    // "mindestens eine", nicht bei "alle" -- sonst waere die Wache viel zu breit und wuerde jede
    // Messung mit Nullen unter der Uhr-Aufloesung verwerfen.
    std::vector<std::int64_t> gemischt(35, 0);
    gemischt[17] = 1;
    auto const r = cmd::make_execution_result("fast_tot", gemischt);
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.degeneriert);
}

TEST(F15ProbenSindTot, NegativeProbenZaehlenNichtAlsErfolg) {
    // Negative Latenzen sind ein Zeitnahme-DEFEKT. Eine Reihe aus Nullen und negativen Werten
    // haette ein ">= 0" durchgelassen -- deshalb steht dort "> 0".
    std::vector<std::int64_t> const kaputt{0, -5, 0, -1200, 0};
    auto const                      r = cmd::make_execution_result("defekte_uhr", kaputt);
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.degeneriert);
}

TEST(F15ProbenSindTot, LeereProbenBleibenOhneErfolg) {
    auto const r = cmd::make_execution_result("keine", {});
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.degeneriert);
}

TEST(F15ProbenSindTot, PraedikatIstDieEineDefinitionUndHatBeideRichtungen) {
    std::vector<std::int64_t> const leer{};
    std::vector<std::int64_t> const nullen(35, 0);
    std::vector<std::int64_t> const negativ{-1, -2};
    std::vector<std::int64_t> const echt{0, 0, 7};
    EXPECT_TRUE(cmd::proben_sind_tot(std::span<const std::int64_t>{leer}));
    EXPECT_TRUE(cmd::proben_sind_tot(std::span<const std::int64_t>{nullen}));
    EXPECT_TRUE(cmd::proben_sind_tot(std::span<const std::int64_t>{negativ}));
    EXPECT_FALSE(cmd::proben_sind_tot(std::span<const std::int64_t>{echt})); // hier faellt es
}

TEST(F15ProbenSindTot, ResultCsvHeaderIstEINGEFROREN) {
    // ERSATZ-ORAKEL fuer die auf DIESER Datei fehlende Schema-Wache (MT-L3 bewacht die grosse
    // Produktions-Mess-CSV in cache_engine_builder_iterator.hpp, nicht diesen Export). Der
    // Header steht hier als LITERAL, damit eine weitere Spalte nicht unbemerkt entsteht: die
    // eine Spaltenaenderung dieser Woche ist `degeneriert`, danach ist der Kopf zu.
    EXPECT_EQ(cmd::result_csv_header(),
              "engine_name,workload_kind,throughput_ops_per_sec,latency_p50_ns,latency_p99_ns,"
              "total_cache_misses,memory_footprint_bytes,H1_clu_improvement,H2_layout_score,"
              "H3_inline_external_ratio,n_latency_samples,success,degeneriert");
    // Die Zeile traegt genau so viele Felder wie der Kopf Spalten hat -- ein Kopf, der laenger
    // ist als seine Zeilen, waere ein CSV, das niemand bemerkt, bis die Auswertung schief liegt.
    cmd::ExecutionResult r{};
    r.engine_name              = "x";
    auto const zaehle_kommata  = [](std::string const& s) { return std::count(s.begin(), s.end(), ','); };
    EXPECT_EQ(zaehle_kommata(cmd::to_csv_row(r)), zaehle_kommata(cmd::result_csv_header()));
    EXPECT_NE(cmd::to_json_object(r).find("\"degeneriert\":"), std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// D4e (2026-08-09) — f15-CLI: TOTE-PROBEN-GRUPPE + SUMMENZEILE A+B+C+D+gemessen == geladen
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
// DER BEFUND: die Lade-Schleife der CLI hatte drei benannte Ausschluss-Gruende, aber keinen
// Zaehler; am Ende stand nur "N gemessen" -- ohne Grundgesamtheit. Und eine vierte Gruppe fehlte
// ganz: ein Ergebnis aus lauter Nullen hat p50=0 und GEWINNT jedes p50-Ranking.
//
// Die Bilanz liegt bewusst in einem eigenen Header (lade_bilanz.hpp) und nicht in der main():
// der einzige heutige Test der CLI ist ein CTest-Regex auf ihre Ausgabe, gefahren gegen echte
// R5.G-DLLs -- also gegen Daten, in denen der degenerierte Fall per Konstruktion nicht vorkommt.
// Ein Regex auf einer Ausgabe, in der der Fall nicht auftreten KANN, ist kein Testat.

TEST(F15LadeBilanz, SummeGehtAufUndDieZeileNenntDenNenner) {
    cmd::LadeBilanz b{};
    b.geladen            = 40;
    b.gemessen           = 31;
    b.nicht_messfaehig   = 3;
    b.konformitaet_fehlt = 2;
    b.zu_wenige_proben   = 3;
    b.tote_proben        = 1;
    EXPECT_EQ(b.verbucht(), 40u);
    EXPECT_TRUE(b.summe_stimmt());
    EXPECT_EQ(b.zeile(), "BILANZ: 40 geladen = 31 gemessen + 3 nicht mess-faehig + 2 Konformitaet + "
                         "3 zu wenige Proben + 1 tote Proben");
}

TEST(F15LadeBilanz, EineUnverbuchteBinaryWirdALSBEFUNDGEMELDET) {
    // GEGENEINGANG (T-4): genau EINE Binary verschwindet. Die alte CLI haette weiterhin "31
    // gemessen" gedruckt und nichts bemerkt.
    cmd::LadeBilanz b{};
    b.geladen          = 40;
    b.gemessen         = 31;
    b.nicht_messfaehig = 3;
    b.zu_wenige_proben = 5; // 31+3+5 = 39, eine fehlt
    EXPECT_FALSE(b.summe_stimmt());
    EXPECT_NE(b.zeile().find("Summe geht NICHT auf"), std::string::npos) << b.zeile();
    EXPECT_NE(b.zeile().find("verbucht=39"), std::string::npos) << b.zeile();
    EXPECT_EQ(cmd::lade_bilanz_exit_code(b), cmd::kExitBilanzGehtNichtAuf);
}

TEST(F15LadeBilanz, ToteProbenErzeugenEinenEIGENENExitCode) {
    cmd::LadeBilanz b{};
    b.geladen     = 4;
    b.gemessen    = 3;
    b.tote_proben = 1;
    EXPECT_TRUE(b.summe_stimmt());
    EXPECT_EQ(cmd::lade_bilanz_exit_code(b), 6);
    // 5 war schon vergeben (Lastprofil-Pfad: "keine einzige DLL gemessen") -- deshalb 6.
    EXPECT_NE(cmd::lade_bilanz_exit_code(b), 5);
}

TEST(F15LadeBilanz, EineSAUBEREBilanzGibtNull) {
    // Die andere Richtung (K13): ohne tote Proben und mit aufgehender Summe darf NICHTS anschlagen.
    cmd::LadeBilanz b{};
    b.geladen            = 12;
    b.gemessen           = 10;
    b.konformitaet_fehlt = 2;
    EXPECT_TRUE(b.summe_stimmt());
    EXPECT_TRUE(b.hat_messung());
    EXPECT_EQ(cmd::lade_bilanz_exit_code(b), 0);
}

TEST(F15LadeBilanz, KaputteZaehlungWiegtSchwererAlsToteProben) {
    // Beide Befunde gleichzeitig: dann muss der Zaehl-Befund gewinnen. Wenn die Bilanz nicht
    // aufgeht, ist auch die Zahl der toten Proben nicht belastbar.
    cmd::LadeBilanz b{};
    b.geladen     = 10;
    b.gemessen    = 5;
    b.tote_proben = 2; // 7 != 10
    EXPECT_EQ(cmd::lade_bilanz_exit_code(b), cmd::kExitBilanzGehtNichtAuf);
}

TEST(F15LadeBilanz, DieVierteGruppeEntstehtAusMakeExecutionResult) {
    // Die NAHT zu D4d: die CLI entscheidet nicht selbst, was eine tote Probe ist -- sie liest
    // ExecutionResult::degeneriert. Hier wird genau diese Kette in Miniatur gefahren, mit dem
    // gewuerfelten Koeder aus D4d (35 Nullen), damit die CLI-Aenderung ein Orakel hat, das ohne
    // DLL-Laden auskommt.
    cmd::LadeBilanz           b{};
    std::vector<std::vector<std::int64_t>> proben_je_dll{
        {10, 20, 30, 40},                     // echt
        std::vector<std::int64_t>(35, 0),     // KOEDER, gewuerfelt: tot
        {11, 22, 33},                         // echt
    };
    b.geladen = proben_je_dll.size();
    for (auto& p : proben_je_dll) {
        auto const r = cmd::make_execution_result("dll", p);
        if (r.degeneriert)
            ++b.tote_proben;
        else
            ++b.gemessen;
    }
    EXPECT_EQ(b.geladen, 3u);
    EXPECT_EQ(b.gemessen, 2u);
    EXPECT_EQ(b.tote_proben, 1u);
    EXPECT_TRUE(b.summe_stimmt());
    EXPECT_EQ(cmd::lade_bilanz_exit_code(b), 6);
}
