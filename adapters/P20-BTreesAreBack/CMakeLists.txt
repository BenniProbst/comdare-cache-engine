# V25.C (2026-05-14) — P20-BTreesAreBack Adapter-Skelett
add_library(comdare_adapter_p20_btreesareback INTERFACE)
add_library(comdare::adapter::p20_btreesareback ALIAS comdare_adapter_p20_btreesareback)
target_include_directories(comdare_adapter_p20_btreesareback INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
