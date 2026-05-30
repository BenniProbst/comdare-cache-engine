# axis_14_value_handle — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix C/E — Engineering-Techniken (Value-Handle-Varianten) mit Paper-Ankern; KEINE is_original-faehigen Wrapper. Wormhole = GPL-3.0 (Klasse C, blockiert), RCU = LGPL-2.1-or-later; ART/Masstree = permissiv, aber nur lokal angebunden (Re-Impl, kein Original-Linking).

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (OSS+permissiv). Alle 4 Wrapper sind lokale Re-Impl/Baseline-Anbindungen: `InlineValueHandle` (ART, lokal Apache-2.0) und `VersionedPointerValueHandle` (Masstree, lokal MIT) stammen aus permissiven Quellen, sind aber als CE-Re-Impl gefuehrt; `ExternalPoolValueHandle` (Wormhole) ist durch GPL-3.0 blockiert; `ImmutableSharedRefValueHandle` (RCU) ist LGPL-2.1-or-later. is_original = 0/4.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| InlineValueHandle | Inline value storage in node slot (combined pointer/value) | The Adaptive Radix Tree: ARTful Indexing… (Leaf Nodes §) | ICDE 2013 | 10.1109/ICDE.2013.6544812 | lokal | Apache-2.0 | ✗ |
| ExternalPoolValueHandle | Out-of-line value storage in pool (node hält Offset) | Wormhole: A Fast Ordered Index for In-memory Data Management | EuroSys 2019 | 10.1145/3302424.3303955 | lokal | GPL-3.0 | ✗ |
| ImmutableSharedRefValueHandle | Immutable value via ref-counted Pointer (RCU-style snapshot) | Read-Copy Update: Using Execution History to Solve Concurrency Problems | PDCS 1998 | semanticscholar.org/.../21e51da40ab080ca2b71ad36094e2b686008b6cc | lokal | LGPL-2.1-or-later | ✗ |
| VersionedPointerValueHandle | Pointer mit Version-Tag (MVCC/optimistic; Masstree+SMART) | Cache Craftiness for Fast Multicore Key-Value Storage (Masstree) | EuroSys 2012 | 10.1145/2168836.2168855 | lokal | MIT | ✗ |

## §3 Compliance-Status
Alle 4 Wrapper haben einen verifizierten Paper-Anker → Habich-Pflicht erfuellt. Keiner ist als reine Lehrbuch-Baseline gefuehrt (jeder traegt ein konkretes Quellpaper). is_original-Kandidaten (§3 der Map): KEINE — axis_14 erscheint nicht in der is_original-Eligible-Liste (20 Wrapper, keiner aus value_handle). Lizenz-blockiert: `ExternalPoolValueHandle` (Wormhole, GPL-3.0) — kein Original-Linking moeglich.

**§4 Korrekturen (KORRIGIERTE Angaben, oben in §2 bereits eingearbeitet — alte Attributionen waren falsch):**
- `InlineValueHandle`: alte PAPER_REFERENCES.md §2.1 behauptete Rao/Ross CSS-Tree (P11, SIGMOD 2000) → KORREKT: Leis ART ICDE 2013 (P01).
- `ExternalPoolValueHandle`: alter Header "Wormhole SIGMOD 2019" + alte §2.2 "Oracle In-Memory, kein Paper" → KORREKT: Wormhole EuroSys 2019 (P07).
- `VersionedPointerValueHandle`: alte §2.4 behauptete Hazard Pointers (Michael TPDS 2004, P30) — andere Technik → KORREKT: Masstree (Mao EuroSys 2012, P03) bzw. SMART (OSDI 2023, nicht 2022).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_14_value_handle, §4 Korrekturen, §6 P-ID-Katalog)
- Doku 17 §4.5 (Klassifikation A/C/D/E)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md — P-IDs dieser Achse: P01 (ART/unodb, Apache-2.0), P03 (Masstree/masstree-beta, MIT), P07 (Wormhole, GPL-3.0), P29 (userspace-rcu/liburcu, LGPL-2.1)
