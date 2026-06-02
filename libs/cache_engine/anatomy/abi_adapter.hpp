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
#include "rollbackable_tier.hpp"     // V5-I6: memento_all-ABI-Sub-Interface (tier_save_all/tier_rollback_all)
#include "scannable_tier.hpp"        // V5-#49-E: Range-Scan-ABI-Sub-Interface (tier_scan, YCSB-E)
#include "observer_aggregate.hpp"    // ObservableAxis + ObserverAggregate (observable_count)
#include "memento_aggregate.hpp"     // V5-I6-SUBSTANZ (#44): MementoAxis + save_axis/restore_axis (per-Achsen-Memento)
#include "../execution_engine/execution_engine_base.hpp"
// R6 Inkrement 2b: die allocator-Achse im Cross-ABI-Observer-POD — der ComposedStore<N,L,A>-Vector-Growth
// treibt die Allocator-Statistik REAL (2. Mess-Achse, spiegelt builder/AnatomyExecutionContext). Da der
// host-seitige Loader jetzt das LEICHTE anatomy_module_abi_v1_decl.hpp nutzt (NICHT mehr abi_adapter.hpp),
// belasten diese topics/-Includes nur die Voll-Header-Konsumenten (DLLs/Tests, die die Pfade ohnehin haben).
#include "../topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp"

