# axis_06_allocator — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix — A (standalone, is_original-faehige OSS-Codes: mimalloc/jemalloc/snmalloc/rpmalloc/dlmalloc/lrmalloc, MIT/BSD/Public-Domain) + C (lizenz-blockiert: ptmalloc2 LGPL-2.1+) + D/E (kein OSS-Code / Engineering-Baselines: Slab, Buddy, CAMA, HMalloc, VmemMagazines, StdMalloc).

## §1 Pflicht-Note
Echtes `is_original`-Linking (OSS + permissiv) haben 6 Wrapper dieser Achse: MimallocAllocator (MIT), JemallocAllocator (BSD-2-Clause), SnmallocAllocator (MIT), RPMallocAllocator (Public-Domain/MIT), DlmallocAllocator (Public-Domain/CC0) und LRMallocAllocator (MIT) — alle bereits mit OriginalCodeMixin aktiv (is_original 2/2). Alle uebrigen Wrapper sind Re-Impl bzw. Baseline: Hoard/TCMalloc/scalloc/snmalloc-verwandte sind OSS, aber als Wrapper nicht original gebunden; MichaelLockFreeAllocator ist eine MIT-Re-Impl (IBM-Patent-Hintergrund); ptmalloc2 ist LGPL-2.1+ (linking-blockiert); Slab, Buddy, CAMA, HMalloc, VmemMagazines haben keinen OSS-Code; StdMalloc ist die libc-Baseline. RPMallocInitGuard ist KEIN Allocator (Init-on-First-Use Guard).

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| HoardAllocator | Hoard (Per-Heap Free-Lists, SMP scalable) | Hoard: A Scalable Memory Allocator for Multithreaded Applications | ASPLOS-IX 2000 | 10.1145/378993.379232 | OSS | Apache-2.0 | ✗ |
| SlabAllocator | Slab Allocator (Object-Caching Kernel Allocator) | The Slab Allocator: An Object-Caching Kernel Memory Allocator | USENIX Summer 1994 | usenix.org/.../bonwick.html | nein | none | ✗ |
| MichaelLockFreeAllocator | Michael Lock-Free CAS Allocator | Scalable Lock-Free Dynamic Memory Allocation | PLDI 2004 (ACM SIGPLAN) | 10.1145/996841.996848 | OSS | MIT (Re-Impl; IBM-Patent) | ✗ |
| MimallocAllocator | mimalloc (Free-List Sharding) | Mimalloc: Free List Sharding in Action | APLAS 2019; MSR-TR-2019-18 | 10.1007/978-3-030-34175-6_13 | OSS | MIT | ✓ |
| JemallocAllocator | jemalloc (Arena-based Size-Classes) | A Scalable Concurrent malloc(3) Implementation for FreeBSD | BSDCan 2006 | papers.freebsd.org/2006/bsdcan/evans-jemalloc/ | OSS | BSD-2-Clause | ✓ |
| TCMallocAllocator | TCMalloc (Thread-Caching Malloc) | TCMalloc Design-Doku (Ghemawat/Menage); später TEMERAIRE | Google ca. 2005; OSDI 2021 | gperftools.github.io/.../tcmalloc.html | OSS | Apache-2.0 | ✗ |
| SnmallocAllocator | snmalloc (Message-Passing Allocator) | snmalloc: a message passing allocator | ISMM 2019 | 10.1145/3315573.3329980 | OSS | MIT | ✓ |
| ScallocAllocator | scalloc (Span-based, Virtual Spans) | Fast, Multicore-Scalable, Low-Fragmentation Memory Allocation… | OOPSLA 2015 | 10.1145/2814270.2814294 | OSS | BSD-3-Clause | ✗ |
| RPMallocAllocator | rpmalloc (Per-Thread Span Caching) | — (kein Paper, OSS-Projekt) | 2017 | github.com/mjansson/rpmalloc | OSS | Public-Domain/MIT | ✓ |
| RPMallocInitGuard | rpmalloc Init-on-First-Use Guard (KEIN Allocator) | — | — | — | lokal | Public-Domain/MIT | ✗ |
| NUMAllocAllocator | NUMAlloc (NUMA origin-aware Allocator) | NUMAlloc: A Faster NUMA Memory Allocator | ISMM 2023 | 10.1145/3591195.3595276 | OSS | unknown (vermutl. Apache-2.0) | ✗ |
| PIMMallocAllocator | PIM-malloc (Processing-In-Memory Allocator) | PIM-malloc: A Fast and Scalable Dynamic Memory Allocator for PIM… | HPCA 2026 (arXiv 2025) | arxiv.org/abs/2505.13002 | OSS | MIT | ✗ |
| PmrResourceAllocator | std::pmr::memory_resource | N3916: Polymorphic Memory Resources (WG21) | WG21 2014 → C++17 | open-std.org/.../n3916.pdf | OSS | Standard-Library | ✗ |
| PoolResourceAllocator | std::pmr::unsynchronized_pool_resource (eigener Size-Class-Pool) | N3916: Polymorphic Memory Resources (WG21) | WG21 2014 → C++17 | open-std.org/.../n3916.pdf | OSS | Standard-Library | ✗ |
| PtMalloc2Allocator | ptmalloc2 (glibc malloc) | — (kein Peer-Review; Gloger/Lea) | 1990er-2000er | malloc.de/en/ | OSS | LGPL-2.1+ | ✗ |
| BuddyAllocator | Buddy System (Power-of-2 Splitting) | A Fast Storage Allocator (Buddy); Knuth TAOCP Vol.1 | CACM 1965; TAOCP 1968 | 10.1145/365628.365655 | nein | none | ✗ |
| ExgenAllocator | Exgen-Malloc (Single-Threaded Specialized) | Old is Gold: Optimizing Single-threaded Applications with Exgen-Malloc | IEEE CAL 2025 (UT Austin) | arxiv.org/abs/2510.10219 | ? | unknown | ✗ |
| StarMallocAllocator | StarMalloc (Formally Verified Hardened, F*/Steel) | StarMalloc: Verifying a Modern, Hardened Memory Allocator | OOPSLA 2024; arXiv 2403.09435 | 10.1145/3689773 | OSS | Apache-2.0 | ✗ |
| DlmallocAllocator | dlmalloc (Doug Lea Malloc, Bins-based) | A Memory Allocator (Design-Artikel) | 1996 (Code seit 1987) | gee.cs.oswego.edu/dl/html/malloc.html | OSS | Public-Domain (CC0) | ✓ |
| LRMallocAllocator | lrmalloc (Lock-Free + Hazard-Pointers) | LRMalloc: A Modern and Competitive Lock-Free Dynamic Memory Allocator | VECPAR 2018 (LNCS 11333) | 10.1007/978-3-030-15996-2_17 | OSS | MIT | ✓ |
| HMallocAllocator | HMalloc (Hybrid, Scalable, Lock-Free) | HMalloc: A Hybrid, Scalable, and Lock-Free Memory Allocator… | IEEE ICPADS 2019 | 10.1109/ICPADS47876.2019.00114 | nein | none | ✗ |
| CAMAAllocator | CAMA (Predictable Cache-Aware Allocator) | CAMA: A Predictable Cache-Aware Memory Allocator | ECRTS 2011 (Saarland) | 10.1109/ECRTS.2011.10 | nein | none | ✗ |
| TCMallocWarehouseAllocator | TCMalloc-Warehouse/Hyperscale (hugepage-aware) | TEMERAIRE; Characterizing a Memory Allocator at Warehouse Scale | OSDI 2021; ASPLOS 2024 | usenix.org/.../hunter ; 10.1145/3620666.3651350 | OSS | Apache-2.0 | ✗ |
| StdMalloc | Standard libc malloc (Baseline/Concept-Beweis) | — | — | — | OSS | Standard-Library | ✗ |
| VmemMagazinesAllocator | Vmem + Magazines (Slab-Erweiterung) | Magazines and Vmem: Extending the Slab Allocator to Many CPUs… | USENIX ATC 2001 | usenix.org/.../bonwick.html | nein | none | ✗ |

