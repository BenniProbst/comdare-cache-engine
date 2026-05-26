# V41.F.6.1.P2.A is_original_codegen Module
#
# Ruft apps/is_original_validator zur Build-Time auf und generiert pro Paper-Wrapper
# einen Header mit kIsOriginal_<wrapper_fn> constexpr-Bools + PaperManifest-Struct
# + OriginalCodeMixin-Alias.
#
# Konsumiert wird das Resultat im Wrapper-Header via Inheritance:
#   class MimallocAllocator : public generated::a04_mimalloc::OriginalCodeMixin { ... };
#
# @reference Memory [[legacy-code-sha256-validation]] [[compile-time-only-no-runtime]]
# @reference docs/architektur/13_paper_legacy_code_architektur.md §13

#
# Generiert pro Wrapper EINEN Header mit allen kIsOriginal_<fn> + PaperManifest + Mixin-Alias.
#
# Args:
#   WRAPPER_NAME       — z.B. "mimalloc" (fuer Logging)
#   PAPER_ID           — z.B. "a04_mimalloc" (Verzeichnis paper_<id>)
#   LEGACY_CODE_DIR    — Pfad zu legacy_code/paper_<id>/
#   OUTPUT_HEADER      — Ziel-Header-Pfad (typisch ${CMAKE_BINARY_DIR}/generated/...)
#   NAMESPACE          — z.B. "generated::a04_mimalloc"
#   AXIS_MIXIN_TYPE    — fully-qualified: "::comdare::cache_engine::allocator::axis_06_allocator::concepts::AllocatorOriginalCodeMixin"
function(comdare_generate_is_original_mixin)
    set(_options)
    # AXIS_MIXIN_TYPE bleibt akzeptiert (Backward-Compat-Hint), wird aber aus manifest.txt
    # @axis_mixin_type Annotation gelesen — Tool akzeptiert kein CLI-Arg dafuer.
    set(_one_value WRAPPER_NAME PAPER_ID LEGACY_CODE_DIR OUTPUT_HEADER NAMESPACE AXIS_MIXIN_TYPE)
    set(_multi_value)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg WRAPPER_NAME PAPER_ID LEGACY_CODE_DIR OUTPUT_HEADER NAMESPACE)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_generate_is_original_mixin: ${_arg} required")
        endif()
    endforeach()

    set(_manifest_path "${ARG_LEGACY_CODE_DIR}/manifest.txt")
    if(NOT EXISTS "${_manifest_path}")
        message(FATAL_ERROR "comdare_generate_is_original_mixin: manifest not found at ${_manifest_path}")
    endif()

    # Lock-File ([[legacy-code-sha256-validation]] User-Direktive P2.A0.5):
    # Tool berechnet auto sha256_locked.txt beim ersten Lauf, User committed.
    set(_lock_file "${ARG_LEGACY_CODE_DIR}/sha256_locked.txt")

    # Sicherstellen, dass Output-Verzeichnis existiert.
    get_filename_component(_output_dir "${ARG_OUTPUT_HEADER}" DIRECTORY)
    file(MAKE_DIRECTORY "${_output_dir}")

    add_custom_command(
        OUTPUT "${ARG_OUTPUT_HEADER}"
        COMMAND $<TARGET_FILE:is_original_validator>
                --manifest  "${_manifest_path}"
                --base-dir  "${ARG_LEGACY_CODE_DIR}"
                --lock-file "${_lock_file}"
                --output    "${ARG_OUTPUT_HEADER}"
                --namespace "${ARG_NAMESPACE}"
        DEPENDS is_original_validator "${_manifest_path}"
        COMMENT "comdare is_original_codegen: ${ARG_WRAPPER_NAME} (paper_${ARG_PAPER_ID})"
        VERBATIM
    )

    message(STATUS "comdare is_original_codegen: registered ${ARG_WRAPPER_NAME} -> ${ARG_OUTPUT_HEADER}")
endfunction()

#
# comdare_paper_init: Initialisiert legacy_code/paper_<id>/src+include aus einer
# unkuratierten Source-Distribution (z.B. ext/<vendor>/). User-Direktive 2026-05-26:
# Original-Quelle bleibt unangetastet (User hat nur EINE Kopie), Cache-Engine bekommt
# eigene Kopie zur kuratierten Pflege.
#
# Idempotenz via .extracted.marker. Erstmal nur File-Liste; spaeter ARCHIVE_EXTRACT support.
#
# Args:
#   PAPER_ID         — z.B. "a04_mimalloc" (definiert legacy_code/paper_<id>/)
#   EXT_SOURCE_DIR   — Quell-Verzeichnis (z.B. ${CMAKE_SOURCE_DIR}/ext/A04-mimalloc/)
#   LEGACY_CODE_DIR  — Ziel-Verzeichnis (z.B. ${CMAKE_SOURCE_DIR}/libs/.../legacy_code/paper_a04_mimalloc/)
#   FILES            — relative Pfade zu kopierenden Files (multi-value), z.B. src/alloc.c
function(comdare_paper_init)
    set(_options)
    set(_one_value PAPER_ID EXT_SOURCE_DIR LEGACY_CODE_DIR)
    set(_multi_value FILES)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg PAPER_ID EXT_SOURCE_DIR LEGACY_CODE_DIR FILES)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_paper_init: ${_arg} required")
        endif()
    endforeach()

    set(_marker "${ARG_LEGACY_CODE_DIR}/.extracted.marker")
    if(EXISTS "${_marker}")
        message(STATUS "comdare paper-init HIT: ${ARG_PAPER_ID} already extracted")
        return()
    endif()

    if(NOT EXISTS "${ARG_EXT_SOURCE_DIR}")
        message(FATAL_ERROR "comdare_paper_init: EXT_SOURCE_DIR not found at ${ARG_EXT_SOURCE_DIR}")
    endif()

    message(STATUS "comdare paper-init: copying ${ARG_PAPER_ID} from ${ARG_EXT_SOURCE_DIR}")

    foreach(_rel_file IN LISTS ARG_FILES)
        set(_src "${ARG_EXT_SOURCE_DIR}/${_rel_file}")
        set(_dst "${ARG_LEGACY_CODE_DIR}/${_rel_file}")
        if(NOT EXISTS "${_src}")
            message(FATAL_ERROR "comdare paper-init: source file missing: ${_src}")
        endif()
        get_filename_component(_dst_dir "${_dst}" DIRECTORY)
        file(MAKE_DIRECTORY "${_dst_dir}")
        configure_file("${_src}" "${_dst}" COPYONLY)
    endforeach()

    file(TOUCH "${_marker}")
    list(LENGTH ARG_FILES _n_files)
    message(STATUS "comdare paper-init READY: ${ARG_PAPER_ID} (${_n_files} files)")
endfunction()
