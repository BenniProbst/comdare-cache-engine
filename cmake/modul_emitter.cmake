# STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, Weiche A; Board #147, 2026-08-27) -- modul_emitter Module:
# die CMAKE-ANBINDUNG der L4-Andock-Flaeche (H-23 L4 = D1: "Ausgang = kompilierbare, GESTEMPELTE Quelldatei
# + CMake-Anbindung"). Baut die 13 C2-Vertragspaar-Fixtures, die comdare-modul-emitter (apps/modul_emitter)
# emittiert, als SHARED-Module -- der dlopen-Weg von test_stempel2_vertragspaare laedt sie.
#
# DEKLARIERTE ABWEICHUNG VOM CONFIGURE-ZEIT-2-PASS (adhoc_emitter.cmake / anatomy_codegen_runner.cmake):
# hier laeuft das Werkzeug als BUILD-ZEIT-Custom-Command. Der 2-Pass jener Module existiert, weil die
# ZAHL ihrer Erzeugnisse erst der Werkzeuglauf kennt (Permutations-Raum -> file(GLOB) -> N Targets); die
# Vertragspaar-Menge ist GESCHLOSSEN und deterministisch benannt (13 Dateien, Liste unten == Liste im
# Werkzeug, Drift-Wache = manifest.txt im Test). Ein Configure-Zeit-Lauf haette die J-1-/J-0b-Falle
# (Memory lokale-vollbau-luecken): ohne vorher gebautes Werkzeug verschwaenden die Fixtures und ihr Test
# LAUTLOS aus der Testmenge (SKIPPED-Block), und die K17-Rezeptur ("J-1 SIEBEN Werkzeuge -> RE-CONFIGURE")
# bekaeme ein achtes Werkzeug. Die Custom-Command-Form haengt die Erzeugnisse an den Build-Graphen: aendert
# sich das Werkzeug (und damit modul_emitter.hpp/hybrid_modul_emitter.hpp), werden die 13 Quellen neu
# emittiert und die Module neu gebaut -- ohne Configure, ohne Sonderschritt, in jeder Zelle gleich.
#
# @doku ~/backups-workflow/20260826-stempel-teil2/STAND.md (E-6) + H-23 TEIL B L3/L4

