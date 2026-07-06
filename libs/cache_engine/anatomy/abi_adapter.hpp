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
//
// ── DATEI-NAVIGATION (god-header ~1805 Z.: EIN Adapter, viele ABI-Belange; bei Anker-Drift den
//    jeweiligen `// ───`-Banner greppen — Zeilen-Anker sind approximativ) ──────────────────────────
//   • SearchAlgorithmAbiAdapter<A>    — Klassen-Doku + Vererbungs-Matrix (MESSUNG-AN vs -AUS)     (~:127)
//   • IExecutionEngine-Override       — Lifecycle warm_up/run/reset/shutdown + engine_name        (~:150)
//   • IResourceControllableTier (KF-4)— tier_query_resource_caps / tier_apply_resource_control    (~:180)
//        IMMER verfuegbar; 5 steuerbare Achsen (concurrency/prefetch/allocator/traversal/value_handle)
//   - IAllocatorProxyTier (AP15-2)   -- tier_get_allocator (non-dagger get_allocator-Proxy)         (~:205)
//   • IAnatomyBase-Override           — composition_name / paper_id / genus / organ_count          (~:212)
//   • IMeasurableWorkload (Pfad A)    — run_workload / _segmented (4 Seg) / _segmented_v2 (19 Seg)  (~:232)
//   • IDriveableTier-Antrieb (IMMER)  — tier_insert/lookup/erase/clear/size (Pfad-B Observer-Auto-Kopplung)(~:672)
//        + Observer (MESSUNG-AN)      — fill_observer_v3 (~:907) · fill_segment_timing_v3 (~:1136) · tier_observe (~:1312)
//   • IRollbackableTier (MESSUNG-AN)  — memento_all: tier_save_all / tier_rollback_all (CoW)        (~:1331)
//   • §3.3-Delegations-Diagnose       — const-Diagnosen, KEINE ABI-Flaeche (tier_rollback_is_exact …)(~:1463)
//   • IScannableTier (MESSUNG-AN)     — tier_scan (YCSB-E Range-Scan)                               (~:1511)
//   • IMigratableTier (MESSUNG-AN)    — tier_migrate_step (echter 2-Ebenen-Move, P4 #123)           (~:1557)
//   • private Member (~:1631)         — container_algorithm_ (LayoutAwareChunkedStore<N,L,A>, ~:1680) + Mess-Organe + state_

#include "anatomy_base.hpp"
#include "measurable_workload.hpp"        // F15/Stufe B: optionales Mess-Sub-Interface (ABI-sicher)
#include "observable_tier.hpp"            // R6/Pfad B: ABI-stabiler Observer-Zugriff (Doku 24 §8.6)
#include "rollbackable_tier.hpp"          // V5-I6: memento_all-ABI-Sub-Interface (tier_save_all/tier_rollback_all)
#include "scannable_tier.hpp"             // V5-#49-E: Range-Scan-ABI-Sub-Interface (tier_scan, YCSB-E)
#include "resource_controllable_tier.hpp" // KF-4/L-MEAS: Laufzeit-Steuer-Sub-Interface (IMMER, nicht messungs-gated)
#include "allocator_proxy_tier.hpp"       // AP15-2: non-dagger get_allocator-Proxy-Sub-Interface
#include "observer_aggregate.hpp"         // ObservableAxis + ObserverAggregate (observable_count)
#include "memento_aggregate.hpp" // V5-I6-SUBSTANZ (#44): MementoAxis + save_axis/restore_axis (per-Achsen-Memento)
#include "../execution_engine/execution_engine_base.hpp"
// R6 Inkrement 2b: die allocator-Achse im Cross-ABI-Observer-POD — der ComposedStore<N,L,A>-Vector-Growth
// treibt die Allocator-Statistik REAL (2. Mess-Achse, spiegelt builder/AnatomyExecutionContext). Da der
// host-seitige Loader jetzt das LEICHTE anatomy_module_abi_v1_decl.hpp nutzt (NICHT mehr abi_adapter.hpp),
// belasten diese topics/-Includes nur die Voll-Header-Konsumenten (DLLs/Tests, die die Pfade ohnehin haben).
#include "../topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp"
// (E-Welle-A2 / Befund-2 / A2.5) Klassifikation + Mapping store-traversierbarer Such-Algos (für die container_algorithm_t-Traversal-Wahl)
#include <axes/lookup/composable/capacity_constraint.hpp>
#include <axes/lookup/composable/store_traversable_search_algo.hpp>
#include <axes/lookup/composable/traversal_for_search_algo.hpp>
#include <axes/lookup/composable/organ_for_search_algo.hpp>         // #188-4b-b1b: organ_for_search_algo_t (Pool→Organ)
#include <axes/lookup/composable/observable_composed_container.hpp> // #188-4b-b1b: ObservableComposedContainer<Organ>
#include "../axes/node/axis_04_node_type_layout_aware_store.hpp" // Plan v2 S1: layout-honorierender Store (CLA-Stride echt, OOB behoben)
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
// per Registry-make_observable_* vorab dekoriert sind), getrieben über das ECHTE container_algorithm_-Slot-Backing
// (store_observe_value_handle/store_observe_isa → NodeChunkedStore::organ_observe_*, analog layout/serialization).
#include "../axes/value_handle_axis/axis_14_value_handle_observable.hpp"
#include "../axes/simd/axis_09_isa_observable.hpp"
// Phase B (2026-06-04) Abschluss: die 5 verbleibenden Observer-HÜLLEN. T3 path_compression (Instanz-Driver compress(),
// auto-gekoppelt in tier_insert/lookup wie T1/T2) + T13 index_org / T14 io_dispatch / T15 migration_policy / T16 filter
// (scan-Achsen, getrieben über das ECHTE container_algorithm_-Slot-Backing in fill_observer_v3, store_observe_* → NodeChunkedStore::
// organ_observe_*, idempotenter reset()+scan, analog value_handle/isa/memory_layout/serialization). Anders als
// telemetry/q1/q2 trägt die Composition für T3/T13/T14/T15/T16 die NACKTE Strategie (kein statistics()) → die Hülle hält
// das Mess-Organ.
#include "../axes/path_compression/axis_02_path_compression_observable.hpp"
#include "../axes/index_organization/axis_01_index_organization_observable.hpp"
#include "../axes/io_dispatch/axis_io_dispatch_observable.hpp"
#include "../axes/migration_policy/axis_migration_observable.hpp"
#include "../axes/filter_axis/axis_filter_observable.hpp"

#include <array>
#include <chrono>
#include <cstring> // (X) std::memcpy in den 19-Segment-Treibe-Ops
#if defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h> // (X) _mm_prefetch für das T7-Prefetch-Segment (MSVC/x86_64-Mess-Build)
#endif
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits> // V5-I6: is_copy_constructible/assignable-Guards für die In-Memory-Memento-Kopie
#include <utility>     // #133 CoW-Memento: std::declval im OrganStatsSnap-Typ-Extrakt
#include <vector>      // keys-Ernte (Per-Achsen-Seg-Timing) + Memento-Snapshot-Listen (save_state)

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// SearchAlgorithmAbiAdapter — bridge AnatomyConcept-konformer SearchAlgorithm-
// Anatomie zu IAnatomyBase (Mammal-Gattung in Tier-Metapher, Doku 14 §27.2)
// ─────────────────────────────────────────────────────────────────────────────

/// SearchAlgorithmAbiAdapter<A> — generischer Runtime-ABI-Adapter fuer die
/// SearchAlgorithm-Gattung (Mammal in Tier-Metapher).
///
/// GoF-Ehrlichkeit (K10/#224): "ABI-Adapter" = Adaption der Composition-Anatomie
/// (A::composition_t-Organtypen) an die extern-C-IAnatomyBase-vtable — KEIN klassischer
/// GoF-Object-Adapter. Die Organe werden INTERN konstruiert (eigene Member
/// container_algorithm_/telemetry_organ_/...), es gibt KEINEN injizierten Adaptee-
/// Instanz-Member (anders als SetAbiAdapter mit `A anatomy_{}`, das an eine Adaptee-
/// Instanz delegiert). "Adapter" ist hier als vtable-/ABI-Adapter ehrlich, nicht als
/// Adaptee-delegierender Object-Adapter. Die volle Delegations-Vereinheitlichung
/// (echte Adaptee-Delegation) ist bewusst an Befund-2/#188 gekoppelt.
///
/// I1-Vereinheitlichung (Doc 36 §2.5/§4, TODO-6, 2026-06-25): DIES ist die EINE
/// ABI-Laufzeit-SICHT des SearchAlgorithm-Lebewesens — in der Thesis (ch4 fig:abi)
/// „SearchEngine" genannt. Es gibt KEINE zweite, achsentragende Engine-Hierarchie:
/// die frueheren Legacy-Klassen comdare::search_engine<> / comdare::execution_engine<>
/// (REV-7-„Drei-Schichten") waren ein toter Parallel-Baum und wurden ENTFERNT. Die 19
/// Organe leben GENAU EINMAL in SearchAlgorithmAnatomy<A::composition_t>; die variadische
/// Hybrid-Kollektion (1=>vector / 2=>map / N=>map<K,tuple>) lebt in search_algorithm_type_collection.
/// Lebewesen == SearchAlgorithm; Anatomie = sein Koerper; „SearchEngine" = nur diese Sicht.
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
/// Phasen `Uninitialized → Warming → Running → Idle → Shutdown`; die Lifecycle-Hooks
/// (warm_up/run/reset/shutdown) setzen NUR `state_` (kein eigener Preheat). Die echte
/// Mess-Last/Bulk-Load laeuft separat ueber den Workload-Treiber (run_workload, s.u. IMeasurableWorkload).
template <AnatomyConcept A>
class SearchAlgorithmAbiAdapter final
    : public IAnatomyBase,
      public IResourceControllableTier, // KF-4/L-MEAS: IMMER (auch Messung-aus), eigenständiges Sub-Interface (dynamic_cast)
      public IAllocatorProxyTier,       // AP15-2: IMMER, non-dagger get_allocator-Proxy-Sub-Interface (dynamic_cast)
// V5-I2.2/I6: Antrieb ⊥ Observer ⊥ Memento compile-time disjunkt (kein Diamond — IObservableTier/IRollbackableTier IS-A nichts Gemeinsames; IDriveableTier kommt über IObservableTier).
//   MESSUNG-AN  : IObservableTier (Antrieb + tier_observe/observer_all) + IMeasurableWorkload (Pfad-A, host-relokal. I9) + IRollbackableTier (memento_all, V5-I6) + IScannableTier (Range-Scan, V5-#49-E).
//   MESSUNG-AUS : NUR IDriveableTier (funktionaler Antrieb) → Release-/funktional-only-DLL OHNE jeden Mess-Overhead (kein Observer-, kein Memento-, kein Scan-vtable-Slot).
#if COMDARE_MEASUREMENT_ON
      public IObservableTier, // I1: GENAU EINE Observer-Schnittstelle (V2/V3/V4 konsolidiert, s. docs/architecture/31_*)
      public IMeasurableWorkload,
      public IMeasurableWorkloadV2, // (C-2): eigenständiges V2-Sub-Interface — per-Segment-Timer (4 Achsen)
      public IMeasurableWorkloadV3, // (X): eigenständiges V3-Sub-Interface — per-Segment-Timer ALLER 19 Achsen
      public IRollbackableTier,
      public IMigratableTier, // P4 (#123): ECHTER 2-Ebenen-Migrations-Schritt (tier_moves real > 0)
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

    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }

    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }

    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }

    /// R5.C.A4: Mess-Phase aktivieren — Lifecycle Warming/Idle → Running.
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }

    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }

    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    // ─────────────────────────────────────────────────────────────────────
    // KF-4 / L-MEAS — IResourceControllableTier (IMMER verfügbar, auch Messung-aus). Die SearchAlgorithm-Gattungs-
    // Komposition trägt genau 5 laufzeit-steuerbare Achsen (concurrency/prefetch/allocator/traversal/value_handle);
    // der Host-Loop (RuntimeVariableLoop) quert die Caps + wendet je dyn. Einstellung an (kein Reload). apply klammert
    // an die Caps + merkt die zuletzt angewandte Steuerung (applied_rc_). Eigenständiges Sub-Interface (dynamic_cast).
    // ─────────────────────────────────────────────────────────────────────
    void tier_query_resource_caps(ComdareResourceControlV1* out_caps) const noexcept override {
        if (out_caps == nullptr) return;
        *out_caps                         = ComdareResourceControlV1{};
        out_caps->thread_count            = 64;                       // concurrency (axis_08)
        out_caps->prefetch_distance       = 64;                       // prefetch (axis_07), in Cache-Lines
        out_caps->pool_budget_bytes       = (std::uint64_t{1} << 30); // allocator (axis_06), 1 GiB Arena-Obergrenze
        out_caps->batch_size              = 4096;                     // traversal (axis_03a) Working-Set
        out_caps->inline_threshold_bytes  = 256;                      // value_handle (axis_14)
        out_caps->controllable_axis_count = 5;                        // genus-invariant: 5 steuerbare Achsen
    }
    [[nodiscard]] std::uint64_t tier_apply_resource_control(ComdareResourceControlV1 const* in) noexcept override {
        if (in == nullptr) return 0;
        ComdareResourceControlV1 caps{};
        tier_query_resource_caps(&caps);
        std::uint64_t applied = 0;
        auto          apply1  = [&applied](std::uint64_t v, std::uint64_t cap, std::uint64_t& dst) noexcept {
            if (v != 0) {
                dst = (cap != 0 && v > cap) ? cap : v;
                ++applied;
            } // 0 = Default beibehalten; sonst an cap klammern
        };
        apply1(in->thread_count, caps.thread_count, applied_rc_.thread_count);
        apply1(in->prefetch_distance, caps.prefetch_distance, applied_rc_.prefetch_distance);
        apply1(in->pool_budget_bytes, caps.pool_budget_bytes, applied_rc_.pool_budget_bytes);
        apply1(in->batch_size, caps.batch_size, applied_rc_.batch_size);
        apply1(in->inline_threshold_bytes, caps.inline_threshold_bytes, applied_rc_.inline_threshold_bytes);
        // RC-prefetch_distance = Laufzeit-Distanz-Override am realen Store-Prefetch;
        // #229-Folge = die 4 übrigen RC-Achsen + KF-5-§7-Voll-API.
        if constexpr (requires { pf_organ_.set_runtime_distance(std::uint32_t{}); }) {
            pf_organ_.set_runtime_distance(static_cast<std::uint32_t>(applied_rc_.prefetch_distance));
        }
        return applied;
    }

    // ----------------------------------------------------------------------
    // AP15-2 -- IAllocatorProxyTier (IMMER verfuegbar, non-dagger get_allocator-Proxy). Kein std::map-Oracle-
    // Bestandteil: Der Host bekommt nur eine ehrliche axis_06-Identitaet und, falls vorhanden, die schmale
    // S7-T6-Allocator-Statistikroute. Die T6-Kaskade in fill_observer_v3 bleibt absichtlich unveraendert;
    // die gleiche requires-Folge ist hier gespiegelt, damit der alte Observer-Pfad byte-/verhaltensstabil bleibt.
    // ----------------------------------------------------------------------
    void tier_get_allocator(ComdareAllocatorProxyV1* out) const noexcept override {
        if (out == nullptr) return;
        *out                 = ComdareAllocatorProxyV1{};
        out->format_version  = kAllocatorProxyFormatVersion;
        constexpr auto idbit = std::uint64_t{1} << 0u;
        constexpr auto stbit = std::uint64_t{1} << 1u;
        constexpr auto orbit = std::uint64_t{1} << 2u;

        if constexpr (requires { typename Composition::allocator::family_id; }) {
            out->allocator_family_id = static_cast<std::uint64_t>(Composition::allocator::family_id::value);
            out->flags |= idbit;
            if constexpr (requires { Composition::allocator::is_original_module(); }) {
                if (Composition::allocator::is_original_module()) { out->flags |= orbit; }
            }
        }

#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (ObservableAxis<typename Composition::allocator> &&
                      requires { container_algorithm_.store_allocator_statistics(); }) {
            auto const a = container_algorithm_.store_allocator_statistics();
            if constexpr (requires {
                              a.total_bytes_allocated;
                              a.total_bytes_in_use;
                              a.allocation_count;
                              a.deallocation_count;
                              a.failure_count;
                          }) {
                out->total_bytes_allocated = static_cast<std::uint64_t>(a.total_bytes_allocated);
                out->total_bytes_in_use    = static_cast<std::uint64_t>(a.total_bytes_in_use);
                out->allocation_count      = static_cast<std::uint64_t>(a.allocation_count);
                out->flags |= stbit;
            } else if constexpr (requires {
                                     a.bytes_allocated;
                                     a.alloc_calls;
                                 }) {
                out->total_bytes_allocated = static_cast<std::uint64_t>(a.bytes_allocated);
                out->total_bytes_in_use    = static_cast<std::uint64_t>(a.bytes_allocated);
                out->allocation_count      = static_cast<std::uint64_t>(a.alloc_calls);
                if constexpr (requires { a.live_nodes; }) {
                    out->live_nodes = static_cast<std::uint64_t>(a.live_nodes);
                }
                out->flags |= stbit;
            }
        }
