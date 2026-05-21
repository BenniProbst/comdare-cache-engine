# V32.JJ.1 (2026-05-18 spaet) - cache-engine Commands Header-only Module
# Bridge zwischen V31.F-Stand und V32-Command-Pattern (AA.2 Korrektur)

add_library(cache_engine_commands_v32 INTERFACE)
target_include_directories(cache_engine_commands_v32 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(cache_engine_commands_v32 INTERFACE cxx_std_23)

# Optional: V32_ENABLE Compile-Flag (default OFF, V31.F-Code bleibt unveraendert)
option(COMDARE_V32_ENABLE "Enable V32 Command-Pattern + AutoPermutator + 14-Banks" OFF)
if(COMDARE_V32_ENABLE)
    target_compile_definitions(cache_engine_commands_v32 INTERFACE COMDARE_V32_ENABLE=1)
endif()

# V32.JJ.1 (2026-05-21) - Tests aktiviert (II.2 + II.4 Welch-T-Test)
if(COMDARE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
