# prt_art — Sub-Komponente

add_library(comdare_prt_art INTERFACE)
target_include_directories(comdare_prt_art INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Phase 6+ (2026-05-13): 14 LEGACY_REIMPL Skelette ausgefuellt
add_subdirectory(legacy_reimpl)
