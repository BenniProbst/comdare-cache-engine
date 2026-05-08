# check_submodules.cmake — Pruefe COMDARE-Submodules-Verfuegbarkeit
#
# Korrektur F12-K: comdare-cache-engine nutzt AUSNAHMSWEISE feste Git-
# Submodules (entgegen der globalen No-Submodules-Regel). Diese Pruefung
# stellt sicher, dass die Submodule initialisiert sind.

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
  message(WARNING "──────────────────────────────────────────────────────")
  message(WARNING "Fehlende COMDARE-Submodules:")
  foreach(sm ${_missing_submodules})
    message(WARNING "  - ${sm}")
  endforeach()
  message(WARNING "")
  message(WARNING "Bitte ausfuehren (nach Cluster-Migration verfuegbar):")
  message(WARNING "  git submodule update --init --recursive")
  message(WARNING "")
  message(WARNING "Skelett-Build moeglich (Submodules werden in Phase 4.B")
  message(WARNING "Cluster-Migration eingebunden).")
  message(WARNING "──────────────────────────────────────────────────────")
endif()
