#pragma once
// D10 / L-76b (2026-06-02) — SequenceAnatomy: die SEQUENCE-GATTUNG (Reptil, genus()==Sequence, V-indexed).
// Hält ein V-Speicher-Organ (wachsendes Array) und treibt die axis_growth-Policy der Komposition REAL
// (next_capacity steuert reserve → growth_events). API: push_back(v)/at(idx)→value/size/clear. Doku 14 §27.2/§28/§32.

// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3):
// PRODUKTIONSTIEFE, Schnitt identisch zur Set-Gattung (set_anatomy.hpp). Bis C2 hielt diese Anatomie genau
// ein reales Achsen-Organ (die growth_policy) und lieferte aus observe_all() einen flachen Hand-POD; die
// uebrigen acht Slots existierten nur als Kompositions-TYPEN. C3 macht daraus reale Organ-Member mit
// Accessoren + die per-Achsen-Einsammlung nach dem SA-Muster (search_algorithm_anatomy.hpp:65-115).
//
// ABI-NEUTRAL (a-Teil): observe_all() liefert UNVERAENDERT den flachen SequenceObserverSnapshot, den
// sequence_abi_adapter.hpp in den Wire-POD SequenceObserverSnapshotV1 spiegelt. Die per-Achsen-Sicht kommt
// ADDITIV als observe_axes() hinzu; ihre Promotion in die Wire-Ebene ist der ABI-Schritt C6 des b-Teils.

#include "anatomy_base.hpp"         // AnatomyGenus
#include "observer_aggregate.hpp"   // E-24 C3: ObservableAxis / snapshot_of_t (die ACHSEN-Ebene, SA-Muster)
#include "organ_concept.hpp"        // E-24 C1: OrganGuard (CRTP-Wache)
#include "sequence_composition.hpp" // SequenceComposition / GrowthPolicy

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::anatomy {

/// In-process Sequence-Observer (reich; SequenceObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct SequenceObserverSnapshot {
    std::uint64_t push_count    = 0;
    std::uint64_t at_count      = 0;
    std::uint64_t at_oob_count  = 0;
    std::uint64_t current_size  = 0;
    std::uint64_t peak_size     = 0;
    std::uint64_t growth_events = 0;
};

/// SequenceAxisObservation<Composition> -- E-24 C3: die PER-ACHSEN-Sicht der Sequence-Gattung, gebaut nach
/// dem SearchAlgorithm-Vorbild ObserverAggregate<Composition> (observer_aggregate.hpp:93-146). Reihenfolge
/// der Member == GenusBindingTraits<Sequence>::axis_names() (Kreuz-Wache in der Test-TU
/// tests/unit/test_e24_c3_sequence_anatomy.cpp; hier waere sie ein Include-Zyklus).
/// ABGRENZUNG zu C6: in-process-Sicht, KEIN Wire-POD.
template <class Composition>
struct SequenceAxisObservation {
    snapshot_of_t<typename Composition::memory_layout>    memory_layout;
    snapshot_of_t<typename Composition::allocator>        allocator;
    snapshot_of_t<typename Composition::prefetch>         prefetch;
    snapshot_of_t<typename Composition::concurrency>      concurrency;
    snapshot_of_t<typename Composition::serialization>    serialization;
    snapshot_of_t<typename Composition::value_handle>     value_handle;
    snapshot_of_t<typename Composition::io_dispatch>      io_dispatch;
    snapshot_of_t<typename Composition::migration_policy> migration_policy;
    snapshot_of_t<typename Composition::growth_policy>    growth_policy;

    /// Wie viele Slots liefern echte Snapshots (Rest = EmptyAxisSnapshot)?
    [[nodiscard]] static constexpr std::size_t observable_count() noexcept {
        std::size_t n = 0;
        if constexpr (ObservableAxis<typename Composition::memory_layout>) ++n;
        if constexpr (ObservableAxis<typename Composition::allocator>) ++n;
        if constexpr (ObservableAxis<typename Composition::prefetch>) ++n;
        if constexpr (ObservableAxis<typename Composition::concurrency>) ++n;
        if constexpr (ObservableAxis<typename Composition::serialization>) ++n;
        if constexpr (ObservableAxis<typename Composition::value_handle>) ++n;
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) ++n;
        if constexpr (ObservableAxis<typename Composition::migration_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::growth_policy>) ++n;
        return n;
    }

    /// Slot-Zahl aus der Komposition GERECHNET, nicht als Literal gepflegt (9, INC-2d).
    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return Composition::slot_count; }
};

