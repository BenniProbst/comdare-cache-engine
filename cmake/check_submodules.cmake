# check_submodules.cmake — Pruefe COMDARE-Submodules-Verfuegbarkeit
#
# Korrektur F12-K: comdare-cache-engine nutzt AUSNAHMSWEISE feste Git-
# Submodules (entgegen der globalen No-Submodules-Regel). Diese Pruefung
# stellt sicher, dass die Submodule initialisiert sind.
#
# V35.F (2026-05-22): COMDARE_QUIET_SUBMODULE_CHECK ON -> Ausgabe als
# STATUS statt WARNING. Damit verschwinden die CLion-Debug-Warnings, wenn
# das umliegende Repo (z.B. Diplomarbeit) den cache-engine-Skelett-Build
# bewusst akzeptiert. Cluster-Migration setzt die Variable nicht und sieht
# weiterhin WARNINGs.

set(COMDARE_REQUIRED_SUBMODULES
  modules/comdare-search-engine
  modules/comdare-cache-engine-core
  modules/comdare-measurement
  modules/comdare-isa-dispatch
  modules/comdare-build-tools
  modules/comdare-test-system
)

set(_missing_submodules "")
foreach(sm ${COMDARE_REQUIRED_SUBMODULES})
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${sm}/CMakeLists.txt")
    list(APPEND _missing_submodules "${sm}")
  endif()
endforeach()

if(_missing_submodules)
  if(COMDARE_QUIET_SUBMODULE_CHECK)
    set(_log STATUS)
  else()
    set(_log WARNING)
  endif()
  message(${_log} "──────────────────────────────────────────────────────")
  message(${_log} "Fehlende COMDARE-Submodules:")
  foreach(sm ${_missing_submodules})
    message(${_log} "  - ${sm}")
  endforeach()
  message(${_log} "")
  message(${_log} "Bitte ausfuehren (nach Cluster-Migration verfuegbar):")
  message(${_log} "  git submodule update --init --recursive")
  message(${_log} "")
  message(${_log} "Skelett-Build moeglich (Submodules werden in Phase 4.B")
  message(${_log} "Cluster-Migration eingebunden).")
  message(${_log} "──────────────────────────────────────────────────────")
endif()
