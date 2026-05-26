# V25.C (2026-05-14) — P04-CoCo-trie Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p04_coco_trie_adapter.hpp (CocoTrieAdapter)
# Lizenz GPL-3 -> Architekt-Direktive II 2026-05-14 (NOTICE)
option(COMDARE_HAVE_COCOTRIE "Activate ext/traversal/P04-CoCo-trie/CoCo-trie/ (GPL-3)" OFF)
add_library(comdare_adapter_p04_cocotrie INTERFACE)
add_library(comdare::adapter::p04_cocotrie ALIAS comdare_adapter_p04_cocotrie)
target_include_directories(comdare_adapter_p04_cocotrie INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p04_cocotrie INTERFACE cxx_std_23)
if(COMDARE_HAVE_COCOTRIE)
    target_compile_definitions(comdare_adapter_p04_cocotrie INTERFACE COMDARE_HAVE_COCOTRIE=1)
    target_include_directories(comdare_adapter_p04_cocotrie INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/traversal/P04-CoCo-trie/CoCo-trie")
endif()
