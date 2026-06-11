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
#include "resource_controllable_tier.hpp"  // KF-4/L-MEAS: Laufzeit-Steuer-Sub-Interface (IMMER, nicht messungs-gated)
#include "observer_aggregate.hpp"    // ObservableAxis + ObserverAggregate (observable_count)
#include "memento_aggregate.hpp"     // V5-I6-SUBSTANZ (#44): MementoAxis + save_axis/restore_axis (per-Achsen-Memento)
#include "../execution_engine/execution_engine_base.hpp"
// R6 Inkrement 2b: die allocator-Achse im Cross-ABI-Observer-POD — der ComposedStore<N,L,A>-Vector-Growth
// treibt die Allocator-Statistik REAL (2. Mess-Achse, spiegelt builder/AnatomyExecutionContext). Da der
// host-seitige Loader jetzt das LEICHTE anatomy_module_abi_v1_decl.hpp nutzt (NICHT mehr abi_adapter.hpp),
// belasten diese topics/-Includes nur die Voll-Header-Konsumenten (DLLs/Tests, die die Pfade ohnehin haben).
#include "../topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp"
#include "../topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp"
#include "../axes/node/axis_04_node_type_chunked_store.hpp"   // Audit-30 Fix Q2: node-WIRKSamer Store (Delegation)
#include "../axes/node/axis_04_node_type_layout_aware_store.hpp"  // Plan v2 S1: layout-honorierender Store (CLA-Stride echt, OOB behoben)
// (X): ByteWiseKeyPrefix als kanonisches T3-Mess-Organ — IMMER deklariert (auch wenn die Composition
// PatriciaPathCompression/PathCompressionNone trägt; der `else`-Zweig der T3-Treibe-Op qualifiziert den Namen).
#include "../axes/path_compression/axis_02_path_compression_byte_wise.hpp"
// Phase B (2026-06-04): Per-Achsen-Observer-Hüllen für T7 prefetch + T8 concurrency. Diese Hüllen halten ein
// Mess-Organ (ObservablePrefetch<Strategy>/ObservableConcurrency<Strategy>) um die NACKTE Composition-Strategie
// (anders als telemetry/memory_layout/… trägt die Composition für prefetch/concurrency die rohe Strategie, s.
// art_reference.hpp). Der abi_adapter koppelt sie in tier_insert/lookup an die Tier-Op (Pfad-B Auto-Kopplung).
#include "../axes/prefetch_axis/axis_07_prefetch_observable.hpp"
#include "../axes/concurrency_axis/axis_08_concurrency_observable.hpp"
// Phase B (2026-06-04): die value_handle- (T11) + isa- (T12) Observer-HÜLLEN. Analog T7/T8: gehalten als
// EXPLIZITE Member-Organe über Composition::value_handle/isa (die — anders als memory_layout/telemetry — NICHT
// per Registry-make_observable_* vorab dekoriert sind), getrieben über das ECHTE container_-Slot-Backing
// (store_observe_value_handle/store_observe_isa → NodeChunkedStore::organ_observe_*, analog layout/serialization).
#include "../axes/value_handle_axis/axis_14_value_handle_observable.hpp"
#include "../axes/simd/axis_09_isa_observable.hpp"
// Phase B (2026-06-04) Abschluss: die 5 verbleibenden Observer-HÜLLEN. T3 path_compression (Instanz-Driver compress(),
// auto-gekoppelt in tier_insert/lookup wie T1/T2) + T13 index_org / T14 io_dispatch / T15 migration_policy / T16 filter
// (scan-Achsen, getrieben über das ECHTE container_-Slot-Backing in fill_observer_v3, store_observe_* → NodeChunkedStore::
// organ_observe_*, idempotenter reset()+scan, analog value_handle/isa/memory_layout/serialization). Anders als
// telemetry/q1/q2 trägt die Composition für T3/T13/T14/T15/T16 die NACKTE Strategie (kein statistics()) → die Hülle hält
// das Mess-Organ.
#include "../axes/path_compression/axis_02_path_compression_observable.hpp"
#include "../axes/index_organization/axis_01_index_organization_observable.hpp"
#include "../axes/io_dispatch/axis_io_dispatch_observable.hpp"
#include "../axes/migration_policy/axis_migration_observable.hpp"
#include "../axes/filter_axis/axis_filter_observable.hpp"

#include <algorithm>     // V5-#49-E: std::sort für den geordneten Range-Scan (tier_scan)
#include <array>
#include <chrono>
#include <cstring>       // (X) std::memcpy in den 19-Segment-Treibe-Ops
#if defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>   // (X) _mm_prefetch für das T7-Prefetch-Segment (MSVC/x86_64-Mess-Build)
#endif
#include <concepts>      // #133 Undo-Log: std::convertible_to im organ_undo_capable_v-Requires
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>   // V5-I6: is_copy_constructible/assignable-Guards für die In-Memory-Memento-Kopie
#include <utility>       // #133 Undo-Log: std::declval im OrganStatsSnap-Typ-Extrakt
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
                                        public IResourceControllableTier,   // KF-4/L-MEAS: IMMER (auch Messung-aus), eigenständiges Sub-Interface (dynamic_cast)
