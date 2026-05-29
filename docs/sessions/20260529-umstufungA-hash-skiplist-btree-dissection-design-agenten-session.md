# Umstufung-A (Task #41) — Sezier-Design Hash / SkipList / B-Baum: Agenten-Workflow-Ergebnis

**Stand:** 2026-05-29
**Workflow:** `wvnzp8jkp` / Run `wf_c7dd6b30-9ac` (7 Agenten, 604k Subagent-Tokens, 122 Tool-Calls, ~468 s)
**Auftrag:** „Understand+Design: Hash/SkipList/B-Baum (axis_03a-Monolithen) in komponierbare
Organ-Familien sezieren (Pool/Store-Concept + Traversal-Organ + ComposedXSearch + Äquivalenz-Beleg,
analog TreeNodePool/BST). Erklärte Ordnung: Tier nur seziert."
**Bezug:** Doku 14 §1–§3 (Organ-Metapher), §3.1 (Gesamt-Algorithmus außerhalb des Systems bis zerlegt),
Doku 24 §6/§7 (erklärte Ordnung), `[[feedback_no_whole_tier_axes_genus_configurator]]` (NIE Ganz-Tiere).

> **Einordnung in die erklärte Ordnung:** Diese Charge (#41) belegt NUR die *Rekonstruierbarkeit*
> (Brücke). Sie entfernt NICHTS aus `EnabledStrategies`/Registry und legt KEINE Ganz-Tier-Achsen an.
> Das tatsächliche Entfernen + Rekonstruktion als Gattungs-Konfiguratoren ist **#42 (Umstufung-B)**.
> Hash/SkipList/B-Baum sind `[[pseudocode-papers-fallback]]` (`is_original=false`); echtes Paper-Code-
> Linking der OriginalXxx-Tiere ist eine spätere Charge.

---

## 1 Phase „Understand" — Anatomie der 3 Monolithen (3 Reader-Agenten)

### 1.1 Vollständige Organ-Zerlegung (Substrat vs Traversal)

| Monolith | Datei (axis_03a_search_algo/) | Substrat-Organ (Pool/Store) | Traversal-Organ (Such-Logik) |
|---|---|---|---|
| **HashSearchAlgo** | `axis_03a_search_algo_hash_search.hpp:42–212` | `buckets_[Slot{key,val,state}]` (204), `mask_` (205), `tombstones_` (207), `SlotState::Empty/Occupied/Deleted` (167), `rehash()` bei load≥0.7 (175) | Fibonacci-Hash `hash_index()` (171: `k*11400714819323198485 & mask_`), Linear-Probe-Loop (79: `(start+i)&mask_`), Tombstone-Reuse (84) |
| **SkipListSearchAlgo** | `axis_03a_search_algo_skip_list.hpp:52–226` | `nodes_[Node{key,val,live,next<uint32>[]}]` (218), forward-Index je Level (189), `kHead`-Sentinel (65), `level_` (220), `rng_` mt19937_64 (221), `live_count_` (219) | `find_update()` Multi-Level-Walk `level_-1 → 0` (118–123), `random_level()` Coin-Flip P=0.5 (212–216), Forward-Verkettung (104–106) |
| **BTreeSearchAlgo** | `axis_03a_search_algo_btree.hpp:49–371` | `nodes_[alignas(64) Node{n,leaf,key[7],val[7],child[8]}]` (363–370), `free_`-List (364), `root_` (365), `t=4 kMaxKeys=7 kMaxChildren=8` (158–160) | CLRS B-Tree-Walk `lookup()` (99–115), top-down-split `insert()` (78–97), CLRS-delete `erase()` (118–152), `split_child`/`merge`/`borrow` (209–353 Transformations-Organe) |

**Schlüssel-Constraints:** key=uint16, value=uint64 (Monolithe). Hash `supports_range_scan=false`
(ungeordnet); SkipList `supports_range_scan=true` (geordnet); BTree `has_cache_line_alignment=true`
(`alignas(64)` — das messbare F15-Merkmal, muss erhalten bleiben). Alle: no_simd, no_thread_safe.
Hash-Load-Trigger: `(size_+tombstones_)*10 ≥ (mask_+1)*7`. SkipList `kMaxLevel=16, kNil=0xFFFFFFFFu,
kHead=0u, Seed=0xC0FFEEu`. BTree `kT=4, root_=kNil=0xFFFFFFFFu`.

### 1.2 Verallgemeinertes Sezier-Muster (Reader 2)

Das BST-Muster (`TreeNodePool → TreeNodePoolStore → BSTTraversalOrgan → ComposedTreeSearch`) ist ein
4-Schichten-Bauplan, der **1:1 dreimal repliziert** wird; die Varianz liegt NUR im Pool-Concept-Signatur
+ in der Traversal-Organ-Implementierung. Kritische Insights:

1. **Concept-Trennung ist zentral** — BST-Pools ≠ Hash-Pools ≠ SkipList-Pools (unvereinbare Invarianten).
   KEIN Universal-Pool, sondern `HashBucketPool`, `SkipListNodePool`, `BTreeNodePool` als **separate Concepts**.
2. **Index-Stabilität variiert** — BST/SkipList/BTree index-stabil (Free-List/kein Rehash); Hash-Rehash
   destabilisiert alle Indizes → eigenes Concept ohne Index-Stabilitäts-Garantie.
3. **Traversal-Organ-Austauschbarkeit ist NICHT universell** — jedes Organ spezialisiert auf EIN Pool-Concept.
4. **Selbstbeweis-Pattern** — jede Store/Organ-Datei endet mit `static_assert(Concept<Typ>)`.

### 1.3 Äquivalenz-Beleg-Rahmen (Reader 3)

`tests/unit/support/std_map_equivalence_harness.hpp` (result-only, NICHT Slot-Reihenfolge, Z.66–81):
- `verify_matches_std_map<W>(key_mod,query_max)` — vertikal: deterministische 600-Op-Sequenz
  (seed 2654435761, 86% insert `v=k*11+1`, 14% erase `i%7==0`, Lookup-Sweep) gegen `std::map`; latenzfrei.
- `verify_variants_equivalent<Anchor,Others...>(key_mod,query_max)` — horizontal: identischer Op-Stream,
  Vergleich `occupied_count()` + Lookup-Sweep. **Duck-typed** (keine gemeinsame Basis nötig).
- Transitivität: `Organ≡Monolith ∧ Monolith≡std::map ⇒ Organ≡std::map`. Key-Mod uint16-Tiere = **1000/1000**.

---

## 2 Phase „Design" — 3 Linsen

| Linse | Ansatz | Risiko | Verdikt |
|---|---|---|---|
| **A** | 3 eigenständige Pool-Concept-Familien analog INC-2b, je eigene `ComposedXSearch`-Schale (17 Z.) | medium | Basis für die Wahl |
| **B** | Gemeinsames `NodePoolBase`-Meta-Concept + EINE `ComposedNodePoolSearch` | medium | **VERWORFEN** |
| **C** | 3 **isolierte** je-Struktur build-grüne Increments (Hash→SkipList→B-Baum) | **low** | **GEWÄHLT** |

**Warum C (gegrafted mit A's `composed_*_search.hpp`-Begründung):** Der Auftrag verlangt Bruch-Risiko-
Isolation („je Struktur ein eigener build-grüner Increment"). Linse B koppelt die 3 Familien über eine
geteilte Basisdatei → ein Concept-Schnittfehler färbt alle 3 gleichzeitig rot (Gegenteil von Isolation)
und bringt keinen realen Payoff, weil `allocate_node`-Signaturen strukturell divergieren (Hash `(k,v)→idx`,
SkipList zieht intern Level, BTree `(leaf)→idx`) — die einzige echte Gemeinsamkeit (`node_count/clear/
kNil/uint64`) ist trivial. Linse C macht die Increment-Grenze (Build grün, bevor nächste Struktur) zur
harten Regel.

### 2.1 Korrekturen gegen die Understand-Vorschläge (Blueprint-verifiziert)

- **`HashSlotStore` als `StorageOrgan` (Understand-Vorschlag) ist FALSCH** → verworfen. `RawSlotStore`/
  `StorageOrgan` haben Index-Shift-Semantik (`insert_slot_at`/`erase_slot_at`), die Open-Addressing +
  Tombstones bricht. Hash braucht ein **eigenes** `HashBucketPool`-Concept, KEINE StorageOrgan-Erweiterung.
- **SkipList-RNG gehört in den Store** (`random_level()` mutiert `rng_`, ist non-const; `find_update()` const).
  `draw_level()` = mutierende Store-Methode; das Organ bleibt **stateless**.
- **`ComposedTreeSearch` ist NICHT wiederverwendbar** — es `requires TreeTraversalOrgan<Traversal,Pool>`
  und ruft `pool_.node_count()`; Hash/SkipList/BTree haben andere Concepts (`occupied()`/`live_count()`/
  `size()`). Daher je eine eigene 17-Zeilen-`Composed*Search`-Schale.
- **`alignas(64)` des B-Baum-Node bleibt erhalten** (= messbares `has_cache_line_alignment`).
- **B-Baum `Node&`-Caching entfällt** beim Port auf Pool-Getter/Setter automatisch (Reallokations-Falle weg).
- **MSVC: kein `unsigned __int128`** — alle 3 nutzen nur `uint64_t`-Multiplikation mit `& mask_` (MSVC-sicher).

---

## 3 Gewählter Blueprint — 12 neue additive Header (3 Familien × 4)

Alle unter `libs/cache_engine/topics/traversal/axis_03a_search_algo/composable/`:

| Increment | Concept | Store (+self-assert) | Organ (+self-assert) | Komposition | Mapping-Alias |
|---|---|---|---|---|---|
| **1 HASH** | `hash_bucket_pool_concept.hpp` | `hash_bucket_pool_store.hpp` | `hash_probe_traversal_organ.hpp` | `composed_hash_search.hpp` | `HashSearchOrgan = ComposedHashSearch<HashProbeTraversalOrgan, HashBucketPoolStore>` |
| **2 SKIPLIST** | `skip_list_node_pool_concept.hpp` | `skip_list_node_pool_store.hpp` | `skip_list_traversal_organ.hpp` | `composed_skip_list_search.hpp` | `SkipListOrgan = ComposedSkipListSearch<SkipListTraversalOrgan, SkipListNodePoolStore>` |
| **3 BTREE** | `btree_node_pool_concept.hpp` | `btree_node_pool_store.hpp` | `btree_traversal_organ.hpp` | `composed_btree_search.hpp` | `BTreeSearchOrgan = ComposedBTreeSearch<BTreeTraversalOrgan, BTreeNodePoolStore>` |

**Additiv geändert (KEINE neuen Dateien):** `composable/tier_to_organ_mapping.hpp` (3 `#include` + 3 `using`
nach Z.29); `tests/unit/test_v41_axis_03a_tier_organ_equivalence.cpp` (3 neue TEST-Cases nach Z.50).
**KEINE Änderung:** `tests/unit/CMakeLists.txt` (Target Z.410–419 inkludiert bereits
`${PROJECT_SOURCE_DIR}/libs/cache_engine` + Boost::mp11), Registry, alle Bestands-Organe/-Concepts/-Tests.

### 3.1 Concept-Pflicht-API je Familie (uint64 key==value durchgängig)

**`HashBucketPool<S>`:** `{S::kNil}`; const `bucket_count()`, `slot_state(i)→int` (0=Empty/1=Occ/2=Del),
`slot_key(i)`, `slot_value(i)`, `occupied()`, `tombstones()`; mut. `set_slot(i,k,v,state)`,
`set_slot_value(i,v)`, `mark_deleted(i)`, `rehash(newcap)` (darf werfen, `[[allocation-failure-exception]]`),
`clear()`. Store-Substrat 1:1 hash_search.hpp:166–194, Ktor `buckets_(16), mask_(15)`. `hash_index` im Store.
Organ-Konstante `kFibonacciMul=11400714819323198485ULL`. ComposedHashSearch `occupied_count()→pool_.occupied()`.

**`SkipListNodePool<S>`:** `{S::kNil}`, `{S::kHead}`, `{S::kMaxLevel}`; const `head()`, `list_level()→int`,
`live_count()`, `node_key/value(i)`, `node_live(i)→bool`, `forward_at(i,lvl)`; mut. `allocate_node(k,v,lvl)→idx`,
`draw_level()→int` (**RNG-mutierend, im Pool**), `node_level(i)`, `set_forward_at(i,lvl,t)`, `set_node_value(i,v)`,
`set_node_live(i,b)`, `set_list_level(lvl)`, `dec_live()`, `clear()`. Store 1:1 skip_list.hpp:184–221,
`mutable mt19937_64 rng_{0xC0FFEEu}`, Ktor `init_head()`. ComposedSkipListSearch `occupied_count()→pool_.live_count()`.

**`BTreeNodePool<S>`:** `{S::kNil}`, `{S::kT}`, `{S::kMaxKeys}`, `{S::kMaxChildren}`; const `root()`, `size()`,
`node_n(i)→int`, `node_leaf(i)→bool`, `node_key_at(i,j)`, `node_value_at(i,j)`, `node_child_at(i,j)`; mut.
`new_node(leaf)→idx`, `free_node(i)`, `set_root(i)`, `set_node_n(i,n)`, `set_node_leaf(i,b)`,
`set_node_key_at/value_at/child_at(...)`, `clear()`. **Split/Merge/Borrow im Organ, NICHT im Pool.** Store 1:1
btree.hpp:157–181/363–366, `struct alignas(64) Node` erhalten. Organ-Helfer: split_child/insert_nonfull/
contains/update_existing/find_key/remove_from/remove_from_nonleaf/fill/borrow_from_prev/borrow_from_next/merge
(alle `template<Pool>`, `nodes_[i].xxx → p.node_xxx(i)`). ComposedBTreeSearch `occupied_count()→pool_.size()`.

### 3.2 Build-grüne Reihenfolge (Lens C)

S0 (Ist-State, `[[verify-ist-state-before-gross-tasks]]`): `./configure.sh` → `build/msvc-release` →
Target `test_v41_axis_03a_tier_organ_equivalence` GRÜN. Build über **`./configure.sh`** (Buildsystem),
NICHT `cmake -B` direkt. **Hash zuerst** (flachster Pool, keine Tree-Berührung, kein RNG) → **SkipList**
(RNG-im-Store) → **B-Baum zuletzt** (11 CLRS-Helfer, höchstes Bruchrisiko isoliert ans Ende). Pro Increment
INNEN: Concept → Store(+assert) → Organ(+assert) → ComposedXSearch → Mapping-Alias → Test → Build grün,
BEVOR der nächste Increment beginnt.

### 3.3 Test-Plan (additiv nach Z.50, key_mod=1000/query_max=1000)

```cpp
TEST(Axis03aTierOrgan, Uint16HashReconstructibleFromOrgan) {
    ts::verify_matches_std_map<ce_cmp::HashSearchOrgan>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::HashSearchOrgan, ce_03a::HashSearchAlgo>(1000u, 1000u);
    SUCCEED();
}
// + Uint16SkipListReconstructibleFromOrgan + Uint16BTreeReconstructibleFromOrgan analog
```
Doppel-Beleg: vertikal (Organ≡std::map) + horizontal (Organ≡Monolith) ⇒ transitiv Organ≡std::map.
Äquivalenz-Risiko: Harness result-only → Hash-Ungeordnetheit unkritisch; SkipList deterministischer Seed
`0xC0FFEEu` + Verbatim-Port (`draw_level` nur nach Update-Negativ-Check) → Bit-Gleichheit; B-Baum deterministisch.

---

## 4 Scope-Grenze

**DRIN (#41, additiv):** 12 neue Header; 3 `using` + 3 `#include` in `tier_to_organ_mapping.hpp`; 3 TEST-Cases;
je Familie `static_assert`-Selbstbeweis (Store erfüllt Concept + Organ erfüllt Organ-Concept). uint64-Key,
statische `insert_into/lookup_in/erase_from`, KEINE Runtime-Switches (`[[no-runtime-switch]]`), Body 1:1
verbatim aus Monolith (kein Quick-Fix, `[[no-quick-fixes]]`).

**DRAUSSEN (spätere Chargen):** (a) OriginalXxx-Paper-Code-Bindung (`is_original`-Linking, `legacy_code/ext/
paper_<id>`) — Hash/SkipList/B-Baum sind `[[pseudocode-papers-fallback]]`; (b) Entfernen aus
`EnabledStrategies` + Rekonstruktion als Gattungs-Konfiguratoren = **#42 (Umstufung-B)**; (c) Registry,
Monolith-Wrapper, Bestands-Organe/-Stores/-Concepts, `composable_search.hpp`, `tree_*`-Header,
`observable_composed_search.hpp`, alle Bestands-Tests/-asserts UNANGETASTET; (d) kein Universal-„SuperPool";
(e) keine CMakeLists-Änderung; (f) Statistics/observe_all-Anbindung der neuen Organe (späteres Increment).

---

## 5 Verweise

- Workflow-Output (vollständig): `tasks/wvnzp8jkp.output` (99k chars; understanding[3] + designs[3] + blueprint).
- Handoff: `docs/sessions/20260529-SESSION-HANDOFF-organ-metapher-umstufung.md`.
- Erklärte Ordnung: Doku 24 §7 + `[[feedback_no_whole_tier_axes_genus_configurator]]`.
- Organ-Metapher (Pflicht-Pre-Read): `<Diplomarbeit>/docs/architektur/14_achsen_komposition_organ_metapher.md`
  §1–§42 (Teil 1–7) — Organ-Metapher, 5 Gattungen (§27), AnatomyBase (§27.1), Gattungs-Constraint (§32),
  ExecutionEngine-Wurzel (§33–§40), technische Identifier (§41); §43–§54 = R5.D–R6.A-Sprint-Lieferungen.
- Vorlage: BST-Familie `composable/tree_node_pool_concept.hpp` + `tree_node_pool_store.hpp` +
  `tree_traversal_organ.hpp` + `composed_tree_search.hpp` (INC-2b).

---

## 6 LIEFERSTAND #41-Schritt-A (2026-05-29) — 3 CE-native Strukturen seziert

Alle 3 CE-nativen Monolithen sind seziert, build-grün, äquivalenz-belegt, getaggt, gepusht, da-synchron:

| Inc | Struktur | ce-Commit | Tag | Test |
|---|---|---|---|---|
| 1 | **Hash** (HashSearchAlgo S14) | `a9bb093` | `v41-umstufungA-inc1-hash-dissection` | `Uint16HashReconstructibleFromOrgan` |
| 2 | **SkipList** (SkipListSearchAlgo S13) | `d109c86` | `v41-umstufungA-inc2-skiplist-dissection` | `Uint16SkipListReconstructibleFromOrgan` |
| 3 | **B-Baum** (BTreeSearchAlgo S17) | `d914a6e` | `v41-umstufungA-inc3-btree-dissection` | `Uint16BTreeReconstructibleFromOrgan` + `BTreeDeleteStressMatchesStdMap` (adversarial, 400 Keys, borrow/merge/root-shrink) |

`ctest test_v41_axis_03a_tier_organ_equivalence`: **7/7 grün** (3 Bestand + 3 neue + 1 Stress).
da-Pointer zuletzt `57b2bed` → ce `d914a6e`. 12 neue Header in `composable/` + 3 Mapping-Aliase
(`HashSearchOrgan`/`SkipListOrgan`/`BTreeSearchOrgan`) + 4 TEST-Cases. Bestand unangetastet.
**Naming-Korrektur ggü. Blueprint:** Concept ≠ Struct (C++-Namenskonflikt) — `HashProbeTraversal` vs
`HashProbeTraversalOrgan`, `SkipListTraversal` vs `SkipListTraversalOrgan`, `BTreeTraversal` vs
`BTreeTraversalOrgan`. **B-Baum-Inc3:** `inc_size()`/`dec_size()` zum Concept ergänzt (logische
Schlüsselzahl ≠ Knotenzahl — Blueprint-Lücke geschlossen). `alignas(64)`-Node erhalten (F15-Merkmal).

## 7 OFFEN #41-Schritt-A — OriginalXxx-Tiere (eigene Design-Runde nötig)

Ist-State (`axis_03a_search_algo_registry.hpp:45-72`): von 17 `AllStrategies` sind nach Inc1-3 **nur noch
5 unseziert** — die paper-gebundenen Radix-Tries:
- `OriginalArtSearchAlgo` (S04, P01 ART, Leis ICDE 2013, 4/4 original)
- `OriginalHotSearchAlgo` (S05, P02 HOT, Binna PVLDB 2018)
- `OriginalStartSearchAlgo` (S06, P05 START, Mertens ICDE 2024)
- `OriginalWormholeSearchAlgo` (S07, P07 Wormhole, Wu/Ni/Jiang ATC 2019)
- `OriginalSurfSearchAlgo` (S08, P10 SuRF, Zhang/Lim/Andersen SIGMOD 2018)

**Warum eigene Charge (NICHT wie die 3 CE-nativen):** (a) paper-gebunden — `is_original`-Linking + Web-
Recherche-Pflicht je Algorithmus (`[[feedback_web_research_per_algorithm_pflicht]]`); (b) Multi-Achsen-
Zerlegung statt EINER Pool-Organ-Familie — ART/HOT/etc. zerfallen gemäß §13-Anatomie in node_type
(axis_04), path_compression (axis_02), traversal-Organ (axis_03a) u.a., die teils als EIGENE Achsen
existieren; (c) die Gattungs-Konfiguratoren (`ArtComposition`, `SearchAlgorithmAnatomy<ArtComposition>`)
existieren bereits (Doku 14 §11.2/§14.2 R3.2) — vor der Sezierung Ist-State verifizieren
(`[[feedback_verify_ist_state_before_gross_tasks]]`): WAS an Organ-Zerlegung existiert schon, was fehlt.

**Nächster Schritt (Planrunde):** eigene Understand+Design-Runde für die OriginalXxx-Sezierung — pro
Tier die vorhandene Composition/Anatomie + die fehlenden Traversal-Organe kartografieren, dann je Tier
ein build-grüner Sezier-Increment mit Rekonstruktions-Beleg. **Erst danach #42** (Entfernung aller Tiere
aus `EnabledStrategies` → Gattungs-Konfiguratoren), gemäß Option 3 (erst ALLE sezieren, dann umstufen).
Erklärte Ordnung: auch OriginalXxx müssen seziert werden — KEINES bleibt monolithisch.
