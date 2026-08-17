# axis_filter — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** A (standalone) — 3 von 4 Wrappern (CuckooFilter, RangeSurfFilter, XorFilter) sind is_original-faehige OSS-Codes unter permissiven Lizenzen (Apache-2.0); BloomFilter ist Lehrbuch-Baseline (kein Code). Kein license-blockierter (C) oder reiner Pseudocode-Eintrag (D) in dieser Achse.

## §1 Pflicht-Note
Drei Wrapper (CuckooFilter, RangeSurfFilter, XorFilter) haben echtes is_original-Linking-Potenzial: alle drei verweisen auf permissiv lizenzierte OSS-Referenzimplementierungen (Apache-2.0). BloomFilter ist eine probabilistische Membership-Baseline ohne dediziertes OSS-Code-Artefakt (Re-Impl/Lehrbuch).

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| BloomFilter | Bloom Filter (k-hash bitmap, probabilistic membership) | Space/Time Trade-offs in Hash Coding with Allowable Errors | CACM 13(7) 1970 | 10.1145/362686.362692 | nein | none | ✗ |
| CuckooFilter | Cuckoo Filter (bucketed cuckoo hashing + fingerprints) | Cuckoo Filter: Practically Better Than Bloom | CoNEXT 2014 | 10.1145/2674005.2674994 | OSS | Apache-2.0 | ✓ |
| RangeSurfFilter | SuRF — Succinct Range Filter (LOUDS-Bitmap-Trie) | SuRF: Practical Range Query Filtering with Fast Succinct Tries | SIGMOD 2018 | 10.1145/3183713.3196931 | OSS | Apache-2.0 | ✓ |
| XorFilter | Xor Filter (XOR-Hash immutable filter, ~9 bits/key) | Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters | ACM JEA 25 2020 | 10.1145/3376122 | OSS | Apache-2.0 (xorfilter.h teils MIT) | ✓ |

> §4-Korrekturen der Map eingearbeitet (alte annahmebasierte Angaben ersetzt):
> - RangeSurfFilter: Lizenz war faelschlich "BSD-3-Clause" → korrekt **Apache-2.0** (efficient/SuRF LICENSE geprueft).
> - XorFilter: Lizenz war faelschlich "MIT" → korrekt Repo-LICENSE **Apache-2.0** (xorfilter.h teils MIT; beide permissiv).

## §3 Compliance-Status
Alle 4 Wrapper haben eine Paper-Referenz; BloomFilter ist zusaetzlich als probabilistische Membership-Baseline (CACM 1970) gekennzeichnet → Habich-Pflicht erfuellt.

is_original-Kandidaten (Map §3, alle `is_original_eligible = true`):
- CuckooFilter — github.com/efficient/cuckoofilter — Apache-2.0
- RangeSurfFilter — github.com/efficient/SuRF — Apache-2.0 (lokal P10)
- XorFilter — github.com/FastFilter/fastfilter_cpp — Apache-2.0 (xorfilter.h teils MIT)

Lizenz-blockierte Wrapper: keine in dieser Achse (kein GPL-3.0-Eintrag wie z.B. Wormhole in anderen Achsen).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_filter, §3 is_original-Kandidaten, §4 Wrapper-Header-Korrekturen RangeSurfFilter/XorFilter)
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): P10 = SuRF / FST (Zhang SIGMOD 2018, Apache-2.0) → RangeSurfFilter. CuckooFilter und XorFilter sind NICHT im P01-P33-Katalog, aber als is_original-Kandidaten zum Klonen empfohlen (efficient/cuckoofilter bzw. FastFilter/fastfilter_cpp).
