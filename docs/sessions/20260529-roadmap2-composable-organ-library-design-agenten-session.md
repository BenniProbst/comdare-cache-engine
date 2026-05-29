# Session — Roadmap-2 (Komponierbare Organ-Bibliothek erweitern): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow · **Task:** #37 (User-Roadmap Schritt 2, Pflicht)
**Workflow:** `wc38kta4o` / Run `wf_6e998f70-415` — 7 Agenten, ~604k Subagent-Tokens, 137 Tool-Uses.
**Zweck:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 14 §1-§3/§11.3 (Organ-Metapher), Doku 24 §6 (entblockt Tier-Wrapper-Umstufung #40). Vorgänger: Roadmap-1 (ce `d26c5de`).

> **Frage:** Wie nehmen wir Hash/BST-Baum/B-Baum als komponierbare Organe ins composable-Modell auf —
> additiv, build-grün, ohne das flache StorageOrgan zu brechen?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)

### 1.1 Tier-Wrapper-Struktur (was zu zerlegen ist)
- **BST** (`bst.hpp:163-193`): `Node{key,val,left,right}` (uint32-Kind-Indizes) + `vector<Node> nodes_` + Free-List + `root_`. uint16-Key. Hibbard-Deletion.
- **B-Baum** (`btree.hpp:164-170`): `alignas(64) Node{n,leaf,key[7],val[7],child[8]}` + nodes_ + Free-List. t=4 (CLRS).
- **Hash** (`hash_search.hpp:167-207`): `Slot{key,val,state}` + `buckets_` + mask + tombstones + Rehash (Fibonacci-Hash, load 0.7).
- **SkipList** (`skip_list.hpp:185-221`): `Node{key,val,live,next[]}` (variable Forward-Indizes) + RNG.
- Array256 (flat std::array, uint8), VectorU8U8 (sortierte Parallel-Arrays) — flach, bereits durch SortedBinary abgedeckt.

### 1.2 Composable-Modell-Grenzen
Das flache `StorageOrgan` (`storage_organ_concept.hpp:40-48`) bietet pro Slot nur `key_at`/`value_at` (2 uint64) + `insert_slot_at`/`erase_slot_at` (Shift → **Index-Instabilität**). → **BST/B-Baum strukturell unmöglich** auf dem flachen Store (brauchen Kind-Index-Felder + index-stabile Knoten). Hash braucht Bucket-Array + Rehash. **Sofort auf dem flachen (sortierten) Store gehen nur weitere lookup-Strategien** (Interpolation, Galloping — insert/erase = SortedBinary-Invariante).

### 1.3 Ziel + Tests
`verify_matches_std_map<Wrapper>` ist `template`, `K = Wrapper::key_type` → für `ComposedSearch<...>` ist K=uint64; **verbatim wiederverwendbar** (breite Keys via key_mod=100000 bereits bewiesen). Organ-Swappability = mehrere Traversal-Organe über demselben Store, alle std::map-äquivalent.

---

## 2. Design-Phase (3 Linsen) + Korrekturen

| Linse | Kern | Verdikt |
|-------|------|---------|
| A — flach-Store-Traversals (Interpolation/Galloping) | sofort, kein Storage-Umbau | **GEWÄHLT (INC-2a)** |
| B — Baum/Hash auf flachem Store | — | **widerlegt** (Feld-Arität + Index-Instabilität) |
| C — Traversal trägt Struktur, Pool-Concept für Baum | sauberes index-stabiles Pool-Concept | **GEWÄHLT (INC-2b)** |

**Korrekturen:** (1) kein neuer Harness nötig (template, K=uint64). (2) Interpolation muss **uint64-generalisiert** werden (Original castet zu `unsigned` → Trunkierung + Produkt-Überlauf; Lösung: `unsigned __int128`-Zwischenprodukt ODER Fallback auf `lower_bound_index`). (3) BST/B-Baum-auf-flach-Store widerlegt → separates `TreeNodePool`-Concept (KEINE StorageOrgan-Erweiterung).

