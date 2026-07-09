# profile_facade — E4-XML-Roadmap Phase 1 / #230 (Dossier 17): die PRODUKTIVE Fassade um die umbrella-schwere
# CEB-Eintritts-API tlz::run_profile (profile_run_entry.hpp). GoF-Pattern FACADE — GENAU EINE .cpp zieht den
# all_axes_umbrella (alle 19 Achsen compile-time) + baut den realen g++-16-@rsp-Compiler-Aufruf; der Konsument
# (super messung_driver / main.cpp) sieht NUR den schlanken POD-Header profile_run_facade.hpp.
#
# ADDITIV & golden/ABI-NEUTRAL. EXCLUDE_FROM_ALL: baut NUR, wenn ein Konsument die Lib linkt (bzw. das Target
# explizit gebaut wird) — kein Zusatz-Compile beim reinen `ninja` der ce-Bibliothek, denn der Umbrella-Compile
# ist schwer. Der Include-/Define-Satz spiegelt m3v2_pmc_smoke/test_harness_compile (die bewiesen umbrella-
# kompilierenden Test-Targets, tests/unit/CMakeLists.txt:2125-2161).

# ── Generierte Achsen-Flag-Header-Dirs. ROBUST gegen Configure-Ordering: aus dem SOURCE-Tree abgeleitet →
#    Binary-generated-Pfad (die Dirs entstehen per configure_file am ce-Top-Level, exakt wie
#    tests/unit/CMakeLists.txt:11-30). PROJECT_BINARY_DIR (= ce-Top-Binary-Dir via project(comdare-cache-engine))
#    statt CMAKE_BINARY_DIR, damit die Pfade im Standalone-ce-Build UND im super-Sub-Build (ce = add_subdirectory,
#    generated unter _cache_engine_external/) korrekt sind. ──
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

# ── Der schwere Include-Satz der umbrella-TU. Enthaelt zusaetzlich tests/unit/thesis_tiere: die run_profile-Naht-
#    Header (profile_run_entry/source_catalog/sota_catalog/profile_runner, S7c) liegen in Phase 1 NOCH im Harness;
#    eine spaetere Roadmap-Phase hebt sie in eine reine Produktiv-Lage. Reiner Include-Pfad (KEIN Test-Bau). ──
set(_facade_heavy_includes
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization"
    "${PROJECT_SOURCE_DIR}/tests/unit/thesis_tiere"
    "${PROJECT_BINARY_DIR}/generated"
    ${_facade_axis_generated_dirs})

add_library(comdare_profile_run_facade STATIC EXCLUDE_FROM_ALL profile_run_facade.cpp)
add_library(comdare::profile_run_facade ALIAS comdare_profile_run_facade)

# ── Codegen-Kante (CMakeLists.txt:1410-1462 / is_original_codegen.cmake): die Fassade zieht via
#    all_axes_umbrella die is_original-Mixin-Header (paper_<id>_is_original.hpp), die per Build-Zeit-Codegen
#    (comdare_paper_<id>_codegen, ALL) entstehen. Die Fassade ist damit ein neuer CODEGEN-CONSUMER. Sie am
#    dafuer vorgesehenen GLOBAL-Property registrieren → das ce-Top-Level haengt comdare_paper_codegen_all als
#    Dependency an, NACHDEM alle Paper-Wrapper registriert sind (order-sicher: das Property wird erst dort
#    gelesen). So generiert ein isolierter `--target comdare_profile_run_facade`-Build die Mixin-Header ZUERST
#    (sonst C1083/„No such file" auf paper_p01_art_is_original.hpp bei Cold-Build). ──
set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS comdare_profile_run_facade)

target_compile_features(comdare_profile_run_facade PUBLIC cxx_std_23)
# PUBLIC: der Konsument (super messung_driver) inkludiert <builder/profile_facade/profile_run_facade.hpp> ueber
#         den libs/cache_engine-Include-Root (ce-Konvention). Reiner Suchpfad — der schlanke Header zieht NICHTS
#         Umbrella-schweres nach. PRIVATE: der umbrella-schwere Include-Satz nur fuer die .cpp.
target_include_directories(comdare_profile_run_facade
    PUBLIC  "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    PRIVATE ${_facade_heavy_includes})
# Echte (NICHT-inline) Link-Abhaengigkeiten der run_profile-Laufzeit-Kette:
#   • comdare_builder_xml_config_parser : parse_thesis_profile (XmlConfigParser::, .cpp-Methode; wie test_profile_coverage)
#   • comdare::workload_driver          : WorkloadGenerator (run_workload_perm ruft ctor/generate_all — .cpp-Symbole)
#   • comdare::anatomy_module_loader    : AnatomyModuleLoader::load / AnatomyModuleHandle::unload (Lade-DLL-Grenze)
#   • Boost::mp11                       : der Umbrella-Compile
# Alle PRIVATE — STATIC-Lib propagiert sie als LINK_ONLY-Usage-Requirement an den Konsumenten (messung_driver),
# ohne die schlanke Fassaden-API (Header) mit Umbrella-/Loader-Typen zu belasten.
target_link_libraries(comdare_profile_run_facade PRIVATE
    Boost::mp11
    comdare_builder_xml_config_parser
    comdare::workload_driver
    comdare::anatomy_module_loader)

# Mess-/Plattform-Defines wie die umbrella-kompilierenden Test-Targets (sonst kein IObservableTier/Observer).
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_profile_run_facade)
endif()
target_compile_definitions(comdare_profile_run_facade PRIVATE
    COMDARE_MEASUREMENT_ON=1 COMDARE_CE_ENABLE_STATISTICS=1 COMDARE_EXPERIMENT_MODE_ON=1)

# ── Gebackener Default-Include-Satz fuer den perm_<id>.cpp-Sub-Compile (g++-16 @rsp), '|'-getrennt (kein ';',
#    das CMake als Listentrenner im Define-Wert missdeutet). So ist die Fassade OHNE externes COMDARE_PILOT_INCLUDES
#    selbst-genuegsam. Die vendored Boost.MP11 mitgeben (der emittierte perm.cpp zieht all_axes_umbrella → boost/mp11
#    — konkreter Pfad, KEINE Generator-Expression, sicher im Define-String). ──
set(_facade_perm_inc ${_facade_heavy_includes})
set(_facade_mp11_inc "${PROJECT_SOURCE_DIR}/cmake/third_party/boost_mp11/include")
if(EXISTS "${_facade_mp11_inc}/boost/mp11.hpp")
    list(APPEND _facade_perm_inc "${_facade_mp11_inc}")
endif()
list(JOIN _facade_perm_inc "|" _facade_perm_inc_joined)
target_compile_definitions(comdare_profile_run_facade PRIVATE
    "COMDARE_FACADE_PERM_INCLUDES=\"${_facade_perm_inc_joined}\"")