#endif
    }

    // ─────────────────────────────────────────────────────────────────────
    // IAnatomyBase-Pflicht-Override (R5.C.A Anatomie-Schicht)
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }

    [[nodiscard]] std::string_view paper_id() const noexcept override { return A::paper_id(); }

    [[nodiscard]] AnatomyGenus genus() const noexcept override { return A::genus(); }

    [[nodiscard]] std::size_t organ_count() const noexcept override { return A::organ_count(); }

    // ─────────────────────────────────────────────────────────────────────
    // IMeasurableWorkload-Override (F15 / Stufe B): Mess-Last DURCH die DLL
    // ─────────────────────────────────────────────────────────────────────

    // (C-2): Akkumulator der 4 per-Segment-ns über die Batches (nur Mess-Hilfsstruktur, quert die ABI NICHT).
    struct SegmentAccumulator {
        std::int64_t s1 = 0, s2 = 0, s3 = 0, s4 = 0;
    };

    /// KOMPOSIT-Mess-Last (R5.B, F15 Stufe B) — uebt VIER Achsen der geladenen Komposition aus:
    ///   Segment 1: insert/lookup auf A::composition_t::search_algo  (search_algo-Achse)
    ///   Segment 2: alloc/dealloc-Churn ueber A::composition_t::allocator (allocator-Achse)
    ///   Segment 3: Feld-Scan ueber A::composition_t::memory_layout (memory_layout-Achse; AoS-strided
    ///              vs SoA-contiguous → echter Cache-Effekt)
    ///   Segment 4: Encode-Scan ueber A::composition_t::serialization (serialization-Achse; raw <
    ///              compressed < var_len < succinct CPU-Aufwand, Doku 22 §4)
    /// Alle vier kompiliert IN der DLL; die Batch-Latenz misst die GANZE Komposition entlang der
    /// variierten Achsen. F15s paarweiser Holm-Test isoliert jede Achse (Paare, die sich nur in einer
    /// Achse unterscheiden) — Dominanz/Balance der Segmente ist dafuer irrelevant.
#if COMDARE_MEASUREMENT_ON // V5-I2.2: Pfad-A run_workload NUR bei Messung-AN (V3-Designfehler; host-relokalisiert in I9)
    [[nodiscard]] std::uint64_t run_workload(std::uint64_t ops_per_batch, std::uint64_t batches, std::uint64_t seed,
                                             std::int64_t* out_latencies_ns,
                                             std::uint64_t out_capacity) noexcept override {
        if (out_latencies_ns == nullptr || out_capacity == 0 || batches == 0 || ops_per_batch == 0) { return 0; }
        try {
            using SearchAlgo = typename A::composition_t::search_algo;
            using Allocator  = typename A::composition_t::allocator;     // R5.B: 2. operative Achse
            using MemLayout  = typename A::composition_t::memory_layout; // R5.B: 3. operative Achse
            using Serializer = typename A::composition_t::serialization; // R5.B: 4. operative Achse (axis_10)
            using K          = typename SearchAlgo::key_type;
            SearchAlgo algo;
            for (int k = 0; k < 256; ++k) { algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u); }
            Allocator                 alloc;         // Komposition-Allocator (persistiert ueber Batches)
            constexpr std::size_t     kChurn = 2048; // moderate alloc/dealloc-Last je Batch
            constexpr std::size_t     kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const                churn_size = [](std::size_t j) noexcept {
                return std::size_t{16} + ((j * 16u) & 0xF0u); // 16..256 Bytes, deterministisch
            };
            // Layout-Scan-Puffer (Segment 3): 16384 Datensaetze. LAYOUT-FIX (X-§4, 2026-06-04): kRecordSize=48
            // (NICHT 64) — sonst fiele cache_line_aligned (aligned_stride=round_up(48,64)=64) mit aos_strict
            // (Stride 48) zusammen und die Layout-Achse differenzierte nicht. Damit liest aos_strict bei Stride 48,
            // cache_line_aligned bei Stride 64. PFLICHT-OOB-SCHUTZ: der Puffer wird nach dem GRÖSSTMÖGLICHEN Stride
            // (64) dimensioniert (kLbufBytes = kRecords*64 >= jeder Layout-Stride), sonst läse der CLA-Scan bei
            // (kRecords-1)*64+4 über das Ende hinaus. EINMAL via Komposition-Allocator alloziert (Setup, NICHT gemessen).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes =
                kRecords * 64; // OOB-Schutz: größtmöglicher Layout-Stride (64), nicht kRecordSize
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
                using clock        = std::chrono::steady_clock;
                auto seg_ns        = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
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
                    void* p                         = alloc.allocate(churn_size(j), kAlign);
                    *static_cast<unsigned char*>(p) = static_cast<unsigned char>(j); // touch
                    sink += *static_cast<unsigned char*>(p);
                    blocks[j] = p;
                }
                for (std::size_t j = 0; j < kChurn; ++j) { alloc.deallocate(blocks[j], churn_size(j), kAlign); }
                auto const s2b = clock::now();
                // Segment 3: memory_layout-Achse (Feld-Scan im layout-charakteristischen Zugriffsmuster)
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                auto const s3b = clock::now();
                // Segment 4: serialization-Achse (Encode-Scan im strategie-charakteristischen CPU-Aufwand:
                // raw=Byte-Sum < compressed=Delta+Zigzag < var_len=LEB128 < succinct=Bit-Packing). Doku 22 §4.
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
                auto const         s4b  = clock::now();
                std::int64_t const seg1 = seg_ns(s1a, s1b);
                std::int64_t const seg2 = seg_ns(s1b, s2b);
                std::int64_t const seg3 = seg_ns(s2b, s3b);
                std::int64_t const seg4 = seg_ns(s3b, s4b);
                if (seg != nullptr) {
                    seg->s1 += seg1;
                    seg->s2 += seg2;
                    seg->s3 += seg3;
                    seg->s4 += seg4;
                }
                std::int64_t const ns = seg1 + seg2 + seg3 + seg4;
                return (sink == ~0ull) ? (ns ^ 1) : ns; // sink-Nutzung gegen Wegoptimierung
            };
            do_batch(nullptr); // Warmup (verworfen)
            std::uint64_t const n = (batches < out_capacity) ? batches : out_capacity;
            for (std::uint64_t b = 0; b < n; ++b) out_latencies_ns[b] = do_batch(nullptr);
            alloc.deallocate(lbuf, kLbufBytes, 64); // Layout-Puffer freigeben
            return n;
        } catch (...) {
            return 0; // noexcept-Vertrag: interne Exception (z.B. OOM) → 0 Samples
        }
    }

    // (C-2): per-Segment-Timer-Variante. EXAKT derselbe 4-Segment-do_batch wie run_workload, aber je Segment
    // ein eigener steady_clock-Timer, über die batches AUFSUMMIERT → echte per-Achsen-Zeit für genau die 4
    // run_workload-getriebenen Achsen (search_algo/allocator/memory_layout/serialization). COMDARE_MEASUREMENT_ON-gegated.
    [[nodiscard]] std::uint64_t run_workload_segmented(std::uint64_t ops_per_batch, std::uint64_t batches,
                                                       std::uint64_t            seed,
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
            for (int k = 0; k < 256; ++k) { algo.insert(static_cast<K>(k), static_cast<std::uint64_t>(k) * 7u + 1u); }
            Allocator                 alloc;
            constexpr std::size_t     kChurn = 2048;
            constexpr std::size_t     kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const churn_size = [](std::size_t j) noexcept { return std::size_t{16} + ((j * 16u) & 0xF0u); };
            // LAYOUT-FIX (X-§4): kRecordSize=48 + kLbufBytes nach größtmöglichem Layout-Stride (64) dimensioniert
            // (OOB-Schutz), identisch zu run_workload. So differenziert die memory_layout-Achse aos_strict(48) vs
            // cache_line_aligned(64).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes  = kRecords * 64; // OOB-Schutz: größtmöglicher Layout-Stride (64)
            unsigned char*        lbuf        = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);
            std::mt19937_64 rng{seed};
            using clock = std::chrono::steady_clock;
            auto seg_ns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            SegmentAccumulator acc{};
            std::uint64_t      sink         = 0;
            auto               do_seg_batch = [&]() {
                auto const s1a = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
                    if (v) sink += *v;
                }
                auto const s1b = clock::now();
                for (std::size_t j = 0; j < kChurn; ++j) {
                    void* p                         = alloc.allocate(churn_size(j), kAlign);
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
                acc.s1 += seg_ns(s1a, s1b);
                acc.s2 += seg_ns(s1b, s2b);
                acc.s3 += seg_ns(s2b, s3b);
                acc.s4 += seg_ns(s3b, s4b);
            };
            do_seg_batch(); // Warmup (verworfen) — acc zurücksetzen
            acc = SegmentAccumulator{};
            for (std::uint64_t b = 0; b < batches; ++b) do_seg_batch();
            alloc.deallocate(lbuf, kLbufBytes, 64);
            out->seg_search_algo_ns   = (sink == ~0ull) ? (acc.s1 ^ 1) : acc.s1; // sink-Nutzung gegen Wegoptimierung
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
    [[nodiscard]] std::uint64_t run_workload_segmented_v2(std::uint64_t ops_per_batch, std::uint64_t batches,
                                                          std::uint64_t            seed,
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

            CacheTraversal traversal; // T1: register K Einträge → resolve N-fach
            using TV                    = typename CacheTraversal::value_type;
            constexpr std::size_t kTrav = 256;
            for (std::size_t k = 0; k < kTrav; ++k)
                traversal.register_entry(static_cast<typename CacheTraversal::key_type>(k),
                                         static_cast<TV>(k * 7u + 1u));

            Mapping               mapping; // T2: register K Slots → resolve_offset N-fach
            constexpr std::size_t kSlots = 256;
            for (std::size_t s = 0; s < kSlots; ++s)
                mapping.register_slot(static_cast<typename Mapping::slot_index_type>(s),
                                      static_cast<typename Mapping::offset_type>(s * 64u));

            Allocator                 alloc;
            constexpr std::size_t     kChurn = 2048;
            constexpr std::size_t     kAlign = 16;
            std::array<void*, kChurn> blocks{};
            auto const churn_size = [](std::size_t j) noexcept { return std::size_t{16} + ((j * 16u) & 0xF0u); };

            // LAYOUT-FIX (X-§4): kRecordSize=48; Lbuf nach größtmöglichem Stride (64) dimensioniert (OOB-Schutz).
            // path_descend_scan (T3 Patricia) liest 8 B/Record → record_size>=8 sicher (letzter Read (kRecords-1)*48+8).
            constexpr std::size_t kRecords    = 16384;
            constexpr std::size_t kRecordSize = 48;
            constexpr std::size_t kLbufBytes  = kRecords * 64;
            unsigned char*        lbuf        = static_cast<unsigned char*>(alloc.allocate(kLbufBytes, 64));
            for (std::size_t i = 0; i < kLbufBytes; ++i) lbuf[i] = static_cast<unsigned char>(i * 31u + 7u);

            // Query-Puffer für T16 filter_probe_scan (1 Byte je Query).
            constexpr std::size_t               kQueries = 4096;
            std::array<unsigned char, kQueries> qbuf{};
            for (std::size_t i = 0; i < kQueries; ++i) qbuf[i] = static_cast<unsigned char>(i * 53u + 11u);

            Telemetry telemetry_local; // T10: eigenes Segment (entkoppelt vom Cross-ABI-Auto-Coupling)
            QueuingQ1 q1_local;        // T17: put/get-Churn
            QueuingQ2 q2_local;        // T18: should_flush/on_flush_complete
            using QElem = typename QueuingQ1::element_type;

            // GAP #2 Re-Grounding (P5d-Rest, User-Plan dynamic-frolicking-truffle Step 3, 2026-06-18): das T7-Segment
            // (Pfad-A, isolierte Achsen-Zeit seg_prefetch_ns) misst NICHT MEHR nur synthetisches lbuf-Prefetch, sondern
            // den REALEN Descent-Prefetch auf einen lokalen LayoutAwareChunkedStore mit echten Records — STRATEGIE-DISTINKT
            // via der bereits committeten PrefetchDescentPolicy<Prefetch> / observe_prefetch_descent (K9-Fix). Damit ist
            // seg_prefetch_ns real + per-Strategie differenziert (none=0 _mm_prefetch / hardware/distance/path>0), genau
            // wie der Pfad-B-Hot-Path (Z.1150 ff). Setup EINMAL (NICHT gemessen), gleicher Store-Typ wie container_algorithm_:
            // LayoutAwareChunkedStore<node_type, memory_layout, allocator> der Composition. Die anderen 18 Segmente
            // bleiben UNVERAENDERT (rein additiv im T7). @related [[reference_thesis_core_contribution_axis_library]]
            using PfStore = ::comdare::cache_engine::node::LayoutAwareChunkedStore<NodeType, MemLayout, Allocator>;
            PfStore               pf_store;        // T7: lokaler realer Store (echte Slot-Adressen)
            constexpr std::size_t kPfSlots = 4096; // genug echte Records fuer Distance/Path-Voraus
            for (std::size_t i = 0; i < kPfSlots; ++i)
                pf_store.append_slot(static_cast<typename PfStore::key_type>(i * 2654435761ull + 1u),
                                     static_cast<typename PfStore::value_type>(i * 7u + 1u));
            ::comdare::cache_engine::prefetch_axis::ObservablePrefetch<Prefetch> pf_seg_organ{}; // T7-Mess-Organ

            std::mt19937_64 rng{seed};
            using clock = std::chrono::steady_clock;
            auto seg_ns = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            std::int64_t  acc[19] = {};
            std::uint64_t sink    = 0;

            // ── EIN Batch: die 19 Achsen-Segmente, jedes einzeln gezeitet (Index = Seg-Map T0..T18) ─────────
            auto do_seg19 = [&]() {
                clock::time_point t0, t1;
                // T0 search_algo: Lookups auf der geladenen Such-Struktur.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = algo.lookup(static_cast<K>(rng() & 0xFFu));
                    if (v) sink += *v;
                }
                t1 = clock::now();
                acc[0] += seg_ns(t0, t1);
                // T1 cache_traversal: resolve N-fach.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = traversal.resolve(static_cast<typename CacheTraversal::key_type>(rng() % kTrav));
                    if (v) sink += static_cast<std::uint64_t>(*v);
                }
                t1 = clock::now();
                acc[1] += seg_ns(t0, t1);
                // T2 mapping: resolve_offset N-fach.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto o = mapping.resolve_offset(static_cast<typename Mapping::slot_index_type>(rng() % kSlots));
                    if (o) sink += static_cast<std::uint64_t>(*o);
                }
                t1 = clock::now();
                acc[2] += seg_ns(t0, t1);
                // T3 path_compression: Patricia → path_descend_scan; sonst (ByteWise/None) → kanonisches
                // ByteWiseKeyPrefix::common_prefix_len-Organ über den Record-Puffer (echtes Mess-Organ der Achse).
                t0 = clock::now();
                if constexpr (requires { PathCompression::path_descend_scan(lbuf, kRecords, kRecordSize); }) {
                    sink += PathCompression::path_descend_scan(lbuf, kRecords, kRecordSize);
                } else {
                    for (std::size_t i = 0; i < kRecords; ++i) {
                        std::uint64_t key;
                        std::memcpy(&key, lbuf + i * kRecordSize, sizeof(key));
                        auto const prefix =
                            ::comdare::cache_engine::path_compression::ByteWiseKeyPrefix::from_bytes(key, 7u);
                        sink += prefix.common_prefix_len(key >> 8U);
                    }
                }
                t1 = clock::now();
                acc[3] += seg_ns(t0, t1);
                // T4 node_type: node_find_scan (KF-6 Pflicht-API, order-sensitiver Probe-Scan).
                t0 = clock::now();
                sink += NodeType::node_find_scan(lbuf, kRecords, qbuf.data(), kQueries);
                t1 = clock::now();
                acc[4] += seg_ns(t0, t1);
                // T5 memory_layout: scan_field_sum (layout-charakteristischer Stride; aos_strict 48 vs CLA 64).
                t0 = clock::now();
                sink += MemLayout::scan_field_sum(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[5] += seg_ns(t0, t1);
                // T6 allocator: alloc → touch → dealloc-Churn (Pool reused Free-Lists).
                t0 = clock::now();
                for (std::size_t j = 0; j < kChurn; ++j) {
                    void* p                         = alloc.allocate(churn_size(j), kAlign);
                    *static_cast<unsigned char*>(p) = static_cast<unsigned char>(j);
                    sink += *static_cast<unsigned char*>(p);
                    blocks[j] = p;
                }
                for (std::size_t j = 0; j < kChurn; ++j) alloc.deallocate(blocks[j], churn_size(j), kAlign);
                t1 = clock::now();
                acc[6] += seg_ns(t0, t1);
                // T7 prefetch: GAP #2 Re-Grounding (P5d-Rest, 2026-06-18) — der isolierte Pfad-A-Achsen-Timer misst jetzt
                // den REALEN Descent-Prefetch auf den lokalen pf_store (echte Slot-Adressen), STRATEGIE-DISTINKT via der
                // committeten PrefetchDescentPolicy<Prefetch> / observe_prefetch_descent (identisch zum Pfad-B-Hot-Path):
                //   • none      → 0 reale _mm_prefetch (0-Overhead-Baseline; kleinste Latenz, ehrlich).
                //   • hardware  → 1 _mm_prefetch je Op auf den aktuellen realen Slot.
                //   • distance  → 1 _mm_prefetch auf den realen Slot i+N voraus.
                //   • path      → MEHRERE _mm_prefetch (Bundle entlang des Pfads).
                // Damit ist seg_prefetch_ns REAL + per-Strategie differenziert statt synthetisches-lbuf-Prefetch. Der
                // descent-Slot je Op ist eine echte (geklemmte) Position im realen Backing (kein OOB, descent_for_pf_).
                // is_active() statisch → kein zusaetzlicher Instanz-Bedarf der Strategie selbst (pf_seg_organ haelt die Huelle).
                t0 = clock::now();
                {
                    std::size_t const pf_n = pf_store.slot_count();
                    if (pf_n != 0) {
                        for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                            std::size_t const slot =
                                static_cast<std::size_t>(rng()) % pf_n; // echter, in-range Descent-Slot
                            pf_seg_organ.observe_prefetch_descent(pf_store,
                                                                  slot); // REALES _mm_prefetch (strat-distinkt)
                        }
                    }
                }
                sink += MemLayout::scan_field_sum(lbuf, kRecords,
                                                  kRecordSize); // Re-Scan (Prefetch-Hint-Wirkung auf den Layout-Scan)
                t1 = clock::now();
                acc[7] += seg_ns(t0, t1);
                // T8 concurrency: acquire/release um eine Mini-Critical-Section N-fach (strategie-charakteristisches
                // Sync-Primitiv: None=no-op, Blocking=mutex, LockFree=CAS, …). if-constexpr-detektiert (optionale Op).
                t0 = clock::now();
                if constexpr (requires {
                                  Concurrency::acquire();
                                  Concurrency::release();
                              }) {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                        Concurrency::acquire();
                        sink += (i & 1u);
                        Concurrency::release();
                    }
                } else {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i)
                        sink += (i & 1u); // ehrlicher No-Sync-Baseline-Fall
                }
                t1 = clock::now();
                acc[8] += seg_ns(t0, t1);
                // T9 serialization: serialize_scan (strategie-charakteristischer Encode-CPU-Aufwand).
                t0 = clock::now();
                sink += Serializer::serialize_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[9] += seg_ns(t0, t1);
                // T10 telemetry: record_node_touch N-fach (eigenes Segment, lokales Organ).
                t0 = clock::now();
                if constexpr (requires { telemetry_local.record_node_touch(true); }) {
                    for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                        telemetry_local.record_node_touch((i & 1u) != 0u);
                    }
                    sink += static_cast<std::uint64_t>(ops_per_batch);
                } else {
                    sink += static_cast<std::uint64_t>(ops_per_batch);
                }
                t1 = clock::now();
                acc[10] += seg_ns(t0, t1);
                // T11 value_handle: value_access_scan (strategie-charakteristische Deref-Last).
                t0 = clock::now();
                sink += ValueHandle::value_access_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[11] += seg_ns(t0, t1);
                // T12 isa: simd_field_sum (SSE2 bei Amd64 / Scalar-Fallback sonst). Nur (buf,n) — kein record_size.
                t0 = clock::now();
                sink += Isa::simd_field_sum(lbuf, kRecords);
                t1 = clock::now();
                acc[12] += seg_ns(t0, t1);
                // T13 index_organization: index_org_scan (sequential/random/embedded/unordered je Strategie).
                t0 = clock::now();
                sink += IndexOrg::index_org_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[13] += seg_ns(t0, t1);
                // T14 io_dispatch: io_dispatch_scan (in-memory-Dispatch-Simulation je Strategie).
                t0 = clock::now();
                sink += IoDispatch::io_dispatch_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[14] += seg_ns(t0, t1);
                // T15 migration_policy: migration_decide_scan (Entscheidungslogik-Kosten ohne 2. Tier).
                t0 = clock::now();
                sink += Migration::migration_decide_scan(lbuf, kRecords, kRecordSize);
                t1 = clock::now();
                acc[15] += seg_ns(t0, t1);
                // T16 filter: filter_probe_scan (Bloom/Cuckoo/SuRF/Xor-Probe; buf,n,queries,q).
                t0 = clock::now();
                sink += Filter::filter_probe_scan(lbuf, kRecords, qbuf.data(), kQueries);
                t1 = clock::now();
                acc[16] += seg_ns(t0, t1);
                // T17 queuing_q1: put N + get N (Buffer-Strategy).
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) q1_local.put(static_cast<QElem>(i));
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto v = q1_local.get();
                    if (v) sink += static_cast<std::uint64_t>(*v);
                }
                t1 = clock::now();
                acc[17] += seg_ns(t0, t1);
                // T18 queuing_q2: should_flush N + on_flush_complete (Flush-Policy).
                t0 = clock::now();
                for (std::uint64_t i = 0; i < ops_per_batch; ++i) {
                    auto const d = q2_local.should_flush(static_cast<std::size_t>(i & 0x3FFu), std::size_t{1024});
                    sink += static_cast<std::uint64_t>(d);
                    q2_local.on_flush_complete();
                }
                t1 = clock::now();
                acc[18] += seg_ns(t0, t1);
            };

            do_seg19(); // Warmup (verworfen)
            for (auto& a : acc) a = 0;
            // P-MD3 (Coverage-Versöhnung, Pfad A analog Pfad B): äußere Wall-Clock um die gemessenen Batches → der
            // kommensurable Nenner; seg_framework_ns = run_total − Σseg_ns (Loop-/Instrumentierungs-Rest).
            clock::time_point const run_t0 = clock::now();
            for (std::uint64_t b = 0; b < batches; ++b) do_seg19();
            clock::time_point const run_t1 = clock::now();
            alloc.deallocate(lbuf, kLbufBytes, 64);

            std::int64_t total = 0;
            for (int i = 0; i < 19; ++i) {
                out->seg_ns[i] = acc[i];
                total += acc[i];
            }
            // sink-Nutzung gegen Wegoptimierung (verfälscht total NICHT — nur ein 1-bit-XOR im unmöglichen Fall).
            if (sink == ~0ull) out->seg_ns[0] ^= 1;
            out->total_ns                = total;
            out->batches_measured        = batches;
            std::int64_t const run_total = seg_ns(run_t0, run_t1);
            out->seg_run_total_ns        = run_total;
            out->seg_framework_ns        = (run_total > total) ? (run_total - total) : 0;
            return batches;
        } catch (...) {
            *out = ComdareSegmentLatencyV2{};
            return 0;
        }
    }
