# paper_a11_lrmalloc — Kuratierte lrmalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.D Batch 2 — 3/6 Roll-out)
**Wrapper:** `LRMallocAllocator` (A11, Leite/Rocha JPDC 2019)

## Paper-Referenz

Leite, R., Rocha, R. *LRMalloc: A Modern and Competitive Lock-Free Dynamic
Memory Allocator.* Journal of Parallel and Distributed Computing, Vol. 142,
August 2020, pp. 116-130. Elsevier.
DOI: 10.1016/j.jpdc.2020.04.010

## Source

`ext/A11-lrmalloc/lrmalloc.cpp` (BSD License — siehe LICENSE Kopie).
API (extern "C"): `lf_aligned_alloc(alignment, size)` + `lf_free(ptr)`.
