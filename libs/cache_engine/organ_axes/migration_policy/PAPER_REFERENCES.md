# axis_migration — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** mix (E/C) — kein Wrapper ist is_original-faehig. NoMigration ist reine Baseline (E Engineering); HotColdMigration und AdaptiveMigration verweisen auf OSS-Code, sind aber als Re-Impl umgesetzt (HotCold zusaetzlich C: GPL-3.0 license-blockiert); TierBasedMigration ist code-unsicher (RocksDB als praktische Referenz, kein van-Renen-Repo).

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle is_original = ✗). HotColdMigration (Anti-Caching, GPL-3.0) und AdaptiveMigration (LeCaR + CacheLib) referenzieren OSS-Code, sind aber als Re-Impl/Baseline ohne Original-Bindung umgesetzt; HotColdMigration ist zusaetzlich durch GPL-3.0 fuer Linking gesperrt. NoMigration ist eine bewusste Vergleichs-Baseline ohne Paper.

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| NoMigration | Static placement / No migration (baseline) | — | — | — | nein | none | ✗ |
| HotColdMigration | Hot/Cold-Daten-Separation | Anti-Caching: A New Approach to DBMS Architecture | PVLDB 6(14) 2013 | 10.14778/2556549.2556575 | OSS | GPL-3.0 | ✗ |
| TierBasedMigration | Multi-Tier-Migration RAM→NVM→SSD→HDD | Managing Non-Volatile Memory in Database Systems | SIGMOD 2018 | 10.1145/3183713.3196897 | ? | Apache-2.0 OR GPL-2.0 (RocksDB) | ✗ |
| AdaptiveMigration | ML-driven adaptive Migration (Online-Learning) | Driving Cache Replacement with ML-based LeCaR | USENIX HotStorage 2018 | usenix.org/.../vietri | OSS | LeCaR: none ; CacheLib: Apache-2.0 | ✗ |

## §3 Compliance-Status
Alle 4 Wrapper haben eine Paper-Referenz ODER sind als Baseline gekennzeichnet (NoMigration = Static-Placement-Baseline ohne Paper) → Habich-Pflicht erfuellt.

- **is_original-Kandidaten (Map §3):** keine — kein axis_migration-Wrapper hat is_original_eligible = true.
- **Lizenz-blockiert:** HotColdMigration (Anti-Caching, GPL-3.0) → kein Original-Code-Linking moeglich.
- **Korrektur angewandt (Map §4):** AdaptiveMigration — der alte Code-Kommentar (PAPER_REFERENCES.md §2.4) behauptete faelschlich "Database Cracking (Idreos CIDR 2007)". Korrekt ist LeCaR (Vietri, USENIX HotStorage 2018) + CacheLib (Berg, OSDI 2020) — Block-Migration, NICHT Index-Cracking. Diese Datei verwendet ausschliesslich die korrigierte Attribution.
- **Offener Punkt (Map §5):** TierBasedMigration (conf=high, code=?) — van-Renen-NVM-Code ist nicht als Repo auffindbar; RocksDB dient als praktische Referenz, daher c_cpp_code_exists=unknown und keine Original-Bindung.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_migration, §4 AdaptiveMigration-Korrektur, §5 TierBasedMigration)
- Doku 17 §4.5 (Klassifikation A standalone / C license-blockiert / D pseudocode / E engineering)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md: kein P-ID-Eintrag fuer axis_migration (Anti-Caching/LeCaR/CacheLib/RocksDB nicht im P01-P33-Katalog).
