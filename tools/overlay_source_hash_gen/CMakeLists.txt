# E-E (07.08.2026) -- overlay_source_hash_gen: der Pre-Build-Codegen des Overlay-Glieds [7].
#
# NICHT EXCLUDE_FROM_ALL (anders als die Registry-Generatoren): sein Ergebnis ist ein Compile-Define,
# das JEDE Uebersetzungseinheit mit einem Anatomy-Fingerprint braucht. Ein Werkzeug, das der
# Standard-Bau nicht baut, koennte diese Zusage nicht halten.
#
# Er zieht NUR den Schnitt-Header + die SHA-512-Primitive -- bewusst KEINEN Achsen-Code: der Codegen
# laeuft VOR der Uebersetzung dessen, was er hasht, und darf davon nicht abhaengen. Boost::mp11 kommt
# ueber experiment_tree/axis_path_serialization.hpp herein (dort wohnt kCompositionAxisNames, die
# kanonische Organ-Ordnung, gegen die die Schnitt-Wachen pruefen).
#
# -- WARUM DIE GEERBTE VERZEICHNIS-KANTE HIER GELOESCHT WIRD (08.08.2026) ------------------------------
# Der Satz "er darf von nichts abhaengen, was er hasht" stand oben schon als ABSICHT -- durchgesetzt war
# er nicht. Die Wurzel-CMakeLists.txt legt zwei Verzeichnis-Kanten (CMakeLists.txt, link_libraries(
# comdare::vendor_mimalloc) und link_libraries(comdare::vendor_snmalloc)), und die vererben sich auf
# JEDES danach angelegte Ziel -- add_subdirectory(tools) steht spaeter, also auch auf dieses hier.
# Am Objekt gemessen trug der Codegen damit
#     LINK_LIBRARIES = comdare::vendor_mimalloc;comdare::vendor_snmalloc;Boost::mp11
# obwohl seine eigene Zeile unten NUR Boost::mp11 nennt.
#
# Die Folge war ein CMake-Zyklus, sobald das Vendor-Provisioning an ist (super-Superbuild immer,
# ce-standalone mit -DCOMDARE_BUILD_PERMUTATIONS=ON): comdare_vendor_mimalloc ist eine STATIC_LIBRARY,
# bekommt also von cmake/overlay_source_hash.cmake die Wartekante auf den Codegen -- und der Codegen
# wartete zugleich auf mimalloc. CMake bricht das mit "strongly connected component (cycle)" ab; latent
# blieb es nur, weil ein warmes Bau-Verzeichnis nicht neu generiert.
#
# GEHEILT WIRD AN DER WURZEL, NICHT AM GRAPHEN: der Codegen wird von der geerbten Kante befreit, statt
# die Ausnahme in die Kanten-Schleife zu schreiben. Die Loeschung ist MECHANISCH (sie nennt keinen
# Bibliotheks-Namen) und wirkt nur in diesem Unterverzeichnis, das genau ein Ziel enthaelt. Eine
# Namensliste waere hier derselbe Fehler, den cmake/overlay_source_hash.cmake ausdruecklich vermeidet:
# der naechste vendored Baustein fiele wieder herein.
#
# Der Codegen VERLIERT dabei nichts, was er braucht: mimalloc ist ein Allokator (er hasht Dateien und
# alloziert nichts Nennenswertes), snmalloc traegt reine Compile-Belange fuer Ziele, die
# <snmalloc/snmalloc.h> ueber die Katalog-Kette ziehen -- main.cpp zieht sie nicht (Includes: nur
# builder/overlay_source_set.hpp, sha512/ctsha512.hpp und Standard-Header). Boost::mp11 setzt die Zeile
# unten SELBST, ist also von der Loeschung nicht betroffen.
#
# Bewacht wird die Zusage in cmake/overlay_source_hash.cmake: dort prueft eine Wache die transitive
# Abhaengigkeits-Schliessung dieses Ziels und bricht LAUT ab, falls sie je wieder ein bauendes Ziel
# enthaelt. Diese Zeile und jene Wache gehoeren zusammen.
set_property(DIRECTORY PROPERTY LINK_LIBRARIES "")

add_executable(comdare_overlay_source_hash_gen main.cpp)
target_compile_features(comdare_overlay_source_hash_gen PRIVATE cxx_std_23)
target_include_directories(comdare_overlay_source_hash_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(comdare_overlay_source_hash_gen PRIVATE Boost::mp11)
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_overlay_source_hash_gen)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle tools). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_overlay_source_hash_gen)
