#pragma once
// L-LAZY-E2E (gate-frei, 2026-06-03) — lazy_pilot_engine: die EINE gemeinsame Pilot-Definition für den
// Lazy-E2E-Treiber (cache_engine_builder_iterator.hpp). Hier (und NUR hier) leben:
//   • PilotEngine    — ein PermutationEngine, dessen kartesisches Produkt ≥150 REALE SA-Kompositionen ergibt
//                       (search_algo × node_type × memory_layout × prefetch = 4·4·5·4 = 320, > 150).
//   • PilotSubEngine<N> — dieselben statischen Achsen, aber je Achse auf K Varianten reduziert (mp_take_c) für den
//                       KLEINEN Pilot (z.B. 1·2·2·1 = 4 Binaries) — für den schnellen E2E-Test/Probelauf.
//   • build_pilot_axis_levels<Engine reduktion> — die AxisLevels für den ExperimentTree (gleiche reduzierten Listen
//                       + gleiche Achsen-Namen wie der Engine → IDENTISCHE binary_ids, BR-1↔BR-2-Round-Trip),
//                       PLUS die DYNAMISCHEN Dimensionen (concurrency.thread_count / prefetch.prefetch_distance) als
//                       virtuelle for-Schleifen (NICHT teil der binary_id — Laufzeit-Variation auf der GELADENEN DLL).
//
// COMPILE-FEASIBILITY (Doc 27 §6, Task-Vorgabe): das VOLLE 22-Achsen-Produkt ist C1060. Diese Datei materialisiert
// in EINEM TU NUR TYP-NAMEN (build_pilot_source_map → type_name<T>, KEINE Anatomie-Instanz) für ≤320 Permutationen;
// die SCHWERE Anatomie-Instanziierung passiert je separat emittiertem perm_<id>.cpp (eine DLL = ein TU = EINE
// Komposition), nie gesammelt. Damit ist ≥150 erreichbar OHNE C1060. Der Treiber baut ohnehin nur die ersten N
// (BuildSelection) — der volle 320er-Raum wird gezählt, nicht voll gebaut.
//
// Umbrella-schwer (all_axes_umbrella via pilot_source_map.hpp) → gehört in die HARNESS-/Test-.cpp, NICHT in den
// engine-agnostischen Treiber-Header. C++23.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>  // run_lazy_static_then_dynamic
#include <builder/experiment_tree/pilot_source_map.hpp>               // build_pilot_source_map / make_source_gen_from_map
#include <builder/experiment_tree/axis_reflect.hpp>                   // push_static_axis
#include <permutations/permutation_engine.hpp>                        // PermutationEngine

// Die statischen Pilot-Achsen-ConfigSets (echte Wrapper):
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
#include <topics/queuing/topic_queuing_config_set.hpp>

#include <boost/mp11.hpp>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace perm = ::comdare::cache_engine::permutations;
namespace ce   = ::comdare::cache_engine;
namespace mp   = boost::mp11;

// PermutationEngine erwartet je Topic ::StaticAxisVariants. Wir wickeln eine beliebige mp_list als ConfigSet.
template <class List> struct Cfg { using StaticAxisVariants = List; };

