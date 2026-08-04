#pragma once
// D11 / L-76c (2026-06-02) — ViewAnatomy: die VIEW-GATTUNG (Pflanze, genus()==View, non-owning). Referenziert
// einen EXTERNEN Puffer (kein eigener Speicher) und liest über die axis_layout-/axis_accessor-Policy. API:
// bind(data,size)/read(index)→value/size. KEIN insert/erase/clear (non-owning + immutabel). Doku 14 §27.2/§28/§32.

// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3):
// PRODUKTIONSTIEFE, Schnitt identisch zu set_/sequence_anatomy.hpp. Bis C2 hielt diese Anatomie zwei reale
// Achsen-Organe (layout_policy + accessor_policy, ueber die read() laeuft); die uebrigen drei Slots
// existierten nur als Kompositions-TYPEN. C3 macht daraus reale Organ-Member mit Accessoren + die
// per-Achsen-Einsammlung nach dem SA-Muster (search_algorithm_anatomy.hpp:65-115).
//
// ABI-NEUTRAL (a-Teil): observe_all() liefert UNVERAENDERT den flachen ViewObserverSnapshot, den
// view_abi_adapter.hpp in den Wire-POD ViewObserverSnapshotV1 spiegelt. Die per-Achsen-Sicht kommt ADDITIV
// als observe_axes() hinzu; ihre Promotion in die Wire-Ebene ist der ABI-Schritt C6 des b-Teils.

#include "anatomy_base.hpp"       // AnatomyGenus
#include "observer_aggregate.hpp" // E-24 C3: ObservableAxis / snapshot_of_t (die ACHSEN-Ebene, SA-Muster)
#include "organ_concept.hpp"      // E-24 C1: OrganGuard (CRTP-Wache)
#include "view_composition.hpp"   // ViewComposition / Layout/Accessor-Policies

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// In-process View-Observer (reich; ViewObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct ViewObserverSnapshot {
    std::uint64_t read_count     = 0;
    std::uint64_t read_oob_count = 0;
    std::uint64_t bound_size     = 0;
    std::uint64_t bind_count     = 0;
};

/// ViewAxisObservation<Composition> -- E-24 C3: die PER-ACHSEN-Sicht der View-Gattung, gebaut nach dem
/// SearchAlgorithm-Vorbild ObserverAggregate<Composition> (observer_aggregate.hpp:93-146). Reihenfolge der
/// Member == GenusBindingTraits<View>::axis_names() (Kreuz-Wache in der Test-TU
/// tests/unit/test_e24_c3_view_anatomy.cpp; hier waere sie ein Include-Zyklus).
/// ABGRENZUNG zu C6: in-process-Sicht, KEIN Wire-POD.
template <class Composition>
struct ViewAxisObservation {
    snapshot_of_t<typename Composition::memory_layout>   memory_layout;
    snapshot_of_t<typename Composition::value_handle>    value_handle;
    snapshot_of_t<typename Composition::extent_policy>   extent_policy;
    snapshot_of_t<typename Composition::layout_policy>   layout_policy;
    snapshot_of_t<typename Composition::accessor_policy> accessor_policy;

    /// Wie viele Slots liefern echte Snapshots (Rest = EmptyAxisSnapshot)?
    [[nodiscard]] static constexpr std::size_t observable_count() noexcept {
        std::size_t n = 0;
        if constexpr (ObservableAxis<typename Composition::memory_layout>) ++n;
        if constexpr (ObservableAxis<typename Composition::value_handle>) ++n;
        if constexpr (ObservableAxis<typename Composition::extent_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::layout_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::accessor_policy>) ++n;
        return n;
    }

    /// Slot-Zahl aus der Komposition GERECHNET, nicht als Literal gepflegt (5, INC-2d).
    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return Composition::slot_count; }
};

