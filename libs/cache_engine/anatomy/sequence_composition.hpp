#pragma once
// D10 / L-76b (2026-06-02) — SequenceComposition: die SEQUENCE-Gattungs-Komposition (Reptil, V-indexed). 10 geteilte
// Achsen (Doku 14 §28 Reptile-Spalte, K-B aufgelöst) + axis_growth (eigene Achse) = 11 Slots. GETRENNTE Gattung
// (Cross-Genus type-unmöglich, Doku 14 §32). Kein search_algo/cache_traversal/mapping/path_compression/node_type/
// index_organization/filter (Sequence ist V-indexed, kein K-Suchorgan).
//
// axis_growth (growth_policy): hier ein leichtgewichtiges Default-Organ (DoublingGrowth). Der Goldstandard-
// Vollausbau der Achse (eigenes Topic + weitere Policies GoldenRatio/FixedChunk/Exact) ist ein Folgeschritt.

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// GrowthPolicy-Concept (axis_growth): bestimmt die nächste Kapazität bei Überlauf der V-Sequenz.
template <class P>
concept GrowthPolicy = requires(P p, std::size_t current, std::size_t requested) {
    { p.next_capacity(current, requested) } -> std::convertible_to<std::size_t>;
    { p.growth_factor() } -> std::convertible_to<double>;
};

/// DoublingGrowth — std::vector-Standard: Kapazität verdoppeln (mind. requested). Default-axis_growth-Organ.
struct DoublingGrowth {
    [[nodiscard]] std::size_t next_capacity(std::size_t current, std::size_t requested) const noexcept {
        std::size_t next = (current == 0) ? 1 : current * 2;
        return next < requested ? requested : next;
    }
    [[nodiscard]] double growth_factor() const noexcept { return 2.0; }
};

/// SequenceComposition<T0..T9, Growth> — 10 geteilte Achsen (§28 Reptile) + axis_growth. V-indexed (K=∅).
template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9,
          class Growth = DoublingGrowth>
struct SequenceComposition {
    using memory_layout    = T0;     // axis_05
    using allocator        = T1;     // axis_06
    using prefetch         = T2;     // axis_07
    using concurrency      = T3;     // axis_08
    using serialization    = T4;     // axis_10
    using telemetry        = T5;     // axis_11
    using value_handle     = T6;     // axis_14
    using isa              = T7;     // axis_09
    using io_dispatch      = T8;     // axis_io
    using migration_policy = T9;     // axis_migration
    using growth_policy    = Growth; // NEU axis_growth (eigene Sequence-Achse)

    static constexpr std::size_t      slot_count = 11; // 10 geteilte + axis_growth
    static constexpr std::string_view name       = "SequenceComposition";
    static constexpr std::string_view paper_id   = "P00 Sequence Gattung (Reptil, V-indexed)";
};

/// IsSequenceComposition — Concept: 10 geteilte named Achsen + growth_policy. Kein search_algo (V-indexed).
template <class C>
concept IsSequenceComposition = requires {
    typename C::memory_layout;
    typename C::allocator;
    typename C::prefetch;
    typename C::concurrency;
    typename C::serialization;
    typename C::telemetry;
    typename C::value_handle;
    typename C::isa;
    typename C::io_dispatch;
    typename C::migration_policy;
    typename C::growth_policy;
    { C::slot_count } -> std::convertible_to<std::size_t>;
};

inline constexpr std::size_t kSequenceCompositionSlotCount = 11;

} // namespace comdare::cache_engine::anatomy
