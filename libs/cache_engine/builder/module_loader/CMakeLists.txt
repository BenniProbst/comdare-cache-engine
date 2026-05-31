# cache_engine/builder/module_loader — Phase 7.2.D
# Plattform-agnostischer DLL/.so-Loader fuer Permutations-Module

comdare_add_library(comdare_module_loader PROJECT CACHE_ENGINE SOURCES
    module_loader.cpp
)
target_include_directories(comdare_module_loader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
)
target_compile_features(comdare_module_loader PUBLIC cxx_std_23)

# Plattform-spezifische Dependencies
if(UNIX AND NOT APPLE)
    target_link_libraries(comdare_module_loader PUBLIC dl)
endif()