#endif // COMDARE_MEASUREMENT_ON (run_workload / Pfad A)

    // ─────────────────────────────────────────────────────────────────────
    // IDriveableTier-Override (V5-I2.2): funktionaler Gattungs-Antrieb — IMMER einkompiliert (auch Release-DLL).
    // Treibt den EINEN konstitutiven container_algorithm_-Zustand (gemeinsamer uint64-Key nach Umstufung-B). GETRENNT
    // vom Pfad-A-`run_workload` (Messung-AN-only); bei MESSUNG-AN zieht tier_observe zusaetzlich observer_all (Doku 24 §8.6).
    // ─────────────────────────────────────────────────────────────────────

    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
        constexpr auto cap = ::comdare::cache_engine::lookup::composable::capacity_constraint_of<SearchAlgo>();
        if constexpr (cap.kind == ::comdare::cache_engine::lookup::composable::CapacityKind::Static &&
                      cap.max_size != 0) {
            if (key >= cap.max_size) return false; // #217-2a: ehrliche Kapazitaets-Ablehnung (kein stiller Slot-Wrap)
        }
#if COMDARE_MEASUREMENT_ON
        // Copy-on-Write-Memento (#133 Rev. 2): in der Warmup-Phase (save→rollback) materialisiert die ERSTE
        // mutierende Op die Vollkopie des save-Stands — VOR der Mutation. Read-Ops materialisieren NIE
        // (O(1)-Periode). Empirisch begründet (Smoke 2026-06-11): die frühere Einzel-Key-Organ-Inverse war
        // auf Rebuild-Strukturen (k_ary/SortedBinary, container_algorithm_ IMMER) teurer als die memcpy-Vollkopie.
        if (cow_armed_) cow_materialize_copy_();
#endif
        // Neu-Flag = container_algorithm_.insert-bool (insert_or_assign-Semantik; KEIN interner lookup — der wuerde die
        // lookup_count-Observer-Statistik verfaelschen; das fruehere occupied_count-Delta entfiel mit dem Spiegel).
        // #188-4c-iii (2026-07-02): EIN Speicher, konstitutiv. container_algorithm_ ist die einzige T0-Datenquelle
        // (flacher Store mit Such-Traversal, native Organ-Huelle oder bereits observable SearchAlgo-Huelle).
        bool const m8_new_flag = container_algorithm_.insert(key, value);
#if COMDARE_MEASUREMENT_ON
        // ── K10-PMAJOR-04 (2026-06-18): ALLE Observer-feeding Auto-Kopplungen NUR im Mess-Build ──────────────
        // Diese Kopplungen (telemetry/ct/map/q1/q2/cc/pf/pc/flt/vh) speisen AUSSCHLIESSLICH den Observer
        // (fill_observer_v3, selbst #if COMDARE_MEASUREMENT_ON) — kein funktionaler Beitrag zum Daten-Pfad
        // (container_algorithm_ oben ist unkonditional). In der funktional-only DLL (Messung-AUS) waren sie
        // reiner Overhead. Symmetrisch zu tier_lookup + dem bereits geklammerten CoW-Block oben. Mess-Build
        // identisch (kein M3-Effekt: derselbe Code lief vorher, nur jetzt explizit gegated).
        // V42 L-74c Cross-ABI-Auto-Kopplung: jeder insert berührt einen Blatt-Knoten → telemetry mit-treiben.
        if constexpr (requires { telemetry_organ_.record_node_touch(true); }) telemetry_organ_.record_node_touch(true);
        // Phase A (2026-06-04) Auto-Kopplung der 4 neu verdrahteten Achsen (Pfad B): ein insert registriert den
        // Eintrag in der cache_traversal-/mapping-Indirektion (T1/T2) und puffert ihn in q1 + befragt die
        // q2-Flush-Policy (T17/T18) → ihre statistics() reflektieren die echte Tier-Op beim Observer-Read (tier_observe).
        if constexpr (requires {
                          ct_organ_.register_entry(typename Composition::cache_traversal::key_type{},
                                                   typename Composition::cache_traversal::value_type{});
                      }) {
            ct_organ_.register_entry(static_cast<typename Composition::cache_traversal::key_type>(key),
                                     static_cast<typename Composition::cache_traversal::value_type>(value));
        }
        if constexpr (requires {
                          map_organ_.register_slot(typename Composition::mapping::slot_index_type{},
                                                   typename Composition::mapping::offset_type{});
                      }) {
            map_organ_.register_slot(static_cast<typename Composition::mapping::slot_index_type>(key),
                                     static_cast<typename Composition::mapping::offset_type>(value));
        }
        if constexpr (requires { queuing_q1_organ_.put(typename Composition::queuing_q1::element_type{}); }) {
            queuing_q1_organ_.put(static_cast<typename Composition::queuing_q1::element_type>(value));
        }
        if constexpr (requires {
                          queuing_q2_organ_.should_flush(std::size_t{}, std::size_t{});
                          queuing_q2_organ_.on_flush_complete();
                      }) {
            (void)queuing_q2_organ_.should_flush(static_cast<std::size_t>(container_algorithm_.occupied_count()),
                                                 std::size_t{1024});
            queuing_q2_organ_.on_flush_complete();
        }
        // Phase B (2026-06-04) Auto-Kopplung T7/T8 (Pfad B): ein insert berührt eine Adresse (key) → die
        // prefetch-Achse reiht sie in die Pfad-Trajektorie ein (PathOriented treibt echten Tracker; None/Hardware
        // = 0-Baseline) inkl. Hot-Path-Hint aus den rohen key-Bytes; die concurrency-Achse exerziert ein echtes
        // Sync-Primitiv-Paar (acquire→release; strategie-distinkt: None=no-op, Blocking=mutex, LockFree=CAS, …).
        cc_organ_.observe_critical_section();
        // K9-Fix (User §4.4 / 2026-06-18): T7 prefetch REAL — ein insert hat den (key,value) real in container_algorithm_
        // gelegt; die prefetch-Achse setzt strategie-distinkt `_mm_prefetch` auf die TATSAECHLICHEN Slot-Adressen
        // des Stores ab (NICHT key-als-Adresse). descent_slot = die Lower-Bound-Position des Keys im real
        // allozierten Store-Backing → echte Traversal-Adresse, kein OOB (descent_slot_for_ klemmt auf slot_count()).
        // #188-4b-b1a: nur wenn container_algorithm_ store-backed ist (organ-backed Familien nach 4b-b1b/#188-4a -> honest-0, kein store()).
        if constexpr (container_algorithm_is_store_backed_) {
            if (container_algorithm_.store().slot_count() != 0)
                pf_organ_.observe_prefetch_descent(container_algorithm_.store(), descent_slot_for_(key));
        }
        // Phase B Abschluss T3 (Pfad B): ein insert ordnet den Schlüssel in den Trie ein → die path_compression-Achse
        // misst die gemeinsame Byte-Prefix-Länge / cut() am ECHTEN ByteWiseKeyPrefix-Organ (PathCompressionNone =
        // ehrliche niedrige Kompressions-Aktivität, Patricia/ByteWise höher). depth=0 = Trie-Wurzel-Abstieg.
        (void)pc_organ_.compress(key, 0u);
        // P5 (#124, 2026-06-04, User §4.3): den REALEN Filter aus dem eingefuegten Key bauen (Build-Phase, NICHT
        // gemessen — gehoert zur Insert/Setup-Phase wie pc_organ_.compress). if-constexpr-geguarded: nur Strategien
        // mit echter Struktur (Bloom/Cuckoo/SuRF/Xor via insert_key) bauen; None-artige bleiben unberuehrt.
        if constexpr (requires { flt_organ_.insert_key(key); }) flt_organ_.insert_key(key);
        // §4.3 (User 2026-06-04): den (key,value) in die REALE value_handle-Slot-Struktur legen (Pool/Version/Chain-
        // Backing, Build-Phase NICHT gemessen — gehoert zur Insert/Setup-Phase wie flt_organ_.insert_key). if-constexpr-
        // geguarded: NUR Nicht-Inline-Strategien (ExternalPool/ImmutableSharedRef/VersionedPointer/ChainRef) tragen
        // store_value (EmptyRealSlot fuer Inline traegt es NICHT → Inline-M3-Pin bleibt EXAKT unveraendert + messneutral).
        if constexpr (requires { vh_organ_.store_value(key, value); }) vh_organ_.store_value(key, value);
        // §4.3 (User 2026-06-04): den Schluessel inkrementell in den MATERIALISIERTEN Patricia-Trie einordnen
        // (Build-Phase, NICHT gemessen — gehoert zur Insert/Setup-Phase wie pc_organ_.compress / vh_organ_.store_value).
        // if-constexpr-geguarded: NUR Patricia (PatriciaTrie traegt insert_key) baut den Trie inkrementell auf;
        // `none` (M3-Pin, EmptyPatriciaTrie ohne insert_key) bleibt EXAKT No-Op — kein Trie, kein Build-Effekt.
        if constexpr (requires { pc_organ_.insert_key(key); }) pc_organ_.insert_key(key);
#endif // COMDARE_MEASUREMENT_ON (K10-PMAJOR-04: Observer-feeding Auto-Kopplungen tier_insert)
        return m8_new_flag;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        // #188-4c-iii (2026-07-02): lookup liest immer aus dem einzigen konstitutiven container_algorithm_-Zustand.
        auto const cv     = container_algorithm_.lookup(key);
        bool const m8_hit = cv.has_value();
        if (m8_hit && out_value != nullptr) *out_value = static_cast<std::uint64_t>(*cv);
