# tools/ — Build-Skripte + CLIs

add_subdirectory(ycsb_cli)
add_subdirectory(latex_anhang)
# V31.K5 (2026-05-14): P27 Bundle Finder (C++23-Port von hp_soft.py)
add_subdirectory(p27_bundle_finder)
# #172 Versprechen 1 (2026-06-20): beste-Binary-Auswahl + Versand (vorher nur ad-hoc build_and_run.bat).
add_subdirectory(best_binary_selector)
# INC-A (2026-07-14): axis_registry_gen — C++-Tool, erzeugt cache_engine_axis_registry.xml per
# compile-time-Reflektion der Enabled*-Achsen-Listen (EXCLUDE_FROM_ALL, explizit gebaut).
add_subdirectory(axis_registry_gen)
# PAKET W2-B (2026-07-19): die 2 fehlenden Art-Registry-Generatoren (Ledger §28/§30) — je Achsen-ART
# ein eigenes XML-Angebot des Compiles per compile-time-Reflektion (EXCLUDE_FROM_ALL, explizit gebaut).
# system_axis_registry_gen -> System-Art (system_config); measurement_axis_registry_gen -> Mess-Art
# (system_measurement). Roundtrip-Gates in tests/unit/CMakeLists.txt (Default-Sweep).
add_subdirectory(system_axis_registry_gen)
add_subdirectory(measurement_axis_registry_gen)
# Bau-INC-2c (2026-07-17): gen_golden_fullpilot — autoritativer Golden-Fixture-Generator
# (offizieller StaticBinaryView-Pfad; EXCLUDE_FROM_ALL, explizit im ABI-Fenster gebaut).
add_subdirectory(gen_golden_fullpilot)
# tools/latex_toolchain ist eine CMake-only-Toolchain (kein C++-Target);
# wird via cmake -P aufgerufen, kein add_subdirectory noetig.
# PAKET W3-C (Ledger Sec.32-F4/GN-8/O-4): axis_version_lock -- Content-Digest-Tripwire der Heuristik-
# Strategie-Header (EXCLUDE_FROM_ALL, INERT-by-default; explizit gebaut vom CI-Lock-Check-Job).
add_subdirectory(axis_version_lock)
