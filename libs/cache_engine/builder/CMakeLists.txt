# cache_engine/builder — Builder-Bibliotheken (ohne main.cpp)
# REV 7.7 V23.B (2026-05-14): main.cpp wurde nach apps/cache_engine_builder/
# verschoben (Pitchfork-/ClickHouse-Konvention: apps/ fuer Executables,
# libs/ fuer Bibliotheken). Hier verbleibt nur der Library-Anteil.

# Phase 6.4 (REV 7, 2026-05-13): Builder-Subkomponenten
add_subdirectory(codegen)
add_subdirectory(permutation_loop)
add_subdirectory(experiment_runner)
add_subdirectory(experiment_driver)  # REV 7.6 (2026-05-13): Phase 1-7 als Library

# V32.JJ.1 (2026-05-18) - Command-Pattern Header-only Module (AA.2 Korrektur)
add_subdirectory(commands)

# Komponenten-Subdirectories
add_subdirectory(cache_engine_component)
add_subdirectory(decision_lambda_trees)
add_subdirectory(measurement_matrix)
add_subdirectory(permutation_engine)
add_subdirectory(module_loader)
add_subdirectory(anatomy_module_loader)  # V41.F.6.1.R5.E
add_subdirectory(anatomy_codegen_tool)   # V41.F.6.1.R5.F
add_subdirectory(workload_driver)        # V41.F.6.1.R6.A
add_subdirectory(pruef_dock)             # V41.F.6.1.R6 — per-Gattung ABI-Mess-Übergang (Doku 24 §8.8)
add_subdirectory(platform_probe)
add_subdirectory(live_cpu_model)
add_subdirectory(observer_registry)
add_subdirectory(compile_time_knowledge)
add_subdirectory(runtime_micro_benchmarks)
add_subdirectory(telemetry_spool)
add_subdirectory(in_memory_measurement_buffer)
add_subdirectory(disk_serializer)
add_subdirectory(latex_renderer)
