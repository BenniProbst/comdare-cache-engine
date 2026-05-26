# V25.C (2026-05-14) — P10-SuRF Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p10_surf_adapter.hpp (SurfAdapter)
option(COMDARE_HAVE_SURF "Activate ext/traversal/P10-SuRF/SuRF/ (Apache-2.0)" OFF)
add_library(comdare_adapter_p10_surf INTERFACE)
add_library(comdare::adapter::p10_surf ALIAS comdare_adapter_p10_surf)
target_include_directories(comdare_adapter_p10_surf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p10_surf INTERFACE cxx_std_23)
if(COMDARE_HAVE_SURF)
    target_compile_definitions(comdare_adapter_p10_surf INTERFACE COMDARE_HAVE_SURF=1)
    target_include_directories(comdare_adapter_p10_surf INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/traversal/P10-SuRF/SuRF/include")
endif()
