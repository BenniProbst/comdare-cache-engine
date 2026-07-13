# V25.C (2026-05-14) — P25-Mahling Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p25_mahling_adapter.hpp (FillBufferProbe)
option(COMDARE_HAVE_MAHLING "Activate ext/traversal/P25-Mahling/prefetching/ (no-license)" OFF)
add_library(comdare_adapter_p25_mahling INTERFACE)
add_library(comdare::adapter::p25_mahling ALIAS comdare_adapter_p25_mahling)
target_include_directories(comdare_adapter_p25_mahling INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p25_mahling INTERFACE cxx_std_23)
if(COMDARE_HAVE_MAHLING)
    target_compile_definitions(comdare_adapter_p25_mahling INTERFACE COMDARE_HAVE_MAHLING=1)
    target_include_directories(comdare_adapter_p25_mahling INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P25-Mahling/prefetching/src")
endif()
