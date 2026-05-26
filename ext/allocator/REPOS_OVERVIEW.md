# Allokator-Repositories Overview (Phase V41.F.6.1.struct ext/-Reorganisation)

**Stand:** 2026-05-26 nach ext/-Topic-Reorganisation + 6/10 Original-Wrapper integriert
**Pflicht-Klone abgeschlossen:** 10 von 23 Allokator-Papern (A01-A23)
**Vorlage:** Phase 4.B-detail (12 Such-Algorithmus-Repos in P*-Verzeichnissen)
**Roll-out Status (V41.F.6.1.P2.B/P2.D/P2.D.b2):** 6 als Original-Wrapper integriert, 4 deferred (Task #685)

## Geklonte Repositories (Pflicht, alle Lizenzen permissive)

| ID | Verzeichnis | Repo | Lizenz | Groesse | Wrapper-Status | Originall |
|----|-------------|------|--------|---------|:--------------:|:---------:|
| **A01** | `A01-hoard/` | [emeryberger/Hoard](https://github.com/emeryberger/Hoard) | Apache 2.0 | 983K | DEFERRED Task #685 (Custom-Shim) | (TBD) |
| **A03** | `A03-michael-lockfree/` | [scotts/michael](https://github.com/scotts/michael) | MIT (re-impl) | 173K | DEFERRED Task #685 (Custom-Shim) | (TBD) |
| **A04** | `A04-mimalloc/` | [microsoft/mimalloc](https://github.com/microsoft/mimalloc) | MIT | 7.4M | ✅ MimallocAllocator (Pilot P2.B) | 2/2 |
| **A05** | `A05-jemalloc/` | [jemalloc/jemalloc](https://github.com/jemalloc/jemalloc) | BSD-2 | 6.4M | ✅ JemallocAllocator (P2.D) | 2/2 |
| **A06** | `A06-tcmalloc/` | [google/tcmalloc](https://github.com/google/tcmalloc) | Apache 2.0 | 6.5M | DEFERRED Task #685 (Bazel-Build) | (TBD) |
| **A07** | `A07-snmalloc/` | [microsoft/snmalloc](https://github.com/microsoft/snmalloc) | MIT | 9.2M | ✅ SnmallocAllocator (P2.D) | 2/2 |
| **A08** | `A08-scalloc/` | [cksystemsgroup/scalloc](https://github.com/cksystemsgroup/scalloc) | BSD-3 | 558K | DEFERRED Task #685 (Custom-Shim) | (TBD) |
| **A10** | `A10-rpmalloc/` | [mjansson/rpmalloc](https://github.com/mjansson/rpmalloc) | PD/MIT | 617K | ✅ RPMallocAllocator (P2.D.b2) | 2/2 |
| **A11** | `A11-lrmalloc/` | [ricleite/lrmalloc](https://github.com/ricleite/lrmalloc) | MIT | 223K | ✅ LRMallocAllocator (P2.D.b2) | 2/2 |
| **A20** | `A20-dlmalloc/` | [ennorehling/dlmalloc](https://github.com/ennorehling/dlmalloc) | Public Domain | 366K | ✅ DlmallocAllocator (P2.D.b2) | 2/2 |

**Total Disk:** ~32 MiB (mit `--depth 1` schmal gehalten)
**Roll-out Bilanz:** 6/10 integriert, 4 DEFERRED (Custom-Shim/Bazel-Build noetig)

## Lizenz-Kompatibilitaet (alle 10 Repos)

✅ **Alle Lizenzen permissive** und mit Comdare's Apache-2.0 kompatibel.
✅ **Statisches Linking + Modularisierung** ist nach Architekt-Direktive 2026-05-08
   ("modularisierte Bruchstuecke + C++23-Metaprogrammierung = neues Werk") zulaessig.
✅ **Pro Repo:** LICENSE-Datei wurde NICHT geloescht — bleibt bit-identisch erhalten.

⚠️ **A03 IBM Patent:** US 2006/0190697 A1 (Maged Michael "lock-free memory allocator
   with delayed coalescing"). Vor commercial use rechtliche Pruefung; fuer
   academic Diplomarbeit unbedenklich.

## Nicht-geklont (Phase 6.2.C optional / spaeter)

| ID | Grund | Plan |
|----|-------|------|
| **A02** Bonwick Slab | nur in Kernel-Source-Bases | Linux mm/slub.c als Reference + ggf. eigene Re-Impl |
| **A09** NUMAlloc | Forschungsprototyp, URL pending | bei UMass-Group anfragen |
| **A12** CAMA | Saarland-internal, URL pending | bei Reineke-Group anfragen |
| **A13** StarMalloc | F\* Build-Stack-Komplexitaet | klone bei Bedarf, nicht in Default-Build |
| **A14** ASPLOS-2024-Optimierungen | bereits in google/tcmalloc gerollt | A06 enthaelt diese Optimierungen bereits |
| **A15** HMalloc | IEEE-Code unklar | bei Autoren anfragen |
| **A16** PIM-malloc | UPMEM-Hardware-only | klone fuer Future-Plattform-Studie |
| **A17** Crystalline | URL pending | bei Penn-State-Group anfragen |
| **A18** Exgen-Malloc | URL pending | bei UT-Austin-SysML anfragen |
| **A19** Buddy | klassischer Algorithmus | eigene Re-Impl als Algorithmus-Studie (~200 LOC) |
| **A21** ptmalloc2 | LGPL via system glibc | bereits System-Default; LGPL inkompatibel mit static-link in commercial |
| **A22** N3916 PMR | C++17 Standard-Library | Compiler-included; nicht klone-pflichtig |
| **A23** Vmem+Magazines | illumos / FreeBSD UMA | klone illumos kmem.c als Reference |

## Naechste Schritte (Phase 6.2.E ff.)

Nach diesem Klone-Schritt geht es weiter mit:

1. **Phase 6.2.E:** Aufbau `cache_engine/include/cache_engine/allocators/` Verzeichnis-Struktur
   - Concept-Wurzeln (i_allocation_strategy.hpp, allocator_concept.hpp,
     pmr_resource_concept.hpp, locking_concept.hpp)
   - Pro Paper: families/a01_hoard/, families/a04_mimalloc/, ...
   - Pro Achse: axes/aa1_freelist_topology/, axes/aa2_size_class_schema/, ...
2. **Phase 6.2.F:** Container-Test-Matrix (vector/map/unordered_map/deque/list/set/pmr-Variants)
3. **Phase 6.2.G:** Concurrency-Tests (single-W/multi-R Default + Cache-Page-Aware Multi-W optional)

## Pro-Repo STRUKTUR_NOTIZ-Empfehlung

Pro geklontem Repo waere eine STRUKTUR_NOTIZ.md analog Phase 4.B-detail
(P*-Verzeichnisse) wuenschenswert, aber NICHT mehr Phase-6.2.C-Pflicht.
Stattdessen ist diese Overview-Datei + die einzelnen `_paper_extractions_allocators/A{NN}-*.md`
Extraktionen ausreichend zur Code-Verstaendnis.

Falls spaeter Adapter-Skelette in `comdare-cache-engine/adapters/A{NN}-<short>/`
geschrieben werden, ist dort Detail-Mapping pro Adapter Pflicht.
