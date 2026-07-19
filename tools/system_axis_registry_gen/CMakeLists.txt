# PAKET W2-B (2026-07-19) - system_axis_registry_gen: eigenstaendiges C++-Executable, das die CEB-System-
# Achsen-Registry-XML system_axis_registry.xml per compile-time-Reflektion der realen CebSystemAxis-Typen
# erzeugt (Ledger §28/§30, System-Art-Registry im Mess-/System-Modul). NUR ein Build-Target; die XML-
# Generierung ist ein Laufzeit-Schritt (KEINE CMake-Logik). EXCLUDE_FROM_ALL: der Standard-Build baut das
# Tool nicht mit; es wird explizit per Target (bzw. ueber die ctest-COMDARE_TEST_TARGETS-Schleife) gebaut.
#
# Keine Paper-Codegen-Mixins und keine axis_*_flags-Glob-Dirs noetig: dieses Tool reflektiert NUR die
# system_config-Achsen (measurement/*_system_axis.hpp/*_sub_axis.hpp) und zieht KEINE Organ-Topic-Registries.

add_executable(comdare_system_axis_registry_gen EXCLUDE_FROM_ALL main.cpp)
target_compile_features(comdare_system_axis_registry_gen PRIVATE cxx_std_23)
target_include_directories(comdare_system_axis_registry_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_system_axis_registry_gen)
endif()