/// SequenceAnatomy<Composition> — genus()==Sequence; V-indexed-API über ein wachsendes Array; treibt die
/// axis_growth-Policy (Composition::growth_policy) real (Kapazitäts-Wachstum).
/// E-24 C1 (Luecke L5): erbt die CRTP-Wache OrganGuard (Goldstandard axis_io_strategy_base.hpp:15-21) --
/// die erste Konstruktion prueft compile-hart gegen OrganConcept. Zero-cost, kein ABI-Ereignis.
template <class Composition>
class SequenceAnatomy : public OrganGuard<SequenceAnatomy<Composition>> {
public:
    using composition_t = Composition;
    using growth_t      = typename Composition::growth_policy;
    using element_type  = std::uint64_t;
    /// E-24 C3: der per-Achsen-Beobachtungs-Typ dieser Gattung (in-process, kein Wire-POD -- s. C6).
    using axis_observation_t = SequenceAxisObservation<Composition>;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::Sequence; }        // Reptil
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 9

    // ── Sequence-Gattungs-API (V-indexed) — treibt das V-Organ + die axis_growth-Policy ──
    void push_back(element_type v) {
        if (data_.size() == capacity_) { // Überlauf → Growth-Policy befragen
            capacity_ = axis_growth_policy_.next_capacity(capacity_, data_.size() + 1);
            data_.reserve(capacity_);
            ++obs_.growth_events;
        }
        data_.push_back(v);
        ++obs_.push_count;
        obs_.current_size = static_cast<std::uint64_t>(data_.size());
        if (obs_.current_size > obs_.peak_size) obs_.peak_size = obs_.current_size;
    }
    [[nodiscard]] std::optional<element_type> at(std::uint64_t index) const {
        ++obs_.at_count;
        if (index >= data_.size()) {
            ++obs_.at_oob_count;
            return std::nullopt;
        }
        return data_[static_cast<std::size_t>(index)];
    }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    void                      clear() noexcept {
        data_.clear();
        obs_.current_size = 0;
    }

    /// observe_all() -- die flache GATTUNGS-Sicht. UNVERAENDERT gegenueber C2 (Wire-Spiegelung im
    /// sequence_abi_adapter); die per-Achsen-Sicht steht daneben in observe_axes().
    [[nodiscard]] SequenceObserverSnapshot observe_all() const noexcept { return obs_; }

    // -- E-24 C3: ORGAN-ACCESSOREN (einer je Slot, Reihenfolge == axis_names()) --------------------
    // Bewusst AUSGESCHRIEBEN statt makro-generiert (grep-Doktrin: `grep -n growth_policy_organ` muss
    // treffen). Muster + Begruendung identisch zu set_anatomy.hpp.
    [[nodiscard]] typename Composition::memory_layout& memory_layout_organ() noexcept { return axis_memory_layout_; }
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
    [[nodiscard]] typename Composition::value_handle&       value_handle_organ() noexcept { return axis_value_handle_; }
    [[nodiscard]] typename Composition::value_handle const& value_handle_organ() const noexcept {
        return axis_value_handle_;
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
    [[nodiscard]] typename Composition::growth_policy& growth_policy_organ() noexcept { return axis_growth_policy_; }
    [[nodiscard]] typename Composition::growth_policy const& growth_policy_organ() const noexcept {
        return axis_growth_policy_;
    }

    /// axis_organ_names() -- Praesenz-DEKLARATION der real gehaltenen Organ-Member in Slot-Reihenfolge.
    /// Die WACHE dagegen lebt in der Test-TU (Kreuz gegen GenusBindingTraits<Sequence>::axis_names()).
    [[nodiscard]] static constexpr std::array<std::string_view, 9> const& axis_organ_names() noexcept {
        static constexpr std::array<std::string_view, 9> kNames = {
            "memory_layout", "allocator",   "prefetch",         "concurrency",  "serialization",
            "value_handle",  "io_dispatch", "migration_policy", "growth_policy"};
        return kNames;
    }

    /// observe_axes() -- E-24 C3 / Luecke-L2-Rest: sammelt die Sub-Organ-Beobachtung Slot fuer Slot ein,
    /// exakt nach dem SA-Muster. Ein Slot ohne statistics() bleibt EmptyAxisSnapshot.
    [[nodiscard]] axis_observation_t observe_axes() const noexcept {
        axis_observation_t agg{};
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
        if constexpr (ObservableAxis<typename Composition::value_handle>) {
            agg.value_handle = axis_value_handle_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) {
            agg.io_dispatch = axis_io_dispatch_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::migration_policy>) {
            agg.migration_policy = axis_migration_policy_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::growth_policy>) {
            agg.growth_policy = axis_growth_policy_.statistics();
        }
        return agg;
    }

    /// Diagnose: wie viele Slots liefern echte Snapshots? (Namensgleich zum SA-Vorbild.)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return axis_observation_t::observable_count();
    }

private:
    std::vector<element_type> data_{};
    std::size_t               capacity_ = 0;

    // E-24 C3: die 9 Slots als REALE Organ-Member. growth_policy ist der getriebene Slot (war bis C2 als
    // `growth_` das einzige Achsen-Member); die uebrigen acht werden GETRAGEN und ueber ihre Accessoren
    // getrieben -- R5.B-Grenze ehrlich. Init-Stil wie SearchAlgorithmAnatomy:194-221 (default-init OHNE
    // `{}`, weil eine nackte Aggregat-Strategie mit `{}` ill-formed waere, Befund test_d_v42_probe2);
    // growth_policy behaelt sein `{}`, weil es seit L-76b so gebaut ist und real getrieben wird.
    typename Composition::memory_layout    axis_memory_layout_;
    typename Composition::allocator        axis_allocator_;
    typename Composition::prefetch         axis_prefetch_;
    typename Composition::concurrency      axis_concurrency_;
    typename Composition::serialization    axis_serialization_;
    typename Composition::value_handle     axis_value_handle_;
    typename Composition::io_dispatch      axis_io_dispatch_;
    typename Composition::migration_policy axis_migration_policy_;
    growth_t                               axis_growth_policy_{};

    mutable SequenceObserverSnapshot obs_{}; // mutable: at() ist const, zählt aber Zugriffe
};

} // namespace comdare::cache_engine::anatomy
