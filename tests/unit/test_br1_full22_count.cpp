// test_br1_full22_count — BR-1 (2026-06-02, Doc 27 §4 Gate-1) — VOLL-22-Achsen-Bindung, literal.
//
// Verifiziert das Vollständigkeits-Gate (Doc 27 §4) über ALLE 22 realen Achsen — registry-getrieben,
// NICHT string-getrieben. Nutzt build_all_axis_levels() (registry_to_axis_levels.hpp) → 22 AxisLevels
// aus den ECHTEN Enabled-Listen, baut den Experiment-Baum, prüft die Kardinalitäts-Identität
// tree.binary_count() == ∏ mp_size(Enabled_i) == all_axes_binary_count() (Doc 27 §6: == PermutationEngine::
// count() per mp_size<mp_product<L…>> = ∏|L|, OHNE den C1060-infeasiblen mp_product-Typ-Baum).
//
// ⚠️ Dieser TU inkludiert ALLE 22 Achsen-Registries (registry_to_axis_levels.hpp). Er nutzt aber NUR
// mp_size + reflect_names (KEIN mp_product) — der C1060/OOM kam vom mp_product bzw. all_axes_umbrella,
// NICHT vom Include + mp_size. Dieser Test belegt literal, ob der Count-only-Voll-22-TU baubar ist.
//
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<include> /I<src> /I<alle build/generated/...-flags> /I<boost_mp11>

#include "builder/experiment_tree/registry_to_axis_levels.hpp"

#include <iostream>
#include <memory>
#include <set>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "BR-1 VOLL-22 (Doc 27 §4 Gate-1): registry-getriebene Bindung ALLER 22 Achsen:\n";

    // ── Die 22 realen Achsen aus den Registry-Enabled-Listen (registry-getrieben) ──
    std::vector<ex::AxisLevel> lv = ex::build_all_axis_levels();
    check_eq("Gate-2: 22 Achsen als Baum-Ebene", lv.size(), std::size_t{22});

    // Jede Achse hat ihr volles Enabled-Inventar (>0 reale Wrapper), block_id == Achsen-Name (Bidir.-Tag).
    bool nonempty = true, block_ok = true;
    std::size_t prod = 1;
    for (auto const& l : lv) {
        std::cout << "    " << l.axis << " : " << l.values.size() << " Wrapper  block_id=" << l.block_id
                  << (l.values.empty() ? "" : ("  (z.B. " + l.values.front() + ")")) << "\n";
        if (l.values.empty()) nonempty = false;
        if (l.block_id != l.axis) block_ok = false;
        prod *= l.values.size();
    }
    check_true("Gate-2: jede der 22 Achsen hat volles Enabled-Inventar (>0)", nonempty);
    check_true("block_id == Achsen-Name (Bidir.-Tag) für alle 22", block_ok);

    // GATE-1 (Doc 27 §4.1+§6): tree.binary_count() == ∏ mp_size(Enabled_i) == all_axes_binary_count()
    // (== PermutationEngine::count() per Kardinalitäts-Identität, OHNE mp_product-Materialisierung).
    constexpr std::size_t expected = ex::all_axes_binary_count();
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(lv);
    std::cout << "  ∏ (Laufzeit über values.size()) = " << prod << "\n";
    std::cout << "  all_axes_binary_count() (constexpr ∏ mp_size) = " << expected << "\n";
    std::cout << "  tree.binary_count() = " << tree.binary_count() << "\n";
    check_eq("Laufzeit-∏ == constexpr ∏ mp_size (reflect_names == Enabled-Inventar)", prod, expected);
    check_eq("GATE-1: tree.binary_count() == ∏ mp_size(Enabled_i)", tree.binary_count(), expected);

    // GATE (Bidir.): jeder materialisierte Knoten trägt block_id() == seine Achse, 22 distinkte Blöcke.
    std::size_t nodes = 0; bool all_block = true; std::set<std::string> blocks;
    tree.for_each_node([&](ex::INodeDescription const& d) {
        ++nodes; if (d.block_id() != d.axis()) all_block = false; blocks.insert(d.block_id());
    });
    check_true("Knoten materialisiert (>0)", nodes > 0);
    check_true("jeder Knoten block_id()==axis() (Bidir. auf dem Gesamtbaum)", all_block);
    check_eq("Knoten block-zuordbar (22 distinkte Achsen-Blöcke)", blocks.size(), std::size_t{22});

    std::cout << "\n==== BR-1 VOLL-22 Gate-1: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
