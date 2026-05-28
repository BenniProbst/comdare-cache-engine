# axis_01_index_organization — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** D/E-Mix — kein Wrapper ist `is_original_eligible`. Drei Wrapper referenzieren seminale/Lehrbuch-Paper ohne verfuegbaren OSS-Code (Klasse D, pseudocode/konzeptuell), einer ist eine bewusste Lehrbuch-Baseline (Heap/Pile-File, Klasse E/Baseline). Kein Original-Code-Linking moeglich.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (OSS + permissiv). Alle vier Index-Organisations-Strategien sind CE-Re-Implementierungen bzw. Baselines, gemappt gegen seminale DBMS-Paper (B-Tree-Wurzel Bayer/McCreight 1972, Ubiquitous-B-Tree-Survey Comer 1979, Oracle8i IOT VLDB 2000) und ein Lehrbuch-Konzept (Heap/Pile-File, Garcia-Molina/Ullman/Widom). `c_cpp_code_exists = nein` fuer alle vier; keine lokalen P-IDs zugeordnet.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| ClusteredIndexOrganization | Clustered Index (storage-order = index-order) | Organization and Maintenance of Large Ordered Indexes (B-Tree-Wurzel) | Acta Informatica 1972 | 10.1007/BF00288683 | nein | none | ✗ |
| HeapIndexOrganization | Heap/Pile (unordered) File, kein Index, Full-Scan (Baseline) | — (Lehrbuch, Garcia-Molina/Ullman/Widom) | — | — | nein | none | ✗ |
| IotIndexOrganization | Index-Organized Table (Daten in B+Tree-Leaf-Pages) | Oracle8i Index-Organized Table and Its Application to New Domains | VLDB 2000 | vldb.org/conf/2000/P285.pdf | nein | none | ✗ |
| NonClusteredIndexOrganization | Non-Clustered (Secondary) Index (Pointer-Hop zur Row) | The Ubiquitous B-Tree (Survey); Wurzel Bayer/McCreight 1972 | CSUR 11(2) 1979 | 10.1145/356770.356776 | nein | none | ✗ |

## §3 Compliance-Status
Alle vier Wrapper haben eine Paper-Referenz (ClusteredIndexOrganization, IotIndexOrganization, NonClusteredIndexOrganization) bzw. sind explizit als Lehrbuch-Baseline gekennzeichnet (HeapIndexOrganization, Garcia-Molina/Ullman/Widom) → Habich-Pflicht erfuellt. Confidence aller vier Wrapper laut Map: high.

- **is_original-Kandidaten (Map §3):** keine fuer diese Achse. Kein Wrapper ist `is_original_eligible = true`.
- **Lizenz-blockierte:** keine — alle `c_cpp_code_exists = nein` (none-Lizenz); es existiert kein zu linkender Original-Code.
- **Map §4 (Korrekturen):** keine Eintraege fuer axis_01_index_organization.
- **Map §5 (offene Punkte):** keine Eintraege fuer axis_01_index_organization. Der `paper_found=false`-Fall dieser Achse (HeapIndexOrganization) faellt laut Map-§5-Schlussnote unter die bewussten Baselines/Lehrbuch-Konzepte (Heap-Org explizit genannt) und ist kein Klaerungsbedarf.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_01_index_organization)
- Doku 17 §4.5 (Klassifikation der Wrapper-Klassen)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md: keine P-ID dieser Achse zugeordnet (Index-Organization-Achse referenziert seminale Paper/Lehrbuch-Konzepte, kein OSS-Repo — vgl. Map §6 Hinweis).
