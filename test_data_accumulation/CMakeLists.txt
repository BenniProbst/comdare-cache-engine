# test_data_accumulation - Phase 6.5 (REV 7 §7)

add_library(comdare_test_data_accumulation INTERFACE)
add_library(comdare::test_data_accumulation ALIAS comdare_test_data_accumulation)
target_include_directories(comdare_test_data_accumulation INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(comdare_test_data_accumulation INTERFACE cxx_std_23)
