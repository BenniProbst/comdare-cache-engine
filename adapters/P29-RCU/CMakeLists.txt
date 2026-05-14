# V25.C (2026-05-14) — P29-RCU Adapter-Skelett
add_library(comdare_adapter_p29_rcu INTERFACE)
add_library(comdare::adapter::p29_rcu ALIAS comdare_adapter_p29_rcu)
target_include_directories(comdare_adapter_p29_rcu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
