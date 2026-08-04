#pragma once
// D9 / L-76a (2026-06-02) — SetAnatomy: die SET-GATTUNG (Vogel, genus()==Set, K-only). Hält das ECHTE
// search_algo-Organ der Komposition (Composition::search_algo) und treibt es als MENGE (K=V): insert(k)=
// insert(k,k), contains(k)=lookup(k).has_value(), erase(k). Liefert observe_all → SetObserverSnapshot.
//
// Leichtgewichtig: nutzt NUR Composition::search_algo direkt (kein ComposedStore/Umbrella); der
// SearchAlgorithmAbiAdapter treibt seit #188-4c den konstitutiven Zustand über container_algorithm_. Die 15 Set-Achsen
// sind die Komposition-Identität;
// real getrieben ist das search_algo-Kern-Organ (R5.B-Grenze ehrlich, wie SearchAlgorithm). Doku 14 §27.2/§28/§32.

// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3):
// PRODUKTIONSTIEFE. Bis C2 trug diese Anatomie GENAU EIN reales Organ (das search_algo-Kern-Organ) und lieferte
// aus observe_all() einen flachen Hand-POD; die uebrigen zwoelf Slots existierten nur als Kompositions-TYPEN.
// C3 macht daraus reale Organ-Member mit Accessoren + eine per-Achsen-Einsammlung nach dem SA-Muster
// (search_algorithm_anatomy.hpp:65-115: `if constexpr (ObservableAxis<...>) agg.X = axis_X_.statistics();`).
// Damit ist ein als SUB-ORGAN gehaltenes fremdes Genus-Organ (ObservableOrgan-Huelle, organ_concept.hpp) auch
// in der Set-Gattung nicht mehr messtechnisch stumm -- das war der in organ_concept.hpp L2 benannte C3-Rest.
//
// ABI-NEUTRAL (a-Teil): observe_all() liefert UNVERAENDERT den flachen SetObserverSnapshot, den
// set_abi_adapter.hpp:66-80 in den Wire-POD SetObserverSnapshotV1 spiegelt. Kein Byte an abi_adapter.hpp, an
// set_tier.hpp, an abi/*_decl.hpp oder an den Stempel-/Fingerprint-Flaechen. Die per-Achsen-Sicht kommt
// ADDITIV als eigene Abnahme observe_axes() hinzu; ihre Promotion in die Wire-Ebene (SetObserverAggregate<13>
// STATT des flachen PODs) ist der ABI-Schritt C6 des b-Teils, nicht dieser Commit.

#include "anatomy_base.hpp"       // AnatomyGenus
#include "observer_aggregate.hpp" // E-24 C3: ObservableAxis / snapshot_of_t (die ACHSEN-Ebene, SA-Muster)
#include "organ_concept.hpp"      // E-24 C1: OrganGuard (CRTP-Wache) -- Layer: zieht NUR anatomy_base.hpp nach
#include "set_composition.hpp"    // SetComposition / IsSetComposition

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// In-process Set-Observer (reich; der ABI-POD SetObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct SetObserverSnapshot {
    std::uint64_t insert_count        = 0;
    std::uint64_t contains_count      = 0;
    std::uint64_t contains_hit_count  = 0;
    std::uint64_t contains_miss_count = 0;
    std::uint64_t erase_count         = 0;
    std::uint64_t current_size        = 0;
    std::uint64_t peak_size           = 0;
};

/// SetAxisObservation<Composition> -- E-24 C3: die PER-ACHSEN-Sicht der Set-Gattung, gebaut nach dem
/// SearchAlgorithm-Vorbild ObserverAggregate<Composition> (observer_aggregate.hpp:93-146): je Slot ein
/// Snapshot-Member, dessen Typ ueber snapshot_of_t aus der Achse selbst kommt. Eine Achse ohne statistics()
/// bekommt EmptyAxisSnapshot -- kein Sonderfall, kein erfundener Wert.
///
/// Reihenfolge der Member == Reihenfolge von GenusBindingTraits<Set>::axis_names() (Kreuz-Wache in der
/// Test-TU tests/unit/test_e24_c3_set_anatomy.cpp; hier waere sie ein Include-Zyklus).
///
/// ABGRENZUNG zu C6: dies ist eine IN-PROCESS-Sicht, KEIN Wire-POD. Der cross-boundary-Transport der
/// per-Achsen-Werte (SetObserverSnapshotV1-Nachfolge) ist der ABI-Schritt C6.
template <class Composition>
struct SetAxisObservation {
    snapshot_of_t<typename Composition::search_algo>        search_algo;
    snapshot_of_t<typename Composition::cache_traversal>    cache_traversal;
    snapshot_of_t<typename Composition::path_compression>   path_compression;
    snapshot_of_t<typename Composition::node_type>          node_type;
    snapshot_of_t<typename Composition::memory_layout>      memory_layout;
    snapshot_of_t<typename Composition::allocator>          allocator;
    snapshot_of_t<typename Composition::prefetch>           prefetch;
    snapshot_of_t<typename Composition::concurrency>        concurrency;
    snapshot_of_t<typename Composition::serialization>      serialization;
    snapshot_of_t<typename Composition::index_organization> index_organization;
    snapshot_of_t<typename Composition::io_dispatch>        io_dispatch;
    snapshot_of_t<typename Composition::migration_policy>   migration_policy;
    snapshot_of_t<typename Composition::filter>             filter;

