# tools/best_binary_selector — #172 Versprechen (1): automatische Auswahl + Versand der besten Binary.
#
# Registriert das vorher NUR via ad-hoc build_and_run.bat baubare Werkzeug als reguläres CMake-Target,
# damit es Teil des reproduzierbaren Builds (G5: frischer git-clone baut identisch) ist. Self-contained
# C++17-Werkzeug (nur Standardbibliothek + <filesystem>): KEINE Engine-/Boost-/generated-Abhängigkeit,
# daher hier als schlankes add_executable (analog tools/ycsb_cli, tools/p27_bundle_finder).
add_executable(comdare_best_binary_selector
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector/best_binary_selector.cpp"
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector/best_binary_selector_main.cpp")
target_include_directories(comdare_best_binary_selector PRIVATE
    "${CMAKE_SOURCE_DIR}/libs/cache_engine/builder/best_binary_selector")
target_compile_features(comdare_best_binary_selector PRIVATE cxx_std_23)
set_target_properties(comdare_best_binary_selector PROPERTIES OUTPUT_NAME "best_binary_selector")
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_best_binary_selector)
endif()
