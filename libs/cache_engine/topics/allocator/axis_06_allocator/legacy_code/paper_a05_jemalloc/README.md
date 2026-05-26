# paper_a05_jemalloc — Kuratierte jemalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.D Roll-out 1/3)
**Topic:** allocator
**Achse:** axis_06_allocator
**Wrapper-Klasse:** `comdare::cache_engine::allocator::axis_06_allocator::JemallocAllocator`

## Paper-Referenz (Vollangabe)

Evans, J. *A Scalable Concurrent malloc(3) Implementation for FreeBSD.*
In Proceedings of the BSDCan Conference, Ottawa, Canada, April 2006.
URL: https://www.bsdcan.org/2006/papers/jemalloc.pdf

Erweiterte Production-Version dokumentiert in:
Evans, J. *jemalloc Internals.* Facebook Engineering Blog, 2011-2024.
URL: https://github.com/jemalloc/jemalloc (Upstream-Repo, BSD-2-clause).

## Quellen

- **Upstream-Repo:** https://github.com/jemalloc/jemalloc
- **Lizenz:** BSD-2-clause (siehe LICENSE — Kopie aus ext/A05-jemalloc/COPYING)
- **Cache-Engine-Kopie aus:** `ext/A05-jemalloc/` (User-Original-Distribution)

## Files (analog mimalloc-Pattern)

| Datei | git-tracked |
|---|---|
| LICENSE | ja (BSD-2) |
| README.md | ja |
| compiler_info.txt | ja |
| manifest.txt | ja |
| MODIFICATIONS.md | ja |
| sha256_locked.txt | ja (auto-generated First-Build) |
| .extracted.marker | nein (gitignored) |
| src/*.c | nein (gitignored, Build-Time-Init aus ext/) |
| include/*.h | nein (gitignored, falls noetig) |

## Cross-Refs

- `[[paper-original-code-pattern]]` `[[legacy-code-sha256-validation]]` `[[axis-base-pattern]]`
- mimalloc-Vorlage: `legacy_code/paper_a04_mimalloc/`
- Doku: `docs/architektur/13_paper_legacy_code_architektur.md` §19 (P2.C-Endstand)
