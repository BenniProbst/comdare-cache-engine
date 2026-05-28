# axis_03b_cache_traversal — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** D/E — Baseline/Lehrbuch-Konzepte (keine OSS-Codes, kein is_original-faehiger Wrapper). LinearFanout = klassisches B-Tree-Lehrbuchverfahren (Bayer/McCreight 1972), HashLookup = Knuth-Lehrbuch-Hashing. Beide Engineering-/Textbook-Baselines ohne Original-Code-Linking.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (kein OSS-Code, kein permissiver Original-Code-Body). Beide Wrapper sind reine CE-Re-Implementierungen klassischer Lehrbuch-/Baseline-Verfahren (B-Tree-Knotensuche bzw. multiplikatives Hashing mit Open-Addressing). is_original ist fuer beide ✗.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| LinearFanout | Linear-scan lookup auf kleinen Fanout-Arrays | Organization and maintenance of large ordered indexes | Acta Informatica 1972 | 10.1007/BF00288683 | nein | none | ✗ |
| HashLookup | Fibonacci-/multiplikatives Hashing, Open-Addressing Linear Probing | The Art of Computer Programming Vol.3 §6.4 (Hashing) | Addison-Wesley 2nd Ed. 1998 | en.wikipedia.org/wiki/Hash_function | nein | none | ✗ |

## §3 Compliance-Status
Beide Wrapper haben eine Paper-/Lehrbuch-Referenz und sind als Baseline/Lehrbuch-Konzept gekennzeichnet → Habich-Pflicht erfuellt. Es gibt keine is_original-Kandidaten dieser Achse (axis_03b_cache_traversal kommt in Map §3 nicht vor) und keine lizenz-blockierten Eintraege. Confidence laut Map: beide `high`. Keine Korrektur in Map §4 und kein offener Punkt in Map §5 betrifft diese Achse.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md: kein P-ID-Eintrag fuer diese Achse (beide Wrapper referenzieren klassische Lehrbuch-/Baseline-Literatur, keinen lokalen Repo-Klon).
