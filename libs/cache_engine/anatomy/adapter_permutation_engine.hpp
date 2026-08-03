#pragma once
// E-24 C2 / OP-3-ENTSCHEID (2026-08-04) -- AdapterPermutationEngine: die Gattungs-Spezialisierung der
// generischen PermutationEngine fuer die ADAPTER-Tier-Unterklasse der Container-Gattung (Wirbelloses,
// genus()==Adapter, gattung_of->Container), analog set_/sequence_/view_permutation_engine.hpp.
//
// WARUM DIESE DATEI NEU IST (Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md
// Paragraf 1.3, OP-3-ENTSCHEID -- am Ist nachgeprueft): per-Genus-Engines existierten fuer
// SearchAlgorithm/Set/Sequence/View; Adapter war die EINZIGE der fuenf Gattungen ohne eigene Engine. Der
// Entscheid lautet ausdruecklich "NEUE eigene anatomy/adapter_permutation_engine.hpp nach dem
// Geschwister-Muster", Begruendung im Dossier: die Engine-pro-Genus-Struktur IST der gebaute Kanon; eine
// Sonderloesung nur fuer Adapter waere die stille Wiedereinfuehrung eines Ganz-Tier-Achsen-Konfigurators
// (Memory-Kanon: keine Ganz-Tier-Achsen). Der generische anatomy_permutation_driver.hpp bleibt Treiber-Ebene
// und wird NICHT angefasst. Ausserdem ankuendigt die SA-Engine diese Datei seit V41 wortwoertlich:
// "Andere Gattungen (Sequence/Set/Adapter/View) folgen in V42" (search_algorithm_permutation_engine.hpp:13-15).
//
// Drei Bausteine (Reihenfolge und Zuschnitt exakt wie bei den drei Geschwistern):
//   (1) AdapterCompositionFromPermTuple<PermT> -- Brueckenkopf PermTuple<V0..V10> (11 Werte: 10 geteilt/
//       delegiert + inner_container) -> AdapterComposition<V0..V9, Inner>.
//   (2) AdapterPermutationEngine<TopicConfigSets...> -- Genus-Marker + Pflicht-Slot-Genus-Validierung +
//       technisch benannte Iteration (for_each_adapter materialisiert AdapterAnatomy<AdapterComp>).
//   (3) for_each_abi_adapter nach dem SA-Muster (search_algorithm_permutation_engine.hpp:174-185):
//       materialisiert AdapterAbiAdapter<...> je Permutation und liefert dem Visitor die ABI-Basis.
//
// EINE ASYMMETRIE, DIE HIER NICHT ERFUNDEN, SONDERN GEERBT WIRD: Set/Sequence/View tragen ihre Komposition
// in einer EIGENEN Datei (set_/sequence_/view_composition.hpp); die AdapterComposition liegt seit #87+#90
// zusammen mit der Anatomie in adapter_anatomy.hpp (dort :113-133). Diese Engine inkludiert deshalb NUR
// adapter_anatomy.hpp -- eine Datei-Aufteilung waere eine Umstrukturierung des Bestands und gehoert nicht in
// diesen ABI-neutralen Schnitt.
//
// ABI-NEUTRAL (a-Teil des Fensters): kein Byte an abi_adapter.hpp, an den Wire-PODs (adapter_tier.hpp), an
// abi/*_decl.hpp oder an den Stempel-/Fingerprint-Flaechen. Diese Datei RUFT die bestehende Adapter-Bruecke
// nur auf; die Cross-Genus-Sperre traegt weiterhin der AdapterAbiAdapter selbst (adapter_abi_adapter.hpp:28-30).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md Paragraf 29.2 + 32.3 (+ 28 Invertebrate, 26.4)
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]] [[technical-identifiers-over-metaphor]]

#include "anatomy_base.hpp"        // AnatomyGenus / AnatomyGattung / IAnatomyBase
#include "adapter_anatomy.hpp"     // AdapterComposition / IsAdapterComposition / AdapterAnatomy / DequeInner
#include "adapter_abi_adapter.hpp" // AdapterAbiAdapter (ABI-Materialisierung je Permutation)
#include "pruefling_merge.hpp"     // PrueflingSlotConcept / IsSlotOfGenus_v

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// -- (1) Composition-Factory: PermTuple<V0..V10> -> AdapterComposition (Adapter-Gattungs-Aritaet = 11) --
namespace detail {
template <class PermT>
struct AdapterCompositionFromPermTupleImpl;
template <template <class...> class PermTupleTmpl, class... Vs>
struct AdapterCompositionFromPermTupleImpl<PermTupleTmpl<Vs...>> {
    static_assert(sizeof...(Vs) == 11,
                  "Adapter-PermTuple muss exakt 11 Achsen-Werte enthalten (10 geteilt/delegiert + "
                  "inner_container; Doku 14 Paragraf 28 Invertebrate. INC-2c: telemetry / INC-2d: isa sind "
                  "System-Achsen, kein Slot. Der 11. Wert belegt inner_container und ueberschreibt den "
                  "Default DequeInner).");
    using type = AdapterComposition<Vs...>;
};
} // namespace detail

