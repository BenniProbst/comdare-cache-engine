# axis_io — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #735 R7.6 Paper-Identifikation
**Klasse:** E (engineering-pattern) — is_original = false

## §1 Pflicht-Note (Goldstandard)

Klasse E: IO-Strategien aus DBMS-Systemforschung. Re-Impl mit is_original=false.

## §2 Wrapper-Paper-Mapping

### §2.1 BufferedIo
- **Titel:** "Operating System Support for Database Management"
- **Autoren:** Michael Stonebraker
- **Venue:** CACM 1981
- **DOI:** 10.1145/358769.358773 (Konzept-Quelle)
- **is_original_module:** false

### §2.2 DirectIo
- **Quelle:** Linux `O_DIRECT` (Kernel-API, kein DBMS-Paper)
- **is_original_module:** false

### §2.3 InMemoryOnly
- **Quelle:** Baseline (kein Paper)
- **is_original_module:** false

### §2.4 MmapIo
- **Titel:** "Are You Sure You Want to Use MMAP in Your Database Management
  System?"
- **Autoren:** Andrew Crotty, Viktor Leis, Andrew Pavlo
- **Venue:** CIDR 2022 (mmapbench)
- **Hinweis:** Anti-Pattern-Paper — MmapIo dient als Vergleichs-Baseline.
- **is_original_module:** false

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| BufferedIo | Stonebraker CACM 1981 | false | OK (Konzept) |
| DirectIo | Linux O_DIRECT | false | OK (kein Paper) |
| InMemoryOnly | Baseline | false | OK |
| MmapIo | Crotty CIDR 2022 | false | OK (Web-verifiziert) |

## §4 Cross-Refs
- Doku 17 §4.5 Klasse E
