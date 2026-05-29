# V41.F.6.1 G.1 — Hierarchische Achsen-Iteration im Build-Output + Permutations-Raum-Auswertung
#
# Druckt zur Configure-Time eine HIERARCHISCHE Uebersicht der Anatomie (Topic -> Achse -> Wrapper-Zahl)
# statt der bisherigen 21 flachen per-Achse-message()-Zeilen, plus die "Auswertung": die Kardinalitaet
# des kartesischen Permutations-Raums (Produkt der Achsen-Wrapper-Zahlen). Quell-abgeleitet (GLOB der
# topics/<topic>/axis_*-Struktur), daher ordering-invariant + ohne Compile-Schritt.
#
# Wrapper-Zaehlung: top-level *.hpp einer Achse MINUS strukturelle Dateien (Concept/Base/Registry/
# Subaxes/Flags/Mixin/Config) — dieselbe Heuristik wie die Goldstandard-Audits; deckt sich mit den
# Registry-AllStrategies-Groessen (z.B. axis_03a=13, axis_06=24).
#
# Permutations-Raum: das Produkt explodiert kombinatorisch (das ist die These!), daher Overflow-Guard
# bei 1e15 → Ausgabe ">= 1e15 (kombinatorisch)".
#
# @task V41.F.6.1 G.1
# @reference docs/architektur/14_achsen_komposition_organ_metapher.md

function(comdare_print_axis_hierarchy)
    set(_topics_root "${CMAKE_CURRENT_SOURCE_DIR}/libs/cache_engine/topics")
    if(NOT IS_DIRECTORY "${_topics_root}")
        return()
    endif()

    file(GLOB _topic_dirs LIST_DIRECTORIES true "${_topics_root}/*")
    list(SORT _topic_dirs)

    set(_total_topics 0)
    set(_total_axes 0)
    set(_total_wrappers 0)
    set(_product 1)
    set(_overflow FALSE)
    set(_cap 1000000000000000)  # 1e15

    message(STATUS "")
    message(STATUS "═══════════════════════════════════════════════════════════════")
    message(STATUS " COMDARE Achsen-Hierarchie (G.1) — Anatomie-Achsen pro Topic")
    message(STATUS "═══════════════════════════════════════════════════════════════")

    foreach(_topic_dir ${_topic_dirs})
        if(NOT IS_DIRECTORY "${_topic_dir}")
            continue()
        endif()
        get_filename_component(_topic_name "${_topic_dir}" NAME)

        file(GLOB _axis_dirs LIST_DIRECTORIES true "${_topic_dir}/axis_*")
        list(SORT _axis_dirs)

        set(_topic_axis_lines "")
        set(_topic_has_axis FALSE)
        foreach(_axis_dir ${_axis_dirs})
            if(NOT IS_DIRECTORY "${_axis_dir}")
                continue()
            endif()
            get_filename_component(_axis_name "${_axis_dir}" NAME)

            file(GLOB _hdrs "${_axis_dir}/*.hpp")
            set(_count 0)
            foreach(_h ${_hdrs})
                get_filename_component(_hn "${_h}" NAME)
                string(TOLOWER "${_hn}" _hl)
                if(_hl MATCHES "concept|_base|registry|subaxes|flags|mixin|config")
                    continue()
                endif()
                math(EXPR _count "${_count} + 1")
            endforeach()

            if(_count GREATER 0)
                list(APPEND _topic_axis_lines "      ${_axis_name}: ${_count} Wrapper")
                math(EXPR _total_axes "${_total_axes} + 1")
                math(EXPR _total_wrappers "${_total_wrappers} + ${_count}")
                if(NOT _overflow)
                    math(EXPR _product "${_product} * ${_count}")
                    if(_product GREATER _cap)
                        set(_overflow TRUE)
                    endif()
                endif()
                set(_topic_has_axis TRUE)
            endif()
        endforeach()

        if(_topic_has_axis)
            math(EXPR _total_topics "${_total_topics} + 1")
            message(STATUS "  ${_topic_name}/")
            foreach(_line ${_topic_axis_lines})
                message(STATUS "${_line}")
            endforeach()
        endif()
    endforeach()

    message(STATUS "───────────────────────────────────────────────────────────────")
    message(STATUS " Gesamt: ${_total_topics} Topics · ${_total_axes} Achsen · ${_total_wrappers} Wrapper")
    if(_overflow)
        message(STATUS " Permutations-Raum (Produkt der Achsen-Kardinalitaeten): >= 1e15 (kombinatorisch)")
    else()
        message(STATUS " Permutations-Raum (Produkt der Achsen-Kardinalitaeten): ${_product}")
    endif()
    message(STATUS "═══════════════════════════════════════════════════════════════")
    message(STATUS "")
endfunction()