    /// Wie viele der Slots liefern echte Snapshots (Rest = EmptyAxisSnapshot)? Diagnose fuer Mess-Treiber,
    /// identische Semantik wie ObserverAggregate<C>::observable_count().
    [[nodiscard]] static constexpr std::size_t observable_count() noexcept {
        std::size_t n = 0;
        if constexpr (ObservableAxis<typename Composition::search_algo>) ++n;
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) ++n;
        if constexpr (ObservableAxis<typename Composition::path_compression>) ++n;
        if constexpr (ObservableAxis<typename Composition::node_type>) ++n;
        if constexpr (ObservableAxis<typename Composition::memory_layout>) ++n;
        if constexpr (ObservableAxis<typename Composition::allocator>) ++n;
        if constexpr (ObservableAxis<typename Composition::prefetch>) ++n;
        if constexpr (ObservableAxis<typename Composition::concurrency>) ++n;
        if constexpr (ObservableAxis<typename Composition::serialization>) ++n;
        if constexpr (ObservableAxis<typename Composition::index_organization>) ++n;
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) ++n;
        if constexpr (ObservableAxis<typename Composition::migration_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::filter>) ++n;
        return n;
    }

    /// Slot-Zahl aus der Komposition GERECHNET, nicht als Literal gepflegt (13, INC-2d).
    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return Composition::slot_count; }
};

