# Koeder-Klasse "subdir_shadow": ein Subprojekt (Vendor-Muster) ueberschattet die
# CACHE-FORCE-Release-Flags mit einer NORMALEN Variable -- exakt der Reichweiten-
# Riss der design/probes/ (Probe B/C: die Cache luegt, die TUs bauen -O3).
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_sub.cpp" "int vo31_koeder_sub() { return 3; }\n")
add_library(koeder_subdir STATIC "${CMAKE_CURRENT_BINARY_DIR}/gen/koeder_sub.cpp")
