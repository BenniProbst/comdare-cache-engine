# REV 7.7 V23.B — apps/ Top-Level
# Aggregator fuer alle Top-Level-Executables (Pitchfork-/ClickHouse-Konvention).
# Library-Code lebt in cache_engine/, experiment/, workload_generator/, etc. —
# hier nur die main.cpp-Wrapper.

add_subdirectory(cache_engine_builder)


# V41.F.6.1.P1 Phase B — Pre-Build-Tool fuer is_original-Codegen
add_subdirectory(is_original_validator)

# V41.F.6.1.R5.F — CLI-Tool fuer Anatomy-Composition-Snippet-Codegen
add_subdirectory(anatomy_codegen_tool)

# V41.F.6.1.R5.G — CLI-Tool, das pro Permutation eines Raums ein AdHoc-Modul-.cpp emittiert
add_subdirectory(adhoc_emitter)

# STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, Weiche A, 2026-08-27) -- CLI der L4-Andock-Flaeche: emittiert
# die 13 C2-Vertragspaar-Fixtures (gestempelt/stempellos je Gattung + Hybrid) fuer cmake/modul_emitter.cmake
add_subdirectory(modul_emitter)

# Limits-Entkopplung Vorstufe - CLI-Tool fuer XML-Profil -> generierter Source-Katalog-Header
add_subdirectory(catalog_codegen_tool)

# #25 Teil B — CLI-Tool: C++23-Port von codegen.cmake (opt-in Backend 'cpp', byte-identische permutations.cmake)

# V41.F.6.1.R5.D — CLI-Treiber fuer den F15-Messlauf ueber materialisierte Permutations-DLLs
add_subdirectory(f15_compare)

# L-CLUSTER (gate-frei) — lokaler Cluster-Mess-Runner je Binary (laedt 1 perm-DLL -> result_ingest-Zeile)
add_subdirectory(perm_runner)

# GO-5 Fork 7 — Generator der TOOL-BERECHNETEN H2-Code-Qualitaets-Akte (sota_h2_scores.xml)
add_subdirectory(h2_score_akte_tool)

# W1 (F1-HART, Owner-KERN 05.08.2026) -- der Experiment-Planer als EIGENE Binary
# (comdare-experiment-planner): die PLANER-Rolle (Stufe 1) getrennt von der CEB-Rolle des
# comdare-messung-driver. Zwei Module ueber DIESELBE Fassaden-Bibliothek, keine Vererbung.
add_subdirectory(experiment_planner)
