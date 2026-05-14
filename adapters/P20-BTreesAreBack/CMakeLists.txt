# V25.C (2026-05-14) — P20-BTreesAreBack Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p20_leanstore_adapter.hpp (LeanStoreAdapter)
option(COMDARE_HAVE_LEANSTORE "Activate ext/P20-BTreesAreBack/leanstore/ (MIT)" OFF)
add_library(comdare_adapter_p20_btreesareback INTERFACE)
add_library(comdare::adapter::p20_btreesareback ALIAS comdare_adapter_p20_btreesareback)
target_include_directories(comdare_adapter_p20_btreesareback INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p20_btreesareback INTERFACE cxx_std_23)
if(COMDARE_HAVE_LEANSTORE)
    target_compile_definitions(comdare_adapter_p20_btreesareback INTERFACE COMDARE_HAVE_LEANSTORE=1)
    target_include_directories(comdare_adapter_p20_btreesareback INTERFACE
        "${CMAKE_SOURCE_DIR}/ext/P20-BTreesAreBack/leanstore/backend")
endif()
