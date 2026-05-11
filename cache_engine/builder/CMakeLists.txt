# cache_engine/builder — CacheEngineBuilder = main.cpp Orchestrator
# Phase 6 (REV 6, 2026-05-12): main.cpp noch nicht implementiert (siehe Termin 7 §5 Lifecycle).
# Skelett-Build wird nur aktiviert, wenn main.cpp existiert.

if(COMDARE_BUILD_BUILDER AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
  add_executable(cache_engine_builder
    main.cpp
  )
  target_link_libraries(cache_engine_builder PRIVATE
    comdare_search_engine
    comdare_measurement
    comdare_hardware_isa
  )
  set_target_properties(cache_engine_builder PROPERTIES
    OUTPUT_NAME "comdare-cache-engine-builder"
  )
endif()

# Komponenten-Subdirectories
add_subdirectory(cache_engine_component)
add_subdirectory(decision_lambda_trees)
add_subdirectory(measurement_matrix)
add_subdirectory(permutation_engine)
add_subdirectory(module_loader)
add_subdirectory(platform_probe)
add_subdirectory(live_cpu_model)
add_subdirectory(algorithm_visitor)
add_subdirectory(observer_registry)
add_subdirectory(compile_time_knowledge)
add_subdirectory(runtime_micro_benchmarks)
add_subdirectory(telemetry_spool)
add_subdirectory(in_memory_measurement_buffer)
add_subdirectory(disk_serializer)
add_subdirectory(latex_renderer)
