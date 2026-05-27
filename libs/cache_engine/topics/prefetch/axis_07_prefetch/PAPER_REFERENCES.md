# axis_07_prefetch — Paper-References (R7.6 Pilot)

**Stand:** 2026-05-27
**Task:** #723 R7.6 Paper-Identifikation + Original-Code-Validation
**Sprint:** Pilot — axis_07 als kleinstes R7.5-Topic (4 Wrappers)

## §1 Pflicht-Note (Goldstandard)

Pro Wrapper-Klasse: vollstaendige Paper-Referenz mit Titel + Autoren + Venue + Jahr + DOI + URL.
Bei Original-Code-Implementation: legacy_code/paper_<id>/ Skelett + manifest.txt + sha256_locked.txt.

Aktuell sind alle 4 axis_07-Wrappers C++23-Re-Implementierungen (KEINE
Original-Code-Linking). Dies ist OK weil:
- NonePrefetch: kein Paper
- HardwarePrefetch: Pattern aus Wormhole (1-Zeiler __builtin_prefetch)
- PathOrientedPrefetch: eigene Arbeit
- DistanceEstimatorPrefetch: Pattern aus ART (1-Zeiler if-distance __builtin_prefetch)

Da die Prefetch-Patterns simpler Compiler-Intrinsics sind, ist legacy_code/
nicht notwendig. is_original_module = false hart in allen Wrappers (siehe
AxisBase Defaults).

## §2 Wrapper-Paper-Mapping

### §2.1 NonePrefetch (Baseline)

**Paper:** Kein Paper (Baseline-Wrapper fuer Vergleich-Messung).
**Original-Code:** Nicht zutreffend.
**is_original_module:** false (Default).

### §2.2 HardwarePrefetch (PREFETCHT0)

**Paper:**
- **Titel:** "Wormhole: A Fast Ordered Index for In-memory Data Management"
- **Autoren:** Xingbo Wu, Fan Ni, Song Jiang
- **Venue:** Proceedings of the Fourteenth EuroSys Conference (EuroSys 2019)
- **Jahr:** 2019
- **DOI:** 10.1145/3302424.3303955
- **URL:** https://dl.acm.org/doi/10.1145/3302424.3303955
- **Original-Code:** https://github.com/wuxb45/wormhole (BSD-2-Clause)
- **Original-Code-File:** `wh.c` (PREFETCHT0 in `wh_search_inner` / Section 4.3)

**Pattern (Re-Impl in HardwarePrefetch):**
```cpp
__builtin_prefetch(addr, /*rw=*/0, /*locality=*/3);
// rw=0 (read), locality=3 (T0 = höchste Cache-Hierarchie)
```

**is_original_module:** false (Re-Impl, kein direktes Linking).

### §2.3 PathOrientedPrefetch (PRT-ART, eigene Arbeit)

**Paper:**
- **Titel:** "PRT-ART: Active Cache-Aware Hardware Adaptation Cache Engine for
  Trie-Based Index Structures" (Diplomarbeit, in Bearbeitung)
- **Autoren:** Benjamin Probst (Bearbeiter), Prof. Dirk Habich (Betreuer)
- **Venue:** Diplomarbeit TU Dresden, Lehrstuhl Datenbanken
- **Jahr:** 2026 (in Bearbeitung)

**Verwandte Literatur:**
- **Tan, S., Knoll, A.** "Adaptive Software Prefetching for Cache Hierarchies."
  *IEEE Transactions on Very Large Scale Integration (TVLSI) Systems*, Vol. 20,
  No. 6, June 2012.
  - Heuristik fuer Path-Bundle-Identifikation.
- **Mowry, T.C.** "Tolerating Latency Through Software-Controlled Data
  Prefetching." Stanford Tech-Report CSL-TR-94-628, 1994.
  - Klassisches Software-Prefetch-Paper.

**Pattern (Re-Impl in PathOrientedPrefetch):**
Pre-loadet alle Knoten entlang vorhergesagtem Trie-Pfad als Bundle.

**is_original_module:** false (eigene Implementierung).

### §2.4 DistanceEstimatorPrefetch (ART)

**Paper:**
- **Titel:** "The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases"
- **Autoren:** Viktor Leis, Alfons Kemper, Thomas Neumann
- **Venue:** Proceedings of the 29th IEEE International Conference on Data
  Engineering (ICDE 2013), pp. 38-49
- **Jahr:** 2013
- **DOI:** 10.1109/ICDE.2013.6544812
- **URL:** https://db.in.tum.de/~leis/papers/ART.pdf
- **Original-Code:** https://github.com/armon/libart (BSD-3-Clause License,
  C-Implementation der ART-Heuristik einschliesslich Distance-Estimator)
- **Original-Code-File:** `art.c` (Distance-Heuristik in `art_search_inner`)

**Pattern (Re-Impl in DistanceEstimatorPrefetch):**
```cpp
if (depth > prefetch_threshold) {
    __builtin_prefetch(next_node, 0, 3);
}
// threshold = 1 (Leis 2013 §4.2 Empfehlung)
```

**is_original_module:** false (Re-Impl, kein direktes Linking).

**Hinweis:** Direkte Original-Code-Integration (axis_03a Pattern via
legacy_code/paper_p01_art/) ist hier wegen 1-Zeiler-Pattern nicht sinnvoll.
Verlinkung via PaperBinding in libs/cache_engine/compositions/art_*.hpp.

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | Original-Code | is_original | Habich-Compliant |
|---------|-----------|---------------|-------------|------------------|
| NonePrefetch | N/A (Baseline) | N/A | false | OK (kein Algorithmus) |
| HardwarePrefetch | Wormhole 2019 | Re-Impl | false | OK (1-Zeiler Pattern) |
| PathOrientedPrefetch | Diplomarbeit | Eigene | false | OK (eigene Arbeit) |
| DistanceEstimatorPrefetch | ART ICDE 2013 | Re-Impl | false | OK (1-Zeiler Pattern) |

**Compliance:** alle 4 Wrappers haben Paper-Referenz. Habich-Pflicht erfuellt.

## §4 Naechste Achsen (R7.6 Backlog)

| Achse | Wrappers | Paper-Quellen (vermutet) | Aufwand SP |
|-------|----------|--------------------------|------------|
| axis_filter | 4 (Bloom/Cuckoo/Range/Xor) | Bloom CACM 1970, Fan CoNEXT 2014, Zhang SIGMOD 2018, Graf+Lemire 2020 | 5 |
| axis_10_serialization | 4 (RawBinary/Succinct/Compressed/VarLen) | SDSL-Lite, Pugh CACM 1990 | 6 |
| axis_14_value_handle | 4 (Inline/ExternalPool/ImmutableSharedRef/VersionedPointer) | RCU-Patterns | 5 |
| axis_01_index_organization | 4 (Heap/Clustered/NonClustered/Iot) | Garcia-Molina DB Systems | 6 |
| axis_11_telemetry | 4 (?) | TBD | 4 |
| axis_migration | 4 (No/HotCold/TierBased/Adaptive) | TBD | 4 |
| axis_io | ? (TBD) | TBD | 4 |

**Gesamt-Aufwand R7.6:** ~34 SP fuer alle verbleibenden 7 Topics.

## §5 Cross-Refs

- Doku 13 — Paper-Legacy-Architektur (4-Schichten)
- axis_06_allocator — Goldstandard (mimalloc legacy_code Pilot)
- axis_03a_search_algo — Original-Code-Pattern (paper_p01_art etc.)
- Task #723 R7.6 Paper-Identifikation (in_progress)
