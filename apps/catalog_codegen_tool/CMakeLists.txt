# Limits-Entkopplung Vorstufe: XML-Profil -> generierter Source-Katalog-Header.

add_executable(comdare_catalog_codegen_cli main.cpp)
set_target_properties(comdare_catalog_codegen_cli PROPERTIES
    OUTPUT_NAME "comdare-catalog-codegen")

file(GLOB _ccg_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_ccg_generated_root "${PROJECT_BINARY_DIR}/generated")
set(_ccg_axis_gen_dirs "")
foreach(_ccg_d ${_ccg_axis_src_dirs})
    if(IS_DIRECTORY "${_ccg_d}")
        file(RELATIVE_PATH _ccg_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_ccg_d}")
        list(APPEND _ccg_axis_gen_dirs "${_ccg_generated_root}/topics/${_ccg_rel}")
    endif()
endforeach()
file(GLOB _ccg_axes_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_ccg_d ${_ccg_axes_dirs})
    if(IS_DIRECTORY "${_ccg_d}")
        get_filename_component(_ccg_name "${_ccg_d}" NAME)
        list(APPEND _ccg_axis_gen_dirs "${_ccg_generated_root}/axes/${_ccg_name}")
    endif()
endforeach()

# Adapter-/Vendor-Includes: im ce-STANDALONE-Build nachweislich unbenutzt (Dep-Sets byte-identisch,
# Review wf_1009d16f), aber im super-Sub-Build TRAGEND: dort aktivieren HAVE_*-Sentinels (z.B.
# COMDARE_HAVE_MIMALLOC=ON -> COMDARE_AXIS_06_USE_MIMALLOC=1) echte Vendor-Pfade der Katalog-
# Include-Kette (source_catalog -> topic_allocator -> mimalloc_include.hpp -> mimalloc.h).
# Duplikat von _facade_adapter_ext_includes (profile_facade/CMakeLists.txt) — Zentralisierung
# in ein geteiltes cmake/-Modul = dokumentierte Folge-Hygiene.
set(_ccg_adapter_ext_includes
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
    "${PROJECT_SOURCE_DIR}/ext/traversal/P30-HazardPointers")

target_include_directories(comdare_catalog_codegen_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${_ccg_generated_root}"
    ${_ccg_axis_gen_dirs}
    ${_ccg_adapter_ext_includes})

target_link_libraries(comdare_catalog_codegen_cli PRIVATE
    Boost::mp11
    comdare_builder_xml_config_parser)
target_compile_features(comdare_catalog_codegen_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_catalog_codegen_cli)
endif()

if(TARGET comdare_paper_codegen_all)
    add_dependencies(comdare_catalog_codegen_cli comdare_paper_codegen_all)
endif()
