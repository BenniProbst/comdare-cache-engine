# axis_migration — Paper-References (R7.6)

**Stand:** 2026-05-28
**Task:** #734 R7.6 Paper-Identifikation
**Klasse:** D/E (web-verifizierte DBMS-Referenzen) — is_original = false

## §1 Pflicht-Note (Goldstandard)

Klasse D/E: Hot/Cold- und Tier-Migration aus DBMS-Forschung. Lizenz-/Komplexitaets-
Gruende → Re-Impl mit is_original=false.

## §2 Wrapper-Paper-Mapping

### §2.1 NoMigration
- **Quelle:** Baseline (kein Paper)
- **is_original_module:** false

### §2.2 HotColdMigration
- **Titel:** "Anti-Caching: A New Approach to Database Management System
  Architecture"
- **Autoren:** Justin DeBrabant, Andrew Pavlo, Stephen Tu, Michael Stonebraker,
  Stan Zdonik
- **Venue:** PVLDB 2013, Vol. 6, No. 14
- **DOI:** 10.14778/2556549.2556575 (H-Store, GPL)
- **Verwandt:** Levandoski et al. "Hekaton Project Siberia (Hot/Cold)", ICDE 2013
- **is_original_module:** false

### §2.3 TierBasedMigration
- **Titel:** "Managing Non-Volatile Memory in Database Systems"
- **Autoren:** Alexander van Renen, Viktor Leis, Alfons Kemper, Thomas Neumann
  et al.
- **Venue:** SIGMOD 2018
- **DOI:** 10.1145/3183713.3196897
- **Verwandt:** Eisenman et al. "Reducing DRAM Footprint with NVM", EuroSys 2018
- **is_original_module:** false

### §2.4 AdaptiveMigration
- **Titel:** "Database Cracking"
- **Autoren:** Stratos Idreos, Martin Kersten, Stefan Manegold
- **Venue:** CIDR 2007 (MonetDB)
- **Verwandt:** Chaudhuri, Narasayya "Self-Tuning Database Systems", VLDB 2007
- **is_original_module:** false

## §3 Achsen-Compliance-Status

| Wrapper | Paper-Ref | is_original | Habich-Compliant |
|---------|-----------|-------------|------------------|
| NoMigration | Baseline | false | OK |
| HotColdMigration | DeBrabant PVLDB 2013 | false | OK (Web-verifiziert) |
| TierBasedMigration | van Renen SIGMOD 2018 | false | OK |
| AdaptiveMigration | Idreos CIDR 2007 | false | OK |

## §4 Cross-Refs
- Doku 17 §4.5 Klasse D/E
- Naming-Backlog: axis_migration ohne Topic-Suffix (optional P2)
