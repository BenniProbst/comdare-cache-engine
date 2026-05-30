# axis_07_prefetch — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix C/D/E — kein is_original-faehiger Wrapper. HardwarePrefetch ist license-blockiert (Klasse C, Wormhole GPL-3.0); DistanceEstimatorPrefetch ist Engineering ueber ART-Vorlage (Klasse E); PathOrientedPrefetch ist eigene Diplomarbeit + verwandte Lit. (Klasse D); NonePrefetch ist Mess-Baseline.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle is_original = ✗). DistanceEstimatorPrefetch und HardwarePrefetch sind Re-Impl/Engineering ueber lokalem Paper-Code (unodb/libart bzw. Wormhole), wobei das Wormhole-Original wegen GPL-3.0 fuer Linking blockiert ist. PathOrientedPrefetch ist die eigene PRT-ART-Diplomarbeit (verwandte Literatur Chen/Gibbons/Mowry). NonePrefetch ist eine reine Mess-Baseline ohne Paper.

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| DistanceEstimatorPrefetch | Distance-Estimator software prefetch fuer ART | The Adaptive Radix Tree: ARTful Indexing… (kein dedizierter Prefetch-Algo im Paper) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 (unodb) / BSD-3 (libart) | ✗ |
| HardwarePrefetch | Explizite HW-Prefetch-Instr. (PREFETCHT0/T1/T2/NTA) | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | lokal | GPL-3.0 (verifiziert — blockiert Linking) | ✗ |
| NonePrefetch | No prefetch (Mess-Baseline) | — | — | — | nein | none | ✗ |
| PathOrientedPrefetch | Path-oriented prefetch (pre-load entlang Trie-Pfad) | PRT-ART Diplomarbeit (eigene Arbeit); verwandt: Chen/Gibbons/Mowry SIGMOD 2001 + Mowry 1994 | Diplomarbeit 2026; SIGMOD 2001 | 10.1145/375663.375688 (verwandt) | nein | none | ✗ |

## §3 Compliance-Status
Alle 4 Wrapper haben eine Paper-Ref ODER sind als Baseline gekennzeichnet → Habich-Pflicht erfuellt:
- DistanceEstimatorPrefetch, HardwarePrefetch, PathOrientedPrefetch: Paper-Ref vorhanden.
- NonePrefetch: bewusste Mess-Baseline (paper_found=false, kein Klaerungsbedarf, korrekt als Vergleichs-Nullpunkt gekennzeichnet).

is_original-Kandidaten (Map §3): KEINE fuer diese Achse.

Lizenz-blockiert: HardwarePrefetch — Wormhole ist GPL-3.0. GPL-3.0 blockiert is_original-Linking.

Beruecksichtigte §4-Korrekturen der Map (alte falsche Angaben ersetzt):
- HardwarePrefetch: Wormhole-Lizenz "BSD-2-Clause" (alter Header/PAPER_REFERENCES.md §2.2) → korrekt GPL-3.0 (verifiziert).
- PathOrientedPrefetch: verwandte Literatur "Tan, Knoll, IEEE TVLSI 2012" (nicht verifizierbar) ENTFERNT → korrekt: Chen/Gibbons/Mowry SIGMOD 2001 (P21) + Mowry 1994.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_07_prefetch, §4 Korrekturen, §6 P-ID-Cross-Ref)
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md:
  - P01 — ART / unodb (Leis ICDE 2013, Apache-2.0) → DistanceEstimatorPrefetch
  - P07 — Wormhole / wormhole (Wu EuroSys 2019, GPL-3.0) → HardwarePrefetch
  - P21 — Chen/Gibbons/Mowry pB+-Trees Prefetching (SIGMOD 2001) → PathOrientedPrefetch (verwandt)
