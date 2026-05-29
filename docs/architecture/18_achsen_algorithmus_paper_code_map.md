# Doku 18 — Achsen↔Algorithmus↔Paper↔Code Map (R7.6 umfassende Web-Recherche)
**Stand:** 2026-05-28
**Methode:** Web-Recherche pro Algorithmus (21 Achsen, ~110 Wrapper), Gegenprobe lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md
**Zweck:** Annahmefreie Provenienz-Map — Grundlage für PAPER_REFERENCES.md je Achse + R7.6.c is_original-Linking-Auswahl

---

## §1 Zusammenfassung (Statistik)

| Kennzahl | Wert |
|----------|------|
| Gesamtzahl Algorithmen | 110 |
| `paper_found = true` | 62 |
| `paper_found = false` | 48 |
| `c_cpp_code_exists = yes_oss` | 31 |
| `c_cpp_code_exists = yes_local_forschung` | 14 |
| `c_cpp_code_exists = no` | 60 |
| `c_cpp_code_exists = unknown` | 5 |
| `is_original_eligible = true` | 20 |
| `is_original_eligible = false` | 90 |
| confidence = high | 96 |
| confidence = medium | 13 |
| confidence = low | 1 |

---

## §2 Map pro Achse

### axis_06_allocator

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| HoardAllocator | Hoard (Per-Heap Free-Lists, SMP scalable) | Hoard: A Scalable Memory Allocator for Multithreaded Applications | ASPLOS-IX 2000 | 10.1145/378993.379232 | OSS | Apache-2.0 | — | ✗ | high |
| SlabAllocator | Slab Allocator (Object-Caching Kernel Allocator) | The Slab Allocator: An Object-Caching Kernel Memory Allocator | USENIX Summer 1994 | usenix.org/.../bonwick.html | nein | none | — | ✗ | high |
| MichaelLockFreeAllocator | Michael Lock-Free CAS Allocator | Scalable Lock-Free Dynamic Memory Allocation | PLDI 2004 | 10.1145/996841.996848 | OSS | MIT (Re-Impl; IBM-Patent) | — | ✗ | high |
| MimallocAllocator | mimalloc (Free-List Sharding) | Mimalloc: Free List Sharding in Action | APLAS 2019; MSR-TR-2019-18 | 10.1007/978-3-030-34175-6_13 | OSS | MIT | — | ✓ | high |
| JemallocAllocator | jemalloc (Arena-based Size-Classes) | A Scalable Concurrent malloc(3) Implementation for FreeBSD | BSDCan 2006 | papers.freebsd.org/2006/bsdcan/evans-jemalloc/ | OSS | BSD-2-Clause | — | ✓ | high |
| TCMallocAllocator | TCMalloc (Thread-Caching Malloc) | TCMalloc Design-Doku (Ghemawat/Menage); später TEMERAIRE | Google ca. 2005; OSDI 2021 | gperftools.github.io/.../tcmalloc.html | OSS | Apache-2.0 | — | ✗ | high |
| SnmallocAllocator | snmalloc (Message-Passing Allocator) | snmalloc: a message passing allocator | ISMM 2019 | 10.1145/3315573.3329980 | OSS | MIT | — | ✓ | high |
| ScallocAllocator | scalloc (Span-based, Virtual Spans) | Fast, Multicore-Scalable, Low-Fragmentation Memory Allocation… | OOPSLA 2015 | 10.1145/2814270.2814294 | OSS | BSD-3-Clause | — | ✗ | high |
| RPMallocAllocator | rpmalloc (Per-Thread Span Caching) | — (kein Paper, OSS-Projekt) | 2017 | github.com/mjansson/rpmalloc | OSS | Public-Domain/MIT | — | ✓ | high |
| RPMallocInitGuard | rpmalloc Init-on-First-Use Guard (KEIN Allocator) | — | — | — | lokal | Public-Domain/MIT | — | ✗ | high |
| NUMAllocAllocator | NUMAlloc (NUMA origin-aware Allocator) | NUMAlloc: A Faster NUMA Memory Allocator | ISMM 2023 | 10.1145/3591195.3595276 | OSS | unknown (vermutl. Apache-2.0) | — | ✗ | medium |
| PIMMallocAllocator | PIM-malloc (Processing-In-Memory Allocator) | PIM-malloc: A Fast and Scalable Dynamic Memory Allocator for PIM… | HPCA 2026 (arXiv 2025) | arxiv.org/abs/2505.13002 | OSS | MIT | — | ✗ | high |
| PmrResourceAllocator | std::pmr::memory_resource | N3916: Polymorphic Memory Resources (WG21) | WG21 2014 → C++17 | open-std.org/.../n3916.pdf | OSS | Standard-Library | — | ✗ | high |
| PtMalloc2Allocator | ptmalloc2 (glibc malloc) | — (kein Peer-Review; Gloger/Lea) | 1990er-2000er | malloc.de/en/ | OSS | LGPL-2.1+ | — | ✗ | high |
| BuddyAllocator | Buddy System (Power-of-2 Splitting) | A Fast Storage Allocator (Buddy); Knuth TAOCP Vol.1 | CACM 1965; TAOCP 1968 | 10.1145/365628.365655 | nein | none | — | ✗ | high |
| ExgenAllocator | Exgen-Malloc (Single-Threaded Specialized) | Old is Gold: Optimizing Single-threaded Applications with Exgen-Malloc | IEEE CAL 2025 | arxiv.org/abs/2510.10219 | ? | unknown | — | ✗ | medium |
| StarMallocAllocator | StarMalloc (Formally Verified Hardened, F*/Steel) | StarMalloc: Verifying a Modern, Hardened Memory Allocator | OOPSLA 2024; arXiv 2403.09435 | 10.1145/3689773 | OSS | Apache-2.0 | — | ✗ | high |
| DlmallocAllocator | dlmalloc (Doug Lea Malloc, Bins-based) | A Memory Allocator (Design-Artikel) | 1996 (Code seit 1987) | gee.cs.oswego.edu/dl/html/malloc.html | OSS | Public-Domain (CC0) | — | ✓ | high |
| LRMallocAllocator | lrmalloc (Lock-Free + Hazard-Pointers) | LRMalloc: A Modern and Competitive Lock-Free Dynamic Memory Allocator | VECPAR 2018 | 10.1007/978-3-030-15996-2_17 | OSS | MIT | — | ✓ | high |
| HMallocAllocator | HMalloc (Hybrid, Scalable, Lock-Free) | HMalloc: A Hybrid, Scalable, and Lock-Free Memory Allocator… | IEEE ICPADS 2019 | 10.1109/ICPADS47876.2019.00114 | nein | none | — | ✗ | medium |
| CAMAAllocator | CAMA (Predictable Cache-Aware Allocator) | CAMA: A Predictable Cache-Aware Memory Allocator | ECRTS 2011 | 10.1109/ECRTS.2011.10 | nein | none | — | ✗ | high |
| TCMallocWarehouseAllocator | TCMalloc-Warehouse/Hyperscale (hugepage-aware) | TEMERAIRE; Characterizing a Memory Allocator at Warehouse Scale | OSDI 2021; ASPLOS 2024 | usenix.org/.../hunter ; 10.1145/3620666.3651350 | OSS | Apache-2.0 | — | ✗ | medium |
| StdMalloc | Standard libc malloc (Baseline/Concept-Beweis) | — | — | — | OSS | Standard-Library | — | ✗ | high |
| VmemMagazinesAllocator | Vmem + Magazines (Slab-Erweiterung) | Magazines and Vmem: Extending the Slab Allocator to Many CPUs… | USENIX ATC 2001 | usenix.org/.../bonwick.html | nein | none | — | ✗ | high |