> Header-Korrekturen aus Map §4 (alte Code-Kommentare waren falsch — hier bereits korrigiert eingetragen):
> - MichaelLockFreeAllocator: "PODC 2002 + JPDC 2004" → korrekt PLDI 2004 (ACM SIGPLAN).
> - SnmallocAllocator: Erstautor "Lipp/Bond/Parkinson" → korrekt Paul Liétar (snmalloc, ISMM 2019).
> - ScallocAllocator: "Aigner/Iurca/Wimmer PPoPP 2015" → korrekt Aigner/Kirsch/Lippautz/Sokolova, OOPSLA 2015.
> - NUMAllocAllocator: "Linden/Liu/Williams ICDCS 2018" → korrekt Yang/Zhao/…/Liu, ISMM 2023 (UTSASRG).
> - StarMallocAllocator: "Dang/Charguer 2024" → korrekt Inria-Prosecco (Reitz/Fromherz/Protzenko et al.), OOPSLA 2024.
> - LRMallocAllocator: "JPDC 2019" → korrekt Leite/Rocha, VECPAR 2018 (LNCS 11333).
> - HMallocAllocator: "Tang 2020" → korrekt Li/Yao/Tang/Lin, ICPADS 2019.
> - CAMAAllocator: "Bhattacharyya/Beard/Cohen 2020" → korrekt Herter/Backes/Haupenthal/Reineke, ECRTS 2011 (Saarland).
> - PIMMallocAllocator: vage "UPMEM/HBM-PIM 2023+" → präzisiert VIA-Research, HPCA 2026 / arXiv 2505.13002.
> - ExgenAllocator: vage "Exception-Generated Single-Thread" → präzisiert "Old is Gold", UT Austin, IEEE CAL 2025.

