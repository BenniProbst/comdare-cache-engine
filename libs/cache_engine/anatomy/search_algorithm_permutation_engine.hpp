#pragma once
// V41.F.6.1.R5.C.B — SearchAlgorithmPermutationEngine (Genus-Specialization)
//
// User-Direktive 2026-05-26 sehr spaet (Doku 14 Teil 4 §29.2 + §32):
// "Die Permutation Engine muss fuer die anatomischen Moeglichkeiten jeder
//  Anatomie-Gattungen durch Unterklassen spezifiziert werden, die von der
//  Haupt-Permutation-Engine erben."
// "Wir koennen nur gleiche Gattungen an Algorithmen miteinander kreuzen, weil
//  Gattungen die exakt selben permutativen Achsen verwenden. ... Pruefling-Slots
//  muessen Gattung explizit deklarieren — PermutationEngine prueft Gattung-Match
//  zur Compile-Zeit."
//
// SearchAlgorithmPermutationEngine ist die ERSTE Gattungs-Specialization. Andere
// Gattungen (SequencePermutationEngine/SetPermutationEngine/AdapterPermutationEngine/
// ViewPermutationEngine) folgen in V42.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §29.2 + §32.3
// @task #703 V41.F.6.1.R5.C.B
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]
//          [[technical-identifiers-over-metaphor]]

#include "abi_adapter.hpp"
#include "anatomy_base.hpp"
#include "anatomy_permutation_driver.hpp"
#include "pruefling_merge.hpp"

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace pf = ::comdare::cache_engine::anatomy::pruefling;

// ─────────────────────────────────────────────────────────────────────────────
// SearchAlgorithmPermutationEngine — genus-aware Specialization
// ─────────────────────────────────────────────────────────────────────────────

/// SearchAlgorithmPermutationEngine<TopicConfigSets...>
///
/// Genus-Specialization der generischen `PermutationEngine` fuer die
/// SearchAlgorithm-Gattung (Mammal in Tier-Metapher).
///
/// **Was diese Klasse zusaetzlich liefert (vs `AnatomyPermutationDriver`):**
/// 1. Compile-Time-Marker `genus == AnatomyGenus::SearchAlgorithm`
/// 2. `for_each_search_algorithm(visitor)` — technisch benannte Iteration
///    (vs `for_each_animal` mit Tier-Metapher in `AnatomyPermutationDriver`)
/// 3. R5.E-Vorbereitung: `for_each_abi_adapter(visitor)` materialisiert
///    `SearchAlgorithmAbiAdapter<...>` pro Permutation
///
/// **R5.C.B Pruefling-Slot-Validierung (Doku 14 §32.3):**
/// `assert_pruefling_slot_genus<Slot>()` — Compile-Time-Check ob Slot zur
/// SearchAlgorithm-Gattung gehoert. Cross-Genus-Slots brechen den Build mit
/// klarer Diagnostik.
///
/// **Verwendung:**
/// ```cpp
/// using Engine = SearchAlgorithmPermutationEngine<TopicConfigA, TopicConfigB>;
/// static_assert(Engine::genus == AnatomyGenus::SearchAlgorithm);
/// // Validierung Pruefling-Slot:
/// Engine::assert_pruefling_slot_genus<prt_art::axis_03a::Slot>();
/// // Iteration:
/// Engine::for_each_search_algorithm([](auto& algo, std::string_view name) {
///     // algo ist SearchAlgorithmAnatomy<AdHoc>
/// });
/// ```
template <class... TopicConfigSets>
class SearchAlgorithmPermutationEngine {
    using Driver = AnatomyPermutationDriver<TopicConfigSets...>;
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    // ─────────────────────────────────────────────────────────────────────
    // Gattungs-Marker (Doku 14 Teil 4 §27.2 + §29.2)
    // ─────────────────────────────────────────────────────────────────────
    static constexpr AnatomyGenus genus = AnatomyGenus::SearchAlgorithm;

    // ─────────────────────────────────────────────────────────────────────
    // Permutations-Raum-Inspektion (delegiert an PermutationEngine)
    // ─────────────────────────────────────────────────────────────────────
    using all_permutations = typename Engine::AllPermutations;

    [[nodiscard]] static constexpr std::size_t count() noexcept {
        return Engine::count();
    }

    [[nodiscard]] static constexpr std::size_t arity() noexcept {
        return Engine::arity;
    }

    // ─────────────────────────────────────────────────────────────────────
    // R5.C.B Pruefling-Slot-Validierung (Doku 14 §32.3 Gattungs-Constraint)
    // ─────────────────────────────────────────────────────────────────────