# ----------------------------------------------------------------------------------------------------
# comdare_stempel2_vertragspaar_fixtures -- emittiert (Build-Zeit) und baut die 13 Vertragspaar-Module.
#   OUTPUT_DIR    <dir>   Wurzel der emittierten Quellen (<dir>/<fixture>/<Loader-Pattern>.cpp + manifest.txt)
#   AXIS_GEN_DIRS <dirs>  generierte axis_*_flags.hpp-Include-Dirs (COMDARE_ALL_AXIS_GENERATED_DIRS)
#   [TARGETS_OUT  <var>]  Liste der erzeugten SHARED-Targets (Reihenfolge == Fixture-Liste)
# ----------------------------------------------------------------------------------------------------
function(comdare_stempel2_vertragspaar_fixtures)
    set(_options)
    set(_one_value OUTPUT_DIR TARGETS_OUT)
    set(_multi_value AXIS_GEN_DIRS)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "comdare_stempel2_vertragspaar_fixtures: OUTPUT_DIR required.")
    endif()
    if(NOT TARGET comdare_modul_emitter_cli)
        message(FATAL_ERROR
            "comdare_stempel2_vertragspaar_fixtures: Target comdare_modul_emitter_cli fehlt -- apps/ muss VOR "
            "tests/ verarbeitet sein (apps/modul_emitter).")
    endif()

    # DIE GESCHLOSSENE FIXTURE-LISTE: Target|Unterverzeichnis/Datei. Die Datei-Namen sind das Loader-Pattern
    # des Emitters (<datei_praefix><idx>.cpp, modul_emitter.hpp kXxxModulStrategie.datei_praefix), das
    # Unterverzeichnis der Fixture-Name des Werkzeugs (apps/modul_emitter/main.cpp). Beide Listen muessen
    # deckungsgleich sein -- test_stempel2_vertragspaare prueft das gegen manifest.txt (Drift-Wache).
    set(_fixtures
        "stempel2_sa_gestempelt|sa_gestempelt/comdare_anatomy_perm_auto_0.cpp"
        "stempel2_sa_stempellos|sa_stempellos/comdare_anatomy_perm_auto_0.cpp"
        "stempel2_set_gestempelt|set_gestempelt/comdare_anatomy_perm_auto_set_0.cpp"
        "stempel2_set_stempellos|set_stempellos/comdare_anatomy_perm_auto_set_0.cpp"
        "stempel2_sequence_gestempelt|sequence_gestempelt/comdare_anatomy_perm_auto_sequence_0.cpp"
        "stempel2_sequence_stempellos|sequence_stempellos/comdare_anatomy_perm_auto_sequence_0.cpp"
        "stempel2_view_gestempelt|view_gestempelt/comdare_anatomy_perm_auto_view_0.cpp"
        "stempel2_view_stempellos|view_stempellos/comdare_anatomy_perm_auto_view_0.cpp"
        "stempel2_adapter_gestempelt|adapter_gestempelt/comdare_anatomy_perm_auto_adapter_0.cpp"
        "stempel2_adapter_stempellos|adapter_stempellos/comdare_anatomy_perm_auto_adapter_0.cpp"
        "stempel2_hybrid_sa_gestempelt|hybrid_sa_gestempelt/comdare_anatomy_perm_auto_hybrid_0.cpp"
        "stempel2_hybrid_set_gestempelt|hybrid_set_gestempelt/comdare_anatomy_perm_auto_hybrid_0.cpp"
        "stempel2_hybrid_stempellos|hybrid_stempellos/comdare_anatomy_perm_auto_hybrid_0.cpp")

    set(_cpps "")
    set(_targets "")
    foreach(_f ${_fixtures})
        string(REPLACE "|" ";" _parts "${_f}")
        list(GET _parts 0 _t)
        list(GET _parts 1 _rel)
        list(APPEND _cpps "${ARG_OUTPUT_DIR}/${_rel}")
        list(APPEND _targets "${_t}")
    endforeach()
    list(LENGTH _targets _n)
    if(NOT _n EQUAL 13)
        message(FATAL_ERROR "comdare_stempel2_vertragspaar_fixtures: 13 Fixtures erwartet, Liste traegt ${_n}.")
    endif()

    # EIN Werkzeuglauf schreibt alle 13 Quellen + manifest.txt (Multi-Output-Regel; Ninja/Make fuehren
    # das Command genau einmal). DEPENDS auf das Werkzeug-Target = jede Header-Aenderung der Andock-
    # Flaeche emittiert neu.
    add_custom_command(
        OUTPUT ${_cpps} "${ARG_OUTPUT_DIR}/manifest.txt"
        COMMAND "$<TARGET_FILE:comdare_modul_emitter_cli>" "${ARG_OUTPUT_DIR}"
        DEPENDS comdare_modul_emitter_cli
        COMMENT "comdare modul_emitter (Stempel Teil 2, Weiche A): 13 C2-Vertragspaar-Fixtures emittieren"
        VERBATIM)

    set(_ce_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    set(_lib_dir "${ARG_OUTPUT_DIR}/lib")
    set(_i 0)
    foreach(_t ${_targets})
        list(GET _cpps ${_i} _cpp)
        add_library(${_t} SHARED "${_cpp}")
        # Loader-Pattern comdare_anatomy_perm_* (dokumentiert in anatomy_codegen.cmake), PREFIX "" wie r5g.
        set_target_properties(${_t} PROPERTIES PREFIX "" OUTPUT_NAME "comdare_anatomy_perm_${_t}")
        set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS ${_t})
        # Dieselbe Kante wie comdare_build_adhoc_modules: Kompositions-Header koennen generierte
        # Paper-Original-Code-Wrapper ziehen.
        foreach(_pc
                comdare_paper_a04_mimalloc_codegen  comdare_paper_a05_jemalloc_codegen
                comdare_paper_a07_snmalloc_codegen  comdare_paper_a20_dlmalloc_codegen
                comdare_paper_a10_rpmalloc_codegen  comdare_paper_a11_lrmalloc_codegen
                comdare_paper_p01_art_codegen       comdare_paper_p02_hot_codegen
                comdare_paper_p05_start_codegen     comdare_paper_p07_wormhole_codegen
                comdare_paper_p10_surf_codegen      comdare_paper_p03_masstree_codegen
                comdare_paper_q01_concurrentqueue_codegen)
            if(TARGET ${_pc})
                add_dependencies(${_t} ${_pc})
            endif()
        endforeach()
        target_include_directories(${_t} PRIVATE
            "${_ce_root}/libs/cache_engine"
            "${_ce_root}/libs/cache_engine/include"
            "${_ce_root}/libs/cache_engine/src"
            "${_ce_root}/libs/common"
            "${_ce_root}/apps"
            "${PROJECT_BINARY_DIR}/generated"
            ${ARG_AXIS_GEN_DIRS})
        target_link_libraries(${_t} PRIVATE Boost::mp11)
        target_compile_features(${_t} PRIVATE cxx_std_23)
        target_compile_definitions(${_t} PRIVATE COMDARE_ANATOMY_MODULE_BUILD=1)
        set_target_properties(${_t} PROPERTIES
            POSITION_INDEPENDENT_CODE                ON
            RUNTIME_OUTPUT_DIRECTORY                 "${_lib_dir}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE         "${_lib_dir}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG           "${_lib_dir}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_lib_dir}"
            LIBRARY_OUTPUT_DIRECTORY                 "${_lib_dir}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE         "${_lib_dir}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG           "${_lib_dir}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO  "${_lib_dir}")
        if(COMMAND comdare_set_platform_defines)
            comdare_set_platform_defines(${_t})
        endif()
        math(EXPR _i "${_i} + 1")
    endforeach()

    if(ARG_TARGETS_OUT)
        set("${ARG_TARGETS_OUT}" "${_targets}" PARENT_SCOPE)
    endif()
    message(STATUS
        "comdare_stempel2_vertragspaar_fixtures: 13 Vertragspaar-Module (Build-Zeit-Emission, output=${ARG_OUTPUT_DIR})")
endfunction()
