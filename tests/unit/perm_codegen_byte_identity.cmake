# perm_codegen_byte_identity.cmake — #25 B Byte-Identitaets-Treiber (offizieller CMake/ctest-Weg).
#
# Beweis der TREUEN, VOLLEN Ersetzung: faehrt das Default-Backend (tools/permutation_codegen/
# codegen.cmake via `cmake -P`) UND das opt-in cpp-Tool (comdare_permutation_codegen_cli) auf
# DIESELBEN Eingaben und assertiert byte-identische Ausgabe fuer ALLE VIER Artefakte:
#   (1) permutations.cmake              — byte-identisch
#   (2) permutations_manifest.txt       — byte-identisch
#   (3) perm_src/perm_<id>.cpp          — byte-identisch (voll, je Datei)
#   (4) perm_versions/perm_<id>.version — byte-identisch AUSSER der `last_codegen`-Zeile
#       (codegen.cmake Z.508 = ${CMAKE_CURRENT_LIST_FILE}, backend-identifizierend; das cpp-Tool
#       schreibt ehrlich seinen EIGENEN Marker). Diese eine Zeile wird vor dem Vergleich in BEIDEN
#       normalisiert.
#
# METHODIK (wichtig): (a) Beide Backends erhalten den IDENTISCHEN --output-Pfad, damit der in
# add_library(...) eingebettete _perm_src_dir (= dirname(OUTPUT)/perm_src) fuer beide gleich ist.
# (b) Das cmake-Backend laeuft zuerst und wird als Referenz weg-kopiert; DANN werden perm_src/ +
# perm_versions/ geloescht, damit die V36.E selective-rebuild-Logik des cpp-Backends die Wrapper
# tatsaechlich NEU schreibt (sonst wuerde es die schon existierenden ueberspringen). (c) VOLLER-
# ERSATZ-NACHWEIS: jeder in der cpp-permutations.cmake referenzierte perm_src/*.cpp MUSS danach
# existieren (Referenzen erfuellt).
#
# Erwartete -D Variablen: CODEGEN_CMAKE CLI_TOOL AXES_VERSIONS PROFILE TARGET_ISA MODE WORKDIR

foreach(_req CODEGEN_CMAKE CLI_TOOL AXES_VERSIONS PROFILE TARGET_ISA MODE WORKDIR)
    if(NOT DEFINED ${_req})
        message(FATAL_ERROR "perm_codegen_byte_identity: -D${_req} fehlt.")
    endif()
endforeach()

# Frisches Arbeitsverzeichnis (deterministisch).
file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")

set(_out "${WORKDIR}/permutations.cmake")
set(_manifest "${WORKDIR}/permutations_manifest.txt")
set(_perm_src "${WORKDIR}/perm_src")
set(_perm_versions "${WORKDIR}/perm_versions")

# ── 1. Default-Backend: codegen.cmake via cmake -P (schreibt alle vier Artefakte) ─────────────
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DCOMDARE_TARGET_ISA=${TARGET_ISA}"
        "-DCOMDARE_PROFILE=${PROFILE}"
        "-DCOMDARE_MODE=${MODE}"
        "-DCOMDARE_OUTPUT=${_out}"
        -P "${CODEGEN_CMAKE}"
    RESULT_VARIABLE _rc_cmake
    OUTPUT_VARIABLE _log_cmake
    ERROR_VARIABLE  _log_cmake_err)
if(NOT _rc_cmake EQUAL 0)
    message(FATAL_ERROR "cmake-Backend fehlgeschlagen (rc=${_rc_cmake}):\n${_log_cmake}\n${_log_cmake_err}")
endif()
foreach(_expect "${_out}" "${_manifest}" "${_perm_src}" "${_perm_versions}")
    if(NOT EXISTS "${_expect}")
        message(FATAL_ERROR "cmake-Backend erzeugte nicht: ${_expect}")
    endif()
endforeach()

# Referenz sichern (gleiches Verzeichnis -> gleicher _perm_src_dir).
set(_ref_out "${WORKDIR}/ref_permutations.cmake")
set(_ref_manifest "${WORKDIR}/ref_permutations_manifest.txt")
set(_ref_perm_src "${WORKDIR}/ref_perm_src")
set(_ref_perm_versions "${WORKDIR}/ref_perm_versions")
file(COPY_FILE "${_out}" "${_ref_out}")
file(COPY_FILE "${_manifest}" "${_ref_manifest}")
file(COPY "${_perm_src}/" DESTINATION "${_ref_perm_src}")
file(COPY "${_perm_versions}/" DESTINATION "${_ref_perm_versions}")

# perm_src/ + perm_versions/ loeschen, damit das cpp-Backend NEU schreibt (selective-rebuild).
file(REMOVE_RECURSE "${_perm_src}" "${_perm_versions}")

# ── 2. opt-in cpp-Backend: gebautes CLI-Tool, IDENTISCHER --output-Pfad ───────────────────────
execute_process(
    COMMAND "${CLI_TOOL}"
        "--target-isa=${TARGET_ISA}"
        "--profile=${PROFILE}"
        "--mode=${MODE}"
        "--output=${_out}"
        "--axes-versions=${AXES_VERSIONS}"
    RESULT_VARIABLE _rc_cpp
    OUTPUT_VARIABLE _log_cpp
    ERROR_VARIABLE  _log_cpp_err)