## §3 Compliance-Status
Alle 26 Allocator-Wrapper (RPMallocInitGuard ist Guard, kein Allocator) haben entweder eine Paper-Referenz oder sind explizit als Baseline/Lehrbuch gekennzeichnet (StdMalloc = libc-Baseline; Buddy/Slab/Vmem = Lehrbuch-/Kernel-Klassiker ohne OSS-Code; PoolResourceAllocator = std::pmr-Standardbibliothek, R5.B 2026-05-29). → Habich-Pflicht erfuellt.

> **Zaehl-Korrektur (2026-07-13, Registry AllVendors = 26 Wrapper, autoritativ):** Gesamtzahl auf **26** angeglichen
> (vorher 25; davon 25 default-enabled, VampirNfpAllocator default-OFF). In der §2-Tabelle fehlen noch
> CrystallineAllocator (A17) und VampirNfpAllocator (A24) — Attribution/Lizenz dieser beiden Zeilen wird hier NICHT
> erfunden, sondern separat nachgezogen.

**R5.B-Hinweis (2026-05-29):** PoolResourceAllocator ist der erste axis_06-Wrapper, der sich OHNE externes Linking verhaltens-distinkt von System-malloc verhaelt (besitzt einen eigenen unsynchronized_pool_resource statt new_delete-Fallback). Damit wird die Allocator-Achse F15-operativ (nicht-hohle 2-Achsen-Messung search_algo × allocator moeglich, vs. die uebrigen Wrapper, die ohne Vendor-Linking auf portable_aligned_alloc = System zurueckfallen).

**is_original-Linking-Kandidaten dieser Achse (Map §3, alle bereits aktiv mit OriginalCodeMixin, is_original 2/2):**
- MimallocAllocator — github.com/microsoft/mimalloc — MIT
- JemallocAllocator — github.com/jemalloc/jemalloc — BSD-2-Clause
- SnmallocAllocator — github.com/microsoft/snmalloc — MIT
- RPMallocAllocator — github.com/mjansson/rpmalloc — Public-Domain/MIT
- DlmallocAllocator — github.com/ennorehling/dlmalloc (Mirror); gee.cs.oswego.edu — Public-Domain (CC0)
- LRMallocAllocator — github.com/ricleite/lrmalloc — MIT

**Lizenz-/Code-Einschraenkungen (kein is_original-Linking):**
- PtMalloc2Allocator — LGPL-2.1+ (copyleft, Linking blockiert).
- NUMAllocAllocator — Lizenz nicht eindeutig (404 auf raw LICENSE; vermutl. Apache-2.0), confidence medium (Map §5).
- ExgenAllocator — Paper sicher (CAL 2025), aber kein oeffentliches Repo, Algorithmus-Identitaet zum Wrapper unklar, confidence medium (Map §5).
- HMallocAllocator — Paper sicher (ICPADS 2019), kein oeffentlicher Code, Wrapper-Beschreibung generisch, confidence medium (Map §5).
- TCMallocWarehouseAllocator — Marketing-Variante von TCMalloc, kein eigenstaendiges Artefakt (identisch zu TCMallocAllocator), confidence medium (Map §5).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_06_allocator, §3 is_original-Kandidaten, §4 Header-Korrekturen, §5 offene Punkte).
- Doku 17 §4.5 (Klassifikation A/C/D/E).
- Hinweis (Map §6): axis_06_allocator ist NICHT im lokalen P01-P33-Katalog (Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md enthaelt ausschliesslich Tree/Trie/Prefetch/Concurrency-DBMS-Paper). Die Allokatoren liegen in einem separaten Bereich (`ext/allocator/A01–A23` mit eigenem REPOS_OVERVIEW).
