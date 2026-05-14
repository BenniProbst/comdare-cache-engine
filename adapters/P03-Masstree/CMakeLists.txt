# V25.C (2026-05-14) — P03-Masstree Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p03_masstree_adapter.hpp (MasstreeAdapter)
option(COMDARE_HAVE_MASSTREE "Activate ext/P03-Masstree/masstree-beta/ (MIT)" OFF)
add_library(comdare_adapter_p03_masstree INTERFACE)
add_library(comdare::adapter::p03_masstree ALIAS comdare_adapter_p03_masstree)
target_include_directories(comdare_adapter_p03_masstree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p03_masstree INTERFACE cxx_std_23)
if(COMDARE_HAVE_MASSTREE)
    target_compile_definitions(comdare_adapter_p03_masstree INTERFACE COMDARE_HAVE_MASSTREE=1)
    target_include_directories(comdare_adapter_p03_masstree INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/P03-Masstree/masstree-beta")
endif()
