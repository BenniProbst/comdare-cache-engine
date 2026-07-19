#pragma once
// L-76a (2026-06-03) — SetPermutationEngine: die Gattungs-Spezialisierung der generischen PermutationEngine für die
// SET-Gattung (Vogel, genus()==Set), analog SearchAlgorithmPermutationEngine (Doku 14 Teil 4 §29.2 + §32).
//
// User-Direktive 2026-05-26 (§29.2): "Die Permutation Engine muss für die anatomischen Möglichkeiten jeder
// Anatomie-Gattung durch Unterklassen spezifiziert werden, die von der Haupt-Permutation-Engine erben." Die
// SearchAlgorithmPermutationEngine kündigt explizit an: "Andere Gattungen (Sequence/Set/Adapter/View) folgen in V42."
//
// Zwei Bausteine (analog composition_factory.hpp + search_algorithm_permutation_engine.hpp):
//   (1) SetCompositionFromPermTuple<PermT> — Brückenkopf PermTuple<V0..V13> (14 Werte; INC-2c ohne telemetry) → SetComposition<V0..V13>.
//   (2) SetPermutationEngine<TopicConfigSets...> — Genus-Marker + Pflicht-Slot-Genus-Validierung + technisch
//       benannte Iteration (for_each_set materialisiert SetAnatomy<SetComp> pro Permutation).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32.3
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]] [[technical-identifiers-over-metaphor]]

#include "anatomy_base.hpp"    // AnatomyGenus
#include "set_composition.hpp" // SetComposition / IsSetComposition
#include "set_anatomy.hpp"     // SetAnatomy
#include "pruefling_merge.hpp" // PrueflingSlotConcept / IsSlotOfGenus_v

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// ── (1) Composition-Factory: PermTuple<V0..V14> → SetComposition (Set-Gattungs-Arität = 15) ──
namespace detail {
template <class PermT>
struct SetCompositionFromPermTupleImpl;
template <template <class...> class PermTupleTmpl, class... Vs>
struct SetCompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(sizeof...(Vs) == 13,
                  "Set-PermTuple muss exakt 13 Achsen-Werte enthalten (INC-2c: telemetry / "
                  "INC-2d: isa sind System-Achsen) (Set-Gattung K-only, kein mapping/value_handle).");
    using type = SetComposition<Vs...>;
};
} // namespace detail

/// SetCompositionFromPermTuple<PermT> — materialisiert eine SetComposition aus einem 13-Slot-PermTuple.
template <class PermT>
using SetCompositionFromPermTuple = typename detail::SetCompositionFromPermTupleImpl<PermT>::type;

// ── (2) Genus-Specialization der PermutationEngine ──

/// SetPermutationEngine<TopicConfigSets...> — Genus-Specialization für die Set-Gattung (Vogel).
template <class... TopicConfigSets>
class SetPermutationEngine {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    static constexpr AnatomyGenus genus = AnatomyGenus::Set;
    using all_permutations              = typename Engine::AllPermutations;

    [[nodiscard]] static constexpr std::size_t count() noexcept { return Engine::count(); }
    [[nodiscard]] static constexpr std::size_t arity() noexcept { return Engine::arity; }

    /// Compile-Time-Check ob ein Prüfling-Slot zur Set-Gattung gehört (Cross-Genus-Joins type-unmöglich, Doku 14 §32).
    template <class Slot>
    static constexpr void assert_pruefling_slot_genus() noexcept {
        static_assert(pf::PrueflingSlotConcept<Slot>,
                      "Slot erfuellt PrueflingSlotConcept nicht (PrueflingVariants + has_pruefling).");
        static_assert(pf::IsSlotOfGenus_v<Slot, AnatomyGenus::Set>,
                      "Slot gehoert nicht zur Set-Gattung. Cross-Genus-Joins sind type-system-mathematisch "
                      "unmoeglich (Doku 14 §32).");
    }

    template <class... Slots>
    static constexpr bool slots_match_genus_v = (pf::IsSlotOfGenus_v<Slots, AnatomyGenus::Set> && ...);

    /// for_each_set — iteriert über alle Permutationen, instantiiert SetAnatomy<SetComp> + ruft Visitor(anatomy, name).
    template <class Visitor>
    static constexpr void for_each_set(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using SetComp = SetCompositionFromPermTuple<P>;
            SetAnatomy<SetComp> anatomy;
            std::forward<Visitor>(v)(anatomy, SetComp::name);
        });
    }

    /// for_each_composition_type — Compile-Time-Visitor pro Set-Composition-Type (für CacheEngineBuilder-Codegen).
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using SetComp = SetCompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<SetComp>();
        });
    }
};

} // namespace comdare::cache_engine::anatomy
