#pragma once
// V41.F.6.1.D Topic-Config-Set fuer Allocator-Topic (2026-05-26)
//
// @topic allocator
// @stand V41.F.6.1.D Stufe 4
//
// **Doku-Referenz:** §15.7 (TopicConfigSet + CacheEngineBuilder), §15.4 (PermutationEngine)
//
// Jedes Topic stellt ein constexpr-Config-Set bereit, das PermutationEngine
// konsumiert. Pflicht-Member:
//   - StaticAxisVariants  = mp_list aller enabled Vendor (= axis_06::EnabledVendors)
//   - AspectIterations<V> = void wenn kein iterable_aspect_t, sonst Vendor::iterable_aspect_t
//   - aspect_values<V>()  = leerer span / Vendor::iterable_values()
//
// **F.6.1.E iterable_aspect_t (hybride Laufzeit-Permutation):**
//   Heute Skelett: kein Vendor hat noch iterable_aspect_t. F.6.1.E baut das Pattern
//   in 1+ Achs-Variant ein (z.B. concurrency thresholds).

#include <topics/allocator/axis_06_allocator/axis_06_allocator_registry.hpp>

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::allocator {

/**
 * @brief TopicConfigSet — zentrale Konfiguration fuer Topic `allocator`
 *
 * Wird von PermutationEngine<TopicConfigSets...> konsumiert. Kombiniert
 * statische Achs-Varianten (EnabledVendors aus axis_06-Registry) mit dem
 * optional dynamischen iterable_aspect_t pro Vendor (F.6.1.E).
 */
struct TopicConfigSet {
    /// MP11-Liste aller enabled Vendor (= mp_filter ueber AllVendors mit is_enabled)
    using StaticAxisVariants = axis_06_allocator::EnabledVendors;

    /**
     * @brief Pro-Vendor iterable Aspekt-Typ (F.6.1.E hybride Laufzeit-Permutation)
     *
     * Default: void (kein iterable_aspect_t). Vendor kann selbst typename
     * iterable_aspect_t = std::size_t (etc.) definieren — dann generiert
     * PermutationEngine Hybrid-Variant (1 Binary + Runtime-Loop, siehe Doku §15.5).
     */
    template <class Vendor>
    using AspectIterations = std::conditional_t<requires { typename Vendor::iterable_aspect_t; },
                                                void, // bis F.6.1.E aktiv: immer void (Skelett)
                                                void>;

    /**
     * @brief Pro-Vendor iterable Werte (F.6.1.E Stufe 3+)
     *
     * Heute leerer span. F.6.1.E ergaenzt: wenn Vendor::iterable_values()
     * vorhanden, liefere diese (Compile-Time-Auswertung via if constexpr).
     */
    template <class /*Vendor*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

} // namespace comdare::cache_engine::allocator
