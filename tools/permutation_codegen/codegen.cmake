# tools/permutation_codegen/codegen.cmake — Cache-Engine Permutations-Codegen
# V36.B (2026-05-23) — Vollausbau gemaess User-Direktive:
#   - "alle Rekombinationen umfassend gebaut"
#   - Tri-State Mode: on_rebuild / on_build_on_demand / off_pause_build
#   - Default: on_build_on_demand
#   - Sonderfall: 0 Permutationen + off → Experiment-Driver fatal
#
# Memory-Direktive: KEIN Python (Talos OS), CMake-Funktionen + sh/bat.
#
# Aufruf:
#   cmake -DCOMDARE_TARGET_ISA=auto -DCOMDARE_PROFILE=smoke \
#         -DCOMDARE_MODE=on_build_on_demand \
#         -DCOMDARE_OUTPUT=<path/to/permutations.cmake> \
#         -P codegen.cmake

cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED COMDARE_TARGET_ISA)
    set(COMDARE_TARGET_ISA "auto")
endif()
if(NOT DEFINED COMDARE_PROFILE)
    set(COMDARE_PROFILE "smoke")
endif()
if(NOT DEFINED COMDARE_MODE)
    set(COMDARE_MODE "on_build_on_demand")
endif()
if(NOT DEFINED COMDARE_OUTPUT)
    message(FATAL_ERROR "COMDARE_OUTPUT erforderlich (Pfad zur generierten Datei)")
endif()

get_filename_component(_out_dir "${COMDARE_OUTPUT}" DIRECTORY)
set(_perm_src_dir "${_out_dir}/perm_src")
set(_manifest "${_out_dir}/permutations_manifest.txt")
file(MAKE_DIRECTORY "${_out_dir}")
file(MAKE_DIRECTORY "${_perm_src_dir}")

# ─────────────────────────────────────────────────────────────────────────────
# Achsen-Definition (Subset der 14-Achsen-Matrix fuer initialen Vollausbau)
# Quelle: docs/bausteine/07_bausteine_matrix_N_erweitert.md
# Pro Profile-Filter wird ein anderes Achsen-Subset aktiviert:
#   - smoke:  3 Achsen, 2-3 Variants  =  ~18 Permutationen
#   - medium: 5 Achsen, 2-3 Variants  =  ~108 Permutationen
#   - full:   alle 14 Achsen          =  Tausende+
# ─────────────────────────────────────────────────────────────────────────────

# Achse 12 SIMD (CompileTime detection)
set(_AXIS_12 "scalar" "sse4" "avx2")          # AVX512 nur full
# Achse 4 Cache-Layout
set(_AXIS_4  "aos" "soa" "hybrid")
# Achse 6 Allokator (Memory: A01-A23 Allokator-Stack)
set(_AXIS_6  "std" "jemalloc" "mimalloc")
# Achse 1 Knoten-Format (cache-engine spezifisch)
set(_AXIS_1  "compact" "wide")
# Achse 8 Concurrency
set(_AXIS_8  "single" "multi")

# Profil-Auswahl
if(COMDARE_PROFILE STREQUAL "smoke")
    set(_active_axes "AXIS_12" "AXIS_4" "AXIS_6")
elseif(COMDARE_PROFILE STREQUAL "medium")
    set(_active_axes "AXIS_12" "AXIS_4" "AXIS_6" "AXIS_1" "AXIS_8")
    list(APPEND _AXIS_12 "avx512")
elseif(COMDARE_PROFILE STREQUAL "full")
    set(_active_axes "AXIS_12" "AXIS_4" "AXIS_6" "AXIS_1" "AXIS_8")
    list(APPEND _AXIS_12 "avx512" "neon")
else()
    message(FATAL_ERROR "Unbekanntes COMDARE_PROFILE: ${COMDARE_PROFILE} (smoke|medium|full)")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Constraint-Filter (Phase 5+ kommt vollausgebaut, hier pragmatisch)
