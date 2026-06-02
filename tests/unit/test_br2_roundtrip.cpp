// test_br2_roundtrip — BR-2 (2026-06-02, Doc 27 §3) — Baum-Blatt ↔ reale AdHocComposition<17> Round-Trip.
//
// Beweist gegen ECHTE Wrapper: jeder statische Baum-Blatt-Pfad bildet auf GENAU EINE reale, materialisierbare
// AdHocComposition<17> ab, und die Pfad-Serialisierung ist BR-1↔BR-2 IDENTISCH (axis_path_serialization.hpp).
//
// C1060-SICHER: das volle 17-Achsen-mp_product über reale Enabled-Inventare sprengt den Heap → wir nutzen ein
// PILOT-Engine (schwere Achsen ×1, leichte node_type/memory_layout ×2 → ∏=4). Doc 27 §6: nur EIN (bzw. wenige
// Pilot-)Blätter werden compile-time materialisiert, NIE der ganze Typ-Baum.
//
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<libs/cache_engine/src> /I<include> /I<build/generated…> /I<boost_mp11>

#include "builder/experiment_tree/composition_registry.hpp"   // CompositionRegistry, composition_definition
#include "builder/experiment_tree/axis_reflect.hpp"           // push_static_axis
#include "builder/experiment_tree/experiment_tree.hpp"
#include <permutations/permutation_engine.hpp>                // PermutationEngine

// Die 17 Komposition-Topic-ConfigSets (echte Wrapper):
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/telemetry/topic_telemetry_config_set.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>
#include <topics/io/topic_io_config_set.hpp>
#include <topics/migration/topic_migration_config_set.hpp>
#include <topics/filter/topic_filter_config_set.hpp>

#include <boost/mp11.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <string>

namespace ex   = comdare::cache_engine::builder::experiment;
namespace perm = comdare::cache_engine::permutations;
namespace ce   = comdare::cache_engine;
namespace mp   = boost::mp11;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

// ── PILOT: PermutationEngine erwartet je Topic ::StaticAxisVariants. Wir reduzieren die realen Enabled-Listen
//    via mp_take_c (schwere Achsen ×1, leichte ×2) — klein genug für mp_product (kein C1060). ──
template <class List> struct PilotCfg { using StaticAxisVariants = List; };

using L0  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, 1>;     // search_algo ×1 (schwer)
using L1  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>;     // cache_traversal
using L2  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>;     // mapping
using L3  = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;          // path_compression
using L4  = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 2>;          // node_type ×2 (Fanout)
using L5  = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 2>;     // memory_layout ×2 (Fanout)
using L6  = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;         // allocator ×1 (schwer)
using L7  = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, 1>;
using L8  = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;
using L9  = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>;
using L10 = mp::mp_take_c<ce::telemetry::TopicConfigSet::StaticAxisVariants, 1>;
using L11 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;
using L12 = mp::mp_take_c<ce::hardware::TopicConfigSet::StaticAxisVariants_09, 1>;       // isa
using L13 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>;     // index_organization
using L14 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;                // io_dispatch
using L15 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;         // migration_policy
using L16 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;            // filter

using PilotEngine = perm::PermutationEngine<
    PilotCfg<L0>,  PilotCfg<L1>,  PilotCfg<L2>,  PilotCfg<L3>,  PilotCfg<L4>,  PilotCfg<L5>,
    PilotCfg<L6>,  PilotCfg<L7>,  PilotCfg<L8>,  PilotCfg<L9>,  PilotCfg<L10>, PilotCfg<L11>,
    PilotCfg<L12>, PilotCfg<L13>, PilotCfg<L14>, PilotCfg<L15>, PilotCfg<L16>>;

