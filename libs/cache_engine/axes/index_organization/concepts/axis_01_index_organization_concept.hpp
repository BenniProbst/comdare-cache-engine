#pragma once
// V41.F.6.1.R7.5.h axis_01_index_organization Strategy-Concept (Clustering)
//
// User-Direktive 2026-05-27: Achse = Sub-Aufgabe (Organ), nicht Container-
// Familie (Gattung). axis_01 modelliert Clustering-Strategie nach
// Garcia-Molina/Ullman "Database Systems" + Oracle/SQL Server-Theorie.
//
// `is_clustered()` = Index-Order entspricht Storage-Order
// `has_secondary_indexes()` = mehrere Indizes pro Tabelle moeglich
// `data_embedded_in_leaf()` = Daten direkt in Index-Leaf-Pages (IOT)

#include <topics/search_engine/concepts/topic_search_engine_concept.hpp>
#include <concepts>

namespace comdare::cache_engine::index_organization::concepts {

template <typename I>
concept IndexOrganizationStrategy =
    ::comdare::cache_engine::search_engine::concepts::SearchEngineComponent<I> && requires {
        { I::is_clustered() } noexcept -> std::convertible_to<bool>;
        { I::has_secondary_indexes() } noexcept -> std::convertible_to<bool>;
        { I::data_embedded_in_leaf() } noexcept -> std::convertible_to<bool>;
    };

} // namespace comdare::cache_engine::index_organization::concepts
