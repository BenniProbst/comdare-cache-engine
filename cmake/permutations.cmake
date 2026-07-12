# permutations.cmake — Trigger fuer Permutations-Codegen (F-EXTRA-5 NO-PYTHON, REVIDIERT durch H6)
#
# Memory-Direktive: KEIN Python in Build-Pipeline (Talos OS hat kein Python).
# Standardmaessig wird codegen.cmake via "cmake -P" aufgerufen.
# Optional via codegen.sh (POSIX) oder codegen.bat (Windows) — synchron gepflegt.

set(COMDARE_PERMUTATION_OUTPUT
    "${CMAKE_BINARY_DIR}/generated/permutations.cmake")

set(COMDARE_TARGET_ISA "auto" CACHE STRING
    "Target ISA fuer ConstraintFilter (auto = via PlatformProbe)")

set(COMDARE_PERMUTATION_CODEGEN_BACKEND "cmake" CACHE STRING
    "Codegen-Backend: cmake (Standard via cmake -P), sh, bat, oder cpp (#25 B C++23-Tool)")
set_property(CACHE COMDARE_PERMUTATION_CODEGEN_BACKEND PROPERTY STRINGS cmake sh bat cpp)

if(NOT COMDARE_BUILD_PERMUTATIONS)
    return()
endif()

# V36.A (2026-05-23): Pfad-Bug-Fix — CMAKE_CURRENT_LIST_DIR ist robust
# auch wenn cache-engine als add_subdirectory eingebunden ist
# (CMAKE_SOURCE_DIR zeigt sonst auf den Container, nicht auf cache-engine).
set(_ce_root "${CMAKE_CURRENT_LIST_DIR}/..")
set(_codegen_tools "${_ce_root}/tools/permutation_codegen")

# V36.B (2026-05-23): Default-Profil "smoke" (~27 Perms) statt voller
# Permutations-Explosion (~5.5 Mrd, Memory-Direktive ProfileFilter Pflicht)
set(COMDARE_PERMUTATION_PROFILE "smoke" CACHE STRING
    "Profile-Filter: smoke (~27 Perms), medium (~108), full (Stunden bis Tage)")
set_property(CACHE COMDARE_PERMUTATION_PROFILE PROPERTY STRINGS smoke medium full)

# V36.B (2026-05-23): Tri-State Mode (User-Direktive 2026-05-23)
#   on_build_on_demand  - Default. Nur fehlende/veraltete Wrappers neu schreiben.
#   on_rebuild          - Bei jedem cmake-Run alle Wrappers neu schreiben.
#   off_pause_build     - Codegen pausiert. Vorhandenes Manifest wird benutzt.
#                         Wenn 0 Permutationen vorhanden -> Runtime-Error
#                         (siehe messung_driver/permutations_runtime_check).
set(COMDARE_PERMUTATION_MODE "on_build_on_demand" CACHE STRING
    "Permutations-Build-Modus: on_rebuild | on_build_on_demand | off_pause_build")
set_property(CACHE COMDARE_PERMUTATION_MODE PROPERTY STRINGS
    on_rebuild on_build_on_demand off_pause_build)

if(COMDARE_PERMUTATION_MODE STREQUAL "off_pause_build")
    message(STATUS "Permutations: Build pausiert (off_pause_build).")
    # Pruefe ob Manifest existiert
    if(EXISTS "${_out_dir}/permutations_manifest.txt")
        # Re-include existing generated permutations.cmake wenn vorhanden
        if(EXISTS "${COMDARE_PERMUTATION_OUTPUT}")
            message(STATUS "Permutations: nutze vorhandenes Manifest.")
        else()
            message(WARNING "Permutations: Manifest vorhanden aber permutations.cmake fehlt.")
        endif()
    else()
        message(WARNING "Permutations: kein Manifest. Experiment-Driver wird Fatal werfen.")
    endif()
    return()
endif()

if(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "cmake")
    set(_codegen_cmd
        ${CMAKE_COMMAND}
        "-DCOMDARE_TARGET_ISA=${COMDARE_TARGET_ISA}"
        "-DCOMDARE_PROFILE=${COMDARE_PERMUTATION_PROFILE}"
        "-DCOMDARE_MODE=${COMDARE_PERMUTATION_MODE}"
        "-DCOMDARE_OUTPUT=${COMDARE_PERMUTATION_OUTPUT}"
        -P "${_codegen_tools}/codegen.cmake")
