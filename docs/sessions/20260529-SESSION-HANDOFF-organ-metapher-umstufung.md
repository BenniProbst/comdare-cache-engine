# SESSION-HANDOFF (2026-05-29) — Doku-24-Säulen + Organ-Metapher-Umstufung

**Zweck:** Vollständiger Wiederaufnahme-Stand am Kontext-Schnitt. Direkt fortfahren — keine neue Planrunde nötig,
der Resume-Plan ist unten eindeutig. Aktives `/goal` (autonome Fortsetzung bis alle TODOs + Architektur vollständig).

---

## 0. PFLICHT-PRE-READ (vor jeder Lebewesen/Organ/Achsen-Arbeit ZUERST lesen)
1. **`docs/architektur/14_achsen_komposition_organ_metapher.md` (Doku 14) — VOLLSTÄNDIG, Teil 1–7 / §1–§42.** AUTORITATIV. Organ-Metapher, Gattungen, ExecutionEngine-Wurzel, Konfigurator. Bei jeder Unklarheit hier nachsehen, NICHT aus Gedächtnis/Workflow-Befund ableiten.
2. **`docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md` (Doku 24)** — 2-Dimensionen-Messmodell + §6 RESUME-PLAN + **§7 (Lebewesen→Organ-Umstufung + die erklärte Ordnung)**.
3. **`docs/architektur/10_schichten_modell_M.md`** (4-Subsystem-Modell) + **`11_konzept_achsen_extension_visitor_pattern.md`** + `11_axes_vs_strategies_disambiguation.md`.
4. Die 7 Design-Session-Doks dieser Serie: `20260529-saeule1-inc2-…`, `…-saeule2-observable-…`, `…-roadmap1-…`, `…-roadmap2-…`, `…-roadmap3-…`, `…-roadmap4-…`, `…-roadmap40-tier-wrapper-umstufung-…` (alle in docs/sessions/).
5. Memory `MEMORY.md` (Kritische Direktiven) — besonders die unten gelisteten.

---

## 1. DIE ERKLÄRTE ORDNUNG (KRITISCH, User 2026-05-29 — dreifach geschärft)

> **Achsen enthalten ausschließlich Organe — NIEMALS ganze Tiere.**
> **Ein Tier darf NUR SEZIERT vorliegen** (ausschließlich als Organ-Komposition / Gattungs-Konfigurator).
> Es gibt **NIE** monolithische Tiere als Achsen-Werte — auch nicht übergangsweise.
> Ein noch **nicht seziertes** Tier steht **außerhalb des Systems** (Doku 14 §3.1) — NICHT als Achsen-Wert.
> Ganze Tiere werden **unter ihrer Gattung** organisiert, mit einem **Konfigurator** (Composition über alle,
> teils optional genutzten, Organ-Achsen), den der CacheEngineBuilder metaprogrammiert zur **exakten
> Wiederherstellung** des Tieres aus seinen Organen.

Memory: `[[feedback_no_whole_tier_axes_genus_configurator]]` (oberste kritische Direktive).
Gattungs-Mechanismus existiert bereits: `SearchAlgorithmAnatomy<Composition>` + `AdHocComposition<17>` +
`SearchAlgorithmPermutationEngine` (Doku 14 §27–§29, §41–§42). 5 Gattungen (Mammal/Bird/Reptile/Adapter/View).

---

## 2. ERLEDIGT diese Session (alles verifiziert, getaggt, gepusht, Submodule-synchron)

ce-Repo `comdare-cache-engine` (origin = GitHub BenniProbst/comdare-cache-engine), Branch `main`:
| Increment | ce-Commit | Tag | ctest |
|-----------|-----------|-----|-------|
| Build-Unblock (Boost-Offline-Prerequisite, FortiGate-Egress) | — | — | — |
| Säule-1 Inc1 (node_type Storage-Organ) | `176c60b` | v41-saeule1-inc1-nodetype-storage-organ | 2012 |
| Säule-1 Inc2 (ComposedStore<N,L,A>) | `ccdbbde` | v41-saeule1-inc2-composed-store | 2013 |
| Säule-2 Kern (observe_all real search_algo) | `8bc39fb` | v41-saeule2-observe-all-real | 2014 |
| Roadmap-1 (observe_all 2. Achse: allocator) | `d26c5de` | v41-roadmap1-observe-allocator-axis | 2015 |
| Roadmap-2 INC-2a (Interpolation+Galloping-Organe) | `7d0f741` | v41-roadmap2-inc2a-interpolation-galloping | 2017 |
| Roadmap-2 INC-2b (BST-Organ über TreeNodePool) | `7155cae` | v41-roadmap2-inc2b-bst-treepool | 2019 |
| Roadmap-3 (TierObserveTrace Mess-Pfad) | `a89566c` | v41-roadmap3-tier-observe-trace | 2025 |
| Roadmap-4 (Säule-3 Cross-Varianten-Äquivalenz) | `f81ea2a` | v41-roadmap4-axis-cross-variant-equivalence | 2030 |
| #40-Design-Doku | `b64dc09` | (Restore-Tags pre-*) | — |

