// D9.2 / L-76a — SetAbiAdapter in-process: bridge SetAnatomy → IAnatomyBase + ISetTier (dynamic_cast).
// Verifiziert die ABI-Adapter-Header der Set-Gattung OHNE schweren Achsen-Compile (TestKeySet-Organ). Der
// DLL-Round-Trip via COMDARE_DEFINE_SET_MODULE + echtem search_algo-Wrapper = D9.3. Build: cl /I libs/cache_engine.

#include "anatomy/set_abi_adapter.hpp"
#include "anatomy/set_anatomy.hpp"
#include "anatomy/set_composition.hpp"
#include "anatomy/set_tier.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>

namespace cea = comdare::cache_engine::anatomy;
namespace eng = comdare::cache_engine::execution_engine;

struct TestKeySet {
    using key_type = std::uint64_t;
    std::set<std::uint64_t> s;
    void insert(std::uint64_t k, std::uint64_t) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt; }
    void erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void clear() { s.clear(); }
};
using SC    = cea::SetComposition<TestKeySet, int, int, int, int, int, int, int, int, int, int, int, int, int, int>;
using SAnat = cea::SetAnatomy<SC>;

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    cea::SetAbiAdapter<SAnat> adapter;
    std::cout << "==== D9.2 SetAbiAdapter über IAnatomyBase ====\n";
    cea::IAnatomyBase* base = &adapter;
    tr("genus() == Set", base->genus() == cea::AnatomyGenus::Set);
    tr("engine_kind() == Anatomy", base->engine_kind() == eng::ExecutionEngineKind::Anatomy);
    eq("organ_count() == 15", base->organ_count(), std::size_t{15});
    eq("composition_name", std::string{base->composition_name()}, std::string{"SetComposition"});
    base->warm_up(); base->run();
    tr("lifecycle Running", base->lifecycle_state() == eng::EngineLifecycleState::Running);

    std::cout << "\n==== D9.2 Set-Antrieb über ISetTier (dynamic_cast) ====\n";
    auto* st = dynamic_cast<cea::ISetTier*>(base);
    tr("dynamic_cast<ISetTier*> != null", st != nullptr);
    if (st) {
        tr("tier_set_insert(1)", st->tier_set_insert(1));
        tr("tier_set_insert(2)", st->tier_set_insert(2));
        tr("tier_set_insert(3)", st->tier_set_insert(3));
        tr("tier_set_insert(2) dup → false", !st->tier_set_insert(2));
        eq("tier_set_size() == 3", st->tier_set_size(), std::uint64_t{3});
        tr("tier_set_contains(2)", st->tier_set_contains(2));
        tr("tier_set_contains(99) == false", !st->tier_set_contains(99));
        tr("tier_set_erase(2)", st->tier_set_erase(2));
        eq("tier_set_size() == 2 nach erase", st->tier_set_size(), std::uint64_t{2});
        cea::SetObserverSnapshotV1 pod{};
        st->tier_observe_set(&pod);
        eq("observe: insert_count == 3", pod.insert_count, std::uint64_t{3});
        eq("observe: organ_count == 15", pod.organ_count, std::uint64_t{15});
        eq("observe: erase_count == 1", pod.erase_count, std::uint64_t{1});
    }
    std::cout << "\n==== D9.2 SetAbiAdapter: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
