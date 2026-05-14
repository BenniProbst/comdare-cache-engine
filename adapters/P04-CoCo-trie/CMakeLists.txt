# V25.C (2026-05-14) — P04-CoCo-trie Adapter-Skelett
add_library(comdare_adapter_p04_cocotrie INTERFACE)
add_library(comdare::adapter::p04_cocotrie ALIAS comdare_adapter_p04_cocotrie)
target_include_directories(comdare_adapter_p04_cocotrie INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
