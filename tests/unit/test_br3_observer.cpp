// test_br3_observer — BR-3 (2026-06-02, Doc 27 §3+§4-Gate-4) — Baum-Knoten trägt ECHTEN ObserverAggregate-
// Snapshot (kein Stub) einer REALEN Komposition + read-only Achsen-Definition. R5.B-Grenze ehrlich.
//
// Pfad B in-process: measure_composition<P> instanziiert den realen genus-ABI-Adapter, treibt das echte Such-
// Organ + allocator-Store, zieht tier_observe → NodeValue.observer. Beweis: der gemessene Knoten trägt echte
// Observer-Werte (search_insert/lookup > 0), ein UNGEMESSENER Knoten bleibt 0 (Kontrast Stub vs. echt).
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS /I... (volle Pfade)

#include "builder/experiment_tree/node_value_measurement.hpp"   // measure_composition<P>
#include "builder/experiment_tree/composition_registry.hpp"     // CompositionRegistry (Achsen-Definition)
#include "builder/experiment_tree/axis_path_serialization.hpp"  // serialize_composition_path<P>
#include "builder/experiment_tree/axis_reflect.hpp"             // push_static_axis
#include "builder/experiment_tree/experiment_tree.hpp"
#include <permutations/permutation_engine.hpp>

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

// 1er-PILOT: alle 17 Achsen mp_take_c<…,1> → ∏=1 (EINE reale Komposition, C1060-/OOM-leicht).
template <class List> struct PilotCfg { using StaticAxisVariants = List; };
using L0  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, 1>;
using L1  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>;
using L2  = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>;
using L3  = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;
using L4  = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, 1>;
using L5  = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, 1>;
using L6  = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;
using L7  = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, 1>;
using L8  = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;
using L9  = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>;
using L10 = mp::mp_take_c<ce::telemetry::TopicConfigSet::StaticAxisVariants, 1>;
using L11 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;
using L12 = mp::mp_take_c<ce::hardware::TopicConfigSet::StaticAxisVariants_09, 1>;
using L13 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>;
using L14 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;
using L15 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;
using L16 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;
using PilotEngine = perm::PermutationEngine<
    PilotCfg<L0>,  PilotCfg<L1>,  PilotCfg<L2>,  PilotCfg<L3>,  PilotCfg<L4>,  PilotCfg<L5>,
    PilotCfg<L6>,  PilotCfg<L7>,  PilotCfg<L8>,  PilotCfg<L9>,  PilotCfg<L10>, PilotCfg<L11>,
    PilotCfg<L12>, PilotCfg<L13>, PilotCfg<L14>, PilotCfg<L15>, PilotCfg<L16>>;

int main() {
    std::cout << "BR-3 (Pilot): Baum-Knoten trägt ECHTEN ObserverAggregate-Snapshot (kein Stub):\n";
    constexpr std::uint64_t kKeys = 256;

    // (1) EINE reale Komposition messen (Pfad B in-process über den realen genus-ABI-Adapter).
    ex::NodeValue measured; std::string path; bool got = false; std::size_t obs_axes = 0;
    PilotEngine::for_each_permutation([&]<class P>() {
        measured = ex::measure_composition<P>(kKeys);
        path     = ex::serialize_composition_path<P>();
        obs_axes = ex::composition_observable_axis_count<P>();
        got = true;
    });
    check_true("genau eine Pilot-Komposition gemessen", got && PilotEngine::count() == 1);

    // (2) ECHTER Observer (kein Stub): observer_real + getriebene search-Werte > 0.
    check_true("observer_real (via realem observe_all/tier_observe gezogen)", measured.observer_real);
    std::cout << "    search: insert=" << measured.observer.search_insert_count
              << " lookup=" << measured.observer.search_lookup_count
              << " peak_occ=" << measured.observer.search_peak_occupancy
              << " | alloc: bytes=" << measured.observer.alloc_bytes_allocated
              << " count=" << measured.observer.alloc_allocation_count
              << " | observable_axes=" << measured.observer.observable_axis_count
              << " fill=" << measured.observer.tier_fill_level << "\n";
    check_true("search_insert_count > 0 (ECHT getrieben, nicht Stub-0)", measured.observer.search_insert_count > 0);
    check_true("search_lookup_count > 0 (ECHT getrieben)", measured.observer.search_lookup_count > 0);
    check_true("tier_fill_level > 0 (Substrat real befüllt)", measured.observer.tier_fill_level > 0);
    // R5.B-Grenze EHRLICH: mind. search_algo ist ObservableAxis; observable_axis_count macht es transparent.
    check_true("observable_axis_count >= 1 (R5.B: mind. search_algo real)", measured.observer.observable_axis_count >= 1);
    std::cout << "    R5.B-Grenze: " << obs_axes << " von 17 Achsen sind ObservableAxis (Rest passive Deskriptoren)\n";

    // (3) In den Baum-Knoten ablegen (sparse value_map) + read-only zurücklesen.
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
    check_eq("Baum hat 1 Blatt (1er-Pilot)", tree.binary_count(), std::size_t{1});

    // Kontrast VOR dem Setzen: ungemessener Knoten = Default-0 (kein Stub-Wert).
    check_eq("ungemessen: search_insert_count == 0 (Default)", tree.node_value(path).observer.search_insert_count, std::uint64_t{0});
    tree.set_node_value(path, measured);
    ex::NodeValue read = tree.node_value(path);   // read-only Rücklesen (Diplomarbeit-Seite)
    check_true("Knoten trägt jetzt den ECHTEN Snapshot (read-only abrufbar)", read.observer_real);
    check_eq("Knoten-Snapshot == gemessener insert_count", read.observer.search_insert_count, measured.observer.search_insert_count);
    check_eq("measured_node_count == 1 (sparse: nur der gemessene)", tree.measured_node_count(), std::size_t{1});

    // (4) Achsen-Definition read-only je Knoten (BR-3): 17-Achsen-Definition via CompositionRegistry.
    ex::CompositionRegistry reg;
    reg.register_from_engine<PilotEngine>();
    ex::CompositionRecord const* rec = reg.lookup(path);
    check_true("Achsen-Definition für den Knoten abrufbar", rec != nullptr);
    if (rec) check_eq("Definition trägt alle 17 Achsen (achse,wrapper)", rec->definition.size(), std::size_t{17});

    std::cout << "\n==== BR-3 Observer: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
