# V26.B (2026-05-14) — A04-mimalloc Adapter-Skelett
# V31.K2 (2026-05-14) — Inhalt: a04_mimalloc_adapter.hpp (MimallocAdapter)

option(COMDARE_HAVE_MIMALLOC "Activate ext/A04-mimalloc/ (MIT)" OFF)

add_library(comdare_adapter_a04_mimalloc INTERFACE)
add_library(comdare::adapter::a04_mimalloc ALIAS comdare_adapter_a04_mimalloc)
target_include_directories(comdare_adapter_a04_mimalloc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_a04_mimalloc INTERFACE cxx_std_23)

if(COMDARE_HAVE_MIMALLOC)
    target_compile_definitions(comdare_adapter_a04_mimalloc INTERFACE COMDARE_HAVE_MIMALLOC=1)
    target_include_directories(comdare_adapter_a04_mimalloc INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/A04-mimalloc/include")
endif()
