#pragma once
// BR-1 (2026-06-02, Doc 27 §3) — registry_to_axis_levels: reflektiert ALLE 22 realen Achsen aus den Registry-
// Enabled-Listen (mp_lists) in die Experiment-Baum-AxisLevels — REGISTRY-getrieben, NICHT string-getrieben.
// Ersetzt die externe XML-String-AxisRegistry (profile_to_tree) als Quelle der Wahrheit.
//
// User-Korrektur 2026-06-02: KEINE 17-Achsen-Bindung, sondern ALLE 22 (15 Topics). 17 davon sind AdHocComposition-
// Slots T0..T16; die übrigen 5 Registry-Achsen = 3 build-only-Achsen (page_type/01, simd_extension/09b,
// general_hardware/12; DefinitionOnly) + die 2 queuing-KOMPOSITION-Achsen q1/q2 (gehören zur 19-Slot-Komposition
// als T17/T18, NICHT „andere Gattung" — nur spät in der Registry-Reihenfolge), die der Baum EBENFALLS bindet.
// Anker (namespace-sicher): TopicConfigSet::StaticAxisVariants_* (+ axis_01_page_type::EnabledPageTypes).
//
// ⚠️ OOM-HINWEIS: dieser Header inkludiert ALLE Achsen-Registries → in EINEM TU compiler-heap-schwer (Windows-OOM
// beobachtet). Der Reflektions-MECHANISMUS wird daher LEICHT gegen eine Teilmenge verifiziert (axis_reflect.hpp +
// wenige ConfigSets, test_br1_subset); die Voll-22-Bindung baut der projekteigene CMake-Build RAM-verwaltet (KF-16b).
// C++23, header-only.

#include "axis_reflect.hpp"   // generische Reflektions-Helfer (reflect_names/enabled_count/push_static_axis)

// Die Topic-Config-Sets, die die Achsen tragen (jedes inkludiert seine Achsen-Registries).
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_registry.hpp>  // page_type (nicht im nodes-ConfigSet)
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

#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Die 22 realen Achsen-Enabled-Listen, namespace-sicher via TopicConfigSet (+ page_type-Registry) ──
namespace axes22 {
namespace ce = ::comdare::cache_engine;
// 17 Kern-Komposition-Achsen T0..T16 (die AdHocComposition hat 19 = diese 17 + q1/q2 @ T17/T18):
using T00_search_algo        = ce::traversal::TopicConfigSet::StaticAxisVariants_03a;
using T01_cache_traversal    = ce::traversal::TopicConfigSet::StaticAxisVariants_03b;
using T02_mapping            = ce::traversal::TopicConfigSet::StaticAxisVariants_03m;
using T03_path_compression   = ce::nodes::TopicConfigSet::StaticAxisVariants_02;
using T04_node_type          = ce::nodes::TopicConfigSet::StaticAxisVariants_04;
using T05_memory_layout      = ce::memory_layout::TopicConfigSet::StaticAxisVariants;
using T06_allocator          = ce::allocator::TopicConfigSet::StaticAxisVariants;
using T07_prefetch           = ce::prefetch::TopicConfigSet::StaticAxisVariants;
using T08_concurrency        = ce::concurrency::TopicConfigSet::StaticAxisVariants;
using T09_serialization      = ce::serialization::TopicConfigSet::StaticAxisVariants;
using T10_telemetry          = ce::telemetry::TopicConfigSet::StaticAxisVariants;
using T11_value_handle       = ce::value_handle::TopicConfigSet::StaticAxisVariants;
using T12_isa                = ce::hardware::TopicConfigSet::StaticAxisVariants_09;
using T13_index_organization = ce::search_engine::TopicConfigSet::StaticAxisVariants;
using T14_io_dispatch        = ce::io::TopicConfigSet::StaticAxisVariants;
using T15_migration_policy   = ce::migration::TopicConfigSet::StaticAxisVariants;
using T16_filter             = ce::filter::TopicConfigSet::StaticAxisVariants;
// 5 weitere Registry-Achsen: 3 build-only (page_type/simd_extension/general_hardware) + 2 queuing-Komposition (q1/q2, gehoeren zur Komposition):
using T17_page_type          = ce::nodes::axis_01_page_type::EnabledPageTypes;
using T18_simd_extension     = ce::hardware::TopicConfigSet::StaticAxisVariants_09b;
using T19_general_hardware   = ce::hardware::TopicConfigSet::StaticAxisVariants_12;
using T20_queuing_q1         = ce::queuing::TopicConfigSet::StaticAxisVariants_Q1;
using T21_queuing_q2         = ce::queuing::TopicConfigSet::StaticAxisVariants_Q2;
}  // namespace axes22