// V5-I2.2/I6: Antrieb ⊥ Observer ⊥ Memento compile-time disjunkt (kein Diamond — IObservableTier/IRollbackableTier IS-A nichts Gemeinsames; IDriveableTier kommt über IObservableTier).
//   MESSUNG-AN  : IObservableTier (Antrieb + tier_observe/observer_all) + IMeasurableWorkload (Pfad-A, host-relokal. I9) + IRollbackableTier (memento_all, V5-I6) + IScannableTier (Range-Scan, V5-#49-E).
//   MESSUNG-AUS : NUR IDriveableTier (funktionaler Antrieb) → Release-/funktional-only-DLL OHNE jeden Mess-Overhead (kein Observer-, kein Memento-, kein Scan-vtable-Slot).
#if COMDARE_MEASUREMENT_ON
                                        public IObservableTier,   // I1: GENAU EINE Observer-Schnittstelle (V2/V3/V4 konsolidiert, s. docs/architecture/31_*)
                                        public IMeasurableWorkload,
                                        public IMeasurableWorkloadV2,  // (C-2): eigenständiges V2-Sub-Interface — per-Segment-Timer (4 Achsen)
                                        public IMeasurableWorkloadV3,  // (X): eigenständiges V3-Sub-Interface — per-Segment-Timer ALLER 19 Achsen
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
    // KF-4 / L-MEAS — IResourceControllableTier (IMMER verfügbar, auch Messung-aus). Die SearchAlgorithm-Gattungs-
    // Komposition trägt genau 5 laufzeit-steuerbare Achsen (concurrency/prefetch/allocator/traversal/value_handle);
    // der Host-Loop (RuntimeVariableLoop) quert die Caps + wendet je dyn. Einstellung an (kein Reload). apply klammert
    // an die Caps + merkt die zuletzt angewandte Steuerung (applied_rc_). Eigenständiges Sub-Interface (dynamic_cast).
    // ─────────────────────────────────────────────────────────────────────
    void tier_query_resource_caps(ComdareResourceControlV1* out_caps) const noexcept override {
        if (out_caps == nullptr) return;
        *out_caps = ComdareResourceControlV1{};
        out_caps->thread_count            = 64;                          // concurrency (axis_08)
        out_caps->prefetch_distance       = 64;                          // prefetch (axis_07), in Cache-Lines
        out_caps->pool_budget_bytes       = (std::uint64_t{1} << 30);    // allocator (axis_06), 1 GiB Arena-Obergrenze
        out_caps->batch_size              = 4096;                        // traversal (axis_03a) Working-Set
        out_caps->inline_threshold_bytes  = 256;                         // value_handle (axis_14)
        out_caps->controllable_axis_count = 5;                           // genus-invariant: 5 steuerbare Achsen
    }
    [[nodiscard]] std::uint64_t tier_apply_resource_control(ComdareResourceControlV1 const* in) noexcept override {
        if (in == nullptr) return 0;
        ComdareResourceControlV1 caps{};
        tier_query_resource_caps(&caps);
        std::uint64_t applied = 0;
        auto apply1 = [&applied](std::uint64_t v, std::uint64_t cap, std::uint64_t& dst) noexcept {
            if (v != 0) { dst = (cap != 0 && v > cap) ? cap : v; ++applied; }  // 0 = Default beibehalten; sonst an cap klammern
        };
        apply1(in->thread_count,           caps.thread_count,           applied_rc_.thread_count);
        apply1(in->prefetch_distance,      caps.prefetch_distance,      applied_rc_.prefetch_distance);
        apply1(in->pool_budget_bytes,      caps.pool_budget_bytes,      applied_rc_.pool_budget_bytes);
        apply1(in->batch_size,             caps.batch_size,             applied_rc_.batch_size);
        apply1(in->inline_threshold_bytes, caps.inline_threshold_bytes, applied_rc_.inline_threshold_bytes);
        return applied;
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

    // (C-2): Akkumulator der 4 per-Segment-ns über die Batches (nur Mess-Hilfsstruktur, quert die ABI NICHT).
    struct SegmentAccumulator { std::int64_t s1 = 0, s2 = 0, s3 = 0, s4 = 0; };

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
            // Layout-Scan-Puffer (Segment 3): 16384 Datensaetze. LAYOUT-FIX (X-§4, 2026-06-04): kRecordSize=48
            // (NICHT 64) — sonst fiele cache_line_aligned (aligned_stride=round_up(48,64)=64) mit aos_strict
            // (Stride 48) zusammen und die Layout-Achse differenzierte nicht. Damit liest aos_strict bei Stride 48,
            // cache_line_aligned bei Stride 64. PFLICHT-OOB-SCHUTZ: der Puffer wird nach dem GRÖSSTMÖGLICHEN Stride
            // (64) dimensioniert (kLbufBytes = kRecords*64 >= jeder Layout-Stride), sonst läse der CLA-Scan bei
            // (kRecords-1)*64+4 über das Ende hinaus. EINMAL via Komposition-Allocator alloziert (Setup, NICHT gemessen).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes  = kRecords * 64;   // OOB-Schutz: größtmöglicher Layout-Stride (64), nicht kRecordSize
            unsigned char* lbuf = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);
            std::mt19937_64 rng{seed};
            // (C-2): do_batch misst JE SEGMENT einen eigenen steady_clock-Timer. `seg`(!=nullptr) akkumuliert
            // die 4 per-Segment-ns (für run_workload_segmented); der Batch-Gesamtwert (Σ der 4 Segmente) wird
            // immer zurückgegeben (für das bestehende run_workload, ABI unverändert). Identische Arbeit in
            // beiden Pfaden → der Segment-Split ist NICHT der Batch-Gesamtmessung gegenüber additiv verfälscht
            // (nur 4 statt 1 now()-Paar je Batch — vernachlässigbar gegenüber den Segment-Schleifen).
            auto do_batch = [&](SegmentAccumulator* seg) -> std::int64_t {
                std::uint64_t sink = 0;
                using clock = std::chrono::steady_clock;
                auto seg_ns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
                };
                // Segment 1: search_algo-Achse (Lookups auf der geladenen Such-Struktur)
                auto const s1a = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
                    if (v) sink += *v;
                }
                auto const s1b = clock::now();
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
                auto const s2b = clock::now();
                // Segment 3: memory_layout-Achse (Feld-Scan im layout-charakteristischen Zugriffsmuster)
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                auto const s3b = clock::now();
                // Segment 4: serialization-Achse (Encode-Scan im strategie-charakteristischen CPU-Aufwand:
                // raw=Byte-Sum < compressed=Delta+Zigzag < var_len=LEB128 < succinct=Bit-Packing). Doku 22 §4.
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
                auto const s4b = clock::now();
                std::int64_t const seg1 = seg_ns(s1a, s1b);
                std::int64_t const seg2 = seg_ns(s1b, s2b);
                std::int64_t const seg3 = seg_ns(s2b, s3b);
                std::int64_t const seg4 = seg_ns(s3b, s4b);
                if (seg != nullptr) { seg->s1 += seg1; seg->s2 += seg2; seg->s3 += seg3; seg->s4 += seg4; }
                std::int64_t const ns = seg1 + seg2 + seg3 + seg4;
                return (sink == ~0ull) ? (ns ^ 1) : ns;  // sink-Nutzung gegen Wegoptimierung
            };
            do_batch(nullptr);  // Warmup (verworfen)
            std::uint64_t const n = (batches < out_capacity) ? batches : out_capacity;
            for (std::uint64_t b = 0; b < n; ++b) out_latencies_ns[b] = do_batch(nullptr);
            alloc.deallocate(lbuf, kLbufBytes, 64);   // Layout-Puffer freigeben
            return n;
        } catch (...) {
            return 0;  // noexcept-Vertrag: interne Exception (z.B. OOM) → 0 Samples
        }
    }

    // (C-2): per-Segment-Timer-Variante. EXAKT derselbe 4-Segment-do_batch wie run_workload, aber je Segment
    // ein eigener steady_clock-Timer, über die batches AUFSUMMIERT → echte per-Achsen-Zeit für genau die 4
    // run_workload-getriebenen Achsen (search_algo/allocator/memory_layout/serialization). COMDARE_MEASUREMENT_ON-gegated.
    [[nodiscard]] std::uint64_t run_workload_segmented(std::uint64_t ops_per_batch,
                                                       std::uint64_t batches,
                                                       std::uint64_t seed,
                                                       ComdareSegmentLatencyV1* out) noexcept override {
        if (out == nullptr) return 0;
        *out = ComdareSegmentLatencyV1{};
        if (batches == 0 || ops_per_batch == 0) return 0;
        try {
            using SearchAlgo = typename A::composition_t::search_algo;
            using Allocator  = typename A::composition_t::allocator;
            using MemLayout  = typename A::composition_t::memory_layout;
            using Serializer = typename A::composition_t::serialization;
            using K          = typename SearchAlgo::key_type;
            SearchAlgo algo;
            for (int k = 0; k < 256; ++k) {
                algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u);
            }
            Allocator alloc;
            constexpr std::size_t kChurn = 2048;
            constexpr std::size_t kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const churn_size = [](std::size_t j) noexcept {
                return std::size_t{16} + ((j * 16u) & 0xF0u);
            };
            // LAYOUT-FIX (X-§4): kRecordSize=48 + kLbufBytes nach größtmöglichem Layout-Stride (64) dimensioniert
            // (OOB-Schutz), identisch zu run_workload. So differenziert die memory_layout-Achse aos_strict(48) vs
            // cache_line_aligned(64).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes  = kRecords * 64;   // OOB-Schutz: größtmöglicher Layout-Stride (64)
            unsigned char* lbuf = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);
            std::mt19937_64 rng{seed};
            using clock = std::chrono::steady_clock;
            auto seg_ns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            SegmentAccumulator acc{};
            std::uint64_t sink = 0;
            auto do_seg_batch = [&]() {
                auto const s1a = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
                    if (v) sink += *v;
                }
                auto const s1b = clock::now();
                for (std::size_t j = 0; j < kChurn; ++j) {
                    void* p = alloc.allocate(churn_size(j), kAlign);
                    *static_cast<unsigned char*>(p) = static_cast<unsigned char>(j);
                    sink += *static_cast<unsigned char*>(p);
                    blocks[j] = p;
                }
                for (std::size_t j = 0; j < kChurn; ++j) alloc.deallocate(blocks[j], churn_size(j), kAlign);
                auto const s2b = clock::now();
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                auto const s3b = clock::now();
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
                auto const s4b = clock::now();
                acc.s1 += seg_ns(s1a, s1b); acc.s2 += seg_ns(s1b, s2b);
                acc.s3 += seg_ns(s2b, s3b); acc.s4 += seg_ns(s3b, s4b);
            };
            do_seg_batch();        // Warmup (verworfen) — acc zurücksetzen
            acc = SegmentAccumulator{};
            for (std::uint64_t b = 0; b < batches; ++b) do_seg_batch();
            alloc.deallocate(lbuf, kLbufBytes, 64);
            out->seg_search_algo_ns   = (sink == ~0ull) ? (acc.s1 ^ 1) : acc.s1;  // sink-Nutzung gegen Wegoptimierung
            out->seg_allocator_ns     = acc.s2;
            out->seg_memory_layout_ns = acc.s3;
            out->seg_serialization_ns = acc.s4;
            out->total_ns             = acc.s1 + acc.s2 + acc.s3 + acc.s4;
            out->batches_measured     = batches;
            return batches;
        } catch (...) {
            *out = ComdareSegmentLatencyV1{};
            return 0;
        }
    }

    // (X): per-Segment-Timer-Variante auf ALLE 19 SearchAlgorithm-Achsen ausgeweitet. Je Achse ein eigener
    // steady_clock-Timer, über die batches AUFSUMMIERT → echte per-Achsen-ns für T0..T18 (kein n/a mehr).
    // Seg-Index = kCompositionAxisNames-Reihenfolge (axis_path_serialization.hpp): 0 search_algo … 18 queuing_q2.
    // Setup je Achse EINMAL vor der Batch-Schleife (Instanz + register/fill), gemessen wird nur die Op-Schleife.
    // sink-Akkumulation gegen Wegoptimierung. COMDARE_MEASUREMENT_ON-gegated (wie run_workload_segmented).
    //
    // EHRLICHKEIT [[no-success-marks-without-literal-output]]: jede Achse treibt eine REALE, strategie-abhängige
    // Op (die in §5 der Spec deklarierten synthetischen Mindest-Ops sind in den jeweiligen Wrapper-Doc-Kommentaren
    // als SIMULATION gekennzeichnet — keine konstanten/erfundenen Werte). None-artige Strategien (prefetch_none,
    // concurrency_none, migration_none) sind bewusste 0-Overhead-Baselines (so deklariert), KEINE n/a.
    [[nodiscard]] std::uint64_t run_workload_segmented_v2(std::uint64_t ops_per_batch,
                                                          std::uint64_t batches,
                                                          std::uint64_t seed,
                                                          ComdareSegmentLatencyV2* out) noexcept override {
        if (out == nullptr) return 0;
        *out = ComdareSegmentLatencyV2{};
        if (batches == 0 || ops_per_batch == 0) return 0;
        try {
            using SearchAlgo      = typename A::composition_t::search_algo;
            using CacheTraversal  = typename A::composition_t::cache_traversal;
            using Mapping         = typename A::composition_t::mapping;
            using PathCompression = typename A::composition_t::path_compression;
            using NodeType        = typename A::composition_t::node_type;
            using MemLayout       = typename A::composition_t::memory_layout;
            using Allocator       = typename A::composition_t::allocator;
            using Prefetch        = typename A::composition_t::prefetch;
            using Concurrency     = typename A::composition_t::concurrency;
            using Serializer      = typename A::composition_t::serialization;
            using Telemetry       = typename A::composition_t::telemetry;
            using ValueHandle     = typename A::composition_t::value_handle;
            using Isa             = typename A::composition_t::isa;
            using IndexOrg        = typename A::composition_t::index_organization;
            using IoDispatch      = typename A::composition_t::io_dispatch;
            using Migration       = typename A::composition_t::migration_policy;
            using Filter          = typename A::composition_t::filter;
            using QueuingQ1       = typename A::composition_t::queuing_q1;
            using QueuingQ2       = typename A::composition_t::queuing_q2;
            using K               = typename SearchAlgo::key_type;

            // ── Setup (EINMAL, NICHT gemessen) ───────────────────────────────────────────────────────────
            SearchAlgo algo;
            for (int k = 0; k < 256; ++k) algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u);

            CacheTraversal traversal;           // T1: register K Einträge → resolve N-fach
            using TV = typename CacheTraversal::value_type;
            constexpr std::size_t kTrav = 256;
            for (std::size_t k = 0; k < kTrav; ++k)
                traversal.register_entry(static_cast<typename CacheTraversal::key_type>(k),
                                         static_cast<TV>(k * 7u + 1u));

            Mapping mapping;                    // T2: register K Slots → resolve_offset N-fach
            constexpr std::size_t kSlots = 256;
            for (std::size_t s = 0; s < kSlots; ++s)
                mapping.register_slot(static_cast<typename Mapping::slot_index_type>(s),
                                      static_cast<typename Mapping::offset_type>(s * 64u));

            Allocator alloc;
            constexpr std::size_t kChurn = 2048;
            constexpr std::size_t kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const churn_size = [](std::size_t j) noexcept { return std::size_t{16} + ((j * 16u) & 0xF0u); };

            // LAYOUT-FIX (X-§4): kRecordSize=48; Lbuf nach größtmöglichem Stride (64) dimensioniert (OOB-Schutz).
            // path_descend_scan (T3 Patricia) liest 8 B/Record → record_size>=8 sicher (letzter Read (kRecords-1)*48+8).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes  = kRecords * 64;
            unsigned char* lbuf = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);

            // Query-Puffer für T16 filter_probe_scan (1 Byte je Query).
            constexpr std::size_t kQueries = 4096;
            std::array<unsigned char, kQueries> qbuf{};
            for (std::size_t i = 0; i < kQueries; ++i) qbuf[i] = static_cast<unsigned char>(i * 53u + 11u);

            Telemetry  telemetry_local;         // T10: eigenes Segment (entkoppelt vom Cross-ABI-Auto-Coupling)
            QueuingQ1  q1_local;                 // T17: put/get-Churn
            QueuingQ2  q2_local;                 // T18: should_flush/on_flush_complete
            using QElem = typename QueuingQ1::element_type;

            std::mt19937_64 rng{seed};
            using clock = std::chrono::steady_clock;
            auto seg_ns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            std::int64_t acc[19] = {};
            std::uint64_t sink = 0;

            // ── EIN Batch: die 19 Achsen-Segmente, jedes einzeln gezeitet (Index = Seg-Map T0..T18) ─────────
            auto do_seg19 = [&]() {
                clock::time_point t0, t1;
                // T0 search_algo: Lookups auf der geladenen Such-Struktur.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) { auto v = algo.lookup(static_cast<K>(rng() & 0xFFu)); if (v) sink += *v; }
                t1 = clock::now(); acc[0] += seg_ns(t0, t1);
                // T1 cache_traversal: resolve N-fach.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) { auto v = traversal.resolve(static_cast<typename CacheTraversal::key_type>(rng() % kTrav)); if (v) sink += static_cast<std::uint64_t>(*v); }
                t1 = clock::now(); acc[1] += seg_ns(t0, t1);
                // T2 mapping: resolve_offset N-fach.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) { auto o = mapping.resolve_offset(static_cast<typename Mapping::slot_index_type>(rng() % kSlots)); if (o) sink += static_cast<std::uint64_t>(*o); }
                t1 = clock::now(); acc[2] += seg_ns(t0, t1);
                // T3 path_compression: Patricia → path_descend_scan; sonst (ByteWise/None) → kanonisches
                // ByteWiseKeyPrefix::common_prefix_len-Organ über den Record-Puffer (echtes Mess-Organ der Achse).
                t0 = clock::now();
                if constexpr (requires { PathCompression::path_descend_scan(lbuf, kRecords, kRecordSize); }) {
                    sink += PathCompression::path_descend_scan(lbuf, kRecords, kRecordSize);
                } else {
                    for (std::size_t i = 0; i < kRecords; ++i) {
                        std::uint64_t key; std::memcpy(&key, lbuf + i * kRecordSize, sizeof(key));
                        auto const prefix = ::comdare::cache_engine::path_compression::ByteWiseKeyPrefix::from_bytes(key, 7u);
                        sink += prefix.common_prefix_len(key >> 8U);
                    }
                }
                t1 = clock::now(); acc[3] += seg_ns(t0, t1);
                // T4 node_type: node_find_scan (KF-6 Pflicht-API, order-sensitiver Probe-Scan).
                t0 = clock::now();
                sink += NodeType::node_find_scan(lbuf, kRecords, qbuf.data(), kQueries);
                t1 = clock::now(); acc[4] += seg_ns(t0, t1);
                // T5 memory_layout: scan_field_sum (layout-charakteristischer Stride; aos_strict 48 vs CLA 64).
                t0 = clock::now();
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[5] += seg_ns(t0, t1);
                // T6 allocator: alloc → touch → dealloc-Churn (Pool reused Free-Lists).
                t0 = clock::now();
                for (std::size_t j = 0; j < kChurn; ++j) { void* p = alloc.allocate(churn_size(j), kAlign); *static_cast<unsigned char*>(p) = static_cast<unsigned char>(j); sink += *static_cast<unsigned char*>(p); blocks[j] = p; }
                for (std::size_t j = 0; j < kChurn; ++j) alloc.deallocate(blocks[j], churn_size(j), kAlign);
                t1 = clock::now(); acc[6] += seg_ns(t0, t1);
                // T7 prefetch: aktiv (HW/PathOriented, is_active()==true) → SW-Prefetch-Hint-Loop über lbuf + Re-Scan;
                // None (is_active()==false) → reiner Re-Scan OHNE Hints (echte, kleinere Latenz, ehrliche Baseline).
                // is_active() ist eine static constexpr Eigenschaft → kein Prefetch-Instanz-Bedarf (CRTP-Base hat
                // protected ctor; die Strategie ist eine statische Prefetch-API, kein zu konstruierendes Organ).
                t0 = clock::now();
                if constexpr (Prefetch::is_active()) {
#if defined(_M_X64) || defined(__x86_64__)
                    for (std::size_t i = 0; i < kRecords; ++i) _mm_prefetch(reinterpret_cast<char const*>(lbuf + i * kRecordSize), _MM_HINT_T0);
#else
                    for (std::size_t i = 0; i < kRecords; ++i) { volatile unsigned char hint = lbuf[i * kRecordSize]; (void)hint; }
#endif
                }
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);   // Re-Scan (Prefetch-Hint-Wirkung)
                t1 = clock::now(); acc[7] += seg_ns(t0, t1);
                // T8 concurrency: acquire/release um eine Mini-Critical-Section N-fach (strategie-charakteristisches
                // Sync-Primitiv: None=no-op, Blocking=mutex, LockFree=CAS, …). if-constexpr-detektiert (optionale Op).
                t0 = clock::now();
                if constexpr (requires { Concurrency::acquire(); Concurrency::release(); }) {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i) { Concurrency::acquire(); sink += (i & 1u); Concurrency::release(); }
                } else {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i) sink += (i & 1u);   // ehrlicher No-Sync-Baseline-Fall
                }
                t1 = clock::now(); acc[8] += seg_ns(t0, t1);
                // T9 serialization: serialize_scan (strategie-charakteristischer Encode-CPU-Aufwand).
                t0 = clock::now();
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[9] += seg_ns(t0, t1);
                // T10 telemetry: record_node_touch N-fach (eigenes Segment, lokales Organ).
                t0 = clock::now();
                if constexpr (requires { telemetry_local.record_node_touch(true); }) {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i) { telemetry_local.record_node_touch((i & 1u) != 0u); }
                    sink += static_cast<std::uint64_t>(ops_per_batch);
                } else { sink += static_cast<std::uint64_t>(ops_per_batch); }
                t1 = clock::now(); acc[10] += seg_ns(t0, t1);
                // T11 value_handle: value_access_scan (strategie-charakteristische Deref-Last).
                t0 = clock::now();
                sink += ValueHandle::value_access_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[11] += seg_ns(t0, t1);
                // T12 isa: simd_field_sum (SSE2 bei Amd64 / Scalar-Fallback sonst). Nur (buf,n) — kein record_size.
                t0 = clock::now();
                sink += Isa::simd_field_sum(lbuf, kRecords);
                t1 = clock::now(); acc[12] += seg_ns(t0, t1);
                // T13 index_organization: index_org_scan (sequential/random/embedded/unordered je Strategie).
                t0 = clock::now();
                sink += IndexOrg::index_org_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[13] += seg_ns(t0, t1);
                // T14 io_dispatch: io_dispatch_scan (in-memory-Dispatch-Simulation je Strategie).
                t0 = clock::now();
                sink += IoDispatch::io_dispatch_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[14] += seg_ns(t0, t1);
                // T15 migration_policy: migration_decide_scan (Entscheidungslogik-Kosten ohne 2. Tier).
                t0 = clock::now();
                sink += Migration::migration_decide_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now(); acc[15] += seg_ns(t0, t1);
                // T16 filter: filter_probe_scan (Bloom/Cuckoo/SuRF/Xor-Probe; buf,n,queries,q).
                t0 = clock::now();
                sink += Filter::filter_probe_scan(lbuf, kRecords, qbuf.data(), kQueries);
                t1 = clock::now(); acc[16] += seg_ns(t0, t1);
                // T17 queuing_q1: put N + get N (Buffer-Strategy).
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) q1_local.put(static_cast<QElem>(i));
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) { auto v = q1_local.get(); if (v) sink += static_cast<std::uint64_t>(*v); }
                t1 = clock::now(); acc[17] += seg_ns(t0, t1);
                // T18 queuing_q2: should_flush N + on_flush_complete (Flush-Policy).
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto const d = q2_local.should_flush(static_cast<std::size_t>(i & 0x3FFu), std::size_t{1024});
                    sink += static_cast<std::uint64_t>(d);
                    q2_local.on_flush_complete();
                }
                t1 = clock::now(); acc[18] += seg_ns(t0, t1);
            };

            do_seg19();                                    // Warmup (verworfen)
            for (auto& a : acc) a = 0;
            for (std::uint64_t b = 0; b < batches; ++b) do_seg19();
            alloc.deallocate(lbuf, kLbufBytes, 64);

            std::int64_t total = 0;
            for (int i = 0; i < 19; ++i) { out->seg_ns[i] = acc[i]; total += acc[i]; }
            // sink-Nutzung gegen Wegoptimierung (verfälscht total NICHT — nur ein 1-bit-XOR im unmöglichen Fall).
            if (sink == ~0ull) out->seg_ns[0] ^= 1;
            out->total_ns         = total;
            out->batches_measured = batches;
            return batches;
        } catch (...) {
            *out = ComdareSegmentLatencyV2{};
            return 0;
        }
    }
