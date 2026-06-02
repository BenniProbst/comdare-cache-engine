// D9 / L-76a — Set-Gattung Typ-Ebene + Anatomie-Mechanik (in-process, leichtgewichtig).
// Verifiziert: SetComposition (15 §28-Achsen, kein mapping/value_handle) + IsSetComposition + SetObserverSnapshotV1
// (ABI-POD) + SetAnatomy K-only-Mengen-Semantik (genus()==Set, keine Duplikate, contains/erase + Observer).
// Das search_algo-Organ ist hier ein minimales std::set-basiertes API-Organ (echter Mengen-Algo; der Achsen-
// Wrapper-Vollbeleg + DLL-Round-Trip via COMDARE_DEFINE_SET_MODULE = D9.2). Build: cl /I libs/cache_engine (kein Boost).

#include "anatomy/set_tier.hpp"
#include "anatomy/set_composition.hpp"
#include "anatomy/set_anatomy.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <type_traits>

namespace cea = comdare::cache_engine::anatomy;

// Minimales search_algo-API-Organ (key_type + insert(k,v)/lookup(k)/erase(k)/occupied_count()/clear()) auf std::set.
struct TestKeySet {
    using key_type = std::uint64_t;
    std::set<std::uint64_t> s;
    void insert(std::uint64_t k, std::uint64_t /*v*/) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void clear() { s.clear(); }
};

using SC   = cea::SetComposition<TestKeySet, int, int, int, int, int, int, int, int, int, int, int, int, int, int>;
using SAnat = cea::SetAnatomy<SC>;

static_assert(cea::IsSetComposition<SC>, "SetComposition muss IsSetComposition erfuellen");
static_assert(SC::slot_count == 15, "Set = 15 Achsen (§28, K-A)");
static_assert(cea::kSetCompositionSlotCount == 15);
static_assert(SAnat::genus() == cea::AnatomyGenus::Set, "SetAnatomy genus == Set");

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n";
}
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D9 Set-Typ-Ebene ====\n";
    tr("IsSetComposition<SC>", cea::IsSetComposition<SC>);
    eq("SC::slot_count == 15", SC::slot_count, std::size_t{15});
    eq("composition_name", std::string{SAnat::composition_name()}, std::string{"SetComposition"});
    tr("genus() == Set (Vogel)", SAnat::genus() == cea::AnatomyGenus::Set);
    eq("organ_count() == 15", SAnat::organ_count(), std::size_t{15});
    eq("SetObserverSnapshotV1 = 9 uint64", sizeof(cea::SetObserverSnapshotV1), std::size_t{9 * 8});

    std::cout << "\n==== D9 SetAnatomy K-only-Mengen-Semantik ====\n";
    SAnat set;
    tr("insert(1) is_new", set.insert(1));
    tr("insert(2) is_new", set.insert(2));
    tr("insert(3) is_new", set.insert(3));
    tr("insert(2) erneut NICHT is_new (Set: keine Duplikate)", !set.insert(2));
    eq("size() == 3", set.size(), std::size_t{3});
    tr("contains(2) == true", set.contains(2));
    tr("contains(99) == false", !set.contains(99));
    tr("erase(2) == true", set.erase(2));
    eq("size() == 2 nach erase", set.size(), std::size_t{2});
    tr("contains(2) == false nach erase", !set.contains(2));

    cea::SetObserverSnapshot const o = set.observe_all();
    eq("Observer insert_count == 3", o.insert_count, std::uint64_t{3});
    eq("Observer contains_count == 3", o.contains_count, std::uint64_t{3});
    eq("Observer contains_hit_count == 1", o.contains_hit_count, std::uint64_t{1});
    eq("Observer contains_miss_count == 2", o.contains_miss_count, std::uint64_t{2});
    eq("Observer erase_count == 1", o.erase_count, std::uint64_t{1});
    eq("Observer peak_size == 3", o.peak_size, std::uint64_t{3});

    std::cout << "\n==== D9 Set: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
