#pragma once
// V41.F.6.1.R7.1.b TopicConfigSet fuer memory_layout-Topic
//
// @topic memory_layout
//
// 1-Achse Topic (axis_05 ist die einzige Layout-Achse). TopicConfigSet
// liefert StaticAxisVariants direkt aus EnabledLayouts der Achse.
//
// Doku-Referenz: Doku 11 §15.7 (TopicConfigSet + CacheEngineBuilder).
//
// Pflicht-Member analog axis_06_allocator/topic_allocator_config_set.hpp:
//   - StaticAxisVariants_<id> pro Achse (hier nur _05)
//   - StaticAxisVariants als Default
//   - AspectIterations<V> Pflicht-Template (heute Skelett)
//   - aspect_values<V>() Pflicht-Helper (heute leerer span)

#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_registry.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>   // V42 L-74c: ObservableMemoryLayout-Huelle

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::memory_layout {

namespace mp = boost::mp11;

// V42 L-74c: macht den Permutations-Pfad memory_layout-OBSERVABLE (analog telemetry, Doc 29 §3e). Die
// Huelle reicht cache_line_size/name/topic_tag durch → als L in ComposedStore<N,L,A> einsetzbar (bewiesen).
template <class S> using make_observable_layout = ::comdare::cache_engine::layout::ObservableMemoryLayout<S>;

/// TopicConfigSet — zentrale Konfiguration fuer Topic `memory_layout`.
///
/// 1-Achse Topic: axis_05 ist die einzige Memory-Layout-Achse.
/// StaticAxisVariants = EnabledLayouts der Achse.
struct TopicConfigSet {
    // axis_05_memory_layout: Layout-Family (Cache-Line/AoS/SoA/Bitmap) — V42 L-74c: observable Huellen
    using StaticAxisVariants_05 = mp::mp_transform<make_observable_layout, axis_05_memory_layout::EnabledLayouts>;

    // Default-StaticAxisVariants — die einzige Achse
    using StaticAxisVariants = StaticAxisVariants_05;

    /// Pro-Wrapper iterable Aspekt-Typ (F.6.1.E hybride Laufzeit-Permutation).
    /// Default: void (kein iterable_aspect_t). Wrapper kann selbst
    /// `typename Wrapper::iterable_aspect_t = std::size_t` definieren — dann
    /// generiert PermutationEngine Hybrid-Variant (1 Binary + Runtime-Loop).
    template <class Wrapper>
    using AspectIterations = std::conditional_t<
        requires { typename Wrapper::iterable_aspect_t; },
        void,
        void
    >;

    /// Pro-Wrapper iterable Werte (F.6.1.E Stufe 3+). Heute leerer span.
    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

}  // namespace
