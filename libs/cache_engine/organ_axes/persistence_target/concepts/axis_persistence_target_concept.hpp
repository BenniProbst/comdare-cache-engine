#pragma once
// STRUKT-R ORG-18 axis_persistence_target Achsen-Concept
// Vorbild 1:1 organ_axes/io_dispatch/concepts/axis_io_concept.hpp (Topic-Marker + EIN achsen-eigenes Praedikat).
//
// Das achsen-eigene Praedikat ist writes_back_to_disk(): es ist die Frage, die diese Achse ueberhaupt
// aufspannt (Session-Doc §5: "unterscheidet, ob ein Algorithmus nur in Memory arbeitet oder auf Platte
// zurueckschreiben muss"). Es ist NICHT dasselbe wie io_dispatch::is_in_memory_only() (Dispatch-Profil).

#include <topics/io/concepts/topic_io_concept.hpp>
#include <concepts>

namespace comdare::cache_engine::persistence_target::concepts {

template <typename P>
concept PersistenceTargetStrategy = ::comdare::cache_engine::io::concepts::IoComponent<P> && requires {
    { P::writes_back_to_disk() } noexcept -> std::convertible_to<bool>;
};

} // namespace comdare::cache_engine::persistence_target::concepts
