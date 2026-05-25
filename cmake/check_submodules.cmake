# check_submodules.cmake — Pruefe COMDARE-Module-Verfuegbarkeit
#
# V41.E4 (2026-05-25, User-Direktive): Architektur-Korrektur. Die 6 Module
# unter modules/ sind cache-engine INTERN (eine Cache-Engine, 6 funktionale
# Saeulen). Sie sind NICHT als separate Git-Submodules getrackt — cache-engine
# wird nur als Gesamt-Framework versioniert. .gitmodules.template wird damit
# obsolet; die Module sind direkt im cache-engine-Repo als Skelett-
# Verzeichnisse enthalten und werden ab Phase 6+ inhaltlich befuellt.
#
# Diese Pruefung verifiziert nur noch, dass die 6 Verzeichnisse existieren.
# Wenn ja: STATUS-Bestaetigung. Wenn nein: WARNING (Repo-Inkonsistenz).

set(COMDARE_REQUIRED_SUBMODULES
  modules/comdare-search-engine
  modules/comdare-cache-engine-core
  modules/comdare-measurement
  modules/comdare-isa-dispatch
  modules/comdare-build-tools
  modules/comdare-test-system
)

set(_missing_modules "")
foreach(sm ${COMDARE_REQUIRED_SUBMODULES})
  # V41.E4: pruefe direktes Verzeichnis (modules/ ist KEIN Submodule mehr,
  # sondern interne Skelett-Hierarchie). CMakeLists.txt + README.md beweisen,
  # dass das Skelett richtig generiert ist.
  if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../${sm}/CMakeLists.txt"
     OR NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../${sm}/README.md")
    list(APPEND _missing_modules "${sm}")
  endif()
endforeach()

if(_missing_modules)
  # Hier nutzen wir WARNING — fehlende Skelette sind ein Repo-Bug, kein
  # Cluster-Migrations-TODO mehr.
  message(WARNING "──────────────────────────────────────────────────────")
  message(WARNING "Fehlende cache-engine-interne Module (Skelett-Inkonsistenz):")
  foreach(sm ${_missing_modules})
    message(WARNING "  - ${sm}")
  endforeach()
  message(WARNING "  Erwarteter Inhalt pro Modul: CMakeLists.txt + README.md")
  message(WARNING "──────────────────────────────────────────────────────")
else()
  message(STATUS "V41.E4 cache-engine interne Module: 6/6 Skelett vollstaendig")
endif()
