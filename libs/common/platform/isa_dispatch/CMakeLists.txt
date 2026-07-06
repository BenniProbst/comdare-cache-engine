# hardware_isa/isa_dispatch — Sub-Komponente
# Skelett (Phase 4.B) — keine Implementation
# #265: Skelett an die vendored foundation-Zellen angeschlossen (Host-Wahrheit aus
# comdare::platform/comdare::simd; die axis_09-Achse bleibt per D2 der autoritative Konsum-Ort).

add_library(comdare_isa_dispatch INTERFACE)
target_include_directories(comdare_isa_dispatch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(comdare_isa_dispatch INTERFACE comdare::platform comdare::simd)
