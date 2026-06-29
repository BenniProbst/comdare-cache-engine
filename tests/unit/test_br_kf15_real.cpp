// test_br_kf15_real — Gate-5 (2026-06-02, Doc 26 §3 / Doc 27 §4) — inverse Signatur-Projektion (KF-15) über
// REALE, registry-getriebene Kompositionen (NICHT string-Stubs).
//
// Baut den Experiment-Baum aus ECHTEN Enabled-Listen (BR-1, push_static_axis): search_algo GEPINNT (1 Wert →
// = die Paper-Signatur) + 3 freigegebene Achsen (node_type/memory_layout/path_compression). Die inverse
// Auswertung (ReadOnlyResultView, KF-15) projiziert die gemessenen Blätter per gepinnter Signatur — die
// Signatur trägt REALE Wrapper-Namen, der NodeValue (BR-3) ist echt. Doc 26 §3: lineare Signatur-Filter-Projektion.
//
// Build: cl /std:c++latest /EHsc /I<…> /I<build/generated…>  (leicht: nur traversal+nodes+memory_layout-ConfigSets)

#include "builder/experiment_tree/inverse_signature_eval.hpp" // ReadOnlyResultView (KF-15)
#include "builder/experiment_tree/axis_reflect.hpp"           // push_static_axis (BR-1)
#include "builder/experiment_tree/experiment_tree.hpp"

#include <topics/traversal/topic_traversal_config_set.hpp> // search_algo (axis_03a)
#include <topics/nodes/topic_nodes_config_set.hpp>         // node_type (04) + path_compression (02)
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>

#include <boost/mp11.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;
namespace ce = comdare::cache_engine;
namespace mp = boost::mp11;

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

// REALE Achsen: search_algo GEPINNT (1 → Paper-Signatur), node/layout/path_compression FREIGEGEBEN (2 → variieren).
using SApin = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, 1>;
using NT2   = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 2>;
using ML2   = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 2>;
using PC2   = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 2>;

int main() {
    std::cout << "Gate-5 (KF-15): inverse Signatur-Projektion über REALE registry-getriebene Kompositionen:\n";

    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<SApin>(lv, "search_algo");    // 1 Wert → gepinnt (= Paper-Signatur)
    ex::push_static_axis<NT2>(lv, "node_type");        // 2 → freigegeben
    ex::push_static_axis<ML2>(lv, "memory_layout");    // 2 → freigegeben
    ex::push_static_axis<PC2>(lv, "path_compression"); // 2 → freigegeben

    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(lv);
    check_eq("binary_count == 1×2×2×2", tree.binary_count(), std::size_t{8});

    // Gepinnte Paper-Signatur + erstes Blatt einsammeln (vor der const-View) + BR-3-NodeValue setzen.
    std::string paper_sig, first_bin;
    tree.for_each_binary([&](std::string const& b, std::string const& p, ex::TreeNode const&) {
        if (first_bin.empty()) {
            first_bin = b;
            paper_sig = p;
        }
    });
    std::cout << "    Paper-Signatur (gepinnt): " << paper_sig << "\n";
    check_true("Signatur registry-getrieben (beginnt mit search_algo=<realer Wrapper>)",
               paper_sig.rfind("search_algo=", 0) == 0 && paper_sig.find("=/") == std::string::npos);
    check_true("Signatur trägt NUR die gepinnte Achse (search_algo), keine freigegebene",
               paper_sig.find("node_type=") == std::string::npos);

    ex::NodeValue nv; // BR-3: echter Observer-Snapshot (hier direkt gesetzt — ohne schweren DLL-Bau)
    nv.observer.search_insert_count = 256;
    nv.observer.tier_fill_level     = 256;
    nv.observer_real                = true;
    nv.has_result                   = true;
    nv.measured_setting_count       = 1;
    nv.sum_op_count                 = 512;
    tree.set_node_value(first_bin, nv);

    // KF-15 inverse Auswertung über die REALEN Pfade.
    ex::ReadOnlyResultView rv{tree};
    auto const             sigs = rv.signatures();
    check_eq("genau 1 distinkte Paper-Signatur (nur search_algo gepinnt)", sigs.size(), std::size_t{1});
    auto const bins = rv.binaries_with_signature(paper_sig);
    check_eq("alle 8 realen Blätter projizieren auf die Paper-Signatur", bins.size(), std::size_t{8});
    bool real_paths = true;
    for (auto const& b : bins)
        if (b.find("node_type=") == std::string::npos || b.find("memory_layout=") == std::string::npos)
            real_paths = false;
    check_true("projizierte Blätter sind volle reale Komposition-Pfade", real_paths);

    // BR-3-Integration: die Aggregation über die Signatur sieht den echten gemessenen Knoten.
    ex::NodeValue const agg = rv.aggregate_for_signature(paper_sig);
    check_true("aggregate_for_signature sieht den gemessenen Knoten (has_result)", agg.has_result);
    check_true("Aggregation zählt >=1 gemessene Einstellung", agg.measured_setting_count >= 1);

    std::cout << "\n==== Gate-5 KF-15 (real): " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
