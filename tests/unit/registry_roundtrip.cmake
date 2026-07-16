# registry_roundtrip.cmake — WP-4/F29+F64(a) (Voll-Audit 2026-07-16): Round-Trip-Gate der Baustein-
# Registry-XML (Muster R-E/M-CE-27, analog perm_codegen_byte_identity.cmake).
#
# PROBLEM (F29/F64): die committete Registry-XML deklariert sich als GENERIERT ("Reflektion ueber
# Enabled* ist Pflicht (Round-Trip-Garantie)"), aber der Generator war EXCLUDE_FROM_ALL ohne jeden
# ctest/CI-Bezug — eine Aenderung der Enabled*-mp_lists liesse die committete XML STUMM vom Code
# wegdriften (exakt die M-CE-27-Stale-Artefakt-Klasse). Dieses Gate schliesst das Loch:
#   1. Das VOR-gebaute Generator-Tool (GEN_TOOL, $<TARGET_FILE:...>) regeneriert die XML nach WORKDIR.
#   2. Byte-Diff gegen die committete Datei (COMMITTED) — jede Abweichung == FAIL.
# Die committete Datei wird NUR GELESEN (TABU-Wache); geschrieben wird ausschliesslich unter WORKDIR.
#
# REFERENZ-KONFIGURATION (der Byte-Stand der committeten XML ist konfigurations-gebunden, s.
# axis_registry_gen/main.cpp Kopf "flag-parametrisch"): Default-STANDALONE-Konfiguration des Repos
# (Linux g++/Ninja, Default-ENABLE-Flag-Satz, ohne Vendor-HAVE-Opt-ins, ohne --with-extra-axes) —
# dieselbe Konfiguration, unter der die committete XML erzeugt wurde (INC-A 2026-07-14, byte-
# verifiziert 2026-07-16) und unter der die CI-Gates (contract:profile_coverage) konfigurieren.
# Wer mit abweichenden ENABLE-/HAVE-Flags baut, aendert das ENABLED-Inventar → der Diff schlaegt
# ehrlich an (kein Fehler des Gates, sondern der Drift-Beweis).
#
# Erwartete -D Variablen: GEN_TOOL (gebautes Generator-Executable), COMMITTED (committete XML),
# WORKDIR (Scratch-Verzeichnis), optional GEN_ARGS (Zusatz-Argumente, z.B. --with-extra-axes).

foreach(_req GEN_TOOL COMMITTED WORKDIR)
    if(NOT DEFINED ${_req})
        message(FATAL_ERROR "registry_roundtrip: -D${_req} fehlt.")
    endif()
endforeach()

if(NOT EXISTS "${GEN_TOOL}")
    message(FATAL_ERROR
        "registry_roundtrip: Generator-Tool nicht gebaut: ${GEN_TOOL}\n"
        "Vorher bauen (2-Pass, wie #25-B): cmake --build <build> --target <generator-target>.")
endif()
if(NOT EXISTS "${COMMITTED}")
    message(FATAL_ERROR "registry_roundtrip: committete Registry-XML fehlt: ${COMMITTED}")
endif()

# Frisches Arbeitsverzeichnis (deterministisch); NUR hier wird geschrieben.
file(REMOVE_RECURSE "${WORKDIR}")
file(MAKE_DIRECTORY "${WORKDIR}")
set(_regen "${WORKDIR}/regenerated_registry.xml")

execute_process(
    COMMAND "${GEN_TOOL}" --out "${_regen}" ${GEN_ARGS}
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _log
    ERROR_VARIABLE  _log_err)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "registry_roundtrip: Generator fehlgeschlagen (rc=${_rc}):\n${_log}\n${_log_err}")
endif()
message(STATUS "Generator-Ausgabe:\n${_log}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${_regen}" "${COMMITTED}"
    RESULT_VARIABLE _diff)
if(NOT _diff EQUAL 0)
    # Fuer die Diagnose den Unterschied zeigen (diff ist auf den CI-Runnern/Linux vorhanden; bei
    # Fehlen bleibt der FATAL_ERROR mit beiden Pfaden der Beleg).
    execute_process(COMMAND diff -u "${COMMITTED}" "${_regen}" OUTPUT_VARIABLE _diff_out ERROR_QUIET)
    message(FATAL_ERROR
        "registry_roundtrip: BYTE-DRIFT — die committete Registry-XML entspricht NICHT der Reflektion "
        "des aktuellen Codes (Enabled*-Listen geaendert ohne Regeneration? Konfiguration != Referenz?).\n"
        "  committet:   ${COMMITTED}\n"
        "  regeneriert: ${_regen}\n"
        "Fix: Generator laufen lassen + XML mit-committen (NIE von Hand editieren).\n${_diff_out}")
endif()
message(STATUS "registry_roundtrip: Byte-Diff == 0 — committete XML == Code-Reflektion.")
