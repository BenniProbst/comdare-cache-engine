# V31.K5 (2026-05-14) — P27 Bundle Finder
# C++23-Port von ext/.../hp-soft/hp_soft.py
# (Author: T. Zhang et al. 2025, ASPLOS, Hierarchical Prefetching)
#
# Lizenz: hp-soft.zip enthielt KEINE LICENSE -> Architekt-Direktive II
#         2026-05-14 (citation-only). Provenance in Forschungsarbeiten/
#         code/P27-Zhang-ASPLOS-Hierarchical/STATUS.md.

add_executable(p27_bundle_finder
    src/main.cpp
)
target_compile_features(p27_bundle_finder PRIVATE cxx_std_23)
target_include_directories(p27_bundle_finder PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# =============================================================================
# B16/K12 (bau/w1-luecken, 2026-08-21) -- B16-WARN-DECKUNG: -Wall-Familie fuer die
# Nicht-Test-Targets dieser Datei (Welle tools). Bis heute wirkte
# COMDARE_set_default_warnings NUR ueber cmake/gtest_setup.cmake (Test-Executables);
# Produktions-TUs bauten ohne -Wall. Anhang ANS DATEIENDE (Kollisionsschutz).
COMDARE_set_default_warnings(p27_bundle_finder)
