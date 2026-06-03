#pragma once
// L-76b (2026-06-03) — SequencePermutationEngine: die Gattungs-Spezialisierung der PermutationEngine für die
// SEQUENCE-Gattung (Reptil, genus()==Sequence), analog SearchAlgorithmPermutationEngine (Doku 14 §29.2 + §32).
//
// Zwei Bausteine:
//   (1) SequenceCompositionFromPermTuple<PermT> — PermTuple<V0..V10> (11 Werte: 10 geteilte + axis_growth) →
//       SequenceComposition<V0..V10>. Der 11. Wert füllt den Growth-Slot (überschreibt den Default DoublingGrowth).
//   (2) SequencePermutationEngine<TopicConfigSets...> — Genus-Marker + Slot-Genus-Validierung + for_each_sequence
//       (materialisiert SequenceAnatomy<SeqComp> pro Permutation).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32.3
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy_base.hpp"           // AnatomyGenus
#include "sequence_composition.hpp"   // SequenceComposition / IsSequenceComposition / DoublingGrowth
#include "sequence_anatomy.hpp"       // SequenceAnatomy
#include "pruefling_merge.hpp"        // PrueflingSlotConcept / IsSlotOfGenus_v

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// ── (1) Composition-Factory: PermTuple<V0..V10> → SequenceComposition (Sequence-Gattungs-Arität = 11) ──
namespace detail {
template <class PermT> struct SequenceCompositionFromPermTupleImpl;
template <template <class...> class PermTupleTmpl, class... Vs>
struct SequenceCompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(sizeof...(Vs) == 11,
        "Sequence-PermTuple muss exakt 11 Achsen-Werte enthalten (10 geteilte + axis_growth).");
    using type = SequenceComposition<Vs...>;
};
}  // namespace detail

/// SequenceCompositionFromPermTuple<PermT> — materialisiert eine SequenceComposition aus einem 11-Slot-PermTuple.
template <class PermT>
using SequenceCompositionFromPermTuple = typename detail::SequenceCompositionFromPermTupleImpl<PermT>::type;

// ── (2) Genus-Specialization der PermutationEngine ──

/// SequencePermutationEngine<TopicConfigSets...> — Genus-Specialization für die Sequence-Gattung (Reptil).
template <class... TopicConfigSets>
class SequencePermutationEngine {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    static constexpr AnatomyGenus genus = AnatomyGenus::Sequence;
    using all_permutations = typename Engine::AllPermutations;

    [[nodiscard]] static constexpr std::size_t count() noexcept { return Engine::count(); }
    [[nodiscard]] static constexpr std::size_t arity() noexcept { return Engine::arity; }

    /// Compile-Time-Check ob ein Prüfling-Slot zur Sequence-Gattung gehört (Cross-Genus type-unmöglich, Doku 14 §32).
    template <class Slot>
    static constexpr void assert_pruefling_slot_genus() noexcept {
        static_assert(pf::PrueflingSlotConcept<Slot>,
                      "Slot erfuellt PrueflingSlotConcept nicht (PrueflingVariants + has_pruefling).");
        static_assert(pf::IsSlotOfGenus_v<Slot, AnatomyGenus::Sequence>,
                      "Slot gehoert nicht zur Sequence-Gattung. Cross-Genus-Joins sind type-system-mathematisch "
                      "unmoeglich (Doku 14 §32).");
    }

    template <class... Slots>
    static constexpr bool slots_match_genus_v = (pf::IsSlotOfGenus_v<Slots, AnatomyGenus::Sequence> && ...);

    /// for_each_sequence — iteriert über alle Permutationen, instantiiert SequenceAnatomy<SeqComp> + Visitor(anatomy, name).
    template <class Visitor>
    static constexpr void for_each_sequence(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using SeqComp = SequenceCompositionFromPermTuple<P>;
            SequenceAnatomy<SeqComp> anatomy;
            std::forward<Visitor>(v)(anatomy, SeqComp::name);
        });
    }

    /// for_each_composition_type — Compile-Time-Visitor pro Sequence-Composition-Type (für CacheEngineBuilder-Codegen).
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using SeqComp = SequenceCompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<SeqComp>();
        });
    }
};

}  // namespace comdare::cache_engine::anatomy