/// ViewAnatomy<Composition> — genus()==View; non-owning V-indexed read-only über die axis_layout/accessor-Policy.
/// E-24 C1 (Luecke L5): erbt die CRTP-Wache OrganGuard (Goldstandard axis_io_strategy_base.hpp:15-21) --
/// die erste Konstruktion prueft compile-hart gegen OrganConcept. Zero-cost, kein ABI-Ereignis.
template <class Composition>
class ViewAnatomy : public OrganGuard<ViewAnatomy<Composition>> {
public:
    using composition_t = Composition;
    using layout_t      = typename Composition::layout_policy;
    using accessor_t    = typename Composition::accessor_policy;
    using element_type  = std::uint64_t;
    /// E-24 C3: der per-Achsen-Beobachtungs-Typ dieser Gattung (in-process, kein Wire-POD -- s. C6).
    using axis_observation_t = ViewAxisObservation<Composition>;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::View; } // Pflanze
    /// E-24 C7-7 (Symmetrie-Heilung): die Ebene-1-Zuordnung, ABGELEITET aus genus() (s. set_anatomy.hpp).
    static constexpr AnatomyGattung gattung() noexcept { return gattung_of(genus()); }
    static constexpr std::size_t    organ_count() noexcept { return Composition::slot_count; } // 5

    // ── View-Gattungs-API (non-owning) — bind externer Puffer + read über layout/accessor ──
    void bind(element_type const* data, std::size_t size) noexcept {
        data_ = data;
        size_ = (data == nullptr) ? 0 : size;
        ++obs_.bind_count;
        obs_.bound_size = static_cast<std::uint64_t>(size_);
        // E-24 C6-V: das Binden ist ein REALES Ereignis des Ausdehnungs-VERTRAGS -- die extent_policy-Achse
        // bekommt es gemeldet, WENN ihr Slot eine beobachtende Huelle traegt (ObservableExtent,
        // view_policies_observable.hpp). Eine nackte Policy hat den Member nicht: `if constexpr` blendet die
        // Meldung dann restlos aus (zero-cost, kein Verhalten geaendert).
        if constexpr (requires(typename Composition::extent_policy const& e, std::size_t n) { e.note_bind(n); }) {
            axis_extent_policy_.note_bind(size_);
        }
    }
    [[nodiscard]] std::optional<element_type> read(std::uint64_t index) const noexcept {
        ++obs_.read_count;
        std::size_t const off = axis_layout_policy_.index_of(static_cast<std::size_t>(index)); // axis_layout
        // E-24 C6-V: die Grenz-Pruefung gegen die gebundene Ausdehnung ist das zweite reale Vertrags-Ereignis
        // der extent_policy-Achse. Der Speicher-Sicherheits-Guard darunter bleibt unveraendert bestehen --
        // die Achse beobachtet, sie ersetzt ihn nicht.
        if constexpr (requires(typename Composition::extent_policy const& e, std::size_t o, std::size_t b) {
                          e.note_bounds_check(o, b);
                      }) {
            axis_extent_policy_.note_bounds_check(off, size_);
        }
        if (data_ == nullptr || off >= size_) {
            ++obs_.read_oob_count;
            return std::nullopt;
        }
        return axis_accessor_policy_.access(data_, off); // axis_accessor
    }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    /// observe_all() -- die flache GATTUNGS-Sicht. UNVERAENDERT gegenueber C2 (Wire-Spiegelung im
    /// view_abi_adapter); die per-Achsen-Sicht steht daneben in observe_axes().
    [[nodiscard]] ViewObserverSnapshot observe_all() const noexcept { return obs_; }

    // -- E-24 C3: ORGAN-ACCESSOREN (einer je Slot, Reihenfolge == axis_names()) --------------------
    // Bewusst AUSGESCHRIEBEN statt makro-generiert (grep-Doktrin). Muster identisch zu set_anatomy.hpp.
    [[nodiscard]] typename Composition::memory_layout& memory_layout_organ() noexcept { return axis_memory_layout_; }
    [[nodiscard]] typename Composition::memory_layout const& memory_layout_organ() const noexcept {
        return axis_memory_layout_;
    }
    [[nodiscard]] typename Composition::value_handle&       value_handle_organ() noexcept { return axis_value_handle_; }
    [[nodiscard]] typename Composition::value_handle const& value_handle_organ() const noexcept {
        return axis_value_handle_;
    }
    [[nodiscard]] typename Composition::extent_policy& extent_policy_organ() noexcept { return axis_extent_policy_; }
    [[nodiscard]] typename Composition::extent_policy const& extent_policy_organ() const noexcept {
        return axis_extent_policy_;
    }
    [[nodiscard]] typename Composition::layout_policy& layout_policy_organ() noexcept { return axis_layout_policy_; }
    [[nodiscard]] typename Composition::layout_policy const& layout_policy_organ() const noexcept {
        return axis_layout_policy_;
    }
    [[nodiscard]] typename Composition::accessor_policy& accessor_policy_organ() noexcept {
        return axis_accessor_policy_;
    }
    [[nodiscard]] typename Composition::accessor_policy const& accessor_policy_organ() const noexcept {
        return axis_accessor_policy_;
    }

    /// axis_organ_names() -- Praesenz-DEKLARATION der real gehaltenen Organ-Member in Slot-Reihenfolge.
    /// Die WACHE dagegen lebt in der Test-TU (Kreuz gegen GenusBindingTraits<View>::axis_names()).
    [[nodiscard]] static constexpr std::array<std::string_view, 5> const& axis_organ_names() noexcept {
        static constexpr std::array<std::string_view, 5> kNames = {"memory_layout", "value_handle", "extent_policy",
                                                                   "layout_policy", "accessor_policy"};
        return kNames;
    }

    /// observe_axes() -- E-24 C3 / Luecke-L2-Rest: sammelt die Sub-Organ-Beobachtung Slot fuer Slot ein,
    /// exakt nach dem SA-Muster. Ein Slot ohne statistics() bleibt EmptyAxisSnapshot.
    [[nodiscard]] axis_observation_t observe_axes() const noexcept {
        axis_observation_t agg{};
        if constexpr (ObservableAxis<typename Composition::memory_layout>) {
            agg.memory_layout = axis_memory_layout_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::value_handle>) {
            agg.value_handle = axis_value_handle_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::extent_policy>) {
            agg.extent_policy = axis_extent_policy_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::layout_policy>) {
            agg.layout_policy = axis_layout_policy_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::accessor_policy>) {
            agg.accessor_policy = axis_accessor_policy_.statistics();
        }
        return agg;
    }

    /// Diagnose: wie viele Slots liefern echte Snapshots? (Namensgleich zum SA-Vorbild.)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return axis_observation_t::observable_count();
    }

private:
    element_type const* data_ = nullptr; // EXTERN (non-owning)
    std::size_t         size_ = 0;

    // E-24 C3: die 5 Slots als REALE Organ-Member. layout_policy/accessor_policy sind die getriebenen
    // (read() laeuft ueber sie, seit L-76c); memory_layout/value_handle/extent_policy werden GETRAGEN und
    // ueber ihre Accessoren getrieben -- R5.B-Grenze ehrlich. Init-Stil wie SearchAlgorithmAnatomy:194-221
    // (default-init OHNE `{}` fuer die getragenen, `{}` fuer die seit L-76c so gebauten Policy-Organe).
    typename Composition::memory_layout axis_memory_layout_;
    typename Composition::value_handle  axis_value_handle_;
    typename Composition::extent_policy axis_extent_policy_{};
    layout_t                            axis_layout_policy_{};
    accessor_t                          axis_accessor_policy_{};

    mutable ViewObserverSnapshot obs_{}; // mutable: read() ist const, zählt aber Zugriffe
};

} // namespace comdare::cache_engine::anatomy
