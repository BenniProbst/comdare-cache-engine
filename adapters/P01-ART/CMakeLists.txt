# V25.C (2026-05-14) — P01-ART Adapter-Skelett
# V31.K1 (2026-05-14) — Inhalt: p01_art_adapter.hpp (UnodbDbAdapter)
# Header-only INTERFACE library; ext/traversal/P01-ART/unodb/ wird optional via
# -DCOMDARE_HAVE_UNODB=ON aktiviert.

option(COMDARE_HAVE_UNODB "Activate ext/traversal/P01-ART/unodb/ (Apache-2.0)" OFF)

add_library(comdare_adapter_p01_art INTERFACE)
add_library(comdare::adapter::p01_art ALIAS comdare_adapter_p01_art)
target_include_directories(comdare_adapter_p01_art INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p01_art INTERFACE cxx_std_23)

if(COMDARE_HAVE_UNODB)
    target_compile_definitions(comdare_adapter_p01_art INTERFACE COMDARE_HAVE_UNODB=1)
    target_include_directories(comdare_adapter_p01_art INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P01-ART/unodb")
    # NOTE: unodb itself is built by ext/traversal/P01-ART/CMakeLists.txt with its
    # own toolchain (Architekt-Direktive 2026-05-08). Linking is wired
    # there, not here.
endif()
