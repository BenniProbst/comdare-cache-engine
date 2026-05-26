# V25.C (2026-05-14) — P30-HazardPointers Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p30_hazard_pointers_adapter.hpp (HazardPointerAdapter)
option(COMDARE_HAVE_HAZ_PTR "Activate ext/traversal/P30-HazardPointers/haz_ptr/ (no-license)" OFF)
add_library(comdare_adapter_p30_hazardpointers INTERFACE)
add_library(comdare::adapter::p30_hazardpointers ALIAS comdare_adapter_p30_hazardpointers)
target_include_directories(comdare_adapter_p30_hazardpointers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p30_hazardpointers INTERFACE cxx_std_23)
if(COMDARE_HAVE_HAZ_PTR)
    target_compile_definitions(comdare_adapter_p30_hazardpointers INTERFACE COMDARE_HAVE_HAZ_PTR=1)
    target_include_directories(comdare_adapter_p30_hazardpointers INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/traversal/P30-HazardPointers/haz_ptr")
endif()