### axis_08_concurrency

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| NoneConcurrency | No synchronization (single-threaded baseline) | — | — | — | nein | none | — | ✗ | high |
| BlockingConcurrency | Coarse-grained mutex (pessimistic locking) | Solution of a Problem in Concurrent Programming Control | CACM 8(9) 1965 | 10.1145/365559.365617 | nein | none | — | ✗ | high |
| ReaderWriterConcurrency | Single-writer/multi-reader lock (shared_mutex) | Concurrent Control with "Readers" and "Writers" | CACM 14(10) 1971 | 10.1145/362759.362813 | nein | none | — | ✗ | high |
| OlcOptimisticConcurrency | Optimistic Lock Coupling (OLC) | The ART of Practical Synchronization; OLC (Verallgemeinerung) | DaMoN 2016; DEBULL 2019 | 10.1145/2933349.2933352 | lokal | Apache-2.0 | P08 | ✓ | high |
| LockFreeConcurrency | Lock-free via CAS-Loops (system-wide progress) | Simple, Fast, and Practical Non-Blocking… Concurrent Queue Algorithms | PODC 1996 | 10.1145/248052.248106 | nein | none | — | ✗ | medium |
| WaitFreeConcurrency | Wait-free synchronization (bounded per-thread steps) | Wait-Free Synchronization; Seqlock-Realisierung | TOPLAS 1991; Gelato 2005 | 10.1145/114005.102808 | nein | none | — | ✗ | medium |
| RcuConcurrency | Read-Copy-Update (deferred grace-period reclamation) | Read-Copy Update; User-Level Implementations of RCU | OLS 2001; TPDS 2012 | 10.1109/TPDS.2011.159 | lokal | LGPL-2.1-or-later | P29 | ✗ | high |
| HazardPointerConcurrency | Hazard Pointers (safe memory reclamation, ABA) | Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects | TPDS 15(6) 2004 | 10.1109/TPDS.2004.8 | lokal | none | P30 | ✗ | high |

### axis_filter

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| BloomFilter | Bloom Filter (k-hash bitmap, probabilistic membership) | Space/Time Trade-offs in Hash Coding with Allowable Errors | CACM 13(7) 1970 | 10.1145/362686.362692 | nein | none | — | ✗ | high |
| CuckooFilter | Cuckoo Filter (bucketed cuckoo hashing + fingerprints) | Cuckoo Filter: Practically Better Than Bloom | CoNEXT 2014 | 10.1145/2674005.2674994 | OSS | Apache-2.0 | — | ✓ | high |
| RangeSurfFilter | SuRF — Succinct Range Filter (LOUDS-Bitmap-Trie) | SuRF: Practical Range Query Filtering with Fast Succinct Tries | SIGMOD 2018 | 10.1145/3183713.3196931 | OSS | Apache-2.0 | P10 | ✓ | high |
| XorFilter | Xor Filter (XOR-Hash immutable filter, ~9 bits/key) | Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters | ACM JEA 25 2020 | 10.1145/3376122 | OSS | Apache-2.0 | — | ✓ | high |

### axis_09_isa

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| Aarch64Isa | AArch64 / ARMv8-A 64-bit ISA (A64) | ARM Architecture Reference Manual (DDI 0487) — Vendor-Spec | ARM Ltd., ab 2011/2013 | developer.arm.com/documentation/ddi0487/latest | nein | none | — | ✗ | high |
| Amd64Isa | x86-64 / AMD64 / Intel 64 ISA | AMD64 Programmer's Manual + Intel 64/IA-32 SDM — Vendor-Spec | AMD 2000 / Intel fortlaufend | docs.amd.com … ; intel.com/.../intel-sdm | nein | none | — | ✗ | high |
| PowerPcIsa | Power ISA (POWER9/10, ppc64le) | Power ISA Specification (v3.0/3.1) — Vendor/Standards-Spec | IBM/OpenPOWER; v3.0 2017, v3.1 2020 | openpowerfoundation.org/specifications/isa/ | nein | none | — | ✗ | high |
| RiscVIsa | RISC-V RV64GC Open-Source ISA | The RISC-V Instruction Set Manual — Standards-Spec (UCB TR Ursprung) | RISC-V Intl.; UCB TR ab 2011 | github.com/riscv/riscv-isa-manual | nein | none | — | ✗ | high |

