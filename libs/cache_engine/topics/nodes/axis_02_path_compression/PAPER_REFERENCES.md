# axis_02_path_compression — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** A (standalone) — beide nicht-Baseline-Wrapper sind is_original-faehig (OSS + permissiv: Apache-2.0 / ISC); ein reiner Baseline-Wrapper (PathCompressionNone) als Mess-Nullpunkt.

## §1 Pflicht-Note
Beide funktionalen Wrapper haben echtes is_original-Linking-Potenzial: `ByteWisePathCompression` (ART/unodb, Apache-2.0) und `PatriciaPathCompression` (HOT/hot, ISC) sind beide `is_original_eligible = true` mit verfuegbaren permissiv lizenzierten OSS-Quellen. `PathCompressionNone` ist eine bewusste Baseline (raw path, kein Paper, kein Original-Code).

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| PathCompressionNone | Keine Path-Compression (raw path, Baseline) | — | — | — | nein | none | ✗ |
| ByteWisePathCompression | Byte-by-Byte Path-Compression (ART-Stil) | The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | OSS | Apache-2.0 | ✓ |
| PatriciaPathCompression | Single-Bit-Split Path-Compression (Patricia/HOT) | HOT: A Height Optimized Trie Index… (Ur-Technik: Morrison PATRICIA JACM 1968) | SIGMOD 2018; JACM 1968 | 10.1145/3183713.3196896 | OSS | ISC | ✓ |

> §4-Korrektur (Map): Falls ein Code-Kommentar in diesem Achsen-Verzeichnis Wormhole als "SIGMOD 2019" zitiert, ist korrekt **Wormhole EuroSys 2019** (DOI 10.1145/3302424.3303955). Die hier gelisteten Paper-Attributionen sind bereits die korrigierten Werte.

## §3 Compliance-Status
Alle Wrapper erfuellen die Habich-Pflicht: jeder funktionale Wrapper hat eine verifizierte Paper-Referenz, der Nullpunkt-Wrapper ist explizit als Baseline gekennzeichnet.

- **is_original-Kandidaten (Map §3):**
  - `ByteWisePathCompression` → github.com/laurynas-biveinis/unodb (Apache-2.0)
  - `PatriciaPathCompression` → github.com/speedskater/hot (ISC)
- **Lizenz-blockiert:** keine (beide Quellen permissiv, kein GPL-Blocker auf dieser Achse).
- **Baseline:** `PathCompressionNone` (kein Paper, kein Code — Vergleichs-Nullpunkt).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md):
  - **P01** — ART / unodb (Leis ICDE 2013, Apache-2.0) → ByteWisePathCompression
  - **P02** — HOT / hot (Binna SIGMOD 2018, ISC) → PatriciaPathCompression
