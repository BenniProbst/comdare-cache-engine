# prt_art/legacy_reimpl — MIGRATED nach prt-art (REV 7.6 V10.1)
#
# Die 14 P*-Subordner wurden in V10.1 physisch nach comdare-prt-art
# verschoben (siehe README.md fuer Details).
# Dieses CMakeLists ist nur noch ein no-op + Hinweis-Message.

if(COMDARE_BUILD_LEGACY_REIMPL)
    message(WARNING
        "[V10.1] cache-engine prt_art/legacy_reimpl/ ist LEER — die Pruefling-"
        "Re-Implementations wurden nach comdare-prt-art verschoben. Aktiviere "
        "stattdessen dort COMDARE_PRT_ART_BUILD_LEGACY_REIMPL=ON.")
endif()
