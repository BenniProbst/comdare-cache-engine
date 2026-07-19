#pragma once
// BR-1 (2026-06-02, Doc 27 §3) — registry_to_axis_levels: reflektiert ALLE 26 realen Achsen aus den Registry-
// Enabled-Listen (mp_lists) in die Experiment-Baum-AxisLevels — REGISTRY-getrieben, NICHT string-getrieben.
// Ersetzt die externe XML-String-AxisRegistry (profile_to_tree) als Quelle der Wahrheit.
//
// User-Korrektur 2026-06-02: KEINE 17-Achsen-Bindung, sondern ALLE 26 (15 Topics). 17 davon sind AdHocComposition-
// Slots T0..T16; die übrigen 5 Registry-Achsen = 3 build-only-Achsen (page_type/01, simd_extension/09b,
// general_hardware/12; DefinitionOnly) + die 2 queuing-KOMPOSITION-Achsen q1/q2 (gehören zur 17-Slot-Komposition
// als T15/T16, NICHT „andere Gattung" — nur spät in der Registry-Reihenfolge), die der Baum EBENFALLS bindet.
// Anker (namespace-sicher): TopicConfigSet::StaticAxisVariants_* (+ axis_01_page_type::EnabledPageTypes).
//
// ⚠️ OOM-HINWEIS: dieser Header inkludiert ALLE Achsen-Registries → in EINEM TU compiler-heap-schwer (Windows-OOM
// beobachtet). Der Reflektions-MECHANISMUS wird daher LEICHT gegen eine Teilmenge verifiziert (axis_reflect.hpp +
// wenige ConfigSets, test_br1_subset); die Voll-26-Bindung baut der projekteigene CMake-Build RAM-verwaltet (KF-16b).
// C++23, header-only.

#include "axis_reflect.hpp" // generische Reflektions-Helfer (reflect_names/enabled_count/push_static_axis)

// Die Topic-Config-Sets, die die Achsen tragen (jedes inkludiert seine Achsen-Registries).
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_registry.hpp> // page_type (nicht im nodes-ConfigSet)
#include <topics/nodes/axis_btree_order/axis_btree_order_registry.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_registry.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_registry.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_registry.hpp>
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

// ── Die 26 realen Achsen-Enabled-Listen, namespace-sicher via TopicConfigSet (+ page_type-Registry) ──
namespace axes26 {
namespace ce = ::comdare::cache_engine;
// 16 Kern-Komposition-Achsen (die AdHocComposition hat 18 = diese 16 + q1/q2; INC-2c: telemetry ist
// System-Achse — der axes26::T10_telemetry-Alias bleibt als Registry-Reflexions-Quelle der System-Seite):
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
using T17_page_type        = ce::nodes::axis_01_page_type::EnabledPageTypes;
using T18_simd_extension   = ce::hardware::TopicConfigSet::StaticAxisVariants_09b;
using T19_general_hardware = ce::hardware::TopicConfigSet::StaticAxisVariants_12;
using T20_queuing_q1       = ce::queuing::TopicConfigSet::StaticAxisVariants_Q1;
using T21_queuing_q2       = ce::queuing::TopicConfigSet::StaticAxisVariants_Q2;
using T22_btree_order      = ce::nodes::axis_btree_order::EnabledShapes;
using T23_skip_list_shape  = ce::nodes::axis_skip_list_shape::EnabledShapes;
using T24_bst_shape        = ce::nodes::axis_bst_shape::EnabledShapes;
using T25_hash_probe_shape = ce::nodes::axis_hash_probe_shape::EnabledShapes;
} // namespace axes26

/// Bau-INC-1b (Schichtung, System ⊃ Tier): die 26 Achsen zerfallen in drei benannte Schicht-Bausteine.
/// Die Organ-/Kompositions-Seite permutiert die binary_id; die System-Schicht (build-only) ist
/// binary_id-orthogonal und wird kuenftig von der CEB-System-Achsen-Schicht getrieben (Q2-Ruling:
/// simd_extension = Auspraegungsquelle der Erweiterungshardware-System-Achse, von der CEB zur
/// Laufzeit selbst permutiert). build_all_axis_levels() bleibt byte-identisch (Reihenfolge + Labels).

/// Die 15 Kern-Kompositions-Achsen (Organ-Seite, permutieren die binary_id).
/// Bau-INC-2c: telemetry / Bau-INC-2d: isa sind KEINE Organ-Slots mehr — sie stehen in build_system_axis_levels().
inline void append_organ_core_axis_levels(std::vector<AxisLevel>& lv) {
    push_static_axis<axes26::T00_search_algo>(lv, "search_algo");
    push_static_axis<axes26::T01_cache_traversal>(lv, "cache_traversal");
    push_static_axis<axes26::T02_mapping>(lv, "mapping");
    push_static_axis<axes26::T03_path_compression>(lv, "path_compression");
    push_static_axis<axes26::T04_node_type>(lv, "node_type");
    push_static_axis<axes26::T05_memory_layout>(lv, "memory_layout");
    push_static_axis<axes26::T06_allocator>(lv, "allocator");
    push_static_axis<axes26::T07_prefetch>(lv, "prefetch");
    push_static_axis<axes26::T08_concurrency>(lv, "concurrency");
    push_static_axis<axes26::T09_serialization>(lv, "serialization");
    push_static_axis<axes26::T11_value_handle>(lv, "value_handle");
    push_static_axis<axes26::T13_index_organization>(lv, "index_organization");
    push_static_axis<axes26::T14_io_dispatch>(lv, "io_dispatch");
    push_static_axis<axes26::T15_migration_policy>(lv, "migration_policy");
    push_static_axis<axes26::T16_filter>(lv, "filter");
}