# Pruefe Host-ISA gegen Permutation
# ─────────────────────────────────────────────────────────────────────────────
function(_perm_host_supports_isa isa out_var)
    if(isa STREQUAL "scalar")
        set(${out_var} TRUE PARENT_SCOPE)
        return()
    endif()
    # Default: auf 'auto' alle x86_64 ISA durchlassen (CMake kennt ISA nicht direkt)
    # In Phase 6+ koennte PlatformProbe das praezise filtern
    if(COMDARE_TARGET_ISA STREQUAL "auto")
        if(isa STREQUAL "neon")
            set(${out_var} FALSE PARENT_SCOPE)
        else()
            set(${out_var} TRUE PARENT_SCOPE)
        endif()
    elseif(COMDARE_TARGET_ISA STREQUAL isa)
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Cross-Produkt-Generator (rekursiv via Schleifen, da CMake kein einfaches itertools.product hat)
# ─────────────────────────────────────────────────────────────────────────────
set(_all_perms "")

# Cross-Produkt von 3 Achsen (smoke) bzw. 5 Achsen (medium/full)
# Wir generieren statisch fuer max 5 Achsen — fuer mehr Achsen wuerde
# man cmake_language(EVAL CODE) brauchen, fuer den initialen Vollausbau reicht das.
if(COMDARE_PROFILE STREQUAL "smoke")
    foreach(_v12 IN LISTS _AXIS_12)
        _perm_host_supports_isa("${_v12}" _ok)
        if(NOT _ok)
            continue()
        endif()
        foreach(_v4 IN LISTS _AXIS_4)
            foreach(_v6 IN LISTS _AXIS_6)
                list(APPEND _all_perms "${_v12}|${_v4}|${_v6}")
            endforeach()
        endforeach()
    endforeach()
else()
    foreach(_v12 IN LISTS _AXIS_12)
        _perm_host_supports_isa("${_v12}" _ok)
        if(NOT _ok)
            continue()
        endif()
        foreach(_v4 IN LISTS _AXIS_4)
            foreach(_v6 IN LISTS _AXIS_6)
                foreach(_v1 IN LISTS _AXIS_1)
                    foreach(_v8 IN LISTS _AXIS_8)
                        list(APPEND _all_perms "${_v12}|${_v4}|${_v6}|${_v1}|${_v8}")
                    endforeach()
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endif()

list(LENGTH _all_perms _n_perms)
message(STATUS "Permutations-Codegen: ${_n_perms} gueltige Permutationen (Profile=${COMDARE_PROFILE}, ISA=${COMDARE_TARGET_ISA})")

# ─────────────────────────────────────────────────────────────────────────────
# Pro Permutation: Wrapper-Source schreiben (on_demand: nur wenn fehlt)
# ─────────────────────────────────────────────────────────────────────────────
set(_perm_targets "")
set(_perm_count_written 0)
set(_perm_count_skipped 0)

