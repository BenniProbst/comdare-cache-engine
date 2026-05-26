#pragma once
// V41.F.6.1.R4 — AnatomyPermutationDriver: PermutationEngine + Anatomie-Brueckenkopf
//
// Iteriert ueber alle Cartesian-Permutationen einer PermutationEngine und
// instantiiert pro Permutation eine SearchAlgorithmAnatomy<AdHocComposition>.
// Visitor erhaelt fertige Algorithmus-Instanz und kann messen/builden.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §14.3
// @task V41.F.6.1.R4

#include "composition_factory.hpp"
#include "search_algorithm_anatomy.hpp"

#include <src/permutations/permutation_engine.hpp>

#include <cstddef>

namespace comdare::cache_engine::anatomy {

namespace pe = ::comdare::cache_engine::permutations;
namespace mp = boost::mp11;

/// AnatomyPermutationDriver<TopicConfigSets...> — Spezialisierte
/// PermutationEngine-Facade die jeden Permutations-Punkt zu einer
/// SearchAlgorithmAnatomy<AdHocComposition> materialisiert.
///
/// Pflicht: TopicConfigSets muessen in der Topic-Slot-Convention-Reihenfolge
/// uebergeben werden (siehe AdHocComposition T0-T16 in composition_factory.hpp).
template <class... TopicConfigSets>
class AnatomyPermutationDriver {
    using Engine = pe::PermutationEngine<TopicConfigSets...>;

public:
    using all_permutations = typename Engine::AllPermutations;

    /// Anzahl Cartesian-Produkt-Punkte (Tiere) im Permutations-Raum
    [[nodiscard]] static constexpr std::size_t count() noexcept {
        return Engine::count();
    }

    /// Anzahl Topic-Achsen (Pflicht: 17 fuer vollstaendige Anatomie)
    [[nodiscard]] static constexpr std::size_t arity() noexcept {
        return Engine::arity;
    }

    /**
     * @brief Iteriert ueber alle Permutationen und ruft Visitor mit dem
     *        instantiierten Algorithmus + composition_name.
     *
     * Visitor-Signatur: `void operator()(AlgoInstance&, std::string_view name)`.
     *
     * Beispiel:
     * @code
     *   AnatomyPermutationDriver<...>::for_each_animal(
     *       [](auto& algo, std::string_view name){
     *           algo.insert(42, 4242);
     *           std::cout << name << ": size=" << algo.size() << '\n';
     *       });
     * @endcode
     */
    template <class Visitor>
    static constexpr void for_each_animal(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using AdHoc = CompositionFromPermTuple<P>;
            SearchAlgorithmAnatomy<AdHoc> anatomy;
            // R5.B: Visitor erhaelt Anatomie + Name (KEINE Container-API mehr in Anatomie)
            // Fuer Container-Ops Builder muss AnatomyExecutionContext<AdHoc> wrappen.
            std::forward<Visitor>(v)(anatomy, AdHoc::name);
        });
    }

    /**
     * @brief Compile-Time-Visitor mit Typen-Information (kein Runtime-Algorithmus).
     *
     * Visitor-Signatur: `template <class Composition> void operator()()`.
     * Verwendet fuer CacheEngineBuilder-Integration (jeden Composition-Type
     * zu eigenem .so/.dll builden).
     */
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        Engine::for_each_permutation([&]<class P>(){
            using AdHoc = CompositionFromPermTuple<P>;
            std::forward<Visitor>(v).template operator()<AdHoc>();
        });
    }
};

}  // namespace comdare::cache_engine::anatomy
