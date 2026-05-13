# tools/ — Build-Skripte + CLIs

add_subdirectory(ycsb_cli)
add_subdirectory(latex_anhang)
# tools/latex_toolchain ist eine CMake-only-Toolchain (kein C++-Target);
# wird via cmake -P aufgerufen, kein add_subdirectory noetig.
