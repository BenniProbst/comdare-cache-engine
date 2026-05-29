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
#include <builder/commands/welch_t_test.hpp>

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_vector_u8u8.hpp>
// Dim 2 (Algorithmus-Ebene): volle Kompositionen, gemessen über C::search_algo
#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <random>
#include <span>
#include <string_view>
#include <vector>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace stats  = ::comdare::cache_engine::builder::commands::stats;
namespace ce03a  = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace comp   = ::comdare::cache_engine::compositions;

namespace {

/// Misst Batch-Lookup-Latenzen (ns) einer Such-Datenstruktur (= Innenleben einer std::map<key,value>)
/// mit gleichem 256-Key-Workload. Generisch über `Algo::key_type` (uint8/uint16). Batch-Granularität
/// (ops_per_batch Lookups/Sample) statt Einzel-Op, weil ein direkter Lookup unter der steady_clock-
/// Auflösung liegt. Liefert `batches` Latenz-Samples.
template <class Algo>
std::vector<std::int64_t> measure_lookup_batches(int batches, int ops_per_batch, std::uint64_t seed) {
    using K = typename Algo::key_type;
    Algo algo;
    for (int k = 0; k < 256; ++k) {
        algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u);
    }
    std::mt19937_64 rng{seed};
    auto do_batch = [&]() -> std::int64_t {
        std::uint64_t sink = 0;
        auto const t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < ops_per_batch; ++i) {
            auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
            if (v) sink += *v;
        }
        auto const t1 = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        // sink-Nutzung verhindert Wegoptimierung der Lookups.
        return (sink == ~0ull) ? (ns ^ 1) : ns;
    };
    do_batch();  // Warmup (verworfen)
    std::vector<std::int64_t> samples;
    samples.reserve(static_cast<std::size_t>(batches));
    for (int b = 0; b < batches; ++b) samples.push_back(do_batch());
    return samples;
}

/// Welch-Vergleich zweier Mess-Reihen + Verdikt-Log. label_a/label_b für die F15-Ausgabe.
stats::WelchResult welch_compare(std::vector<std::int64_t> const& a,
                                 std::vector<std::int64_t> const& b,
                                 char const* dim, char const* label_a, char const* label_b) {
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{a},
                                       std::span<const std::int64_t>{b});
    bool const sig = w.valid && w.p_value < 0.05;
    char const* verdict = !sig ? "Tie/Inconclusive"
                        : (w.mean_a < w.mean_b ? label_a : label_b);
    std::cout << "[F15 " << dim << "] " << label_a << " mean=" << w.mean_a << "ns vs "
              << label_b << " mean=" << w.mean_b << "ns  t=" << w.t_statistic
              << " p=" << w.p_value << "  => " << verdict << " schneller\n";
    return w;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// (1) Pipeline: generierte Permutations-DLLs laden + Lifecycle fahren (R5.I-Pattern)
// ─────────────────────────────────────────────────────────────────────────────
TEST(F15Measurement, LoadsGeneratedPermutationDllsAndRunsLifecycle) {
    std::filesystem::path const dir{COMDARE_F15_MEASUREMENT_DIR};
    std::vector<loader::AnatomyModuleHandle> handles;
    int const st = loader::AnatomyModuleLoader::load_all(dir, handles);
    ASSERT_EQ(st, loader::status_ok) << "load_all: " << loader::status_name(st) << " (dir=" << dir << ")";
    ASSERT_GE(handles.size(), 1u) << "Keine Permutations-DLLs geladen.";

    for (auto& h : handles) {
        auto* a = h.anatomy();
        ASSERT_NE(a, nullptr);
        EXPECT_FALSE(a->composition_name().empty());   // Identität aus der DLL
        EXPECT_EQ(a->organ_count(), 17u);              // volle Anatomie (17 Achsen)
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
    constexpr int kBatches = 200;
    constexpr int kOpsPerBatch = 4000;
    auto dense  = measure_lookup_batches<ce03a::Array256SearchAlgo>(kBatches, kOpsPerBatch, 0xF15u);
    auto sorted = measure_lookup_batches<ce03a::VectorU8U8SearchAlgo>(kBatches, kOpsPerBatch, 0xF15u);
    ASSERT_EQ(dense.size(),  static_cast<std::size_t>(kBatches));
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
    constexpr int kBatches = 200;
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

// Welch-Validität-Randfall: <2 Samples → valid=false (dokumentierter Fallback).
TEST(F15Measurement, WelchInvalidOnTooFewSamples) {
    std::vector<std::int64_t> one{42};
    auto const w = stats::welch_t_test(std::span<const std::int64_t>{one},
                                       std::span<const std::int64_t>{one});
    EXPECT_FALSE(w.valid);
}
