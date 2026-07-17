// BR-4 Phase 1 (2026-06-02, Doc 27 §3) — EMITTER: aus EINEM Baum-Blatt (reale Pilot-Komposition) die ECHTE
// Anatomie-Modul-Quelle schreiben (#include all_axes_umbrella.hpp + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC mit den
// 17 FQ-Achsen-Typen) + den Baum-Blatt-Pfad ausgeben. Das emittierte .cpp wird in Phase 2 als reale DLL gebaut.
//
// Build: cl /std:c++latest /EHsc /I... br4_emit.cpp  → br4_emit.exe <out_perm.cpp> <out_path.txt>

#include "builder/experiment_tree/axis_path_serialization.hpp" // serialize_composition_path / _from_slots
#include "anatomy/composition_factory.hpp"                     // CompositionFromPermTuple
#include <builder/codegen/adhoc_emitter.hpp>                   // render_adhoc_module_source + adhoc_macro_args
#include <permutations/permutation_engine.hpp>

#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>
#include <topics/io/topic_io_config_set.hpp>
#include <topics/migration/topic_migration_config_set.hpp>
#include <topics/filter/topic_filter_config_set.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp> // queuing q1/q2 (Doc 30 §8.0: SA-Achsen T17/T18)

#include <boost/mp11.hpp>
#include <fstream>
#include <iostream>
#include <string>

namespace ex   = comdare::cache_engine::builder::experiment;
namespace cg   = comdare::cache_engine::builder::codegen;
namespace an   = comdare::cache_engine::anatomy;
namespace perm = comdare::cache_engine::permutations;
namespace ce   = comdare::cache_engine;
namespace mp   = boost::mp11;

template <class List>
struct PilotCfg {
    using StaticAxisVariants = List;
};
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
using L11 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;
using L12 = mp::mp_take_c<ce::hardware::TopicConfigSet::StaticAxisVariants_09, 1>;
using L13 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>;
using L14 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;
using L15 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;
using L16 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;
using L17 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>; // queuing_q1 (Doc 30 §8.0)
using L18 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>; // queuing_q2 (Doc 30 §8.0)
using PilotEngine =
    perm::PermutationEngine<PilotCfg<L0>, PilotCfg<L1>, PilotCfg<L2>, PilotCfg<L3>, PilotCfg<L4>, PilotCfg<L5>,
                            PilotCfg<L6>, PilotCfg<L7>, PilotCfg<L8>, PilotCfg<L9>, PilotCfg<L11>, PilotCfg<L12>,
                            PilotCfg<L13>, PilotCfg<L14>, PilotCfg<L15>, PilotCfg<L16>, PilotCfg<L17>, PilotCfg<L18>>;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: br4_emit <out_perm.cpp> <out_path.txt>\n";
        return 2;
    }
    std::string source, tree_path, slot_path;
    PilotEngine::for_each_permutation([&]<class P>() {
        using Comp = an::CompositionFromPermTuple<P>; // AdHocComposition<17> (reale Komposition)
        source     = cg::render_adhoc_module_source(0, cg::adhoc_macro_args<Comp>()); // ECHTE Anatomie-Quelle
        tree_path  = ex::serialize_composition_path<P>();                             // Baum-Blatt-Pfad (BR-1/BR-2)
        slot_path  = ex::serialize_composition_from_slots<Comp>(); // aus den 17 Slots (Round-Trip-Beleg)
    });
    std::ofstream{argv[1], std::ios::trunc} << source;
    std::ofstream{argv[2], std::ios::trunc} << tree_path << "\n" << slot_path << "\n";
    std::cerr << "br4_emit: perm-Quelle geschrieben (" << source.size() << " B). Baum-Blatt-Pfad:\n  " << tree_path
              << "\n";
    std::cerr << "br4_emit: enthält COMDARE_DEFINE_ANATOMY_MODULE_ADHOC: "
              << (source.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC") != std::string::npos ? "ja" : "NEIN") << "\n";
    return (tree_path == slot_path && !source.empty()) ? 0 : 1; // Pfad-Round-Trip P→Composition
}