int main() {
    std::cout << "BR-2 (Pilot, C1060-sicher): Baum-Blatt ↔ reale AdHocComposition<17> Round-Trip:\n";

    // (1) CompositionRegistry aus dem PILOT-Engine: jede Permutation → reale AdHocComposition<17>.
    ex::CompositionRegistry reg;
    reg.register_from_engine<PilotEngine>();
    std::cout << "  PilotEngine::count() = " << PilotEngine::count() << "  reg.size() = " << reg.size() << "\n";
    check_eq("reg.size() == PilotEngine::count()", reg.size(), PilotEngine::count());
    check_true("∏ > 1 (echtes Fanout: node_type ×2 · memory_layout ×2)", reg.size() > 1);

    // (2) Round-Trip P → CompositionFromPermTuple → AdHocComposition<17>: path (aus PermTuple) == slot_path
    //     (aus den 17 named Slots) + materialisiert + 17-Achsen-Definition.
    bool rt = true; std::size_t def_ok = 0;
    reg.for_each([&](ex::CompositionRecord const& r) {
        if (r.path != r.slot_path)        rt = false;   // P→Composition Slot-Reihenfolge verlustfrei
        if (!r.materialized)              rt = false;   // CompositionFromPermTuple<P> kompilierte
        if (r.definition.size() == 17)    ++def_ok;     // reale Achsen-Definition je Slot
    });
    check_true("Round-Trip: path == slot_path + materialized (alle)", rt);
    check_eq("jede Komposition hat 17-Achsen-Definition", def_ok, reg.size());

    // (3) Baum über DIESELBEN Pilot-Listen + Achsen-Namen (= kCompositionAxisNames) → identische Pfade.
    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<L0>(lv,  "search_algo");        ex::push_static_axis<L1>(lv,  "cache_traversal");
    ex::push_static_axis<L2>(lv,  "mapping");            ex::push_static_axis<L3>(lv,  "path_compression");
    ex::push_static_axis<L4>(lv,  "node_type");          ex::push_static_axis<L5>(lv,  "memory_layout");
    ex::push_static_axis<L6>(lv,  "allocator");          ex::push_static_axis<L7>(lv,  "prefetch");
    ex::push_static_axis<L8>(lv,  "concurrency");        ex::push_static_axis<L9>(lv,  "serialization");
    ex::push_static_axis<L10>(lv, "telemetry");          ex::push_static_axis<L11>(lv, "value_handle");
    ex::push_static_axis<L12>(lv, "isa");                ex::push_static_axis<L13>(lv, "index_organization");
    ex::push_static_axis<L14>(lv, "io_dispatch");        ex::push_static_axis<L15>(lv, "migration_policy");
    ex::push_static_axis<L16>(lv, "filter");

    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(lv);
    check_eq("tree.binary_count() == reg.size()", tree.binary_count(), reg.size());

    // (4) DER BR-2-BEWEIS: jedes Baum-Blatt (binary_id) schlägt GENAU EINE reale Komposition nach (lookup).
    std::size_t leaves = 0, matched = 0; std::set<std::string> leaf_paths;
    tree.for_each_binary([&](std::string const& bin, std::string const&, ex::TreeNode const&) {
        ++leaves; leaf_paths.insert(bin);
        if (ex::CompositionRecord const* r = reg.lookup(bin)) { if (r->path == bin) ++matched; }
    });
    check_eq("Baum-Blätter (distinkt)", leaf_paths.size(), reg.size());
    check_eq("JEDES Blatt round-trippt auf eine reale Komposition", matched, leaves);

    // (5) Umkehrung: jeder Registry-Pfad ist ein Baum-Blatt (Pfad-Mengen BR-1 == BR-2).
    bool all_reg_in_tree = true;
    reg.for_each([&](ex::CompositionRecord const& r) { if (!leaf_paths.contains(r.path)) all_reg_in_tree = false; });
    check_true("jede reale Komposition ist ein Baum-Blatt (Pfad-Mengen identisch)", all_reg_in_tree);

    std::cout << "\n==== BR-2 Round-Trip: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
