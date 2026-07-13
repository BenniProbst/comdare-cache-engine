# V25.C (2026-05-14) — P07-Wormhole Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p07_wormhole_adapter.hpp (WormholeAdapter)
# Lizenz GPL-3 -> Architekt-Direktive II 2026-05-14 (NOTICE)
option(COMDARE_HAVE_WORMHOLE "Activate ext/traversal/P07-Wormhole/wormhole/ (GPL-3)" OFF)
add_library(comdare_adapter_p07_wormhole INTERFACE)
add_library(comdare::adapter::p07_wormhole ALIAS comdare_adapter_p07_wormhole)
target_include_directories(comdare_adapter_p07_wormhole INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p07_wormhole INTERFACE cxx_std_23)
if(COMDARE_HAVE_WORMHOLE)
    target_compile_definitions(comdare_adapter_p07_wormhole INTERFACE COMDARE_HAVE_WORMHOLE=1)
    target_include_directories(comdare_adapter_p07_wormhole INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P07-Wormhole/wormhole")
endif()
