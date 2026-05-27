#pragma once
// V41.F.6.1.R7.1.a.2 Topic-Config-Set fuer Hardware-Topic
//
// @topic hardware
//
// Analog queuing-Topic (auch 2 Achsen): TopicConfigSet bietet beide Achsen
// separat fuer PermutationEngine + ein kombiniertes Cartesian-Product fuer
// ISA x Plattform.
//
// **Doku-Referenz:** Doku 11 §15.7 (TopicConfigSet + CacheEngineBuilder),
// §15.4 (PermutationEngine)
//
// Pflicht-Member analog axis_06_allocator/topic_allocator_config_set.hpp:
//   - StaticAxisVariants_<id> pro Achse
//   - StaticAxisVariants als Default (zeigt auf Haupt-Achse)
//   - CartesianISAxPlatform fuer kombinierte Permutationen
//   - AspectIterations<V> Pflicht-Template fuer F.6.1.E (heute Skelett)
//   - aspect_values<V>() leerer span (heute Skelett)

#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_registry.hpp>

// R7.5.i: axis_09_isa hat jetzt eigene Registry (4 ISAs: Scalar/Sse2/Avx2/Neon).
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>

namespace comdare::cache_engine::hardware {

namespace mp = boost::mp11;

/**
 * @brief TopicConfigSet — zentrale Konfiguration fuer Topic `hardware`
 *
 * Wird von PermutationEngine<TopicConfigSets...> konsumiert. Kombiniert
 * statische Achs-Varianten beider Achsen (axis_09 ISA + axis_12 Platform)
 * mit optional dynamischen iterable_aspect_t pro Wrapper (F.6.1.E).
 */
struct TopicConfigSet {
    // axis_09_isa: ISA-Family (R7.5.i Goldstandard: Scalar/Sse2/Avx2/Neon)
    using StaticAxisVariants_09 = axis_09_isa::EnabledIsas;

    // axis_12_general_hardware: Plattform-Familie (Generic/X86_64/Aarch64)
    using StaticAxisVariants_12 = axis_12_general_hardware::EnabledPlatforms;

    // Default-StaticAxisVariants — PermutationEngine 1-Topic-Variante nimmt axis_12
    // (Plattform-Konfiguration ist die uebergeordnete Achse, ISA als Sub-Permutation)
    using StaticAxisVariants = StaticAxisVariants_12;

    // Cartesian-Product axis_09 x axis_12 (1 x 3 = 3, mit eigener axis_09-Registry spaeter Nx3)
    using CartesianIsa09xPlatform12 = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_09,
        StaticAxisVariants_12
    >;

    /**
     * @brief Pro-Vendor iterable Aspekt-Typ (F.6.1.E hybride Laufzeit-Permutation)
     *
     * Default: void (kein iterable_aspect_t). Vendor kann selbst typename
     * iterable_aspect_t = std::size_t (etc.) definieren — dann generiert
     * PermutationEngine Hybrid-Variant (1 Binary + Runtime-Loop).
     */
    template <class Vendor>
    using AspectIterations = std::conditional_t<
        requires { typename Vendor::iterable_aspect_t; },
        void,  // bis F.6.1.E aktiv: immer void (Skelett)
        void
    >;

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

}  // namespace comdare::cache_engine::hardware
