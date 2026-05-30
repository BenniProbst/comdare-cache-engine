#pragma once
// V41.F.6.1.R5.C.A3 — MammalAbiAdapter Production-Header (Module-Loader-Bruecke)
//
// User-Direktive 2026-05-27 frueh (Session-Doku Teil II §18, NEXT-Action R5.C.A3):
// "AnatomyAbiAdapter Template von Test §5 in eigenen Production-Header verschieben
//  (anatomy/abi_adapter.hpp). Wird in R5.E fuer Module-Factory benoetigt."
//
// Architektur-Rolle:
// `MammalAbiAdapter<A>` bridge eine konkrete `SearchAlgorithmAnatomy<Composition>`
// (Mammal-Gattung, Compile-Time-Concept) zur Runtime-ABI-Schicht (`IAnatomyBase`
// → `IExecutionEngine`). Ein generierter Permutations-Binary (.so/.dll) wird in
// R5.D/R5.E exakt EINEN solchen Adapter via `extern "C"` Factory exportieren.
//
// Bisheriger Stand: Adapter-Pattern war in tests/ lokal dupliziert in zwei Varianten:
//   - test_v41_anatomy_base.cpp:110 AnatomyAbiAdapter (unvollstaendig, fehlte
//     Lifecycle-Override — broken seit R5.C.A2 Wurzel-Inheritance)
//   - test_v41_execution_engine.cpp:115 MammalAbiAdapter (vollstaendig, Vorlage)
//
// R5.C.A3 promoviert die vollstaendige Variante zum Production-Header und ersetzt
// beide lokalen Test-Klassen.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §35.3 + Session-Doku Teil II §18
// @task #702 V41.F.6.1.R5.C.A3
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]
//          [[anatomie-nur-achsen-und-observer]]

#include "anatomy_base.hpp"
#include "measurable_workload.hpp"   // F15/Stufe B: optionales Mess-Sub-Interface (ABI-sicher)
#include "observable_tier.hpp"       // R6/Pfad B: ABI-stabiler Observer-Zugriff (Doku 24 §8.6)
#include "observer_aggregate.hpp"    // ObservableAxis + ObserverAggregate (observable_count)
#include "../execution_engine/execution_engine_base.hpp"
// R6 Inkrement 2b: die allocator-Achse im Cross-ABI-Observer-POD — der ComposedStore<N,L,A>-Vector-Growth
// treibt die Allocator-Statistik REAL (2. Mess-Achse, spiegelt builder/AnatomyExecutionContext). Da der
// host-seitige Loader jetzt das LEICHTE anatomy_module_abi_v1_decl.hpp nutzt (NICHT mehr abi_adapter.hpp),
// belasten diese topics/-Includes nur die Voll-Header-Konsumenten (DLLs/Tests, die die Pfade ohnehin haben).
#include "../topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// SearchAlgorithmAbiAdapter — bridge AnatomyConcept-konformer SearchAlgorithm-
// Anatomie zu IAnatomyBase (Mammal-Gattung in Tier-Metapher, Doku 14 §27.2)
// ─────────────────────────────────────────────────────────────────────────────

