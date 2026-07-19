# PAKET W3-C (Ledger Sec.32-F4/GN-8/O-4) - axis_version_lock: eigenstaendiges C++23-Executable, das den
# Content-Digest der Heuristik-Strategie-Header gegen tools/axis_version_lock/axis_version.lock prueft
# (--check) bzw. das Lock neu schreibt (--write). Wiederverwendung der vorhandenen consteval-SHA-256
# (libs/cache_engine/src/sha256/ctsha.hpp). KEINE Fremdbibliothek, kein Python.
#
# EXCLUDE_FROM_ALL (INERT-by-default, Anti-Phantom): der Standard-Build baut das Tool NICHT mit; es wird
# explizit per Target gebaut (analog axis_registry_gen / gen_golden_fullpilot). Der CI-Job (W3-A) baut das
# Target und ruft --check. golden/TABU bleiben unberuehrt (das Tool liest nur Header, schreibt nur das Lock).

add_executable(comdare_axis_version_lock EXCLUDE_FROM_ALL axis_version_lock.cpp)
target_compile_features(comdare_axis_version_lock PRIVATE cxx_std_23)
target_include_directories(comdare_axis_version_lock PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")   # <sha256/ctsha.hpp>
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_axis_version_lock)
endif()
