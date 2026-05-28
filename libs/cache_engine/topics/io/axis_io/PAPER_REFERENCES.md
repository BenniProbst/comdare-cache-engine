# axis_io — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** E (Engineering) mit Baseline-Anteil — kein Wrapper is_original-faehig. BufferedIo/DirectIo/MmapIo sind OS-/Engineering-Patterns, InMemoryOnly ist reine RAM-only-Baseline. Kein OSS-Code mit permissiver Lizenz fuer echtes Original-Linking vorgesehen.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle is_original = ✗). BufferedIo und DirectIo bilden OS-Mechanismen (Page-Cache bzw. O_DIRECT/open(2)-Flag) nach, InMemoryOnly ist eine RAM-only-Mess-Baseline ohne IO, und MmapIo ist eine memory-mapped-File-Implementierung, deren zugehoeriges Paper (CIDR 2022) explizit als Anti-Pattern-Baseline zitiert wird. Alle vier sind damit Engineering-/Baseline-Re-Implementierungen, keine OSS-Code-Bindungen.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| BufferedIo | Buffered I/O via OS Page-Cache | Operating System Support for Database Management | CACM 24(7) 1981 | 10.1145/358699.358703 | nein | none | ✗ |
| DirectIo | Direct I/O (O_DIRECT, bypass Page-Cache) | — (Linux-Kernel-API-Flag, open(2)) | Linux 2.4.10 | man7.org/.../open.2.html | nein | none | ✗ |
| InMemoryOnly | In-Memory-Only (kein IO, RAM-only Baseline) | — | — | — | nein | none | ✗ |
| MmapIo | Memory-Mapped File I/O (mmap, file-backed) | Are You Sure You Want to Use MMAP in Your DBMS? (Anti-Pattern-Baseline) | CIDR 2022 | db.cs.cmu.edu/.../cidr2022-p13-crotty.pdf | OSS | MIT | ✗ |

## §3 Compliance-Status
Alle Wrapper haben eine Paper-Ref ODER sind ausdruecklich als Baseline/Engineering-Pattern gekennzeichnet → Habich-Pflicht erfuellt:
- **BufferedIo:** Paper-Anker Stonebraker CACM 1981 (Konzept-/Position-Paper, kein Algo-Code; confidence = medium, §5 der Map). DOI gemaess §4-Korrektur **10.1145/358699.358703** (Code-Kommentar behauptete faelschlich 10.1145/358769.358773 — korrigiert).
- **DirectIo:** OS-API-Flag (O_DIRECT, open(2)), kein Paper — bewusste Engineering-Baseline.
- **InMemoryOnly:** reine RAM-only-Mess-Baseline (Vergleichs-Nullpunkt), kein Paper noetig.
- **MmapIo:** Paper Crotty et al. CIDR 2022, bewusst als Anti-Pattern-Baseline zitiert; OSS-Referenz MIT-lizenziert.

is_original-Kandidaten (§3 der Map): **keine** fuer axis_io.
Lizenz-blockierte Wrapper: **keine** fuer axis_io (kein GPL-Code in dieser Achse).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_io; §4 BufferedIo-DOI-Korrektur; §5 BufferedIo confidence=medium)
- Doku 17 §4.5 (Klassifikation A/C/D/E)
- Lokaler Katalog Forschungsarbeiten/code/ — kein P-ID fuer axis_io (siehe Map §6: IO-Achse referenziert externe OSS-Repos bzw. ist reine CE-Baseline, nicht im P01-P33-Katalog)
