# E-E (07.08.2026) -- die CMake-Haelfte des Overlay-Glieds [7].
#
# Sie tut GENAU ZWEI Dinge:
#   (a) sie laesst tools/overlay_source_hash_gen VOR jeder Uebersetzung laufen und den
#       Compile-Define-Header ${PROJECT_BINARY_DIR}/generated/cache_engine/abi/
#       overlay_source_hash_generated.hpp schreiben;
#   (b) sie haengt JEDES Bau-Ziel des Projekts an dieses Werkzeug, damit kein Ziel uebersetzt, bevor
#       der Header steht.
#
# -- WARUM DAS WERKZEUG BEI JEDEM BAU LAEUFT (add_custom_target statt add_custom_command mit DEPENDS) --
# Ein add_custom_command mit einer DEPENDS-Dateiliste braeuchte diese Liste HIER, in CMake -- also eine
# ZWEITE Kopie des Schnitts neben builder/overlay_source_set.hpp. Genau solche Zweit-Kopien laufen
# auseinander, und die Folge waere die schlimmste denkbare: ein Quelltext aendert sich, CMake sieht ihn
# nicht in seiner Glob-Liste, der Header bleibt stehen, und der Fingerprint LUEGT. Der Schnitt hat
# deshalb GENAU EINE Quelle, und das Werkzeug liest sie bei jedem Bau neu.
#
# Die Kosten dafuer sind vernachlaessigbar (etwa 3 MB lesen und einmal hashen), und die
# Neuuebersetzungs-Kaskade entsteht NICHT: das Werkzeug schreibt den Header nur, wenn sich sein INHALT
# aendert (schreibe_wenn_anders in main.cpp). Bleibt der Quellbaum gleich, bleibt der Zeitstempel des
# Headers gleich, und kein einziges Ziel uebersetzt neu.
#
# -- WAS PASSIERT, WENN SICH DER HASH AENDERT ----------------------------------------------------------
# Dann uebersetzt alles neu, was den Fingerprint traegt. Das ist KEIN Nebeneffekt, sondern der Zweck:
# der Fingerprint MUSS sich mit dem Quelltext bewegen, sonst liefert das Lager alte Binaries aus
# (build_orchestrator.hpp, dll_is_current).

set(_ovl_gen_header "${PROJECT_BINARY_DIR}/generated/cache_engine/abi/overlay_source_hash_generated.hpp")
set(_ovl_listing    "${PROJECT_BINARY_DIR}/generated/cache_engine/abi/overlay_source_set_manifest.txt")

if(NOT TARGET comdare_overlay_source_hash_gen)
    message(FATAL_ERROR
        "E-E: comdare_overlay_source_hash_gen fehlt. Ohne den Codegen bliebe das Overlay-Glied leer, und der "
        "Tier-Fingerprint deckte reine Quell-Code-Aenderungen NICHT -- genau die Luecke, die E-E schliesst. "
        "add_subdirectory(tools) muss VOR diesem Modul stehen.")
endif()

add_custom_target(comdare_overlay_source_hash ALL
    COMMAND $<TARGET_FILE:comdare_overlay_source_hash_gen>
            --root    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
            --output  "${_ovl_gen_header}"
            --listing "${_ovl_listing}"
    BYPRODUCTS "${_ovl_gen_header}" "${_ovl_listing}"
    COMMENT "comdare E-E: Overlay-Quell-Hash (Glied [7] des Tier-Fingerprints)"
    VERBATIM)
add_dependencies(comdare_overlay_source_hash comdare_overlay_source_hash_gen)

# -- (b) jedes Bau-Ziel haengt an dem Werkzeug --------------------------------------------------------
# Die Kante wird MECHANISCH ueber alle Ziele gelegt, nicht ueber eine gepflegte Namensliste. Begruendung
# aus der Erfahrung des Hauses: die Consumer-Liste des Paper-Codegens (CMakeLists.txt, Z. 1533+) musste
# jahrelang von Hand nachgetragen werden, und ein vergessener Eintrag war jedes Mal ein roter Bau. Hier
# waere ein vergessener Eintrag SCHLIMMER als rot -- das Ziel uebersetzte gegen einen fehlenden Header
# (harter Fehler) oder, nach einem frueheren Bau, gegen einen VERALTETEN (stille Falschidentitaet).
function(_comdare_overlay_hash_kante_in_verzeichnis _dir)
    get_property(_ziele DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _ziele)
        if(_t STREQUAL "comdare_overlay_source_hash" OR _t STREQUAL "comdare_overlay_source_hash_gen")
            continue() # das Werkzeug selbst haengt nicht an seinem eigenen Ergebnis
        endif()
        get_target_property(_typ "${_t}" TYPE)
        get_target_property(_imported "${_t}" IMPORTED)
        if(_imported OR _typ STREQUAL "INTERFACE_LIBRARY")
            continue() # importierte Ziele und reine INTERFACE-Libs uebersetzen nichts
        endif()
        add_dependencies("${_t}" comdare_overlay_source_hash)
    endforeach()
    get_property(_unter DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_u IN LISTS _unter)
        _comdare_overlay_hash_kante_in_verzeichnis("${_u}")
    endforeach()
endfunction()

_comdare_overlay_hash_kante_in_verzeichnis("${PROJECT_SOURCE_DIR}")

message(STATUS "comdare E-E: Overlay-Quell-Hash verdrahtet -> ${_ovl_gen_header}")