### axis_09b_simd_extension

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| NoSimdExtension | No SIMD / scalar baseline | — | — | — | nein | none | — | ✗ | high |
| Sse2SimdExtension | Intel SSE2 (128-bit, x86_64 ABI-Baseline) | Intel 64/IA-32 SDM / ISA Ext Programming Reference (319433) | Intel Spec, 2000/2003 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | — | ✗ | high |
| Avx2SimdExtension | Intel AVX2 (256-bit, Haswell+ 2013) | Intel ISA Ext Programming Reference (319433); SDM | Intel Spec, 2013 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | — | ✗ | high |
| Avx512SimdExtension | Intel AVX-512 (512-bit + 15 Sub-Flags) | Intel ISA Ext Programming Reference (319433-060) | Intel Spec, 2016/2017 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | — | ✗ | high |
| NeonSimdExtension | ARM NEON / Advanced SIMD (AArch64 baseline, 128-bit) | ARM Architecture Reference Manual (NEON); Cortex-A8 TRM | ARM Spec, 2005 | developer.arm.com/.../intrinsics/ | nein | none | — | ✗ | high |
| Sve2SimdExtension | ARM SVE2 (ARMv9 mandatory, scalable 128-2048 bit) | The ARM Scalable Vector Extension (SVE-Fundamentalpaper) | IEEE Micro 37(2) 2017 | 10.1109/MM.2017.35 | nein | none | — | ✗ | high |
| RvvSimdExtension | RISC-V Vector Extension (RVV v1.0, scalable) | Vitruvius+ (akad. RVV-Impl); RVV Spec v1.0 (Standard) | ACM TACO 2023; RVV v1.0 2021 | 10.1145/3575861 ; github.com/riscvarchive/riscv-v-spec | nein | CC-BY-4.0 (Spec) | — | ✗ | high |
| CudaGh200SimdExtension | NVIDIA CUDA auf Hopper GH200 (massiv-parallel GPU) | NVIDIA Tesla: A Unified Graphics and Computing Architecture; GH200 Whitepaper | IEEE Micro 28(2) 2008; NVIDIA 2023 | 10.1109/MM.2008.31 ; resources.nvidia.com/.../grace-hopper | nein | none (CUDA proprietär) | — | ✗ | high |

### axis_12_general_hardware

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| X86_64HardwareProfile | x86-64 HW-Plattform-Profil (Build-Time-Konstanten) | — | — | — | nein | none | — | ✗ | high |
| Aarch64HardwareProfile | AArch64/ARM64 HW-Plattform-Profil (Build-Time-Konstanten) | — | — | — | nein | none | — | ✗ | high |
| GenericHardwareProfile | Generisches/konservatives HW-Profil (Fallback-Baseline) | — | — | — | nein | none | — | ✗ | high |

### axis_io

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| BufferedIo | Buffered I/O via OS Page-Cache | Operating System Support for Database Management | CACM 24(7) 1981 | 10.1145/358699.358703 | nein | none | — | ✗ | medium |
| DirectIo | Direct I/O (O_DIRECT, bypass Page-Cache) | — (Linux-Kernel-API-Flag, open(2)) | Linux 2.4.10 | man7.org/.../open.2.html | nein | none | — | ✗ | high |
| InMemoryOnly | In-Memory-Only (kein IO, RAM-only Baseline) | — | — | — | nein | none | — | ✗ | high |
| MmapIo | Memory-Mapped File I/O (mmap, file-backed) | Are You Sure You Want to Use MMAP in Your DBMS? (Anti-Pattern-Baseline) | CIDR 2022 | db.cs.cmu.edu/.../cidr2022-p13-crotty.pdf | OSS | MIT | — | ✗ | high |

### axis_05_memory_layout

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| AoSStrictMemoryLayout | Array-of-Structs (strict packed, dense object layout) | — (Folklore/Textbook) | — | — | ? | none | — | ✗ | high |
| CacheLineAlignedMemoryLayout | 64-byte cache-line aligned AoS (False-Sharing-Vermeidung) | — (HW-Standard, Patterson & Hennessy) | — | — | ? | none | — | ✗ | high |
| SoAMemoryLayout | Struct-of-Arrays (spaltenorientiert, SIMD-freundlich) | Column-Stores vs. Row-Stores: How Different Are They Really? (verwandt) | SIGMOD 2008 | 10.1145/1376616.1376712 | nein | none | — | ✗ | medium |
| PackedBitmapMemoryLayout | Bit-packed succinct Layout (LOUDS / SuRF FST) | Space-efficient Static Trees and Graphs (Jacobson LOUDS) | FOCS 1989 | 10.1109/SFCS.1989.63533 | OSS | Apache-2.0 (SuRF) / GPL-3.0 (SDSL) | P09 | ✗ | high |
| AoSoAMemoryLayout | Array-of-Structures-of-Arrays (Block-SoA+AoS Hybrid, SIMD-tiled, block_width=8) | — (HPC-/SIMD-Layout-Idiom, z.B. ISPC; kein einzelnes Ursprungspaper) | — | — | nein (Konvention) | none | — | ✗ | high |

### axis_migration

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| NoMigration | Static placement / No migration (baseline) | — | — | — | nein | none | — | ✗ | high |
| HotColdMigration | Hot/Cold-Daten-Separation | Anti-Caching: A New Approach to DBMS Architecture | PVLDB 6(14) 2013 | 10.14778/2556549.2556575 | OSS | GPL-3.0 | — | ✗ | high |
| TierBasedMigration | Multi-Tier-Migration RAM→NVM→SSD→HDD | Managing Non-Volatile Memory in Database Systems | SIGMOD 2018 | 10.1145/3183713.3196897 | ? | Apache-2.0 OR GPL-2.0 (RocksDB) | — | ✗ | high |
| AdaptiveMigration | ML-driven adaptive Migration (Online-Learning) | Driving Cache Replacement with ML-based LeCaR | USENIX HotStorage 2018 | usenix.org/.../vietri | OSS | LeCaR: none ; CacheLib: Apache-2.0 | — | ✗ | high |

### axis_02_path_compression

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| PathCompressionNone | Keine Path-Compression (raw path, Baseline) | — | — | — | nein | none | — | ✗ | high |
| ByteWisePathCompression | Byte-by-Byte Path-Compression (ART-Stil) | The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | OSS | Apache-2.0 | P01 | ✓ | high |
| PatriciaPathCompression | Single-Bit-Split Path-Compression (Patricia/HOT) | HOT: A Height Optimized Trie Index… (Ur-Technik: Morrison PATRICIA JACM 1968) | SIGMOD 2018; JACM 1968 | 10.1145/3183713.3196896 | OSS | ISC | P02 | ✓ | high |

### axis_04_node_type

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| Node4Layout | ART Node4 (4 slots, linear search) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| Node16Layout | ART Node16 (16 slots, SIMD binary search) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| Node48Layout | ART Node48 (48 slots, indirect 256-byte child-index) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| Node256Layout | ART Node256 (256 slots, direct-addressed) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |

