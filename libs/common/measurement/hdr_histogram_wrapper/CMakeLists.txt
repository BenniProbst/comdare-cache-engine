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
# CC0-1.0 (Public Domain) — HdrHistogram_c (Michael Barker), s. NOTICE.
# Provenienz-Korrektur (M-CE-17): dies ist eine VENDORED copy, NICHT byte-identisch
# zu Upstream und KEINE eigene Re-Impl. Der vendored vendor/hdr_histogram.c enthaelt
# comdare-Modifikationen (AVX2-Runtime-Dispatch, Prefetch-/Likely-Hints). Der frueher
# hier gefuehrte Marker "is_original" (= unmodifiziertes Upstream) war daher unzutreffend.
# Der Vendor-Inhalt bleibt per Vendoring-Doktrin unangetastet; nur die Provenienz-Angabe
# wird hier ehrlich gemacht.

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle libs). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
# AUSNAHME (B16, begruendet statt still): comdare_hdr_histogram_wrapper kompiliert die
# VENDOR-C-Quelle vendor/hdr_histogram.c -- fremder Code bekommt unsere Warnflags nicht
# (dasselbe Prinzip, nach dem ext/ von der Deckung ausgenommen ist). Der ce-eigene
# Wrapper-Anteil ist header-only und wird ueber seine Konsumenten-TUs gedeckt.
# KEIN COMDARE_set_default_warnings auf diesem Target.