#endif  // COMDARE_MEASUREMENT_ON (run_workload / Pfad A)

    // ─────────────────────────────────────────────────────────────────────
    // IDriveableTier-Override (V5-I2.2): funktionaler Gattungs-Antrieb — IMMER einkompiliert (auch Release-DLL).
    // Treibt das ECHTE sezierte Composition-Such-Organ (gemeinsamer uint64-Key nach Umstufung-B). GETRENNT
    // vom Pfad-A-`run_workload` (Messung-AN-only); bei MESSUNG-AN zieht tier_observe zusaetzlich observer_all (Doku 24 §8.6).
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
#if COMDARE_MEASUREMENT_ON
        // Undo-Log-Memento (#133): in der Warmup-Phase (save→rollback) die Inverse VOR der Mutation erfassen.
        // Der interne old-Lookup laeuft DIREKT am search_organ_ (nicht tier_lookup) → beruehrt NUR die T0-Stats,
        // die tier_rollback_all exakt restauriert; die auto-gekoppelten Mess-Organe sehen ihn NICHT.
        if (undo_armed_) record_undo_(key);
#endif
        // Neu-Flag ueber occupied_count-Delta (NICHT ueber einen internen lookup — der wuerde sonst die
        // lookup_count-Observer-Statistik verfaelschen; manche Organe insert()->void).
        auto const before = search_organ_.occupied_count();
        search_organ_.insert(key, value);
        container_.insert(key, value);       // treibt + MISST die allocator-Achse (ComposedStore-Vector-Growth)
        // V42 L-74c Cross-ABI-Auto-Kopplung: jeder insert berührt einen Blatt-Knoten → telemetry mit-treiben.
        if constexpr (requires { telemetry_organ_.record_node_touch(true); }) telemetry_organ_.record_node_touch(true);
        // Phase A (2026-06-04) Auto-Kopplung der 4 neu verdrahteten Achsen (Pfad B): ein insert registriert den
        // Eintrag in der cache_traversal-/mapping-Indirektion (T1/T2) und puffert ihn in q1 + befragt die
        // q2-Flush-Policy (T17/T18) → ihre statistics() reflektieren die echte Tier-Op beim Observer-Read (tier_observe).
        if constexpr (requires { ct_organ_.register_entry(typename Composition::cache_traversal::key_type{}, typename Composition::cache_traversal::value_type{}); }) {
            ct_organ_.register_entry(static_cast<typename Composition::cache_traversal::key_type>(key),
                                     static_cast<typename Composition::cache_traversal::value_type>(value));
        }
        if constexpr (requires { map_organ_.register_slot(typename Composition::mapping::slot_index_type{}, typename Composition::mapping::offset_type{}); }) {
            map_organ_.register_slot(static_cast<typename Composition::mapping::slot_index_type>(key),
                                     static_cast<typename Composition::mapping::offset_type>(value));
        }
        if constexpr (requires { queuing_q1_organ_.put(typename Composition::queuing_q1::element_type{}); }) {
            queuing_q1_organ_.put(static_cast<typename Composition::queuing_q1::element_type>(value));
        }
        if constexpr (requires { queuing_q2_organ_.should_flush(std::size_t{}, std::size_t{}); queuing_q2_organ_.on_flush_complete(); }) {
            (void)queuing_q2_organ_.should_flush(static_cast<std::size_t>(search_organ_.occupied_count()), std::size_t{1024});
            queuing_q2_organ_.on_flush_complete();
        }
        // Phase B (2026-06-04) Auto-Kopplung T7/T8 (Pfad B): ein insert berührt eine Adresse (key) → die
        // prefetch-Achse reiht sie in die Pfad-Trajektorie ein (PathOriented treibt echten Tracker; None/Hardware
        // = 0-Baseline) inkl. Hot-Path-Hint aus den rohen key-Bytes; die concurrency-Achse exerziert ein echtes
        // Sync-Primitiv-Paar (acquire→release; strategie-distinkt: None=no-op, Blocking=mutex, LockFree=CAS, …).
        pf_organ_.observe_prefetch(key, reinterpret_cast<std::byte const*>(&key), sizeof(key));
        cc_organ_.observe_critical_section();
        // Phase B Abschluss T3 (Pfad B): ein insert ordnet den Schlüssel in den Trie ein → die path_compression-Achse
        // misst die gemeinsame Byte-Prefix-Länge / cut() am ECHTEN ByteWiseKeyPrefix-Organ (PathCompressionNone =
        // ehrliche niedrige Kompressions-Aktivität, Patricia/ByteWise höher). depth=0 = Trie-Wurzel-Abstieg.
        (void)pc_organ_.compress(key, 0u);
        return search_organ_.occupied_count() > before;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        auto const v = search_organ_.lookup(key);
        if (v.has_value() && out_value != nullptr) *out_value = *v;
        // V42 L-74c: lookup berührt ebenfalls einen Knoten → telemetry mit-treiben (telemetry_organ_ mutable).
        if constexpr (requires { telemetry_organ_.record_node_touch(true); }) telemetry_organ_.record_node_touch(true);
        // Phase A Auto-Kopplung (Pfad B): ein lookup löst die cache_traversal-/mapping-Indirektion auf (T1/T2)
        // und entnimmt einen Eintrag aus q1 (T17) → resolve/get-Zähler steigen. ct_organ_/map_organ_ mutable.
        if constexpr (requires { ct_organ_.resolve(typename Composition::cache_traversal::key_type{}); }) {
            (void)ct_organ_.resolve(static_cast<typename Composition::cache_traversal::key_type>(key));
        }
        if constexpr (requires { map_organ_.resolve_offset(typename Composition::mapping::slot_index_type{}); }) {
            (void)map_organ_.resolve_offset(static_cast<typename Composition::mapping::slot_index_type>(key));
        }
        if constexpr (requires { queuing_q1_organ_.get(); }) {
            (void)queuing_q1_organ_.get();
        }
        // Phase B Auto-Kopplung T7/T8 (Pfad B): ein lookup folgt einem Such-Pfad → prefetch reiht die Adresse
        // (key) ein (PathOriented treibt den echten Tracker, suggest_next erzeugt die Next-Empfehlung); concurrency
        // exerziert das echte Sync-Primitiv-Paar. pf_organ_/cc_organ_ mutable (Tracking im const lookup nicht-const).
        pf_organ_.observe_prefetch(key, reinterpret_cast<std::byte const*>(&key), sizeof(key));
        cc_organ_.observe_critical_section();
        // Phase B Abschluss T3 (Pfad B): ein lookup folgt einem komprimierten Trie-Pfad → die path_compression-Achse
        // misst die gemeinsame Byte-Prefix-Länge des gesuchten Schlüssels am ECHTEN ByteWiseKeyPrefix-Organ. pc_organ_
        // mutable (Tracking im const lookup nicht-const). depth=0 = Trie-Wurzel.
        (void)pc_organ_.compress(key, 0u);
        return v.has_value();
    }

    [[nodiscard]] bool tier_erase(std::uint64_t key) noexcept override {
#if COMDARE_MEASUREMENT_ON
        if (undo_armed_) record_undo_(key);   // Undo-Log (#133): Inverse VOR der Mutation erfassen (s. tier_insert)
#endif
        auto const before = search_organ_.occupied_count();
        search_organ_.erase(key);            // Rueckgabe-Typ organ-abhaengig → ueber Delta bestimmen
        container_.erase(key);
        return search_organ_.occupied_count() < before;
    }

    void tier_clear() noexcept override {
#if COMDARE_MEASUREMENT_ON
        // Undo-Log (#133): clear ist NICHT O(1)-invertierbar (vernichtet alle n Eintraege) → fuer DIESE
        // save-Periode auf die Vollkopie eskalieren (Zustand wird VOR dem Leeren gekapselt). Greift nur in
        // der Warmup-Phase (undo_armed_); das tier_clear des Mess-Protokolls (zwischen Profilen) ist unberuehrt.
        if (undo_armed_) escalate_undo_to_copy_();
#endif
        search_organ_.clear(); container_.clear();
        // KONSOLIDIERUNG (2026-06-04): T0 search_algo-Statistik je Messung nullen. Der frühere perm_runner-
        // Kommentar „search_organ_ per ABI nicht resetbar" war VERALTET — ObservableComposedContainer/
        // ObservableComposedSearch tragen reset() (stats_={}). Damit ist axis_stats[0] pro (Binary×Setting×Rep)
        // frisch (kein kumulatives 2000→4000→…-Artefakt) → der konsolidierte tier_observe braucht KEIN post−pre-
        // Delta mehr (alle 19 Achsen warmup-frei aus EINEM Post-Observe). if-constexpr: AdHoc-Strategien ohne reset().
        if constexpr (requires { search_organ_.reset(); }) search_organ_.reset();
        // Phase A: die auto-gekoppelten Achsen-Organe mit-leeren (Daten UND kumulative Statistik), damit der
        // Mess-Pfad (perm_runner: tier_clear → pre-Observe → ops → post-Observe) einen sauberen Vor-Zustand hat
        // und q1 (std::deque) nicht über Messungen hinweg unbegrenzt wächst.
        //
        // KORREKTUR (2026-06-04, Defekt-Fix): clear() leert NUR die Daten (entries_/mappings_/Puffer); die
        // statistics()-Zähler dieser Organe (LinearFanout/DirectPlacement/NoBuffer/LazyFlush) leben in einem
        // SEPARATEN stats_ und werden ausschließlich von reset() genullt. Ohne reset() akkumulierte deren
        // V3-axis_stats über die 3 Wiederholungen je (Binary×Setting) (kumulatives Artefakt: 1000→2000→3000).
        // q2 (LazyFlush) besitzt gar kein clear() → wurde zuvor NIE zurückgesetzt. Daher hier zusätzlich reset()
        // für ALLE vier auto-gekoppelten Instanz-Achsen (T1/T2/T17/T18), if-constexpr-geschützt (AdHoc-Strategien
        // ohne reset()/clear() überspringen den jeweiligen Aufruf). Ziel: nach tier_clear() sind ALLE 19 Achsen-
        // statistics bei 0 → je Messung frisch, konsistent mit dem V1-Delta-Block.
        if constexpr (requires { ct_organ_.clear(); })          ct_organ_.clear();
        if constexpr (requires { ct_organ_.reset(); })          ct_organ_.reset();   // T1: stats_ nullen (clear()=nur Daten)
        if constexpr (requires { map_organ_.clear(); })         map_organ_.clear();
        if constexpr (requires { map_organ_.reset(); })         map_organ_.reset();  // T2: stats_ nullen
        if constexpr (requires { queuing_q1_organ_.clear(); })  queuing_q1_organ_.clear();
        if constexpr (requires { queuing_q1_organ_.reset(); })  queuing_q1_organ_.reset();  // T17: stats_ nullen
        if constexpr (requires { queuing_q2_organ_.reset(); })  queuing_q2_organ_.reset();  // T18: KEIN clear() vorhanden → nur reset()
        // T10 telemetry: ebenfalls über tier_insert/lookup auto-gekoppelt (record_node_touch) und in fill_observer_v3
        // DIREKT (kein Delta, kein idempotenter Scan) gelesen → akkumulierte ohne reset() identisch zu T1/T2/T17/T18.
        // ObservableTelemetry::reset() nullt stats_ (axes/telemetry_axis :76). Damit ist auch T10 je Messung frisch.
        if constexpr (requires { telemetry_organ_.reset(); })   telemetry_organ_.reset();
        // Phase B: T7/T8-Mess-Organe zurücksetzen (Statistik + prefetch-Tracker-Pfad), damit der Mess-Pfad
        // (perm_runner: tier_clear → pre-Observe → ops → post-Observe) einen sauberen Vor-Zustand hat.
        if constexpr (requires { pf_organ_.reset(); })          pf_organ_.reset();
        if constexpr (requires { cc_organ_.reset(); })          cc_organ_.reset();
        // Phase B Abschluss: T3-Mess-Organ zurücksetzen (die scan-Achsen T13/T14/T15/T16 sind idempotent — sie
        // reset()+scan je fill_observer_v3-Aufruf, brauchen hier kein Clear). Sauberer Vor-Zustand.
        if constexpr (requires { pc_organ_.reset(); })          pc_organ_.reset();
    }

    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(search_organ_.occupied_count());
    }

