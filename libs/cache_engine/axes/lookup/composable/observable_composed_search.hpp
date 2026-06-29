#pragma once
// V41 Saeule-2 (Doku 24 §2.2/§5.3/§5.4) — ObservableAxis-Wrapper um ComposedSearch.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @saeule 2 (Per-Achsen-Observer)
//
// **Zweck:** Macht das komponierbare Such-Modell (ComposedSearch ⊕ StorageOrgan) zu einer ObservableAxis
// (observer_aggregate.hpp:40-44), OHNE den StorageOrgan/TraversalOrgan-Kernvertrag oder ComposedSearch
// zu aendern (storage_organ_concept.hpp:13-16: Observable ist bewusst NICHT im Kernvertrag). Damit kann
// der CacheEngineBuilder (AnatomyExecutionContext) einen GETRIEBENEN uint64-Container halten, dessen
// statistics() echte Per-Achsen-Werte liefert → observe_all() ist nicht mehr NULL (Doku 24 §5.2-Luecke).
//
// Statistik-Tracking exakt nach Array256SearchAlgo-Praezedenz (axis_03a_search_algo_array256.hpp:124-138):
// snapshot_t = SearchAlgoStatistics, MeasurableObserver, alles unter #ifdef COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: nackter Pass-Through (0 Footprint, ObservableAxis<...> = false → EmptyAxisSnapshot-Fallback).

#include "composable_search.hpp"
#include "../concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp" // SearchAlgoStatistics
#include <measurement/measurable_concept.hpp>                                    // MeasurableObserver

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

namespace ce_concepts = ::comdare::cache_engine::lookup::concepts;

/// ObservableAxis-Wrapper: ein ComposedSearch<Traversal,Store> mit Per-Achsen-Statistik (gegated).
/// Container-Schnittstelle (insert/lookup/erase/clear/occupied_count) bleibt std::map-aequivalent.
template <class Traversal, class Store>
    requires TraversalOrgan<Traversal, Store>
class ObservableComposedSearch {
public:
    using key_type   = typename Store::key_type; // == std::uint64_t (StorageOrgan-Invariante)
    using value_type = typename Store::value_type;

