# cache_engine/reclamation — Sub-Komponente

add_library(comdare_reclamation INTERFACE)
target_include_directories(comdare_reclamation INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Phase 6 (2026-05-13): eigene RCU-Implementation (#104)
add_subdirectory(rcu_reclaim)
