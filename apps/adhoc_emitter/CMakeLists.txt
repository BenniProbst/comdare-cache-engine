# V41.F.6.1 R5.G — comdare-adhoc-emitter CLI Executable
#
# Configure-Time-Tool (2-Pass-Build, analog comdare-anatomy-codegen-tool): emittiert pro Permutation
# des Pilot-Raums ein kompilierbares Modul-.cpp. Die emittierten .cpp werden anschließend per
# comdare_build_adhoc_modules() (cmake/adhoc_emitter.cmake) als Permutations-DLLs gebaut.

add_executable(comdare_adhoc_emitter_cli main.cpp)
set_target_properties(comdare_adhoc_emitter_cli PROPERTIES
    OUTPUT_NAME "comdare-adhoc-emitter")

# Quell-abgeleitete generierte axis_*_flags.hpp-Dirs (identisch zu tests/unit, ordering-invariant).
file(GLOB _ae_axis_src_dirs LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics/*/axis_*")
set(_ae_axis_gen_dirs "")
foreach(_ae_d ${_ae_axis_src_dirs})
    if(IS_DIRECTORY "${_ae_d}")
        file(RELATIVE_PATH _ae_rel
            "${PROJECT_SOURCE_DIR}/libs/cache_engine/topics" "${_ae_d}")
        list(APPEND _ae_axis_gen_dirs "${PROJECT_BINARY_DIR}/generated/topics/${_ae_rel}")
    endif()
endforeach()

target_include_directories(comdare_adhoc_emitter_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_BINARY_DIR}/generated"
    ${_ae_axis_gen_dirs})

target_link_libraries(comdare_adhoc_emitter_cli PRIVATE Boost::mp11)
target_compile_features(comdare_adhoc_emitter_cli PRIVATE cxx_std_23)

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_adhoc_emitter_cli)
endif()

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle apps). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(comdare_adhoc_emitter_cli)
