# tools/ — Build-Skripte + CLIs

add_subdirectory(ycsb_cli)
add_subdirectory(latex_anhang)
# V31.K5 (2026-05-14): P27 Bundle Finder (C++23-Port von hp_soft.py)
add_subdirectory(p27_bundle_finder)
# tools/latex_toolchain ist eine CMake-only-Toolchain (kein C++-Target);
# wird via cmake -P aufgerufen, kein add_subdirectory noetig.