#if COMDARE_MEASUREMENT_ON
        // ── K10-PMAJOR-04 (2026-06-18): ALLE Observer-feeding Auto-Kopplungen NUR im Mess-Build (s. tier_insert) ──
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
        if constexpr (requires { queuing_q1_organ_.get(); }) { (void)queuing_q1_organ_.get(); }
        // Phase B Auto-Kopplung T8 (Pfad B): ein lookup exerziert das echte Sync-Primitiv-Paar. cc_organ_ mutable.
        cc_organ_.observe_critical_section();
        // K9-Fix (User §4.4 / 2026-06-18): T7 prefetch REAL — der lookup folgt einem Such-Descent über das ECHTE
        // container_algorithm_-Slot-Backing; die prefetch-Achse setzt strategie-distinkt `_mm_prefetch` auf die TATSAECHLICHEN
        // Slot-Adressen ab (NICHT mehr key-als-Adresse). `descent_slot` = der real beruehrte Slot des Descents
        // (Lower-Bound-Position des Keys im store, geklemmt auf slot_count()) → echte Traversal-Adresse, kein OOB.
        if constexpr (
            container_algorithm_is_store_backed_) { // #188-4b-b1a: organ-backed nach 4b-b1b/#188-4a (kein store()) -> honest-0 prefetch
            if constexpr (::comdare::cache_engine::lookup::composable::StoreTraversableSearchAlgo<SearchAlgo>) {
                pf_organ_.observe_prefetch_descent(container_algorithm_.store(), descent_slot_for_(key));
            } else {
                // Nicht-store-traversierbare flache Fallbacks tragen trotzdem die realen Storage-Achsen-Records im
                // container_algorithm_. Existiert mind. ein realer Slot, prefetcht der Descent dessen Backing.
                if (container_algorithm_.store().slot_count() != 0)
                    pf_organ_.observe_prefetch_descent(container_algorithm_.store(), descent_slot_for_(key));
            }
        }
        // Phase B Abschluss T3 (Pfad B): ein lookup folgt einem komprimierten Trie-Pfad → die path_compression-Achse
        // misst die gemeinsame Byte-Prefix-Länge des gesuchten Schlüssels am ECHTEN ByteWiseKeyPrefix-Organ. pc_organ_
        // mutable (Tracking im const lookup nicht-const). depth=0 = Trie-Wurzel.
        (void)pc_organ_.compress(key, 0u);
#endif // COMDARE_MEASUREMENT_ON (K10-PMAJOR-04: Observer-feeding Auto-Kopplungen tier_lookup)
        return m8_hit;
    }

    [[nodiscard]] bool tier_erase(std::uint64_t key) noexcept override {
#if COMDARE_MEASUREMENT_ON
        if (cow_armed_) cow_materialize_copy_(); // CoW (#133 Rev. 2): Vollkopie VOR der Mutation (s. tier_insert)
#endif
        // #188-4c-iii (2026-07-02): erase mutiert nur den einzigen konstitutiven container_algorithm_-Zustand.
        return container_algorithm_.erase(key);
    }

    void tier_clear() noexcept override {
#if COMDARE_MEASUREMENT_ON
        // CoW (#133 Rev. 2): clear ist eine Mutation → Vollkopie VOR dem Leeren materialisieren. Greift nur in
        // der Warmup-Phase (cow_armed_); das tier_clear des Mess-Protokolls (zwischen Profilen) ist unberuehrt.
        if (cow_armed_) cow_materialize_copy_();
#endif
        container_algorithm_.clear();
        // P4 (#123): die kalte 2. Ebene mit-leeren — nach tier_clear ist der GANZE Tier-Zustand (heiss + kalt) frisch
        // (sonst akkumulierten migrierte Records ueber Mess-Profile hinweg). Fuer None nie befuellt → no-op-aequivalent.
        container_algorithm_tier1_.clear();
#if COMDARE_MEASUREMENT_ON
        // ── K10-PMAJOR-04 (#166, 2026-06-18): ALLE Observer-feeding Auto-Kopplungen NUR im Mess-Build ───────────
        // Symmetrisch zu tier_insert/tier_lookup. Der DATEN-Pfad oben (container_algorithm_/container_algorithm_tier1_)
        // bleibt unkonditional; ALLES darunter leert/resettet AUSSCHLIESSLICH die Observer-Organe (flt/vh/pc-DATEN
        // + search/ct/map/q1/q2/telemetry/pf/cc-STATISTIK), die in der funktional-only-DLL (Messung-AUS) NIE befuellt
        // werden (ihre Treib-Kopplungen in tier_insert/lookup sind selbst #if-gegated). Mess-Build identisch (kein
        // M3-Effekt: derselbe Code lief vorher, nur jetzt explizit gegated — Observer-Felder pro Messung weiterhin frisch).
        // P5 (#124, R1): den REALEN Filter mit-leeren — symmetrisch zum Daten-clear (sonst truege der Filter Keys
        // ueber Mess-Profile hinweg → Membership-Inkonsistenz). if-constexpr: None-artige ohne clear() = no-op.
        if constexpr (requires { flt_organ_.clear_filter(); }) flt_organ_.clear_filter();
        // §4.3 (User 2026-06-04, R1): die REALE value_handle-Slot-Struktur mit-leeren — symmetrisch zum Daten-clear
        // (sonst truege das Pool/Chain-Backing Keys ueber Mess-Profile hinweg → Deref-Inkonsistenz). if-constexpr:
        // Inline (EmptyRealSlot ohne real Pool/Chain) traegt clear() als no-op — der requires greift, 0-Footprint bleibt.
        if constexpr (requires { vh_organ_.clear_slots(); }) vh_organ_.clear_slots();
        // §4.3 (User 2026-06-04, R1): den MATERIALISIERTEN Patricia-Trie mit-leeren — symmetrisch zum Daten-clear
        // (sonst truege der Trie Keys ueber Mess-Profile hinweg → Descent-Inkonsistenz). if-constexpr: `none`
        // (EmptyPatriciaTrie ohne clear_trie) bleibt unberuehrt → M3-Pin exakt No-Op. Patricia: clear() leert nodes_.
        if constexpr (requires { pc_organ_.clear_trie(); }) pc_organ_.clear_trie();
        // Phase A: Datenpuffer der auto-gekoppelten Instanz-Achsen mit-leeren; die zugehörigen stats_ werden
        // separat in reset_axis_statistics_() genullt, damit tier_reset_statistics() daten-erhaltend bleiben kann.
        if constexpr (requires { ct_organ_.clear(); }) ct_organ_.clear();
        if constexpr (requires { map_organ_.clear(); }) map_organ_.clear();
        if constexpr (requires { queuing_q1_organ_.clear(); }) queuing_q1_organ_.clear();
        // Statistik-Reset separat halten: tier_reset_statistics() nutzt dieselbe Nullung daten-erhaltend nach Load.
        reset_axis_statistics_();
#endif // COMDARE_MEASUREMENT_ON (#166 K10-PMAJOR-04: Observer-feeding Auto-Kopplungen tier_clear)
    }

    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        // #188-4c-iii (2026-07-02): EIN Speicher; container_algorithm_ haelt fuer alle Tiere jeden Insert und liefert den Fuellstand.
        return static_cast<std::uint64_t>(container_algorithm_.occupied_count());
    }

#if COMDARE_MEASUREMENT_ON // V5-I2.2: tier_observe (observer_all) NUR bei Messung-AN
    // ─────────────────────────────────────────────────────────────────────
    // IObservableTier / Observer (MESSUNG-AN) — liegt physisch im IDriveableTier-Block: fill_observer_v3 +
    // fill_segment_timing_v3 + tier_observe (Q1-Sequenz: Observer-READ → Pfad-B-Timing → per-op-Organ-Reset).
    // ─────────────────────────────────────────────────────────────────────
    // I1 (2026-06-05): der frühere V1-Observe + die V2-Observer-Fill/Override-Methoden (eigenes V2-Sub-Interface)
    // sind ENTFERNT. Ihre Felder sind im konsolidierten POD subsumiert (V1 search→axis_stats[0], alloc→[6]; V2
    // telemetry→[10], layout→[5], serialization→[9], node_type→[4]). Rationale: docs/architecture/31_observer_interface_konsolidierung_i1.md.

    // I1: fill_observer_v3 — schreibt die Per-Achsen-Observer DIREKT in den konsolidierten POD (out->axis_stats[T][f]
    // + Meta; Reihenfolge JE Achse = kV3AxisSchema, single-source observable_tier.hpp). Quellen je Achse: T0 search_algo
    // + T6 allocator + T10 telemetry = getriebene Organe; T4 node_type / T5 memory_layout / T9 serialization / T11..T16
    // = Pfad-B Zustand-Scan über das ECHTE container_algorithm_-Slot-Backing (store_observe_*, idempotenter reset()+scan je Aufruf);
    // T1 cache_traversal / T2 mapping / T3 path_compression / T7 prefetch / T8 concurrency / T17 q1 / T18 q2 = in
    // tier_insert/lookup AUTO-gekoppelte Member-Organe. Honest-0 für Baseline-Strategien (echte 0-Teilfelder).
    // STATISTICS-gegated. Der Aufrufer (tier_observe) hat *out bereits genullt (Q1-Sequenz: vor dem Timing).
    void fill_observer_v3(ComdareTierObserverSnapshot* out) const noexcept {
        if (out == nullptr) return;
        auto& s = *out; // I1: direkt in den konsolidierten POD schreiben (kein V3-Zwischen-POD)
        for (auto& row : s.axis_stats)
            for (auto& v : row) v = 0; // nur Observer-Felder nullen (seg_ns/batches: tier_observe)
        s.observable_axis_count = 0;
        s.tier_fill_level       = 0;
        s.filled_axis_count     = 0;
        std::uint64_t filled    = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // ── T0 search_algo ──────────────────────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<SearchAlgo>) {
            // #188-4c-iii (2026-07-02): T0-Such-Metrik kommt immer aus container_algorithm_. Damit wirken
            // node/layout/allocator-Pfade auf dieselbe Datenquelle, die tier_insert/lookup/erase treiben.
            auto const ss = container_algorithm_.statistics();
            auto*      r  = s.axis_stats[0];
            r[0]          = ss.total_lookup_count;
            r[1]          = ss.total_hit_count;
            r[2]          = ss.total_miss_count;
            r[3]          = ss.total_insert_count;
            r[4]          = ss.total_erase_count;
            r[5]          = ss.peak_occupancy;
            ++filled;
        }
        // ── T1 cache_traversal (Phase A neu) ────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) {
            auto const ct = ct_organ_.statistics();
            auto*      r  = s.axis_stats[1];
            r[0]          = ct.total_resolve_count;
            r[1]          = ct.total_resolve_hit_count;
            r[2]          = ct.total_resolve_miss_count;
            r[3]          = ct.total_register_count;
            r[4]          = ct.total_unregister_count;
            r[5]          = ct.peak_tracked;
            ++filled;
        }
        // ── T2 mapping (Phase A neu) ────────────────────────────────────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::mapping>) {
            auto const mp = map_organ_.statistics();
            auto*      r  = s.axis_stats[2];
            r[0]          = mp.total_register_count;
            r[1]          = mp.total_resolve_count;
            r[2]          = mp.total_resolve_hit_count;
            r[3]          = mp.total_resolve_miss_count;
            r[4]          = mp.total_reverse_lookup_count;
            r[5]          = mp.peak_mapped;
            ++filled;
        }
        // ── T4 node_type (Pfad-B Zustand-Scan über container_algorithm_, wie der frühere V2-Pfad) ─────────────────────
        if constexpr (ObservableAxis<typename Composition::node_type> &&
                      requires { container_algorithm_.store_observe_node_type(node_organ_); }) {
            node_organ_.reset();
            (void)container_algorithm_.store_observe_node_type(node_organ_);
            auto const nt = node_organ_.statistics();
            auto*      r  = s.axis_stats[4];
            r[0]          = nt.find_count;
            r[1]          = nt.keys_stored;
            r[2]          = nt.queries_run;
            r[3]          = nt.last_checksum;
            ++filled;
        }
        // ── T5 memory_layout (Pfad-B Zustand-Scan über container_algorithm_) ───────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::memory_layout> &&
                      requires { container_algorithm_.store_observe_layout(ml_organ_); }) {
            ml_organ_.reset();
            (void)container_algorithm_.store_observe_layout(ml_organ_);
            auto const ml = ml_organ_.statistics();
            auto*      r  = s.axis_stats[5];
            r[0]          = ml.scan_count;
            r[1]          = ml.records_scanned;
            r[2]          = ml.field_bytes_read;
            r[3]          = ml.cache_lines_touched;
            r[4]          = ml.last_checksum;
            ++filled;
        }
        // ── T6 allocator (dasselbe getriebene container_algorithm_-Allocator-Organ wie der frühere V2-Pfad) ───────────
        // S7-1: nicht mehr an store_type koppeln. Flache Stores und BST-Pool-Organe liefern beide optional
        // store_allocator_statistics(); Familien ohne diese schmale Route bleiben honest-0.
        if constexpr (ObservableAxis<typename Composition::allocator> &&
                      requires { container_algorithm_.store_allocator_statistics(); }) {
            auto const a = container_algorithm_.store_allocator_statistics();
            auto*      r = s.axis_stats[6];
            if constexpr (requires {
                              a.total_bytes_allocated;
                              a.total_bytes_in_use;
                              a.allocation_count;
                              a.deallocation_count;
                              a.failure_count;
                          }) {
                r[0] = a.total_bytes_allocated;
                r[1] = a.total_bytes_in_use;
                r[2] = a.allocation_count;
                r[3] = a.deallocation_count;
                r[4] = a.failure_count;
                ++filled;
            } else if constexpr (requires {
                                     a.bytes_allocated;
                                     a.alloc_calls;
                                 }) {
                r[0] = a.bytes_allocated;
                r[1] = a.bytes_allocated;
                r[2] = a.alloc_calls;
                ++filled;
            }
        }
        // ── T7 prefetch (K9-Fix, User §4.4 / 2026-06-18: REALER _mm_prefetch auf Store-Adressen) ──────────
        //    ObservablePrefetch-Hülle (pf_organ_) IMMER ObservableAxis → das Schema-Slot ist befüllt. Die
        //    Spalten r[0]/r[5..7] tragen jetzt die REALE Prefetch-Telemetrie (kein Schluessel-als-Adresse mehr):
        //    None bleibt 0 (0-Overhead-Baseline), Hardware/Distance setzen 1 _mm_prefetch je Descent (distinkte
        //    last_distance: HW=0, Distance=N), PathOriented mehrere (Bundle). r[1..4] = die alten Tracker-Felder
        //    (jetzt 0, da PathOriented nicht mehr enqueue-getrieben wird → ehrlich, kein Phantom).
        if constexpr (ObservableAxis<decltype(pf_organ_)>) {
            auto const pf = pf_organ_.statistics();
            auto*      r  = s.axis_stats[7];
            r[0]          = pf.trigger_count;
            r[1]          = pf.suggestions_made;
            r[2]          = pf.hot_path_hints;
            r[3]          = pf.max_queue_depth;
            r[4]          = pf.total_addresses_enqueued;
            r[5]          = pf.real_prefetches_issued;
            r[6]          = pf.last_prefetch_distance;
            r[7]          = pf.last_real_address;
            ++filled;
        }
        // ── T8 concurrency (Phase B neu, AUTO-gekoppelt via tier_insert/lookup observe_critical_section) ──
        //    ObservableConcurrency-Hülle (cc_organ_) treibt das echte Sync-Primitiv (acquire/release) + zählt;
        //    contention/validation_failure im single-thread-Pfad ehrlich 0 (s. axis_08_concurrency_observable.hpp).
        if constexpr (ObservableAxis<decltype(cc_organ_)>) {
            auto const cc = cc_organ_.statistics();
            auto*      r  = s.axis_stats[8];
            r[0]          = cc.acquire_count;
            r[1]          = cc.release_count;
            r[2]          = cc.contention_count;
            r[3]          = cc.validation_failure_count;
            r[4]          = cc.pattern_id;
            ++filled;
        }
        // ── T9 serialization (Pfad-B Zustand-Scan über container_algorithm_) ───────────────────────────────────────
        if constexpr (ObservableAxis<typename Composition::serialization> &&
                      requires { container_algorithm_.store_observe_serialization(ser_organ_); }) {
            ser_organ_.reset();
            (void)container_algorithm_.store_observe_serialization(ser_organ_);
            auto const sr = ser_organ_.statistics();
            auto*      r  = s.axis_stats[9];
            r[0]          = sr.serialize_count;
            r[1]          = sr.records_serialized;
            r[2]          = sr.bytes_serialized;
            r[3]          = sr.last_checksum;
            ++filled;
        }
        // ── T10 telemetry (AUTO-gekoppelt via tier_insert/lookup, wie der frühere V2-Pfad) ──────────────────
        if constexpr (ObservableAxis<typename Composition::telemetry>) {
            auto const t = telemetry_organ_.statistics();
            auto*      r = s.axis_stats[10];
            r[0]         = t.total_events;
            r[1]         = t.leaf_updates;
            r[2]         = t.node_updates;
            r[3]         = t.peak_tracked;
            ++filled;
        }
        // ── T11 value_handle (Phase B neu, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ──────────────
        //    ECHTER Weg (Spec §3): vh_organ_ (ObservableValueHandle-Hülle) treibt value_access_scan über die
        //    REAL gespeicherten Slots (store_observe_value_handle → NodeChunkedStore::organ_observe_value_handle),
        //    KEINE flache Roh-Puffer-Simulation. idempotenter reset()+scan je Observe (wie ser/ml).
        if constexpr (requires { container_algorithm_.store_observe_value_handle(vh_organ_); }) {
            vh_organ_.reset();
            (void)container_algorithm_.store_observe_value_handle(vh_organ_);
            auto const vh = vh_organ_.statistics();
            auto*      r  = s.axis_stats[11];
            r[0]          = vh.total_access_count;
            r[1]          = vh.indirect_deref_count;
            r[2]          = vh.version_tag_strips;
            r[3]          = vh.peak_chain_depth;
            ++filled;
        }
        // ── T12 isa (Phase B neu, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ───────────────────────
        //    isa_organ_ (ObservableIsa-Hülle) treibt simd_field_sum über die REAL gespeicherten Slot-Bytes als
        //    32-bit-Wort-Strom (store_observe_isa → NodeChunkedStore::organ_observe_isa). idempotenter reset()+scan.
        if constexpr (requires { container_algorithm_.store_observe_isa(isa_organ_); }) {
            isa_organ_.reset();
            (void)container_algorithm_.store_observe_isa(isa_organ_);
            auto const is = isa_organ_.statistics();
            auto*      r  = s.axis_stats[12];
            r[0]          = is.simd_calls;
            r[1]          = is.elements_processed;
            r[2]          = is.simd_iterations;
            r[3]          = is.scalar_fallback_count;
            r[4]          = is.last_checksum;
            ++filled;
        }
        // ── T3 path_compression (Phase B Abschluss, AUTO-gekoppelt via tier_insert/lookup compress) ────────
        //    pc_organ_ (ObservablePathCompression-Hülle) treibt das ECHTE ByteWiseKeyPrefix-Organ je Tier-Op; bei
        //    PathCompressionNone ehrlich niedrige Kompressions-Aktivität (kein Sonderfall), bei Patricia/ByteWise höher.
        if constexpr (requires { pc_organ_.statistics(); }) {
            auto const pc = pc_organ_.statistics();
            auto*      r  = s.axis_stats[3];
            r[0]          = pc.compress_calls;
            r[1]          = pc.prefix_len_total;
            r[2]          = pc.bytes_saved_total;
            r[3]          = pc.cuts_performed;
            r[4]          = pc.last_checksum;
            ++filled;
        }
        // ── T13 index_organization (Phase B Abschluss, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ────
        //    idx_organ_ (ObservableIndexOrg-Hülle) treibt index_org_scan über die REAL gespeicherten Slots
        //    (store_observe_index_org → NodeChunkedStore::organ_observe_index_org). predicate_evals/indirect_lookups
        //    folgen den static-deklarierten Strategie-Eigenschaften (is_clustered/has_secondary_indexes). reset()+scan.
        if constexpr (requires { container_algorithm_.store_observe_index_org(idx_organ_); }) {
            idx_organ_.reset();
            (void)container_algorithm_.store_observe_index_org(idx_organ_);
            auto const ix = idx_organ_.statistics();
            auto*      r  = s.axis_stats[13];
            r[0]          = ix.scan_count;
            r[1]          = ix.records_scanned;
            r[2]          = ix.predicate_evals;
            r[3]          = ix.indirect_lookups;
            r[4]          = ix.last_checksum;
            ++filled;
        }
        // ── T14 io_dispatch (Phase B Abschluss, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ───────────
        //    io_organ_ (ObservableIoDispatch-Hülle) treibt io_dispatch_scan über die REAL gespeicherten Slots als
        //    IN-MEMORY-Dispatch (store_observe_io_dispatch → NodeChunkedStore::organ_observe_io_dispatch); KEIN Disk-IO
        //    (Hauptagent-Entscheid). alignment_adjusts honest 0 für InMemoryOnly. reset()+scan.
        if constexpr (requires { container_algorithm_.store_observe_io_dispatch(io_organ_); }) {
            io_organ_.reset();
            (void)container_algorithm_.store_observe_io_dispatch(io_organ_);
            auto const io = io_organ_.statistics();
            auto*      r  = s.axis_stats[14];
            r[0]          = io.dispatch_rounds;
            r[1]          = io.bytes_dispatched;
            r[2]          = io.alignment_adjusts;
            r[3]          = io.total_dispatch_count;
            r[4]          = io.last_checksum;
            ++filled;
        }
        // ── T15 migration_policy (Phase B Abschluss, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ───────
        //    mig_organ_ (ObservableMigration-Hülle) treibt migration_decide_scan über die REAL gespeicherten Slots
        //    (store_observe_migration → NodeChunkedStore::organ_observe_migration). decide-only, KEIN 2. Tier →
        //    tier_moves honest 0; migrations/hot/cold honest 0 für NoMigration (is_active()==false). reset()+scan.
        if constexpr (requires { container_algorithm_.store_observe_migration(mig_organ_); }) {
            mig_organ_.reset();
            (void)container_algorithm_.store_observe_migration(mig_organ_);
            auto const mg = mig_organ_.statistics();
            auto*      r  = s.axis_stats[15];
            r[0]          = mg.total_decisions;
            r[1]          = mg.migrations_triggered;
            r[2]          = mg.hot_votes;
            r[3]          = mg.cold_votes;
            r[4]          = mg.tier_moves;
            ++filled;
        }
        // ── T16 filter (Phase B Abschluss, Pfad-B Zustand-Scan über container_algorithm_-Slot-Backing) ─────────────────
        //    flt_organ_ (ObservableFilter-Hülle) treibt filter_probe_scan über die low-Bytes der REAL gespeicherten
        //    Keys als Query-Strom (store_observe_filter → NodeChunkedStore::organ_observe_filter). REALER In-Memory-
        //    Filter, keine Strategie-Internas (positive/negative je öffentliches Einzel-Query-Ergebnis). reset()+scan.
        if constexpr (requires { container_algorithm_.store_observe_filter(flt_organ_); }) {
            flt_organ_.reset();
            (void)container_algorithm_.store_observe_filter(flt_organ_);
            auto const fl = flt_organ_.statistics();
            auto*      r  = s.axis_stats[16];
            r[0]          = fl.probe_count;
            r[1]          = fl.queries_positive;
            r[2]          = fl.queries_negative;
            r[3]          = fl.hash_probes_total;
            r[4]          = fl.last_checksum;
            ++filled;
        }
        // ── T17 queuing_q1 (Phase A neu, AUTO-gekoppelt via tier_insert/lookup put/get) ──────────────────
        if constexpr (ObservableAxis<typename Composition::queuing_q1>) {
            auto const q = queuing_q1_organ_.statistics();
            auto*      r = s.axis_stats[17];
            r[0]         = q.total_put_count;
            r[1]         = q.total_get_count;
            r[2]         = q.overflow_count;
            r[3]         = q.underflow_count;
            r[4]         = q.peak_size;
            ++filled;
        }
        // ── T18 queuing_q2 (Phase A neu, AUTO-gekoppelt via tier_insert should_flush/on_flush_complete) ──
        if constexpr (ObservableAxis<typename Composition::queuing_q2>) {
            auto const q = queuing_q2_organ_.statistics();
            auto*      r = s.axis_stats[18];
            r[0]         = q.total_decisions_evaluated;
            r[1]         = q.full_flush_count;
            r[2]         = q.partial_flush_count;
            r[3]         = q.no_flush_count;
            r[4]         = q.flush_complete_count;
            ++filled;
        }
