# paper_p10_surf — Kuratierte SuRF-Snapshot (P2.D.tr.s3 Batch 1)

**Paper:** Zhang, H., Lim, H., Andersen, D. et al. *SuRF: Practical Range Query
Filtering with Fast Succinct Tries.* SIGMOD 2018.

**Source:** `ext/traversal/P10-SuRF/SuRF/include/surf.hpp` (Apache-2.0).

**Algorithmus-Charakteristik:** Read-only Succinct-Range-Filter (Bulk-Loaded
Trie nach Konstruktion). KEIN klassisches Insert/Erase/Clear im Paper-Design —
SuRF wird einmalig aufgebaut + nur lookupKey/lookupRange unterstützt.

**API-Mapping (1/4 originall, 3 Lücken):**
- insert → **LUECKE** (Bulk-Loaded, kein incremental Insert — Re-Impl)
- lookup → lookupKey (Z143)
- erase  → **LUECKE** (Read-only Index — Re-Impl als Tombstone)
- clear  → **LUECKE** (Cache-Engine Re-Impl als Reconstruction)

**Wrapper-Klasse:** OriginalSurfSearchAlgo (S08).
is_original_module() = false (3/4 Lücken).
