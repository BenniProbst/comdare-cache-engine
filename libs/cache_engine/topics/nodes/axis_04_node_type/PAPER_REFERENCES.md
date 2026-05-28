# axis_04_node_type — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** A (standalone) — alle vier Wrapper basieren auf einem einzigen permissiv lizenzierten OSS-Paper-Code (ART / unodb, Apache-2.0, P01). Aktuell als CE-Re-Impl der Node-Layouts gefuehrt (kein is_original-Linking), Original-Code permissiv verfuegbar.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat aktuell echtes is_original-Linking. Alle vier Node-Layouts (Node4/Node16/Node48/Node256) sind CE-Re-Implementierungen der ART-Knotentypen aus Leis et al. (ICDE 2013). Der zugehoerige Original-Code (unodb, Apache-2.0, lokal P01) ist permissiv lizenziert; die Achse selbst ist in der Map nicht als is_original-Kandidat gelistet (§3 enthaelt keinen axis_04-Eintrag).

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| Node4Layout | ART Node4 (4 slots, linear search) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal (P01) | Apache-2.0 | ✗ |
| Node16Layout | ART Node16 (16 slots, SIMD binary search) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal (P01) | Apache-2.0 | ✗ |
| Node48Layout | ART Node48 (48 slots, indirect 256-byte child-index) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal (P01) | Apache-2.0 | ✗ |
| Node256Layout | ART Node256 (256 slots, direct-addressed) | The adaptive radix tree: ARTful indexing for main-memory databases | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal (P01) | Apache-2.0 | ✗ |

## §3 Compliance-Status
Alle vier Wrapper haben eine eindeutige Paper-Referenz (Leis et al., ART, ICDE 2013, DOI 10.1109/ICDE.2013.6544812) — Habich-Pflicht erfuellt. Keiner der Wrapper ist Baseline/Lehrbuch; alle sind direkt einem Paper zugeordnet (Confidence: high). Keine is_original-Kandidaten in dieser Achse (Map §3 listet keinen axis_04-Eintrag). Keine lizenz-blockierten Wrapper (Apache-2.0 ist permissiv). Keine Header-Korrekturen (Map §4) und keine offenen/unsicheren Punkte (Map §5) betreffen axis_04_node_type.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_04_node_type; §6 P-ID-Zuordnung)
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ — P01 = ART / unodb (Leis ICDE 2013, Apache-2.0); laut Map §6 verwendet in axis_02, axis_04 (N4/16/48/256), axis_07, axis_10, axis_11, axis_03a, axis_14