### axis_07_prefetch

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| DistanceEstimatorPrefetch | Distance-Estimator software prefetch für ART | The Adaptive Radix Tree: ARTful Indexing… (kein dedizierter Prefetch-Algo im Paper) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 (unodb) / BSD-3 (libart) | P01 | ✗ | high |
| HardwarePrefetch | Explizite HW-Prefetch-Instr. (PREFETCHT0/T1/T2/NTA) | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | lokal | GPL-3.0 | P07 | ✗ | high |
| NonePrefetch | No prefetch (Mess-Baseline) | — | — | — | nein | none | — | ✗ | high |
| PathOrientedPrefetch | Path-oriented prefetch (pre-load entlang Trie-Pfad) | PRT-ART Diplomarbeit (eigene Arbeit); verwandt: Chen/Gibbons/Mowry SIGMOD 2001 | Diplomarbeit 2026; SIGMOD 2001 | 10.1145/375663.375688 (verwandt) | nein | none | P21 | ✗ | medium |

### axis_q1_queuing

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| NoBuffer | No-Buffer / Passthrough (Identität, Baseline) | — | — | — | nein | none | — | ✗ | high |
| FIFOQueueBuffer | FIFO-Queue (std::deque) | — (Lehrbuch-ADT) | — | — | nein | none | — | ✗ | high |
| LIFOStackBuffer | LIFO-Stack (std::vector) | — (Lehrbuch-ADT, Knuth TAOCP Vol.1) | — | — | nein | none | — | ✗ | high |
| AppendOnlyBuffer | Append-Only Linear Buffer (LSM/Bw-Tree-Anwendung) | — (Anwendungskontext Bw-Tree 2013 / LSM 1996) | — | — | nein | none | — | ✗ | high |
| PriorityHeapBuffer | Priority-Queue / Binary Max-Heap (std::priority_queue) | — (Heap: Williams Algorithm 232, CACM 1964) | CACM 1964 (allg.) | — | nein | none | — | ✗ | high |
| BoundedRingBuffer | Bounded Ring-Buffer (Disruptor-Pattern) | Disruptor: High performance alternative to bounded queues (LMAX) | LMAX Technical Paper 2011 | lmax-exchange.github.io/disruptor/files/Disruptor-1.0.pdf | OSS | Apache-2.0 | — | ✗ | high |
| DeltaChainBuffer | Delta-Chain Buffer (Bw-Tree Delta-Record-Versioning) | The Bw-Tree: A B-tree for New Hardware Platforms | ICDE 2013 | 10.1109/ICDE.2013.6544834 | nein | unknown (MS-intern) | — | ✗ | high |
| CopyOnWriteBuffer | Copy-on-Write Snapshot-Buffer (Persistent DS) | Making Data Structures Persistent | JCSS 38(1) 1989 (STOC 1986) | 10.1016/0022-0000(89)90034-2 | nein | none | — | ✗ | high |
| EpochBuffer | Epoch-based Buffer / QSBR | Read-Copy Update | OLS 2001 | kernel.org/doc/ols/2001/read-copy.pdf | lokal | LGPL-2.1 / GPL-2 | P29 | ✗ | high |
| SkiplistBuffer | Skip List (sortierter Buffer; CE-Impl via std::set) | Skip Lists: A Probabilistic Alternative to Balanced Trees | CACM 33(6) 1990 | 10.1145/78973.78977 | OSS | Public Domain (Pugh) / BSD-3 (RocksDB) | — | ✗ | high |
| TombstoneBuffer | Tombstone-Marker Buffer (LSM-Delete-Marker + MVCC) | The Log-Structured Merge-Tree (LSM-tree) | Acta Informatica 33(4) 1996 | 10.1007/s002360050048 | nein | none / BSD-3 (RocksDB) | — | ✗ | medium |
| BatchedInsertBuffer | Batched-Insert Buffer (Sub-Batch + bulk_insert) | (Header-Zitat unverifiziert; Anker: Leis ART ICDE 2013) | — | — | nein | none | — | ✗ | low |
| LockFreeSPSCBuffer | Lock-Free SPSC Ring-Queue (Lamport-Modell) | Specifying Concurrent Program Modules | TOPLAS 5(2) 1983 | 10.1145/69624.357207 | OSS | MIT / BSL-1.0 / Apache-2.0 | — | ✗ | medium |
| LockFreeMPMCBuffer | Lock-Free Bounded MPMC Queue (Vyukov per-Cell-Seq) | Bounded MPMC queue (Vyukov); akad.: Michael/Scott PODC 1996 | 1024cores.net 2010-11; PODC 1996 | 1024cores.net/.../bounded-mpmc-queue ; 10.1145/248052.248106 | OSS | BSD-2/BSD-style (Vyukov) | — | ✓ | high |
| OriginalLockFreeMpmcConcurrentQueue | moodycamel::ConcurrentQueue (Block-Based MPMC) | A Fast General Purpose Lock-Free Queue for C++ | moodycamel.com Blog 2014 | moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++ | OSS | BSD-2 / BSL-1.0 (dual) | — | ✓ | high |