#endif // COMDARE_CE_ENABLE_STATISTICS
        s.observable_axis_count = ObserverAggregate<Composition>::observable_count();
        s.tier_fill_level       = tier_size();
        s.filled_axis_count     = filled;
    }

    // I1: die früheren V2/V3/V4-Observer-Override-Methoden (eigene Sub-Interfaces) sind ENTFERNT — die EINE
    // tier_observe(ComdareTierObserverSnapshot*) unten vereint Observer-Stats + Pfad-B-Timing (s. Doc 31).

    // Pfad-B Per-Achsen-TIMING-Kern (Plan v2): zeitet die 19 REALEN per-Achsen-Ops ueber die EINE schon
    // befuellte composite-Tier-Struktur (container_algorithm_ + Instanz-Organe) -- KEIN synthetischer Puffer (Pfad A).
    // Alle Ops lesend/idempotent (lookup/resolve/compress const; store_observe_* reset()+const-scan ueber chunks_);
    // die per-op-getriebenen Zaehler werden NACH dem Timing zurueckgesetzt (der Host zieht V3-Observer ohnehin
    // VOR V4-Timing -> keine Verfaelschung). Memento-neutral: keine Daten-Mutation an container_algorithm_.
    void fill_segment_timing_v3(ComdareSegmentLatencyV2* out) const noexcept {
        if (out == nullptr) return;
        *out = ComdareSegmentLatencyV2{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        try {
            using clock = std::chrono::steady_clock;
            auto dns    = [](clock::time_point a, clock::time_point b) noexcept -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
            };
            // Reale gespeicherte Keys EINMAL beziehen (NICHT gemessen) -- fuer die per-op-Achsen (T0/T1/T2/T3).
            // #188-4c-iii: Keys kommen ausschliesslich aus container_algorithm_. Store-traversierbare flache Container
            // liefern save_state().data; Organ-Huellen nutzen for_each_record, wenn vorhanden. Fehlt ein nativer
            // Walk (z.B. Masstree-{0}), bleibt keys leer und der Default unten nutzt {0}.
            std::vector<std::uint64_t> keys;
            if constexpr (::comdare::cache_engine::lookup::composable::StoreTraversableSearchAlgo<SearchAlgo>) {
                auto snap = container_algorithm_.save_state()
                                .data; // ObservableComposedSearch::save_state -> (key,value)-Liste im Store
                keys.reserve(snap.size());
                for (auto const& kv : snap) keys.push_back(static_cast<std::uint64_t>(kv.first));
            } else if constexpr (requires {
                                     container_algorithm_.for_each_record([](std::uint64_t, std::uint64_t){});
                                 }) {
                keys.reserve(container_algorithm_.occupied_count());
                container_algorithm_.for_each_record([&](std::uint64_t k, std::uint64_t) { keys.push_back(k); });
            }
            std::size_t const nk = keys.empty() ? std::size_t{1} : keys.size();
            if (keys.empty()) keys.push_back(0);
            // #278 (2026-07-06): Mindest-Op-Zahl je Batch gegen Clock-Aufloesung/Framework-Fixkosten. Walk-lose
            // Huellen (Masstree: keys={0} -> nk=1) fuhren 1 Op je Segment -> 9µs-Lauf, framework_share 42%,
            // P-MD3-Abnahme (Coverage > 0.90) physikalisch unerreichbar. Zyklisches Wiederholen derselben Ops
            // (keys[i % nk]) aendert die Mess-Semantik nicht, amortisiert nur die Fixkosten (Batch-weise fuer
            // ALLE Achsen desselben Laufs -> kommensurabel).
            std::uint64_t const     n_ops    = std::max<std::uint64_t>(static_cast<std::uint64_t>(nk), 256u);
            constexpr std::uint64_t kBatches = 8;
            std::int64_t            acc[19]  = {};
            std::uint64_t           sink     = 0;

            auto do_batch = [&]() {
                clock::time_point t0, t1;
                // T0 search_algo: echte Lookups auf dem real befuellten container_algorithm_.
                t0 = clock::now();
                for (std::uint64_t i = 0; i < n_ops; ++i) {
                    auto v = container_algorithm_.lookup(keys[i % nk]);
                    if (v) sink += static_cast<std::uint64_t>(*v);
                }
                t1 = clock::now();
                acc[0] += dns(t0, t1);
                // T1 cache_traversal: resolve über die real registrierten Einträge.
                t0 = clock::now();
                if constexpr (requires { ct_organ_.resolve(typename Composition::cache_traversal::key_type{}); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) {
                        auto v = ct_organ_.resolve(
                            static_cast<typename Composition::cache_traversal::key_type>(keys[i % nk]));
                        if (v) sink += static_cast<std::uint64_t>(*v);
                    }
                }
                t1 = clock::now();
                acc[1] += dns(t0, t1);
                // T2 mapping: resolve_offset über die real registrierten Slots.
                t0 = clock::now();
                if constexpr (requires {
                                  map_organ_.resolve_offset(typename Composition::mapping::slot_index_type{});
                              }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) {
                        auto o = map_organ_.resolve_offset(
                            static_cast<typename Composition::mapping::slot_index_type>(keys[i % nk]));
                        if (o) sink += static_cast<std::uint64_t>(*o);
                    }
                }
                t1 = clock::now();
                acc[2] += dns(t0, t1);
                // T3 path_compression: echte compress(key,0) über die real gespeicherten Keys.
                t0 = clock::now();
                if constexpr (requires { pc_organ_.compress(std::uint64_t{}, 0u); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i)
                        sink += static_cast<std::uint64_t>(pc_organ_.compress(keys[i % nk], 0u));
                }
                t1 = clock::now();
                acc[3] += dns(t0, t1);
                // T4 node_type: store_observe über das ECHTE container_algorithm_-Slot-Backing (chunks_).
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::node_type> &&
                              requires { container_algorithm_.store_observe_node_type(node_organ_); }) {
                    if constexpr (requires { node_organ_.reset(); }) node_organ_.reset();
                    sink += container_algorithm_.store_observe_node_type(node_organ_);
                }
                t1 = clock::now();
                acc[4] += dns(t0, t1);
                // T5 memory_layout: store_observe über chunks_ (layout-honorierender Store → CLA-Stride echt).
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::memory_layout> &&
                              requires { container_algorithm_.store_observe_layout(ml_organ_); }) {
                    if constexpr (requires { ml_organ_.reset(); }) ml_organ_.reset();
                    sink += container_algorithm_.store_observe_layout(ml_organ_);
                }
                t1 = clock::now();
                acc[5] += dns(t0, t1);
                // T6 allocator: O(1)-Stats-Read (Aufbau-Effekt, ehrliche kleine Baseline — kein erfundener Scan).
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_allocator_statistics(); }) {
                    auto a = container_algorithm_.store_allocator_statistics();
                    if constexpr (requires { a.total_bytes_in_use; }) {
                        sink += a.total_bytes_in_use;
                    } else if constexpr (requires { a.bytes_allocated; }) {
                        sink += a.bytes_allocated;
                    }
                }
                t1 = clock::now();
                acc[6] += dns(t0, t1);
                // T7 prefetch: K9-Fix (User §4.4 / 2026-06-18) — REALER _mm_prefetch auf die TATSAECHLICHEN Slot-
                // Adressen des Stores (strategie-distinkt via PrefetchDescentPolicy), NICHT mehr key-als-Adresse.
                // descent_slot_for_(k) = der real beruehrte Slot je Op → reale Traversal-Adresse, kein OOB.
                t0 = clock::now();
                if constexpr (requires {
                                  pf_organ_.observe_prefetch_descent(container_algorithm_.store(), std::size_t{});
                              }) {
                    // #188-4c-iii: voller u64-Key direkt (der fruehere K-Roundtrip truncierte auf die schmale
                    // search_organ_-Key-Breite und war damit INKONSISTENT zum Hot-Path :808/:866, der nie truncierte).
                    if (container_algorithm_.store().slot_count() != 0)
                        for (std::uint64_t i = 0; i < n_ops; ++i)
                            pf_organ_.observe_prefetch_descent(container_algorithm_.store(),
                                                               descent_slot_for_(keys[i % nk]));
                }
                t1 = clock::now();
                acc[7] += dns(t0, t1);
                // T8 concurrency: echtes Sync-Primitiv-Paar (observe_critical_section).
                t0 = clock::now();
                if constexpr (requires { cc_organ_.observe_critical_section(); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) {
                        cc_organ_.observe_critical_section();
                        sink += (i & 1u);
                    }
                }
                t1 = clock::now();
                acc[8] += dns(t0, t1);
                // T9 serialization: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (ObservableAxis<typename Composition::serialization> &&
                              requires { container_algorithm_.store_observe_serialization(ser_organ_); }) {
                    if constexpr (requires { ser_organ_.reset(); }) ser_organ_.reset();
                    sink += container_algorithm_.store_observe_serialization(ser_organ_);
                }
                t1 = clock::now();
                acc[9] += dns(t0, t1);
                // T10 telemetry: echtes record_node_touch.
                t0 = clock::now();
                if constexpr (requires { telemetry_organ_.record_node_touch(true); }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) telemetry_organ_.record_node_touch((i & 1u) != 0u);
                    sink += n_ops;
                }
                t1 = clock::now();
                acc[10] += dns(t0, t1);
                // T11 value_handle: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_value_handle(vh_organ_); }) {
                    vh_organ_.reset();
                    sink += container_algorithm_.store_observe_value_handle(vh_organ_);
                }
                t1 = clock::now();
                acc[11] += dns(t0, t1);
                // T12 isa: store_observe (SIMD-Feld-Reduktion) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_isa(isa_organ_); }) {
                    isa_organ_.reset();
                    sink += container_algorithm_.store_observe_isa(isa_organ_);
                }
                t1 = clock::now();
                acc[12] += dns(t0, t1);
                // T13 index_organization: store_observe über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_index_org(idx_organ_); }) {
                    idx_organ_.reset();
                    sink += container_algorithm_.store_observe_index_org(idx_organ_);
                }
                t1 = clock::now();
                acc[13] += dns(t0, t1);
                // T14 io_dispatch: store_observe (In-Memory-Dispatch) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_io_dispatch(io_organ_); }) {
                    io_organ_.reset();
                    sink += container_algorithm_.store_observe_io_dispatch(io_organ_);
                }
                t1 = clock::now();
                acc[14] += dns(t0, t1);
                // T15 migration_policy: store_observe (decide-only) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_migration(mig_organ_); }) {
                    mig_organ_.reset();
                    sink += container_algorithm_.store_observe_migration(mig_organ_);
                }
                t1 = clock::now();
                acc[15] += dns(t0, t1);
                // T16 filter: store_observe (Probe über Key-Low-Bytes) über chunks_.
                t0 = clock::now();
                if constexpr (requires { container_algorithm_.store_observe_filter(flt_organ_); }) {
                    flt_organ_.reset();
                    sink += container_algorithm_.store_observe_filter(flt_organ_);
                }
                t1 = clock::now();
                acc[16] += dns(t0, t1);
                // T17 queuing_q1: put+get-Paar (bounded → kein Deque-Wachstum über die Batches).
                t0 = clock::now();
                if constexpr (requires {
                                  queuing_q1_organ_.put(typename Composition::queuing_q1::element_type{});
                                  queuing_q1_organ_.get();
                              }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) {
                        queuing_q1_organ_.put(static_cast<typename Composition::queuing_q1::element_type>(i));
                        auto v = queuing_q1_organ_.get();
                        if (v) sink += static_cast<std::uint64_t>(*v);
                    }
                }
                t1 = clock::now();
                acc[17] += dns(t0, t1);
                // T18 queuing_q2: should_flush + on_flush_complete.
                t0 = clock::now();
                if constexpr (requires {
                                  queuing_q2_organ_.should_flush(std::size_t{}, std::size_t{});
                                  queuing_q2_organ_.on_flush_complete();
                              }) {
                    for (std::uint64_t i = 0; i < n_ops; ++i) {
                        sink += static_cast<std::uint64_t>(
                            queuing_q2_organ_.should_flush(static_cast<std::size_t>(i & 0x3FFu), std::size_t{1024}));
                        queuing_q2_organ_.on_flush_complete();
                    }
                }
                t1 = clock::now();
                acc[18] += dns(t0, t1);
            };

            do_batch(); // Warmup (verworfen)
            for (auto& a : acc) a = 0;
            // P-MD3 (Coverage-Versöhnung): ÄUSSERE Wall-Clock um die gemessenen (Nicht-Warmup-)Batches. Sie erfasst
            // ALLES — die 19 Segment-Timer UND den Rest dazwischen (rng, Schleifen-/Branch-/if-constexpr-Overhead,
            // die Lücken zwischen aufeinanderfolgenden clock::now()-Paaren). seg_run_total_ns ist damit der
            // KOMMENSURABLE Nenner für die Coverage des Segment-Laufs; seg_framework_ns der explizite, benannte Rest.
            clock::time_point const run_t0 = clock::now();
            for (std::uint64_t b = 0; b < kBatches; ++b) do_batch();
            clock::time_point const run_t1 = clock::now();

            std::int64_t total = 0;
            for (int i = 0; i < 19; ++i) {
                out->seg_ns[i] = acc[i];
                total += acc[i];
            }
            if (sink == ~0ull) out->seg_ns[0] ^= 1; // sink-Schutz gegen Wegoptimierung (verfälscht total NICHT)
            out->total_ns         = total;
            out->batches_measured = kBatches;
            // P-MD3: Σseg_ns + seg_framework_ns ≡ seg_run_total_ns. seg_framework_ns >= 0 by-construction (die äußere
            // Wall-Clock umschließt alle inneren Segment-Spannen); ein theoretischer Mess-Jitter (innere Summe knapp
            // über äußere) wird auf 0 geklemmt, damit der benannte Rest nie negativ in die Coverage geht.
            std::int64_t const run_total = dns(run_t0, run_t1);
            out->seg_run_total_ns        = run_total;
            out->seg_framework_ns        = (run_total > total) ? (run_total - total) : 0;

            // Per-op-getriebene Zähler nach dem Timing zurücksetzen (memento-/observer-neutral; der Host zieht
            // V3 VOR V4, daher sind die Observer schon erhoben — aber ein defensiver Reset hält die Member sauber).
            if constexpr (requires { ct_organ_.reset(); }) ct_organ_.reset();
            if constexpr (requires { map_organ_.reset(); }) map_organ_.reset();
            if constexpr (requires { pc_organ_.reset(); }) pc_organ_.reset();
            if constexpr (requires { pf_organ_.reset(); }) pf_organ_.reset();
            if constexpr (requires { cc_organ_.reset(); }) cc_organ_.reset();
            if constexpr (requires { telemetry_organ_.reset(); }) telemetry_organ_.reset();
            if constexpr (requires { queuing_q1_organ_.clear(); }) queuing_q1_organ_.clear();
            if constexpr (requires { queuing_q1_organ_.reset(); }) queuing_q1_organ_.reset();
            if constexpr (requires { queuing_q2_organ_.reset(); }) queuing_q2_organ_.reset();
        } catch (...) { *out = ComdareSegmentLatencyV2{}; }
