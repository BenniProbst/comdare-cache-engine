# axis_03a_search_algo — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix A/C — A (standalone, is_original-faehige permissive OSS-Codes: ART/unodb Apache-2.0, HOT ISC, START MIT, SuRF Apache-2.0) + C (license-blockiert: Wormhole GPL-3.0).

## §1 Pflicht-Note
Vier Wrapper sind is_original-faehig mit permissivem OSS-Original-Code (OriginalArtSearchAlgo via unodb Apache-2.0, OriginalHotSearchAlgo via hot ISC, OriginalStartSearchAlgo via START MIT, OriginalSurfSearchAlgo via SuRF Apache-2.0); diese sind aktuell s2-Standalone-Stubs, echtes Original-Code-Linking ist fuer s4 geplant. Die uebrigen Wrapper sind CE-Re-Implementierungen bzw. vereinfachte Konzept-Wrapper (Array256SearchAlgo Re-Impl von ART Node256; VectorU16U16SearchAlgo vereinfachtes START-Konzept; VectorU8U8SearchAlgo HOT-k-Konzept) oder license-blockiert (OriginalWormholeSearchAlgo GPL-3.0 → kein Linking).

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| Array256SearchAlgo | ART Node256 direct-addressed (CE-Re-Impl) | The Adaptive Radix Tree: ARTful Indexing… | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | ✗ |
| OriginalArtSearchAlgo | ART — Original-Bindung (unodb::db) | The Adaptive Radix Tree: ARTful Indexing… | ICDE 2013 | 10.1109/ICDE.2013.6544812 | OSS | Apache-2.0 | ✓ |
| OriginalHotSearchAlgo | HOT — Height Optimized Trie (HOTRowex) | HOT: A Height Optimized Trie Index for Main-Memory Database Systems | SIGMOD 2018 (erw. Version ACM TODS 2022) | 10.1145/3183713.3196896 | OSS | ISC | ✓ |
| OriginalStartSearchAlgo | START — Self-Tuning Adaptive Radix Tree | START — Self-Tuning Adaptive Radix Tree (Fent/Jungmair/Kipf/Neumann) | ICDEW 2020 | 10.1109/ICDEW49219.2020.00015 | OSS | MIT | ✓ |
| OriginalSurfSearchAlgo | SuRF — Succinct Range Filter (FST, LOUDS-DS) | SuRF: Practical Range Query Filtering with Fast Succinct Tries | SIGMOD 2018 | 10.1145/3183713.3196931 | OSS | Apache-2.0 | ✓ |
| OriginalWormholeSearchAlgo | Wormhole — Hash+Trie+B+ Hybrid Ordered Index | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | OSS | GPL-3.0 | ✗ |
| VectorU16U16SearchAlgo | Sorted u16-vector lower_bound (START-Konzept, vereinfacht) | START — Self-Tuning Adaptive Radix Tree (Fent et al.) | ICDEW 2020 | 10.1109/ICDEW49219.2020.00015 | OSS | MIT | ✗ |
| VectorU8U8SearchAlgo | Sparse sorted u8-vector lower_bound (HOT-k-Konzept) | HOT: A Height Optimized Trie Index… | SIGMOD 2018 | 10.1145/3183713.3196896 | OSS | ISC | ✗ |

> §4-Korrekturen (Map) angewendet:
> - OriginalHotSearchAlgo: Venue korrigiert von Code-Kommentar "PVLDB 11(3):274-286, 2018" → **SIGMOD 2018** (erw. Version ACM TODS 2022).
> - OriginalStartSearchAlgo: Attribution korrigiert von "START (Mertens et al., ICDE 2024)" → **Fent/Jungmair/Kipf/Neumann, ICDEW 2020**.
> - VectorU16U16SearchAlgo: korrigiert von "Mertens ICDE 2024" → **Fent et al., ICDEW 2020** (gleicher Fehler wie OriginalStartSearchAlgo).
> - OriginalWormholeSearchAlgo: Venue korrigiert von "USENIX ATC 2019" → **EuroSys 2019**.
> - VectorU8U8SearchAlgo: HOT-Venue praezisiert von "PVLDB 2018" → **SIGMOD 2018**.
> - OriginalSurfSearchAlgo: Klarstellung (kein Bug) — Mehr-Header-Bibliothek, kein Single-Header.

## §3 Compliance-Status
Alle 8 Wrapper besitzen eine Paper-Referenz; vereinfachte/Re-Impl-Wrapper (Array256SearchAlgo, VectorU16U16SearchAlgo, VectorU8U8SearchAlgo) sind als Konzept-Ableitung des jeweiligen Anker-Papers gekennzeichnet → Habich-Pflicht erfuellt.

is_original-Kandidaten (Map §3, fuer R7.6.c — echtes Original-Code-Linking):
- OriginalArtSearchAlgo → github.com/laurynas-biveinis/unodb (Apache-2.0)
- OriginalHotSearchAlgo → github.com/speedskater/hot (ISC)
- OriginalStartSearchAlgo → github.com/jungmair/START (MIT)
- OriginalSurfSearchAlgo → github.com/efficient/SuRF (Apache-2.0)

Lizenz-blockiert (kein Linking):
- OriginalWormholeSearchAlgo — Wormhole GPL-3.0 (verifiziert) → bleibt Standalone-Wrapper ohne Original-Code-Linking.

Status (Map §3-Hinweis): Die Original*SearchAlgo-Wrapper sind aktuell s2-Standalone-Stubs; echtes Linking ist fuer s4 geplant.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 Achsen-Sektion axis_03a_search_algo, §3 is_original-Kandidaten, §4 Header-Korrekturen, §6 P-ID-Katalog)
- Doku 17 §4.5 (Klassifikation A standalone / C license-blockiert)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md — P-IDs dieser Achse:
  - P01 = ART / unodb (Leis ICDE 2013, Apache-2.0)
  - P02 = HOT / hot (Binna SIGMOD 2018, ISC)
  - P05 = START / START (Fent ICDEW 2020, MIT)
  - P07 = Wormhole / wormhole (Wu EuroSys 2019, GPL-3.0)
  - P10 = SuRF / FST (Zhang SIGMOD 2018, Apache-2.0)
