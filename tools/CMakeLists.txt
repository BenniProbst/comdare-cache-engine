# tools/ — Build-Skripte + CLIs

add_subdirectory(ycsb_cli)
add_subdirectory(latex_anhang)
# V31.K5 (2026-05-14): P27 Bundle Finder (C++23-Port von hp_soft.py)
add_subdirectory(p27_bundle_finder)
# #172 Versprechen 1 (2026-06-20): beste-Binary-Auswahl + Versand (vorher nur ad-hoc build_and_run.bat).
add_subdirectory(best_binary_selector)
# tools/latex_toolchain ist eine CMake-only-Toolchain (kein C++-Target);
# wird via cmake -P aufgerufen, kein add_subdirectory noetig.
