#pragma once
// L-76c — View-Achsen-Registries: die zentralen Varianten-Listen der 3 eigenen View-Achsen (axis_extent/layout/
// accessor), analog axis_growth_registry. Jede trägt den inline-Default (aus view_composition.hpp) + die
// Goldstandard-Erweiterungen (view_policies.hpp). Konsumierbar als die 3 View-eigenen ViewPermutationEngine-Slots.

#include "view_policies.hpp"
#include "anatomy/view_composition.hpp"   // DynamicExtent / LayoutRight / DefaultAccessor (Defaults)

#include <boost/mp11.hpp>
#include <cstddef>

namespace comdare::cache_engine::view {

namespace mp = boost::mp11;

/// axis_extent: dynamische (Laufzeit-bind) + statische (compile-time) Ausdehnung.
using AllExtents     = mp::mp_list<::comdare::cache_engine::anatomy::DynamicExtent, StaticExtent<256>>;
/// axis_layout: row-major (Default) + column-major + gestrided (mdspan layout_right/left/stride).
using AllLayouts     = mp::mp_list<::comdare::cache_engine::anatomy::LayoutRight, LayoutLeft, LayoutStrided<2>>;
/// axis_accessor: direkter (Default) + aligned Zugriff (mdspan default/aligned accessor).
using AllAccessors   = mp::mp_list<::comdare::cache_engine::anatomy::DefaultAccessor, AlignedAccessor<64>>;

using EnabledExtents   = AllExtents;
using EnabledLayouts   = AllLayouts;
using EnabledAccessors = AllAccessors;

inline constexpr std::size_t kExtentCount   = mp::mp_size<AllExtents>::value;     // 2
inline constexpr std::size_t kLayoutCount   = mp::mp_size<AllLayouts>::value;     // 3
inline constexpr std::size_t kAccessorCount = mp::mp_size<AllAccessors>::value;   // 2

}  // namespace comdare::cache_engine::view