#include <algorithm>     // V5-#49-E: std::sort für den geordneten Range-Scan (tier_scan)
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>   // V5-I6: is_copy_constructible/assignable-Guards für die In-Memory-Memento-Kopie
#include <vector>        // V5-#49-E: save_state()-Snapshot-Liste für den Range-Scan

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
class SearchAlgorithmAbiAdapter final : public IAnatomyBase,
// V5-I2.2/I6: Antrieb ⊥ Observer ⊥ Memento compile-time disjunkt (kein Diamond — IObservableTier/IRollbackableTier IS-A nichts Gemeinsames; IDriveableTier kommt über IObservableTier).
//   MESSUNG-AN  : IObservableTier (Antrieb + tier_observe/observer_all) + IMeasurableWorkload (Pfad-A, host-relokal. I9) + IRollbackableTier (memento_all, V5-I6) + IScannableTier (Range-Scan, V5-#49-E).
//   MESSUNG-AUS : NUR IDriveableTier (funktionaler Antrieb) → Release-/funktional-only-DLL OHNE jeden Mess-Overhead (kein Observer-, kein Memento-, kein Scan-vtable-Slot).
#if COMDARE_MEASUREMENT_ON
                                        public IObservableTier,
                                        public IObservableTierV2,   // V42 L-74c: eigenständiges V2-Sub-Interface (ABI-robust, kein vtable-Append)
                                        public IMeasurableWorkload,
                                        public IRollbackableTier,
                                        public IScannableTier {
#else
                                        public IDriveableTier {
#endif
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
#if COMDARE_MEASUREMENT_ON   // V5-I2.2: Pfad-A run_workload NUR bei Messung-AN (V3-Designfehler; host-relokalisiert in I9)
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
            using Serializer = typename A::composition_t::serialization;   // R5.B: 4. operative Achse (axis_10)
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
                // Segment 4: serialization-Achse (Encode-Scan im strategie-charakteristischen CPU-Aufwand:
                // raw=Byte-Sum < compressed=Delta+Zigzag < var_len=LEB128 < succinct=Bit-Packing). Doku 22 §4.
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
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
#endif  // COMDARE_MEASUREMENT_ON (run_workload / Pfad A)

    // ─────────────────────────────────────────────────────────────────────
    // IDriveableTier-Override (V5-I2.2): funktionaler Gattungs-Antrieb — IMMER einkompiliert (auch Release-DLL).
    // Treibt das ECHTE sezierte Composition-Such-Organ (gemeinsamer uint64-Key nach Umstufung-B). GETRENNT
    // vom Pfad-A-`run_workload` (Messung-AN-only); bei MESSUNG-AN zieht tier_observe zusaetzlich observer_all (Doku 24 §8.6).
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
        // Neu-Flag ueber occupied_count-Delta (NICHT ueber einen internen lookup — der wuerde sonst die
        // lookup_count-Observer-Statistik verfaelschen; manche Organe insert()->void).
        auto const before = search_organ_.occupied_count();
        search_organ_.insert(key, value);
        container_.insert(key, value);       // treibt + MISST die allocator-Achse (ComposedStore-Vector-Growth)
        // V42 L-74c Cross-ABI-Auto-Kopplung: jeder insert berührt einen Blatt-Knoten → telemetry mit-treiben.
        if constexpr (requires { telemetry_organ_.record_node_touch(true); }) telemetry_organ_.record_node_touch(true);
        return search_organ_.occupied_count() > before;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        auto const v = search_organ_.lookup(key);
        if (v.has_value() && out_value != nullptr) *out_value = *v;
        // V42 L-74c: lookup berührt ebenfalls einen Knoten → telemetry mit-treiben (telemetry_organ_ mutable).
        if constexpr (requires { telemetry_organ_.record_node_touch(true); }) telemetry_organ_.record_node_touch(true);
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

#if COMDARE_MEASUREMENT_ON   // V5-I2.2: tier_observe (observer_all) NUR bei Messung-AN
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

    // V42 L-74c: Cross-ABI-Observe in den erweiterten V2-POD (observable_tier.hpp) — V1-Achsen (search_algo +
    // allocator) PLUS die per tier_insert/lookup AUTO-GEKOPPELTE telemetry-Achse. Non-virtual + additiv (kein
    // IObservableTier-vtable-Bruch). Die scan-Achsen (memory_layout/serialization/node_type) sind run_workload-
    // getrieben (Pfad A) und folgen in einem späteren Inkrement über die echte DLL-Grenze.
    void fill_observer_v2(ComdareTierObserverSnapshotV2* out) const noexcept {
        if (out == nullptr) return;
        ComdareTierObserverSnapshotV2 s{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (ObservableAxis<SearchAlgo>) {
            auto const ss = search_organ_.statistics();
            s.search_lookup_count   = ss.total_lookup_count;
            s.search_hit_count      = ss.total_hit_count;
            s.search_miss_count     = ss.total_miss_count;
            s.search_insert_count   = ss.total_insert_count;
            s.search_erase_count    = ss.total_erase_count;
            s.search_peak_occupancy = ss.peak_occupancy;
        }
        if constexpr (ObservableAxis<typename Composition::allocator>
                   && container_t::template store_has_allocator_stats<typename container_t::store_type>) {
            auto const a = container_.store_allocator_statistics();
            s.alloc_bytes_allocated    = a.total_bytes_allocated;
            s.alloc_bytes_in_use       = a.total_bytes_in_use;
            s.alloc_allocation_count   = a.allocation_count;
            s.alloc_deallocation_count = a.deallocation_count;
            s.alloc_failure_count      = a.failure_count;
        }
        if constexpr (ObservableAxis<typename Composition::telemetry>) {
            auto const t = telemetry_organ_.statistics();   // AUTO-gekoppelt via tier_insert/lookup
            s.telemetry_total_events = t.total_events;
            s.telemetry_leaf_updates = t.leaf_updates;
            s.telemetry_node_updates = t.node_updates;
            s.telemetry_peak_tracked = t.peak_tracked;
        }
        // V42 L-74c scan-Achsen-Auto-Kopplung (Pfad-B Zustand-Scan): treibt memory_layout/serialization über das
        // ECHTE Slot-Backing des container_ (organ_observe_layout/serialization am ComposedStore) → ihre
        // statistics() reflektieren die realen Tier-Daten zum Observe-Zeitpunkt. if-constexpr-geschützt.
        if constexpr (ObservableAxis<typename Composition::memory_layout>
                   && requires { container_.store_observe_layout(ml_organ_); }) {
            ml_organ_.reset();   // idempotenter Observe: frischer Zustand-Scan je tier_observe_v2-Aufruf
            (void)container_.store_observe_layout(ml_organ_);
            auto const ml = ml_organ_.statistics();
            s.layout_scan_count          = ml.scan_count;
            s.layout_records_scanned     = ml.records_scanned;
            s.layout_field_bytes_read    = ml.field_bytes_read;
            s.layout_cache_lines_touched = ml.cache_lines_touched;
            s.layout_last_checksum       = ml.last_checksum;
        }
        if constexpr (ObservableAxis<typename Composition::serialization>
                   && requires { container_.store_observe_serialization(ser_organ_); }) {
            ser_organ_.reset();   // idempotenter Observe
            (void)container_.store_observe_serialization(ser_organ_);
            auto const sr = ser_organ_.statistics();
            s.serialization_serialize_count    = sr.serialize_count;
            s.serialization_records_serialized = sr.records_serialized;
            s.serialization_bytes_serialized   = sr.bytes_serialized;
            s.serialization_last_checksum      = sr.last_checksum;
        }
#endif
        s.observable_axis_count = ObserverAggregate<Composition>::observable_count();
        s.tier_fill_level       = tier_size();
        *out = s;
    }

    /// V42 L-74c: IObservableTier-Override — macht den V2-POD über die echte Gattungs-ABI verfügbar
    /// (der Host zieht ihn via dynamic_cast<IObservableTier*> + tier_observe_v2, analog tier_observe-V1).
    void tier_observe_v2(ComdareTierObserverSnapshotV2* out) const noexcept override { fill_observer_v2(out); }
#endif  // COMDARE_MEASUREMENT_ON (tier_observe / observer_all)

#if COMDARE_MEASUREMENT_ON
    // ─────────────────────────────────────────────────────────────────────
    // IRollbackableTier-Override (V5-I6): memento_all — In-Memory-Rollback der getriebenen Organe.
    // Der host-seitige Zwei-Phasen-Treiber (I7) triggert: save_all → op (warmup) → rollback_all → op (measure),
    // sodass der GEMESSENE Op gegen DENSELBEN Vor-Zustand läuft (eliminiert Pfad-Abhängigkeit der Latenz).
    //
    // IN-MEMORY-Memento (search_organ_ + container_ leben komplett im RAM): eine Tiefkopie IST der vollständige,
    // korrekte Memento für die RAM-residenten Achsen (search_algo/node_type/memory_layout/allocator-Arena, die
    // alle im ComposedStore liegen). Für die DISK-residenten Achsen (io_dispatch/serialization/migration) ist
    // „ein einfacher Snapshot reicht NICHT" → explizites Checkpoint-save_state je Achse folgt in V5-I8.
    //
    // if-constexpr-Kopierbarkeits-Guards: kompiliert für JEDE Komposition. Nicht-kopierbare Organe (selten;
    // z.B. Allocator mit OS-Handle) degradieren zu no-op-Rollback — der Treiber (I7) muss diese via
    // Capability-Probe als Kalt-Messung behandeln. Die produktiven SearchAlgorithm-Organe sind kopierbar
    // (build-verifiziert), die Degradation greift dort nicht.
    // ─────────────────────────────────────────────────────────────────────

    // V5-#44-SUBSTANZ: memento_all läuft BEVORZUGT über den per-Achsen-Memento (MementoAxis::save_state/
    // restore_state der Organe — der vom /goal geforderte „je stateful Achsen-Interface"); nur Organe OHNE
    // MementoAxis fallen auf die In-Memory-Tiefkopie zurück. Die produktiven Such-Organe (ComposedSearch /
    // ObservableComposedSearch) SIND MementoAxis (test_v5_organ_memento) → hier läuft der echte per-Achsen-Pfad.
    void tier_save_all() noexcept override {
        try {
            if constexpr (MementoAxis<SearchAlgo>)                        saved_search_m_    = search_organ_.save_state();
            else if constexpr (std::is_copy_constructible_v<SearchAlgo>)  saved_search_.emplace(search_organ_);
            if constexpr (MementoAxis<container_t>)                       saved_container_m_ = container_.save_state();
            else if constexpr (std::is_copy_constructible_v<container_t>) saved_container_.emplace(container_);
        } catch (...) {
            saved_search_.reset();      // OOM beim Kapseln → Kopie-Fallback-Memento verwerfen (Mess-Robustheit)
            saved_container_.reset();
        }
    }

    void tier_rollback_all() noexcept override {
        try {
            if constexpr (MementoAxis<SearchAlgo>)                       search_organ_.restore_state(saved_search_m_);
            else if constexpr (std::is_copy_assignable_v<SearchAlgo>)  { if (saved_search_)    search_organ_ = *saved_search_; }
            if constexpr (MementoAxis<container_t>)                      container_.restore_state(saved_container_m_);
            else if constexpr (std::is_copy_assignable_v<container_t>) { if (saved_container_) container_    = *saved_container_; }
        } catch (...) {
            // noexcept-Vertrag: ein Rollback-Fehler darf den Mess-Lauf nicht abreißen.
        }
    }

    /// Diagnose für den Zwei-Phasen-Treiber (I7): exakter Rollback, wenn jedes Organ ENTWEDER MementoAxis ODER
    /// kopierbar ist. Sonst muss der Treiber Kalt-Messung wählen (empirische Probe in tier_observe_trace_abi.hpp).
    [[nodiscard]] bool tier_rollback_is_exact() const noexcept {
        constexpr bool search_ok = MementoAxis<SearchAlgo>
            || (std::is_copy_constructible_v<SearchAlgo>  && std::is_copy_assignable_v<SearchAlgo>);
        constexpr bool cont_ok = MementoAxis<container_t>
            || (std::is_copy_constructible_v<container_t> && std::is_copy_assignable_v<container_t>);
        return search_ok && cont_ok;
    }

    // ─────────────────────────────────────────────────────────────────────
    // IScannableTier-Override (V5-#49-E): YCSB-E Range-Scan über das getriebene Such-Organ.
    // Liest ab dem kleinsten gespeicherten Key >= start_key bis zu max_count Records IN KEY-REIHENFOLGE.
    // Implementierung über den MementoAxis-Snapshot (save_state() = vollständige (key,value)-Liste, in JEDEM
    // produktiven Such-Organ vorhanden) → sortieren → ab start_key bis max_count akkumulieren. Organe OHNE
    // MementoAxis sind nicht scanbar → 0 (ehrlich, kein Fake). const + noexcept (Mess-Robustheit: jede
    // interne Störung → 0). Der Scan verändert weder Daten noch Observer-Zähler (reines Lesen des Substrats).
    // ─────────────────────────────────────────────────────────────────────
    [[nodiscard]] std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                                          std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        try {
            if constexpr (MementoAxis<SearchAlgo>) {
                auto snapshot = search_organ_.save_state();   // vollständige (key,value)-Liste des Substrats
                std::sort(snapshot.begin(), snapshot.end(),
                          [](auto const& a, auto const& b) noexcept { return a.first < b.first; });
                for (auto const& kv : snapshot) {
                    if (static_cast<std::uint64_t>(kv.first) < start_key) continue;   // vor dem Scan-Fenster
                    if (visited >= max_count) break;                                   // Fenster gefüllt
                    sum += static_cast<std::uint64_t>(kv.second);
                    ++visited;
                }
            }
        } catch (...) {
            return 0;   // noexcept-Vertrag: interne Störung (z.B. OOM beim Snapshot) → 0 Records
        }
        if (out_checksum != nullptr) *out_checksum += sum;
        return visited;
    }
#endif  // COMDARE_MEASUREMENT_ON (tier_save_all / tier_rollback_all / memento_all / tier_scan)

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
    // V42 L-74c: telemetry-Organ für die Cross-ABI-Auto-Kopplung. Bei tier_insert/lookup wird record_node_touch
    // getrieben (if-constexpr-geschützt: AdHoc-Compositions tragen die nackte Strategie ohne record_node_touch).
    // OHNE {} (Aggregat-Strategie UND Huelle beide default-init-fähig, test_d_v42_probe2). mutable: das Tracking
    // im const tier_lookup ist logisch nicht-const (analog observer-notify-Muster, observable_composed_search.hpp).
    mutable typename Composition::telemetry telemetry_organ_;
    // V42 L-74c scan-Achsen-Auto-Kopplung: memory_layout + serialization Observer-Organe. In fill_observer_v2
    // ueber das ECHTE Slot-Backing des container_ getrieben (Pfad-B Zustand-Scan). mutable (const-Methode).
    mutable typename Composition::memory_layout ml_organ_;
    mutable typename Composition::serialization ser_organ_;
#if COMDARE_MEASUREMENT_ON
    // V5-#44 memento_all: Warmup-Vor-Zustand der getriebenen Organe. Lebt IN der Binary, quert die ABI NICHT.
    // PER-ACHSEN-Memento (bevorzugt, MementoAxis): memento_of_t<Organ> = Organ::memento_t (riche (key,value)-
    // Liste + Stats) bzw. EmptyMemento wenn das Organ KEIN MementoAxis ist (dann greift der Kopie-Fallback).
    memento_of_t<SearchAlgo>   saved_search_m_{};
    memento_of_t<container_t>  saved_container_m_{};
    // Kopie-Fallback NUR für Organe ohne MementoAxis (std::optional → leer bis save_all; reset bei Kapsel-OOM).
    std::optional<SearchAlgo>  saved_search_{};
    std::optional<container_t> saved_container_{};
#endif
};

}  // namespace comdare::cache_engine::anatomy
