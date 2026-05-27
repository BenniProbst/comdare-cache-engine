# axis_filter — Paper-References (R7.6 Phase 2)

**Stand:** 2026-05-27
**Task:** #723 R7.6 Paper-Identifikation + Original-Code-Validation
**Phase:** 2 — axis_filter (Pflicht R7.5-Achse)

## §1 Wrapper-Paper-Mapping

### §1.1 BloomFilter (Bloom 1970)

- **Titel:** Space/Time Trade-offs in Hash Coding with Allowable Errors
- **Autor:** Burton H. Bloom
- **Venue:** Communications of the ACM (CACM), Vol. 13, No. 7, July 1970, pp. 422-426
- **DOI:** 10.1145/362686.362692
- **Original-Code:** N/A (Klassisches Paper, kein Open-Source-Original)
- **is_original_module:** false (Re-Impl basierend auf Pseudocode §3)

### §1.2 CuckooFilter (Fan CoNEXT 2014)

- **Titel:** Cuckoo Filter: Practically Better Than Bloom
- **Autoren:** Bin Fan, David G. Andersen, Michael Kaminsky, Michael D. Mitzenmacher
- **Venue:** Proceedings of CoNEXT 2014, pp. 75-88
- **DOI:** 10.1145/2674005.2674994
- **URL:** https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf
- **Original-Code:** https://github.com/efficient/cuckoofilter (Apache-2.0)
- **Original-Code-Files:** `src/cuckoofilter.h`, `src/cuckoofilter.cc`
- **Vorteil vs Bloom:** unterstuetzt **delete + lookup-Counting**
- **is_original_module:** false (Re-Impl, koennte spaeter Original-Code linken)

### §1.3 XorFilter (Graf/Lemire 2020)

- **Titel:** Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters
- **Autoren:** Thomas Mueller Graf, Daniel Lemire
- **Venue:** ACM Journal of Experimental Algorithmics (JEA), Vol. 25, Article 1.5, March 2020
- **DOI:** 10.1145/3376122
- **URL (Preprint):** https://arxiv.org/abs/1912.08258
- **Original-Code:** https://github.com/FastFilter/fastfilter_cpp (MIT License)
- **Original-Code-File:** `src/xorfilter/xorfilter.h`
- **Vorteil:** ~9 bits/key (vs Bloom ~10 bits/key) bei gleicher FPR. Immutable.
- **is_original_module:** false (Re-Impl, koennte spaeter Original-Code linken)

### §1.4 RangeSurfFilter (Zhang SIGMOD 2018)

- **Titel:** SuRF: Practical Range Query Filtering with Fast Succinct Tries
- **Autoren:** Huanchen Zhang, Hyeontaek Lim, Viktor Leis, David G. Andersen,
  Michael Kaminsky, Kimberly Keeton, Andrew Pavlo
- **Venue:** Proceedings of SIGMOD 2018, pp. 323-336
- **DOI:** 10.1145/3183713.3196931
- **URL:** https://www.cs.cmu.edu/~huanche1/publications/surf_paper.pdf
- **Original-Code:** https://github.com/efficient/SuRF (BSD-3-Clause License)
- **Original-Code-Files:** `include/surf.hpp` + `src/louds_dense.cpp`
- **Erstes Filter mit Range-Query** (lower_bound/upper_bound). Wird in RocksDB
  fuer SST-Files genutzt.
- **is_original_module:** false (Re-Impl, koennte spaeter Original-Code linken)

## §2 Achsen-Compliance-Status axis_filter

| Wrapper | Paper-Ref | Original-Code Available | Habich-Compliant |
|---------|-----------|-------------------------|------------------|
| BloomFilter | Bloom CACM 1970 | N/A (klassisches Paper) | OK |
| CuckooFilter | Fan CoNEXT 2014 | github.com/efficient/cuckoofilter | OK |
| XorFilter | Graf+Lemire JEA 2020 | github.com/FastFilter/fastfilter_cpp | OK |
| RangeSurfFilter | Zhang SIGMOD 2018 | github.com/efficient/SuRF | OK |

**Compliance:** alle 4 Wrappers haben Paper-Referenz. 3 davon haben Open-Source-
Original-Code verfuegbar (CuckooFilter/XorFilter/RangeSurfFilter).

## §3 Original-Code-Linking-Roadmap (R7.6.b spaeter)

Falls echte Original-Code-Integration analog axis_06_allocator gewuenscht ist:

1. **CuckooFilter:** ext/filter/F02-cuckoofilter/ als Submodule mit Apache-2.0
   License. legacy_code/paper_f02_cuckoofilter/ + manifest.txt + sha256_locked.txt.
2. **XorFilter:** ext/filter/F03-fastfilter_cpp/ als Submodule mit MIT-License.
3. **RangeSurfFilter:** ext/filter/F04-surf/ als Submodule mit BSD-3-Clause.

Aufwand: ~12 SP fuer 3 Wrapper (4 SP pro Roll-out analog allocator Pattern).

## §4 Cross-Refs

- Doku 13 — Paper-Legacy-Architektur (4-Schichten)
- axis_06_allocator — Goldstandard (mimalloc Pilot)
- axis_07_prefetch/PAPER_REFERENCES.md — R7.6 Pilot-Template
- Task #723 R7.6 Paper-Identifikation (in_progress)