/// Die System-Schicht-Keimzelle: die build-only-/System-Achsen (binary_id-orthogonal, stehen NICHT in
/// kCompositionAxisNames). Eigenstaendig abrufbar, damit die CEB-System-Achsen-Schicht sie getrennt
/// von der Organ-Permutation treiben kann (Erweiterungshardware/Compiler-Ordner statt Baum-Ebenen).
/// Bau-INC-2c (F12iii): telemetry gehoert HIERHER — TelemetryConfig (Active/Silent) ist CEB-System-Achsen-
/// Belegung, H-10-Sidecar NEBEN dem Binary (kein binary_id-Segment). Bau-INC-2d: isa gehoert EBENFALLS
/// hierher — die Ziel-ISA wird build-config-gewaehlt (Target-ISA-System-Achse, +target=-Sidecar); das
/// isa-ORGAN (T12_isa-Alias) bleibt als Registry-Reflexions-Quelle der System-Seite (kein binary_id-Segment).
[[nodiscard]] inline std::vector<AxisLevel> build_system_axis_levels() {
    std::vector<AxisLevel> lv;
    lv.reserve(5);
    push_static_axis<axes26::T17_page_type>(lv, "page_type");
    push_static_axis<axes26::T18_simd_extension>(lv, "simd_extension");
    push_static_axis<axes26::T19_general_hardware>(lv, "general_hardware");
    push_static_axis<axes26::T10_telemetry>(lv, "telemetry");
    push_static_axis<axes26::T12_isa>(lv, "isa");
    return lv;
}

/// Die 2 queuing-Kompositions-Achsen (q1/q2, T15/T16 der 17-Slot-Komposition) + die 4 Shape-Achsen.
inline void append_composition_tail_axis_levels(std::vector<AxisLevel>& lv) {
    push_static_axis<axes26::T20_queuing_q1>(lv, "queuing_q1");
    push_static_axis<axes26::T21_queuing_q2>(lv, "queuing_q2");
    push_static_axis<axes26::T22_btree_order>(lv, "btree_order");
    push_static_axis<axes26::T23_skip_list_shape>(lv, "skip_list_shape");
    push_static_axis<axes26::T24_bst_shape>(lv, "bst_shape");
    push_static_axis<axes26::T25_hash_probe_shape>(lv, "hash_probe_shape");
}

/// Baut ALLE 26 Achsen als statische AxisLevels aus den REALEN Enabled-Listen (Fanout = volles Enabled-Inventar,
/// block_id-getaggt → Knoten-Rück-Referenz). Reihenfolge UNVERAENDERT: 17 Kern-Achsen (T0..T16), dann die
/// 3 build-only-System-Achsen (page_type/simd/hw), dann q1/q2 + die 4 Shape-Achsen.
[[nodiscard]] inline std::vector<AxisLevel> build_all_axis_levels() {
    std::vector<AxisLevel> lv;
    lv.reserve(26);
    append_organ_core_axis_levels(lv);
    for (AxisLevel& system_level : build_system_axis_levels()) lv.push_back(std::move(system_level));
    append_composition_tail_axis_levels(lv);
    return lv;
}

/// Compile-time-Produkt der 26 Enabled-Größen = die volle Permutations-Kardinalität (ohne mp_product-Materialisierung).
/// V7/P4 (K-7, 2026-07-19): umbenannt von all_axes_binary_count() -- der alte Name suggerierte ein
/// binary_id-Produkt, aber System-/Build-/Shape-Achsen permutieren die binary_id NICHT (nur die 17
/// Organ-Slots tun das). Das 26-Achsen-Produkt ist die MATRIX-Kardinalitaet des Experiment-Baums.
[[nodiscard]] inline constexpr std::size_t all_axes_matrix_count() {
    return enabled_count<axes26::T00_search_algo> * enabled_count<axes26::T01_cache_traversal> *
           enabled_count<axes26::T02_mapping> * enabled_count<axes26::T03_path_compression> *
           enabled_count<axes26::T04_node_type> * enabled_count<axes26::T05_memory_layout> *
           enabled_count<axes26::T06_allocator> * enabled_count<axes26::T07_prefetch> *
           enabled_count<axes26::T08_concurrency> * enabled_count<axes26::T09_serialization> *
           enabled_count<axes26::T10_telemetry> * enabled_count<axes26::T11_value_handle> *
           enabled_count<axes26::T12_isa> * enabled_count<axes26::T13_index_organization> *
           enabled_count<axes26::T14_io_dispatch> * enabled_count<axes26::T15_migration_policy> *
           enabled_count<axes26::T16_filter> * enabled_count<axes26::T17_page_type> *
           enabled_count<axes26::T18_simd_extension> * enabled_count<axes26::T19_general_hardware> *
           enabled_count<axes26::T20_queuing_q1> * enabled_count<axes26::T21_queuing_q2> *
           enabled_count<axes26::T22_btree_order> * enabled_count<axes26::T23_skip_list_shape> *
           enabled_count<axes26::T24_bst_shape> * enabled_count<axes26::T25_hash_probe_shape>;
}

} // namespace comdare::cache_engine::builder::experiment