/// Baut ALLE 22 Achsen als statische AxisLevels aus den REALEN Enabled-Listen (Fanout = volles Enabled-Inventar,
/// block_id-getaggt → Knoten-Rück-Referenz). Die 17 Kern-Achsen zuerst (T0..T16), dann 3 build-only (page_type/simd/hw) + die 2 queuing-Komposition-Achsen (q1/q2).
[[nodiscard]] inline std::vector<AxisLevel> build_all_axis_levels() {
    std::vector<AxisLevel> lv;
    lv.reserve(22);
    push_static_axis<axes22::T00_search_algo>(lv,        "search_algo");
    push_static_axis<axes22::T01_cache_traversal>(lv,    "cache_traversal");
    push_static_axis<axes22::T02_mapping>(lv,            "mapping");
    push_static_axis<axes22::T03_path_compression>(lv,   "path_compression");
    push_static_axis<axes22::T04_node_type>(lv,          "node_type");
    push_static_axis<axes22::T05_memory_layout>(lv,      "memory_layout");
    push_static_axis<axes22::T06_allocator>(lv,          "allocator");
    push_static_axis<axes22::T07_prefetch>(lv,           "prefetch");
    push_static_axis<axes22::T08_concurrency>(lv,        "concurrency");
    push_static_axis<axes22::T09_serialization>(lv,      "serialization");
    push_static_axis<axes22::T10_telemetry>(lv,          "telemetry");
    push_static_axis<axes22::T11_value_handle>(lv,       "value_handle");
    push_static_axis<axes22::T12_isa>(lv,                "isa");
    push_static_axis<axes22::T13_index_organization>(lv, "index_organization");
    push_static_axis<axes22::T14_io_dispatch>(lv,        "io_dispatch");
    push_static_axis<axes22::T15_migration_policy>(lv,   "migration_policy");
    push_static_axis<axes22::T16_filter>(lv,             "filter");
    push_static_axis<axes22::T17_page_type>(lv,          "page_type");
    push_static_axis<axes22::T18_simd_extension>(lv,     "simd_extension");
    push_static_axis<axes22::T19_general_hardware>(lv,   "general_hardware");
    push_static_axis<axes22::T20_queuing_q1>(lv,         "queuing_q1");
    push_static_axis<axes22::T21_queuing_q2>(lv,         "queuing_q2");
    return lv;
}

/// Compile-time-Produkt der 22 Enabled-Größen = die volle Permutations-Kardinalität (ohne mp_product-Materialisierung).
[[nodiscard]] inline constexpr std::size_t all_axes_binary_count() {
    return enabled_count<axes22::T00_search_algo>      * enabled_count<axes22::T01_cache_traversal>
         * enabled_count<axes22::T02_mapping>          * enabled_count<axes22::T03_path_compression>
         * enabled_count<axes22::T04_node_type>        * enabled_count<axes22::T05_memory_layout>
         * enabled_count<axes22::T06_allocator>        * enabled_count<axes22::T07_prefetch>
         * enabled_count<axes22::T08_concurrency>      * enabled_count<axes22::T09_serialization>
         * enabled_count<axes22::T10_telemetry>        * enabled_count<axes22::T11_value_handle>
         * enabled_count<axes22::T12_isa>              * enabled_count<axes22::T13_index_organization>
         * enabled_count<axes22::T14_io_dispatch>      * enabled_count<axes22::T15_migration_policy>
         * enabled_count<axes22::T16_filter>           * enabled_count<axes22::T17_page_type>
         * enabled_count<axes22::T18_simd_extension>   * enabled_count<axes22::T19_general_hardware>
         * enabled_count<axes22::T20_queuing_q1>       * enabled_count<axes22::T21_queuing_q2>;
}

}  // namespace comdare::cache_engine::builder::experiment
