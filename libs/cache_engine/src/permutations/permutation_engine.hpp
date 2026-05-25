#pragma once
// V41.F.6.1.D Stufe 4 PermutationEngine — Cartesian-Product ueber Topic-Achsen (2026-05-26)
//
// @stand V41.F.6.1.D Stufe 4
//
// **User-Direktive 2026-05-26 (Doku §15.4 + §15.7):**
//   PermutationEngine ist die zentrale Klasse die alle Topic-Achsen-Listen
//   (EnabledVendors aus jeweils einer Achs-Registry) zur Compile-Time in ein
//   Kartesisches Produkt aufloest. Pro Permutation eine Binary (statisch),
//   spaeter via CacheEngineBuilder (F.6.1.G) mit CLI-Flags gebaut.
//
// **F.6.1.H Pflicht-Constraint (User-Direktive 2026-05-26 / Doku §15.9):**
//   Pro Topic-Achse muss min. 1 Vendor enabled sein. Sonst kann CacheEngineBuilder
//   versehentlich eine 0-Variant-Achse konfigurieren → Build-Crash. Frueher
//   Compile-Fail mit klarer Diagnostik statt spaeter Linker-/Runtime-Crash.
//
// **MP11-Idioms (User-Direktive 2026-05-26 nachgelesen Boost.MP11 Reference):**
//   - mp_product<F, L1, L2, ...>     Cartesian-Product (statisch + variadisch)
//   - mp_for_each<L>(visitor)        Compile-Time-Visitor mit Tag-Dispatch
//   - mp_all_of<L, P>                Pflicht-Constraint pro Achse
//   - mp_count_if<L, P>              Diagnose-Count fuer leere Achsen
//   - mp_filter<P, L> / mp_filter_q  Pruefling-Predicate-Filterung
//   - mp_append<L1, L2, ...>         Full-Join Multi-Pruefling (Stufe 3, future)
//   - mp_unique<L>                   Non-Redundant Union (Stufe 3)

#include <boost/mp11.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::permutations {

namespace mp = boost::mp11;

// ───────────────────────────────────────────────────────────────────────────
// (1) PermTuple<Vs...> — eine vollstaendige Achsen-Permutation
// ───────────────────────────────────────────────────────────────────────────
//
// Cartesian-Product Element: ein Tuple von Vendor-Klassen, eine pro Topic.
// Hash via FNV-1a ueber V::name() (Wrapper-Pflicht-API, nicht variant_hash —
// die Achs-Wrapper haben kein variant_hash, sondern static name()).

template <class... Vs>
struct PermTuple {
    using variants = mp::mp_list<Vs...>;
    static constexpr std::size_t arity = sizeof...(Vs);

    /**
     * @brief FNV-1a Compile-Time-Hash ueber V::name() aller Vendor.
     *
     * Stabil ueber Builds (basiert auf Wrapper-namen-Konstanten, nicht Type-IDs).
     * Pro Permutation eine eindeutige uint64_t als Build-Verzeichnis-Suffix.
     */
    [[nodiscard]] static constexpr std::uint64_t hash() noexcept {
        std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a offset basis
        auto hash_one = [&h](std::string_view s) constexpr {
            for (char c : s) {
                h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
                h *= 0x100000001b3ULL;
            }
            // Separator zwischen Vendor-Namen damit "abc"+"def" != "ab"+"cdef"
            h ^= 0x7CULL;  // '|'
            h *= 0x100000001b3ULL;
        };
        (hash_one(Vs::name()), ...);
        return h;
    }
};

// ───────────────────────────────────────────────────────────────────────────
// (2) MP11-Helper: TopicConfigSet Predicate (Pflicht: Achse nicht-leer)
// ───────────────────────────────────────────────────────────────────────────

/// Compile-Time-Predicate: TopicConfigSet hat min. 1 Vendor in StaticAxisVariants
template <class TopicConfig>
using has_non_empty_axis = mp::mp_bool<
    (mp::mp_size<typename TopicConfig::StaticAxisVariants>::value > 0)
>;

// ───────────────────────────────────────────────────────────────────────────
// (3) PermutationEngine<TopicConfigSets...>
// ───────────────────────────────────────────────────────────────────────────
//
// Verlangt von jedem TopicConfigSet:
//   - typename StaticAxisVariants  = mp_list<Vendor1, Vendor2, ...>
//     (typischerweise EnabledVendors aus axis_<NN>_<topic>_registry.hpp)

