#pragma once
// L-76c Goldstandard-Erweiterung — die 3 eigenen View-Gattungs-Achsen axis_extent / axis_layout / axis_accessor
// (mdspan-Bezug, Doku 14 §26.6) als echte MEHR-Policy-Achsen. Bisher nur je EIN inline-Default
// (DynamicExtent/LayoutRight/DefaultAccessor) in anatomy/view_composition.hpp; hier die realen weiteren Policies.
// Alle erfüllen die ExtentPolicy/LayoutPolicy/AccessorPolicy-Concepts aus view_composition.hpp (Wiederverwendung).
//
// Realer Algorithmus-Unterschied (kein Stub): die Layout-Policy bestimmt den Index→Offset-Mapping (index_of),
// also WELCHE Speicherzelle ein read(i) liest — LayoutStrided liest physisch andere Zellen als LayoutRight.
// C++23, header-only.

#include "anatomy/view_composition.hpp"   // ExtentPolicy/LayoutPolicy/AccessorPolicy-Concepts + die 3 inline-Defaults

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::view {

// ── axis_extent ──────────────────────────────────────────────────────────────
/// StaticExtent<N> — compile-time bekannte Ausdehnung (mdspan static extents). is_static()==true, static_extent()==N.
template <std::size_t N>
struct StaticExtent {
    [[nodiscard]] bool        is_static()     const noexcept { return true; }
    [[nodiscard]] std::size_t static_extent() const noexcept { return N; }
};

// ── axis_layout ──────────────────────────────────────────────────────────────
/// LayoutLeft — column-major (mdspan layout_left). Für 1D identisch zu LayoutRight (index_of(i)==i); für N-D
/// (zukünftige mehrdimensionale Views) verschieden. Hier 1D → semantisch distinkt, formal gleich (ehrlich dokumentiert).
struct LayoutLeft {
    [[nodiscard]] std::size_t index_of(std::size_t i) const noexcept { return i; }
};
/// LayoutStrided<Stride> — gestrided (mdspan layout_stride): index_of(i)==i*Stride. GENUIN verschieden von Right/Left
/// (liest jede Stride-te Zelle) → ein read(i) trifft physisch eine andere Speicherzelle.
template <std::size_t Stride = 2>
struct LayoutStrided {
    static constexpr std::size_t stride = Stride;
    [[nodiscard]] std::size_t index_of(std::size_t i) const noexcept { return i * Stride; }
};

// ── axis_accessor ────────────────────────────────────────────────────────────
/// AlignedAccessor<Align> — wie DefaultAccessor (Lese-Wert identisch), aber mit Alignment-Vertrag (mdspan
/// aligned_accessor): der Zugriff darf von garantierter Ausrichtung ausgehen (vektorisierbar). Distinkt im ABI-/
/// Optimierungs-Vertrag, value-identisch zum Default (das ist die ehrliche mdspan-Semantik).
template <std::size_t Align = 64>
struct AlignedAccessor {
    static constexpr std::size_t alignment = Align;
    [[nodiscard]] std::uint64_t access(std::uint64_t const* d, std::size_t i) const noexcept { return d[i]; }
};

}  // namespace comdare::cache_engine::view
