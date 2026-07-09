# V41.F.6.1.R6.A — workload_driver Library-Target

comdare_add_library(comdare_workload_driver PROJECT CACHE_ENGINE SOURCES
    workload_generator.cpp)

add_library(comdare::workload_driver ALIAS comdare_workload_driver)

# PROJECT_SOURCE_DIR (ce-Wurzel) statt CMAKE_SOURCE_DIR: als add_subdirectory-Modul (super-Sub-Build) ist
# CMAKE_SOURCE_DIR die super-Wurzel → falscher -I-Pfad. Standalone-ce identisch = No-Op. (#230 super-Andockung)
target_include_directories(comdare_workload_driver PUBLIC
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

target_compile_features(comdare_workload_driver PUBLIC cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_workload_driver)
endif()
