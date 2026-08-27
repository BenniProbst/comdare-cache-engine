# STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, Weiche A; Board #147, 2026-08-27) -- comdare-modul-emitter CLI.
#
# Das Werkzeug der L4-Andock-Flaeche (builder/codegen/modul_emitter.hpp + hybrid_modul_emitter.hpp) als
# Executable: emittiert die 13 C2-Vertragspaar-Fixtures (je Gattung gestempelt/stempellos, Hybrid 2+1).
# Gerufen als BUILD-ZEIT-Custom-Command von cmake/modul_emitter.cmake -- KEIN Configure-Zeit-2-Pass wie
# adhoc_emitter/anatomy_codegen_tool (deklarierte Abweichung, Begruendung in cmake/modul_emitter.cmake).
# Include-/Link-Vertrag: comdare::ce_hybrid traegt die Hybrid-Speisung (hybrid_config_xml -> libs/common
# xml_reader, Stempel-Kette -> src/ ctsha); der Umbrella braucht die generierten axis_*_flags.hpp.

add_executable(comdare_modul_emitter_cli main.cpp)
set_target_properties(comdare_modul_emitter_cli PROPERTIES
    OUTPUT_NAME "comdare-modul-emitter")

# Quell-abgeleitete generierte axis_*_flags.hpp-Dirs (identisch zu apps/adhoc_emitter, ordering-invariant:
# tests/unit definiert COMDARE_ALL_AXIS_GENERATED_DIRS erst NACH apps/).
file(GLOB _me_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_me_axis_gen_dirs "")
foreach(_me_d ${_me_axis_src_dirs})
    if(IS_DIRECTORY "${_me_d}")
        file(RELATIVE_PATH _me_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_me_d}")
        list(APPEND _me_axis_gen_dirs "${PROJECT_BINARY_DIR}/generated/topics/${_me_rel}")
    endif()
endforeach()
file(GLOB _me_axes_dirs LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/libs/cache_engine/organ_axes/*")
foreach(_me_d ${_me_axes_dirs})
    if(IS_DIRECTORY "${_me_d}")
        get_filename_component(_me_name "${_me_d}" NAME)
        list(APPEND _me_axis_gen_dirs "${PROJECT_BINARY_DIR}/generated/organ_axes/${_me_name}")
    endif()
endforeach()

target_include_directories(comdare_modul_emitter_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/common"
    # die Fixture-Huellen liegen NEBEN der CLI (apps/modul_emitter/stempel2_fixture_slots.hpp) und werden
    # von den emittierten Modul-TUs als <modul_emitter/stempel2_fixture_slots.hpp> gesehen.
    "${PROJECT_SOURCE_DIR}/apps"
    "${PROJECT_BINARY_DIR}/generated"
    ${_me_axis_gen_dirs})

target_link_libraries(comdare_modul_emitter_cli PRIVATE comdare::ce_hybrid Boost::mp11)
target_compile_features(comdare_modul_emitter_cli PRIVATE cxx_std_23)
# Der Umbrella zieht ueber die Kompositions-Header generierte Paper-Original-Code-Wrapper (wie die
# Permutations-DLLs des R5.G-Pfads) -> Consumer-Registry, damit die Codegen-Ziele VOR dieser TU laufen.
set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS comdare_modul_emitter_cli)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_modul_emitter_cli)
endif()

# B16-WARN-DECKUNG (Welle apps, Muster adhoc_emitter): -Wall-Familie fuer das Nicht-Test-Target.
COMDARE_set_default_warnings(comdare_modul_emitter_cli)