**da-Repo `probst-Diplomarbeit-cache-engine`** Submodule-Pointer zuletzt auf `f81ea2a` (`572e3f9`); **NACH dem
nächsten ce-Commit erneut bumpen** (Submodule-Sync-Regel). Tasks #32–#39 = completed; #40 = in_progress.

**Komponierbares Organ-Modell (Stand):** `composable/` enthält StorageOrgan-Concept, RawSlotStore,
4 Traversal-Organe (LinearScan/SortedBinary/Interpolation/Galloping), TreeNodePool-Concept + TreeNodePoolStore +
BSTTraversalOrgan + ComposedTreeSearch, NodeTypeSlotStore<N>, ComposedStore<N,L,A>, ObservableComposedSearch.
Alle uint64-Key, std::map-äquivalent (verify_matches_std_map). Säulen 1+2+3 substanziell erfüllt.

---

## 3. IN FLIGHT bei Kontext-Schnitt: #40-Rekonstruktions-Beleg

**Hintergrund-Build `boz2ruzht`** (2× configure + build + ctest) lief beim Schnitt noch. Er verifiziert den
**korrekt gerahmten** #40-Rekonstruktions-Beleg:
- NEU `composable/tier_to_organ_mapping.hpp` — Lebewesen→Organ-Pendant-Aliase (LinearScanOrgan/SortedBinaryOrgan/
  InterpolationOrgan/BstTreeOrgan) + TierOrganPair-Doku. **KEINE** „mp_size==17 muss bleiben"-Invariante (die
  würde die Umstufung blockieren).
- NEU `tests/unit/test_v41_axis_03a_tier_organ_equivalence.cpp` — belegt Lebewesen ≡ Organ-Komposition ≡ std::map
  (key-type-sicher: uint8→200/255, uint16→1000/1000). 3 Tests.
- GEÄNDERT `tests/unit/CMakeLists.txt` (+ Test-Block) + `docs/architecture/24_…md` (§7, korrekt gerahmt).