template <class... TopicConfigSets>
class PermutationEngine {
    using Topics = mp::mp_list<TopicConfigSets...>;

public:
    // F.6.1.H Pflicht (Doku §15.9, User-Direktive 2026-05-26):
    // Pro Topic-Achse muss min. 1 Vendor enabled sein.
    // MP11-Idiom: mp_all_of statt Fold-Pattern (idiomatisch + Diagnose-tauglich).
    static_assert(
        mp::mp_all_of<Topics, has_non_empty_axis>::value,
        "PermutationEngine: jede Topic-Achse muss min. 1 enabled Vendor haben. "
        "Pruefe COMDARE_AXIS_<NN>_ENABLE_<VENDOR> CMake-Flags pro Achse — "
        "sonst kann CacheEngineBuilder eine 0-Achsen-Permutation starten."
    );

    /// Anzahl der Topic-Achsen (statische Compile-Time-Konstante)
    static constexpr std::size_t arity = sizeof...(TopicConfigSets);

    /// Diagnose-Helper: Anzahl der NICHT-leeren Achsen (entweder == arity oder Static-Assert greift)
    static constexpr std::size_t non_empty_axis_count =
        mp::mp_count_if<Topics, has_non_empty_axis>::value;

    /// Vollstaendige Cartesian-Product-Liste aller Permutationen (mp_list von PermTuple)
    using AllPermutations = mp::mp_product<
        PermTuple,
        typename TopicConfigSets::StaticAxisVariants...
    >;

    /// Anzahl der Permutationen (Compile-Time-Konstante)
    [[nodiscard]] static constexpr std::size_t count() noexcept {
        return mp::mp_size<AllPermutations>::value;
    }

    /**
     * @brief Compile-Time-Iteration ueber alle Permutationen
     *
     * Visitor wird mit `Visitor::operator()<P>()` aufgerufen pro Permutation.
     *
     * Verwendung im CacheEngineBuilder (F.6.1.G):
     *   PermutationEngine<...>::for_each_permutation([]<class P>(){
     *       constexpr auto h = P::hash();
     *       std::string cmake_cmd = build_cmake_command_for<P>(h);
     *       std::system(cmake_cmd.c_str());
     *   });
     */
    template <class Visitor>
    static constexpr void for_each_permutation(Visitor&& v) {
        mp::mp_for_each<AllPermutations>([&]<class P>(P){
            std::forward<Visitor>(v).template operator()<P>();
        });
    }

    /**
     * @brief Pruefling-spezifische Filterung (V41.F.6.3 Pattern)
     *
     * MP11-Idiom: mp_filter<P, L> mit template-template Predicate.
     * Predicate erhaelt jede Permutation (PermTuple<...>) und liefert mp_bool.
     *
     * Verwendung Stufe 2 Pruefling-einzeln:
     *   template <class P>
     *   using compatible_with_prt_art = mp::mp_bool<...>;  // user-defined
     *
     *   Engine::for_each_filtered<compatible_with_prt_art>([]<class P>(){ ... });
     */
    template <template <class> class Predicate, class Visitor>
    static constexpr void for_each_filtered(Visitor&& v) {
        using Filtered = mp::mp_filter<Predicate, AllPermutations>;
        mp::mp_for_each<Filtered>([&]<class P>(P){
            std::forward<Visitor>(v).template operator()<P>();
        });
    }

    /**
     * @brief Pruefling-spezifische Filterung mit gequoteter Predicate (MP11-Idiom)
     *
     * Alternative zu for_each_filtered fuer Quote-basierte Higher-Order-Predicates:
     *   using PrtArtCompatible = mp::mp_bind_front<is_compatible, prt_art_tag>;
     *   Engine::for_each_filtered_q<PrtArtCompatible>([]<class P>(){ ... });
     */
    template <class QuotedPredicate, class Visitor>
    static constexpr void for_each_filtered_q(Visitor&& v) {
        using Filtered = mp::mp_filter_q<QuotedPredicate, AllPermutations>;
        mp::mp_for_each<Filtered>([&]<class P>(P){
            std::forward<Visitor>(v).template operator()<P>();
        });
    }

    /// Diagnose: Anzahl Permutationen die ein Predicate erfuellen (ohne Iteration)
    template <template <class> class Predicate>
    static constexpr std::size_t count_filtered() noexcept {
        return mp::mp_count_if<AllPermutations, Predicate>::value;
    }
};

