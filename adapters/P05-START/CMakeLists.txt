# V25.C (2026-05-14) — P05-START Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p05_start_adapter.hpp (StartAdapter)
option(COMDARE_HAVE_START "Activate ext/traversal/P05-START/START/ (MIT)" OFF)
add_library(comdare_adapter_p05_start INTERFACE)
add_library(comdare::adapter::p05_start ALIAS comdare_adapter_p05_start)
target_include_directories(comdare_adapter_p05_start INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p05_start INTERFACE cxx_std_23)
if(COMDARE_HAVE_START)
    target_compile_definitions(comdare_adapter_p05_start INTERFACE COMDARE_HAVE_START=1)
    target_include_directories(comdare_adapter_p05_start INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P05-START/START")
endif()
