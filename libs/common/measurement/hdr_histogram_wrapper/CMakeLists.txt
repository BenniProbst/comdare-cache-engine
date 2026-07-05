# measurement/hdr_histogram_wrapper — Sub-Komponente

# CI-Fix (Pipeline 7917): das Root-project() ist CXX-only; die vendored C-Quelle braucht die C-Toolchain
# explizit, sonst bricht der Ninja-/Makefiles-Generator im Configure (CMAKE_C_COMPILE_OBJECT not set —
# 12 Jobs rot am selben Fehler). Der VS-Generator verzieh das lokal. Lokal begrenzt: nur diese Zelle baut C.
enable_language(C)

add_library(comdare_hdr_histogram_wrapper STATIC vendor/hdr_histogram.c)
target_include_directories(comdare_hdr_histogram_wrapper
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/vendor/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set_target_properties(comdare_hdr_histogram_wrapper PROPERTIES LINKER_LANGUAGE C)
# CC0-1.0 (Public Domain) — HdrHistogram_c (Michael Barker), s. NOTICE. is_original.