// ── Die 19 AdHocComposition-Slot-Listen. Variiert (Fanout) werden die 4 Achsen search_algo / node_type /
//    memory_layout / prefetch; alle übrigen 15 sind auf 1 Variante gepinnt (identisch über alle Tiere → der
//    gemessene Unterschied ist den 4 variierten Achsen zurechenbar; die binary_id pinnt die 15 mit). ──
// Voll-Pilot (≥150): K_search=4, K_node=4, K_layout=5, K_prefetch=4 → 320 reale Kompositionen.
// Klein-Pilot (Test): über PilotSubEngine (mp_take_c) auf 1·2·2·1 = 4 reduziert.
template <std::size_t KSearch, std::size_t KNode, std::size_t KLayout, std::size_t KPrefetch>
struct PilotAxes {
    using L00 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03a, KSearch>;     // search_algo
    using L01 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03b, 1>;           // cache_traversal
    using L02 = mp::mp_take_c<ce::traversal::TopicConfigSet::StaticAxisVariants_03m, 1>;           // mapping
    using L03 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_02, 1>;                // path_compression
    using L04 = mp::mp_take_c<ce::nodes::TopicConfigSet::StaticAxisVariants_04, KNode>;            // node_type
    using L05 = mp::mp_take_c<ce::memory_layout::TopicConfigSet::StaticAxisVariants, KLayout>;     // memory_layout
    using L06 = mp::mp_take_c<ce::allocator::TopicConfigSet::StaticAxisVariants, 1>;               // allocator
    using L07 = mp::mp_take_c<ce::prefetch::TopicConfigSet::StaticAxisVariants, KPrefetch>;        // prefetch
    using L08 = mp::mp_take_c<ce::concurrency::TopicConfigSet::StaticAxisVariants, 1>;             // concurrency
    using L09 = mp::mp_take_c<ce::serialization::TopicConfigSet::StaticAxisVariants, 1>;           // serialization
    using L10 = mp::mp_take_c<ce::telemetry::TopicConfigSet::StaticAxisVariants, 1>;               // telemetry
    using L11 = mp::mp_take_c<ce::value_handle::TopicConfigSet::StaticAxisVariants, 1>;            // value_handle
    using L12 = mp::mp_take_c<ce::hardware::TopicConfigSet::StaticAxisVariants_09, 1>;             // isa
    using L13 = mp::mp_take_c<ce::search_engine::TopicConfigSet::StaticAxisVariants, 1>;           // index_organization
    using L14 = mp::mp_take_c<ce::io::TopicConfigSet::StaticAxisVariants, 1>;                      // io_dispatch
    using L15 = mp::mp_take_c<ce::migration::TopicConfigSet::StaticAxisVariants, 1>;               // migration_policy
    using L16 = mp::mp_take_c<ce::filter::TopicConfigSet::StaticAxisVariants, 1>;                  // filter
    using L17 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1, 1>;              // queuing_q1
    using L18 = mp::mp_take_c<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2, 1>;              // queuing_q2

    using Engine = perm::PermutationEngine<
        Cfg<L00>, Cfg<L01>, Cfg<L02>, Cfg<L03>, Cfg<L04>, Cfg<L05>, Cfg<L06>,
        Cfg<L07>, Cfg<L08>, Cfg<L09>, Cfg<L10>, Cfg<L11>, Cfg<L12>, Cfg<L13>,
        Cfg<L14>, Cfg<L15>, Cfg<L16>, Cfg<L17>, Cfg<L18>>;

    /// Die statischen AxisLevels (gleiche reduzierte Listen + Achsen-Namen wie der Engine = identische binary_ids).
    [[nodiscard]] static std::vector<ex::AxisLevel> static_levels() {
        std::vector<ex::AxisLevel> lv;
        ex::push_static_axis<L00>(lv, "search_algo");        ex::push_static_axis<L01>(lv, "cache_traversal");
        ex::push_static_axis<L02>(lv, "mapping");            ex::push_static_axis<L03>(lv, "path_compression");
        ex::push_static_axis<L04>(lv, "node_type");          ex::push_static_axis<L05>(lv, "memory_layout");
        ex::push_static_axis<L06>(lv, "allocator");          ex::push_static_axis<L07>(lv, "prefetch");
        ex::push_static_axis<L08>(lv, "concurrency");        ex::push_static_axis<L09>(lv, "serialization");
        ex::push_static_axis<L10>(lv, "telemetry");          ex::push_static_axis<L11>(lv, "value_handle");
        ex::push_static_axis<L12>(lv, "isa");                ex::push_static_axis<L13>(lv, "index_organization");
        ex::push_static_axis<L14>(lv, "io_dispatch");        ex::push_static_axis<L15>(lv, "migration_policy");
        ex::push_static_axis<L16>(lv, "filter");
        ex::push_static_axis<L17>(lv, "queuing_q1");         ex::push_static_axis<L18>(lv, "queuing_q2");
        return lv;
    }
};

// Voll-Pilot: 4·4·5·4 = 320 reale SA-Kompositionen (≥150). Klein-Pilot: 1·2·2·1 = 4 (für den E2E-Test).
using FullPilot  = PilotAxes<4, 4, 5, 4>;
using SmallPilot = PilotAxes<1, 2, 2, 1>;