/// SearchAlgorithmAbiAdapter<A> — generischer Runtime-ABI-Adapter fuer die
/// SearchAlgorithm-Gattung (Mammal in Tier-Metapher).
///
/// Vorbedingung: `A` erfuellt `AnatomyConcept` UND `A::genus() ==
/// AnatomyGenus::SearchAlgorithm`. Der `static_assert` im Klassen-Body validiert
/// die Gattung zur Compile-Zeit (Doku 14 §32 Gattungs-Constraint).
///
/// Verwendung (R5.E Module-Factory-Pattern):
/// ```cpp
/// // In generiertem Permutations-Binary (.so/.dll):
/// extern "C" comdare::cache_engine::anatomy::IAnatomyBase*
/// comdare_create_anatomy() {
///     using A = comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<MyComposition>;
///     return new comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<A>{};
/// }
/// ```
///
/// Lifecycle-Implementierung: der interne `state_` reflektiert die Pflicht-
/// Phasen `Uninitialized → Warming → Running → Idle → Shutdown`. Pilot R5.C.A3
/// setzt state_ in den entsprechenden Hooks. Echte Cache-Preheat/Bulk-Load
/// kommt mit R5.D (CacheEngineBuilder Workload-Treiber).
template <AnatomyConcept A>
class SearchAlgorithmAbiAdapter final : public IAnatomyBase, public IMeasurableWorkload,
                                        public IObservableTier {
    static_assert(A::genus() == AnatomyGenus::SearchAlgorithm,
                  "SearchAlgorithmAbiAdapter erwartet eine SearchAlgorithm-Gattung-"
                  "Anatomie (AnatomyGenus::SearchAlgorithm). Cross-Genus-Adapter "
                  "sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    // ─────────────────────────────────────────────────────────────────────
    // IExecutionEngine-Pflicht-Override (R5.C.A2 Wurzel-Schicht)
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view engine_name() const noexcept override {
        return A::composition_name();
    }

    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }

    void warm_up() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming;
    }

    /// R5.C.A4: Mess-Phase aktivieren — Lifecycle Warming/Idle → Running.
    void run() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running;
    }

    void reset() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle;
    }

    void shutdown() override {
        state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown;
    }

    // ─────────────────────────────────────────────────────────────────────
    // IAnatomyBase-Pflicht-Override (R5.C.A Anatomie-Schicht)
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view composition_name() const noexcept override {
        return A::composition_name();
    }

    [[nodiscard]] std::string_view paper_id() const noexcept override {
        return A::paper_id();
    }

    [[nodiscard]] AnatomyGenus genus() const noexcept override {
        return A::genus();
    }

    [[nodiscard]] std::size_t organ_count() const noexcept override {
        return A::organ_count();
    }

    // ─────────────────────────────────────────────────────────────────────
    // IMeasurableWorkload-Override (F15 / Stufe B): Mess-Last DURCH die DLL
    // ─────────────────────────────────────────────────────────────────────

    /// KOMPOSIT-Mess-Last (R5.B, F15 Stufe B) — uebt DREI Achsen der geladenen Komposition aus:
    ///   Segment 1: insert/lookup auf A::composition_t::search_algo  (search_algo-Achse)
    ///   Segment 2: alloc/dealloc-Churn ueber A::composition_t::allocator (allocator-Achse)
    ///   Segment 3: Feld-Scan ueber A::composition_t::memory_layout (memory_layout-Achse; AoS-strided
    ///              vs SoA-contiguous → echter Cache-Effekt)
    /// Alle drei kompiliert IN der DLL; die Batch-Latenz misst die GANZE Komposition entlang der
    /// variierten Achsen. F15s paarweiser Holm-Test isoliert jede Achse (Paare, die sich nur in einer
    /// Achse unterscheiden) — Dominanz/Balance der Segmente ist dafuer irrelevant.
    [[nodiscard]] std::uint64_t run_workload(std::uint64_t ops_per_batch,
                                             std::uint64_t batches,
                                             std::uint64_t seed,
                                             std::int64_t* out_latencies_ns,
                                             std::uint64_t out_capacity) noexcept override {
        if (out_latencies_ns == nullptr || out_capacity == 0 || batches == 0 || ops_per_batch == 0) {
            return 0;
        }
        try {
            using SearchAlgo = typename A::composition_t::search_algo;
            using Allocator  = typename A::composition_t::allocator;       // R5.B: 2. operative Achse
            using MemLayout  = typename A::composition_t::memory_layout;   // R5.B: 3. operative Achse
            using K          = typename SearchAlgo::key_type;
            SearchAlgo algo;
            for (int k = 0; k < 256; ++k) {
                algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u);
            }
            Allocator alloc;                              // Komposition-Allocator (persistiert ueber Batches)
            constexpr std::size_t kChurn = 2048;          // moderate alloc/dealloc-Last je Batch
            constexpr std::size_t kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const churn_size = [](std::size_t j) noexcept {
                return std::size_t{16} + ((j * 16u) & 0xF0u);  // 16..256 Bytes, deterministisch
            };
            // Layout-Scan-Puffer (Segment 3): 16384 Datensaetze × 64 B = 1 MB (> L1/L2). EINMAL via
            // Komposition-Allocator alloziert (Setup, NICHT gemessen); pro Batch nur der Scan.
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 64;
            constexpr std::size_t kLbufBytes  = kRecords * kRecordSize;
            unsigned char* lbuf = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);
            std::mt19937_64 rng{seed};
            auto do_batch = [&]() -> std::int64_t {
                std::uint64_t sink = 0;
                auto const t0 = std::chrono::steady_clock::now();
                // Segment 1: search_algo-Achse (Lookups auf der geladenen Such-Struktur)
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
                    if (v) sink += *v;
                }
                // Segment 2: allocator-Achse (alloc N → touch → dealloc N; Pool reused Free-Lists)
                for (std::size_t j = 0; j < kChurn; ++j) {
                    void* p = alloc.allocate(churn_size(j), kAlign);
                    *static_cast<unsigned char*>(p) = static_cast<unsigned char>(j);  // touch
                    sink += *static_cast<unsigned char*>(p);
                    blocks[j] = p;
                }
                for (std::size_t j = 0; j < kChurn; ++j) {
                    alloc.deallocate(blocks[j], churn_size(j), kAlign);
                }
                // Segment 3: memory_layout-Achse (Feld-Scan im layout-charakteristischen Zugriffsmuster)
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                auto const t1 = std::chrono::steady_clock::now();
                auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                return (sink == ~0ull) ? (ns ^ 1) : ns;  // sink-Nutzung gegen Wegoptimierung
            };
            do_batch();  // Warmup (verworfen)
            std::uint64_t const n = (batches < out_capacity) ? batches : out_capacity;
            for (std::uint64_t b = 0; b < n; ++b) out_latencies_ns[b] = do_batch();
            alloc.deallocate(lbuf, kLbufBytes, 64);   // Layout-Puffer freigeben
            return n;
        } catch (...) {
            return 0;  // noexcept-Vertrag: interne Exception (z.B. OOM) → 0 Samples
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    // IObservableTier-Override (R6 / Pfad B, Doku 24 §8.6): Der Host treibt die GATTUNGS-API +
    // zieht die IM Tier eingebauten Observer als flachen POD durch die ABI-Grenze. Getrieben wird
    // das ECHTE sezierte Composition-Such-Organ (gemeinsamer uint64-Key nach Umstufung-B) — GETRENNT
    // vom Pfad-A-`run_workload` (lokales Wegwerf-Organ); beide koexistieren (Hybrid-Modell §8.1).
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
        // Neu-Flag ueber occupied_count-Delta (NICHT ueber einen internen lookup — der wuerde sonst die
        // lookup_count-Observer-Statistik verfaelschen; manche Organe insert()->void).
        auto const before = search_organ_.occupied_count();
        search_organ_.insert(key, value);
        container_.insert(key, value);       // treibt + MISST die allocator-Achse (ComposedStore-Vector-Growth)
        return search_organ_.occupied_count() > before;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        auto const v = search_organ_.lookup(key);
        if (v.has_value() && out_value != nullptr) *out_value = *v;
        return v.has_value();
    }

    [[nodiscard]] bool tier_erase(std::uint64_t key) noexcept override {
        auto const before = search_organ_.occupied_count();
        search_organ_.erase(key);            // Rueckgabe-Typ organ-abhaengig → ueber Delta bestimmen
        container_.erase(key);
        return search_organ_.occupied_count() < before;
    }

    void tier_clear() noexcept override { search_organ_.clear(); container_.clear(); }

    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(search_organ_.occupied_count());
    }

    void tier_observe(ComdareTierObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        ComdareTierObserverSnapshotV1 snap{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (ObservableAxis<SearchAlgo>) {
            auto const s = search_organ_.statistics();   // SearchAlgoStatistics (echtes getriebenes Organ)
            snap.search_lookup_count   = s.total_lookup_count;
            snap.search_hit_count      = s.total_hit_count;
            snap.search_miss_count     = s.total_miss_count;
            snap.search_insert_count   = s.total_insert_count;
            snap.search_erase_count    = s.total_erase_count;
            snap.search_peak_occupancy = s.peak_occupancy;
        }
        // Achse 2 (allocator) — der innere ComposedStore-Vector treibt die Allocator-Achse REAL (Doppeltes
        // Gate wie AnatomyExecutionContext: Composition::allocator observable UND Store bietet allocator-Stats).
        if constexpr (ObservableAxis<typename Composition::allocator>
                   && container_t::template store_has_allocator_stats<typename container_t::store_type>) {
            auto const a = container_.store_allocator_statistics();   // AllocationStatistics
            snap.alloc_bytes_allocated    = a.total_bytes_allocated;
            snap.alloc_bytes_in_use       = a.total_bytes_in_use;
            snap.alloc_allocation_count   = a.allocation_count;
            snap.alloc_deallocation_count = a.deallocation_count;
            snap.alloc_failure_count      = a.failure_count;
        }
#endif
        snap.observable_axis_count = ObserverAggregate<Composition>::observable_count();
        snap.tier_fill_level       = tier_size();
        *out = snap;
    }

private:
    using Composition = typename A::composition_t;
    using SearchAlgo  = typename Composition::search_algo;
    // allocator-messender Container (spiegelt builder/AnatomyExecutionContext): ComposedStore<N,L,A> über
    // die Composition-Achsen → dessen Vector-Growth treibt die Allocator-Statistik real.
    using container_t = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::ObservableComposedSearch<
        ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::SortedBinaryTraversal,
        ::comdare::cache_engine::nodes::axis_04_node_type::ComposedStore<
            typename Composition::node_type, typename Composition::memory_layout, typename Composition::allocator>>;

    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
    // R6 / Pfad B: das getriebene ECHTE Composition-Such-Organ (uint64-Key). Default-konstruiert;
    // Default-konstruierbar + ObservableAxis garantiert durch #42 (ObservableComposedContainer-Huelle).
    SearchAlgo  search_organ_{};
    container_t container_{};   // R6 Inkrement 2b: misst die ALLOCATOR-Achse über die ABI-Grenze
};

}  // namespace comdare::cache_engine::anatomy
