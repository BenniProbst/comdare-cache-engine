# L-CLUSTER (gate-frei) — apps/perm_runner: der lokale Cluster-Mess-Runner je Binary. Lädt EINE perm-DLL via
# AnatomyModuleLoader + emittiert EINE result_ingest-Zeile nach stdout. Boost-FREI + ohne Anatomie-Umbrella
# (lädt nur die DLL, baut keine Komposition) → schlanke Include-Roots + comdare::anatomy_module_loader.

add_executable(comdare_perm_runner main.cpp)
set_target_properties(comdare_perm_runner PROPERTIES OUTPUT_NAME "perm_runner")

target_include_directories(comdare_perm_runner PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    # #271/DEG-5: perm_runner.hpp -> load_profile_parser.hpp -> <serialization/xml_config_parser/xml_reader.hpp>
    # liegt unter libs/common -- f15 bekam es transitiv ueber workload_driver, der schlanke Runner nicht.
    "${PROJECT_SOURCE_DIR}/libs/common")

target_link_libraries(comdare_perm_runner PRIVATE comdare::anatomy_module_loader)
target_compile_features(comdare_perm_runner PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_perm_runner)
endif()