#endif // COMDARE_CE_ENABLE_STATISTICS
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
        // P-MD3: kommensurabler Coverage-Nenner + benannter Rest des Pfad-B-Segment-Laufs in den EINEN POD übertragen.
        out->seg_framework_ns = seg.seg_framework_ns;
        out->seg_run_total_ns = seg.seg_run_total_ns;
#endif // COMDARE_CE_ENABLE_STATISTICS
    }

    void tier_reset_statistics() noexcept override { reset_axis_statistics_(); }
#endif // COMDARE_MEASUREMENT_ON (tier_observe / observer_all)

#if COMDARE_MEASUREMENT_ON
    // ─────────────────────────────────────────────────────────────────────
    // IRollbackableTier-Override (V5-I6): memento_all — In-Memory-Rollback der getriebenen Organe.
    // Der host-seitige Zwei-Phasen-Treiber (I7) triggert: save_all → op (warmup) → rollback_all → op (measure),
    // sodass der GEMESSENE Op gegen DENSELBEN Vor-Zustand läuft (eliminiert Pfad-Abhängigkeit der Latenz).
    //
    // IN-MEMORY-Memento (container_algorithm_ lebt komplett im RAM): eine Tiefkopie IST das vollstaendige,
    // korrekte Memento fuer die RAM-residenten Achsen (search_algo/node_type/memory_layout/allocator-Arena, die
    // alle im ComposedStore liegen). Fuer die DISK-/zustandsreichen Achsen (io_dispatch/serialization/migration),
    // wo "ein einfacher Snapshot NICHT reicht", greift das per-Achsen-Checkpoint via MementoAxis::save_state/
    // restore_state (V5-I8 umgesetzt; s. memento_all unten, das BEVORZUGT ueber den per-Achsen-Memento laeuft).
    //
    // if-constexpr-Kopierbarkeits-Guards: kompiliert fuer JEDE Komposition. Nicht-kopierbare Organe (selten;
    // z.B. Allocator mit OS-Handle) degradieren zu no-op-Rollback -- der Treiber (I7) muss diese via
    // Capability-Probe als Kalt-Messung behandeln.
    // ─────────────────────────────────────────────────────────────────────

    // V5-#44-SUBSTANZ: memento_all laeuft BEVORZUGT ueber den per-Achsen-Memento (MementoAxis::save_state/
    // restore_state der Organe -- der vom /goal geforderte "je stateful Achsen-Interface"); nur Organe OHNE
    // MementoAxis fallen auf die In-Memory-Tiefkopie zurueck. KOSTEN-FIX (2026-06-08, User "Kopie jetzt"):
    // Der COPY-Memento (organ-Vollkopie, O(n)) wird BEVORZUGT vor dem MementoAxis-Pfad
    // (save_state()-Liste + restore_state()-RE-INSERT = O(n^2) je Op bei O(n)-Insert-Organen wie k_ary/sorted/linear).
    //
    // COPY-ON-WRITE-MEMENTO (#133 Rev. 2, 2026-06-11; #188-4c-iii angepasst): save_all ist O(1) --
    // es kapselt NUR das Stat-POD von container_algorithm_ und armt die Periode. Die DATEN-Vollkopie wird LAZY erst von
    // der ERSTEN mutierenden Op der Warmup-Phase materialisiert (tier_insert/tier_erase/tier_clear ->
    // cow_materialize_copy_, VOR der Mutation -> Kopie == save-Stand exakt). rollback_all spielt die
    // materialisierte Kopie zurueck (Read-only-Perioden: nichts zu tun) und restauriert das Stat-POD -> Daten
    // UND Observer-Zaehler exakt auf dem save-Stand (zwei_phase-Vertrag tier_observe_trace_abi.hpp).
    // KOSTEN: Read-Perioden O(1); Write-Perioden = EINE Vollkopie. REVISION-HISTORIE: Die Rev.-1-Einzel-Key-
    // Organ-Inverse (Undo-Log) war empirisch LANGSAMER und bleibt verworfen (Doc 33 §1).
    // Organe ohne restore_statistics/Copy (requires-detektiert) -> unveraendert Copy/MementoAxis-Fallback.
    void tier_save_all() noexcept override {
        // P4 (#123, R1): die kalte 2. Ebene IMMER eager mitsichern (beide Pfade) — symmetrisch zum Rollback unten.
        // container_algorithm_tier1_ ist copy-constructible (== container_algorithm_t) → emplace ist exakt; bei OOM verwerfen (Robustheit).
        try {
            saved_container_algorithm_tier1_.emplace(container_algorithm_tier1_);
        } catch (...) { saved_container_algorithm_tier1_.reset(); }
        // P5 (#124, R1): den REALEN Filter IMMER eager mitsichern (beide Pfade) — symmetrisch zum Rollback unten.
        // flt_organ_ ist copy-constructible (std::array-basiert) → emplace ist bit-exakt; bei OOM verwerfen.
        try {
            saved_flt_.emplace(flt_organ_);
        } catch (...) { saved_flt_.reset(); }
        // §4.3 (User 2026-06-04, R1): die REALE value_handle-Slot-Struktur IMMER eager mitsichern (beide Pfade) —
        // symmetrisch zum Rollback unten. vh_organ_ ist copy-constructible (std::vector-/EmptyRealSlot-basiert) →
        // emplace ist bit-exakt; bei OOM verwerfen. Fuer Inline (EmptyRealSlot) ist die Kopie O(1)-leer.
        try {
            saved_vh_.emplace(vh_organ_);
        } catch (...) { saved_vh_.reset(); }
        // §4.3 (User 2026-06-04, R1): den MATERIALISIERTEN Patricia-Trie IMMER eager mitsichern (beide Pfade) —
        // symmetrisch zum Rollback unten. pc_organ_ ist copy-constructible (std::vector-/EmptyPatriciaTrie-basiert)
        // → emplace ist bit-exakt; bei OOM verwerfen. Fuer `none` (EmptyPatriciaTrie) ist die Kopie O(1)-leer.
        try {
            saved_pc_.emplace(pc_organ_);
        } catch (...) { saved_pc_.reset(); }
        if constexpr (cow_capable_) {
            saved_container_algorithm_stats_ =
                container_algorithm_.statistics(); // O(1) POD-Snapshot (container-/T6-Pfad)
            saved_container_algorithm_.reset();
            cow_materialized_ = false;
            cow_session_      = true;
            cow_armed_        = true;
            return;
        }
        try {
            if constexpr (std::is_copy_constructible_v<container_algorithm_t>)
                saved_container_algorithm_.emplace(container_algorithm_);
            else if constexpr (MementoAxis<container_algorithm_t>)
                saved_container_algorithm_m_ = container_algorithm_.save_state();
        } catch (...) { saved_container_algorithm_.reset(); }
    }

    void tier_rollback_all() noexcept override {
        if constexpr (cow_capable_) {
            if (cow_session_) {
                cow_armed_ = false; // Mess-Phase: nicht mehr materialisieren (Mess-Op = REINE Op, 1 bool-Check)
                try {
                    if (cow_materialized_) { // Write-Periode: materialisierte save-Stand-Kopie zurueckspielen
                        if (saved_container_algorithm_) container_algorithm_ = *saved_container_algorithm_;
                    }
                    // P4 (#123, R1): die kalte 2. Ebene + den migration-Zaehler exakt auf den save-Stand zurueck.
                    if (saved_container_algorithm_tier1_)
                        container_algorithm_tier1_ = *saved_container_algorithm_tier1_;
                    // P5 (#124, R1): den REALEN Filter bit-exakt auf den save-Stand zuruecksetzen.
                    if (saved_flt_) flt_organ_ = *saved_flt_;
                    // §4.3 (User 2026-06-04, R1): die REALE value_handle-Slot-Struktur bit-exakt auf den save-Stand.
                    if (saved_vh_) vh_organ_ = *saved_vh_;
                    // §4.3 (User 2026-06-04, R1): den MATERIALISIERTEN Patricia-Trie bit-exakt auf den save-Stand.
                    if (saved_pc_) pc_organ_ = *saved_pc_;
                    mig_organ_.reset(); // tier_moves/Entscheidungs-Zaehler auf 0 (save-Stand: vor jedem Migrate-Op)
                    // Stats restaurieren (Warmup-Op hat Zaehler beruehrt -- auch read-only) -> exakt save-Stand.
                    container_algorithm_.restore_statistics(saved_container_algorithm_stats_);
                } catch (...) {
                    // noexcept-Vertrag: ein Rollback-Fehler darf den Mess-Lauf nicht abreissen.
                }
                return; // idempotent: erneuter Aufruf -> Kopie/Snapshots unveraendert -> derselbe Stand
            }
        }
        try {
            if constexpr (std::is_copy_assignable_v<container_algorithm_t>) {
                if (saved_container_algorithm_) container_algorithm_ = *saved_container_algorithm_;
            } else if constexpr (MementoAxis<container_algorithm_t>)
                container_algorithm_.restore_state(saved_container_algorithm_m_);
            // P4 (#123, R1): kalte 2. Ebene + migration-Zaehler symmetrisch zum save zuruecksetzen (Fallback-Pfad).
            if (saved_container_algorithm_tier1_) container_algorithm_tier1_ = *saved_container_algorithm_tier1_;
            // P5 (#124, R1): REALEN Filter bit-exakt auf den save-Stand zuruecksetzen (Fallback-Pfad, symmetrisch).
            if (saved_flt_) flt_organ_ = *saved_flt_;
            // §4.3 (User 2026-06-04, R1): REALE value_handle-Slot-Struktur bit-exakt auf den save-Stand (Fallback-Pfad).
            if (saved_vh_) vh_organ_ = *saved_vh_;
            // §4.3 (User 2026-06-04, R1): MATERIALISIERTEN Patricia-Trie bit-exakt auf den save-Stand (Fallback-Pfad).
            if (saved_pc_) pc_organ_ = *saved_pc_;
            mig_organ_.reset();
        } catch (...) {
            // noexcept-Vertrag: ein Rollback-Fehler darf den Mess-Lauf nicht abreissen.
        }
    }

    /// Diagnose (#133 Rev. 2, compile-time): läuft das Zwei-Phasen-Memento dieser Komposition über das lazy
    /// Copy-on-Write (save=O(1)-Stat-Snapshots, Daten-Kopie erst bei der ersten Warmup-Mutation) statt der
    /// eager Organ-Vollkopie je Op? Literal-prüfbar im Test (test_cow_memento) — keine ABI-Fläche (statisch).
    [[nodiscard]] static constexpr bool tier_memento_is_copy_on_write() noexcept { return cow_capable_; }

    // -----------------------------------------------------------------------------
    // §3.3-DELEGATIONS-STATUS (Thesis ch3 sec:sota-axes "verteilte interfaces"; TODO-7/#182, 2026-06-25).
    // Die Anatomie = Verdrahtung ZWISCHEN den Organen (ch3 §3.6.3 / ch4 §4.1): viele Achsen reichen anderen
    // Achsen Detail-Interfaces durch (StorageOrgan -> 9x organ_observe_<achse>; T7 konsumiert Store-Adressen).
    // Nach #188-4c-iii routet T0 durch container_algorithm_; es bleiben ZWEI bewusste, by-design bzw. zurueckgestellte
    // Nicht-Delegationen:
    //   (1) T1 cache_traversal + T2 mapping -- eigener register_entry/register_slot-Index PARALLEL zum Store:
    //       SELF-CONTAINED BY DESIGN. Beide Organe modellieren eine ANDERE Entscheidung als das Store-Layout
    //       (Algorithmus<->Cache-Abbildung bzw. Key->Position), nicht Store-Daten -> eigener Zustand.
    //   (2) T15 migration_policy -- decide-scan delegiert an den Store; der reiche 2-Tier-Blockmove feuert NUR
    //       wo eine 2. Ebene existiert (container_algorithm_tier1_); single-tier/NoMigration -> ehrlich 0.
    // -> Damit ist die §3.3-Schaerfung NACHWEISBAR ueberall, wo eine Achse eine delegierbare Detail-Op hat.
    // -----------------------------------------------------------------------------
    [[nodiscard]] static constexpr bool tier_search_routes_through_store() noexcept {
        // #188-4c-iii (2026-07-02): Diagnose-API bleibt stabil; T0 routet immer ueber container_algorithm_.
        return true;
    }

    /// Diagnose für den Zwei-Phasen-Treiber (I7): exakter Rollback, wenn jedes Organ ENTWEDER MementoAxis ODER
    /// kopierbar ist. Sonst muss der Treiber Kalt-Messung wählen (empirische Probe in tier_observe_trace_abi.hpp).
    [[nodiscard]] bool tier_rollback_is_exact() const noexcept {
        constexpr bool cont_ok =
            MementoAxis<container_algorithm_t> ||
            (std::is_copy_constructible_v<container_algorithm_t> && std::is_copy_assignable_v<container_algorithm_t>);
        // P4 (#123, R1): die kalte 2. Ebene wird via std::optional<container_algorithm_t>-Eager-Kopie exakt zurueckgerollt ->
        // exakt gdw. container_algorithm_t kopierbar ist (== dieselbe Bedingung wie cont_ok; container_algorithm_t ist beidseitig kopierbar).
        constexpr bool tier1_ok =
            std::is_copy_constructible_v<container_algorithm_t> && std::is_copy_assignable_v<container_algorithm_t>;
        return cont_ok && tier1_ok;
    }

    // ─────────────────────────────────────────────────────────────────────
    // IScannableTier-Override (V5-#49-E): YCSB-E Range-Scan. Liest ab dem kleinsten gespeicherten Key >= start_key
    // bis zu max_count Records IN KEY-REIHENFOLGE.
    // (#214 / Audit K4 — GoF-Iterator-Organ, Option A) Uniform über container_algorithm_.scan_range: das Traversal-Organ
    // iteriert geordnet ab start_key — O(log n + scan_len) für sortierte Organe, ehrlicher O(n) für LinearScan
    // (unsortiert). Ersetzt das frühere save_state()-O(n)-Kopie + std::sort JE Scan-Op (der K4-Apparat-Defekt, der
    // bei YCSB-E die Scan-Messung dominierte). const + noexcept (Mess-Robustheit: jede interne Störung → 0). Der
    // Scan verändert weder Daten noch Observer-Zähler (reines Lesen des Substrats).
    // ─────────────────────────────────────────────────────────────────────
    [[nodiscard]] std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                                          std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        try {
            // Drei-Wege-Wahrheit: (1) store-traversierbare Array-Familien laufen ueber ihr treues Traversal-Organ,
            // (2) flache Nicht-StoreTraversal-Familien ueber den SortedBinary-Fallback, (3) organ-backed Familien
            // ohne scan_range bleiben ehrlich bei visited=0, bis die jeweilige Familie einen nativen Range-Walk traegt.
            // EIN uniformer Pfad, kein if-constexpr, kein save_state()/std::sort je Op mehr.
            if constexpr (container_algorithm_is_store_backed_) {
                visited = static_cast<std::uint64_t>(
                    container_algorithm_.scan_range(static_cast<typename container_algorithm_t::key_type>(start_key),
                                                    static_cast<std::size_t>(max_count),
                                                    [&sum](typename container_algorithm_t::key_type /*k*/,
                                                           typename container_algorithm_t::value_type v) noexcept {
                                                        sum += static_cast<std::uint64_t>(v);
                                                    }));
            }
            // #188-4b-b1a: organ-backed Familien (container_algorithm_ = natives Organ ohne scan_range) → visited bleibt 0 (honest-0
            // Scan bis 4b-c/D; der reale per-Achsen-Scan der SOTA-Bäume kommt mit der per-Familie-Node-Shape-Achse).
        } catch (...) {
            return 0; // noexcept-Vertrag: interne Störung (z.B. OOM beim Sammeln in LinearScan) → 0 Records
        }
        if (out_checksum != nullptr) *out_checksum += sum;
        return visited;
    }

    // ─────────────────────────────────────────────────────────────────────
    // IMigratableTier-Override (P4, #123): ECHTER 2-Ebenen-Migrations-Schritt — KEINE Simulation mehr.
    // Treibt den realen Block-Move ÜBER den container_algorithm_-Store (store_migrate_step → LayoutAwareChunkedStore::
    // organ_migrate_step): die von mig_organ_.should_migrate_record markierten (kalten) Records wandern aus
    // container_algorithm_ (heisse 1. Ebene) in container_algorithm_tier1_ (kalte 2. Ebene), und container_algorithm_ wird aus den überlebenden
    // Records neu aufgebaut. mig_organ_.add_tier_moves(n) bucht die REAL bewegten Records → tier_moves > 0 bei
    // aktiver Strategie (Observer T15 r[4] zieht es im naechsten tier_observe). NoMigration: das Praedikat liefert
    // nie true → store_migrate_step gibt 0 → add_tier_moves(0) → tier_moves bleibt 0 (None-Pin unveraendert).
    // mig_organ_.reset() VOR dem Move: tier_moves zaehlt je Schritt frisch (keine Akkumulation ueber Schritte
    // hinweg), konsistent zum reset()+scan-Muster der Observer-Achsen. NICHT im Observer-Pfad (fill_observer_v3
    // bleibt idempotent/lesend) — der Move gehoert ausschliesslich in diesen TREIBE-Pfad.
    // ─────────────────────────────────────────────────────────────────────
    /// P4 (#123) Diagnose (KEINE ABI-Flaeche — plain const-Methode wie tier_rollback_is_exact): Fuellstand der KALTEN
    /// 2. Ebene (container_algorithm_tier1_). 0 fuer alle 320 None-Lebewesen (nie befuellt); > 0 nach einem aktiven Migrate-Schritt.
    /// Literal-prueffbar im Test (test_migration_two_tier), erlaubt die Verifikation „tier1 enthaelt die bewegten Records".
    [[nodiscard]] std::uint64_t tier1_fill_level() const noexcept {
        return static_cast<std::uint64_t>(container_algorithm_tier1_.occupied_count());
    }

    /// P5 (#124) Diagnose (KEINE ABI-Flaeche — plain const-Methode): Lesezugriff auf das REALE Filter-Organ
    /// (ObservableFilter, traegt seit P5 die echte aus den Keys gebaute Struktur). Erlaubt dem Test die
    /// Verifikation „Filter REAL befuellt" + „Memento bit-exakt" (test_filter_real_from_keys). flt_organ_ ist
    /// mutable → const-Methode liefert const&.
    [[nodiscard]] auto const& filter_instance() const noexcept { return flt_organ_; }

    /// §4.3 (User 2026-06-04) Diagnose (KEINE ABI-Flaeche — plain const-Methode): Lesezugriff auf das REALE
    /// value_handle-Organ (ObservableValueHandle, traegt seit §4.3 die echte Pool/Version/Chain-Slot-Struktur).
    /// Erlaubt dem Test die Verifikation „Pool/Chain REAL befuellt + dereferenziert" + „Memento bit-exakt" +
    /// „inline unveraendert" (test_value_handle_real). vh_organ_ ist mutable → const-Methode liefert const&.
    [[nodiscard]] auto const& value_handle_instance() const noexcept { return vh_organ_; }

    /// §4.3 (User 2026-06-04) Diagnose (KEINE ABI-Flaeche — plain const-Methode): Lesezugriff auf das path_compression-
    /// Organ (ObservablePathCompression, traegt seit §4.3 bei Patricia den MATERIALISIERTEN crit-bit-Trie; bei `none`
    /// EmptyPatriciaTrie/leer). Erlaubt dem Test die Verifikation „Trie REAL aus Inserts gebaut + echter Descent" +
    /// „Memento bit-exakt" + „none unveraendert/No-Op" (test_patricia_real). pc_organ_ ist mutable → const& aus const.
    [[nodiscard]] auto const& path_compression_instance() const noexcept { return pc_organ_; }

    [[nodiscard]] std::uint64_t tier_migrate_step(std::uint64_t max_moves) noexcept override {
        // CoW (#133 Rev. 2 / R1): ein Migrate-Schritt MUTIERT container_algorithm_ (+ container_algorithm_tier1_) — wie tier_insert/
        // erase/clear MUSS er daher in der Warmup-Phase die CoW-Vollkopie des save-Stands VOR der Mutation
        // materialisieren, sonst rollte tier_rollback_all container_algorithm_ NICHT zurueck (cow_materialized_ blieb false →
        // Memento-Bruch). container_algorithm_tier1_ selbst wird ueber saved_container_algorithm_tier1_ (eager) zurueckgerollt; diese Materialisierung
        // sichert die HEISSE Ebene (container_algorithm_).
        if (cow_armed_) cow_materialize_copy_();
        std::uint64_t moved = 0;
        try {
            if constexpr (requires {
                              container_algorithm_.store_migrate_step(
                                  mig_organ_, container_algorithm_tier1_.store_mut(), max_moves);
                          }) {
                mig_organ_.reset(); // tier_moves je Schritt frisch (analog Observer-reset()+scan)
                moved = container_algorithm_.store_migrate_step(mig_organ_, container_algorithm_tier1_.store_mut(),
                                                                max_moves);
                mig_organ_.add_tier_moves(moved); // ECHTE Buchung der bewegten Records (stats_ privat → diese API)
            }
        } catch (...) {
            return 0; // noexcept-Vertrag: interne Stoerung (z.B. OOM beim Append) → 0 (Mess-Robustheit)
        }
        return moved;
    }
