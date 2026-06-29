#pragma once
// V41.F.6.1 F.6 axis_01_page_type Standard-Concept (Achse 1 PAGE-TYPE, "6 Pflicht-Seitentypen")
//
// Materialisiert die in der Architektur (Saeule A, IPage) dokumentierte PAGE-TYPE-Achse:
// die Knoten-/Seiten-STRUKTUR-Familie (Dense/Patricia/Redirect/B+/...). Abgegrenzt von
// axis_04_node_type (= ART-Kapazitaetsklasse Node4/16/48/256) und axis_02_path_compression.

#include "../../concepts/topic_nodes_concept.hpp"
#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::nodes::axis_01_page_type::concepts {

/// Die 6 Pflicht-Seitentypen (Termin 4 Scope-Freeze, Saeule A).
enum class PageKind : std::uint8_t {
    DenseByte      = 0, // dichte byte-indizierte Seite
    ExtendedDense  = 1, // erweiterte dichte Seite (>256 Slots / mehrstufig)
    SparsePatricia = 2, // spaerlich, Patricia-komprimiert
    Redirect       = 3, // kollabierter eindeutiger Restpfad (CoCo-Trie)
    CustomCache    = 4, // PRT-Custom-Cache-Page (cache-line-orientiert)
    BPlus          = 5, // B+-artige Verzweigungsseite (Masstree-Familie)
};

/// PageTypeStrategy — Pflicht-API fuer Seitentyp-Strategien.
template <typename P>
concept PageTypeStrategy = ::comdare::cache_engine::nodes::concepts::NodesComponent<P> && requires {
    { P::page_kind() } noexcept -> std::convertible_to<PageKind>;
    { P::is_branch() } noexcept -> std::convertible_to<bool>;
    { P::is_leaf() } noexcept -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::nodes::axis_01_page_type::concepts