if(NOT _rc_cpp EQUAL 0)
    message(FATAL_ERROR "cpp-Backend fehlgeschlagen (rc=${_rc_cpp}):\n${_log_cpp}\n${_log_cpp_err}")
endif()

# ── 3. Byte-Identitaet (permutations.cmake + manifest) ────────────────────────────────────────
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${_ref_out}" "${_out}" RESULT_VARIABLE _d1)
if(NOT _d1 EQUAL 0)
    message(FATAL_ERROR "BYTE-IDENTITAET VERLETZT (permutations.cmake, Profile=${PROFILE}).")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${_ref_manifest}" "${_manifest}" RESULT_VARIABLE _d2)
if(NOT _d2 EQUAL 0)
    message(FATAL_ERROR "BYTE-IDENTITAET VERLETZT (permutations_manifest.txt, Profile=${PROFILE}).")
endif()

# ── 4. Byte-Identitaet perm_src/*.cpp (voll, je Datei) + Set-Gleichheit ───────────────────────
file(GLOB _ref_wrappers RELATIVE "${_ref_perm_src}" "${_ref_perm_src}/*.cpp")
file(GLOB _cpp_wrappers RELATIVE "${_perm_src}" "${_perm_src}/*.cpp")
list(LENGTH _ref_wrappers _n_ref_w)
list(LENGTH _cpp_wrappers _n_cpp_w)
if(NOT _n_ref_w EQUAL _n_cpp_w)
    message(FATAL_ERROR "perm_src Datei-Anzahl weicht ab (cmake=${_n_ref_w}, cpp=${_n_cpp_w}, Profile=${PROFILE}).")
endif()
foreach(_w ${_ref_wrappers})
    if(NOT EXISTS "${_perm_src}/${_w}")
        message(FATAL_ERROR "cpp-Backend erzeugte Wrapper nicht: ${_perm_src}/${_w} (Profile=${PROFILE}).")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${_ref_perm_src}/${_w}" "${_perm_src}/${_w}"
        RESULT_VARIABLE _dw)
    if(NOT _dw EQUAL 0)
        message(FATAL_ERROR "BYTE-IDENTITAET VERLETZT (perm_src/${_w}, Profile=${PROFILE}).")
    endif()
endforeach()

# ── 5. Byte-Identitaet perm_versions/*.version MODULO der last_codegen-Zeile ───────────────────
file(GLOB _ref_versions RELATIVE "${_ref_perm_versions}" "${_ref_perm_versions}/*.version")
foreach(_v ${_ref_versions})
    if(NOT EXISTS "${_perm_versions}/${_v}")
        message(FATAL_ERROR "cpp-Backend erzeugte .version nicht: ${_perm_versions}/${_v} (Profile=${PROFILE}).")
    endif()
    file(READ "${_ref_perm_versions}/${_v}" _cref)
    file(READ "${_perm_versions}/${_v}" _ccpp)
    # last_codegen-Zeile in BEIDEN neutralisieren (backend-identifizierend).
    string(REGEX REPLACE "last_codegen=[^\r\n]*" "last_codegen=<REDACTED>" _cref "${_cref}")
    string(REGEX REPLACE "last_codegen=[^\r\n]*" "last_codegen=<REDACTED>" _ccpp "${_ccpp}")
    if(NOT _cref STREQUAL _ccpp)
        message(FATAL_ERROR
            "BYTE-IDENTITAET VERLETZT (perm_versions/${_v} ausserhalb last_codegen, Profile=${PROFILE}):\n"
            "  cmake: ${_cref}\n  cpp:   ${_ccpp}")
    endif()
endforeach()

# ── 6. VOLLER-ERSATZ-NACHWEIS: jeder in cpp-permutations.cmake referenzierte perm_src/*.cpp existiert ─
file(STRINGS "${_out}" _addlib_lines REGEX "^add_library\\(perm_.* SHARED ")
list(LENGTH _addlib_lines _n_refs)
set(_n_ref_ok 0)
foreach(_line ${_addlib_lines})
    # Pfad zwischen den Anfuehrungszeichen extrahieren: add_library(... SHARED "<path>")
    string(REGEX REPLACE "^add_library\\(perm_.* SHARED \"([^\"]+)\"\\)$" "\\1" _refpath "${_line}")
    if(NOT EXISTS "${_refpath}")
        message(FATAL_ERROR
            "VOLLER-ERSATZ VERLETZT: von permutations.cmake referenzierte Wrapper-Quelle fehlt: ${_refpath}")
    endif()
    math(EXPR _n_ref_ok "${_n_ref_ok} + 1")
endforeach()

message(STATUS
    "perm_codegen_byte_identity[${PROFILE}]: OK — permutations.cmake + manifest + ${_n_ref_w} perm_src/*.cpp (voll) "
    "+ perm_versions/*.version (modulo last_codegen) byte-identisch (cpp == cmake); "
    "${_n_ref_ok}/${_n_refs} referenzierte Wrapper-Quellen existieren (voller Bau-Ersatz).")
