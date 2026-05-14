# V26.B (2026-05-14) — A03-michael-lockfree Adapter-Skelett
add_library(comdare_adapter_a03_michael_lockfree INTERFACE)
add_library(comdare::adapter::a03_michael_lockfree ALIAS comdare_adapter_a03_michael_lockfree)
target_include_directories(comdare_adapter_a03_michael_lockfree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
