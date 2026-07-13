# V25.C (2026-05-14) — P02-HOT Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p02_hot_adapter.hpp (HotAdapter)
option(COMDARE_HAVE_HOT "Activate ext/traversal/P02-HOT/hot/ (ISC)" OFF)
add_library(comdare_adapter_p02_hot INTERFACE)
add_library(comdare::adapter::p02_hot ALIAS comdare_adapter_p02_hot)
target_include_directories(comdare_adapter_p02_hot INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p02_hot INTERFACE cxx_std_23)
if(COMDARE_HAVE_HOT)
    target_compile_definitions(comdare_adapter_p02_hot INTERFACE COMDARE_HAVE_HOT=1)
    target_include_directories(comdare_adapter_p02_hot INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P02-HOT/hot")
endif()
