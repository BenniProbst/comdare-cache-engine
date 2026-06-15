# Session — Roadmap-4 (Säule 3: Achsen-Vergleich): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow · **Task:** #39 (User-Roadmap Schritt 4, Pflicht — letzter)
**Workflow:** `w0qhj9lp6` / Run `wf_896dd6f7-1a2` — 7 Agenten, ~626k Subagent-Tokens, 126 Tool-Uses.
**Zweck:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 24 §2.3/§2.4 (Achsen-Vergleich = Interface-Tests vs. bekannte Algos, NICHT Latenz). Vorgänger: Roadmap-3 (ce `a89566c`).

> **Frage:** Wie realisieren wir Säule 3 — den Achsen-Vergleich gegen das vereinheitlichte std::map-Interface
> vs. bekannte Algorithmen, als eigene Korrektheits-/Interface-Dimension getrennt von der Latenz?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)
- **Harness** `verify_matches_std_map<Wrapper>(key_mod, query_max)` (`test_v41_topic_traversal.cpp:543-567`): template (K=Wrapper::key_type), 600 gemischte Ops (Konstante 2654435761u, i%7==0→erase, v=k·11+1), Lookup-Sweep, occupied_count==size. **22 bestehende Aufrufe** (12 schmale Lebewesen-Wrapper uint8/uint16 + 10 composable Organ-Kombinationen).
- **Was fehlt für §2.3:** (1) **Cross-Varianten-Äquivalenz** (alle Varianten EINER Achse über DENSELBEN Op-Stream == untereinander, nicht nur einzeln == std::map); (2) explizite „welche Variante je Achse"-Dimension (geordnet/range); (3) systematischer typed-Vergleich.
- **Varianten-Inventar (composable, key=uint64):** 5 Traversal-Organe (LinearScan/SortedBinary/Interpolation/Galloping über RawSlotStore/ComposedStore + BST über TreeNodePoolStore). Alle key_type==uint64 → teilen EINEN Op-Stream → Cross-Varianten strukturell möglich (anders als schmale uint8/uint16-Wrapper).
- **§2.3-Soll:** std::map = einheitliches Vergleichs-Interface; Achsen beschreiben INNEN-Verhalten; „welche Variante besser" auf DIESER Dimension = Korrektheit/Eigenschaften (geordnet, range), NICHT Latenz.

## 2. Design (3 Linsen) + gewählt
- **A — Cross-Varianten-Äquivalenz** (horizontal == untereinander): **GEWÄHLT (Kern)**.
- **B — typed-Harness + Eigenschafts-Tabelle** (geordnet/range): **Graft** (constexpr-Klassifikation pro Organ).
- **C — AxisComparisonResult-Struct + Engine-Header**: **verworfen** (überdimensioniert, fügt Engine-Code hinzu; reine Test-Artefakte genügen).

**Korrekturen:** (1) `verify_matches_std_map` NICHT duplizieren → in einen Support-Header **verbatim extrahieren** (Bestandsdatei behält ihre lokale Kopie, bleibt unangetastet; neuer Code nutzt die extrahierte Version). (2) composable Traversal-Organe haben **kein** `supports_range_scan()` → geordnet/range als **lokale constexpr-Klassifikation** im Test (Ordered/Unordered/Tree). (3) Cross-Vergleich prüft **nur lookup-Resultate + occupied_count**, NICHT interne Slot-Reihenfolge (Korrektheit ⊥ Innen-Verhalten).

## 3. Gewählter Blueprint
**NEU** `tests/unit/support/std_map_equivalence_harness.hpp` (namespace `comdare::cache_engine::test_support`): `verify_matches_std_map<Wrapper>` (verbatim) + `verify_variants_equivalent<Anchor, Others...>` (variadisches Fold: ein deterministischer Op-Stream auf Anchor + jede Other-Variante; dann je Other gegen Anchor: Lookup-Sweep-Gleichheit + occupied_count; duck-typed → ComposedSearch UND ComposedTreeSearch ohne gemeinsamen Basistyp; transitiv: Other==Anchor && Anchor==std::map ⇒ alle == std::map).
**NEU** `tests/unit/test_v41_axis_03a_cross_variant_equivalence.cpp` (GTest-TU, gleiche composable-Includes + Support-Header).
**EDIT** `tests/unit/CMakeLists.txt` (additiver Test-Target-Block, ALL_AXIS_GENERATED_DIRS + Boost::mp11).
**Unangetastet:** test_v41_topic_traversal.cpp, alle Organ-Header, Registry, Engine/Lib.

## 4. Test-Plan (deterministisch, latenzfrei)
- FlatOrgansEquivalentToStdMap: 4 Flat-Varianten einzeln == std::map (km=100000).
- FlatOrgansEquivalentToEachOther: `verify_variants_equivalent<LinearScan, SortedBinary, Interpolation, Galloping>` — horizontal == untereinander (Kern-These).
- TreeOrganEquivalentToFlatAnchor: BST == std::map + == Flat-Anker.
- StorageSwapEquivalent (optional): LinearScan/SortedBinary über ComposedStore<…Mimalloc> vs <…Pmr> == .
- OrderingPropertyTable: constexpr-Klassifikation (LinearScan→Unordered, Sorted/Interp/Gallop→Ordered, BST→Tree); static_assert heterogen (≥2 Klassen) + trotzdem identisches std::map-Resultat → Korrektheit ⊥ Innen-Verhalten.
- IsLatencyFree: dokumentierend — keine chrono/ns/Throughput-Felder.

## 5. Scope-Grenze
**DRIN:** Korrektheits-/Interface-Dimension der composable axis_03a-Organe (vertikal == std::map + horizontal == untereinander + Eigenschaftstabelle), rein additiv, latenzfrei.
**DRAUSSEN:** echte Performance-Rangfolge („welche schneller") = Lebewesen-Wall-Clock-Dimension (Roadmap-3/V42, NULL Latenz-Felder hier); horizontaler Vergleich der schmalen Lebewesen-Wrapper (Key-Type-Normalisierung nötig; vertikal bereits abgedeckt); property-gefilterte Sub-Suiten + kartesisches 03a×03b×03m-Produkt (Roadmap-4b); Observer/Statistics-Gleichheit (Innen-Verhalten DARF abweichen).
