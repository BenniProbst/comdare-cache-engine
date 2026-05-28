# axis_10_serialization — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix A/D — A (standalone is_original-faehig: CompressedSerialization, SuccinctSerialization, VarLenSerialization via permissivem OSS) + D (Lehrbuch/Baseline ohne Paper: RawBinarySerialization).

## §1 Pflicht-Note
Drei der vier Wrapper sind `is_original_eligible = true` und stuetzen sich auf permissiv lizenzierte OSS-Codes: CompressedSerialization (LZ4 BSD-2 / Snappy BSD-3), SuccinctSerialization (SuRF Apache-2.0) und VarLenSerialization (Protobuf BSD-3 / unodb Apache-2.0). RawBinarySerialization ist eine reine Lehrbuch-Baseline (memcpy Raw-Byte-Layout, kein Paper) und liefert kein Original-Linking.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| RawBinarySerialization | Raw binary serialization (memcpy raw-byte layout) | — (Lehrbuch, Patterson & Hennessy) | — | — | nein | none | ✗ |
| CompressedSerialization | Block-Compression (LZ4/Snappy; theor. LZ77) | A Universal Algorithm for Sequential Data Compression | IEEE IT-23(3) 1977 | 10.1109/TIT.1977.1055714 | OSS | BSD-2 (LZ4) / BSD-3 (Snappy) | ✓ |
| SuccinctSerialization | Succinct bit-packed Encoding (LOUDS / FST) | SuRF (Fast Succinct Trie); Fundament Jacobson LOUDS | SIGMOD 2018; FOCS 1989 | 10.1145/3183713.3196931 ; 10.1109/SFCS.1989.63533 | OSS | Apache-2.0 | ✓ |
| VarLenSerialization | Variable-Length Integer Encoding (VarInt/VLQ/LEB128) | — (Standard-Technik; ART nutzt es, ist nicht Ursprung) | — | — | OSS | BSD-3 (Protobuf) / Apache-2.0 (unodb) | ✓ |

## §3 Compliance-Status
Habich-Pflicht erfuellt: Alle vier Wrapper haben entweder eine Paper-Referenz (CompressedSerialization: Ziv/Lempel IEEE IT 1977; SuccinctSerialization: SuRF SIGMOD 2018 + Jacobson LOUDS FOCS 1989) oder sind explizit als Baseline/Standard-Technik gekennzeichnet (RawBinarySerialization = Lehrbuch-Baseline; VarLenSerialization = Standard-Technik/VLQ/LEB128, ART als Nutzer, nicht Ursprung).

is_original-Linking-Kandidaten (Map §3) — 3 Wrapper dieser Achse:
- CompressedSerialization → github.com/lz4/lz4 ; github.com/google/snappy (BSD-2 / BSD-3)
- SuccinctSerialization → github.com/efficient/SuRF (Apache-2.0)
- VarLenSerialization → github.com/protocolbuffers/protobuf ; github.com/laurynas-biveinis/unodb (BSD-3 / Apache-2.0)

Lizenz-blockierte Codes: keine in dieser Achse (alle drei Kandidaten sind permissiv lizenziert; kein GPL-Blocker wie z.B. Wormhole GPL-3.0 in anderen Achsen).

Confidence: VarLenSerialization = medium (Standard-Technik ohne kanonisches Ursprungspaper), uebrige = high.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_10_serialization, §3 is_original-Kandidaten)
- Doku 17 §4.5 (Klassifikation A/D)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): P01 = ART / unodb (Leis ICDE 2013, Apache-2.0) → VarLenSerialization; P10 = SuRF / FST (Zhang SIGMOD 2018, Apache-2.0) → SuccinctSerialization
- §4 (Header-Korrekturen) der Map listet KEINE Korrektur fuer axis_10_serialization
- §5 (offene Punkte) der Map listet KEINEN Eintrag fuer axis_10_serialization
