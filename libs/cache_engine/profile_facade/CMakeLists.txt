# profile_facade -- produktive Fassade um die umbrella-schwere run_profile-API.

set(_facade_axis_generated_dirs "")
file(GLOB _facade_axis_src_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
foreach(_d ${_facade_axis_src_dirs})
    if(IS_DIRECTORY "${_d}")
        file(RELATIVE_PATH _rel "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_d}")
        list(APPEND _facade_axis_generated_dirs "${PROJECT_BINARY_DIR}/generated/topics/${_rel}")
    endif()
endforeach()
file(GLOB _facade_axes_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_d ${_facade_axes_dirs})
    if(IS_DIRECTORY "${_d}")
        get_filename_component(_n "${_d}" NAME)
        list(APPEND _facade_axis_generated_dirs "${PROJECT_BINARY_DIR}/generated/axes/${_n}")
    endif()
endforeach()

set(_facade_adapter_ext_includes
    "${PROJECT_SOURCE_DIR}/ext/allocator/A01-hoard/src/include"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A03-michael-lockfree"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A04-mimalloc/include"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A05-jemalloc/include"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A06-tcmalloc/tcmalloc"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A07-snmalloc/src"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A08-scalloc/src"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A10-rpmalloc/rpmalloc"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A11-lrmalloc"
    "${PROJECT_SOURCE_DIR}/ext/allocator/A20-dlmalloc"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P01-ART/unodb"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P02-HOT/hot"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P03-Masstree/masstree-beta"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P04-CoCo-trie/CoCo-trie"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P05-START/START"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P06-B2tree/b2-tree-master/include"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P07-Wormhole/wormhole"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P10-SuRF/SuRF/include"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P20-BTreesAreBack/leanstore/backend"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P25-Mahling/prefetching/src"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P29-RCU/userspace-rcu/include"
    "${PROJECT_SOURCE_DIR}/ext/traversal/P30-HazardPointers/haz_ptr")

# Phase-1-Naht: profile_run_entry.hpp ist noch im Test-Harness. Nur dieser PRIVATE
# Include-Pfad zieht ihn; eine echte Hebung ist ein Folge-Increment.
set(_facade_heavy_includes
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere"
    "${PROJECT_BINARY_DIR}/generated"
    ${_facade_axis_generated_dirs}
    ${_facade_adapter_ext_includes})

set(_facade_adapter_targets
    comdare::adapter::p01_art
    comdare::adapter::p02_hot
    comdare::adapter::p03_masstree
    comdare::adapter::p04_cocotrie
    comdare::adapter::p05_start
    comdare::adapter::p06_b2tree
    comdare::adapter::p07_wormhole
    comdare::adapter::p10_surf
    comdare::adapter::p20_btreesareback
    comdare::adapter::p25_mahling
    comdare::adapter::p29_rcu
    comdare::adapter::p30_hazardpointers
    comdare::adapter::a01_hoard
    comdare::adapter::a03_michael_lockfree
    comdare::adapter::a04_mimalloc
    comdare::adapter::a05_jemalloc
    comdare::adapter::a06_tcmalloc
    comdare::adapter::a07_snmalloc
    comdare::adapter::a08_scalloc
    comdare::adapter::a10_rpmalloc
    comdare::adapter::a11_lrmalloc
    comdare::adapter::a20_dlmalloc)

add_library(comdare_profile_run_facade STATIC EXCLUDE_FROM_ALL profile_run_facade.cpp)
add_library(comdare::profile_run_facade ALIAS comdare_profile_run_facade)

set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS comdare_profile_run_facade)

target_compile_features(comdare_profile_run_facade PUBLIC cxx_std_23)
target_include_directories(comdare_profile_run_facade
    PUBLIC  "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    PRIVATE ${_facade_heavy_includes})

target_link_libraries(comdare_profile_run_facade PRIVATE
    Boost::mp11
    comdare::workload_driver
    comdare::anatomy_module_loader)

# E3-Haertung fuer den Super-Subbuild: diese Targets tragen historisch noch
# CMAKE_SOURCE_DIR-basierte Include-Roots. Additiv echte ce-Roots setzen, ohne
# die Producer-CMakeLists in diesem Paket anzufassen.
set(_facade_ce_roots
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${_facade_axis_generated_dirs})
foreach(_ce_target comdare_anatomy_module_loader comdare_workload_driver comdare_anatomy_codegen_tool)
    if(TARGET ${_ce_target})
        target_include_directories(${_ce_target} PUBLIC ${_facade_ce_roots})
    endif()
endforeach()
if(TARGET comdare_ce_pruef_dock)
    target_include_directories(comdare_ce_pruef_dock INTERFACE ${_facade_ce_roots})
endif()

# Die Adapter und der XML-Parser werden im ce-Top-CMake erst nach libs/cache_engine
# angelegt. Das Late-Linking laeuft am Ende des ce-Top-Verzeichnisses, wenn alle
# Targets existieren; so bleibt die Fassade in libs/cache_engine/profile_facade.
cmake_language(DEFER DIRECTORY "${PROJECT_SOURCE_DIR}" CALL target_link_libraries
    comdare_profile_run_facade PRIVATE comdare_builder_xml_config_parser ${_facade_adapter_targets})

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_profile_run_facade)
endif()
target_compile_definitions(comdare_profile_run_facade PRIVATE
    COMDARE_MEASUREMENT_ON=1
    COMDARE_CE_ENABLE_STATISTICS=1
    COMDARE_EXPERIMENT_MODE_ON=1)

set(_facade_perm_inc_entries ${_facade_heavy_includes})
list(APPEND _facade_perm_inc_entries "$<TARGET_PROPERTY:Boost::mp11,INTERFACE_INCLUDE_DIRECTORIES>")
foreach(_target ${_facade_adapter_targets})
    list(APPEND _facade_perm_inc_entries "$<TARGET_PROPERTY:${_target},INTERFACE_INCLUDE_DIRECTORIES>")
endforeach()
list(JOIN _facade_perm_inc_entries ";" _facade_perm_inc_expr)
target_compile_definitions(comdare_profile_run_facade PRIVATE
    "COMDARE_FACADE_PERM_INCLUDES=\"$<JOIN:$<REMOVE_DUPLICATES:${_facade_perm_inc_expr}>,|>\"")