---

## 3. Gewählter Blueprint (INC-2a sofort + INC-2b additiv-parallel)

### INC-2a — auf dem unveränderten flachen StorageOrgan (Pflicht-Minimum: 2 Organe + std::map-Äquiv)
- **`InterpolationTraversalOrgan`** (`composable/interpolation_traversal_organ.hpp`): erfüllt `TraversalOrgan`; insert/erase = SortedBinary-Invariante; `lookup_in` = Interpolationssuche (uint64-generalisiert, Überlauf-Guard/Fallback). Storage: nur `StorageOrgan`.
- **`GallopingTraversalOrgan`** (`composable/galloping_traversal_organ.hpp`): exponentielle Sprünge + binäre Verfeinerung; insert/erase = SortedBinary-Invariante. Storage: nur `StorageOrgan`.

### INC-2b — neues eigenständiges Pool-Concept (KEINE StorageOrgan-Erweiterung)
- **`TreeNodePool<S>` Concept** (`composable/tree_node_pool_concept.hpp`): index-**stabile** API (kNil; allocate_node/free_node; node_key/value/left/right/root/node_count; set_*; clear), uint64-Key, kein noexcept-Zwang.
- **`TreeNodePoolStore`** (`composable/tree_node_pool_store.hpp`): vector<Node>+Free-List+root_ (1:1 nach bst.hpp, ohne Such-Logik) — reines Substrat. static_assert(TreeNodePool<…>).
- **`BSTTraversalOrgan`** (`composable/tree_traversal_organ.hpp`): `TreeTraversalOrgan<T,Pool>`-Concept; statische insert_into/lookup_in/erase_from über Pool-Indizes; Hibbard-Deletion. (B-Baum-Organ optional.)
- **`ComposedTreeSearch<Tr,Pool>`** (`composable/composed_tree_search.hpp`): std::map-Schnittstelle wie ComposedSearch; lässt ComposedSearch unberührt.

**Unangetastet:** composable_search.hpp, storage_organ_concept.hpp, observable_composed_search.hpp, slot_store, composed_store — alle static_asserts bleiben gültig. **Keine CMake-Änderung** (Test-Include-Root `libs/cache_engine`).

---

## 4. Build-grüne Reihenfolge
INC-2a Header (je static_assert) → INC-2a Tests (RawSlotStore + Node4 + ComposedStore, verify_matches_std_map) → **Build+ctest grün** → INC-2b Header (Pool-Concept → Pool-Store → BSTTraversalOrgan → ComposedTreeSearch, je static_assert) → INC-2b Tests (BST über Pool: std::map + degenerierter sortierter Input + Hibbard-3-Fälle) → **Build+ctest grün**. Rein additiv.

## 5. Entblockt #40 (Tier-Wrapper-Umstufung)
Nach diesem Increment existiert das Organ-Pendant pro Such-METHODE: VectorU16U16/InterpolationSearchAlgo → `ComposedSearch<{Sorted|Interpolation}TraversalOrgan>`; BST/B-Baum → `ComposedTreeSearch<BSTTraversalOrgan, TreeNodePoolStore>`. Erst danach ist #40 (BST/B-Baum) durchführbar. Liefert nur die Voraussetzung, führt die EnabledStrategies-Umstufung NICHT aus.

## 6. Scope-Grenze (Folge-Increments)
Hash als O(1)-Bucket-Organ (eigene `HashBucketPool`+`ProbeTraversalOrgan`-Familie); SkipList; Eytzinger/k-ary; voller CLRS-B-Baum-Delete falls zu groß; observable_composed_tree_search (Säule-2-Durchgriff für Pool-Organe); abi_adapter/Tier-Wall-Clock (#38); tatsächliche EnabledStrategies-Umstufung (#40). KEINE StorageOrgan/TraversalOrgan-Änderung, KEINE Infrastruktur.