/// SetAnatomy<Composition> — genus()==Set; K-only-Mengen-API über das echte search_algo-Organ (K=V).
/// E-24 C1 (Luecke L5): erbt die CRTP-Wache OrganGuard nach dem Repo-Goldstandard
/// axes/io_dispatch/axis_io_strategy_base.hpp:15-21 -- die erste Konstruktion prueft compile-hart, dass
/// diese Anatomie OrganConcept erfuellt (Identitaet + const-noexcept observe_all). Zero-cost: leere Basis,
/// protected Ctor, keine virtuelle Funktion. Kein ABI-Ereignis (EBO, keine Layout-Aenderung).
template <class Composition>
class SetAnatomy : public OrganGuard<SetAnatomy<Composition>> {
public:
    using composition_t = Composition;
    using set_organ_t   = typename Composition::search_algo;
    /// E-24 C3: der per-Achsen-Beobachtungs-Typ dieser Gattung (Namens-Analogie zu
    /// SearchAlgorithmAnatomy::observer_aggregate_t; ABGRENZUNG: in-process, kein Wire-POD -- s. C6).
    using axis_observation_t = SetAxisObservation<Composition>;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::Set; }             // Vogel
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 13 (INC-2d)

    // ── Set-Gattungs-API (K-only) — treibt das ECHTE search_algo-Organ als Menge (K=V) ──
    bool insert(std::uint64_t key) {
        auto const before = static_cast<std::uint64_t>(axis_search_algo_.occupied_count());
        axis_search_algo_.insert(static_cast<key_t>(key), static_cast<value_t>(key)); // K=V
        auto const after  = static_cast<std::uint64_t>(axis_search_algo_.occupied_count());
        bool const is_new = after > before;
        if (is_new) ++obs_.insert_count;
        obs_.current_size = after;
        if (after > obs_.peak_size) obs_.peak_size = after;
        return is_new;
    }
    [[nodiscard]] bool contains(std::uint64_t key) const {
        ++obs_.contains_count;
        bool const hit = axis_search_algo_.lookup(static_cast<key_t>(key)).has_value();
        if (hit)
            ++obs_.contains_hit_count;
        else
            ++obs_.contains_miss_count;
        return hit;
    }
    bool erase(std::uint64_t key) {
        auto const before = static_cast<std::uint64_t>(axis_search_algo_.occupied_count());
        axis_search_algo_.erase(static_cast<key_t>(key));
        auto const after   = static_cast<std::uint64_t>(axis_search_algo_.occupied_count());
        bool const removed = after < before;
        if (removed) ++obs_.erase_count;
        obs_.current_size = after;
        return removed;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return static_cast<std::size_t>(axis_search_algo_.occupied_count());
    }
    void clear() {
        axis_search_algo_.clear();
        obs_.current_size = 0;
    }

    /// observe_all() -- die flache GATTUNGS-Sicht. UNVERAENDERT gegenueber C2 (Wire-Spiegelung
    /// set_abi_adapter.hpp:66-80); die per-Achsen-Sicht steht daneben in observe_axes().
    [[nodiscard]] SetObserverSnapshot observe_all() const noexcept { return obs_; }

    // -- E-24 C3: ORGAN-ACCESSOREN (einer je Slot, Reihenfolge == axis_names()) --------------------
    // Zweck identisch zum SA-Vorbild (search_algorithm_anatomy.hpp:119-170): der Builder/Mess-Treiber
    // treibt ueber sie die real gehaltenen Achsen-Organe; ihre statistics() fliessen ueber observe_axes()
    // in die per-Achsen-Sicht. Wer hier ein FREMDES Genus-Organ einsetzt, setzt es als ObservableOrgan-
    // Huelle ein (organ_concept.hpp) -- dann traegt der Slot echte Sub-Organ-Werte statt EmptyAxisSnapshot.

    // Bewusst AUSGESCHRIEBEN statt makro-generiert: dieses Repo arbeitet mit der grep-Doktrin ("bindend sind
    // die grep-Kommandos, nicht die Zahlen"). Ein `COMDARE_ORGAN_ACCESSOR(search_algo)` liesse `grep -n
    // search_algo_organ` ins Leere laufen. Gleiche Entscheidung wie beim SA-Vorbild.
    [[nodiscard]] typename Composition::search_algo&       search_algo_organ() noexcept { return axis_search_algo_; }
    [[nodiscard]] typename Composition::search_algo const& search_algo_organ() const noexcept {
        return axis_search_algo_;
    }
    [[nodiscard]] typename Composition::cache_traversal& cache_traversal_organ() noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::cache_traversal const& cache_traversal_organ() const noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::path_compression& path_compression_organ() noexcept {
        return axis_path_compression_;
    }
    [[nodiscard]] typename Composition::path_compression const& path_compression_organ() const noexcept {
        return axis_path_compression_;
    }
    [[nodiscard]] typename Composition::node_type&       node_type_organ() noexcept { return axis_node_type_; }
    [[nodiscard]] typename Composition::node_type const& node_type_organ() const noexcept { return axis_node_type_; }
    [[nodiscard]] typename Composition::memory_layout&   memory_layout_organ() noexcept { return axis_memory_layout_; }
    [[nodiscard]] typename Composition::memory_layout const& memory_layout_organ() const noexcept {
        return axis_memory_layout_;
    }
    [[nodiscard]] typename Composition::allocator&         allocator_organ() noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::allocator const&   allocator_organ() const noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::prefetch&          prefetch_organ() noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::prefetch const&    prefetch_organ() const noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::concurrency&       concurrency_organ() noexcept { return axis_concurrency_; }
    [[nodiscard]] typename Composition::concurrency const& concurrency_organ() const noexcept {
        return axis_concurrency_;
    }
    [[nodiscard]] typename Composition::serialization& serialization_organ() noexcept { return axis_serialization_; }
    [[nodiscard]] typename Composition::serialization const& serialization_organ() const noexcept {
        return axis_serialization_;
    }
    [[nodiscard]] typename Composition::index_organization& index_organization_organ() noexcept {
        return axis_index_organization_;
    }
    [[nodiscard]] typename Composition::index_organization const& index_organization_organ() const noexcept {
        return axis_index_organization_;
    }
    [[nodiscard]] typename Composition::io_dispatch&       io_dispatch_organ() noexcept { return axis_io_dispatch_; }
    [[nodiscard]] typename Composition::io_dispatch const& io_dispatch_organ() const noexcept {
        return axis_io_dispatch_;
    }
    [[nodiscard]] typename Composition::migration_policy& migration_policy_organ() noexcept {
        return axis_migration_policy_;
    }
    [[nodiscard]] typename Composition::migration_policy const& migration_policy_organ() const noexcept {
        return axis_migration_policy_;
    }
    [[nodiscard]] typename Composition::filter&       filter_organ() noexcept { return axis_filter_; }
    [[nodiscard]] typename Composition::filter const& filter_organ() const noexcept { return axis_filter_; }

    /// axis_organ_names() -- die Namen der REAL GEHALTENEN Organ-Member in Slot-Reihenfolge. Diese Liste ist
    /// die Praesenz-DEKLARATION; die WACHE dagegen lebt in der Test-TU, wo sie elementweise gegen die
    /// autoritative GenusBindingTraits<Set>::axis_names() gekreuzt wird (hier waere das ein Include-Zyklus:
    /// genus_binding_traits.hpp inkludiert diesen Header).
    [[nodiscard]] static constexpr std::array<std::string_view, 13> const& axis_organ_names() noexcept {
        static constexpr std::array<std::string_view, 13> kNames = {
            "search_algo",   "cache_traversal",    "path_compression", "node_type",
            "memory_layout", "allocator",          "prefetch",         "concurrency",
            "serialization", "index_organization", "io_dispatch",      "migration_policy",
            "filter"};
        return kNames;
    }

    /// observe_axes() -- E-24 C3 / Luecke-L2-Rest: sammelt die Sub-Organ-Beobachtung Slot fuer Slot ein,
    /// exakt nach dem SA-Muster (search_algorithm_anatomy.hpp:65-115). Ein Slot ohne statistics() bleibt
    /// EmptyAxisSnapshot -- die Anatomie erfindet keinen Wert.
    [[nodiscard]] axis_observation_t observe_axes() const noexcept {
        axis_observation_t agg{};
        if constexpr (ObservableAxis<typename Composition::search_algo>) {
            agg.search_algo = axis_search_algo_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) {
            agg.cache_traversal = axis_cache_traversal_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::path_compression>) {
            agg.path_compression = axis_path_compression_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::node_type>) { agg.node_type = axis_node_type_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::memory_layout>) {
            agg.memory_layout = axis_memory_layout_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::allocator>) { agg.allocator = axis_allocator_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::prefetch>) { agg.prefetch = axis_prefetch_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::concurrency>) {
            agg.concurrency = axis_concurrency_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::serialization>) {
            agg.serialization = axis_serialization_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::index_organization>) {
            agg.index_organization = axis_index_organization_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) {
            agg.io_dispatch = axis_io_dispatch_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::migration_policy>) {
            agg.migration_policy = axis_migration_policy_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::filter>) { agg.filter = axis_filter_.statistics(); }
        return agg;
    }

    /// Diagnose: wie viele Slots liefern echte Snapshots? (Namensgleich zum SA-Vorbild
    /// search_algorithm_anatomy.hpp:173-175 -- ein Name, eine Bedeutung ueber alle Gattungen.)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return axis_observation_t::observable_count();
    }