/// AdapterCompositionFromPermTuple<PermT> -- materialisiert eine AdapterComposition aus einem 11-Slot-PermTuple.
template <class PermT>
using AdapterCompositionFromPermTuple = typename detail::AdapterCompositionFromPermTupleImpl<PermT>::type;

// -- (2) Genus-Specialization der PermutationEngine --

/// AdapterPermutationEngine<TopicConfigSets...> -- Genus-Specialization fuer die Adapter-Tier-Unterklasse
/// (Wirbelloses) der Container-Gattung.
template <class... TopicConfigSets>
class AdapterPermutationEngine {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    static constexpr AnatomyGenus genus = AnatomyGenus::Adapter; ///< Ebene 2: Tier-Unterklasse
    /// Ebene 1: das Aussen-Interface. Nur die Adapter-Bindung fuehrt diesen Marker explizit -- genau so wie
    /// GenusBindingTraits<Adapter>::gattung (genus_binding_traits.hpp:69) und AdapterAnatomy::gattung()
    /// (adapter_anatomy.hpp:170) es bereits tun. Kein neues Vokabular, nur derselbe Marker an der Engine.
    static constexpr AnatomyGattung gattung = AnatomyGattung::Container;

    using all_permutations = typename Engine::AllPermutations;

    [[nodiscard]] static constexpr std::size_t count() noexcept { return Engine::count(); }
    [[nodiscard]] static constexpr std::size_t arity() noexcept { return Engine::arity; }

    /// Compile-Time-Check ob ein Pruefling-Slot zur Adapter-Gattung gehoert (Cross-Genus-Joins sind
    /// type-system-mathematisch unmoeglich, Doku 14 Paragraf 32).
    template <class Slot>
    static constexpr void assert_pruefling_slot_genus() noexcept {
        static_assert(pf::PrueflingSlotConcept<Slot>,
                      "Slot erfuellt PrueflingSlotConcept nicht (PrueflingVariants + has_pruefling).");
        static_assert(pf::IsSlotOfGenus_v<Slot, AnatomyGenus::Adapter>,
                      "Slot gehoert nicht zur Adapter-Gattung. Cross-Genus-Joins sind type-system-mathematisch "
                      "unmoeglich (Doku 14 Paragraf 32).");
    }

    /// Variadische Bulk-Validierung mehrerer Slots in einem Aufruf (Muster SA-Engine).
    template <class... Slots>
    static constexpr void assert_all_pruefling_slots_genus() noexcept {
        (assert_pruefling_slot_genus<Slots>(), ...);
    }

    template <class... Slots>
    static constexpr bool slots_match_genus_v = (pf::IsSlotOfGenus_v<Slots, AnatomyGenus::Adapter> && ...);

    /// for_each_adapter -- iteriert ueber alle Permutationen, instantiiert AdapterAnatomy<AdapterComp> und
    /// ruft Visitor(anatomy, name). Technisch benannte API (User-Direktive
    /// [[technical-identifiers-over-metaphor]]), analog for_each_set / for_each_sequence / for_each_view.
    template <class Visitor>
    static constexpr void for_each_adapter(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using AdapterComp = AdapterCompositionFromPermTuple<P>;
            AdapterAnatomy<AdapterComp> anatomy;
            std::forward<Visitor>(v)(anatomy, AdapterComp::name);
        });
    }

    /// for_each_composition_type -- Compile-Time-Visitor pro Adapter-Composition-Type (CacheEngineBuilder-Codegen).
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using AdapterComp = AdapterCompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<AdapterComp>();
        });
    }

    /// for_each_abi_adapter -- E-24 C2: materialisiert je Permutation einen
    /// AdapterAbiAdapter<AdapterAnatomy<AdapterComp>> und ruft den Visitor mit dessen ABI-Basis.
    ///
    /// Visitor-Signatur: `void operator()(IAnatomyBase& base, std::string_view composition_name)` -- BYTE-GLEICH
    /// zur SearchAlgorithm-Gattung (search_algorithm_permutation_engine.hpp:177-178). Der gattungs-agnostische
    /// Loader liefert IAnatomyBase*, der Adapter-Dock fragt daran IAdapterTier per dynamic_cast ab
    /// (adapter_abi_adapter.hpp:5-8).
    ///
    /// ABWEICHUNG im RUMPF (deklariert, gleicher Vertrag): die SA-Engine delegiert an for_each_search_algorithm
    /// und verwirft die dort gebaute Anatomie ungenutzt. Hier laeuft die Materialisierung direkt ueber
    /// Engine::for_each_permutation -- lokales Idiom der vier Container-Engines, kein doppelter Anatomie-Aufbau
    /// je Permutation.
    template <class Visitor>
    static constexpr void for_each_abi_adapter(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>() {
            using AdapterComp = AdapterCompositionFromPermTuple<P>;
            AdapterAbiAdapter<AdapterAnatomy<AdapterComp>> adapter;
            IAnatomyBase&                                  base = adapter;
            std::forward<Visitor>(v)(base, AdapterComp::name);
        });
    }
};

} // namespace comdare::cache_engine::anatomy
