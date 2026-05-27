# 17 — Paper-Kartografie R7.6.b: Mass-Migration-Plan

**Stand:** 2026-05-27
**Status:** OFFEN — Roadmap fuer R7.6.b Sprints (Tasks #728-#735)
**Verwandt:** Doku 13 (Paper-Legacy-Architektur), [[forschungsarbeiten-paper-repository]]

## §1 Kontext

User-Direktive 2026-05-27 (erweitert): "Alle Achsen, die nicht auch eine
erweiterte wissenschaftliche Recherche zu jedem Algorithmus einbezogen haben,
müssen diese nachholen. Die Paper müssen gefunden und nach topics und Achsen
im paper Ordner katalogisiert werden. Dann dasselbe Verfahren wie in der
Allocator Achse."

Diese Doku konsolidiert das Mapping zwischen:
- `Diplomarbeit/Forschungsarbeiten/code/P01-P33` (kuratierte Repos)
- `cache-engine/libs/.../legacy_code/paper_<id>_<name>/` (Pflicht-Ziel)

## §2 Forschungsarbeiten-Inventar (33 Paper-Code-Quellen)

### §2.1 Open-Source-Repos (11 mit Code)

| P-ID | Paper | Repo | License | Topic-Zuordnung |
|------|-------|------|---------|------------------|
| P01 | ART (Leis ICDE 2013) | unodb | Apache-2.0 | traversal + prefetch (Distance-Estimator) |
| P02 | HOT (Binna ICDE 2018) | speedskater/hot | ISC | traversal |
| P03 | Masstree (Mao EuroSys 2012) | kohler/masstree-beta | MIT | traversal |
| P04 | CoCo-Trie (Boffa 2024) | aboffa/CoCo-trie | GPL-3.0 | traversal |
| P05 | START (Fent CIDR 2020) | jungmair/START | MIT | traversal |
| P07 | Wormhole (Wu EuroSys 2019) | wuxb45/wormhole | GPL-3.0 | traversal + prefetch (PREFETCHT0) |
| P10 | SuRF (Zhang SIGMOD 2018) | efficient/SuRF | Apache-2.0 | traversal + filter (Range-Filter) |
| P20 | B-Trees Are Back (Mueller 2025) | leanstore/leanstore | MIT | traversal + memory_layout |
| P25 | hpides/prefetching (Mahling 2025) | hpides/prefetching | NO LICENSE | prefetch |
| P29 | RCU (McKenney OLS 2001) | urcu/userspace-rcu | LGPL-2.1 | value_handle (ImmutableSharedRef) |
| P30 | Hazard Pointers (Michael TPDS 2004) | huangjiahua/haz_ptr | NO LICENSE | value_handle (VersionedPointer) |

### §2.2 ALT-OHNE-REPO (14, eigene Implementation noetig)

P06, P11-P14, P16-P19, P21-P24, P26-P27. Pseudocode-only oder
proprietaer/intern. Pattern: Re-Impl + is_original_module = false.

### §2.3 INSTITUTION-INTERN (6, nicht oeffentlich)

P28 (TU Dortmund DBIS), P06 (TUM), P31-P32 (TU Dresden), P33 (SPP2377 Poster).
Pattern: Re-Impl + is_original_module = false.

### §2.4 ORIGINALPAPER-KONZEPT (2, kein Code)

P09 (Jacobson LOUDS 1989), P15 (Graefe Survey 2001). Pattern: Doku-only.

## §3 R7.5-Achsen Paper-Mapping

### §3.1 axis_07_prefetch (4 Wrappers, ~6 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| NonePrefetch | Baseline | N/A | OK |
| HardwarePrefetch | P07-Wormhole | wh.c (GPL-3.0) | Phase 1 done (Commit 481a4d9) |
| DistanceEstimatorPrefetch | P01-ART | art_internal.cpp (Apache-2.0) | Phase 1 done |
| PathOrientedPrefetch | (eigene Diplomarbeit) | N/A | OK |

**Offen (Stufe 2-4):** PrefetchOriginalCodeMixin Concept + CMake + Wrapper-Inheritance + sha256-lock.

### §3.2 axis_filter (4 Wrappers, ~8 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| BloomFilter | Bloom CACM 1970 | N/A (klassisch, Pseudocode) | PAPER_REFERENCES.md done |
| CuckooFilter | Fan CoNEXT 2014 | github.com/efficient/cuckoofilter (Apache-2.0) | TBD: ext-Repo cloning |
| XorFilter | Graf+Lemire JEA 2020 | github.com/FastFilter/fastfilter_cpp (MIT) | TBD |
| RangeSurfFilter | Zhang SIGMOD 2018 | P10-SuRF (Apache-2.0, schon kuratiert) | TBD: legacy_code Kopie |

### §3.3 axis_03a_search_algo (8 Wrappers, ~25 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| Array256SearchAlgo | Re-Impl | N/A | OK |
| VectorU8U8SearchAlgo | Re-Impl | N/A | OK |
| VectorU16U16SearchAlgo | Re-Impl | N/A | OK |
| OriginalArtSearchAlgo | P01-ART | unodb (Apache-2.0) | legacy_code teilweise done |
| OriginalHotSearchAlgo | P02-HOT | speedskater/hot (ISC) | TBD |
| OriginalStartSearchAlgo | P05-START | jungmair/START (MIT) | legacy_code teilweise done |
| OriginalWormholeSearchAlgo | P07-Wormhole | wuxb45/wormhole (GPL-3.0) | TBD |
| OriginalSurfSearchAlgo | P10-SuRF | efficient/SuRF (Apache-2.0) | TBD |

### §3.4 axis_14_value_handle (4 Wrappers, ~6 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| InlineValueHandle | Rao+Ross VLDB 1999 (CSS-Tree, alt) | N/A | OK |
| ExternalPoolValueHandle | Oracle In-Memory Standard | N/A | OK |
| ImmutableSharedRefValueHandle | McKenney OLS 2001 (RCU) | P29-urcu (LGPL-2.1) | TBD: Kopie |
| VersionedPointerValueHandle | Michael TPDS 2004 (Hazard Pointers) | P30-haz_ptr (NO LICENSE) | TBD: Re-Impl + Doku |

### §3.5 axis_05_memory_layout (4 Wrappers, ~5 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| CacheLineAlignedMemoryLayout | Patterson&Hennessy (Lehrbuch) | N/A | OK |
| AoSStrictMemoryLayout | Klassisch | N/A | OK |
| SoAMemoryLayout | Klassisch | N/A | OK |
| PackedBitmapMemoryLayout | Jacobson FOCS 1989 (LOUDS) | P09 Originalpaper, kein Code | OK |

**Verwandte Papers (Doku-only):** P16-Bender Tree Layout, P17-Bender Cache-Oblivious.

### §3.6 axis_10_serialization (4 Wrappers, ~6 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| RawBinarySerialization | Patterson&Hennessy | N/A | OK |
| SuccinctSerialization | Jacobson FOCS 1989 + SDSL-Lite Gog SEA 2014 | SDSL-Lite (GPLv3) | TBD: ext-Repo |
| CompressedSerialization | LZ77 Ziv+Lempel IEEE-IT 1977 + zstd Collet 2015 | facebook/zstd (BSD-3) | TBD: ext-Repo |
| VarLenSerialization | Google Protobuf | protocolbuffers/protobuf (BSD-3) | TBD: ext-Repo |

### §3.7 axis_01_index_organization (4 Wrappers, ~4 SP)

Alle derivative-Patterns von Bayer+McCreight Acta Informatica 1972
"Organization and Maintenance of Large Ordered Indexes".

| Wrapper | Status |
|---------|--------|
| HeapIndexOrganization | Garcia-Molina textbook (Doku-only) |
| ClusteredIndexOrganization | Engineering-Pattern Microsoft/Oracle/Tandem (Doku-only) |
| NonClusteredIndexOrganization | Comer ACM CSUR 1979 "Ubiquitous B-Tree" (Doku) |
| IotIndexOrganization | Oracle 8i 1997 commercial (Doku-only) |

### §3.8 axis_11_telemetry (4 Wrappers, ~4 SP)

| Wrapper | Paper | Code | Stufe-Status |
|---------|-------|------|--------------|
| DensityTracker | Praxis-Heuristik | N/A | OK |
| InsertCounter | Atomic-Standard | N/A | OK |
| LatencyHistogram | Tene 2014 HDR Histogram (Engineering) | giltene/HdrHistogram (CC0+BSD-2) | TBD: ext-Repo |
| LeafOnly | Praxis-Optimierung | N/A | OK |

### §3.9 axis_migration (4 Wrappers, ~6 SP — Web-Recherche AUSSTEHEND)

Recherche-Agent fuer R7.6.b axis_migration noch nicht gestartet.

**Vermutete Papers:**
- HotColdMigration: HotStore VLDB?
- TierBasedMigration: DRAM-NVM-Disk Tiering (Stoica VLDB 2013? Kuehn DAMON 2023 = P28)
- AdaptiveMigration: Self-Tuning DBs (Chaudhuri SIGMOD 2007?)

### §3.10 axis_io (4 Wrappers, ~6 SP — Web-Recherche AUSSTEHEND)

Recherche-Agent fuer R7.6.b axis_io noch nicht gestartet.

**Vermutete Papers:**
- BufferedIo: OS-Standard (Stevens UNIX Programming?)
- DirectIo: Linux Kernel O_DIRECT (kein DBMS-Paper)
- InMemoryOnly: Baseline
- MmapIo: Stonebraker CACM 1981?

## §4 Mass-Migration-Plan (10 Sprints, ~76 SP)

### §4.1 Reihenfolge nach Aufwand (klein zuerst)

1. **Sprint 1 (axis_07):** 6 SP — Phase 1 done, Stufe 2-4 (Mixin+CMake+Wrapper+sha256) offen
2. **Sprint 2 (axis_11_telemetry):** 4 SP — nur HDR-Histogram ext-Repo
3. **Sprint 3 (axis_01_index_organization):** 4 SP — alles derivative-Patterns, Doku-only
4. **Sprint 4 (axis_05_memory_layout):** 5 SP — Pseudocode/klassisch
5. **Sprint 5 (axis_14_value_handle):** 6 SP — P29+P30 schon kuratiert
6. **Sprint 6 (axis_10_serialization):** 6 SP — 3 ext-Repos (SDSL/zstd/protobuf)
7. **Sprint 7 (axis_io):** 6 SP — Web-Recherche + Skelette
8. **Sprint 8 (axis_migration):** 6 SP — Web-Recherche + Skelette
9. **Sprint 9 (axis_filter):** 8 SP — 3 ext-Repos (cuckoofilter/fastfilter_cpp) + P10-SuRF
10. **Sprint 10 (axis_03a):** 25 SP — 5 Original-Code-Wrappers (P01/P02/P05/P07/P10)

**Gesamt:** ~76 SP, ~3-4 Tage autonomes Arbeiten.

### §4.2 Pro Sprint Stufen (5)

| Stufe | Aktion | Aufwand pro Achse |
|-------|--------|-------------------|
| 1 | legacy_code Skelette + Source-Kopie + manifest.txt + README.md | 1-2 SP |
| 2 | OriginalCodeMixin Concept (analog AllocatorOriginalCodeMixin) | 1 SP |
| 3 | CMakeLists.txt: comdare_register_paper_wrapper(...) Aufrufe | 1 SP |
| 4 | Wrapper-Headers Inheritance + is_original_<fn>() Verifikation | 1-2 SP |
| 5 | Build + Test + sha256_locked.txt Commit | 1 SP |

### §4.3 Stufe 1 Pattern (Standard)

```
libs/cache_engine/topics/<topic>/<axis>/legacy_code/paper_<id>_<name>/
  LICENSE              (copy from Forschungsarbeiten/code/P##/<repo>/LICENSE)
  manifest.txt         (@compiler, @has_original_paper_code, @axis_mixin_type +
                        wrapper_fn paper_fn source_relative_path Mappings)
  README.md            (Paper-Reference + Source-Link + Stufen-Status)
  src/
    <function-file>.c/.cpp/.hpp  (relevant Source-Files)
```

### §4.4 Cross-Wrapper Schwierigkeiten

- **P29 RCU LGPL-2.1**: Dynamic-Linking-Only License, statische Verlinkung
  problematisch. Alternative: F2-Beschluss = eigene RCU-Implementierung
  (Task #652).
- **P30 NO LICENSE**: github.com/huangjiahua/haz_ptr hat KEINE Lizenz →
  rechtlich nicht direkt verwendbar. Alternative: folly Hazard Pointers
  oder eigene Re-Impl.
- **P04 CoCo-Trie GPL-3.0**: viral License, kann nicht in BSD/MIT-Engine
  linken. Architekt-Direktive: "OK", Sonderfall.

## §5 Naechste Schritte (User-Entscheidung)

Nach Doku-Commit + Submodule-Bump entscheidet User:

- **A) Sprint 2 (axis_11_telemetry, 4 SP):** schnellster Quick-Win
- **B) Sprint 10 (axis_03a, 25 SP):** groesster Impact, aber lange
- **C) axis_07 Pilot Stufe 2-4 vervollstaendigen (6 SP):** vor Roll-out
- **D) ext-Submodules cloning planen:** strategisch fuer mehrere Sprints

## §6 Cross-Refs

- Doku 13 — Paper-Legacy-Architektur (4-Schichten)
- Doku 15 — ISA-Schichten + Paper-Backlog R7.6/R7.7
- Doku 16 — axis_05 CPU IMC Runtime-Heuristik
- Memory: [[forschungsarbeiten-paper-repository]] (Pflicht-Pfad)
- Memory: [[paper-original-code-pattern]] (Habich-Compliance)
- Memory: [[web-research-per-algorithm-pflicht]] (User-Direktive 2026-05-27 erweitert)
- Forschungsarbeiten/code/KARTOGRAFIERUNG_PLAN_2026_05_09.md (Phase 4.B Plan)
- Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md (33 Paper Inventar)