#if COMDARE_MEASUREMENT_ON   // V5-I2.2: tier_observe (observer_all) NUR bei Messung-AN
    // I1 (2026-06-05): der frühere V1-Observe + die V2-Observer-Fill/Override-Methoden (eigenes V2-Sub-Interface)
    // sind ENTFERNT. Ihre Felder sind im konsolidierten POD subsumiert (V1 search→axis_stats[0], alloc→[6]; V2
    // telemetry→[10], layout→[5], serialization→[9], node_type→[4]). Rationale: docs/architecture/31_observer_interface_konsolidierung_i1.md.

    // I1: fill_observer_v3 — schreibt die Per-Achsen-Observer DIREKT in den konsolidierten POD (out->axis_stats[T][f]
    // + Meta; Reihenfolge JE Achse = kV3AxisSchema, single-source observable_tier.hpp). Quellen je Achse: T0 search_algo
    // + T6 allocator + T10 telemetry = getriebene Organe; T4 node_type / T5 memory_layout / T9 serialization / T11..T16
    // = Pfad-B Zustand-Scan über das ECHTE container_-Slot-Backing (store_observe_*, idempotenter reset()+scan je Aufruf);
    // T1 cache_traversal / T2 mapping / T3 path_compression / T7 prefetch / T8 concurrency / T17 q1 / T18 q2 = in
    // tier_insert/lookup AUTO-gekoppelte Member-Organe. Honest-0 für Baseline-Strategien (echte 0-Teilfelder).
    // STATISTICS-gegated. Der Aufrufer (tier_observe) hat *out bereits genullt (Q1-Sequenz: vor dem Timing).
    void fill_observer_v3(ComdareTierObserverSnapshot* out) const noexcept {
        if (out == nullptr) return;
        auto& s = *out;                    // I1: direkt in den konsolidierten POD schreiben (kein V3-Zwischen-POD)
        for (auto& row : s.axis_stats) for (auto& v : row) v = 0;   // nur Observer-Felder nullen (seg_ns/batches: tier_observe)
        s.observable_axis_count = 0; s.tier_fill_level = 0; s.filled_axis_count = 0;
        std::uint64_t filled = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // ── T0 search_algo ──────────────────────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<SearchAlgo>) {
            auto const ss = search_organ_.statistics();
            auto* r = s.axis_stats[0];
            r[0] = ss.total_lookup_count; r[1] = ss.total_hit_count;  r[2] = ss.total_miss_count;
            r[3] = ss.total_insert_count; r[4] = ss.total_erase_count; r[5] = ss.peak_occupancy;
            ++filled;
        }
        // ── T1 cache_traversal (Phase A neu) ────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) {
            auto const ct = ct_organ_.statistics();
            auto* r = s.axis_stats[1];
            r[0] = ct.total_resolve_count;      r[1] = ct.total_resolve_hit_count; r[2] = ct.total_resolve_miss_count;
            r[3] = ct.total_register_count;     r[4] = ct.total_unregister_count;  r[5] = ct.peak_tracked;
            ++filled;
        }
        // ── T2 mapping (Phase A neu) ────────────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::mapping>) {
            auto const mp = map_organ_.statistics();
            auto* r = s.axis_stats[2];
            r[0] = mp.total_register_count;     r[1] = mp.total_resolve_count;     r[2] = mp.total_resolve_hit_count;
            r[3] = mp.total_resolve_miss_count; r[4] = mp.total_reverse_lookup_count; r[5] = mp.peak_mapped;
            ++filled;
        }
        // ── T4 node_type (Pfad-B Zustand-Scan über container_, wie der frühere V2-Pfad) ─────────────────────
        if constexpr (ObservableAxis<typename Composition::node_type>
                   && requires { container_.store_observe_node_type(node_organ_); }) {
            node_organ_.reset();
            (void)container_.store_observe_node_type(node_organ_);
            auto const nt = node_organ_.statistics();
            auto* r = s.axis_stats[4];
            r[0] = nt.find_count; r[1] = nt.keys_stored; r[2] = nt.queries_run; r[3] = nt.last_checksum;
            ++filled;
        }
        // ── T5 memory_layout (Pfad-B Zustand-Scan über container_) ───────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::memory_layout>
                   && requires { container_.store_observe_layout(ml_organ_); }) {
            ml_organ_.reset();
            (void)container_.store_observe_layout(ml_organ_);
            auto const ml = ml_organ_.statistics();
            auto* r = s.axis_stats[5];
            r[0] = ml.scan_count; r[1] = ml.records_scanned; r[2] = ml.field_bytes_read;
            r[3] = ml.cache_lines_touched; r[4] = ml.last_checksum;
            ++filled;
        }
        // ── T6 allocator (dasselbe getriebene container_-Allocator-Organ wie der frühere V2-Pfad) ───────────
        if constexpr (ObservableAxis<typename Composition::allocator>
                   && container_t::template store_has_allocator_stats<typename container_t::store_type>) {
            auto const a = container_.store_allocator_statistics();
            auto* r = s.axis_stats[6];
            r[0] = a.total_bytes_allocated; r[1] = a.total_bytes_in_use; r[2] = a.allocation_count;
            r[3] = a.deallocation_count;    r[4] = a.failure_count;
            ++filled;
        }
        // ── T7 prefetch (Phase B neu, AUTO-gekoppelt via tier_insert/lookup observe_prefetch) ─────────────
        //    ObservablePrefetch-Hülle (pf_organ_) IMMER ObservableAxis → das Schema-Slot ist befüllt; bei
        //    None/Hardware-Strategie bleiben die Tracker-Zähler 0 (ehrliche Baseline), bei PathOriented >0.
        if constexpr (ObservableAxis<decltype(pf_organ_)>) {
            auto const pf = pf_organ_.statistics();
            auto* r = s.axis_stats[7];
            r[0] = pf.trigger_count;   r[1] = pf.suggestions_made;         r[2] = pf.hot_path_hints;
            r[3] = pf.max_queue_depth; r[4] = pf.total_addresses_enqueued;
            ++filled;
        }
        // ── T8 concurrency (Phase B neu, AUTO-gekoppelt via tier_insert/lookup observe_critical_section) ──
        //    ObservableConcurrency-Hülle (cc_organ_) treibt das echte Sync-Primitiv (acquire/release) + zählt;
        //    contention/validation_failure im single-thread-Pfad ehrlich 0 (s. axis_08_concurrency_observable.hpp).
        if constexpr (ObservableAxis<decltype(cc_organ_)>) {
            auto const cc = cc_organ_.statistics();
            auto* r = s.axis_stats[8];
            r[0] = cc.acquire_count;            r[1] = cc.release_count; r[2] = cc.contention_count;
            r[3] = cc.validation_failure_count; r[4] = cc.pattern_id;
            ++filled;
        }
        // ── T9 serialization (Pfad-B Zustand-Scan über container_) ───────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::serialization>
                   && requires { container_.store_observe_serialization(ser_organ_); }) {
            ser_organ_.reset();
            (void)container_.store_observe_serialization(ser_organ_);
            auto const sr = ser_organ_.statistics();
            auto* r = s.axis_stats[9];
            r[0] = sr.serialize_count; r[1] = sr.records_serialized; r[2] = sr.bytes_serialized; r[3] = sr.last_checksum;
            ++filled;
        }
        // ── T10 telemetry (AUTO-gekoppelt via tier_insert/lookup, wie der frühere V2-Pfad) ──────────────────
        if constexpr (ObservableAxis<typename Composition::telemetry>) {
            auto const t = telemetry_organ_.statistics();
            auto* r = s.axis_stats[10];
            r[0] = t.total_events; r[1] = t.leaf_updates; r[2] = t.node_updates; r[3] = t.peak_tracked;
            ++filled;
        }
        // ── T11 value_handle (Phase B neu, Pfad-B Zustand-Scan über container_-Slot-Backing) ──────────────
        //    ECHTER Weg (Spec §3): vh_organ_ (ObservableValueHandle-Hülle) treibt value_access_scan über die
        //    REAL gespeicherten Slots (store_observe_value_handle → NodeChunkedStore::organ_observe_value_handle),
        //    KEINE flache Roh-Puffer-Simulation. idempotenter reset()+scan je Observe (wie ser/ml).
        if constexpr (requires { container_.store_observe_value_handle(vh_organ_); }) {
            vh_organ_.reset();
            (void)container_.store_observe_value_handle(vh_organ_);
            auto const vh = vh_organ_.statistics();
            auto* r = s.axis_stats[11];
            r[0] = vh.total_access_count; r[1] = vh.indirect_deref_count;
            r[2] = vh.version_tag_strips; r[3] = vh.peak_chain_depth;
            ++filled;
        }
        // ── T12 isa (Phase B neu, Pfad-B Zustand-Scan über container_-Slot-Backing) ───────────────────────
        //    isa_organ_ (ObservableIsa-Hülle) treibt simd_field_sum über die REAL gespeicherten Slot-Bytes als
        //    32-bit-Wort-Strom (store_observe_isa → NodeChunkedStore::organ_observe_isa). idempotenter reset()+scan.
        if constexpr (requires { container_.store_observe_isa(isa_organ_); }) {
            isa_organ_.reset();
            (void)container_.store_observe_isa(isa_organ_);
            auto const is = isa_organ_.statistics();
            auto* r = s.axis_stats[12];
            r[0] = is.simd_calls;            r[1] = is.elements_processed; r[2] = is.simd_iterations;
            r[3] = is.scalar_fallback_count; r[4] = is.last_checksum;
            ++filled;
        }
        // ── T3 path_compression (Phase B Abschluss, AUTO-gekoppelt via tier_insert/lookup compress) ────────
        //    pc_organ_ (ObservablePathCompression-Hülle) treibt das ECHTE ByteWiseKeyPrefix-Organ je Tier-Op; bei
        //    PathCompressionNone ehrlich niedrige Kompressions-Aktivität (kein Sonderfall), bei Patricia/ByteWise höher.
        if constexpr (requires { pc_organ_.statistics(); }) {
            auto const pc = pc_organ_.statistics();
            auto* r = s.axis_stats[3];
            r[0] = pc.compress_calls; r[1] = pc.prefix_len_total;  r[2] = pc.bytes_saved_total;
            r[3] = pc.cuts_performed; r[4] = pc.last_checksum;
            ++filled;
        }
        // ── T13 index_organization (Phase B Abschluss, Pfad-B Zustand-Scan über container_-Slot-Backing) ────
        //    idx_organ_ (ObservableIndexOrg-Hülle) treibt index_org_scan über die REAL gespeicherten Slots
        //    (store_observe_index_org → NodeChunkedStore::organ_observe_index_org). predicate_evals/indirect_lookups
        //    folgen den static-deklarierten Strategie-Eigenschaften (is_clustered/has_secondary_indexes). reset()+scan.
        if constexpr (requires { container_.store_observe_index_org(idx_organ_); }) {
            idx_organ_.reset();
            (void)container_.store_observe_index_org(idx_organ_);
            auto const ix = idx_organ_.statistics();
            auto* r = s.axis_stats[13];
            r[0] = ix.scan_count;       r[1] = ix.records_scanned; r[2] = ix.predicate_evals;
            r[3] = ix.indirect_lookups; r[4] = ix.last_checksum;
            ++filled;
        }
        // ── T14 io_dispatch (Phase B Abschluss, Pfad-B Zustand-Scan über container_-Slot-Backing) ───────────
        //    io_organ_ (ObservableIoDispatch-Hülle) treibt io_dispatch_scan über die REAL gespeicherten Slots als
        //    IN-MEMORY-Dispatch (store_observe_io_dispatch → NodeChunkedStore::organ_observe_io_dispatch); KEIN Disk-IO
        //    (Hauptagent-Entscheid). alignment_adjusts honest 0 für InMemoryOnly. reset()+scan.
        if constexpr (requires { container_.store_observe_io_dispatch(io_organ_); }) {
            io_organ_.reset();
            (void)container_.store_observe_io_dispatch(io_organ_);
            auto const io = io_organ_.statistics();
            auto* r = s.axis_stats[14];
            r[0] = io.dispatch_rounds;     r[1] = io.bytes_dispatched; r[2] = io.alignment_adjusts;
            r[3] = io.total_dispatch_count; r[4] = io.last_checksum;
            ++filled;
        }
        // ── T15 migration_policy (Phase B Abschluss, Pfad-B Zustand-Scan über container_-Slot-Backing) ───────
        //    mig_organ_ (ObservableMigration-Hülle) treibt migration_decide_scan über die REAL gespeicherten Slots
        //    (store_observe_migration → NodeChunkedStore::organ_observe_migration). decide-only, KEIN 2. Tier →
        //    tier_moves honest 0; migrations/hot/cold honest 0 für NoMigration (is_active()==false). reset()+scan.
        if constexpr (requires { container_.store_observe_migration(mig_organ_); }) {
            mig_organ_.reset();
            (void)container_.store_observe_migration(mig_organ_);
            auto const mg = mig_organ_.statistics();
            auto* r = s.axis_stats[15];
            r[0] = mg.total_decisions; r[1] = mg.migrations_triggered; r[2] = mg.hot_votes;
            r[3] = mg.cold_votes;      r[4] = mg.tier_moves;
            ++filled;
        }
        // ── T16 filter (Phase B Abschluss, Pfad-B Zustand-Scan über container_-Slot-Backing) ─────────────────
        //    flt_organ_ (ObservableFilter-Hülle) treibt filter_probe_scan über die low-Bytes der REAL gespeicherten
        //    Keys als Query-Strom (store_observe_filter → NodeChunkedStore::organ_observe_filter). REALER In-Memory-
        //    Filter, keine Strategie-Internas (positive/negative je öffentliches Einzel-Query-Ergebnis). reset()+scan.
        if constexpr (requires { container_.store_observe_filter(flt_organ_); }) {
            flt_organ_.reset();
            (void)container_.store_observe_filter(flt_organ_);
            auto const fl = flt_organ_.statistics();
            auto* r = s.axis_stats[16];
            r[0] = fl.probe_count;       r[1] = fl.queries_positive; r[2] = fl.queries_negative;
            r[3] = fl.hash_probes_total; r[4] = fl.last_checksum;
            ++filled;
        }
        // ── T17 queuing_q1 (Phase A neu, AUTO-gekoppelt via tier_insert/lookup put/get) ──────────────────
        if constexpr (ObservableAxis<typename Composition::queuing_q1>) {
            auto const q = queuing_q1_organ_.statistics();
            auto* r = s.axis_stats[17];
            r[0] = q.total_put_count; r[1] = q.total_get_count; r[2] = q.overflow_count;
            r[3] = q.underflow_count; r[4] = q.peak_size;
            ++filled;
        }
        // ── T18 queuing_q2 (Phase A neu, AUTO-gekoppelt via tier_insert should_flush/on_flush_complete) ──
        if constexpr (ObservableAxis<typename Composition::queuing_q2>) {
            auto const q = queuing_q2_organ_.statistics();
            auto* r = s.axis_stats[18];
            r[0] = q.total_decisions_evaluated; r[1] = q.full_flush_count; r[2] = q.partial_flush_count;
            r[3] = q.no_flush_count;            r[4] = q.flush_complete_count;
            ++filled;
        }
