# paper_a20_dlmalloc — Kuratierte dlmalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.D Batch 2 — 3/6 Roll-out)
**Wrapper:** `DlmallocAllocator` (A20, Lea 1987 Classic Bins)

## Paper-Referenz

Lea, D. *A Memory Allocator.* Unix/Mail, 6/87, 1987.
URL: http://gee.cs.oswego.edu/dl/html/malloc.html

dlmalloc ist die kanonische Bins-basierte malloc-Implementation, Grundlage
fuer glibc's ptmalloc/ptmalloc2.

## Source

`ext/A20-dlmalloc/malloc.c` (Public Domain CC0). API: `public_fREe`/`public_mEMALIGn`
via `#define dlfree public_fREe` / `#define dlmemalign public_mEMALIGn`.
