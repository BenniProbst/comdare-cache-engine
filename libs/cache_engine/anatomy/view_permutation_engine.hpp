#pragma once
// L-76c (2026-06-03) — ViewPermutationEngine: die Gattungs-Spezialisierung der PermutationEngine für die
// VIEW-Gattung (Pflanze, genus()==View, non-owning), analog SearchAlgorithmPermutationEngine (Doku 14 §29.2 + §32).
//
// Zwei Bausteine:
//   (1) ViewCompositionFromPermTuple<PermT> — PermTuple<V0..V6> (7 Werte: 4 geteilte + axis_extent/layout/accessor)
//       → ViewComposition<V0..V6>. Die Werte 5/6/7 füllen Extent/Layout/Accessor (überschreiben die Defaults).
//   (2) ViewPermutationEngine<TopicConfigSets...> — Genus-Marker + Slot-Genus-Validierung + for_each_view
//       (materialisiert ViewAnatomy<ViewComp> pro Permutation).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32.3
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy_base.hpp"       // AnatomyGenus
#include "view_composition.hpp"   // ViewComposition / IsViewComposition / Layout/Accessor/Extent-Policies
#include "view_anatomy.hpp"       // ViewAnatomy
#include "pruefling_merge.hpp"    // PrueflingSlotConcept / IsSlotOfGenus_v

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// ── (1) Composition-Factory: PermTuple<V0..V6> → ViewComposition (View-Gattungs-Arität = 7) ──
namespace detail {
template <class PermT> struct ViewCompositionFromPermTupleImpl;
template <template <class...> class PermTupleTmpl, class... Vs>
struct ViewCompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(sizeof...(Vs) == 7,
        "View-PermTuple muss exakt 7 Achsen-Werte enthalten (4 geteilte + extent/layout/accessor).");
    using type = ViewComposition<Vs...>;
};
}  // namespace detail

/// ViewCompositionFromPermTuple<PermT> — materialisiert eine ViewComposition aus einem 7-Slot-PermTuple.
template <class PermT>
using ViewCompositionFromPermTuple = typename detail::ViewCompositionFromPermTupleImpl<PermT>::type;

// ── (2) Genus-Specialization der PermutationEngine ──

/// ViewPermutationEngine<TopicConfigSets...> — Genus-Specialization für die View-Gattung (Pflanze, non-owning).
template <class... TopicConfigSets>
class ViewPermutationEngine {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    static constexpr AnatomyGenus genus = AnatomyGenus::View;
    using all_permutations = typename Engine::AllPermutations;

    [[nodiscard]] static constexpr std::size_t count() noexcept { return Engine::count(); }
    [[nodiscard]] static constexpr std::size_t arity() noexcept { return Engine::arity; }

    /// Compile-Time-Check ob ein Prüfling-Slot zur View-Gattung gehört (Cross-Genus type-unmöglich, Doku 14 §32).
    template <class Slot>
    static constexpr void assert_pruefling_slot_genus() noexcept {
        static_assert(pf::PrueflingSlotConcept<Slot>,
                      "Slot erfuellt PrueflingSlotConcept nicht (PrueflingVariants + has_pruefling).");
        static_assert(pf::IsSlotOfGenus_v<Slot, AnatomyGenus::View>,
                      "Slot gehoert nicht zur View-Gattung. Cross-Genus-Joins sind type-system-mathematisch "
                      "unmoeglich (Doku 14 §32).");
    }

    template <class... Slots>
    static constexpr bool slots_match_genus_v = (pf::IsSlotOfGenus_v<Slots, AnatomyGenus::View> && ...);

    /// for_each_view — iteriert über alle Permutationen, instantiiert ViewAnatomy<ViewComp> + Visitor(anatomy, name).
    template <class Visitor>
    static constexpr void for_each_view(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using ViewComp = ViewCompositionFromPermTuple<P>;
            ViewAnatomy<ViewComp> anatomy;
            std::forward<Visitor>(v)(anatomy, ViewComp::name);
        });
    }

    /// for_each_composition_type — Compile-Time-Visitor pro View-Composition-Type (für CacheEngineBuilder-Codegen).
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using ViewComp = ViewCompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<ViewComp>();
        });
    }
};

}  // namespace comdare::cache_engine::anatomy
