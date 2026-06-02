// D10/D11-DLL (L-76b/c) — gattungs-generischer DLL-Round-Trip: lädt je übergebene .dll via AnatomyModuleLoader,
// erkennt die Gattung RUNTIME über anatomy()->genus() und fragt das passende Sub-Interface via dynamic_cast ab
// (ISequenceTier / IViewTier). Beweist: Sequence + View sind DLL-baubar/-ladbar/-observierbar über DENSELBEN
// gattungs-agnostischen Loader (Doc 24 §8.8). Build: cl test_dgenus_dll.cpp anatomy_module_loader.cpp → exe <dll...>

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/sequence_tier.hpp>
#include <anatomy/view_tier.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

static int g_fail = 0;
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

static void drive_sequence(ana::IAnatomyBase* a) {
    auto* st = dynamic_cast<ana::ISequenceTier*>(a);
    tr("Sequence: dynamic_cast<ISequenceTier*> != null", st != nullptr);
    if (!st) return;
    for (std::uint64_t i = 0; i < 8; ++i) st->tier_push_back(i * 10);
    std::uint64_t out = 0;
    tr("Sequence: tier_size()==8", st->tier_size() == 8);
    tr("Sequence: tier_at(3)==30 über DLL", st->tier_at(3, &out) && out == 30u);
    ana::SequenceObserverSnapshotV1 pod{};
    st->tier_observe_sequence(&pod);
    tr("Sequence: observe push_count==8 + growth_events>0 über DLL", pod.push_count == 8 && pod.growth_events > 0);
}
static void drive_view(ana::IAnatomyBase* a) {
    auto* vt = dynamic_cast<ana::IViewTier*>(a);
    tr("View: dynamic_cast<IViewTier*> != null", vt != nullptr);
    if (!vt) return;
    std::uint64_t buf[4] = {7, 8, 9, 10};
    vt->tier_bind(buf, 4);
    std::uint64_t out = 0;
    tr("View: tier_size()==4 nach bind über DLL", vt->tier_size() == 4);
    tr("View: tier_read(2)==9 über DLL", vt->tier_read(2, &out) && out == 9u);
    ana::ViewObserverSnapshotV1 pod{};
    vt->tier_observe_view(&pod);
    tr("View: observe bound_size==4 über DLL", pod.bound_size == 4);
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: test_dgenus_dll <dll...>\n"; return 2; }
    std::cout << "D-Genus DLL-Round-Trip (Sequence/View über gattungs-agnostischen Loader):\n";
    int seen_seq = 0, seen_view = 0;
    for (int i = 1; i < argc; ++i) {
        loader::AnatomyModuleHandle handle;
        int const st = loader::AnatomyModuleLoader::load(argv[i], handle);
        tr((std::string{"load == status_ok: "} + argv[i]).c_str(), st == loader::status_ok);
        if (st != loader::status_ok) { std::cerr << "  status: " << loader::status_name(st) << "\n"; continue; }
        ana::IAnatomyBase* a = handle.anatomy();
        if (!a) { tr("anatomy() != null", false); continue; }
        switch (a->genus()) {
            case ana::AnatomyGenus::Sequence: ++seen_seq;  drive_sequence(a); break;
            case ana::AnatomyGenus::View:     ++seen_view; drive_view(a);     break;
            default: tr("unerwartete Gattung", false); break;
        }
    }
    tr("Sequence-DLL geladen + getrieben", seen_seq >= 1);
    tr("View-DLL geladen + getrieben", seen_view >= 1);
    std::cout << "\n==== D-Genus DLL: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