#endif  // COMDARE_CE_ENABLE_STATISTICS
        s.observable_axis_count = ObserverAggregate<Composition>::observable_count();
        s.tier_fill_level       = tier_size();
        s.filled_axis_count     = filled;
    }

    // I1: die früheren V2/V3/V4-Observer-Override-Methoden (eigene Sub-Interfaces) sind ENTFERNT — die EINE
    // tier_observe(ComdareTierObserverSnapshot*) unten vereint Observer-Stats + Pfad-B-Timing (s. Doc 31).

    // Pfad-B Per-Achsen-TIMING-Kern (Plan v2): zeitet die 19 REALEN per-Achsen-Ops über die EINE schon
    // befüllte composite-Tier-Struktur (search_organ_ + container_.chunks_ + Instanz-Organe) — KEIN
    // synthetischer Puffer (Pfad A). Alle Ops lesend/idempotent (lookup/resolve/compress const; store_observe_*
    // reset()+const-scan über chunks_); die per-op-getriebenen Zähler werden NACH dem Timing zurückgesetzt
    // (der Host zieht V3-Observer ohnehin VOR V4-Timing → keine Verfälschung). Memento-neutral: keine
    // Daten-Mutation an search_organ_/container_ (deren save_all/rollback_all-Substrat bleibt unberührt).
    void fill_segment_timing_v3(ComdareSegmentLatencyV2* out) const noexcept {
        if (out == nullptr) return;
        *out = ComdareSegmentLatencyV2{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        try {
            using clock = std::chrono::steady_clock;
            auto dns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            // Reale gespeicherte Keys EINMAL beziehen (NICHT gemessen) — für die per-op-Achsen (T0/T1/T2/T3).
            std::vector<std::uint64_t> keys;
            if constexpr (MementoAxis<SearchAlgo>) {
                auto snap = search_organ_.save_state();   // (key,value)-Liste der real gespeicherten Records
                keys.reserve(snap.size());
                for (auto const& kv : snap) keys.push_back(static_cast<std::uint64_t>(kv.first));
            }
            std::size_t const nk = keys.empty() ? std::size_t{1} : keys.size();
            if (keys.empty()) keys.push_back(0);
            std::uint64_t const n_ops = static_cast<std::uint64_t>(nk);
            constexpr std::uint64_t kBatches = 8;
            std::int64_t acc[19] = {};
            std::uint64_t sink = 0;
            using K = typename SearchAlgo::key_type;

            auto do_batch = [&]() {
                clock::time_point t0, t1;
                // T0 search_algo: echte Lookups auf dem real befüllten Such-Organ (Baum-Traversierung).
                t0 = clock::now();
                for (std::uint64_t i = 0; i < n_ops; ++i) { auto v = search_organ_.lookup(static_cast<K>(keys[i % nk])); if (v) sink += static_cast<std::uint64_t>(*v); }
                t1 = clock::now(); acc[0] += dns(t0, t1);
                // T1 cache_traversal: resolve über die real registrierten Einträge.
                t0 = clock::now();
                if constexpr (requires { ct_organ_.resolve(typename Composition::cache_traversal::key_type{}); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) { auto v = ct_organ_.resolve(static_cast<typename Composition::cache_traversal::key_type>(keys[i % nk])); if (v) sink += static_cast<std::uint64_t>(*v); }
                }
                t1 = clock::now(); acc[1] += dns(t0, t1);
                // T2 mapping: resolve_offset über die real registrierten Slots.
                t0 = clock::now();
                if constexpr (requires { map_organ_.resolve_offset(typename Composition::mapping::slot_index_type{}); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) { auto o = map_organ_.resolve_offset(static_cast<typename Composition::mapping::slot_index_type>(keys[i % nk])); if (o) sink += static_cast<std::uint64_t>(*o); }
                }
                t1 = clock::now(); acc[2] += dns(t0, t1);
                // T3 path_compression: echte compress(key,0) über die real gespeicherten Keys.
                t0 = clock::now();
                if constexpr (requires { pc_organ_.compress(std::uint64_t{}, 0u); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) sink += static_cast<std::uint64_t>(pc_organ_.compress(keys[i % nk], 0u));
                }
                t1 = clock::now(); acc[3] += dns(t0, t1);
                // T4 node_type: store_observe über das ECHTE container_-Slot-Backing (chunks_).
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::node_type> && requires { container_.store_observe_node_type(node_organ_); }) { if constexpr (requires { node_organ_.reset(); }) node_organ_.reset(); sink += container_.store_observe_node_type(node_organ_); }
                t1 = clock::now(); acc[4] += dns(t0, t1);
                // T5 memory_layout: store_observe über chunks_ (layout-honorierender Store → CLA-Stride echt).
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::memory_layout> && requires { container_.store_observe_layout(ml_organ_); }) { if constexpr (requires { ml_organ_.reset(); }) ml_organ_.reset(); sink += container_.store_observe_layout(ml_organ_); }
                t1 = clock::now(); acc[5] += dns(t0, t1);
                // T6 allocator: O(1)-Stats-Read (Aufbau-Effekt, ehrliche kleine Baseline — kein erfundener Scan).
                t0 = clock::now();
                if constexpr (container_t::template store_has_allocator_stats<typename container_t::store_type>) { auto a = container_.store_allocator_statistics(); sink += a.total_bytes_in_use; }
                t1 = clock::now(); acc[6] += dns(t0, t1);
                // T7 prefetch: echte observe_prefetch über die real gespeicherten Keys.
                t0 = clock::now();
                if constexpr (requires { pf_organ_.observe_prefetch(std::uint64_t{}, (std::byte const*)nullptr, std::size_t{}); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) { K k = static_cast<K>(keys[i % nk]); pf_organ_.observe_prefetch(static_cast<std::uint64_t>(k), reinterpret_cast<std::byte const*>(&k), sizeof(k)); }
                }
                t1 = clock::now(); acc[7] += dns(t0, t1);
                // T8 concurrency: echtes Sync-Primitiv-Paar (observe_critical_section).
                t0 = clock::now();
                if constexpr (requires { cc_organ_.observe_critical_section(); }) { for (std::uint64_t i = 0; i < n_ops; ++i) { cc_organ_.observe_critical_section(); sink += (i & 1u); } }
                t1 = clock::now(); acc[8] += dns(t0, t1);
                // T9 serialization: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::serialization> && requires { container_.store_observe_serialization(ser_organ_); }) { if constexpr (requires { ser_organ_.reset(); }) ser_organ_.reset(); sink += container_.store_observe_serialization(ser_organ_); }
                t1 = clock::now(); acc[9] += dns(t0, t1);
                // T10 telemetry: echtes record_node_touch.
                t0 = clock::now();
                if constexpr (requires { telemetry_organ_.record_node_touch(true); }) { for (std::uint64_t i = 0; i < n_ops; ++i) telemetry_organ_.record_node_touch((i & 1u) != 0u); sink += n_ops; }
                t1 = clock::now(); acc[10] += dns(t0, t1);
                // T11 value_handle: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_value_handle(vh_organ_); }) { vh_organ_.reset(); sink += container_.store_observe_value_handle(vh_organ_); }
                t1 = clock::now(); acc[11] += dns(t0, t1);
                // T12 isa: store_observe (SIMD-Feld-Reduktion) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_isa(isa_organ_); }) { isa_organ_.reset(); sink += container_.store_observe_isa(isa_organ_); }
                t1 = clock::now(); acc[12] += dns(t0, t1);
                // T13 index_organization: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_index_org(idx_organ_); }) { idx_organ_.reset(); sink += container_.store_observe_index_org(idx_organ_); }
                t1 = clock::now(); acc[13] += dns(t0, t1);
                // T14 io_dispatch: store_observe (In-Memory-Dispatch) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_io_dispatch(io_organ_); }) { io_organ_.reset(); sink += container_.store_observe_io_dispatch(io_organ_); }
                t1 = clock::now(); acc[14] += dns(t0, t1);
                // T15 migration_policy: store_observe (decide-only) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_migration(mig_organ_); }) { mig_organ_.reset(); sink += container_.store_observe_migration(mig_organ_); }
                t1 = clock::now(); acc[15] += dns(t0, t1);
                // T16 filter: store_observe (Probe über Key-Low-Bytes) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_.store_observe_filter(flt_organ_); }) { flt_organ_.reset(); sink += container_.store_observe_filter(flt_organ_); }
                t1 = clock::now(); acc[16] += dns(t0, t1);
                // T17 queuing_q1: put+get-Paar (bounded → kein Deque-Wachstum über die Batches).
                t0 = clock::now();
                if constexpr (requires { queuing_q1_organ_.put(typename Composition::queuing_q1::element_type{}); queuing_q1_organ_.get(); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) { queuing_q1_organ_.put(static_cast<typename Composition::queuing_q1::element_type>(i)); auto v = queuing_q1_organ_.get(); if (v) sink += static_cast<std::uint64_t>(*v); }
                }
                t1 = clock::now(); acc[17] += dns(t0, t1);
                // T18 queuing_q2: should_flush + on_flush_complete.
                t0 = clock::now();
                if constexpr (requires { queuing_q2_organ_.should_flush(std::size_t{}, std::size_t{}); queuing_q2_organ_.on_flush_complete(); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) { sink += static_cast<std::uint64_t>(queuing_q2_organ_.should_flush(static_cast<std::size_t>(i & 0x3FFu), std::size_t{1024})); queuing_q2_organ_.on_flush_complete(); }
                }
                t1 = clock::now(); acc[18] += dns(t0, t1);
            };

            do_batch();                              // Warmup (verworfen)
            for (auto& a : acc) a = 0;
            for (std::uint64_t b = 0; b < kBatches; ++b) do_batch();

            std::int64_t total = 0;
            for (int i = 0; i < 19; ++i) { out->seg_ns[i] = acc[i]; total += acc[i]; }
            if (sink == ~0ull) out->seg_ns[0] ^= 1;   // sink-Schutz gegen Wegoptimierung (verfälscht total NICHT)
            out->total_ns         = total;
            out->batches_measured = kBatches;

            // Per-op-getriebene Zähler nach dem Timing zurücksetzen (memento-/observer-neutral; der Host zieht
            // V3 VOR V4, daher sind die Observer schon erhoben — aber ein defensiver Reset hält die Member sauber).
            if constexpr (requires { ct_organ_.reset(); })          ct_organ_.reset();
            if constexpr (requires { map_organ_.reset(); })         map_organ_.reset();
            if constexpr (requires { pc_organ_.reset(); })          pc_organ_.reset();
            if constexpr (requires { pf_organ_.reset(); })          pf_organ_.reset();
            if constexpr (requires { cc_organ_.reset(); })          cc_organ_.reset();
            if constexpr (requires { telemetry_organ_.reset(); })   telemetry_organ_.reset();
            if constexpr (requires { queuing_q1_organ_.clear(); })  queuing_q1_organ_.clear();
            if constexpr (requires { queuing_q1_organ_.reset(); })  queuing_q1_organ_.reset();
            if constexpr (requires { queuing_q2_organ_.reset(); })  queuing_q2_organ_.reset();
        } catch (...) {
            *out = ComdareSegmentLatencyV2{};
        }
