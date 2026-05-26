#pragma once
// V41.F.6.1.R3 — SearchAlgorithmAnatomy<Composition> Skelett
//
// Saeugetier-Anatomie: EINE generische Such-Algorithmus-Klasse, die durch
// Template-Spezialisierung mit jeder Composition zu einem konkreten Algorithmus
// wird. Phase R3 ist Pilot-Skelett mit std::map als Container; echte
// Composition-driven Implementation kommt in R4+R5.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§12+§14
// @task V41.F.6.1.R3

#include "composition_concept.hpp"
#include "observer_aggregate.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// SearchAlgorithmAnatomy — zentrale Anatomie-Klasse fuer ALLE Suchalgorithmen.
///
/// Template-Parameter Composition liefert die 17 Achsen-Auspraegungen. Konkrete
/// Algorithmen (ART/HOT/Wormhole/SuRF/Masstree/START) sind reine Template-
/// Instantiationen — siehe `anatomy::Art`, `anatomy::Hot` etc. unten.
///
/// Phase R3 (Pilot): interner std::map<uint64_t,uint64_t> als Container.
/// Phase R4+: Container wird durch Composition::node_type + Composition::allocator
/// + Composition::concurrency-getriebene Implementation ersetzt.
template <IsComposition Composition>
class SearchAlgorithmAnatomy {
public:
    using composition_t        = Composition;
    using key_type             = std::uint64_t;
    using value_type           = std::uint64_t;
    using observer_aggregate_t = ObserverAggregate<Composition>;

    // Composition-Inspection (statisch — Pflicht fuer Mess-Treiber)
    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr std::size_t organ_count() noexcept { return composition_organ_count<Composition>::value; }

    // ─────────────────────────────────────────────────────────────────────
    // R5.A Observer-Aggregate: ABI-stabiler 17-Achsen-Snapshot
    // (User-Direktive 2026-05-26 spaet, Doku 14 Teil 3 §17.2+§20)
    // ─────────────────────────────────────────────────────────────────────

    /// observe_all() — sammelt Snapshots aller 17 Achsen zu einem POD-Struct.
    /// Achsen ohne statistics() liefern EmptyAxisSnapshot.
    /// Pflicht-API fuer CacheEngineBuilder Mess-Treiber + ABI-Loader.
    ///
    /// **R5.A Pilot:** Default-Aggregate (alle Achsen Empty-Snapshot).
    /// **R5.B+ Ziel:** echte Achsen-Members + statistics()-Aufrufe.
    /// Aktueller Block: Achsen-Wrappers haben protected CRTP-Constructor —
    /// direktes Halten als Anatomie-Member blockiert. Wird durch
    /// public-Constructor-Fix oder Tuple-basierte Komposition spaeter geloest.
    [[nodiscard]] observer_aggregate_t observe_all() const noexcept {
        // Default-initialized aggregate — alle Slots EmptyAxisSnapshot oder
        // Default-Snapshot je nach Composition::xxx::snapshot_t Trait
        return observer_aggregate_t{};
    }

    /// Diagnose: wie viele Achsen liefern echte Snapshots? (Rest = EmptyAxisSnapshot)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return observer_aggregate_t::observable_count();
    }

    // ─────────────────────────────────────────────────────────────────────
    // R5.B Container-API ENTFERNT — siehe builder::anatomy_commands::
    //      AnatomyExecutionContext<Composition> fuer insert/lookup/erase/clear/size/empty.
    //
    // User-Direktive 2026-05-26 sehr spaet (Doku 14 Teil 3 §17.2+§24):
    //   "Anatomie enthaelt nur Achsen + Statistik-Observer. Alle anderen Methoden
    //   und tools gehoeren in CacheEngineBuilder."
    //
    // Migration-Pfad fuer existing Code:
    //   ANATOMY-OLD: SearchAlgorithmAnatomy<C> a; a.insert(k,v);
    //   BUILDER-NEU: AnatomyExecutionContext<C> ctx; ctx.insert(k,v);
    //              (Anatomie ist intern Bestandteil des ctx)
    // ─────────────────────────────────────────────────────────────────────
};

}  // namespace comdare::cache_engine::anatomy
