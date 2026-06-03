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

# V41.F.6.1.R5.D — CLI-Treiber fuer den F15-Messlauf ueber materialisierte Permutations-DLLs
add_subdirectory(f15_compare)

# L-CLUSTER (gate-frei) — lokaler Cluster-Mess-Runner je Binary (laedt 1 perm-DLL -> result_ingest-Zeile)
add_subdirectory(perm_runner)