// ───────────────────────────────────────────────────────────────────────────
// (4) F.6.1.E iterable_aspect_t — Hybride Laufzeit-Permutation pro Vendor
// ───────────────────────────────────────────────────────────────────────────
//
// User-Direktive (Doku §15.5 + §14.8): Vendor-Klasse darf optional einen
// "iterierbaren Aspekt" definieren — Runtime-Iteration INNERHALB einer Binary
// (statt jede Threshold-Variante als eigene Binary zu kompilieren). Spart bei
// concurrency-thresholds + buffer-sizes etc. zehntausende Binaries.
//
// Beispiel:
//   struct LockFreeConcurrency {
//       using iterable_aspect_t = std::size_t;
//       static constexpr std::array values{16u, 64u, 256u, 1024u, 4096u};
//       static constexpr std::span<std::size_t const> iterable_values() noexcept {
//           return values;
//       }
//       void set_threshold(std::size_t t) noexcept { ... }
//   };

/// Concept: Vendor hat iterierbaren Aspekt (typename iterable_aspect_t + iterable_values())
template <class V>
concept HasIterableAspect = requires {
    typename V::iterable_aspect_t;
    { V::iterable_values() } -> std::convertible_to<
        std::span<typename V::iterable_aspect_t const>>;
};

/**
 * @brief for_each_aspect — Runtime-Iteration ueber iterable_values<V>()
 *
 * Visitor wird mit dem aspect_value als Argument aufgerufen pro Iteration.
 * Wenn V keinen iterable_aspect_t hat: 1 Aufruf mit Default-Wert (Identitaet).
 *
 * Verwendung INNERHALB einer Permutation-Binary:
 *   for_each_aspect<LockFreeConcurrency>([](auto threshold){
 *       // Mess-Reihe mit diesem Threshold
 *   });
 */
template <class V, class Visitor>
constexpr void for_each_aspect(Visitor&& visitor) {
    if constexpr (HasIterableAspect<V>) {
        for (auto const& val : V::iterable_values()) {
            std::forward<Visitor>(visitor)(val);
        }
    } else {
        // Keine Iteration — Vendor hat keinen iterable_aspect_t
        // Visitor wird genau einmal aufgerufen, optional ohne Argument
        if constexpr (std::is_invocable_v<Visitor>) {
            std::forward<Visitor>(visitor)();
        }
        // sonst: kein Aufruf (Visitor erwartet Argument, Vendor liefert keins)
    }
}

/**
 * @brief aspect_count<V>() — Anzahl der Aspekt-Iterationen pro Vendor
 *
 * 1 wenn kein iterable_aspect_t (Default), sonst V::iterable_values().size().
 */
template <class V>
[[nodiscard]] constexpr std::size_t aspect_count() noexcept {
    if constexpr (HasIterableAspect<V>) {
        return V::iterable_values().size();
    } else {
        return 1u;
    }
}

// ───────────────────────────────────────────────────────────────────────────
// (5) Full-Join Multi-Pruefling (V41.F.6 Stufe 3, vorbereitete API)
// ───────────────────────────────────────────────────────────────────────────
//
// User-Direktive: Stufe 3 = "non-redundant join" — alle Default-Variants ∪
// alle Pruefling-Variants pro Achse, dedupliziert via mp_unique.
// Heute: Skelett-API, F.6.3 baut konkrete Pruefling-Achs-Spezialisierungen ein.

/**
 * @brief AxisFullJoin — vereinigt cache-engine-Default-Variants + Pruefling-Variants
 *        pro Achse, non-redundant.
 *
 * MP11-Idiom: mp_append + mp_unique (siehe Boost.MP11 algorithm reference).
 *
 * @tparam DefaultList   mp_list aus cache-engine EnabledVendors einer Achse
 * @tparam PrueflingLists mp_list je Pruefling (eine pro Pruefling-Repo)
 */
template <class DefaultList, class... PrueflingLists>
using AxisFullJoin = mp::mp_unique<
    mp::mp_append<DefaultList, PrueflingLists...>
>;

// ───────────────────────────────────────────────────────────────────────────
// (5) Default-Predicates (Helper)
// ───────────────────────────────────────────────────────────────────────────

template <class /*Perm*/>
using AlwaysTrue = mp::mp_true;

template <class /*Perm*/>
using AlwaysFalse = mp::mp_false;

}  // namespace comdare::cache_engine::permutations
