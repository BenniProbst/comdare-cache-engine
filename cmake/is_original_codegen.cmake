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
    set(_one_value WRAPPER_NAME PAPER_ID LEGACY_CODE_DIR OUTPUT_HEADER NAMESPACE AXIS_MIXIN_TYPE)
    set(_multi_value)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    foreach(_arg WRAPPER_NAME PAPER_ID LEGACY_CODE_DIR OUTPUT_HEADER NAMESPACE AXIS_MIXIN_TYPE)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "comdare_generate_is_original_mixin: ${_arg} required")
        endif()
    endforeach()

    set(_manifest_path "${ARG_LEGACY_CODE_DIR}/manifest.txt")
    if(NOT EXISTS "${_manifest_path}")
        message(FATAL_ERROR "comdare_generate_is_original_mixin: manifest not found at ${_manifest_path}")
    endif()

    # Lock-File-Tracking ([[legacy-code-sha256-validation]] User-Direktive P2.A0.5):
    # Tool berechnet auto sha256_locked.txt beim ersten Lauf.
    set(_lock_file "${ARG_LEGACY_CODE_DIR}/sha256_locked.txt")

    # Sicherstellen, dass Output-Verzeichnis existiert.
    get_filename_component(_output_dir "${ARG_OUTPUT_HEADER}" DIRECTORY)
    file(MAKE_DIRECTORY "${_output_dir}")

    # add_custom_command: Tool wird ausgefuehrt wenn manifest/locked/sources sich aendern.
    # Tool selbst (apps/is_original_validator) ist normales CMake-Target.
    add_custom_command(
        OUTPUT "${ARG_OUTPUT_HEADER}"
        COMMAND $<TARGET_FILE:is_original_validator>
                --manifest        "${_manifest_path}"
                --base-dir        "${ARG_LEGACY_CODE_DIR}"
                --output          "${ARG_OUTPUT_HEADER}"
                --namespace       "${ARG_NAMESPACE}"
                --axis-mixin-type "${ARG_AXIS_MIXIN_TYPE}"
        DEPENDS is_original_validator "${_manifest_path}"
        COMMENT "comdare is_original_codegen: ${ARG_WRAPPER_NAME} (paper_${ARG_PAPER_ID})"
        VERBATIM
    )

    message(STATUS "comdare is_original_codegen: registered ${ARG_WRAPPER_NAME} -> ${ARG_OUTPUT_HEADER}")
endfunction()
