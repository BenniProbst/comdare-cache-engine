// test_br1_subset — BR-1 (2026-06-02, Doc 27 §3+§6) — OOM-SICHERE Mechanismus-Verifikation gegen ECHTE Achsen.
//
// Der Voll-22-Achsen-TU (registry_to_axis_levels.hpp) ist compiler-heap-schwer (Windows-OOM). Dieser Test
// verifiziert den GENERISCHEN Registry→Baum-Mechanismus (axis_reflect.hpp) LEICHT gegen eine reale Achsen-
// Teilmenge (nodes + memory_layout — keine schweren Vendor/Paper-Mixins): reflect_names liefert ECHTE Wrapper-
// Namen, tree.binary_count() == ∏ enabled_count (Gate-1-Formel), block_id-Rück-Referenz je Knoten (Bidir.).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<include> /I<src> /I<build/generated+flags> /I<boost_mp11>

#include "builder/experiment_tree/axis_reflect.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"

#include <topics/nodes/topic_nodes_config_set.hpp>                 // axis_02 path_compression + axis_04 node_type
#include <topics/memory_layout/topic_memory_layout_config_set.hpp> // axis_05 memory_layout

#include <iostream>
#include <memory>
#include <set>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;
namespace ce = comdare::cache_engine;

// Reale Enabled-Listen (namespace-sicher via TopicConfigSet) — eine leichte, OOM-sichere Teilmenge der 22.
using AxNode   = ce::nodes::TopicConfigSet::StaticAxisVariants_04;      // node_type
using AxPath   = ce::nodes::TopicConfigSet::StaticAxisVariants_02;      // path_compression
using AxLayout = ce::memory_layout::TopicConfigSet::StaticAxisVariants; // memory_layout

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "BR-1 (Subset, OOM-sicher): registry-getriebene Reflektion gegen ECHTE Achsen:\n";

    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<AxNode>(lv, "node_type");
    ex::push_static_axis<AxPath>(lv, "path_compression");
    ex::push_static_axis<AxLayout>(lv, "memory_layout");

    check_eq("3 reale Achsen reflektiert", lv.size(), std::size_t{3});
    bool nonempty = true, block_ok = true;
    for (auto const& l : lv) {
        std::cout << "    " << l.axis << " : " << l.values.size() << " Wrapper  block_id=" << l.block_id
                  << (l.values.empty() ? "" : ("  (z.B. " + l.values.front() + ")")) << "\n";
        if (l.values.empty()) nonempty = false;
        if (l.block_id != l.axis) block_ok = false;
    }
    check_true("jede Achse hat reale Wrapper (Enabled-Inventar)", nonempty);
    check_true("block_id == Achsen-Name (Bidir.-Tag)", block_ok);
    // node_type-Wrapper sind die realen ART-Node-Namen:
    bool has_node4 = false;
    for (auto const& v : lv[0].values)
        if (v == "node4") has_node4 = true;
    check_true("node_type enthält realen Wrapper 'node4'", has_node4);

    // GATE-1-Formel: tree.binary_count() == ∏ enabled_count(Achse_i) (registry-getrieben, ohne mp_product)
    constexpr std::size_t expected =
        ex::enabled_count<AxNode> * ex::enabled_count<AxPath> * ex::enabled_count<AxLayout>;
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(lv);
    std::cout << "  tree.binary_count()=" << tree.binary_count() << "  ∏enabled_count=" << expected << "\n";
    check_eq("GATE-1-Formel: binary_count == ∏ enabled_count", tree.binary_count(), expected);

    // BIDIREKTIONALITÄT: jeder materialisierte Knoten trägt block_id() == seine Achse (Rück-Referenz).
    std::size_t           nodes     = 0;
    bool                  all_block = true;
    std::set<std::string> blocks;
    tree.for_each_node([&](ex::INodeDescription const& d) {
        ++nodes;
        if (d.block_id() != d.axis()) all_block = false;
        blocks.insert(d.block_id());
    });
    check_true("Knoten materialisiert (>0)", nodes > 0);
    check_true("jeder Knoten block_id()==axis() (Bidir. auf dem Gesamtbaum)", all_block);
    check_true("Knoten sind block-zuordbar (3 distinkte Blöcke)", blocks.size() == 3);

    std::cout << "\n==== BR-1 Subset-Mechanismus: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
