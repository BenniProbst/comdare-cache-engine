# axis_10_serialization — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #730 R7.6 Paper-Identifikation
**Klasse:** A/D-mix — Re-Impl (is_original=false); zstd/SDSL → R7.6.c-Kandidaten

## §1 Pflicht-Note (Goldstandard)

Mix aus standalone-faehigen (zstd BSD-3, SDSL-Lite GPLv3) und Pseudocode-Quellen.
Aktuell alle Re-Impl is_original=false; echtes Linking fuer zstd in R7.6.c moeglich.

## §2 Wrapper-Paper-Mapping

### §2.1 RawBinarySerialization
- **Quelle:** Patterson & Hennessy (Lehrbuch, Byte-Layout-Grundlage)
- **is_original_module:** false

### §2.2 SuccinctSerialization
- **Titel:** "Space-efficient Static Trees and Graphs"
- **Autoren:** Guy Jacobson
- **Venue:** FOCS 1989 (SDSL-Lite, GPLv3)
- **Verwandt:** Gog et al. "From Theory to Practice: Plug and Play with Succinct
  Data Structures", SEA 2014
- **is_original_module:** false

### §2.3 CompressedSerialization
- **Titel:** "A Universal Algorithm for Sequential Data Compression"
- **Autoren:** Jacob Ziv, Abraham Lempel
- **Venue:** IEEE Transactions on Information Theory 1977, Vol. 23, No. 3
- **DOI:** 10.1109/TIT.1977.1055714 (zstd, BSD-3 → Linking R7.6.c)
- **is_original_module:** false

### §2.4 VarLenSerialization
- **Quelle:** Google Protobuf LEB128 Varint-Encoding (BSD-3, kein Paper)
- **is_original_module:** false

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| RawBinarySerialization | Patterson&Hennessy | false | OK (Lehrbuch) |
| SuccinctSerialization | Jacobson FOCS 1989 / Gog SEA 2014 | false | OK (GPLv3) |
| CompressedSerialization | Ziv+Lempel IEEE-IT 1977 | false | OK (zstd R7.6.c) |
| VarLenSerialization | Protobuf LEB128 | false | OK (kein Paper) |

## §4 Cross-Refs
- Doku 17 §4.5 Klasse A/D
- R7.6.c echtes Linking (zstd BSD-3)
