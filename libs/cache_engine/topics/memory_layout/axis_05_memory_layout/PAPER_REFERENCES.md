# axis_05_memory_layout — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #730 R7.6 Paper-Identifikation
**Klasse:** D (pseudocode-only) — alle is_original_module = false

## §1 Pflicht-Note (Goldstandard)

Klasse D: Layout-Strategien stammen aus Pseudocode-Papern / Lehrbuechern. Kein
Original-Code-Linking, Re-Impl mit is_original=false. Cross-Ref Doku 16 (axis_05
CPU IMC Runtime-Heuristik).

## §2 Wrapper-Paper-Mapping

### §2.1 CacheLineAlignedMemoryLayout
- **Quelle:** Patterson & Hennessy, "Computer Architecture: A Quantitative
  Approach" (Lehrbuch, Cache-Line-Alignment Grundlage)
- **is_original_module:** false

### §2.2 AoSStrictMemoryLayout
- **Quelle:** Data-Oriented-Design-Pattern (Array-of-Structs, kein Paper)
- **is_original_module:** false

### §2.3 SoAMemoryLayout
- **Titel:** "Column-Stores vs. Row-Stores: How Different Are They Really?"
- **Autoren:** Daniel Abadi, Samuel Madden, Nabil Hachem
- **Venue:** SIGMOD 2008 (verwandt — Struct-of-Arrays-Begruendung)
- **is_original_module:** false

### §2.4 PackedBitmapMemoryLayout
- **Titel:** "Space-efficient Static Trees and Graphs"
- **Autoren:** Guy Jacobson
- **Venue:** FOCS 1989
- **DOI:** 10.1109/SFCS.1989.63533 (P09, Pseudocode)
- **is_original_module:** false

## §3 Verwandte Layouts (Cross-Ref)
- Bender Tree Layout (P16), Cache-Oblivious B-Trees (P17) — siehe Doku 16.

## §4 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| CacheLineAlignedMemoryLayout | Patterson&Hennessy | false | OK (Lehrbuch) |
| AoSStrictMemoryLayout | DOD-Pattern | false | OK (kein Paper) |
| SoAMemoryLayout | Abadi SIGMOD 2008 | false | OK (verwandt) |
| PackedBitmapMemoryLayout | Jacobson FOCS 1989 | false | OK (Pseudocode) |

## §5 Cross-Refs
- Doku 16 — axis_05 CPU IMC Runtime-Heuristik
- Doku 17 §4.5 Klasse D
