#pragma once
// D9 / L-76a (2026-06-02) — SetComposition: die SET-Gattungs-Komposition (Vogel, K-only). 15 Achsen-Slots
// (Doku 14 §28 Bird-Spalte, K-A aufgelöst: kein mapping/value_handle — K=V; mit filter). Analog AdHocComposition
// <19> (SearchAlgorithm), aber GETRENNTE Gattung (Cross-Genus type-unmöglich, Doku 14 §32) → eigene Slot-Namen.
//
// Leichtgewichtig (kein Organ-Include): trägt NUR die 15 named Achsen-Typen als Komposition-Identität. Das
// echte K-only-Such-Organ baut SetAnatomy<Comp> aus node_type/memory_layout/allocator (analog SearchAlgorithm).

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// SetComposition<T0..T12> — 13 named Achsen-Slots der Set-Gattung (Reihenfolge = §28 Bird-Spalte;
/// Bau-INC-2c: telemetry / Bau-INC-2d: isa sind System-Achsen, kein Slot mehr).
/// Bewusst OHNE mapping (axis_03m) + value_handle (axis_14) — Set ist K-only (K=V).
template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10,
          class T11, class T12>
struct SetComposition {
    using search_algo        = T0;  // axis_03a — K-only-Suchkern
    using cache_traversal    = T1;  // axis_03b
    using path_compression   = T2;  // axis_02
    using node_type          = T3;  // axis_04
    using memory_layout      = T4;  // axis_05
    using allocator          = T5;  // axis_06
    using prefetch           = T6;  // axis_07
    using concurrency        = T7;  // axis_08
    using serialization      = T8;  // axis_10
    using index_organization = T9;  // axis_01 (search_engine; INC-2d: isa raus, war T10)
    using io_dispatch        = T10; // axis_io  (INC-2d: war T11)
    using migration_policy   = T11; // axis_migration (INC-2d: war T12)
    using filter             = T12; // axis_filter (INC-2d: war T13)

    static constexpr std::size_t      slot_count = 13; // INC-2d: isa raus (war 14 nach INC-2c-telemetry, 15 davor)
    static constexpr std::string_view name       = "SetComposition";
    static constexpr std::string_view paper_id   = "P00 Set Gattung (Vogel, K-only)";
};

/// IsSetComposition — Concept: 13 named Set-Achsen-Aliase + Meta. KEIN mapping/value_handle (Set-Invariante); INC-2d ohne isa.
template <class C>
concept IsSetComposition = requires {
    typename C::search_algo;
    typename C::cache_traversal;
    typename C::path_compression;
    typename C::node_type;
    typename C::memory_layout;
    typename C::allocator;
    typename C::prefetch;
    typename C::concurrency;
    typename C::serialization;
    typename C::index_organization;
    typename C::io_dispatch;
    typename C::migration_policy;
    typename C::filter;
    { C::slot_count } -> std::convertible_to<std::size_t>;
};

/// Anzahl Set-Komposition-Slots (Gattungs-Invariante, analog AdHocComposition<19> mit 19 Slots).
inline constexpr std::size_t kSetCompositionSlotCount = 15;

} // namespace comdare::cache_engine::anatomy
