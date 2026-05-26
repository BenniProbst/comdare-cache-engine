# paper_a07_snmalloc — Kuratierte snmalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.D Roll-out 2/3)
**Wrapper-Klasse:** `comdare::cache_engine::allocator::axis_06_allocator::SnmallocAllocator`

## Paper-Referenz (Vollangabe)

Lipp, J., Bond, M., Parkinson, M. *snmalloc: A Message Passing Allocator.*
In Proceedings of the 2019 ACM SIGPLAN International Symposium on Memory
Management (ISMM '19), Phoenix, AZ, USA, June 23, 2019, ACM, pp. 122-135.
DOI: 10.1145/3315573.3329980

## Quellen

- **Upstream-Repo:** https://github.com/microsoft/snmalloc
- **Lizenz:** MIT (siehe LICENSE — Kopie aus ext/A07-snmalloc/LICENSE)
- **Cache-Engine-Kopie aus:** `ext/A07-snmalloc/`

## Besonderheiten

snmalloc nutzt **C++ namespaces** (`snmalloc::libc::aligned_alloc`) statt
extern "C". Tool-Pattern: paper_fn ist `aligned_alloc` (Identifier) — Tool findet
es im Source via Regex `\baligned_alloc\s*\(` (greift auf namespace-Body).

Source-File: `src/snmalloc/global/libc.h` (Header-only — inline-Functions).
