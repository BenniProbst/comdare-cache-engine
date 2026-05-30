# axis_05_memory_layout — Paper-References
**Stand:** 2026-05-29
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix D/E — Folklore/Textbook-Layout-Konventionen (AoS, Cache-Line-Alignment) und ein "verwandt/Motivation"-Paper (SoA), plus EIN succinct-Datenstruktur-Fundamentpaper (LOUDS/SuRF FST). Kein Wrapper is_original-faehig.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle ✗). `AoSStrictMemoryLayout` und `CacheLineAlignedMemoryLayout` sind Layout-Konventionen ohne Algorithmus-Code (Folklore/Textbook bzw. HW-Standard, Patterson & Hennessy). `SoAMemoryLayout` ist eine Baseline mit nur "verwandtem" Motivations-Paper (Abadi SIGMOD 2008). `PackedBitmapMemoryLayout` referenziert das LOUDS-Fundamentpaper (Jacobson FOCS 1989, P09 — Konzept ohne Code) mit OSS-Code aus SuRF (Apache-2.0) / SDSL (GPL-3.0), ist aber als Layout-Realisierung nicht is_original-gebunden.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| AoSStrictMemoryLayout | Array-of-Structs (strict packed, dense object layout) | — (Folklore/Textbook) | — | — | unbekannt | none | ✗ |
| CacheLineAlignedMemoryLayout | 64-byte cache-line aligned AoS (False-Sharing-Vermeidung) | — (HW-Standard, Patterson & Hennessy) | — | — | unbekannt | none | ✗ |
| SoAMemoryLayout | Struct-of-Arrays (spaltenorientiert, SIMD-freundlich) | Column-Stores vs. Row-Stores: How Different Are They Really? (verwandt) | SIGMOD 2008 | 10.1145/1376616.1376712 | nein | none | ✗ |
| PackedBitmapMemoryLayout | Bit-packed succinct Layout (LOUDS / SuRF FST) | Space-efficient Static Trees and Graphs (Jacobson LOUDS) | FOCS 1989 | 10.1109/SFCS.1989.63533 | OSS | Apache-2.0 (SuRF) / GPL-3.0 (SDSL) | ✗ |
| AoSoAMemoryLayout | Array-of-Structures-of-Arrays (Block-SoA+AoS Hybrid, SIMD-tiled, block_width=8) | — (HPC-/SIMD-Layout-Idiom, z.B. ISPC; kein einzelnes Ursprungspaper) | — | — | nein (Konvention) | none | ✗ |

## §3 Compliance-Status
Alle 5 Wrapper haben eine Paper-Ref ODER sind als Baseline/Folklore/Lehrbuch-Konvention gekennzeichnet → Habich-Pflicht erfuellt. `AoSoAMemoryLayout` (A4, 2026-05-29) ist das SIMD-getilte Hybrid-Layout (Block-SoA innerhalb fester Block-Breite W=8, Bloecke AoS aneinandergereiht); ein bekanntes HPC-/SIMD-Idiom (ISPC u.a.) ohne einzelnes kanonisches Ursprungspaper → keine is_original-Bindung.

- is_original-Kandidaten (Map §3): **keine** in dieser Achse (0 Wrapper is_original_eligible).
- Lizenz-Korrektur (Map §4): `PackedBitmapMemoryLayout` nutzt SuRF-Code unter **Apache-2.0** — der frueher in PAPER_REFERENCES.md angesetzte Eintrag "BSD-3" war falsch und ist korrigiert. Die alternative SDSL-Implementierung der LOUDS-Bitmap steht unter **GPL-3.0** und waere — falls als Code-Basis verwendet — copyleft-blockiert.
- Offene Punkte (Map §5): `SoAMemoryLayout` (confidence medium — SoA hat kein kanonisches Ursprungspaper; Abadi SIGMOD 2008 nur "verwandt/Motivation"); `AoSStrictMemoryLayout` + `CacheLineAlignedMemoryLayout` (confidence high, aber `c_cpp_code_exists = unknown` — reine Layout-Konventionen, kein Algorithmus-Code).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (Map §6): **P09** = Jacobson LOUDS (FOCS 1989) — Originalpaper-Konzept, KEIN Code; verwendet in axis_05.
