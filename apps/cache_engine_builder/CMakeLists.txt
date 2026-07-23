# REV 7.7 V23.B — apps/cache_engine_builder/
# Top-Level Executable des CacheEngineBuilder. Library-Code (Phasen 1-7,
# Codegen, ModuleLoader, etc.) bleibt unter cache_engine/builder/* als
# Bibliotheken. Hier nur der main.cpp-Wrapper, damit "apps/" die
# eindeutige Einstiegspunkt-Zone fuer ausfuehrbare Programme ist
# (Pitchfork-/ClickHouse-Konvention, siehe docs/sessions/20260514-3000-v23-layout-vorschlag.md).

if(COMDARE_BUILD_BUILDER)
  add_executable(cache_engine_builder
    main.cpp
  )
  target_include_directories(cache_engine_builder PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src" # A5: ceb_version_stamp.hpp -> anatomy_fingerprint.hpp -> sha512/ctsha512.hpp
  )
  target_link_libraries(cache_engine_builder PRIVATE
    comdare_builder_xml_config_parser
    comdare_builder_codegen
    comdare_builder_permutation_loop
    comdare_builder_experiment_runner
    comdare_builder_experiment_driver
    comdare_module_loader
    comdare::workload_generator
    comdare::experiment
  )
  target_compile_features(cache_engine_builder PRIVATE cxx_std_23)
  set_target_properties(cache_engine_builder PROPERTIES
    OUTPUT_NAME "comdare-cache-engine-builder"
  )
endif()
