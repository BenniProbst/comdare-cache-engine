# W9.5/G10 VERMERK: Einmal-Scaffolding-Werkzeug (Stub-Erzeugung), NICHT Teil der Build-Pipeline.
# KEIN Python in der Buildchain (Talos OS hat kein Python, s. cmake/permutations.cmake); nur manueller Lauf.
"""Generiert CMakeLists.txt-Stubs + .gitkeep fuer alle Subverzeichnisse.

Lazy: header-only INTERFACE-Libraries als Stub, damit das Skelett buildet
(auch wenn keine Source vorhanden).
"""
from pathlib import Path

ROOT = Path(__file__).parent

# Verzeichnisse, die eine eigene CMakeLists.txt benoetigen (Sub-Komponenten)
# Format: relativer Pfad → INTERFACE-Library-Name (None = nur .gitkeep)
SUB_DIRS = {
    # search_engine sub-components (Concepts, header-only)
    "search_engine/fan_out_engine": "comdare_fan_out_engine",
    "search_engine/algorithm_strategies": "comdare_algorithm_strategies",
    "search_engine/page": "comdare_page_concept",
    "search_engine/node": "comdare_node_concept",
    "search_engine/value_handle": "comdare_value_handle",
    "search_engine/traversal": "comdare_traversal_concept",
    "search_engine/memory_layout": "comdare_memory_layout_concept",
    "search_engine/operations": "comdare_operations",

    # cache_engine/builder sub-components
    "cache_engine/builder/cache_engine_component": "comdare_cache_engine_component",
    "cache_engine/builder/decision_lambda_trees": "comdare_decision_lambda_trees",
    "cache_engine/builder/measurement_matrix": "comdare_measurement_matrix",
    "cache_engine/builder/permutation_engine": "comdare_permutation_engine",
    "cache_engine/builder/module_loader": "comdare_module_loader",
    "cache_engine/builder/platform_probe": "comdare_builder_platform_probe",
    "cache_engine/builder/live_cpu_model": "comdare_live_cpu_model",
    "cache_engine/builder/algorithm_visitor": "comdare_algorithm_visitor",
    "cache_engine/builder/observer_registry": "comdare_observer_registry",
    "cache_engine/builder/compile_time_knowledge": "comdare_compile_time_knowledge",
    "cache_engine/builder/runtime_micro_benchmarks": "comdare_runtime_micro_benchmarks",
    "cache_engine/builder/telemetry_spool": "comdare_telemetry_spool",
    "cache_engine/builder/in_memory_measurement_buffer": "comdare_in_memory_measurement_buffer",
    "cache_engine/builder/disk_serializer": "comdare_disk_serializer",
    "cache_engine/builder/latex_renderer": "comdare_latex_renderer",

    # cache_engine/concurrency_manager
    "cache_engine/concurrency_manager": "comdare_concurrency_manager",
    "cache_engine/concurrency_manager/page_concurrency": "comdare_concurrency_page",
    "cache_engine/concurrency_manager/node_concurrency": "comdare_concurrency_node",
    "cache_engine/concurrency_manager/array_concurrency": "comdare_concurrency_array",
    "cache_engine/concurrency_manager/data_structure_concurrency": "comdare_concurrency_data_structure",
    "cache_engine/concurrency_manager/path_concurrency": "comdare_concurrency_path",
    "cache_engine/concurrency_manager/memory_access_concurrency": "comdare_concurrency_memory_access",
    "cache_engine/concurrency_manager/simd_thread_concurrency": "comdare_concurrency_simd_thread",
    "cache_engine/concurrency_manager/simd_flow_concurrency": "comdare_concurrency_simd_flow",

    # cache_engine/subsystems
    "cache_engine/subsystems": "comdare_cache_engine_subsystems",
    "cache_engine/subsystems/platform_profiler": "comdare_platform_profiler",
    "cache_engine/subsystems/cost_model": "comdare_cost_model",
    "cache_engine/subsystems/page_type_scheduler": "comdare_page_type_scheduler",
    "cache_engine/subsystems/prefetch_controller": "comdare_prefetch_controller",
    "cache_engine/subsystems/relocation_manager": "comdare_relocation_manager",
    "cache_engine/subsystems/allocator_manager": "comdare_allocator_manager",
    "cache_engine/subsystems/telemetry_aggregator": "comdare_telemetry_aggregator",

    # cache_engine/reclamation
    "cache_engine/reclamation": "comdare_reclamation",
    "cache_engine/reclamation/rcu_reclaim": "comdare_rcu_reclaim",

    # measurement sub-components
    "measurement/perf_wrapper": "comdare_perf_wrapper",
    "measurement/papi_wrapper": "comdare_papi_wrapper",
    "measurement/advisor_wrapper": "comdare_advisor_wrapper",
    "measurement/hdr_histogram_wrapper": "comdare_hdr_histogram_wrapper",
    "measurement/dataset_loader": "comdare_dataset_loader",
    "measurement/platform_probe": "comdare_platform_probe",
    "measurement/run_recorder": "comdare_run_recorder",

    # hardware_isa sub-components
    "hardware_isa/isa_dispatch": "comdare_isa_dispatch",
    "hardware_isa/hybrid_core_pinning": "comdare_hybrid_core_pinning",
    "hardware_isa/memory_type_detector": "comdare_memory_type_detector",
    "hardware_isa/hugepage_manager": "comdare_hugepage_manager",

    # prt_art (eigener Algorithmus)
    "prt_art": "comdare_prt_art",
    "prt_art/pages": None,
    "prt_art/nodes": None,
    "prt_art/value_handles": None,
    "prt_art/traversal": None,
    "prt_art/memory_layout": None,
    "prt_art/allocator": None,
    "prt_art/prefetch": None,
    "prt_art/concurrency": None,
    "prt_art/legacy_reimpl": None,

    # adapters (Habich-Direktive)
    "adapters": None,

    # ext (Originalcode-Bausteine)
    "ext": None,
}

CMAKE_INTERFACE_TEMPLATE = """# {comp} — Sub-Komponente
# Skelett (Phase 4.B) — keine Implementation

add_library({lib} INTERFACE)
target_include_directories({lib} INTERFACE ${{CMAKE_CURRENT_SOURCE_DIR}})
"""

PASSTHROUGH_CMAKE = """# {comp} — Passthrough (keine Sub-Komponenten in Phase 4.B)
"""


def main():
    written_cmakelists = 0
    written_gitkeep = 0
    for relpath, lib in SUB_DIRS.items():
        target = ROOT / relpath
        target.mkdir(parents=True, exist_ok=True)
        cmake = target / "CMakeLists.txt"
        if lib is None:
            # Nur .gitkeep + leeres CMakeLists
            cmake.write_text(PASSTHROUGH_CMAKE.format(comp=relpath), encoding="utf-8")
            (target / ".gitkeep").write_text("", encoding="utf-8")
            written_gitkeep += 1
        else:
            cmake.write_text(
                CMAKE_INTERFACE_TEMPLATE.format(comp=relpath, lib=lib),
                encoding="utf-8",
            )
            (target / ".gitkeep").write_text("", encoding="utf-8")
            written_cmakelists += 1
    print(f"OK: {written_cmakelists} INTERFACE-Libraries + {written_gitkeep} Passthrough-CMakeLists")


if __name__ == "__main__":
    main()
