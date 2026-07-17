# Bau-INC-2c (2026-07-17) - gen_golden_fullpilot: autoritativer Golden-Fixture-Generator
# (BAUPLAN-INC2 §5: Neu-Materialisierung der golden binary_ids ueber den offiziellen
# catalog_static_levels<FullSourceCatalog>/StaticBinaryView-Pfad). EXCLUDE_FROM_ALL: der
# Standard-Build baut das Tool nicht mit; Regeneration NUR explizit im koordinierten ABI-Fenster.
# Include-/Link-Set = Spiegel von apps/catalog_codegen_tool (zieht dieselbe source_catalog-Kette).

file(GLOB _ggf_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_ggf_generated_root "${PROJECT_BINARY_DIR}/generated")
set(_ggf_axis_gen_dirs "")
foreach(_ggf_d ${_ggf_axis_src_dirs})
    if(IS_DIRECTORY "${_ggf_d}")
        file(RELATIVE_PATH _ggf_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_ggf_d}")
        list(APPEND _ggf_axis_gen_dirs "${_ggf_generated_root}/topics/${_ggf_rel}")
    endif()
endforeach()
file(GLOB _ggf_axes_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/axes/*")
foreach(_ggf_d ${_ggf_axes_dirs})
    if(IS_DIRECTORY "${_ggf_d}")
        get_filename_component(_ggf_name "${_ggf_d}" NAME)
        list(APPEND _ggf_axis_gen_dirs "${_ggf_generated_root}/axes/${_ggf_name}")
    endif()
endforeach()

# Adapter-/Vendor-Includes (Duplikat von catalog_codegen_tool — dieselbe Katalog-Include-Kette).
set(_ggf_adapter_ext_includes
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

add_executable(comdare_gen_golden_fullpilot EXCLUDE_FROM_ALL main.cpp)
target_compile_features(comdare_gen_golden_fullpilot PRIVATE cxx_std_23)
target_include_directories(comdare_gen_golden_fullpilot PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${_ggf_generated_root}"
    ${_ggf_axis_gen_dirs}
    ${_ggf_adapter_ext_includes})
target_link_libraries(comdare_gen_golden_fullpilot PRIVATE Boost::mp11)
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_gen_golden_fullpilot)
endif()
if(TARGET comdare_paper_codegen_all)
    add_dependencies(comdare_gen_golden_fullpilot comdare_paper_codegen_all)
endif()
set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS comdare_gen_golden_fullpilot)
