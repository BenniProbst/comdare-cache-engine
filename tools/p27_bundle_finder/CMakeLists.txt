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
