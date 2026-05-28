# axis_01_index_organization — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #732 R7.6 Paper-Identifikation
**Klasse:** D (pseudocode-only) — Wurzel Bayer+McCreight; is_original = false

## §1 Pflicht-Note (Goldstandard)

Klasse D: Index-Organisationsformen aus Grundlagen-Papern + Lehrbuechern.
Kein Original-Code-Linking, Re-Impl mit is_original=false.

## §2 Wrapper-Paper-Mapping

### §2.0 Wurzel-Referenz (alle Wrapper)
- **Titel:** "Organization and Maintenance of Large Ordered Indexes"
- **Autoren:** Rudolf Bayer, Edward M. McCreight
- **Venue:** Acta Informatica 1972
- **DOI:** 10.1007/BF00288683

### §2.1 HeapIndexOrganization
- **Quelle:** Garcia-Molina et al., "Database Systems: The Complete Book"
  (Lehrbuch, derivative)
- **is_original_module:** false

### §2.2 ClusteredIndexOrganization
- **Quelle:** SQL Server / Oracle / Tandem (Industrie, derivative, kein Paper)
- **is_original_module:** false

### §2.3 NonClusteredIndexOrganization
- **Titel:** "The Ubiquitous B-Tree"
- **Autoren:** Douglas Comer
- **Venue:** ACM Computing Surveys (CSUR) 1979
- **DOI:** 10.1145/356770.356776
- **is_original_module:** false

### §2.4 IotIndexOrganization
- **Quelle:** Oracle 8i Index-Organized Tables, 1997 (derivative)
- **is_original_module:** false

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| HeapIndexOrganization | Garcia-Molina (textbook) | false | OK |
| ClusteredIndexOrganization | SQL Server/Oracle | false | OK (derivative) |
| NonClusteredIndexOrganization | Comer CSUR 1979 | false | OK |
| IotIndexOrganization | Oracle 8i 1997 | false | OK (derivative) |

## §4 Cross-Refs
- Wurzel Bayer+McCreight Acta Informatica 1972
- Doku 17 §4.5 Klasse D
