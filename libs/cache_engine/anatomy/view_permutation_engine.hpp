#pragma once
// L-76c (2026-06-03) — ViewPermutationEngine: die Gattungs-Spezialisierung der PermutationEngine für die
// VIEW-Gattung (Pflanze, genus()==View, non-owning), analog SearchAlgorithmPermutationEngine (Doku 14 §29.2 + §32).
//
// Zwei Bausteine:
//   (1) ViewCompositionFromPermTuple<PermT> — PermTuple<V0..V6> (7 Werte: 4 geteilte + axis_extent/layout/accessor)
//       → ViewComposition<V0..V5>. Die Werte 4/5/6 füllen (INC-2c ohne telemetry) Extent/Layout/Accessor (überschreiben die Defaults).
//   (2) ViewPermutationEngine<TopicConfigSets...> — Genus-Marker + Slot-Genus-Validierung + for_each_view
//       (materialisiert ViewAnatomy<ViewComp> pro Permutation).
//
// E-24 C2 (2026-08-04, Bauplan-Dossier 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C2): DRITTER
// Baustein for_each_abi_adapter nach dem SA-Muster (search_algorithm_permutation_engine.hpp:174-185). Bis
// heute war die SA-Engine der EINZIGE Traeger dieser Materialisierung; die vier Container-Engines konnten je
// Permutation nur die Anatomie bzw. den Kompositions-TYP liefern, nicht die ABI-Flaeche (IAnatomyBase), an der
// der gattungs-agnostische Modul-Loader/Dock-Pfad ansetzt. ABI-NEUTRAL: kein Byte an abi_adapter.hpp, an den
// Wire-PODs (view_tier.hpp) oder an abi/*_decl.hpp -- diese Datei RUFT die bestehende Bruecke nur auf.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32.3
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy_base.hpp"     // AnatomyGenus / IAnatomyBase
#include "view_composition.hpp" // ViewComposition / IsViewComposition / Layout/Accessor/Extent-Policies
#include "view_anatomy.hpp"     // ViewAnatomy
#include "view_abi_adapter.hpp" // E-24 C2: ViewAbiAdapter (ABI-Materialisierung je Permutation)
#include "pruefling_merge.hpp"  // PrueflingSlotConcept / IsSlotOfGenus_v

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// ── (1) Composition-Factory: PermTuple<V0..V6> → ViewComposition (View-Gattungs-Arität = 7) ──
namespace detail {
template <class PermT>
struct ViewCompositionFromPermTupleImpl;
template <template <class...> class PermTupleTmpl, class... Vs>
struct ViewCompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(
        sizeof...(Vs) == 5,
        "View-PermTuple muss exakt 5 Achsen-Werte enthalten (INC-2d: isa raus) (2 geteilte + extent/layout/accessor).");
    using type = ViewComposition<Vs...>;
};
} // namespace detail

/// ViewCompositionFromPermTuple<PermT> — materialisiert eine ViewComposition aus einem 5-Slot-PermTuple.
template <class PermT>
using ViewCompositionFromPermTuple = typename detail::ViewCompositionFromPermTupleImpl<PermT>::type;

// ── (2) Genus-Specialization der PermutationEngine ──

/// ViewPermutationEngine<TopicConfigSets...> — Genus-Specialization für die View-Gattung (Pflanze, non-owning).
template <class... TopicConfigSets>
class ViewPermutationEngine {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    static constexpr AnatomyGenus genus = AnatomyGenus::View;
    using all_permutations              = typename Engine::AllPermutations;

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
        Engine::for_each_permutation([&]<class P>() {
            using ViewComp = ViewCompositionFromPermTuple<P>;
            ViewAnatomy<ViewComp> anatomy;
            std::forward<Visitor>(v)(anatomy, ViewComp::name);
        });
    }

    /// for_each_composition_type — Compile-Time-Visitor pro View-Composition-Type (für CacheEngineBuilder-Codegen).
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using ViewComp = ViewCompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<ViewComp>();
        });
    }

    /// for_each_abi_adapter -- E-24 C2: materialisiert je Permutation einen ViewAbiAdapter<ViewAnatomy<ViewComp>>
    /// und ruft den Visitor mit dessen ABI-Basis.
    ///
    /// Visitor-Signatur: `void operator()(IAnatomyBase& base, std::string_view composition_name)` -- BYTE-GLEICH
    /// zur SearchAlgorithm-Gattung (search_algorithm_permutation_engine.hpp:177-178). Der gattungs-agnostische
    /// Loader liefert IAnatomyBase*, der View-Dock fragt daran IViewTier per dynamic_cast ab. Die non-owning-
    /// Asymmetrie der View-Gattung (bind/read statt insert/erase/clear) bleibt unangetastet.
    ///
    /// ABWEICHUNG im RUMPF (deklariert, gleicher Vertrag): die SA-Engine delegiert an for_each_search_algorithm
    /// und verwirft die dort gebaute Anatomie ungenutzt. Hier laeuft die Materialisierung direkt ueber
    /// Engine::for_each_permutation -- lokales Idiom dieser Datei, kein doppelter Anatomie-Aufbau je Permutation.
    template <class Visitor>
    static constexpr void for_each_abi_adapter(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using ViewComp = ViewCompositionFromPermTuple<P>;
            ViewAbiAdapter<ViewAnatomy<ViewComp>> adapter;
            IAnatomyBase&                         base = adapter;
            std::forward<Visitor>(v)(base, ViewComp::name);
        });
    }
};

} // namespace comdare::cache_engine::anatomy
