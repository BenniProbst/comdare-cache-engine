# V32.JJ.1+ (2026-05-21) - Tests fuer cache_engine_commands_v32
# Bridge zur globalen Test-Discovery via tests/CMakeLists.txt.
#
# Targets:
#   test_permutation_flags_v32 (V32.II.1)
#   test_commands              (V32.II.2)

include(GoogleTest)

# Test 1: PermutationFlags V32 Sub-Bank Roundtrips (10 Tests)
add_executable(test_permutation_flags_v32 test_permutation_flags_v32.cpp)
target_link_libraries(test_permutation_flags_v32 PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_include_directories(test_permutation_flags_v32 PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/cache_engine/include
)
target_compile_features(test_permutation_flags_v32 PRIVATE cxx_std_23)

# Test 2: Command-Pattern Smoke + AxisLibraryRegistry + AutoPermutator (6 Tests)
add_executable(test_commands test_commands.cpp)
target_link_libraries(test_commands PRIVATE
    cache_engine_commands_v32
    GTest::gtest_main
)
target_compile_features(test_commands PRIVATE cxx_std_23)

if(COMMAND gtest_discover_tests)
    gtest_discover_tests(test_permutation_flags_v32 DISCOVERY_TIMEOUT 30)
    gtest_discover_tests(test_commands DISCOVERY_TIMEOUT 30)
else()
    add_test(NAME test_permutation_flags_v32 COMMAND test_permutation_flags_v32)
    add_test(NAME test_commands COMMAND test_commands)
endif()
