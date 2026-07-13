# V32.JJ.1+ (2026-05-21) - Tests fuer cache_engine_commands_v32
# Bridge zur globalen Test-Discovery via tests/CMakeLists.txt.
#
# Targets:
#   test_commands              (V32.II.2)
#   test_engine_adapters       (V34.A.1)

include(GoogleTest)

# V32-Bit-Bank GETILGT (User-GO 2026-07-13): test_permutation_flags_v32 +
# permutation_flags_v32.hpp + abi_v1_to_v2_mapper.hpp entfernt — die Bit-Bank war
# ein nicht-standalone Import/Export-Feature des B+-Baums; E4-XML ist der
# Goldstandard (ersetzt nichts). cache_engine_commands_v32 bleibt (breitere
# Command-Pattern-Lib, unabhaengig von der Bit-Bank).

# Command-Pattern Smoke + AxisLibraryRegistry + AutoPermutator (6 Tests)
add_executable(test_commands test_commands.cpp)
target_link_libraries(test_commands PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_compile_features(test_commands PRIVATE cxx_std_23)

# V34.A.1 (2026-05-21) - Engine-Adapter Tests
add_executable(test_engine_adapters test_engine_adapters.cpp)
target_link_libraries(test_engine_adapters PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_include_directories(test_engine_adapters PRIVATE
    ${PROJECT_SOURCE_DIR}/libs/cache_engine/include
)
target_compile_features(test_engine_adapters PRIVATE cxx_std_23)

if(COMMAND gtest_discover_tests)
    gtest_discover_tests(test_commands DISCOVERY_TIMEOUT 30)
    gtest_discover_tests(test_engine_adapters DISCOVERY_TIMEOUT 30)
else()
    add_test(NAME test_commands COMMAND test_commands)
    add_test(NAME test_engine_adapters COMMAND test_engine_adapters)
endif()