    /// insert mit rekonstruiertem inserted-Flag (ComposedSearch::insert ist void) — insert_or_assign-Semantik.
    bool insert(key_type k, value_type v) {
        bool const is_new = !search_.lookup(k).has_value();
        search_.insert(k, v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (search_.occupied_count() > stats_.peak_occupancy) stats_.peak_occupancy = search_.occupied_count();
        // (Audit P5/P8 · E-Welle-A2 Inkr. A2.1) observer_.notify aus dem Mess-Hot-Pfad ENTFERNT: der std::function-
        // Push-Branch je gemessener Op ist über die extern-C-ABI nie mit einem Subscriber besetzt (toter Hot-Pfad-Zweig);
        // der Transport ist PULL via statistics(). observer()/on_event bleiben für die MeasurableComponent-Concept-API erhalten.
#endif
        return is_new;
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        auto const r = search_.lookup(k);
        if (r)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count; // (A2.1) kein observer_.notify im Hot-Pfad
        return r;
#else
        return search_.lookup(k);
#endif
    }

    bool erase(key_type k) {
        bool const ok = search_.erase(k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (ok) ++stats_.total_erase_count; // (A2.1) kein observer_.notify im Hot-Pfad
#endif
        return ok;
    }

    // clear() leert NUR den Container, NICHT die Statistik (Memory-Regel: reset() = Statistik-Reset).
    void                      clear() noexcept { search_.clear(); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return search_.occupied_count(); }

    /// GoF-Iterator (YCSB-E #214): reicht den geordneten Range-Scan an das innere ComposedSearch durch (O(log n +
    /// scan_len) fuer sortierte Traversal-Organe, ehrlicher O(n) fuer LinearScan). const + KEIN Statistik-Touch
    /// (reines Lesen des Substrats — der Scan veraendert weder Daten noch Observer-Zaehler). `sink` aufrufbar mit
    /// (key,value); Rueckgabe = Anzahl in Key-Reihenfolge besuchter Records (<= max_count).
    template <class Sink>
    [[nodiscard]] std::size_t scan_range(key_type start_key, std::size_t max_count, Sink&& sink) const {
        return search_.scan_range(start_key, max_count, std::forward<Sink>(sink));
    }

    using store_type = Store; // fuer den optionalen Allocator-Statistik-Durchgriff (Saeule-2)

    // P4 (#123, 2026-06-04): ECHTER 2-Ebenen-Migrations-Schritt — reicht den MUTABLEN Store-Zugriff durch, damit der
    // abi_adapter den realen Block-Move (organ_migrate_step) ueber das ECHTE Slot-Backing treibt: markierte Records
    // wandern aus diesem (heissen) Store in den uebergebenen 2.-Ebenen-Store `tier1`. if-constexpr-detektierbar
    // (requires), kein Bruch fuer Stores ohne organ_migrate_step. Anders als store_observe_* MUTIEREND (drive-Pfad,
    // NICHT Observer) → KEIN const. Rueckgabe = Zahl der real bewegten Records (NoMigration → 0). NICHT unter
    // COMDARE_CE_ENABLE_STATISTICS gegated: der Move selbst ist funktional, nur das tier_moves-Buchen ist gegated.
    template <class MigOrgan>
    std::uint64_t store_migrate_step(MigOrgan const& org, Store& tier1, std::uint64_t max_moves = 0)
        requires requires(Store& s, MigOrgan const& o, Store& t) { s.organ_migrate_step(o, t, max_moves); }
    {
        return search_.store_mut().organ_migrate_step(org, tier1, max_moves);
    }

    // P4 (#123): MUTABLER + read-only Zugriff auf das innere Storage-Organ — der abi_adapter braucht den 2.-Ebenen-
    // Store (container_tier1_) als Roh-Store-Referenz fuer store_migrate_step(org, tier1) UND fuer die Memento-
    // Verifikation (tier1-Fuellstand). Additiv, bestehende Schnittstelle unveraendert.
    [[nodiscard]] Store&       store_mut() noexcept { return search_.store_mut(); }
    [[nodiscard]] Store const& store() const noexcept { return search_.store(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = ce_concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; } // (A2.1) notify entfernt — Pull-Transport via statistics()
    // Undo-Log-Memento (#133): O(1)-Restore der Statistik auf einen zuvor via statistics() gezogenen Snapshot.
    // Gegenstueck zu reset() (={}): stellt im Zwei-Phasen-Rollback die Zaehler EXAKT auf den save-Stand zurueck,
    // nachdem das Daten-Substrat per op-inversem Replay wiederhergestellt wurde (abi_adapter::tier_rollback_all).
    void restore_statistics(snapshot_t const& s) noexcept { stats_ = s; } // (A2.1) notify entfernt
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

    // Saeule-2 (Roadmap-1): OPTIONALER Durchgriff auf die Allocator-Statistik des inneren Storage-Organs,
    // falls dieses (z.B. ComposedStore<N,L,A>) sie bietet. KEIN Runtime-Switch (reines requires/if constexpr);
    // erweitert NICHT den ObservableAxis-Vertrag (der laeuft weiter ueber search_algo/statistics()).
    template <class S>
    static constexpr bool store_has_allocator_stats = requires(S const& s) { s.allocator_statistics(); };

    template <class S = Store>
    [[nodiscard]] auto store_allocator_statistics() const noexcept
        requires store_has_allocator_stats<S>
    {
        return search_.store().allocator_statistics();
    }

    // V42 L-74c scan-Achsen-Auto-Kopplung: reicht den Store-Slot-Zugriff durch, damit der abi_adapter/Anatomie-
    // Treiber die memory_layout/serialization-Observer-Organe ueber das ECHTE Slot-Backing treiben kann
    // (organ_observe_layout/serialization am ComposedStore). if-constexpr-detektierbar (requires), kein Bruch
    // fuer Stores ohne diese Nicht-Vertrags-Methoden.
    template <class LayoutOrgan>
    std::uint64_t store_observe_layout(LayoutOrgan& org) const
        requires requires(Store const& s, LayoutOrgan& o) { s.organ_observe_layout(o); }
    {
        return search_.store().organ_observe_layout(org);
    }

    template <class SerOrgan>
    std::uint64_t store_observe_serialization(SerOrgan& org) const
        requires requires(Store const& s, SerOrgan& o) { s.organ_observe_serialization(o); }
    {
        return search_.store().organ_observe_serialization(org);
    }

    template <class NodeOrgan>
    std::uint64_t store_observe_node_type(NodeOrgan& org) const
        requires requires(Store const& s, NodeOrgan& o) { s.organ_observe_node_type(o); }
    {
        return search_.store().organ_observe_node_type(org);
    }

    // Phase B (2026-06-04) T11 value_handle / T12 isa scan-Achsen-Auto-Kopplung: reicht den Store-Slot-Zugriff
    // durch, damit der abi_adapter die value_handle-/isa-Observer-Huellen ueber das ECHTE Slot-Backing treibt
    // (organ_observe_value_handle/organ_observe_isa am NodeChunkedStore). if-constexpr-detektierbar (requires),
    // kein Bruch fuer Stores ohne diese Nicht-Vertrags-Methoden.
    template <class VhOrgan>
    std::uint64_t store_observe_value_handle(VhOrgan& org) const
        requires requires(Store const& s, VhOrgan& o) { s.organ_observe_value_handle(o); }
    {
        return search_.store().organ_observe_value_handle(org);
    }

    template <class IsaOrgan>
    std::uint64_t store_observe_isa(IsaOrgan& org) const
        requires requires(Store const& s, IsaOrgan& o) { s.organ_observe_isa(o); }
    {
        return search_.store().organ_observe_isa(org);
    }

    // Phase B (2026-06-04) T13/T14/T15/T16 scan-Achsen-Auto-Kopplung: reicht den Store-Slot-Zugriff durch, damit der
    // abi_adapter die index_org-/io_dispatch-/migration-/filter-Observer-Huellen ueber das ECHTE Slot-Backing treibt
    // (organ_observe_* am NodeChunkedStore). if-constexpr-detektierbar (requires), kein Bruch fuer Stores ohne diese
    // Nicht-Vertrags-Methoden.
    template <class IdxOrgan>
    std::uint64_t store_observe_index_org(IdxOrgan& org) const
        requires requires(Store const& s, IdxOrgan& o) { s.organ_observe_index_org(o); }
    {
        return search_.store().organ_observe_index_org(org);
    }

    template <class IoOrgan>
    std::uint64_t store_observe_io_dispatch(IoOrgan& org) const
        requires requires(Store const& s, IoOrgan& o) { s.organ_observe_io_dispatch(o); }
    {
        return search_.store().organ_observe_io_dispatch(org);
    }

    template <class MigOrgan>
    std::uint64_t store_observe_migration(MigOrgan& org) const
        requires requires(Store const& s, MigOrgan& o) { s.organ_observe_migration(o); }
    {
        return search_.store().organ_observe_migration(org);
    }

    template <class FltOrgan>
    std::uint64_t store_observe_filter(FltOrgan& org) const
        requires requires(Store const& s, FltOrgan& o) { s.organ_observe_filter(o); }
    {
        return search_.store().organ_observe_filter(org);
    }
#endif

    // ── V5-I6-SUBSTANZ (#44) — MementoAxis (per-Achsen-Memento, inkl. Observer-Stats) ──────────────────
    // Kapselt den GESAMTEN beobachtbaren Zustand: Daten (über das innere ComposedSearch-MementoAxis) UND die
    // Observer-Statistik. Letzteres ist für die Zwei-Phasen-Invariante PFLICHT: würde der Warmup-Op die Stats
    // erhöhen ohne Rollback, zählte die Mess-Op doppelt. So rollt restore_state Daten + Stats exakt zurück.
    struct memento_t {
        typename ComposedSearch<Traversal, Store>::memento_t data{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ce_concepts::SearchAlgoStatistics stats{};
#endif
    };

    [[nodiscard]] memento_t save_state() const {
        memento_t m;
        m.data = search_.save_state();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        m.stats = stats_;
#endif
        return m;
    }
    void restore_state(memento_t const& m) {
        search_.restore_state(m.data);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = m.stats; // (A2.1) notify entfernt
#endif
    }

private:
    ComposedSearch<Traversal, Store> search_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable snapshot_t stats_{};
    mutable observer_t observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup::composable
