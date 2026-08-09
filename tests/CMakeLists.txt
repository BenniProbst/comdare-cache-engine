# tests — Google Tests (F-EXTRA-8: generisch + spezifisch)

if(COMDARE_BUILD_TESTS)
  add_subdirectory(unit)
  add_subdirectory(integration)
  add_subdirectory(generic_module_tests)
  add_subdirectory(module_specific_tests)

  # -- HY-0 (2026-08-08): ABI-Etikett- und Anker-Wache ueber die Hybrid-Doku ----
  # WARUM ALS ctest-TEST UND NICHT NUR ALS CI-JOB (T-7: Registrierung IST Teil des
  # Tests): die uebrigen Wachen dieses Repos (XML-Wohlgeformtheit, Diff-Hygiene,
  # YAML-Schluessel) haengen ausschliesslich an einer Zeile in .gitlab-ci.yml. Sie
  # laufen damit NUR dort, wo jemand sie eingetragen hat -- lokal vor dem Push
  # laeuft keine von ihnen mit `ctest`. Diese hier haengt am Bauweg: wer ctest
  # faehrt, faehrt sie mit. Die Test-Sichtbarkeits-Wache (W-1) zaehlt sie
  # mit, und genau daran ist nachweisbar, dass die Registrierung ankommt.
  #
  # KEIN CMAKE_SOURCE_DIR: das ce wird auch als Unterprojekt gebaut, dann zeigte
  # CMAKE_SOURCE_DIR auf die Wurzel des Oberprojekts und die Wache liefe gegen den
  # falschen Baum. CMAKE_CURRENT_SOURCE_DIR/.. ist die ce-Wurzel, unabhaengig davon,
  # wer oben steht -- und sie wird dem Skript zusaetzlich per -w mitgegeben, damit es
  # nicht auf `git rev-parse --show-toplevel` angewiesen ist (das liefert in einem
  # vendorierten, nicht als Submodul eingebundenen ce die falsche Wurzel).
  add_test(NAME hy_label_gate
           COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/ci_hy_label_gate.sh"
                      -w "${CMAKE_CURRENT_SOURCE_DIR}/.."
           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()