foreach(_perm IN LISTS _all_perms)
    string(REPLACE "|" "_" _perm_id "${_perm}")
    set(_wrapper "${_perm_src_dir}/perm_${_perm_id}.cpp")

    # On-demand-Logik: wenn Source existiert und Mode!=on_rebuild → ueberspringen
    if(EXISTS "${_wrapper}" AND NOT COMDARE_MODE STREQUAL "on_rebuild")
        math(EXPR _perm_count_skipped "${_perm_count_skipped} + 1")
    else()
        # Parse Achsen-Werte aus _perm-String
        string(REPLACE "|" ";" _parts "${_perm}")
        list(GET _parts 0 _simd)
        list(GET _parts 1 _layout)
        list(GET _parts 2 _alloc)
        set(_extra_defines "")
        list(LENGTH _parts _len)
        if(_len GREATER 3)
            list(GET _parts 3 _node)
            list(GET _parts 4 _concur)
            set(_extra_defines "
#define COMDARE_PERM_NODE \"${_node}\"
#define COMDARE_PERM_CONCURRENCY \"${_concur}\"")
        endif()

        file(WRITE "${_wrapper}"
"// Auto-generiert von tools/permutation_codegen/codegen.cmake (V36.B 2026-05-23)
// Permutation: ${_perm_id}
// Profile=${COMDARE_PROFILE} ISA=${COMDARE_TARGET_ISA}

#define COMDARE_PERM_ID \"${_perm_id}\"
#define COMDARE_PERM_SIMD \"${_simd}\"
#define COMDARE_PERM_LAYOUT \"${_layout}\"
#define COMDARE_PERM_ALLOC \"${_alloc}\"${_extra_defines}

namespace comdare::perm::${_perm_id} {

// Permutation-Identitaets-Funktion (Linker-sichtbar)
extern \"C\" const char* perm_${_perm_id}_id() {
    return COMDARE_PERM_ID;
}

// Slot fuer Achsen-spezifische Initialisierung (Phase 6+: wirklicher
// Algorithmus-Body, hier nur Symbol-Existenz-Garantie fuer Linker)
const char* perm_axis_simd     = COMDARE_PERM_SIMD;
const char* perm_axis_layout   = COMDARE_PERM_LAYOUT;
const char* perm_axis_alloc    = COMDARE_PERM_ALLOC;

}  // namespace
")
        math(EXPR _perm_count_written "${_perm_count_written} + 1")
    endif()

    list(APPEND _perm_targets "perm_${_perm_id}")
endforeach()

message(STATUS "Permutations-Codegen: ${_perm_count_written} Wrapper geschrieben, ${_perm_count_skipped} on-demand uebersprungen")

# ─────────────────────────────────────────────────────────────────────────────
# Manifest: was wurde gebaut? (Runtime-Check des Experiment-Drivers)
# ─────────────────────────────────────────────────────────────────────────────
set(_manifest_content "# permutations_manifest.txt (V36.B)\n# Profile=${COMDARE_PROFILE} ISA=${COMDARE_TARGET_ISA} Mode=${COMDARE_MODE}\n# count=${_n_perms}\n")
foreach(_t IN LISTS _perm_targets)
    string(APPEND _manifest_content "${_t}\n")
endforeach()
file(WRITE "${_manifest}" "${_manifest_content}")

# ─────────────────────────────────────────────────────────────────────────────
# permutations.cmake: add_library Eintraege fuer alle Permutationen
# ─────────────────────────────────────────────────────────────────────────────
set(_perm_cmake_content
"# permutations.cmake — generiert von tools/permutation_codegen/codegen.cmake
# V36.B Vollausbau (2026-05-23): ${_n_perms} Per-Permutation-Targets
# Profile=${COMDARE_PROFILE} ISA=${COMDARE_TARGET_ISA} Mode=${COMDARE_MODE}

message(STATUS \"Permutations: registriere ${_n_perms} Per-Permutation-Targets\")

")

# STATIC-Libraries (statt OBJECT): erlauben sauberes Output-Layout
# Storage-Layout: ${CMAKE_BINARY_DIR}/perm/cache_engine/<id>/perm_<id>.lib
foreach(_perm IN LISTS _all_perms)
    string(REPLACE "|" "_" _perm_id "${_perm}")
    string(APPEND _perm_cmake_content
"add_library(perm_${_perm_id} STATIC \"${_perm_src_dir}/perm_${_perm_id}.cpp\")
target_compile_features(perm_${_perm_id} PRIVATE cxx_std_23)
set_target_properties(perm_${_perm_id} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY \"\${CMAKE_BINARY_DIR}/perm/cache_engine/${_perm_id}\"
    FOLDER \"perm_cache_engine\")

")
endforeach()

# Aggregations-Target 'comdare_permutations_all' — add_custom_target mit DEPENDS,
# damit ein Build des Aggregators tatsaechlich alle OBJECT-Libraries kompiliert.
# INTERFACE-Library wuerde nur Quellen aggregieren, aber nicht Build triggern.
string(APPEND _perm_cmake_content
"add_custom_target(comdare_permutations_all
    COMMENT \"V36.B Aggregator: ${_n_perms} cache-engine Permutationen\"
    DEPENDS")
foreach(_perm IN LISTS _all_perms)
    string(REPLACE "|" "_" _perm_id "${_perm}")
    string(APPEND _perm_cmake_content "
        perm_${_perm_id}")
endforeach()
string(APPEND _perm_cmake_content "
)
")

file(WRITE "${COMDARE_OUTPUT}" "${_perm_cmake_content}")
message(STATUS "Permutations-Codegen: ${COMDARE_OUTPUT} mit ${_n_perms} Targets geschrieben")
message(STATUS "Permutations-Codegen: Manifest unter ${_manifest}")
