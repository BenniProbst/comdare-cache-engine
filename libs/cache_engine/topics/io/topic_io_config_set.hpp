#pragma once
// V41.F.6.1.R7.5.f TopicConfigSet fuer io-Topic
//
// STRUKT-R ORG-18: das io-Topic traegt jetzt 2 Achsen (vorher 1) -- axis_io (io_dispatch, WIE der Zugriff
// vermittelt wird) und axis_persistence_target (OB zurueckgeschrieben werden muss). Das 2-Achsen-Muster ist
// 1:1 topics/queuing/topic_queuing_config_set.hpp (StaticAxisVariants_Q1/_Q2 + Default auf die Haupt-Achse).

#include <topics/io/axis_io/axis_io_registry.hpp>
#include <topics/io/axis_persistence_target/axis_persistence_target_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::io {

namespace mp = boost::mp11;

struct TopicConfigSet {
    using StaticAxisVariants_IO = axis_io::EnabledIos;

    // STRUKT-R ORG-18 (Bauplan N-4): 18. Organ-Haupt-Achse persistence_target.
    using StaticAxisVariants_PT = axis_persistence_target::EnabledTargets;

    // Default-StaticAxisVariants bleibt BEWUSST axis_io: die PermutationEngine-1-Topic-Variante zieht
    // damit denselben Satz wie vor STRUKT-R -> keine stille Verhaltens-/Byte-Aenderung an bestehenden
    // Nutzern des io-Topics. persistence_target wird explizit ueber StaticAxisVariants_PT verdrahtet
    // (source_catalog/registry_to_axis_levels), analog Q2 unter queuing.
    using StaticAxisVariants = StaticAxisVariants_IO;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<requires { typename Wrapper::iterable_aspect_t; }, void, void>;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::io