#endif // COMDARE_MEASUREMENT_ON (tier_save_all / tier_rollback_all / memento_all / tier_scan / tier_migrate_step)

private:
#if COMDARE_MEASUREMENT_ON
    void reset_axis_statistics_() noexcept {
        // #188-4c-iii (2026-07-02): container_algorithm_ traegt die T0-Statistik -> je Messung nullen.
        // ObservableComposedContainer::clear() leert NUR Daten, NICHT stats_ -> ohne dies akkumulierten
        // T0-Zaehler ueber Mess-Profile hinweg. requires-guard: flache/AdHoc container_algorithm_t ohne reset()
        // = no-op; store-traversierbare bleiben idempotent (kein M3-Effekt).
        if constexpr (requires { container_algorithm_.reset(); }) container_algorithm_.reset();

        // KORREKTUR (2026-06-04, Defekt-Fix): clear() leert NUR die Daten (entries_/mappings_/Puffer); die
        // statistics()-Zaehler dieser Organe (LinearFanout/DirectPlacement/NoBuffer/LazyFlush) leben in einem
        // SEPARATEN stats_ und werden ausschliesslich von reset() genullt. Ziel: ALLE 19 Achsen-statistics bei 0
        // -> je Messung frisch, konsistent mit dem V1-Delta-Block. Daten-clears bleiben in tier_clear().
        if constexpr (requires { ct_organ_.reset(); }) ct_organ_.reset();                 // T1: stats_ nullen
        if constexpr (requires { map_organ_.reset(); }) map_organ_.reset();               // T2: stats_ nullen
        if constexpr (requires { queuing_q1_organ_.reset(); }) queuing_q1_organ_.reset(); // T17: stats_ nullen
        if constexpr (requires { queuing_q2_organ_.reset(); }) queuing_q2_organ_.reset(); // T18: nur reset()

        // T10 telemetry: ebenfalls ueber tier_insert/lookup auto-gekoppelt und in fill_observer_v3 direkt gelesen.
        if constexpr (requires { telemetry_organ_.reset(); }) telemetry_organ_.reset();
        // Phase B: T7/T8/T3-Mess-Organe zuruecksetzen; Scan-Achsen T13/T14/T15/T16 sind idempotent im Observe.
        if constexpr (requires { pf_organ_.reset(); }) pf_organ_.reset();
        if constexpr (requires { cc_organ_.reset(); }) cc_organ_.reset();
        if constexpr (requires { pc_organ_.reset(); }) pc_organ_.reset();
    }
