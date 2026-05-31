# Reale i7-1270P-Organ-Messung → 16-Spalten-Pipeline-CSV (P3-Bridge-Beleg, 2026-05-31)

Erzeugt via `comdare-f15-compare <organ-dll-dir> --batches=64 --pipeline-csv=...` über die 6 sezierten
Observable-Organ-Compositions. `total_cycles` = reale mittlere ns-Latenz (Stufe-05-Konvention). Diese CSV
speist Stufe 04/05 → `pipeline_demo.pdf` (verifiziert: LaTeX-Tabelle traegt 740176/825053 ns + Organ-Namen).

```
permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,coherence_invalidations,energy_micro_joules,bytes_allocated,bytes_in_use_peak,external_frag,internal_frag
ArtComposition_0,287533837128950919,1,micro,2000,825053,0,0,0,0,0,0,128000,128000,0,0
HotComposition_1,18251944374968255158,1,micro,2000,740176,0,0,0,0,0,0,128000,128000,0,0
StartComposition_2,15356030319725185150,1,micro,2000,805750,0,0,0,0,0,0,128000,128000,0,0
SurfComposition_3,8244977537145811207,1,micro,2000,873976,0,0,0,0,0,0,128000,128000,0,0
WormholeComposition_4,15206926672969497633,1,micro,2000,696350,0,0,0,0,0,0,128000,128000,0,0
MasstreeComposition_5,16757571591876626545,1,micro,2000,770871,0,0,0,0,0,0,128000,128000,0,0
```