**RESUME-Schritt 0:** Build-Ergebnis von `boz2ruzht` lesen (`/tmp/…/tasks/boz2ruzht.output` bzw. neu bauen:
`cmake --preset msvc-release` 2× + build + `ctest -C Release`). Bei grün: committen
(„V41 #40: Tier→Organ-Rekonstruktions-Beleg"), Tag `v41-task40-tier-organ-reconstruction`, Push, **da-Pointer-Bump**.
Bei rot: Root-Cause (frühere identische Logik war grün bei exit 0 — bjl2ldaqx). #40 bleibt danach in_progress
(Beleg ist nur die Brücke; die eigentliche Umstufung ist das Programm unten).

---

## 4. RESUME-PLAN (User-Wahl „Option 3": erst ALLE sezieren, dann ALLE umstufen)

**Schritt A — restliche Lebewesen sezieren** (additiv, je build-grün, Design-Runde + Doku + Tag pro Schritt, wie INC-2b):
- **Hash** (HashSearchAlgo, Fibonacci-Open-Addressing) → neue Organ-Familie `HashBucketPool`-Concept + `BucketHashStore` + `ProbeTraversalOrgan` + `ComposedHashSearch` (analog TreeNodePool/BST).
- **SkipList** (SkipListSearchAlgo) → `SkipListPool` + Forward-Walk-Organ + `ComposedSkipSearch`.
- **B-Baum** (BTreeSearchAlgo, t=4 CLRS) → `BTreeNodePool` (Multiway, key[7]/child[8]) + B-Tree-Walk-Organ + `ComposedBTreeSearch`.
- **OriginalXxx (S04–S08, Paper-Bindung)** → paper-gebundene Organ-Kompositionen (Habich-Original-Code pro Organ, ext/-Struktur). Auch diese MÜSSEN seziert werden (keines bleibt monolithisch).
- Pro seziertem Lebewesen: Äquivalenz-Beleg (Lebewesen ≡ Organ-Komposition ≡ std::map) ergänzen.

**Schritt B — ALLE Lebewesen gemeinsam umstufen** (der große, kaskadierende Refactor, tag-gesichert, gestaffelt):
1. `axis_03a::EnabledStrategies` auf **nur Traversal-Organe** reduzieren (alle Monolith-Wrapper raus).
2. Jedes Lebewesen als Gattungs-Konfigurator rekonstruieren: `Composition` (node_type ⊕ traversal ⊕ storage ⊕ …)
   → `SearchAlgorithmAnatomy<Composition>`. Die 11 anatomy-Compositions (`compositions/*_reference.hpp`)
   `search_algo`-Slot von Monolith (Array256 etc.) auf die Organ-Komposition umverdrahten.
3. Harte Tests/Asserts anpassen: `test_v41_topic_traversal.cpp:73` (TYPED_TEST über AllStrategies),
   `:1213/1220` (SimdSubset==6/DenseSubset==2), `test_v41_search_algorithm_permutation_engine.cpp:476-497`
   (kSearch==17), `known_compositions_list.hpp:81-83` (==11). Diese Zahlen ändern sich → bewusst aktualisieren.
4. Verifizieren: alle Tests grün; jedes Lebewesen weiterhin std::map-äquivalent (jetzt als Composition).

**Konsumenten, die beim Entfernen brechen** (alle in Schritt B mitziehen): PermutationEngine /
`topic_traversal_config_set.hpp:23` (StaticAxisVariants_03a) + `mp_all_of`-min-1-Vendor-Invariante;
11 Compositions; `known_algorithms.hpp`; o.g. harte Asserts. (Quelle: Workflow `wfuczshtl`-Befund.)

---

## 5. DIREKTIVEN (Memory — immer beachten)
- `[[feedback_no_whole_tier_axes_genus_configurator]]` — **NIE monolithische Lebewesen; Lebewesen nur seziert; Gattungs-Konfigurator; PFLICHT-Pre-Read Doku 14.** (oberste Priorität)
- `[[feedback_achsen_komposition_organ_metapher]]` — Achse=Organ, Algorithmus=Permutation aller Achsen.
- `[[feedback_zwei_dimensionen_messmodell]]` — 3 Mess-Aspekte (Lebewesen-Wall-Clock / observe_all / Achsen-Vergleich).
- `[[std_map_unified_interface]]` — std::map ist das einheitliche Vergleichs-Interface.
- `[[reference_anatomie_gattungen]]` + `[[feedback_execution_engine_als_wurzel]]` + `[[feedback_gattungs_constraint_pruefling_merge]]` — Gattungen + ExecutionEngine-Wurzel + Cross-Genus verboten.
- `[[reference_boost_mp11_offline_prerequisite]]` — Build offline via vendored mp11 (FortiGate-Egress-Block).
- `[[reference_cache_engine_standalone_build_pipeline]]` — `cmake --preset msvc-release` 2× (GLOB) + build + `ctest -C Release`; build/msvc-release.
- `[[feedback_destructive_autonomy_3repos_with_tag]]` — destruktiv OK in 3 Thesis-Repos mit Tag+Commit+Push (reversibel); Remotes NIE löschen; regelmäßig pushen.
- `[[feedback_submodule_sync_3repos]]` — nach jedem ce-Push da-Pointer bumpen.
- `[[feedback_no_quick_fixes]]`, `[[feedback_no_runtime_switch]]` (nur if constexpr/Concepts), `[[feedback_technical_identifiers_over_metaphor]]` (Code technisch, Metapher nur in Doku/Kommentar), `[[feedback_never_delete_documentation]]`, `[[project_active_goal_directive]]` (aktiv).
- Commit-Trailer: `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`. Nur meine Dateien stagen (NICHT `ext/A05-jemalloc/`). 204/327-MB-Boost-Archive NIE nach GitHub.

---

## 6. REPO-STAND
- ce HEAD `b64dc09` (+ uncommittete #40-Artefakte: tier_to_organ_mapping.hpp, test_v41_axis_03a_tier_organ_equivalence.cpp, CMakeLists.txt-Block, Doku 24 §7) — committen wenn `boz2ruzht` grün.
- da HEAD `572e3f9` (Pointer auf `f81ea2a`) — nach #40-ce-Commit bumpen.
- Restore-Tags pro Increment gesetzt (pre-*-20260529). `ext/A05-jemalloc/` untracked (Allocator-Task, nicht meins).