#endif  // COMDARE_CE_ENABLE_STATISTICS
    }

    // KONSOLIDIERUNG (I1, 2026-06-05): die EINZIGE Observer-Methode über den konsolidierten POD. Vereint Observer-
    // Stats (axis_stats[19][8]+Meta) UND Pfad-B-Timing (seg_ns[19]) in EINEN Snapshot. FIXE Q1-SEQUENZ (Preflight
    // wkqt7a0il): (1) axis_stats VOR dem Timing lesen (fill_observer_v3 schreibt sie DIREKT in *out), (2) DANN seg_ns
    // timen (fill_segment_timing_v3 treibt die per-op-Organe + resettet sie an seinem Ende = SCHRITT 3) → die in (1)
    // gelesenen Stats bleiben unverfälscht. Rationale-Historie: docs/architecture/31_observer_interface_konsolidierung_i1.md.
    void tier_observe(ComdareTierObserverSnapshot* out) const noexcept override {
        if (out == nullptr) return;
        *out = ComdareTierObserverSnapshot{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // SCHRITT 1 — Observer-READ (fill_observer_v3 schreibt axis_stats + Meta direkt in *out, vor dem Timing).
        fill_observer_v3(out);
        // SCHRITT 2 — Pfad-B-Timing (nach dem Observer-READ; resettet die per-op-Organe an seinem Ende = SCHRITT 3).
        ComdareSegmentLatencyV2 seg{};
        fill_segment_timing_v3(&seg);
        for (std::size_t t = 0; t < kV3AxisCount; ++t) out->seg_ns[t] = seg.seg_ns[t];
        out->batches_measured = seg.batches_measured;
#endif  // COMDARE_CE_ENABLE_STATISTICS
    }
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
    // KOSTEN-FIX (2026-06-08, User „Kopie jetzt"): Der COPY-Memento (organ-Vollkopie, O(n)) wird BEVORZUGT vor dem
    // MementoAxis-Pfad (save_state()-Liste + restore_state()-RE-INSERT = O(n²) je Op bei O(n)-Insert-Organen wie
    // k_ary/sorted/linear → O(n_ops·n²)/Messung → Voll-Lauf 11× über ZIH-Kontingent).
    //
    // UNDO-LOG-MEMENTO (#133, 2026-06-11, User „das geplante Refactoring"): O(1)/Op statt Copy-Memento O(n)/Op.
    // save_all kapselt NICHT mehr die Daten, sondern (a) die Stat-PODs von search_organ_ + container_ (O(1)-
    // Snapshot; die T6-Allocator-Statistik des Stores ist aus dem Datenzustand DERIVIERT → kein eigener Snapshot
    // nötig) und (b) armt das Undo-Log: tier_insert/tier_erase zeichnen VOR der Mutation die Einzel-Key-Inverse
    // auf ({key, old_value} via direktem Organ-Lookup). rollback_all spielt das Log RÜCKWÄRTS über die Organ-API
    // ab (Delegationskette node/layout/allocator bleibt gewahrt, Doc 30) und restauriert dann die Stat-PODs →
    // Daten UND Observer-Zähler exakt auf dem save-Stand (zwei_phase-Vertrag tier_observe_trace_abi.hpp:84:
    // „End-Zustand + Observer-Zähler identisch zur Einphasen-Messung"). Read-only-Ops (lookup/scan) mutieren
    // weder search_organ_ noch container_ → Log bleibt leer, save+rollback sind reine POD-Kopien (der große
    // Hebel: die 21 Lastprofile sind großteils read-heavy). VERFAHRENS-NUANCE (dokumentiert, Doc 24-Anhang):
    // Nach dem inversen Replay ist der Zustand LOGISCH identisch (gleiche key→value-Map, gleiche Zähler);
    // physische Substrat-Details (z.B. nicht zurückgeschrumpfte Knoten-Splits) bleiben „warm" — konsistent über
    // alle Tiere/Profile und im Sinne des Cache-Warmup-Zwecks. ESKALATION: tier_clear in der Warmup-Phase (nicht
    // O(1)-invertierbar) sowie OOM beim Log eskalieren für DIE Periode auf die Vollkopie (escalate_undo_to_copy_).
    // Organe ohne lookup/insert/erase/restore_statistics (requires-detektiert) → unverändert Copy/MementoAxis.
    void tier_save_all() noexcept override {
        if constexpr (undo_log_capable_) {
            try {
                if (undo_log_.capacity() < 16) undo_log_.reserve(16);   // einmalig; danach realloc-frei (noexcept-faktisch)
                saved_search_stats_    = search_organ_.statistics();    // O(1) POD-Snapshot (T0)
                saved_container_stats_ = container_.statistics();       // O(1) POD-Snapshot (container-/T6-Pfad)
                undo_log_.clear();
                saved_search_.reset();                                  // alte Eskalations-Vollkopie freigeben
                saved_container_.reset();
                undo_escalated_ = false;
                undo_session_   = true;
                undo_armed_     = true;
                return;
            } catch (...) {
                // OOM bei reserve → für DIESE Periode auf den Vollkopie-Pfad unten ausweichen (Mess-Robustheit).
                undo_session_ = false; undo_escalated_ = false; undo_armed_ = false;
            }
        }
        try {
            if constexpr (std::is_copy_constructible_v<SearchAlgo>)  saved_search_.emplace(search_organ_);
            else if constexpr (MementoAxis<SearchAlgo>)              saved_search_m_    = search_organ_.save_state();
            if constexpr (std::is_copy_constructible_v<container_t>) saved_container_.emplace(container_);
            else if constexpr (MementoAxis<container_t>)             saved_container_m_ = container_.save_state();
        } catch (...) {
            saved_search_.reset();      // OOM beim Kapseln → Kopie-Fallback-Memento verwerfen (Mess-Robustheit)
            saved_container_.reset();
        }
    }

    void tier_rollback_all() noexcept override {
        if constexpr (undo_log_capable_) {
            if (undo_session_) {
                undo_armed_ = false;   // Mess-Phase: nicht mehr aufzeichnen (Mess-Op bleibt die REINE Op, 0 Log-Overhead)
                try {
                    if (undo_escalated_) {   // clear-/OOM-Eskalation: Vollkopie des save-Stands zurückspielen
                        if (saved_search_)    search_organ_ = *saved_search_;
                        if (saved_container_) container_    = *saved_container_;
                    } else {
                        replay_undo_log_();  // Einzel-Key-Inversen RÜCKWÄRTS über die Organ-API (O(#writes), meist 0-1)
                    }
                    // Stats NACH dem Daten-Replay restaurieren (Replay/old-Lookup haben Zähler berührt) → exakt save-Stand.
                    search_organ_.restore_statistics(saved_search_stats_);
                    container_.restore_statistics(saved_container_stats_);
                } catch (...) {
                    // noexcept-Vertrag: ein Rollback-Fehler darf den Mess-Lauf nicht abreißen.
                }
                return;   // idempotent: erneuter Aufruf → Log leer/Eskalations-Kopie unverändert → derselbe Stand
            }
        }
        try {
            if constexpr (std::is_copy_assignable_v<SearchAlgo>)  { if (saved_search_)    search_organ_ = *saved_search_; }
            else if constexpr (MementoAxis<SearchAlgo>)             search_organ_.restore_state(saved_search_m_);
            if constexpr (std::is_copy_assignable_v<container_t>) { if (saved_container_) container_    = *saved_container_; }
            else if constexpr (MementoAxis<container_t>)            container_.restore_state(saved_container_m_);
        } catch (...) {
            // noexcept-Vertrag: ein Rollback-Fehler darf den Mess-Lauf nicht abreißen.
        }
    }

    /// Diagnose (#133, compile-time): läuft das Zwei-Phasen-Memento dieser Komposition über den O(1)-Undo-Log
    /// (statt Organ-Vollkopie O(n))? Literal-prüfbar im Test (test_undolog_memento) — keine ABI-Fläche (statisch).
    [[nodiscard]] static constexpr bool tier_memento_is_undo_log() noexcept { return undo_log_capable_; }

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
    // Plan v2 S1 (2026-06-04): layout-honorierender Store — speichert Records am layout-getriebenen eff_stride
    // (CLA 64-B-gepaddet vs aos 16-B-packed) → die memory_layout-Achse ist ECHT, organ_observe_layout OOB-frei,
    // allocator-Bytes layout-abhängig. Drop-in zu NodeChunkedStore (StorageOrgan-Concept), ersetzt es im Mess-Pfad.
    using container_t = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::ObservableComposedSearch<
        ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::SortedBinaryTraversal,
        ::comdare::cache_engine::node::LayoutAwareChunkedStore<
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
    // V42 L-74c scan-Achsen-Auto-Kopplung: memory_layout + serialization Observer-Organe. Im Observer-Fill (fill_observer_v3)
    // ueber das ECHTE Slot-Backing des container_ getrieben (Pfad-B Zustand-Scan). mutable (const-Methode).
    mutable typename Composition::memory_layout ml_organ_;
    mutable typename Composition::serialization ser_organ_;
    mutable typename Composition::node_type     node_organ_;
    // Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen — in JEDEM Tier-Binary PRÄSENT (mindestens
    // instanziiert, damit das Achsen-Interface uniform vorhanden ist). Default-konstruiert; das Treiben/
    // Observieren (put/get bzw. should_flush in tier_insert/observe) ist ein Folge-Inkrement (Doc 30 §8.2).
    // mutable: ein späteres Mit-Treiben im const tier_lookup/observe bleibt logisch nicht-const (analog
    // telemetry_organ_/ml_organ_). MUSS für die 19-Slot-Composition kompilieren.
    mutable typename Composition::queuing_q1    queuing_q1_organ_;
    mutable typename Composition::queuing_q2    queuing_q2_organ_;
    // Phase A (2026-06-04): cache_traversal (T1) + mapping (T2) als real gehaltene, auto-gekoppelte Organe
    // (register/resolve in tier_insert/lookup). mutable: das Resolve-Tracking im const tier_lookup ist logisch
    // nicht-const (analog telemetry_organ_/queuing_*). Ihre statistics() liefert fill_observer_v3 (Pfad B).
    mutable typename Composition::cache_traversal ct_organ_;
    mutable typename Composition::mapping         map_organ_;
    // Phase B (2026-06-04): T7 prefetch (ObservablePrefetch-Hülle um die rohe Composition::prefetch-Strategie)
    // + T8 concurrency (ObservableConcurrency-Hülle um Composition::concurrency). Anders als telemetry/q1/q2
    // trägt die Composition für T7/T8 die NACKTE Strategie (kein statistics()) → die Hülle hält das Mess-Organ.
    // mutable: das observe_*-Tracking im const tier_lookup ist logisch nicht-const (analog telemetry_organ_).
    // Ihre statistics() liefert fill_observer_v3 (Pfad B). Default-konstruiert; auto-gekoppelt in tier_insert/lookup.
    mutable ::comdare::cache_engine::prefetch_axis::ObservablePrefetch<typename Composition::prefetch>       pf_organ_{};
    mutable ::comdare::cache_engine::concurrency_axis::ObservableConcurrency<typename Composition::concurrency> cc_organ_{};
    // Phase B (2026-06-04): T11 value_handle (ObservableValueHandle-Hülle um die rohe Composition::value_handle-
    // Strategie) + T12 isa (ObservableIsa-Hülle um Composition::isa). Anders als telemetry/q1/q2 trägt die
    // Composition für T11/T12 die NACKTE Strategie (kein statistics()) → die Hülle hält das Mess-Organ. Getrieben
    // NICHT in tier_insert/lookup, sondern als Pfad-B Zustand-Scan über das ECHTE container_-Slot-Backing in
    // fill_observer_v3 (store_observe_value_handle/store_observe_isa, idempotenter reset()+scan, analog ml/ser).
    // mutable: der Observe-Scan im const fill_observer_v3 ist logisch nicht-const (analog ml_organ_/ser_organ_).
    mutable ::comdare::cache_engine::value_handle_axis::ObservableValueHandle<typename Composition::value_handle> vh_organ_{};
    mutable ::comdare::cache_engine::simd::ObservableIsa<typename Composition::isa>                               isa_organ_{};
    // Phase B (2026-06-04) Abschluss: die 5 verbleibenden Observer-Hüllen als Member-Organe.
    //  • T3 path_compression: Instanz-Driver compress(key,depth), AUTO-gekoppelt in tier_insert/lookup (wie T1/T2).
    //  • T13/T14/T15/T16 (index_org/io_dispatch/migration/filter): scan-Achsen, getrieben als Pfad-B Zustand-Scan über
    //    das ECHTE container_-Slot-Backing in fill_observer_v3 (store_observe_*, idempotenter reset()+scan, wie ml/ser/vh).
    // mutable: das Tracking im const tier_lookup / const fill_observer_v3 ist logisch nicht-const (analog telemetry_organ_).
    mutable ::comdare::cache_engine::path_compression::ObservablePathCompression<typename Composition::path_compression> pc_organ_{};
    mutable ::comdare::cache_engine::index_organization::ObservableIndexOrg<typename Composition::index_organization>     idx_organ_{};
    mutable ::comdare::cache_engine::io_dispatch::ObservableIoDispatch<typename Composition::io_dispatch>                 io_organ_{};
    mutable ::comdare::cache_engine::migration_policy::ObservableMigration<typename Composition::migration_policy>        mig_organ_{};
    mutable ::comdare::cache_engine::filter_axis::ObservableFilter<typename Composition::filter>                          flt_organ_{};
    // KF-4/L-MEAS: zuletzt angewandte Resource-Control (IMMER, auch Messung-aus) — vom RuntimeVariableLoop je dyn.
    // Einstellung gesetzt. Reiner Steuer-Zustand (quert die ABI nicht; der Host liest die Wirkung über den Observer).
    ComdareResourceControlV1 applied_rc_{};
#if COMDARE_MEASUREMENT_ON
    // V5-#44 memento_all: Warmup-Vor-Zustand der getriebenen Organe. Lebt IN der Binary, quert die ABI NICHT.
    // PER-ACHSEN-Memento (bevorzugt, MementoAxis): memento_of_t<Organ> = Organ::memento_t (riche (key,value)-
    // Liste + Stats) bzw. EmptyMemento wenn das Organ KEIN MementoAxis ist (dann greift der Kopie-Fallback).
    memento_of_t<SearchAlgo>   saved_search_m_{};
    memento_of_t<container_t>  saved_container_m_{};
    // Kopie-Fallback für Organe ohne MementoAxis (std::optional → leer bis save_all; reset bei Kapsel-OOM).
    // Im Undo-Log-Pfad (#133) zusätzlich der ESKALATIONS-Träger (tier_clear-Warmup / Log-OOM → Vollkopie).
    std::optional<SearchAlgo>  saved_search_{};
    std::optional<container_t> saved_container_{};

    // ── Undo-Log-Memento (#133) — O(1)/Op statt Organ-Vollkopie O(n)/Op ─────────────────────────────────
    // Fähigkeits-Detektion (requires): das Organ trägt die map-äquivalente API (lookup/insert/erase) UND den
    // O(1)-Stat-Snapshot/-Restore (statistics()/restore_statistics(), ObservableComposedSearch/-Container).
    // Copy-Konstruierbarkeit bleibt Pflicht (Eskalations-Vollkopie bei tier_clear-Warmup / OOM).
    template <class S>
    static constexpr bool organ_undo_capable_v = requires(S& s, S const& cs) {
        { cs.lookup(std::uint64_t{}).has_value() } -> std::convertible_to<bool>;
        { *cs.lookup(std::uint64_t{}) }            -> std::convertible_to<std::uint64_t>;
        s.insert(std::uint64_t{}, std::uint64_t{});
        s.erase(std::uint64_t{});
        s.restore_statistics(cs.statistics());
    };
    static constexpr bool undo_log_capable_ =
        organ_undo_capable_v<SearchAlgo> && organ_undo_capable_v<container_t>
        && std::is_copy_constructible_v<SearchAlgo>  && std::is_copy_assignable_v<SearchAlgo>
        && std::is_copy_constructible_v<container_t> && std::is_copy_assignable_v<container_t>;

    /// Ein Log-Eintrag = die Einzel-Key-Inverse EINER Warmup-Mutation: hatte der Key vor der Mutation den
    /// Wert old_value (had_old) → beim Rollback (k → old_value) setzen; sonst → k entfernen. Map-äquivalent
    /// korrekt für insert (insert_or_assign-Semantik) UND erase; LIFO-Replay deckt Mehrfach-Mutationen ab.
    struct UndoEntry { std::uint64_t key; std::uint64_t old_value; bool had_old; };

    /// Stat-POD-Typ eines undo-fähigen Organs (sonst leerer Platzhalter — Member existiert immer).
    struct EmptyStatsSnap {};
    template <class S, bool Cap = organ_undo_capable_v<S>>
    struct OrganStatsSnap { using type = EmptyStatsSnap; };
    template <class S>
    struct OrganStatsSnap<S, true> { using type = decltype(std::declval<S const&>().statistics()); };

    /// Inverse VOR der Mutation erfassen (tier_insert/tier_erase, nur Warmup-Phase). Der old-Lookup läuft
    /// DIREKT am search_organ_ (berührt NUR dessen Stat-POD, der beim Rollback restauriert wird) — search_organ_
    /// und container_ halten dieselbe logische Map (identische Op-Folge), EIN old-Wert deckt beide Inversen.
    void record_undo_(std::uint64_t key) noexcept {
        if (undo_escalated_) return;                  // Periode bereits Vollkopie-gesichert
        try {
            auto const old = search_organ_.lookup(key);
            undo_log_.push_back(UndoEntry{key, old.has_value() ? static_cast<std::uint64_t>(*old) : 0u,
                                          old.has_value()});
        } catch (...) { escalate_undo_to_copy_(); }   // Log-OOM → Vollkopie-Eskalation für diese Periode
    }

    /// Log RÜCKWÄRTS über die Organ-API abspielen (Delegationskette gewahrt); danach ist der DATEN-Zustand
    /// exakt der save-Stand. Die dabei berührten Stat-Zähler restauriert tier_rollback_all im Anschluss.
    void replay_undo_log_() {
        for (auto it = undo_log_.rbegin(); it != undo_log_.rend(); ++it) {
            if (it->had_old) { (void)search_organ_.insert(it->key, it->old_value);
                               (void)container_.insert(it->key, it->old_value); }
            else             { (void)search_organ_.erase(it->key);
                               (void)container_.erase(it->key); }
        }
        undo_log_.clear();
    }

    /// Eskalation auf die Vollkopie für DIESE save-Periode (tier_clear-Warmup ist nicht O(1)-invertierbar;
    /// Log-OOM). Zuerst das bisherige Log zurückspielen (→ Organe wieder auf dem save-Stand), DANN kapseln —
    /// die Kopie trägt damit exakt den save-Daten-Stand (die Stats stellt der Rollback aus den Snapshots her).
    void escalate_undo_to_copy_() noexcept {
        if (undo_escalated_) return;
        try {
            replay_undo_log_();
            saved_search_.emplace(search_organ_);
            saved_container_.emplace(container_);
            undo_escalated_ = true;
        } catch (...) {
            saved_search_.reset(); saved_container_.reset();
            // Eskalation gescheitert (OOM): Rollback dieser Periode degradiert (Status-quo-äquivalente
            // Mess-Robustheit; die empirische rb_exact-Probe deckt strukturelle Fälle je Binary ab).
        }
    }

    typename OrganStatsSnap<SearchAlgo>::type  saved_search_stats_{};    // O(1)-Stat-Snapshot (save-Stand, T0)
    typename OrganStatsSnap<container_t>::type saved_container_stats_{}; // O(1)-Stat-Snapshot (container/T6-Pfad)
    std::vector<UndoEntry> undo_log_{};       // Inversen der Warmup-Mutationen (typisch 0-2 Einträge/Periode)
    bool undo_session_   = false;             // save_all lief über den Undo-Pfad (Routing in rollback_all)
    bool undo_armed_     = false;             // Recording AN (nur save→rollback; Mess-Op läuft log-frei)
    bool undo_escalated_ = false;             // diese Periode per Vollkopie gesichert (clear/OOM)
#endif
};

}  // namespace comdare::cache_engine::anatomy
