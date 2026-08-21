# V41.F.6.1.R5.E — AnatomyModuleLoader Library-Target

comdare_add_library(comdare_anatomy_module_loader PROJECT CACHE_ENGINE SOURCES
    anatomy_module_loader.cpp)

add_library(comdare::anatomy_module_loader ALIAS comdare_anatomy_module_loader)

target_include_directories(comdare_anatomy_module_loader PUBLIC
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include")

target_compile_features(comdare_anatomy_module_loader PUBLIC cxx_std_23)

# POSIX: -ldl Pflicht fuer dlopen/dlsym/dlclose
if(NOT WIN32 AND NOT APPLE)
    target_link_libraries(comdare_anatomy_module_loader PUBLIC ${CMAKE_DL_LIBS})
endif()

# Plattform-Defines konsistent zu rest of project
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_anatomy_module_loader)
endif()

# Boost.MP11 transitiv via abi_v1.hpp -> abi_adapter.hpp -> pruefling_merge.hpp
if(TARGET Boost::mp11)
    target_link_libraries(comdare_anatomy_module_loader PUBLIC Boost::mp11)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_anatomy_module_loader)