private:
    using key_t   = typename set_organ_t::key_type;
    using value_t = typename set_organ_t::key_type; // Set: K=V (Value-Typ == Key-Typ)

    // E-24 C3: die 13 Slots als REALE Organ-Member. Der search_algo-Slot ist der getriebene Kern (war bis C2
    // als `organ_` das einzige Member); die uebrigen zwoelf werden GETRAGEN und ueber ihre Accessoren
    // getrieben -- R5.B-Grenze ehrlich, wie bei SearchAlgorithmAnatomy. Init-Stil wie dort (:194-221): der
    // Kern mit `{}`, die uebrigen default-init OHNE `{}`, weil eine nackte Aggregat-Strategie mit `{}`
    // ill-formed waere (Befund test_d_v42_probe2).
    set_organ_t                              axis_search_algo_{};
    typename Composition::cache_traversal    axis_cache_traversal_;
    typename Composition::path_compression   axis_path_compression_;
    typename Composition::node_type          axis_node_type_;
    typename Composition::memory_layout      axis_memory_layout_;
    typename Composition::allocator          axis_allocator_;
    typename Composition::prefetch           axis_prefetch_;
    typename Composition::concurrency        axis_concurrency_;
    typename Composition::serialization      axis_serialization_;
    typename Composition::index_organization axis_index_organization_;
    typename Composition::io_dispatch        axis_io_dispatch_;
    typename Composition::migration_policy   axis_migration_policy_;
    typename Composition::filter             axis_filter_;

    mutable SetObserverSnapshot obs_{}; // mutable: contains() ist const, zählt aber Abfragen
};

} // namespace comdare::cache_engine::anatomy
