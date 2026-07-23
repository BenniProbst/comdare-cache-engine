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

  # G3/#46b (I1c): Produktions-Verdrahtung des CEB-Executables gegen das io_uring-Writer-Backend.
  # Die Option COMDARE_WRITER_BACKEND ist in libs/cache_engine/builder/CMakeLists.txt definiert (VOR apps/
  # verarbeitet -> hier als globale Cache-Var sichtbar); ihr add_compile_definitions ist aber directory-scoped
  # auf builder/, daher setzt die App das CT-Define + den Vendor-Link hier SELBST. Bei io_uring: das vendored
  # liburing (ext/io/liburing, NOT-TARGET-Guard fuer den on-demand-Einzug) + COMDARE_WRITER_BACKEND_IO_URING ->
  # spool_writer.hpp waehlt io_uring. Default portable => KEINE Zusatz-Dep (byte-neutral). winring = Windows-Define.
  if(COMDARE_WRITER_BACKEND STREQUAL "io_uring")
    if(NOT TARGET comdare_vendored_liburing)
      add_subdirectory("${PROJECT_SOURCE_DIR}/ext/io/liburing" "${CMAKE_BINARY_DIR}/_ext_io_liburing")
    endif()
    target_compile_definitions(cache_engine_builder PRIVATE COMDARE_WRITER_BACKEND_IO_URING)
    target_link_libraries(cache_engine_builder PRIVATE comdare::vendored_liburing)
  elseif(COMDARE_WRITER_BACKEND STREQUAL "winring")
    target_compile_definitions(cache_engine_builder PRIVATE COMDARE_WRITER_BACKEND_WINRING)
  endif()
endif()