#endif // COMDARE_MEASUREMENT_ON
    // K9-Fix (User §4.4 / 2026-06-18) + GAP #3 Exaktheits-Dokumentation (P5d-Rest, 2026-06-18):
    // bestimmt den REAL beruehrten Descent-Slot-Index fuer `key` im container_algorithm_-Store — die Position, die ein
    // Lower-Bound-Descent ansteuert (gleiche Descent-Geometrie wie SortedBinaryTraversal::lower_bound_index).
    //
    // le_limitierung (DE): Die zurueckgegebene prefetch-Zieladresse ist IMMER real + liegt im Store-Backing
    //   (Index < slot_count(), auf slot_count()-1 geklemmt; 0 bei leerem Store → kein OOB). Fuer SORTIERTE Stores
    //   (SortedBinary-/order-erhaltende Traversierung) ist sie ORGAN-EXAKT die naechste beruehrte Descent-Position
    //   (Binaer-Lower-Bound). Fuer UNSORTIERTE Stores (append-only/spread, lineare Traversierung) ist sie die naechste
    //   GESCHAETZTE statt organ-exakte Descent-Position — real + in-range, aber nicht zwingend der exakte next-touch.
    // restriction (EN): The returned prefetch target address is ALWAYS real + inside the store backing (index <
    //   slot_count(), clamped to slot_count()-1; 0 for an empty store → no OOB). For SORTED stores it is the
    //   ORGAN-EXACT next-touched descent position (binary lower bound); for UNSORTED stores (append-only/spread)
    //   it is the next ESTIMATED (not organ-exact) descent position — real + in-range, yet not necessarily the
    //   exact next touch.
    [[nodiscard]] std::size_t descent_slot_for_(std::uint64_t key) const noexcept {
        auto const&       st = container_algorithm_.store();
        std::size_t const n  = st.slot_count();
        if (n == 0) return 0;
        // Lower-Bound ueber die realen Store-Keys. SORTIERT → exakte next-touch-Position; UNSORTIERT → in-range Schaetzung.
        std::size_t lo = 0, hi = n;
        while (lo < hi) {
            std::size_t const m = lo + (hi - lo) / 2;
            if (st.key_at(m) < key)
                lo = m + 1;
            else
                hi = m;
        }
        return (lo < n) ? lo : (n - 1);
    }

    using Composition = typename A::composition_t;
    using SearchAlgo  = typename Composition::search_algo;
    // allocator-messender Container (spiegelt builder/AnatomyExecutionContext): ComposedStore<N,L,A> über
    // die Composition-Achsen → dessen Vector-Growth treibt die Allocator-Statistik real.
    // Plan v2 S1 (2026-06-04): layout-honorierender Store — speichert Records am layout-getriebenen eff_stride
    // (CLA 64-B-gepaddet vs aos 16-B-packed) → die memory_layout-Achse ist ECHT, organ_observe_layout OOB-frei,
    // allocator-Bytes layout-abhängig. Drop-in zu NodeChunkedStore (StorageOrgan-Concept), ersetzt es im Mess-Pfad.
    // #188-4c-iii: Such-Strategie UEBER container_algorithm_: store-traversierbare Algos parametrisieren den flachen Store
    // mit ihrem TREUEN Traversal-Organ; Pool-Familien werden zu ObservableComposedContainer<Organ>;
    // Reference-Huellen sind bereits SearchAlgo selbst. Der flache Rest nutzt den SortedBinary-Fallback ebenfalls
    // als einzigen konstitutiven container_algorithm_.
    using container_algorithm_traversal_t =
        std::conditional_t<::comdare::cache_engine::lookup::composable::StoreTraversableSearchAlgo<SearchAlgo>,
                           ::comdare::cache_engine::lookup::composable::traversal_for_search_algo_t<SearchAlgo>,
                           ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::SortedBinaryTraversal>;
    // #188-4c-iii: container_algorithm_ ist native Organ-Huelle, bereits observable SearchAlgo-Huelle oder flacher Store.
    using flat_container_algorithm_t =
        ::comdare::cache_engine::traversal::axis_03a_search_algo::composable::ObservableComposedSearch<
            container_algorithm_traversal_t,
            ::comdare::cache_engine::node::LayoutAwareChunkedStore<
                typename Composition::node_type, typename Composition::memory_layout, typename Composition::allocator>>;
    // Ist SearchAlgo eine organ_for_search_algo-Familie? Dann traegt container_algorithm_ ObservableComposedContainer<Organ>.
    // Ist SearchAlgo bereits ObservableComposedContainer<XOrgan> (Reference-/PaperBinding-Kompositionen), dann ist
    // container_algorithm_t direkt SearchAlgo: kein flacher SortedBinary-Spiegel, kein Double-Wrap. Nur der Rest bleibt flach.
    static constexpr bool pool_family_ =
        !std::is_same_v<::comdare::cache_engine::lookup::composable::organ_for_search_algo_t<SearchAlgo>, void>;
    static constexpr bool organ_hull_ =
        ::comdare::cache_engine::lookup::composable::is_observable_organ_hull_v<SearchAlgo>;
    // LAZY conditional via std::type_identity: der NICHT gewählte Zweig wird nur BENANNT, nicht instanziiert — sonst
    // wäre ObservableComposedContainer<void> (für Nicht-Pools) ill-formed (void::key_type). ::type extrahiert den
    // gewählten Zweig, ohne den anderen zu instanziieren.
    using container_algorithm_t = typename std::conditional_t<
        pool_family_,
        std::type_identity<::comdare::cache_engine::lookup::composable::ObservableComposedContainer<
            ::comdare::cache_engine::lookup::composable::organ_for_search_algo_t<SearchAlgo>>>,
        std::conditional_t<organ_hull_, std::type_identity<SearchAlgo>,
                           std::type_identity<flat_container_algorithm_t>>>::type;
    // #188-4c-iii: T0/lookup/insert/erase laufen fuer alle Kompositionen ueber container_algorithm_; pool_family_ und
    // organ_hull_ bleiben nur fuer die container_algorithm_t-Typwahl.

    // #188-4b-b1a/#188-4c-i: Kapselungs-Praedikat — traegt container_algorithm_t einen FLACHEN Store (store_type)? TRUE fuer
    // ObservableComposedSearch<Traversal,LayoutAwareChunkedStore> (store()/scan_range/store_observe_*/save_state/
    // store_allocator_statistics ALLE praesent); FALSE fuer ObservableComposedContainer<Organ>, egal ob aus
    // organ_for_search_algo oder als bereits gehuellter SearchAlgo. Store-abhaengige Mess-Stellen unten
    // (prefetch-descent, tier_scan sowie T4/T5-Observer) schalten dann auf honest-0; T6 nutzt eine eigene schmale Stats-Route.
    // Golden-320-neutral: deren flache container_algorithm_t erfuellen store_type weiter; der neue organ_hull_-Zweig greift
    // nur fuer Reference-/PaperBinding-Huellen ausserhalb der 320-Registry.
    static constexpr bool container_algorithm_is_store_backed_ =
        requires { typename container_algorithm_t::store_type; };

    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
    // #188-4c-iv (2026-07-02): ContainerAlgorithm = der Container-ALGORITHMUS-Anteil innerhalb des
    // Such-Algorithmus (Speicher-Substrat der T0-Suche). NICHT zu verwechseln mit der Container-GATTUNG
    // (AnatomyGattung::Container mit Tier-Unterklassen Set/Sequence/Adapter/View) - historische #224-Falle;
    // CMD-2 (#252) schluesselt den Container-Anteil messbar auf.
    // Default-konstruiert; container_algorithm_t kapselt flachen Store, native Organ-Huelle oder bereits
    // observable SearchAlgo-Huelle.
    container_algorithm_t container_algorithm_{};
    // P4 (#123, 2026-06-04): die KALTE 2. Ebene des ECHTEN 2-Ebenen-Migrations-Schritts. Identischer container_algorithm_t-Typ
    // (gleicher node/layout/allocator-getriebener Store) — tier_migrate_step bewegt markierte Records aus container_algorithm_
    // (heiss) hierher. Bleibt leer, solange tier_migrate_step nicht gerufen wird (alle 320 None-Lebewesen tasten ihn
    // NIE an). MUSS im Memento (tier_save_all/tier_rollback_all) symmetrisch mitgesichert werden (R1, kritisch).
    container_algorithm_t container_algorithm_tier1_{};
    // V42 L-74c: telemetry-Organ für die Cross-ABI-Auto-Kopplung. Bei tier_insert/lookup wird record_node_touch
    // getrieben (if-constexpr-geschützt: AdHoc-Compositions tragen die nackte Strategie ohne record_node_touch).
    // OHNE {} (Aggregat-Strategie UND Huelle beide default-init-fähig, test_d_v42_probe2). mutable: das Tracking
    // im const tier_lookup ist logisch nicht-const (analog observer-notify-Muster, observable_composed_search.hpp).
    mutable typename Composition::telemetry telemetry_organ_;
    // V42 L-74c scan-Achsen-Auto-Kopplung: memory_layout + serialization Observer-Organe. Im Observer-Fill (fill_observer_v3)
    // ueber das ECHTE Slot-Backing des container_algorithm_ getrieben (Pfad-B Zustand-Scan). mutable (const-Methode).
    mutable typename Composition::memory_layout ml_organ_;
    mutable typename Composition::serialization ser_organ_;
    mutable typename Composition::node_type     node_organ_;
    // Doc 30 §8.0: queuing q1/q2 als reguläre SA-Achsen — in JEDEM Tier-Binary PRÄSENT (mindestens
    // instanziiert, damit das Achsen-Interface uniform vorhanden ist). Default-konstruiert; das Treiben/
    // Observieren (put/get bzw. should_flush in tier_insert/observe) ist ein Folge-Inkrement (Doc 30 §8.2).
    // mutable: ein späteres Mit-Treiben im const tier_lookup/observe bleibt logisch nicht-const (analog
    // telemetry_organ_/ml_organ_). MUSS für die 19-Slot-Composition kompilieren.
    mutable typename Composition::queuing_q1 queuing_q1_organ_;
    mutable typename Composition::queuing_q2 queuing_q2_organ_;
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
    mutable ::comdare::cache_engine::prefetch_axis::ObservablePrefetch<typename Composition::prefetch> pf_organ_{};
    mutable ::comdare::cache_engine::concurrency_axis::ObservableConcurrency<typename Composition::concurrency>
        cc_organ_{};
    // Phase B (2026-06-04): T11 value_handle (ObservableValueHandle-Hülle um die rohe Composition::value_handle-
    // Strategie) + T12 isa (ObservableIsa-Hülle um Composition::isa). Anders als telemetry/q1/q2 trägt die
    // Composition für T11/T12 die NACKTE Strategie (kein statistics()) → die Hülle hält das Mess-Organ. Getrieben
    // NICHT in tier_insert/lookup, sondern als Pfad-B Zustand-Scan über das ECHTE container_algorithm_-Slot-Backing in
    // fill_observer_v3 (store_observe_value_handle/store_observe_isa, idempotenter reset()+scan, analog ml/ser).
    // mutable: der Observe-Scan im const fill_observer_v3 ist logisch nicht-const (analog ml_organ_/ser_organ_).
    mutable ::comdare::cache_engine::value_handle_axis::ObservableValueHandle<typename Composition::value_handle>
                                                                                    vh_organ_{};
    mutable ::comdare::cache_engine::simd::ObservableIsa<typename Composition::isa> isa_organ_{};
    // Phase B (2026-06-04) Abschluss: die 5 verbleibenden Observer-Hüllen als Member-Organe.
    //  • T3 path_compression: Instanz-Driver compress(key,depth), AUTO-gekoppelt in tier_insert/lookup (wie T1/T2).
    //  • T13/T14/T15/T16 (index_org/io_dispatch/migration/filter): scan-Achsen, getrieben als Pfad-B Zustand-Scan über
    //    das ECHTE container_algorithm_-Slot-Backing in fill_observer_v3 (store_observe_*, idempotenter reset()+scan, wie ml/ser/vh).
    // mutable: das Tracking im const tier_lookup / const fill_observer_v3 ist logisch nicht-const (analog telemetry_organ_).
    mutable ::comdare::cache_engine::path_compression::ObservablePathCompression<typename Composition::path_compression>
        pc_organ_{};
    mutable ::comdare::cache_engine::index_organization::ObservableIndexOrg<typename Composition::index_organization>
                                                                                                          idx_organ_{};
    mutable ::comdare::cache_engine::io_dispatch::ObservableIoDispatch<typename Composition::io_dispatch> io_organ_{};
    mutable ::comdare::cache_engine::migration_policy::ObservableMigration<typename Composition::migration_policy>
                                                                                                 mig_organ_{};
    mutable ::comdare::cache_engine::filter_axis::ObservableFilter<typename Composition::filter> flt_organ_{};
    // KF-4/L-MEAS: zuletzt angewandte Resource-Control (IMMER, auch Messung-aus) — vom RuntimeVariableLoop je dyn.
    // Einstellung gesetzt. Reiner Steuer-Zustand (quert die ABI nicht; der Host liest die Wirkung über den Observer).
    ComdareResourceControlV1 applied_rc_{};
#if COMDARE_MEASUREMENT_ON
    // V5-#44 memento_all: Warmup-Vor-Zustand der getriebenen Organe. Lebt IN der Binary, quert die ABI NICHT.
    // PER-ACHSEN-Memento (bevorzugt, MementoAxis): memento_of_t<Organ> = Organ::memento_t (riche (key,value)-
    // Liste + Stats) bzw. EmptyMemento wenn das Organ KEIN MementoAxis ist (dann greift der Kopie-Fallback).
    memento_of_t<container_algorithm_t> saved_container_algorithm_m_{};
    // Kopie-Fallback fuer Organe ohne MementoAxis (std::optional -> leer bis save_all; reset bei Kapsel-OOM).
    // Im CoW-Pfad (#133 Rev. 2) der Traeger der LAZY materialisierten Daten-Vollkopie (Write-/clear-Perioden).
    std::optional<container_algorithm_t> saved_container_algorithm_{};
    // P4 (#123, R1 — KRITISCH): symmetrischer Memento der KALTEN 2. Ebene. container_algorithm_tier1_ ist eine NEUE persistente
    // Struktur → ohne sie hier zu sichern bräche der Zwei-Phasen-Warmup (V5-I6/I7), sobald ein Warmup-Op migrierte
    // (Daten in container_algorithm_tier1_, die rollback_all nicht zuruecknaehme). EAGER (nicht CoW-lazy), weil tier1 zum
    // save-Zeitpunkt fast immer LEER ist (Kopie ~O(1)) und ein Migrations-Op selten gegen-gemessen wird → kein
    // Kosten-Hebel. Wird in BEIDEN Pfaden (CoW + Fallback) gesichert/zurueckgespielt → uniform exakt.
    std::optional<container_algorithm_t> saved_container_algorithm_tier1_{};
    // P5 (#124, R1 — KRITISCH): symmetrischer Memento des REALEN Filters. flt_organ_ traegt seit P5 eine echte,
    // INKREMENTELL in tier_insert befuellte Struktur (Bloom-Bitmap/Cuckoo-Buckets/Xor-/SuRF-Trie). Ohne diesen
    // Snapshot bliebe ein Warmup-Insert nach rollback_all im Filter stehen (Membership-Drift gegen den save-Stand
    // → Probe in der Mess-Phase ueber einen falsch-befuellten Filter). EAGER (nicht CoW-lazy): die Struktur ist
    // klein (KiB, std::array trivially copyable) → Kopie ~O(1), KEIN Kosten-Hebel; bit-exakt via operator==.
    // Wird — analog saved_container_algorithm_tier1_ — in BEIDEN Pfaden (CoW + Fallback) gesichert/zurueckgespielt → uniform exakt.
    using flt_organ_t = ::comdare::cache_engine::filter_axis::ObservableFilter<typename Composition::filter>;
    std::optional<flt_organ_t> saved_flt_{};
    // §4.3 (User 2026-06-04, R1 — KRITISCH): symmetrischer Memento der REALEN value_handle-Slot-Struktur. vh_organ_
    // traegt seit §4.3 eine echte, INKREMENTELL in tier_insert via store_value befuellte Pool/Version/Chain-Struktur
    // (Nicht-Inline) bzw. EmptyRealSlot (Inline, leer/messneutral). Ohne diesen Snapshot bliebe ein Warmup-store_value
    // nach rollback_all im Slot-Backing stehen (Deref-Drift gegen den save-Stand). EAGER (nicht CoW-lazy): die Struktur
    // ist klein (std::vector, ~KiB) → Kopie ~O(1), KEIN Kosten-Hebel; bit-exakt via operator==. In BEIDEN Pfaden
    // (CoW + Fallback) gesichert/zurueckgespielt → uniform exakt (analog saved_flt_/saved_container_algorithm_tier1_).
    using vh_organ_t =
        ::comdare::cache_engine::value_handle_axis::ObservableValueHandle<typename Composition::value_handle>;
    std::optional<vh_organ_t> saved_vh_{};
    // §4.3 (User 2026-06-04, R1 — KRITISCH): symmetrischer Memento des MATERIALISIERTEN Patricia-Trie. pc_organ_
    // traegt seit §4.3 (Patricia) einen echten, INKREMENTELL in tier_insert via insert_key aufgebauten crit-bit-Trie
    // (bei `none`: EmptyPatriciaTrie, leer/messneutral — M3-Pin). Ohne diesen Snapshot bliebe ein Warmup-insert_key
    // nach rollback_all im Trie stehen (Descent-Drift gegen den save-Stand). EAGER (nicht CoW-lazy): die Struktur ist
    // klein (std::vector aus crit-bit-Knoten) → Kopie ~O(n_keys), KEIN dominanter Kosten-Hebel; bit-exakt via
    // operator==. In BEIDEN Pfaden (CoW + Fallback) gesichert/zurueckgespielt → uniform exakt (analog saved_vh_/saved_flt_).
    using pc_organ_t =
        ::comdare::cache_engine::path_compression::ObservablePathCompression<typename Composition::path_compression>;
    std::optional<pc_organ_t> saved_pc_{};

    // -- Copy-on-Write-Memento (#133 Rev. 2; #188-4c-iii) -- save O(1), Daten-Kopie lazy bei der ersten Warmup-Mutation --
    // Faehigkeits-Detektion (requires): container_algorithm_ traegt den O(1)-Stat-Snapshot/-Restore (statistics()/
    // restore_statistics(), ObservableComposedSearch/-Container); Copy-Konstruierbarkeit fuer die lazy Kopie.
    template <class S>
    static constexpr bool organ_cow_capable_v = requires(S& s, S const& cs) { s.restore_statistics(cs.statistics()); };
    static constexpr bool cow_capable_        = organ_cow_capable_v<container_algorithm_t> &&
                                                std::is_copy_constructible_v<container_algorithm_t> &&
                                                std::is_copy_assignable_v<container_algorithm_t>;

    /// Stat-POD-Typ eines CoW-fähigen Organs (sonst leerer Platzhalter — Member existiert immer).
    struct EmptyStatsSnap {};
    template <class S, bool Cap = organ_cow_capable_v<S>>
    struct OrganStatsSnap {
        using type = EmptyStatsSnap;
    };
    template <class S>
    struct OrganStatsSnap<S, true> {
        using type = decltype(std::declval<S const&>().statistics());
    };

    /// Copy-on-Write: die Daten-Vollkopie des save-Stands EINMAL je Periode materialisieren -- gerufen von
    /// tier_insert/tier_erase/tier_clear VOR ihrer Mutation (nur Warmup-Phase, cow_armed_). Read-Perioden
    /// materialisieren nie (deren save+rollback bleiben reine O(1)-POD-Kopien -- der grosse Kosten-Hebel).
    void cow_materialize_copy_() noexcept {
        if (cow_materialized_) return; // Periode bereits materialisiert (z.B. RMW: insert nach lookup)
        try {
            saved_container_algorithm_.emplace(container_algorithm_);
            cow_materialized_ = true;
        } catch (...) {
            saved_container_algorithm_.reset();
            // Materialisierung gescheitert (OOM): Rollback dieser Periode degradiert (Status-quo-aequivalente
            // Mess-Robustheit; die empirische rb_exact-Probe deckt strukturelle Faelle je Binary ab).
        }
    }

    typename OrganStatsSnap<container_algorithm_t>::type
         saved_container_algorithm_stats_{}; // O(1)-Stat-Snapshot (container/T6-Pfad)
    bool cow_session_      = false;          // save_all lief über den CoW-Pfad (Routing in rollback_all)
    bool cow_armed_        = false;          // Materialisierung AN (nur save→rollback; Mess-Op = 1 bool-Check)
    bool cow_materialized_ = false;          // diese Periode hat die Daten-Vollkopie materialisiert (Write/clear)
#endif
};

} // namespace comdare::cache_engine::anatomy