    /// assert_pruefling_slot_genus<Slot>() — Compile-Time-Check Pflicht-API.
    ///
    /// Loest static_assert mit klarer Diagnostik aus wenn der Slot NICHT zur
    /// SearchAlgorithm-Gattung gehoert. Pruefling-Repos sollten diese API
    /// frueh in ihren Header-Files aufrufen damit Cross-Genus-Mismatch sofort
    /// beim Pruefling-Einbinden auffaellt (nicht erst beim PermutationEngine-Bau).
    ///
    /// Beispiel:
    /// ```cpp
    /// // In prt-art/include/prt_art/axis_03a_slot.hpp:
    /// namespace prt_art::axis_03a {
    ///     struct Slot {
    ///         using PrueflingVariants = mp::mp_list<PrtArtRadix512>;
    ///         static constexpr bool has_pruefling = true;
    ///         static constexpr AnatomyGenus genus = AnatomyGenus::SearchAlgorithm;
    ///     };
    /// }
    /// // Validierung:
    /// SearchAlgorithmPermutationEngine<...>::assert_pruefling_slot_genus<prt_art::axis_03a::Slot>();
    /// ```
    template <class Slot>
    static constexpr void assert_pruefling_slot_genus() noexcept {
        static_assert(pf::PrueflingSlotConcept<Slot>,
                      "Slot erfuellt PrueflingSlotConcept nicht (PrueflingVariants + has_pruefling).");
        static_assert(pf::IsSlotOfGenus_v<Slot, AnatomyGenus::SearchAlgorithm>,
                      "Slot gehoert nicht zur SearchAlgorithm-Gattung. "
                      "Cross-Genus-Joins sind type-system-mathematisch unmoeglich "
                      "(Doku 14 §32). Pruefling-Slot muss "
                      "'static constexpr AnatomyGenus genus = AnatomyGenus::SearchAlgorithm' "
                      "deklarieren oder Default lassen (EmptyPrueflingSlot).");
    }

    /// assert_all_pruefling_slots_genus<Slots...>() — Variadic-Variant fuer
    /// Bulk-Validierung mehrerer Slots in einem Call.
    template <class... Slots>
    static constexpr void assert_all_pruefling_slots_genus() noexcept {
        (assert_pruefling_slot_genus<Slots>(), ...);
    }

    // ─────────────────────────────────────────────────────────────────────
    // R5.C.B Genus-Conformance-Concept (fuer externe Validierung)
    // ─────────────────────────────────────────────────────────────────────

    /// Compile-Time-Predicate: alle uebergebenen Slots passen zur Gattung?
    template <class... Slots>
    static constexpr bool slots_match_genus_v =
        (pf::IsSlotOfGenus_v<Slots, AnatomyGenus::SearchAlgorithm> && ...);

    // ─────────────────────────────────────────────────────────────────────
    // Iteration: technisch benannte Visitor-API (Doku-Konvention §41)
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief for_each_search_algorithm — iteriert ueber alle Permutationen,
     *        instantiiert SearchAlgorithmAnatomy<AdHoc> und ruft Visitor mit
     *        (algo, name).
     *
     * Visitor-Signatur: `void operator()(SearchAlgorithmAnatomy<AdHoc>& algo,
     *                                     std::string_view composition_name)`.
     *
     * Aequivalent zu `AnatomyPermutationDriver::for_each_animal` aber mit
     * technischem API-Namen (User-Direktive [[technical-identifiers-over-metaphor]]).
     */
    template <class Visitor>
    static constexpr void for_each_search_algorithm(Visitor&& v) {
        Driver::for_each_animal(std::forward<Visitor>(v));
    }

    /**
     * @brief for_each_composition_type — Compile-Time-Visitor pro Composition-Type.
     *
     * Visitor-Signatur: `template <class Composition> void operator()()`.
     * Identisch zur AnatomyPermutationDriver-Variante — verwendet fuer
     * CacheEngineBuilder-Codegen (R5.D).
     */
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Driver::for_each_composition_type(std::forward<Visitor>(v));
    }

    /**
     * @brief for_each_abi_adapter — R5.E-Vorbereitung: materialisiert
     *        SearchAlgorithmAbiAdapter<...> pro Permutation und ruft Visitor.
     *
     * Visitor-Signatur: `void operator()(IAnatomyBase& base,
     *                                     std::string_view composition_name)`.
     *
     * Wird in R5.E (Module-Loader) verwendet — der CacheEngineBuilder loadet
     * pro Permutation ein .so/.dll dessen extern "C" Factory genau eine
     * `SearchAlgorithmAbiAdapter`-Instanz exportiert.
     */
    template <class Visitor>
    static constexpr void for_each_abi_adapter(Visitor&& v) {
        Driver::for_each_animal([&]<class Algo>(Algo& algo, std::string_view name) {
            // Wrappe Anatomie in Production-AbiAdapter
            SearchAlgorithmAbiAdapter<Algo> adapter;
            IAnatomyBase& base = adapter;
            std::forward<Visitor>(v)(base, name);
            (void)algo;  // Algo wurde fuer Adapter-Instantiation gebraucht
        });
    }
};

}  // namespace comdare::cache_engine::anatomy
