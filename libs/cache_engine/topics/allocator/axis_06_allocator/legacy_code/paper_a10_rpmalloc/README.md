# paper_a10_rpmalloc — Kuratierte rpmalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.D Batch 2 — 3/6 Roll-out)
**Wrapper:** `RPMallocAllocator` (A10, Jansson 2017)

## Paper-Referenz

Jansson, M. *rpmalloc - General Purpose Memory Allocator.*
Rampant Pixels, 2017-2024.
URL: https://github.com/mjansson/rpmalloc

## Source

`ext/A10-rpmalloc/rpmalloc/rpmalloc.c` (MIT/Public-Domain dual license).
API: `rpaligned_alloc(alignment, size)` + `rpfree(ptr)`.
