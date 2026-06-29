// L-76c — View-Achsen extent/layout/accessor als Mehr-Policy-Achsen: die Policies erfüllen ihre Concepts, die
// Registries tragen alle Varianten, und in der ECHTEN ViewAnatomy liest LayoutStrided physisch ANDERE Speicher-
// zellen als LayoutRight (axis_layout real wirksam, kein Stub). Build: cl /I libs/cache_engine + Boost::mp11.

#include "topics/view/view_registries.hpp"
#include "topics/view/view_policies.hpp"
#include "anatomy/view_anatomy.hpp"
#include "anatomy/view_composition.hpp"

#include <boost/mp11.hpp>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace vw  = comdare::cache_engine::view;
namespace mp  = boost::mp11;

// (1) Alle Policies erfüllen ihre Concepts (aus view_composition.hpp).
static_assert(cea::ExtentPolicy<cea::DynamicExtent>, "DynamicExtent ist ExtentPolicy");
static_assert(cea::ExtentPolicy<vw::StaticExtent<256>>, "StaticExtent ist ExtentPolicy");
static_assert(cea::LayoutPolicy<cea::LayoutRight>, "LayoutRight ist LayoutPolicy");
static_assert(cea::LayoutPolicy<vw::LayoutLeft>, "LayoutLeft ist LayoutPolicy");
static_assert(cea::LayoutPolicy<vw::LayoutStrided<2>>, "LayoutStrided ist LayoutPolicy");
static_assert(cea::AccessorPolicy<cea::DefaultAccessor>, "DefaultAccessor ist AccessorPolicy");
static_assert(cea::AccessorPolicy<vw::AlignedAccessor<64>>, "AlignedAccessor ist AccessorPolicy");

// (2) Registries.
static_assert(vw::kExtentCount == 2, "axis_extent: 2 Policies");
static_assert(vw::kLayoutCount == 3, "axis_layout: 3 Policies (right/left/strided)");
static_assert(vw::kAccessorCount == 2, "axis_accessor: 2 Policies");

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ViewComposition mit variabler Layout-Policy (4 Platzhalter + DynamicExtent + Layout + DefaultAccessor).
template <class Layout>
using ViewComp = cea::ViewComposition<int, int, int, int, cea::DynamicExtent, Layout, cea::DefaultAccessor>;

int main() {
    std::cout << "==== L-76c View-Achsen extent/layout/accessor (Mehr-Policy) ====\n";

    // (3) index_of je Layout: Right/Left == i (1D), Strided == i*Stride (genuin verschieden).
    std::cout << "-- index_of(3) je Layout-Policy --\n";
    eq("LayoutRight.index_of(3)    == 3", cea::LayoutRight{}.index_of(3), std::size_t{3});
    eq("LayoutLeft.index_of(3)     == 3 (1D == Right)", vw::LayoutLeft{}.index_of(3), std::size_t{3});
    eq("LayoutStrided<2>.index_of(3) == 6 (i*Stride)", vw::LayoutStrided<2>{}.index_of(3), std::size_t{6});

    // (4) axis_extent: dynamisch vs statisch.
    tr("DynamicExtent.is_static() == false", !cea::DynamicExtent{}.is_static());
    tr("StaticExtent<256>.is_static() == true", vw::StaticExtent<256>{}.is_static());
    eq("StaticExtent<256>.static_extent() == 256", vw::StaticExtent<256>{}.static_extent(), std::size_t{256});

    // (5) INTEGRATION: in der ECHTEN ViewAnatomy liest LayoutStrided physisch ANDERE Zellen als LayoutRight.
    std::cout << "\n-- ViewAnatomy read(2) über buf={0..7}, je Layout --\n";
    std::uint64_t buf[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    cea::ViewAnatomy<ViewComp<cea::LayoutRight>> v_right;
    v_right.bind(buf, 8);
    std::optional<std::uint64_t> const r_right = v_right.read(2);

    cea::ViewAnatomy<ViewComp<vw::LayoutStrided<2>>> v_strided;
    v_strided.bind(buf, 8);
    std::optional<std::uint64_t> const r_strided = v_strided.read(2);

    tr("LayoutRight: read(2) hat Wert", r_right.has_value());
    tr("LayoutStrided<2>: read(2) hat Wert", r_strided.has_value());
    eq("LayoutRight read(2) == 2 (buf[2])", r_right.value_or(999), std::uint64_t{2});
    eq("LayoutStrided<2> read(2) == 4 (buf[2*2]=buf[4])", r_strided.value_or(999), std::uint64_t{4});
    tr("Strided liest physisch eine ANDERE Zelle als Right (axis_layout real wirksam)",
       r_right.value_or(0) != r_strided.value_or(0));

    // Strided<2> halbiert die lesbare Reichweite: read(4) → off=8 >= size 8 → OOB.
    tr("LayoutStrided<2>: read(4) == OOB (off=8 >= size 8)", !v_strided.read(4).has_value());

    std::cout << "\n==== L-76c View-Achsen: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