elseif(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "sh")
    set(_codegen_cmd
        sh "${_codegen_tools}/codegen.sh"
        "--target-isa=${COMDARE_TARGET_ISA}"
        "--profile=${COMDARE_PERMUTATION_PROFILE}"
        "--mode=${COMDARE_PERMUTATION_MODE}"
        "--output=${COMDARE_PERMUTATION_OUTPUT}")
elseif(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "bat")
    set(_codegen_cmd
        cmd /c "${_codegen_tools}/codegen.bat"
        "--target-isa=${COMDARE_TARGET_ISA}"
        "--profile=${COMDARE_PERMUTATION_PROFILE}"
        "--mode=${COMDARE_PERMUTATION_MODE}"
        "--output=${COMDARE_PERMUTATION_OUTPUT}")
elseif(COMDARE_PERMUTATION_CODEGEN_BACKEND STREQUAL "cpp")
    # #25 B (opt-in): C++23-Port von codegen.cmake (apps/permutation_codegen_tool/ ->
    # comdare_permutation_codegen_cli). Erzeugt eine BYTE-IDENTISCHE permutations.cmake.
    # 2-Pass-Build (wie R5.I anatomy_codegen_runner): permutations.cmake laeuft zur Configure-Time,
    # daher muss das Tool VORHER gebaut sein. Das Default-Backend bleibt "cmake" (kein 2-Pass noetig)
    # -> der reale golden-320-Bau ist von diesem Zweig unberuehrt.
    set(_perm_cli_name "comdare-permutation-codegen")
    if(WIN32)
        set(_perm_cli_name "${_perm_cli_name}.exe")
    endif()
    set(_perm_cli_dir "${CMAKE_BINARY_DIR}/apps/permutation_codegen_tool")
    set(_perm_cli_candidates
        "${_perm_cli_dir}/${_perm_cli_name}"
        "${_perm_cli_dir}/Release/${_perm_cli_name}"
        "${_perm_cli_dir}/Debug/${_perm_cli_name}"
        "${_perm_cli_dir}/RelWithDebInfo/${_perm_cli_name}"
        "${_perm_cli_dir}/MinSizeRel/${_perm_cli_name}")
    set(_perm_cli_path "")
    foreach(_cand ${_perm_cli_candidates})
        if(EXISTS "${_cand}")
            set(_perm_cli_path "${_cand}")
            break()
        endif()
    endforeach()
    if(NOT _perm_cli_path)
        message(FATAL_ERROR
            "Permutations-Codegen Backend 'cpp': Tool noch nicht gebaut.\n"
            "  Erwartet eines von: ${_perm_cli_candidates}\n"
            "  2-Pass-Build: 'cmake --build <build-dir> --target comdare_permutation_codegen_cli' "
            "und danach erneut konfigurieren. (Das Default-Backend 'cmake' braucht keinen 2-Pass.)")
    endif()
    set(_codegen_cmd
        "${_perm_cli_path}"
        "--target-isa=${COMDARE_TARGET_ISA}"
        "--profile=${COMDARE_PERMUTATION_PROFILE}"
        "--mode=${COMDARE_PERMUTATION_MODE}"
        "--output=${COMDARE_PERMUTATION_OUTPUT}"
        "--axes-versions=${_codegen_tools}/axes_versions.txt")
else()
    message(FATAL_ERROR "Unbekanntes COMDARE_PERMUTATION_CODEGEN_BACKEND: ${COMDARE_PERMUTATION_CODEGEN_BACKEND}")
endif()

message(STATUS "Permutations-Codegen (Backend=${COMDARE_PERMUTATION_CODEGEN_BACKEND}, Profile=${COMDARE_PERMUTATION_PROFILE}, Mode=${COMDARE_PERMUTATION_MODE}) wird aufgerufen ...")
execute_process(
    COMMAND ${_codegen_cmd}
    WORKING_DIRECTORY "${_ce_root}"
    RESULT_VARIABLE _codegen_result)

if(NOT _codegen_result EQUAL 0)
    message(FATAL_ERROR "Permutations-Codegen fehlgeschlagen (exit ${_codegen_result})")
endif()