### axis_q2_queuing

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| EagerFlush | Eager (write-through) Flush | — (Lehrbuch Write-Through-Policy) | — | — | nein | none | — | ✗ | high |
| LazyFlush | Lazy (write-back/deferred) Flush | — (Lehrbuch Write-Back-Policy) | — | — | nein | none | — | ✗ | high |
| WatermarkFlush | Watermark/Threshold-Flush (fill ≥ X%, Default 75%) | — (verwandt LSM-Tree Memtable-Trigger, O'Neil 1996) | Acta Informatica 1996 (verwandt) | 10.1007/s002360050048 | nein | none | — | ✗ | medium |
| TimedFlush | Zeit-Window (Micro-Batching) Flush | Kafka: a Distributed Messaging System…; verwandt D-Streams (Spark) | NetDB 2011; SOSP 2013 | cwiki.apache.org/.../Kafka-netdb-06-2011.pdf ; 10.1145/2517349.2522737 | ? | Apache-2.0 (Scala/Java) | — | ✗ | high |
| AdaptiveLsmFlush | Adaptiver Watermark-Flush via EWMA (selbst-tunend) | RocksDB Dynamic Leveled Compaction (kein Paper); LSM-Wurzel O'Neil 1996 | RocksDB Eng.; Acta Inf. 1996 | github.com/facebook/rocksdb/wiki/Leveled-Compaction ; 10.1007/s002360050048 | OSS | Apache-2.0 OR GPLv2 | — | ✗ | medium |

### axis_01_index_organization

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| ClusteredIndexOrganization | Clustered Index (storage-order = index-order) | Organization and Maintenance of Large Ordered Indexes (B-Tree-Wurzel) | Acta Informatica 1972 | 10.1007/BF00288683 | nein | none | — | ✗ | high |
| HeapIndexOrganization | Heap/Pile (unordered) File, kein Index, Full-Scan (Baseline) | — (Lehrbuch, Garcia-Molina/Ullman/Widom) | — | — | nein | none | — | ✗ | high |
| IotIndexOrganization | Index-Organized Table (Daten in B+Tree-Leaf-Pages) | Oracle8i Index-Organized Table and Its Application to New Domains | VLDB 2000 | vldb.org/conf/2000/P285.pdf | nein | none | — | ✗ | high |
| NonClusteredIndexOrganization | Non-Clustered (Secondary) Index (Pointer-Hop zur Row) | The Ubiquitous B-Tree (Survey); Wurzel Bayer/McCreight 1972 | CSUR 11(2) 1979 | 10.1145/356770.356776 | nein | none | — | ✗ | high |

### axis_10_serialization

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| RawBinarySerialization | Raw binary serialization (memcpy raw-byte layout) | — (Lehrbuch, Patterson & Hennessy) | — | — | nein | none | — | ✗ | high |
| CompressedSerialization | Block-Compression (LZ4/Snappy; theor. LZ77) | A Universal Algorithm for Sequential Data Compression | IEEE IT-23(3) 1977 | 10.1109/TIT.1977.1055714 | OSS | BSD-2 (LZ4) / BSD-3 (Snappy) | — | ✓ | high |
| SuccinctSerialization | Succinct bit-packed Encoding (LOUDS / FST) | SuRF (Fast Succinct Trie); Fundament Jacobson LOUDS | SIGMOD 2018; FOCS 1989 | 10.1145/3183713.3196931 ; 10.1109/SFCS.1989.63533 | OSS | Apache-2.0 | P10 | ✓ | high |
| VarLenSerialization | Variable-Length Integer Encoding (VarInt/VLQ/LEB128) | — (Standard-Technik; ART nutzt es, ist nicht Ursprung) | — | — | OSS | BSD-3 (Protobuf) / Apache-2.0 (unodb) | P01 | ✓ | medium |

### axis_11_telemetry

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| DensityTracker | Per-Node Density/Slot-Occupancy Tracking | The Adaptive Radix Tree… (kontextuell; kein benannter Telemetrie-Algo) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| InsertCounter | Globaler atomarer Insert-Zähler | HOT… (kontextuell; kein Insert-Counter-Algo) | SIGMOD 2018 | 10.1145/3183713.3196896 | lokal | MIT | P02 | ✗ | high |
| LatencyHistogram | HDR-Histogramm (p50/p95/p99 Lookup-Latenz) | HdrHistogram (Software/Konzept; Header zitiert fälschlich Wormhole) | ~2014 (Software) | github.com/HdrHistogram/HdrHistogram_c | OSS | CC0-1.0 | — | ✓ | high |
| LeafOnlyCounter | Counter nur in Blatt-Knoten (Anti-False-Sharing) | Towards Data-Based Cache Optimization of B+-Trees (Kuehn; beschreibt KEINEN Leaf-Counter) | DaMoN 2023 | 10.1145/3592980.3595303 | nein | none | P28 | ✗ | high |

### axis_03a_search_algo

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| Array256SearchAlgo | ART Node256 direct-addressed (CE-Re-Impl) | The Adaptive Radix Tree: ARTful Indexing… | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| OriginalArtSearchAlgo | ART — Original-Bindung (unodb::db) | The Adaptive Radix Tree: ARTful Indexing… | ICDE 2013 | 10.1109/ICDE.2013.6544812 | OSS | Apache-2.0 | P01 | ✓ | high |
| OriginalHotSearchAlgo | HOT — Height Optimized Trie (HOTRowex) | HOT: A Height Optimized Trie Index for Main-Memory Database Systems | SIGMOD 2018 | 10.1145/3183713.3196896 | OSS | ISC | P02 | ✓ | high |
| OriginalStartSearchAlgo | START — Self-Tuning Adaptive Radix Tree | START — Self-Tuning Adaptive Radix Tree | ICDEW 2020 | 10.1109/ICDEW49219.2020.00015 | OSS | MIT | P05 | ✓ | high |
| OriginalSurfSearchAlgo | SuRF — Succinct Range Filter (FST, LOUDS-DS) | SuRF: Practical Range Query Filtering with Fast Succinct Tries | SIGMOD 2018 | 10.1145/3183713.3196931 | OSS | Apache-2.0 | P10 | ✓ | high |
| OriginalWormholeSearchAlgo | Wormhole — Hash+Trie+B+ Hybrid Ordered Index | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | OSS | GPL-3.0 | P07 | ✗ | high |
| VectorU16U16SearchAlgo | Sorted u16-vector lower_bound (START-Konzept, vereinfacht) | START — Self-Tuning Adaptive Radix Tree | ICDEW 2020 | 10.1109/ICDEW49219.2020.00015 | OSS | MIT | P05 | ✗ | high |
| VectorU8U8SearchAlgo | Sparse sorted u8-vector lower_bound (HOT-k-Konzept) | HOT: A Height Optimized Trie Index… | SIGMOD 2018 | 10.1145/3183713.3196896 | OSS | ISC | P02 | ✗ | high |
| KArySearchAlgo | k-ary search — Such-METHODE (K-Wege-Partition, Aritaet K iterable) | k-ary search on modern processors (Schlegel/Gemulla/Lehner) | DaMoN 2009 | 10.1145/1565694.1565705 | nein (Pseudocode + Mess-Studie) | none | — | ✗ | high |
| InterpolationSearchAlgo | interpolation search — Such-METHODE (verteilungsbewusst, O(log log N) avg) | Interpolation search — a log log N search (Perl/Itai/Avni) | CACM 21(7) 1978 | 10.1145/359545.359557 | nein (Lehrbuch-Algorithmus) | none | — | ✗ | high |
| EytzingerSearchAlgo | Eytzinger/BFS-Layout-Suche — Such-METHODE (cache-conscious Layout, branch-free) | Array Layouts for Comparison-Based Searching (Khuong/Morin) | JEA 22 2017 | arXiv:1509.05053 | nein (Experiment-Harness, keine Standard-OSS-Lizenz) | none | — | ✗ | high |
| SkipListSearchAlgo | Skip-Liste — probabilistische geordnete STRUKTUR (O(log n) erwartet, kein Rebalancing) | Skip Lists: A Probabilistic Alternative to Balanced Trees (Pugh) | CACM 33(6) 1990 | 10.1145/78973.78977 | nein (Lehrbuch-Algorithmus) | none | — | ✗ | high |

### axis_03b_cache_traversal

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| LinearFanout | Linear-scan lookup auf kleinen Fanout-Arrays | Organization and maintenance of large ordered indexes | Acta Informatica 1972 | 10.1007/BF00288683 | nein | none | — | ✗ | high |
| HashLookup | Fibonacci-/multiplikatives Hashing, Open-Addressing Linear Probing | The Art of Computer Programming Vol.3 §6.4 (Hashing) | Addison-Wesley 2nd Ed. 1998 | en.wikipedia.org/wiki/Hash_function | nein | none | — | ✗ | high |

### axis_03m_mapping

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| DirectPlacement | Direct slot-to-absolute-offset mapping (linear packing) | Organization and Maintenance of Large Ordered Indexes (lose Lineage) | Acta Informatica 1(3) 1972 | 10.1007/BF00288683 | nein | none | — | ✗ | medium |
| PoolRelative | Pool-relative offset mapping (based-pointer, O(1) rebase) | Making Data Structures Persistent (lose Analogie) | JCSS 38(1) 1989 | 10.1016/0022-0000(89)90034-2 | nein | none | — | ✗ | medium |

### axis_14_value_handle

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | lokal P-ID | is_original | Conf. |
|---------|-------------|---------------|------------|---------|------------|--------|-----------|-------------|-------|
| InlineValueHandle | Inline value storage in node slot (combined pointer/value) | The Adaptive Radix Tree: ARTful Indexing… (Leaf Nodes §) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | P01 | ✗ | high |
| ExternalPoolValueHandle | Out-of-line value storage in pool (node hält Offset) | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | lokal | GPL-3.0 | P07 | ✗ | high |
| ImmutableSharedRefValueHandle | Immutable value via ref-counted Pointer (RCU-style snapshot) | Read-Copy Update: Using Execution History to Solve Concurrency Problems | PDCS 1998 | semanticscholar.org/.../21e51da40ab080ca2b71ad36094e2b686008b6cc | lokal | LGPL-2.1-or-later | P29 | ✗ | high |
| VersionedPointerValueHandle | Pointer mit Version-Tag (MVCC/optimistic; Masstree+SMART) | Cache Craftiness for Fast Multicore Key-Value Storage (Masstree) | EuroSys 2012 | 10.1145/2168836.2168855 | lokal | MIT | P03 | ✗ | high |

---

## §3 is_original-Linking-Kandidaten (für R7.6.c)

Alle Algorithmen mit `is_original_eligible = true` (20 Stück). Das ist die Auswahl für echtes Original-Code-Linking.

| Achse | Wrapper | Repo-URL | Lizenz |
|-------|---------|----------|--------|
| axis_06_allocator | MimallocAllocator | github.com/microsoft/mimalloc | MIT |
| axis_06_allocator | JemallocAllocator | github.com/jemalloc/jemalloc | BSD-2-Clause |
| axis_06_allocator | SnmallocAllocator | github.com/microsoft/snmalloc | MIT |
| axis_06_allocator | RPMallocAllocator | github.com/mjansson/rpmalloc | Public-Domain/MIT |
| axis_06_allocator | DlmallocAllocator | github.com/ennorehling/dlmalloc (Mirror); gee.cs.oswego.edu | Public-Domain (CC0) |
| axis_06_allocator | LRMallocAllocator | github.com/ricleite/lrmalloc | MIT |
| axis_08_concurrency | OlcOptimisticConcurrency | github.com/laurynas-biveinis/unodb | Apache-2.0 |
| axis_filter | CuckooFilter | github.com/efficient/cuckoofilter | Apache-2.0 |
| axis_filter | RangeSurfFilter | github.com/efficient/SuRF | Apache-2.0 |
| axis_filter | XorFilter | github.com/FastFilter/fastfilter_cpp | Apache-2.0 (xorfilter.h teils MIT) |
| axis_02_path_compression | ByteWisePathCompression | github.com/laurynas-biveinis/unodb | Apache-2.0 |
| axis_02_path_compression | PatriciaPathCompression | github.com/speedskater/hot | ISC |
| axis_q1_queuing | LockFreeMPMCBuffer | 1024cores.net (Vyukov); Mirror github.com/mstump/queues | BSD-2/BSD-style |
| axis_q1_queuing | OriginalLockFreeMpmcConcurrentQueue | github.com/cameron314/concurrentqueue | BSD-2 / BSL-1.0 (dual) |
| axis_10_serialization | CompressedSerialization | github.com/lz4/lz4 ; github.com/google/snappy | BSD-2 (LZ4) / BSD-3 (Snappy) |
| axis_10_serialization | SuccinctSerialization | github.com/efficient/SuRF | Apache-2.0 |
| axis_10_serialization | VarLenSerialization | github.com/protocolbuffers/protobuf ; github.com/laurynas-biveinis/unodb | BSD-3 / Apache-2.0 |
| axis_11_telemetry | LatencyHistogram | github.com/HdrHistogram/HdrHistogram_c | CC0-1.0 |
| axis_03a_search_algo | OriginalArtSearchAlgo | github.com/laurynas-biveinis/unodb | Apache-2.0 |
| axis_03a_search_algo | OriginalHotSearchAlgo | github.com/speedskater/hot | ISC |
| axis_03a_search_algo | OriginalStartSearchAlgo | github.com/jungmair/START | MIT |
| axis_03a_search_algo | OriginalSurfSearchAlgo | github.com/efficient/SuRF | Apache-2.0 |

> Hinweis: Es sind 22 Tabellenzeilen, weil zwei Kandidaten (CompressedSerialization, VarLenSerialization) je zwei Repos referenzieren; die Anzahl distinkter Wrapper mit `is_original_eligible = true` ist exakt **20**. Bereits mit echtem Original-Code-Linking aktiv (OriginalCodeMixin, is_original 2/2): mimalloc, jemalloc, snmalloc, rpmalloc, dlmalloc, lrmalloc (alle axis_06). Für `OriginalLockFreeMpmcConcurrentQueue` ist `is_original_module()=false` (nur 2/6 Methoden originall — put/get; emplace/peek_front/peek_back/clear sind Re-Impl-Lücken). Die Original*SearchAlgo-Wrapper sind aktuell s2-Standalone-Stubs; echtes Linking ist für s4 geplant.

---

## §4 Wrapper-Header-Korrekturen (KRITISCH)

Aus den `notes`-Feldern extrahierte falsche Attributionen in den Code-Kommentaren. Diese sind später im Code zu fixen.

| Achse / Wrapper | Code-Kommentar behauptet | Korrekt ist |
|-----------------|--------------------------|-------------|
| axis_06 / MichaelLockFreeAllocator | "PODC 2002 + JPDC 2004" | PLDI 2004 (ACM SIGPLAN) |
| axis_06 / SnmallocAllocator | Erstautor "Lipp/Bond/Parkinson" | Erstautor Paul Liétar (snmalloc, ISMM 2019) |
| axis_06 / ScallocAllocator | "Aigner/Iurca/Wimmer PPoPP 2015" | Aigner/Kirsch/Lippautz/Sokolova, OOPSLA 2015 |
| axis_06 / NUMAllocAllocator | "Linden/Liu/Williams ICDCS 2018" | Yang/Zhao/…/Liu, ISMM 2023 (UTSASRG) |
| axis_06 / StarMallocAllocator | "Dang/Charguer 2024" | Inria-Prosecco (Reitz/Fromherz/Protzenko et al.), OOPSLA 2024 |
| axis_06 / LRMallocAllocator | "JPDC 2019" | Leite/Rocha, VECPAR 2018 (LNCS 11333) |
| axis_06 / HMallocAllocator | "Tang 2020" | Li/Yao/Tang/Lin, ICPADS 2019 |
| axis_06 / CAMAAllocator | "Bhattacharyya/Beard/Cohen 2020" | Herter/Backes/Haupenthal/Reineke, ECRTS 2011 (Saarland) |
| axis_06 / PIMMallocAllocator | vage "UPMEM/HBM-PIM 2023+" | VIA-Research, HPCA 2026 / arXiv 2505.13002 (Präzisierung) |
| axis_06 / ExgenAllocator | vage "Exception-Generated Single-Thread" | "Old is Gold", UT Austin, IEEE CAL 2025 (Präzisierung) |
| axis_filter / RangeSurfFilter | Lizenz "BSD-3-Clause" | Apache-2.0 (efficient/SuRF LICENSE geprüft) |
| axis_filter / XorFilter | Lizenz "MIT" | Repo-LICENSE Apache-2.0 (xorfilter.h teils MIT; beide permissiv) |
| axis_05 / PackedBitmapMemoryLayout | (PAPER_REFERENCES.md: SuRF "BSD-3") | SuRF Apache-2.0 |
| axis_migration / AdaptiveMigration | PAPER_REFERENCES.md §2.4: "Database Cracking (Idreos CIDR 2007)" | LeCaR (Vietri HotStorage 2018) + CacheLib (Berg OSDI 2020) — Block-Migration, nicht Index-Cracking |
| axis_io / BufferedIo | PAPER_REFERENCES.md Z.18: DOI 10.1145/358769.358773 | DOI 10.1145/358699.358703 (Stonebraker CACM 1981) |
| axis_02 / PatriciaPathCompression | Wormhole "SIGMOD 2019" | Wormhole EuroSys 2019 (10.1145/3302424.3303955) |
| axis_07 / HardwarePrefetch | Wormhole-Lizenz "BSD-2-Clause" (PAPER_REFERENCES.md §2.2 + Header) | GPL-3.0 (verifiziert) — blockiert Linking |
| axis_07 / PathOrientedPrefetch | verwandte Lit "Tan, Knoll, IEEE TVLSI 2012" (nicht verifizierbar) | Entfernen; korrekt: Chen/Gibbons/Mowry SIGMOD 2001 (P21) + Mowry 1994 |
| axis_q2 / AdaptiveLsmFlush | "Levandoski 2013" (= Bw-Tree, kein LSM-Flush) | Fehlattribution entfernen; konzeptuell RocksDB Dynamic Level + O'Neil 1996 |
| axis_11 / InsertCounter | Venue "PVLDB 2018" (HOT) | SIGMOD 2018 |
| axis_11 / LatencyHistogram | "Wormhole, Wu, SIGMOD 2019" (doppelt falsch) | HdrHistogram (Gil Tene); Wormhole ist EuroSys 2019 und misst keine Latenz-Histogramme |
| axis_11 / LeafOnlyCounter | "Kuehn DaMoN 2023 X1: Counter nur in Blatt-Knoten" | Fehlattribution; Kuehn-Paper behandelt cache-freundl. B+-Layout, KEINEN Leaf-Counter |
| axis_03a / OriginalHotSearchAlgo | "PVLDB 11(3):274-286, 2018" | SIGMOD 2018 (erw. Version ACM TODS 2022) |
| axis_03a / OriginalStartSearchAlgo | "START (Mertens et al., ICDE 2024)" | Fent/Jungmair/Kipf/Neumann, ICDEW 2020 |
| axis_03a / VectorU16U16SearchAlgo | "Mertens ICDE 2024" | Fent et al., ICDEW 2020 (gleicher Fehler wie OriginalStartSearchAlgo) |
| axis_03a / OriginalWormholeSearchAlgo | Venue "USENIX ATC 2019" | EuroSys 2019 |
| axis_03a / OriginalSurfSearchAlgo | (Header-Deps suggerieren Standalone) | Mehr-Header-Bibliothek, kein Single-Header (kein Bug, Klarstellung) |
| axis_03a / VectorU8U8SearchAlgo | HOT Venue ungenau ("PVLDB 2018") | SIGMOD 2018 |
| axis_14 / InlineValueHandle | PAPER_REFERENCES.md §2.1: Rao/Ross CSS-Tree (P11) | Leis ART ICDE 2013 (P01) |
| axis_14 / ExternalPoolValueHandle | Wormhole "SIGMOD 2019" (Header) + PAPER_REFERENCES.md §2.2: "Oracle In-Memory, kein Paper" | Wormhole EuroSys 2019 (P07) |
| axis_14 / VersionedPointerValueHandle | PAPER_REFERENCES.md §2.4: Hazard Pointers (Michael TPDS 2004, P30) — andere Technik | Masstree (Mao EuroSys 2012, P03) bzw. SMART (OSDI 2023, nicht 2022) |

---

## §5 Offene/unsichere Punkte

Algorithmen mit `confidence = low/medium` ODER `c_cpp_code_exists = unknown` ODER `paper_found = false` ohne Baseline-Charakter.

| Achse / Wrapper | Conf. | Code | Kurz-Notiz |
|-----------------|-------|------|------------|
| axis_06 / NUMAllocAllocator | medium | OSS | Lizenz nicht eindeutig (404 auf raw LICENSE); vermutl. Apache-2.0 |
| axis_06 / ExgenAllocator | medium | ? | Paper sicher (CAL 2025), aber kein öffentliches Repo, Algorithmus-Identität zum Wrapper unklar |
| axis_06 / HMallocAllocator | medium | nein | Paper sicher (ICPADS 2019), kein öffentlicher Code, Wrapper-Beschreibung generisch |
| axis_06 / TCMallocWarehouseAllocator | medium | OSS | Marketing-Variante von TCMalloc, kein eigenständiges Artefakt (identisch zu A06) |
| axis_08 / LockFreeConcurrency | medium | nein | Generisches CAS-Pattern, kein einzelner Algorithmus; Michael/Scott PODC 1996 als Anker |
| axis_08 / WaitFreeConcurrency | medium | nein | Generisches Pattern (seqlock); Herlihy TOPLAS 1991 als Theorie-Anker |
| axis_io / BufferedIo | medium | nein | Engineering-Pattern; Stonebraker 1981 ist Konzept-/Position-Paper, kein Algo-Code |
| axis_05 / SoAMemoryLayout | medium | nein | SoA hat kein kanonisches Ursprungspaper; Abadi SIGMOD 2008 nur "verwandt/Motivation" |
| axis_q1 / TombstoneBuffer | medium | nein | Tombstone kein einzelnes Paper; LSM-Kontext (O'Neil 1996) |
| axis_q1 / BatchedInsertBuffer | **low** | nein | "Lopez-Pesch ICDE 2024" NICHT verifizierbar (vermutl. erfunden); paper_found=false bis geklärt |
| axis_q1 / LockFreeSPSCBuffer | medium | OSS | Lamport 1983 ist üblicher Zitat-Anker, beschreibt aber nicht wörtlich "SPSC-Ring" |
| axis_q2 / WatermarkFlush | medium | nein | Kein dediziertes Watermark-Flush-Paper; nur verwandtes LSM-Konzept |
| axis_q2 / TimedFlush | high | ? | Paper sicher (Kafka/Spark), aber Referenzimpl Scala/Java — kein C/C++-Standalone |
| axis_q2 / AdaptiveLsmFlush | medium | OSS | EWMA-Logik CE-eigen; "Levandoski 2013" Fehlattribution; kein dediziertes Paper |
| axis_migration / TierBasedMigration | high | ? | van-Renen-NVM-Code nicht als Repo auffindbar; RocksDB als praktische Referenz |
| axis_05 / AoSStrict… , CacheLineAligned… | high | ? | Layout-Konventionen, kein Algorithmus-Code; c_cpp_code_exists=unknown |
| axis_03m / DirectPlacement | medium | nein | Bayer/McCreight 1972 nur lose Lineage; Wrapper triviale CE-Baseline |
| axis_03m / PoolRelative | medium | nein | Driscoll 1989 nur lose Analogie; eigentlich Standard-Arena/Based-Pointer-Idiom |

Weitere `paper_found=false`-Einträge sind durchgehend bewusste **Baselines/Lehrbuch-Konzepte** (z.B. NoBuffer, NoMigration, NonePrefetch, alle ISA-/SIMD-/HW-Profile, Eager/LazyFlush, Heap-Org, FIFO/LIFO/Stack-ADTs) — kein Klärungsbedarf, korrekt als Vergleichs-Nullpunkte gekennzeichnet.

---

## §6 Cross-Ref lokaler Katalog (Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md)

Vorkommende `local_forschung_id` (P01-P33) in dieser Map:

| P-ID | Repo/Konzept | Verwendet in Achsen |
|------|--------------|---------------------|
| P01 | ART / unodb (Leis ICDE 2013, Apache-2.0) | axis_02, axis_04 (N4/16/48/256), axis_07, axis_10, axis_11, axis_03a, axis_14 |
| P02 | HOT / hot (Binna SIGMOD 2018, ISC) | axis_02, axis_11, axis_03a |
| P03 | Masstree / masstree-beta (Mao EuroSys 2012, MIT) | axis_14 |
| P05 | START / START (Fent ICDEW 2020, MIT) | axis_03a |
| P07 | Wormhole / wormhole (Wu EuroSys 2019, GPL-3.0) | axis_07, axis_03a, axis_14 |
| P08 | ARTSync / OLC in unodb (Leis DaMoN 2016, Apache-2.0) | axis_08 |
| P09 | Jacobson LOUDS (FOCS 1989) — Originalpaper-Konzept, KEIN Code | axis_05 |
| P10 | SuRF / FST (Zhang SIGMOD 2018, Apache-2.0) | axis_filter, axis_10, axis_03a |
| P21 | Chen/Gibbons/Mowry pB+-Trees Prefetching (SIGMOD 2001) | axis_07 (verwandt) |
| P28 | Kuehn Cache-Optimization B+-Trees (DaMoN 2023) — TU-Dortmund-intern, KEIN public Code | axis_11 |
| P29 | userspace-rcu / liburcu (McKenney OLS 2001, LGPL-2.1) | axis_08, axis_q1, axis_14 |
| P30 | huangjiahua/haz_ptr (Michael TPDS 2004, NO LICENSE) | axis_08 |

**Wichtiger Hinweis:** Die **Allocator-Achse (axis_06)** ist NICHT im P01-P33-Katalog (REPO_INVENTAR_FINAL.md enthält ausschließlich Tree/Trie/Prefetch/Concurrency-DBMS-Paper). Die Allokatoren liegen in einem separaten Bereich (`ext/allocator/A01–A23` mit eigenem REPOS_OVERVIEW). Ebenso ohne P-ID: alle ISA-/SIMD-/HW-Profile (axis_09/09b/12), die IO-, Layout-, Serialization-, Index-Organization- und Queuing-Achsen — diese referenzieren externe OSS-Repos oder sind reine CE-Baselines/Standards.

**Nicht im Katalog, aber Klon empfohlen (is_original-Kandidaten ohne P-ID):** CuckooFilter (efficient/cuckoofilter), XorFilter (FastFilter/fastfilter_cpp), HdrHistogram_c, Vyukov-MPMC, moodycamel concurrentqueue (bereits vendored unter ext/queuing/Q01), LZ4/Snappy, Protobuf-VarInt.
