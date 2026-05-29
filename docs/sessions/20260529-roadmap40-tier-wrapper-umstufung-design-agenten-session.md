# Session — #40 (Tier-Wrapper-Umstufung, Doku 24 §6.2): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow · **Task:** #40 (letzter Doku-24-Punkt)
**Workflow:** `wfuczshtl` / Run `wf_1e76c197-487` — 7 Agenten, ~708k Subagent-Tokens, 201 Tool-Uses.
**Zweck:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 24 §6.2 (umstufen) ↔ Doku 14 §14.2 (PROMOTION, keine Löschung). Vorgänger: Roadmap-4 (ce `f81ea2a`).

> **Frage:** Wie stufen wir die monolithischen Tier-Wrapper (Array256/BST/…) zu Reference-Compositions/
> Stufe-2-Referenzen um — ohne Algorithmus/Test-Verlust und ohne die Doku-Spannung zu verletzen?

---

## 1. Understand-Phase (3 Reader, code-verifiziert) — Entfernen bricht ALLES
- **Registry** `axis_03a_search_algo_registry.hpp:44-77`: `AllStrategies` = 17 Wrapper (S01-S17), `EnabledStrategies` = mp_filter. **Konsumenten:** PermutationEngine (`topic_traversal_config_set.hpp:23` → StaticAxisVariants), `mp_all_of`-min-1-Vendor-Invariante, **alle 11 anatomy-Compositions** (z.B. `art_reference.hpp:58` → Array256SearchAlgo als search_algo), `known_algorithms.hpp` (11 Instanzen), TYPED_TESTs über AllStrategies (`test_v41_topic_traversal.cpp:73`), harte `static_assert`s (SimdSubset==6, DenseSubset==2 `:1213/1220`; kSearch==17 `permutation_engine_test:476-497`; ==11 Compositions `known_compositions_list.hpp:81-83`).
- **Befund:** Entfernen von S01-S08 aus AllStrategies/EnabledStrategies → Compile-Bruch-Kaskade (alle 11 Compositions + ≥5 harte static_asserts + TYPED_TESTs). HOCH riskant.
- **Doku-Spannung aufgelöst:** Doku 14 §14.2 (Z.523 „KEINE Loeschung — bleiben legitime Achsen-Werte") ↔ Doku 24 §6.2 (Z.232 „umstufen/reduzieren"). Widerspruchsfreie Lesart: **„umstufen" = Rollen-Umklassifizierung (Stufe-1-Vorrang → Stufe-2-Referenz), NICHT Existenz-Entzug.** Parallel-Existenz.

## 2. Design (3 Linsen) + gewählt
- A — additiv-parallel (Wrapper bleiben + Organe als Reference dokumentiert): Basis.
- B — EnabledStrategies reduzieren + Wrapper in Stufe-2-Schicht: **verworfen** (editiert `test:1172`, berührt min-1-Vendor-Invariante → Risiko).
- **C — verifizierbarer Mapping-Beleg (additiv-dokumentarisch): GEWÄHLT** + Graft aus A.

**Korrektur:** Roadmap-4-Äquivalenz-Setup existiert bereits (`test_v41_axis_03a_cross_variant_equivalence.cpp`) → die Organe sind schon als ==std::map zertifiziert; der neue Beleg prüft nur das **Tier↔Organ-Paar**.

## 3. Gewählter Blueprint — „Vollständigkeit vor Radikalität"
**Reclassification = Rollen-Umklassifizierung, KEINE Mengen-Reduktion. An Registry/EnabledStrategies/Compositions ändert sich NICHTS.**

**NEU** `composable/tier_to_organ_mapping.hpp`: deklariert pro monolithischem Tier-Wrapper sein komponierbares Organ-Pendant (Stufe-1) + trägt `static_assert(mp_contains<AllStrategies, tier>)` pro Eintrag → **beweist zur Compile-Zeit, dass kein Wrapper entfernt wurde** (kodiert Doku 14 §14.2 im Code) + markiert den Tier maschinenlesbar als Stufe-2-/Reference-Baseline (Doku 24 §6.2).

**NEU** `tests/unit/test_v41_axis_03a_tier_organ_equivalence.cpp`: pro Mapping-Eintrag `verify_matches_std_map<tier>` + `verify_variants_equivalent<organ, tier>` (Roadmap-4-Harness) → Tier ≡ Organ-Pendant ≡ std::map. **KORREKTUR (key-type-sicher):** key_mod/query_max INNERHALB der schmalen Tier-Key-Breite (uint8 → 200/255; uint16 → 1000/1000), sonst kollidieren Keys beim Cast.

**EDIT** `tests/unit/CMakeLists.txt` (additiver Test-Block) + **Doku 24 §7** (Tier→Organ-Mapping-Abschnitt).

**Mapping-Tabelle (nur Tiere mit bereits ==std::map-bewiesenem Organ-Pendant):**
| Tier (Sn) | key_type | Organ-Pendant |
|---|---|---|
| Array256(S01), VectorU8U8(S02) | uint8 | `ComposedSearch<LinearScanTraversal, RawSlotStore>` |
| VectorU16U16(S03), Array65535(S09), KAry(S10), Eytzinger(S12), LinearScan(S15) | uint16 | `ComposedSearch<{Linear|SortedBinary}Traversal, RawSlotStore>` |
| Interpolation(S11) | uint16 | `ComposedSearch<InterpolationTraversalOrgan, RawSlotStore>` |
| BinarySearchTree(S16) | uint16 | `ComposedTreeSearch<BSTTraversalOrgan, TreeNodePoolStore>` |

## 4. Was sich an Registry/Compositions ändert: NICHTS
registry/EnabledStrategies/alle 11 Compositions/known_algorithms byte-identisch. Einziger Registry-Kontakt: read-only `#include` für den `mp_contains`-Selbstcheck. Alle 12+ Wrapper-Tests + 11 Anatomien + Roadmap-4-Test bleiben **per Konstruktion** grün.

## 5. Scope-Grenze (Folge-Increments, NICHT jetzt)
- **Kein Organ-Pendant** → Hash(S14), SkipList(S13), B-Baum(S17) (eigene Substrat-Familien); 5 OriginalXxx(S04-S08) bleiben Stufe-2-Paper-Baselines (Doku 14 §14.2).
- EnabledStrategies editieren / Compositions auf Organe umverdrahten (Doku 24 §6 Folge 1+3); Key-Typ-Reconciliation / Anatomie auf ComposedSearch; prt-art-Pruefling-Merge (#8).
- Reine Addition, ein `git tag` macht alles reversibel. Ändert KEIN Laufzeitverhalten + keine Registry-Menge — die maschinell geprüfte Brücke, auf der die riskanteren Säule-1-Folge-Increments aufsetzen.
