# V25.C (2026-05-14) — P01-ART Adapter-Skelett
# Header-only INTERFACE library; Inhalt folgt nach ext/P01-ART/unodb-Aktivierung
# (siehe V21.1 art_body.hpp.template HAVE_UNODB).

add_library(comdare_adapter_p01_art INTERFACE)
add_library(comdare::adapter::p01_art ALIAS comdare_adapter_p01_art)
target_include_directories(comdare_adapter_p01_art INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
