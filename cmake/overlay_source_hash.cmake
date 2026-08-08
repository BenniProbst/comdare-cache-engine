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

# -- (a2) WACHE: der Codegen darf an KEINEM bauenden Ziel des Projekts haengen -------------------------
# Sie schliesst die Luecke, die am 08.08.2026 den super-Superbuild lahmgelegt hat. Die Wurzel-CMakeLists
# legt Verzeichnis-Kanten (link_libraries(comdare::vendor_mimalloc/snmalloc)), die JEDES danach angelegte
# Ziel erben -- auch den Codegen, obwohl seine eigene CMakeLists nur Boost::mp11 nennt. Ergebnis war ein
# CMake-Zyklus: comdare_vendor_mimalloc bekam unten die Wartekante auf den Codegen, und der Codegen
# wartete zugleich auf mimalloc. Latent blieb das nur, weil ein warmes Bau-Verzeichnis nicht neu
# generiert -- vor jeder Messung wird aber frisch gebaut.
#
# WARUM EINE WACHE UND KEINE AUSNAHME IM GRAPHEN: die Ausnahme waere die leichtere Heilung -- die Ziele,
# von denen der Codegen abhaengt, einfach von der Kante unten ausnehmen. Sie waere aber die FALSCHE:
# ein so ausgenommenes Ziel uebersetzte gegen einen fehlenden oder, nach einem frueheren Bau, gegen
# einen VERALTETEN Header -- also gegen den Fehler, den der Kommentar unten (Z. 47ff) ausdruecklich als
# den schlimmeren benennt. Heute traefe es nur vendored C-Code ohne Fingerprint; der naechste Baustein
# in dieser Schliessung koennte aber sehr wohl einen tragen, und dann loege der Fingerprint STILL.
# Deshalb wird nicht der Graph geflickt, sondern die Zusage durchgesetzt: der Codegen laeuft vor allem
# anderen und haengt an nichts. Geheilt wird an der Wurzel (tools/overlay_source_hash_gen/CMakeLists.txt
# loescht die geerbte Verzeichnis-Kante), hier wird die Zusage nur BEWACHT.
#
# MECHANISCH, NICHT PER NAMENSLISTE (dieselbe Doktrin wie die Kante unten): die Schliessung wird aus dem
# Ziel-Graphen abgeleitet. Kein Bibliotheks-Name steht in dieser Datei, der naechste vendored Baustein
# faellt also nicht durch, sondern schlaegt sofort auf.
function(_comdare_overlay_gen_schliessung _ziel _out_var)
    set(_offen "${_ziel}")
    set(_gesehen "")
    while(_offen)
        list(POP_FRONT _offen _t)
        if(_t IN_LIST _gesehen)
            continue()
        endif()
        list(APPEND _gesehen "${_t}")
        set(_kanten "")
        foreach(_prop IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_wert "${_t}" ${_prop})
            if(_wert)
                list(APPEND _kanten ${_wert})
            endif()
        endforeach()
        foreach(_k IN LISTS _kanten)
            # Ein Eintrag kann ein blosser Ziel-Name, ein Pfad oder ein Generator-Ausdruck sein. Statt
            # die Genex-Grammatik nachzubauen, wird JEDER Bezeichner-Token gegen if(TARGET) geprueft --
            # konservativ: lieber ein Ziel zu viel in der Schliessung als eines zu wenig.
            string(REGEX MATCHALL "[A-Za-z_][A-Za-z0-9_:+.-]*" _token "${_k}")
            foreach(_tok IN LISTS _token)
                if(TARGET "${_tok}")
                    get_target_property(_alias "${_tok}" ALIASED_TARGET)
                    if(_alias)
                        list(APPEND _offen "${_alias}")
                    else()
                        list(APPEND _offen "${_tok}")
                    endif()
                endif()
            endforeach()
        endforeach()
    endwhile()
    list(REMOVE_ITEM _gesehen "${_ziel}")
    set(${_out_var} "${_gesehen}" PARENT_SCOPE)
endfunction()

_comdare_overlay_gen_schliessung(comdare_overlay_source_hash_gen _ovl_gen_schliessung)
set(_ovl_gen_bauend "")
foreach(_t IN LISTS _ovl_gen_schliessung)
    get_target_property(_typ      "${_t}" TYPE)
    get_target_property(_imported "${_t}" IMPORTED)
    if(_imported OR _typ STREQUAL "INTERFACE_LIBRARY")
        continue() # importierte Ziele und reine INTERFACE-Libs bauen nichts -- sie bekommen unten auch
                   # keine Kante und koennen den Zyklus daher nicht schliessen
    endif()
    list(APPEND _ovl_gen_bauend "${_t}")
endforeach()
if(_ovl_gen_bauend)
    string(REPLACE ";" ", " _ovl_gen_bauend_text "${_ovl_gen_bauend}")
    message(FATAL_ERROR
        "E-E: comdare_overlay_source_hash_gen haengt an bauenden Zielen des Projekts: ${_ovl_gen_bauend_text}.\n"
        "Das ist ein ZYKLUS, kein Geschmacksfehler: jedes bauende Ziel bekommt unten die Wartekante auf "
        "comdare_overlay_source_hash, und dieses wartet auf den Codegen -- CMake bricht den Generate-Schritt "
        "mit 'strongly connected component (cycle)' ab, sobald frisch generiert wird.\n"
        "Der Codegen erzeugt den Header, gegen den ALLES uebersetzt; er laeuft vor allem anderen und darf "
        "deshalb an nichts haengen. Er hasht Dateien -- er braucht weder Allokator noch Achsen-Code.\n"
        "HEILUNG: die Abhaengigkeit am Codegen entfernen, nicht die Kante unten. Kommt sie aus einem "
        "link_libraries() der Wurzel-CMakeLists (Verzeichnis-Kante, vererbt sich auf jedes spaeter "
        "angelegte Ziel), loescht tools/overlay_source_hash_gen/CMakeLists.txt sie bereits per "
        "set_property(DIRECTORY PROPERTY LINK_LIBRARIES \"\") -- pruefe, ob eine NEUE Quelle hinzugekommen ist.")
endif()

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
