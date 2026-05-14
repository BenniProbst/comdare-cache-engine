# V25.C (2026-05-14) — P30-HazardPointers Adapter-Skelett
add_library(comdare_adapter_p30_hazardpointers INTERFACE)
add_library(comdare::adapter::p30_hazardpointers ALIAS comdare_adapter_p30_hazardpointers)
target_include_directories(comdare_adapter_p30_hazardpointers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
