// D11 / L-76c — View-Gattung: Typ-Ebene + non-owning-Anatomie + ViewAbiAdapter (in-process, leichtgewichtig).
// ViewComposition (4 §28-Plant-Achsen + extent/layout/accessor) + IsViewComposition + ViewObserverSnapshotV1 +
// ViewAnatomy (bind externer Puffer + read über layout/accessor) + ViewAbiAdapter (IAnatomyBase + IViewTier).
// Build: cl /I libs/cache_engine (kein Boost — non-owning + Default-Policies).

#include "anatomy/view_tier.hpp"
#include "anatomy/view_composition.hpp"
#include "anatomy/view_anatomy.hpp"
#include "anatomy/view_abi_adapter.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace eng = comdare::cache_engine::execution_engine;

using VC    = cea::ViewComposition<int, int, int>; // Extent/Layout/Accessor = Defaults (INC-2c: 3 geteilte)
using VAnat = cea::ViewAnatomy<VC>;

static_assert(cea::IsViewComposition<VC>);
static_assert(VC::slot_count == 6, "View = 3 geteilt + extent/layout/accessor (§28, K-C; INC-2c)");
static_assert(cea::ExtentPolicy<cea::DynamicExtent> && cea::LayoutPolicy<cea::LayoutRight> &&
                  cea::AccessorPolicy<cea::DefaultAccessor>,
              "Default-Policies muessen die Concepts erfuellen");
static_assert(VAnat::genus() == cea::AnatomyGenus::View);

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

int main() {
    std::cout << "==== D11 View-Typ-Ebene ====\n";
    tr("IsViewComposition<VC>", cea::IsViewComposition<VC>);
    eq("VC::slot_count == 6 (3 + extent/layout/accessor)", VC::slot_count, std::size_t{6});
    tr("genus() == View (Pflanze)", VAnat::genus() == cea::AnatomyGenus::View);
    eq("organ_count() == 6", VAnat::organ_count(), std::size_t{6});
    eq("ViewObserverSnapshotV1 = 6 uint64", sizeof(cea::ViewObserverSnapshotV1), std::size_t{6 * 8});

    std::cout << "\n==== D11 ViewAnatomy non-owning (bind externer Puffer + read) ====\n";
    std::uint64_t buf[5] = {10, 20, 30, 40, 50}; // EXTERNER Puffer (View besitzt ihn NICHT)
    VAnat         view;
    view.bind(buf, 5);
    eq("size() == 5 nach bind(extern, 5)", view.size(), std::size_t{5});
    auto r0 = view.read(0);
    tr("read(0) == 10 (aus externem Puffer)", r0.has_value() && *r0 == 10u);
    auto r4 = view.read(4);
    tr("read(4) == 50", r4.has_value() && *r4 == 50u);
    tr("read(99) == nullopt (out-of-bounds)", !view.read(99).has_value());
    cea::ViewObserverSnapshot const o = view.observe_all();
    eq("Observer read_count == 3", o.read_count, std::uint64_t{3});
    eq("Observer read_oob_count == 1", o.read_oob_count, std::uint64_t{1});
    eq("Observer bound_size == 5", o.bound_size, std::uint64_t{5});
    eq("Observer bind_count == 1", o.bind_count, std::uint64_t{1});

    std::cout << "\n==== D11 ViewAbiAdapter über IAnatomyBase + IViewTier ====\n";
    cea::ViewAbiAdapter<VAnat> adapter;
    cea::IAnatomyBase*         base = &adapter;
    tr("genus() == View", base->genus() == cea::AnatomyGenus::View);
    tr("engine_kind() == Anatomy", base->engine_kind() == eng::ExecutionEngineKind::Anatomy);
    eq("organ_count() == 6", base->organ_count(), std::size_t{6});
    auto* vt = dynamic_cast<cea::IViewTier*>(base);
    tr("dynamic_cast<IViewTier*> != null", vt != nullptr);
    if (vt) {
        std::uint64_t buf2[3] = {111, 222, 333};
        vt->tier_bind(buf2, 3);
        eq("tier_size() == 3 nach tier_bind", vt->tier_size(), std::uint64_t{3});
        std::uint64_t out = 0;
        tr("tier_read(2) == 333", vt->tier_read(2, &out) && out == 333u);
        tr("tier_read(99) == false (oob)", !vt->tier_read(99, &out));
        cea::ViewObserverSnapshotV1 pod{};
        vt->tier_observe_view(&pod);
        eq("observe: bound_size == 3", pod.bound_size, std::uint64_t{3});
        eq("observe: organ_count == 6", pod.organ_count, std::uint64_t{6});
        tr("observe: read_count >= 2", pod.read_count >= 2);
    }

    std::cout << "\n==== D11 View: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
