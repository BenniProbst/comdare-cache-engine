# V26.B/V31.K4 (2026-05-14) — A03-michael-lockfree Adapter
option(COMDARE_HAVE_MICHAEL "Activate ext/allocator/A03-michael-lockfree/ (LGPL-2.1-or-later, IBM patent applies)" OFF)
add_library(comdare_adapter_a03_michael_lockfree INTERFACE)
add_library(comdare::adapter::a03_michael_lockfree ALIAS comdare_adapter_a03_michael_lockfree)
target_include_directories(comdare_adapter_a03_michael_lockfree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a03_michael_lockfree INTERFACE cxx_std_23)
if(COMDARE_HAVE_MICHAEL)
    target_compile_definitions(comdare_adapter_a03_michael_lockfree INTERFACE COMDARE_HAVE_MICHAEL=1)
    target_include_directories(comdare_adapter_a03_michael_lockfree INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/allocator/A03-michael-lockfree")
endif()
