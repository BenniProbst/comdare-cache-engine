# V41.F.6.1.R5.E — AnatomyModuleLoader Library-Target

comdare_add_library(comdare_anatomy_module_loader PROJECT CACHE_ENGINE SOURCES
    anatomy_module_loader.cpp)

add_library(comdare::anatomy_module_loader ALIAS comdare_anatomy_module_loader)

# PROJECT_SOURCE_DIR (ce-Wurzel) statt CMAKE_SOURCE_DIR: als add_subdirectory-Modul (super-Sub-Build) ist
# CMAKE_SOURCE_DIR die super-Wurzel → falscher -I-Pfad. PROJECT_SOURCE_DIR ist in beiden Faellen die ce-Wurzel
# (project(comdare-cache-engine)); im Standalone-ce-Build identisch = No-Op. (#230 super-Andockung)
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
