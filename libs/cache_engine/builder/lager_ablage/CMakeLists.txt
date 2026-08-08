# A9-S3 -- xlsx-Ergebnis-Writer (F3). Interfaces/Factory/CSV-Fallback sind header-only (kein Ziel noetig
# fuer sie, s. bestandslog/-Praezedenz); dieses Ziel traegt AUSSCHLIESSLICH die xlsx-Backend-TU
# (xlsx_ergebnis_writer.cpp), damit <xlsxwriter.h> nicht in Consumer leckt (Design-Dossier Abschnitt 4.1).
#
# Consumer-only wie der Vendor selbst: dieses Target wird nur gezogen, wenn ein Consumer es
# add_subdirectory't bzw. dagegen linkt -- kein bestehendes ce-Ziel haengt daran, golden-/CI-Pfad
# unberuehrt.

if(NOT TARGET comdare_vendored_xlsxwriter)
    add_subdirectory("${PROJECT_SOURCE_DIR}/ext/io/libxlsxwriter" "${CMAKE_BINARY_DIR}/_ext_io_libxlsxwriter")
endif()

comdare_add_library(comdare_lager_ablage PROJECT CACHE_ENGINE SOURCES
    xlsx_ergebnis_writer.cpp)

add_library(comdare::lager_ablage ALIAS comdare_lager_ablage)

target_include_directories(comdare_lager_ablage PUBLIC
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")

target_compile_features(comdare_lager_ablage PUBLIC cxx_std_23)

# PRIVATE: <xlsxwriter.h> gehoert zum Bau dieser EINEN TU, nicht zur oeffentlichen Schnittstelle
# (ergebnis_mappe.hpp kennt den Vendor-Header nicht). Consumer sehen nur comdare::lager_ablage.
target_link_libraries(comdare_lager_ablage PRIVATE comdare::vendored_xlsxwriter)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_lager_ablage)
endif()