/// Die DYNAMISCHEN Dimensionen = virtuelle for-Schleifen auf der GELADENEN DLL (NICHT teil der binary_id).
/// concurrency.thread_count ∈ {1,2,4} (axis_08) + prefetch.prefetch_distance ∈ {0,8} (axis_07) → 3·2 = 6 Settings,
/// PLUS (D, KF-10) die Wiederholungs-Achse repetition.repetition_index ∈ {0..n_repeats-1} (Default 3) → ×n_repeats.
/// Variablen-Namen der ersten beiden = ComdareResourceControlV1-Felder (runtime_variable_loop.hpp set_field) →
/// real angewandt; `repetition_index` ist KEIN POD-Feld → set_field ignoriert es (architektonische Ausnahme),
/// d.h. die Rep-Dim verändert den Tier NICHT, sie multipliziert nur die Mess-Wiederholungen (je Rep eine eigene
/// Roh-CSV-Zeile, NIE interpoliert — strukturell durch die DynamicDim-Expansion garantiert).
/// Achse 2 (INC-2, 2026-06-07): `workload_values` (z.B. {"A","C","E","F"}) ergänzt eine VIERTE, INNERSTE
/// dynamische Dimension `workload.workload_id` NACH repetition. Leer (Default) ⇒ keine Workload-Dim
/// (rückwärtskompatibel = alter fixer Workload). `workload_id` ist KEIN ComdareResourceControlV1-POD-Feld →
/// set_field ignoriert es (architektonische Ausnahme wie repetition_index); der Iterator dispatcht das Label
/// an perm_runner::run_workload_perm (= der bereits implementierte Interpreter). Workload variiert die Op-Sequenz
/// auf der GELADENEN DLL ohne Neu-Bau → dasselbe Binary gegen alle Workloads = Achse 2 des kartesischen Kreuzes.
[[nodiscard]] inline std::vector<ex::DynamicDim> pilot_dynamic_dims(
    std::uint32_t n_repeats = 3,
    std::vector<std::string> workload_values = {}) {
    std::uint32_t const reps = (n_repeats == 0) ? 1u : n_repeats;   // 0 → 1 normalisieren (RepetitionPlan-Semantik)
    std::vector<std::string> rep_vals;
    rep_vals.reserve(reps);
    for (std::uint32_t r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
    std::vector<ex::DynamicDim> dims = {
        ex::DynamicDim{"concurrency", "thread_count",      {"1", "2", "4"},        "concurrency"},
        ex::DynamicDim{"prefetch",    "prefetch_distance", {"0", "8"},             "prefetch"},
        ex::DynamicDim{"repetition",  "repetition_index",  std::move(rep_vals),    "repetition"},
    };
    if (!workload_values.empty())   // Achse 2: innerste dynamische Ebene (nach repetition)
        dims.push_back(ex::DynamicDim{"workload", "workload_id", std::move(workload_values), "workload"});
    return dims;
}

/// build_pilot_levels<Pilot>() — statische Achsen + dynamische Dimensionen zusammengeführt (= der Gesamt-Level-Satz
/// für ExperimentTree::build). Der Baum filtert intern static_filter()/dynamic_filter() (static/dynamic = Knoten-
/// Eigenschaft, gleichrangig). Mit `with_dynamic=false` nur statische Achsen (1 Mess-Punkt je Binary).
/// `n_repeats` (Default 3) = Anzahl der Wiederholungs-Achsen-Werte (D/KF-10).
template <class Pilot>
[[nodiscard]] inline std::vector<ex::AxisLevel> build_pilot_levels(bool with_dynamic = true,
                                                                   std::uint32_t n_repeats = 3,
                                                                   std::vector<std::string> workload_values = {}) {
    std::vector<ex::AxisLevel> lv = Pilot::static_levels();
    if (with_dynamic) {
        for (auto const& d : pilot_dynamic_dims(n_repeats, std::move(workload_values)))
            lv.push_back(ex::AxisLevel{d.axis, d.values, /*is_static=*/false, d.variable, d.block_id});
    }
    return lv;
}

/// make_pilot_source_gen<Pilot>() — die reale-Anatomie SourceGenFn (binary_id → echte Modul-Quelle) aus dem Pilot.
/// Iteriert die Pilot-Permutationen EINMAL (nur Typ-Namen) → Brücken-map → SourceGenFn (pilot_source_map.hpp).
template <class Pilot>
[[nodiscard]] inline ex::SourceGenFn make_pilot_source_gen() {
    return ex::make_source_gen_from_map(ex::build_pilot_source_map<typename Pilot::Engine>());
}

}  // namespace comdare::cache_engine::thesis_lazy
