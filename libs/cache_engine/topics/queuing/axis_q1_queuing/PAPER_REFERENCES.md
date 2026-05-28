# axis_q1_queuing — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix A/D/E — D/E (Lehrbuch-ADTs + Engineering-Baselines: NoBuffer, FIFO, LIFO, Append-Only, Priority-Heap, Bounded-Ring, Delta-Chain, Copy-on-Write, Epoch, Skiplist, Tombstone, Batched-Insert, Lock-Free-SPSC) mit zwei A-Standalone-faehigen is_original-Kandidaten (OSS+permissiv: LockFreeMPMCBuffer, OriginalLockFreeMpmcConcurrentQueue).

## §1 Pflicht-Note
Zwei Wrapper sind is_original_eligible (OSS, permissiv): `LockFreeMPMCBuffer` (Vyukov Bounded-MPMC, BSD-2/BSD-style) und `OriginalLockFreeMpmcConcurrentQueue` (moodycamel::ConcurrentQueue, BSD-2 / BSL-1.0 dual). Fuer `OriginalLockFreeMpmcConcurrentQueue` ist `is_original_module()=false`, da nur 2/6 Methoden (put/get) original gebunden sind (emplace/peek_front/peek_back/clear sind Re-Impl-Luecken). Alle uebrigen Wrapper sind Re-Impl / Lehrbuch-ADTs / Baselines ohne Original-Code-Linking (`std::deque`, `std::vector`, `std::priority_queue`, `std::set` etc.).

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| NoBuffer | No-Buffer / Passthrough (Identität, Baseline) | — | — | — | nein | none | ✗ |
| FIFOQueueBuffer | FIFO-Queue (std::deque) | — (Lehrbuch-ADT) | — | — | nein | none | ✗ |
| LIFOStackBuffer | LIFO-Stack (std::vector) | — (Lehrbuch-ADT, Knuth TAOCP Vol.1) | — | — | nein | none | ✗ |
| AppendOnlyBuffer | Append-Only Linear Buffer (LSM/Bw-Tree-Anwendung) | — (Anwendungskontext Bw-Tree 2013 / LSM 1996) | — | — | nein | none | ✗ |
| PriorityHeapBuffer | Priority-Queue / Binary Max-Heap (std::priority_queue) | — (Heap: Williams Algorithm 232, CACM 1964) | CACM 1964 (allg.) | — | nein | none | ✗ |
| BoundedRingBuffer | Bounded Ring-Buffer (Disruptor-Pattern) | Disruptor: High performance alternative to bounded queues (LMAX) | LMAX Technical Paper 2011 | lmax-exchange.github.io/disruptor/files/Disruptor-1.0.pdf | OSS | Apache-2.0 | ✗ |
| DeltaChainBuffer | Delta-Chain Buffer (Bw-Tree Delta-Record-Versioning) | The Bw-Tree: A B-tree for New Hardware Platforms | ICDE 2013 | 10.1109/ICDE.2013.6544834 | nein | unknown (MS-intern) | ✗ |
| CopyOnWriteBuffer | Copy-on-Write Snapshot-Buffer (Persistent DS) | Making Data Structures Persistent | JCSS 38(1) 1989 (STOC 1986) | 10.1016/0022-0000(89)90034-2 | nein | none | ✗ |
| EpochBuffer | Epoch-based Buffer / QSBR | Read-Copy Update | OLS 2001 | kernel.org/doc/ols/2001/read-copy.pdf | lokal | LGPL-2.1 / GPL-2 | ✗ |
| SkiplistBuffer | Skip List (sortierter Buffer; CE-Impl via std::set) | Skip Lists: A Probabilistic Alternative to Balanced Trees | CACM 33(6) 1990 | 10.1145/78973.78977 | OSS | Public Domain (Pugh) / BSD-3 (RocksDB) | ✗ |
| TombstoneBuffer | Tombstone-Marker Buffer (LSM-Delete-Marker + MVCC) | The Log-Structured Merge-Tree (LSM-tree) | Acta Informatica 33(4) 1996 | 10.1007/s002360050048 | nein | none / BSD-3 (RocksDB) | ✗ |
| BatchedInsertBuffer | Batched-Insert Buffer (Sub-Batch + bulk_insert) | (Header-Zitat unverifiziert; Anker: Leis ART ICDE 2013) | — | — | nein | none | ✗ |
| LockFreeSPSCBuffer | Lock-Free SPSC Ring-Queue (Lamport-Modell) | Specifying Concurrent Program Modules | TOPLAS 5(2) 1983 | 10.1145/69624.357207 | OSS | MIT / BSL-1.0 / Apache-2.0 | ✗ |
| LockFreeMPMCBuffer | Lock-Free Bounded MPMC Queue (Vyukov per-Cell-Seq) | Bounded MPMC queue (Vyukov); akad.: Michael/Scott PODC 1996 | 1024cores.net 2010-11; PODC 1996 | 1024cores.net/.../bounded-mpmc-queue ; 10.1145/248052.248106 | OSS | BSD-2/BSD-style (Vyukov) | ✓ |
| OriginalLockFreeMpmcConcurrentQueue | moodycamel::ConcurrentQueue (Block-Based MPMC) | A Fast General Purpose Lock-Free Queue for C++ | moodycamel.com Blog 2014 | moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++ | OSS | BSD-2 / BSL-1.0 (dual) | ✓ |

## §3 Compliance-Status
Alle 15 Wrapper haben entweder eine Paper-Referenz oder sind als Baseline/Lehrbuch-ADT gekennzeichnet → Habich-Pflicht erfuellt.

**is_original-Kandidaten (Map §3, fuer R7.6.c Original-Code-Linking):**
- `LockFreeMPMCBuffer` — Vyukov Bounded-MPMC, 1024cores.net; Mirror github.com/mstump/queues; BSD-2/BSD-style.
- `OriginalLockFreeMpmcConcurrentQueue` — moodycamel ConcurrentQueue, github.com/cameron314/concurrentqueue; BSD-2 / BSL-1.0 (dual); bereits vendored unter ext/queuing/Q01. Aktuell `is_original_module()=false` (nur 2/6 Methoden original gebunden: put/get).

**Offene/unsichere Punkte dieser Achse (Map §5):**
- `TombstoneBuffer` (confidence medium): Tombstone kein einzelnes Paper; LSM-Kontext (O'Neil 1996).
- `BatchedInsertBuffer` (confidence **low**): Header-Zitat "Lopez-Pesch ICDE 2024" NICHT verifizierbar (vermutlich erfunden); paper_found=false bis geklaert.
- `LockFreeSPSCBuffer` (confidence medium): Lamport 1983 ist ueblicher Zitat-Anker, beschreibt aber nicht woertlich "SPSC-Ring".

**Lizenz-blockierte:** keine in dieser Achse (kein GPL-3.0-Wrapper wie Wormhole). `EpochBuffer` referenziert lokalen liburcu-Code (LGPL-2.1 / GPL-2, P29) — kein is_original-Linking.

Keine §4-Korrektur betrifft einen Wrapper dieser Achse direkt.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_q1_queuing, §3 is_original-Kandidaten, §5 offene Punkte, §6 P-ID-Katalog)
- Doku 17 §4.5 (Klassifikation A/C/D/E)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md: P29 = userspace-rcu / liburcu (McKenney OLS 2001, LGPL-2.1) — verwendet in EpochBuffer
- Vendored OSS ohne P-ID: moodycamel concurrentqueue (ext/queuing/Q01), Vyukov-MPMC
