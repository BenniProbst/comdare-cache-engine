#pragma once
// V41.F.6.1.A Permutations-Concept (Boost.MP11-basiert, 2026-05-25 revidiert nach W5)
//
// @stand V41.F.6.1.A
//
// **Architektur-Ebene (User-Direktive 2026-05-25):**
//   src/permutations/ = allgemeines Permutations-Konzept (Topic-uebergreifend)
//
// Pattern aus W5-Recherche: Boost.MP11 (header-only, leichtgewichtig) fuer
// Compile-Time-Type-Listen + Cartesian-Product + Visitor-Iteration. KEINE
// std::variant (Runtime-Tag verboten — User-Direktive [[no-runtime-switch]]).
//
// **Konzeptuelle Trennung:**
//   - PermutationVariant<V>  - eine einzelne Variant einer Achse (z.B. StdMalloc)
//   - AxisVariantList        - Liste aller Variants einer Achse (Wrapper um mp_list)
//   - Permutation<V1...>     - eine vollstaendige Achsen-Permutation (Cartesian-Eintrag)
//   - CartesianPermutations  - mp_product aller Achsen-Listen
//   - for_each_permutation   - Compile-Time-Iteration (Codegen-Hook)

#include <boost/mp11.hpp>

#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::permutations {

namespace mp = boost::mp11;

// ───────────────────────────────────────────────────────────────────────────
// (1) Concept-Vertraege
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief PermutationAxis - Marker-Tag pro Achse (leerer Struct)
 *
 * Beispiel: `struct AllocatorAxis { static constexpr std::string_view name = "allocator"; };`
 */
template <typename T>
concept PermutationAxis = requires {
    { T::name } -> std::convertible_to<std::string_view>;
};

/**
 * @brief PermutationVariant - eine konkrete Variant einer Achse
 *
 * Pflicht-API (Topic-uebergreifend):
 *   - typename axis_id      = AllocatorAxis | ConcurrencyAxis | ...
 *   - typename family_id    = StdMallocFamily | MimallocFamily | ...
 *   - typename variant_tag  = unique Compile-Time-Tag fuer Hash
 *   - static constexpr std::string_view variant_name
 *   - static constexpr std::uint64_t    variant_hash
 *   - default-konstruierbar oder std::is_empty_v (fuer Compile-Time-Instanziierung)
 *
 * Cache-engine-eigene Pflicht-API (z.B. allocate/deallocate) wird im Topic-spezifischen
 * Achsen-Concept (z.B. CacheEnginePermutationStrategy) zusaetzlich verlangt — dieses
 * hier ist nur die generelle Permutations-Identitaet.
 */
template <typename V>
concept PermutationVariant = requires {
    typename V::axis_id;
    typename V::family_id;
    typename V::variant_tag;
    { V::variant_name } -> std::convertible_to<std::string_view>;
    { V::variant_hash } -> std::convertible_to<std::uint64_t>;
} && (std::is_empty_v<V> || std::default_initializable<V>);

// ───────────────────────────────────────────────────────────────────────────
// (2) Achsen-Variant-Liste (Wrapper um mp_list)
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief AxisVariantList - statische Liste aller Variants einer Achse
 *
 * Beispiel:
 *   using AllocatorVariants = AxisVariantList<AllocatorAxis,
 *       StdMalloc, MiMalloc, JeMalloc, TcMalloc>;
 */
template <typename Axis, PermutationVariant... Vs>
struct AxisVariantList {
    using axis_id                     = Axis;
    using variants                    = mp::mp_list<Vs...>;
    static constexpr std::size_t size = sizeof...(Vs);
};

// ───────────────────────────────────────────────────────────────────────────
// (3) Permutation = Tuple von Variants (eine je Achse)
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief Permutation - eine vollstaendige Achsen-Kombination
 *
 * Compile-Time-Hash via FNV-1a ueber alle Variant-Hashes (stabile Permutations-ID).
 */
template <PermutationVariant... Vs>
struct Permutation {
    using variants                          = mp::mp_list<Vs...>;
    static constexpr std::size_t axis_count = sizeof...(Vs);

    /**
     * FNV-1a Compile-Time-Hash. Stabil ueber Builds (basiert NICHT auf Type-Namen,
     * sondern auf variant_hash-Konstanten). Pro Permutation eine eindeutige ID
     * fuer Codegen (perm_<hash>.cpp + perm_<hash>.so/.dll).
     */
    [[nodiscard]] static constexpr std::uint64_t hash() noexcept {
        std::uint64_t h = 0xcbf29ce484222325ULL; // FNV-1a offset basis
        ((h = (h ^ Vs::variant_hash) * 0x100000001b3ULL), ...);
        return h;
    }
};

// ───────────────────────────────────────────────────────────────────────────
// (4) Cartesian Product aller Achsen-Listen
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief CartesianPermutations - mp_product aller AxisVariantLists
 *
 * Beispiel:
 *   using All = CartesianPermutations<AllocatorVariants, ConcurrencyVariants, LayoutVariants>;
 *   // -> mp_list<Permutation<StdMalloc, Mutex, AoS>,
 *   //            Permutation<StdMalloc, Mutex, SoA>,
 *   //            Permutation<MiMalloc, Mutex, AoS>, ...>
 */
template <typename... AxisLists>
using CartesianPermutations = mp::mp_product<Permutation, typename AxisLists::variants...>;

// ───────────────────────────────────────────────────────────────────────────
// (5) Constraint-Filter (invalid combos zur Compile-Time aussortieren)
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief FilterValid - filtert Permutations-Liste mit user-definierter Predicate-Klasse
 *
 * Beispiel-Predicate (zur Compile-Time wahr/falsch pro Permutation):
 *   template <typename Perm>
 *   struct is_valid_combo {
 *       static constexpr bool value =
 *           !(std::same_as<mp::mp_at_c<typename Perm::variants, 0>, MiMalloc>
 *           && std::same_as<mp::mp_at_c<typename Perm::variants, 1>, LockFree>);
 *   };
 *
 *   using Valid = FilterValid<is_valid_combo, AllPermutations>;
 */
template <template <typename> class Predicate, typename Perms>
using FilterValid = mp::mp_filter<Predicate, Perms>;

// ───────────────────────────────────────────────────────────────────────────
// (6) Visitor / for_each_permutation (Codegen-Hook)
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief for_each_permutation - Compile-Time-Iteration ueber Permutations-Liste
 *
 * Verwendung im Codegen-Tool:
 *   for_each_permutation<AllPermutations>([]<typename P>() {
 *       constexpr auto hash = P::hash();
 *       std::cout << "perm_" << std::hex << hash << ".cpp\n";
 *   });
 */
template <typename Perms, typename Visitor>
constexpr void for_each_permutation(Visitor&& v) {
    mp::mp_for_each<Perms>([&]<typename P>(P) { std::forward<Visitor>(v).template operator()<P>(); });
}

} // namespace comdare::cache_engine::permutations
