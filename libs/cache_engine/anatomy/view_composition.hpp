#pragma once
// D11 / L-76c (2026-06-02) — ViewComposition: die VIEW-Gattungs-Komposition (Pflanze, non-owning). 7 Slots =
// 3 geteilte Achsen (Doku 14 §28 Plant: memory_layout/value_handle/isa; INC-2c: telemetry ist System-Achse) + 3 eigene
// (axis_extent/axis_layout/axis_accessor, mdspan-Bezug §26.6). GETRENNTE Gattung (Cross-Genus type-unmöglich,
// Doku 14 §32). non-owning → KEIN allocator/concurrency/insert (Doku 14 §28 „— (non-owning)/(immutable)").
//
// extent/layout/accessor: hier leichtgewichtige Default-Policies (DynamicExtent/LayoutRight/DefaultAccessor).
// Goldstandard-Vollausbau (layout_left/right/stride, aligned_accessor) = Folgeschritt.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// axis_extent: statische vs. dynamische Ausdehnung (mdspan extents).
template <class P>
concept ExtentPolicy = requires(P p) {
    { p.is_static() } -> std::convertible_to<bool>;
    { p.static_extent() } -> std::convertible_to<std::size_t>;
};
/// axis_layout: index_of(coord) → linearer Offset (mdspan layout_left/right/stride).
template <class P>
concept LayoutPolicy = requires(P p, std::size_t i) {
    { p.index_of(i) } -> std::convertible_to<std::size_t>;
};
/// axis_accessor: access(ptr, offset) → Element (mdspan default/aligned accessor).
template <class P>
concept AccessorPolicy = requires(P p, std::uint64_t const* d, std::size_t i) {
    { p.access(d, i) } -> std::convertible_to<std::uint64_t>;
};

struct DynamicExtent { // Default-axis_extent: Ausdehnung erst zur Laufzeit (bind) bekannt.
    [[nodiscard]] bool        is_static() const noexcept { return false; }
    [[nodiscard]] std::size_t static_extent() const noexcept { return ~std::size_t{0}; } // dynamic_extent
};
struct LayoutRight { // Default-axis_layout: row-major 1D → Offset == Index.
    [[nodiscard]] std::size_t index_of(std::size_t i) const noexcept { return i; }
};
struct DefaultAccessor { // Default-axis_accessor: direkter Element-Zugriff.
    [[nodiscard]] std::uint64_t access(std::uint64_t const* d, std::size_t i) const noexcept { return d[i]; }
};

/// ViewComposition<T0..T2, Extent, Layout, Accessor> — 3 geteilte Achsen (§28 Plant; INC-2c: telemetry
/// ist System-Achse, kein Slot) + 3 eigene. non-owning.
template <class T0, class T1, class T2, class Extent = DynamicExtent, class Layout = LayoutRight,
          class Accessor = DefaultAccessor>
struct ViewComposition {
    using memory_layout   = T0;       // axis_05
    using value_handle    = T1;       // axis_14
    using isa             = T2;       // axis_09
    using extent_policy   = Extent;   // NEU axis_extent
    using layout_policy   = Layout;   // NEU axis_layout
    using accessor_policy = Accessor; // NEU axis_accessor

    static constexpr std::size_t      slot_count = 6; // 3 geteilt + extent/layout/accessor (INC-2c)
    static constexpr std::string_view name       = "ViewComposition";
    static constexpr std::string_view paper_id   = "P00 View Gattung (Pflanze, non-owning)";
};

/// IsViewComposition — Concept: 4 geteilte named Achsen + extent/layout/accessor. Kein allocator/concurrency (non-owning).
template <class C>
concept IsViewComposition = requires {
    typename C::memory_layout;
    typename C::value_handle;
    typename C::isa;
    typename C::extent_policy;
    typename C::layout_policy;
    typename C::accessor_policy;
    { C::slot_count } -> std::convertible_to<std::size_t>;
};

inline constexpr std::size_t kViewCompositionSlotCount = 7;

} // namespace comdare::cache_engine::anatomy
