# INC-A (2026-07-14) - axis_registry_gen: eigenstaendiges C++-Executable, das die ce-Registry-XML
# cache_engine_axis_registry.xml per compile-time-Reflektion der Enabled*/StaticAxisVariants*-Listen
# erzeugt. NUR ein Build-Target; die XML-Generierung ist ein Laufzeit-Schritt (KEINE CMake-Logik).
# EXCLUDE_FROM_ALL: der Standard-Build baut das Tool nicht mit; es wird explizit per Target gebaut.

# Die generierten axis_*_flags.hpp liegen unter ${PROJECT_BINARY_DIR}/generated/{axes,topics}/<...>.
# Vollpfad-Includes (<axes/lookup/axis_03a_search_algo_flags.hpp>) loesen ueber ${PROJECT_BINARY_DIR}/generated;
# die per-Achse-Dirs werden analog tests/unit/CMakeLists.txt ergaenzt (robust gegen bare Includes).
file(GLOB _arg_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_ARG_GENERATED_DIRS "")
foreach(_axis_src_dir ${_arg_axis_src_dirs})
    if(IS_DIRECTORY "${_axis_src_dir}")
        file(RELATIVE_PATH _axis_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_axis_src_dir}")
        list(APPEND _ARG_GENERATED_DIRS "${PROJECT_BINARY_DIR}/generated/topics/${_axis_rel}")
    endif()
endforeach()
file(GLOB _arg_axes_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_axes_dir ${_arg_axes_dirs})
    if(IS_DIRECTORY "${_axes_dir}")
        get_filename_component(_axes_name "${_axes_dir}" NAME)
        list(APPEND _ARG_GENERATED_DIRS "${PROJECT_BINARY_DIR}/generated/axes/${_axes_name}")
    endif()
endforeach()

add_executable(comdare_axis_registry_gen EXCLUDE_FROM_ALL main.cpp)
target_compile_features(comdare_axis_registry_gen PRIVATE cxx_std_23)
target_include_directories(comdare_axis_registry_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated"
    ${_ARG_GENERATED_DIRS})
target_link_libraries(comdare_axis_registry_gen PRIVATE Boost::mp11)
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_axis_registry_gen)
endif()

# Paper-Original-Code-Mixins (COMDARE_A0{3A,6}/AQ1_IS_ORIGINAL_CODEGEN): die reflektierten Achsen-Registries
# ziehen bei aktivem Codegen die generierten legacy_code/paper_*_is_original.hpp - erzeugt vom ALL-Umbrella
# comdare_paper_codegen_all. Als Consumer registrieren, damit die Root-CMakeLists (Z.1453+) die
# add_dependencies(<target> comdare_paper_codegen_all) setzt (offizieller Mechanismus, wie die adhoc-Targets).
set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS comdare_axis_registry_gen)
