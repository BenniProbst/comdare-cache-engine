# Allokator-Profile (V26.A)

**Eingefuehrt:** V26.A (2026-05-14) — analog SOTA-Algorithmus-Profile in `../sota/`

## Schema

Pro Allokator (A01-A20) ein Profile-XML mit:
- `id`, `family_ref` (A-Nummer aus ext/A*-Verzeichnis)
- `metadata` (Autoren, Lizenz, Quelle)
- `axes`: granularity, numa, thread_local, fragmentation_strategy, thread_safety
- `abi`: C-API + C++-Overload
- `expected_workload`: Default-Workload-Affinitaet (YCSB_A..F)

## Mitglieder (10)

| Profile | Family | Lizenz | expected_workload |
|---|---|---|---|
| hoard | A01 | Apache-2.0 | YCSB_A |
| michael_lockfree | A03 | LGPL-2.1-or-later | YCSB_B |
| mimalloc | A04 | MIT | YCSB_A |
| jemalloc | A05 | BSD-2 | YCSB_B |
| tcmalloc | A06 | Apache-2.0 | YCSB_C |
| snmalloc | A07 | MIT | YCSB_A |
| scalloc | A08 | BSD-3 | YCSB_A |
| rpmalloc | A10 | Public Domain | YCSB_C |
| lrmalloc | A11 | MIT | YCSB_B |
| dlmalloc | A20 | CC0 | YCSB_C |

## Querverweis

- ALLOCATOR_REPOS_OVERVIEW.md (Quell-Repos)
- adapters/A01-A20/ (V26.B Adapter-Skelette)
- ext/A* (Original-Code, kompiliert mit Original-Compiler)
