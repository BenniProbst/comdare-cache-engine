# Entwickler-Dokumentation — `function-handle-hops` der comdare-cache-engine

> **Aufgabe AP-CE1 — zentrales Entwickler-Dokument.** Dieses Dokument ist die in der
> Diplomarbeit (Kap. **2.3 / 2.3.1**, Abschnitt `sec:stdmap-interface` / `ssec:dynamics-levels`)
> und in **Anhang F** (`anhang/de/F_comparison_interfaces.tex`, Label `app:interfaces`) angekündigte
> „eigene Entwickler-Dokumentation der Cache-Engine", die die **exakten internen Funktions-Sprünge
> (`function-handle-hops`)** hinter jeder `std::map`- / `std::vector`-Interface-Funktion erklärt.
>
> **Onboarding-Einstieg.** Wer neu an der Engine arbeitet, liest dieses Dokument zusammen mit
> `docs/architecture/35_function_handle_hops.md` (kompaktere Schwester-Notiz) und
> `docs/architecture/36_eine_architektur_lebewesen_ist_searchalgorithm.md` (Lebewesen-Identität).
>
> **Quelle / Stand.** Erstellt durch **read-only Code-Sichtung** (kein Schätzen). Alle
> `Datei:Zeile`-Belege sind ein Snapshot des Stands vom Erstellungstag; **bei Abweichung gilt der
> Code**. Wo der Code eine `std::map`-/`std::vector`-Funktion NICHT direkt stützt, ist das in §6
> **ehrlich als „nicht verdrahtet" markiert** (nicht erfunden).

---

## 0. Definition: was ist ein „function-handle-hop"?

Ein **function-handle-hop** ist ein einzelner Sprung der Aufrufkette, der einen äußeren
**Prüf-Dock-/ABI-Call** (z. B. `tier_insert`) auf **genau ein Achsen-Organ-Handle** des Lebewesens
weiterleitet (z. B. `container_.insert(...)`, `pc_organ_.compress(...)`,
`telemetry_organ_.record_node_touch(...)`). Die Folge dieser Hops — vom ABI-Eintritt über die
Achsen-Handles bis zum Ergebnis — ist der **Hop-Pfad** einer Funktion.

**Lebewesen-/Anatomie-Metapher (Kap. 2.3, Doc 36).** Ein *Lebewesen* **ist** ein
`SearchAlgorithm` (es gibt keine zweite achsentragende Engine-Hierarchie — der frühere
Drei-Schichten-Parallelbaum `search_engine`/`execution_engine` wurde entfernt,
`abi_adapter.hpp:100-107`). Sein *Körper* ist die `SearchAlgorithmAnatomy<Composition>` mit
**19 Organen** (Achsen T0–T18). Eine **Interface-Funktion ist eine Organ-Kette**: der ABI-Call
betritt den Körper und „durchwandert" in fester Reihenfolge die zuständigen Organe. Genau diese
Kette dokumentiert dieses Papier.

**ABI-Grenze.** Die Modul-Grenze (`.dll`/`.so`) reicht **ausschließlich `uint64`-Primitiven**
(key/value/index) plus einen **POD-Observer** durch — das ist die binär-stabile Schicht. Reiche
`std::map`/`std::vector`-Semantik (Iteratoren, `operator[]`, Bereichsabfragen) wird hier **bewusst
nicht** durchgereicht; siehe §6 und Anhang F.

---

## 1. ABI-Eintrittspunkte (drei Gattungs-Adapter)

Anhang F bildet die variadische Hülle ab: **1 Parameter ⇒ `std::vector`-API**, **2 ⇒ `std::map`-API**,
**N>2 ⇒ `std::map<K,std::tuple<…>>`**. Im Code entsprechen dem drei Gattungs-Adapter:

| Gattung (Tier-Metapher) | Adapter | Datei | Öffentliche Prüf-Dock-/Tier-API |
|---|---|---|---|
| **SearchAlgorithm** (map-artig, Mammal) | `SearchAlgorithmAbiAdapter<A>` | `libs/cache_engine/anatomy/abi_adapter.hpp:127` | `tier_insert`/`tier_lookup`/`tier_erase`/`tier_clear`/`tier_size` + `tier_scan` + `tier_migrate_step` + `tier_observe` |
| **Set** (set-artig, Vogel) | `SetAbiAdapter<A>` | `libs/cache_engine/anatomy/set_abi_adapter.hpp:18` | `tier_set_insert`/`tier_set_contains`/`tier_set_erase`/`tier_set_size`/`tier_set_clear` + `tier_observe_set` |
| **Sequence** (vector-artig) | `SequenceAbiAdapter<A>` | `libs/cache_engine/anatomy/sequence_abi_adapter.hpp:1` | `tier_push_back`/`tier_at`/`tier_size`/`tier_clear` + `tier_observe_sequence` |

Jedes Permutations-Binary exportiert **genau einen** Adapter via `extern "C" comdare_create_anatomy()`
(`abi_adapter.hpp:114-121`); der gattungs-agnostische Loader fragt das passende Dock per
`dynamic_cast` (`set_abi_adapter.hpp:4-5`).

Der `static_assert` im Klassenrumpf erzwingt die Gattung zur Compile-Zeit
(`abi_adapter.hpp:144-147`, „Cross-Genus-Adapter sind type-system-mathematisch unmöglich").

---

## 2. Die 19 Organe (Thesis-T-Nummer ↔ Member-Handle ↔ Code-`axis_NN`)

Die Organe sind **Member** des Adapters (`abi_adapter.hpp:1663-1716`). Achtung: Thesis nummeriert
T0–T18, der Code-`axis_NN` weicht historisch ab (Brücke aus Doc 35 §0).

| T# | Achse | Member-Handle (`abi_adapter.hpp`) | Code-`axis_NN` (Repräsentant) |
|----|-------|-----------------------------------|-------------------------------|
| T0  | search_algo        | `search_organ_` (Weg B) / `container_` (Weg A) — `:1663` / `:1664` | `axis_03a` (`axes/lookup/composable/`) |
| T1  | cache_traversal    | `ct_organ_` `:1690` | `axis_03b` |
| T2  | mapping            | `map_organ_` `:1691` | `axis_03m` |
| T3  | path_compression   | `pc_organ_` `:1712` | `axis_02` |
| T4  | node_type          | `node_organ_` `:1679` (+ `container_`-Store) | `axis_04` |
| T5  | memory_layout      | `ml_organ_` `:1677` | `axis_05` |
| T6  | allocator          | `container_` (Store-Allokator) `:1664` | `axis_06` |
| T7  | prefetch           | `pf_organ_` `:1697` | `axis_07` |
| T8  | concurrency        | `cc_organ_` `:1698` | `axis_08` |
| T9  | serialization      | `ser_organ_` `:1678` | `axis_10` |
| T10 | telemetry          | `telemetry_organ_` `:1674` | `axis_11` |
| T11 | value_handle       | `vh_organ_` `:1705` | `axis_14` |
| T12 | isa                | `isa_organ_` `:1706` | `axis_09` |
| T13 | index_organization | `idx_organ_` `:1713` | `axis_01` |
| T14 | io_dispatch        | `io_organ_` `:1714` | `axis_io_dispatch` |
| T15 | migration_policy   | `mig_organ_` `:1715` | `axis_migration` |
| T16 | filter             | `flt_organ_` `:1716` | `axis_filter` |
| T17 | queuing_q1         | `queuing_q1_organ_` `:1685` | `axis_q1` |
| T18 | queuing_q2         | `queuing_q2_organ_` `:1686` | `axis_q2` |

> Merke (Doc 35): Code-`axis_14` = **value_handle** (T11), **nicht** io_dispatch. Code-`axis_09` =
> **isa** (T12), **nicht** prefetch.

---

## 3. Compile-Time- vs. Runtime-Hops (Kern für Kap. 2.3)

Kap. 2.3 unterscheidet **zwei Dynamik-Ebenen** (`ssec:dynamics-levels`):

- **Compile-Time-Hop (`[CT]`) = statische Achse = eigene Binary.** Welches Organ getroffen wird,
  steht zur **Übersetzungszeit** fest (Template-Parameter der `Composition`, `if constexpr`,
  Concept-Check). Jede Achsen-Wahl erzeugt eine **eigene** `.dll`/`.so` (ein Permutations-Binary).
  Es gibt **keinen Laufzeit-Switch** über Strategien.
- **Runtime-Hop (`[RT]`) = dynamische Sub-Achse / Laufzeit-Zustand = Laufzeit-Schleife über
  *derselben* Binary.** Innerhalb des fixierten Binaries variiert nur **Laufzeit-Zustand**: die
  Daten (`container_`-Slots), die zur Laufzeit angewandte Ressourcen-Steuerung
  (`tier_apply_resource_control`, `abi_adapter.hpp:196`, 5 dynamische Sub-Achsen
  concurrency/prefetch/allocator/traversal/value_handle, `:186-195`) und der CoW-Memento-Zustand.

### 3.1 Wo `resolve_baustein` „Prüfling vs. Standard" entscheidet — `[CT]`

**Datei:** `libs/cache_engine/include/cache_engine/abi/resolve_baustein.hpp`

Pro Achsen-Tag wählt `resolve_baustein<Algo, Tag>` zur **Übersetzungszeit** den Baustein
(`resolve_baustein.hpp:66-83`):

```
1. if constexpr (has_member_baustein<Algo, Tag>)   ->  Algo::template baustein_t<Tag>     (PRÜFLING — Vorrang)   resolve_baustein.hpp:71-72
2. else                                            ->  cache_engine::baustein_t<Tag>::type (CE-STANDARD-Fallback) resolve_baustein.hpp:73-74
3. (Default der Fallback-Quelle)                   ->  void  ("kein Default" → Prüfling muss liefern)             resolve_baustein.hpp:31-33
```

- Das Concept `has_member_baustein<Algo, Tag>` (`resolve_baustein.hpp:22-25`) prüft, ob der Prüfling
  selbst `Algo::baustein_t<Tag>` trägt.
- Die Auswahl ist ein `if constexpr` in `choose()` (`resolve_baustein.hpp:69-79`), abgeschlossen
  über `decltype` → `using type` (`:79`). **Reiner Compile-Time-Tag-Dispatch**, kein
  `std::conditional_t`-Laufzeitkostenanteil, kein vtable.
- **11 Achsen-Tags** (`resolve_baustein.hpp:38-48`): `PageAxisTag, NodeAxisTag, TraversalAxisTag,
  ValueHandleAxisTag, ConcurrencyAxisTag, AllocatorAxisTag, PrefetchAxisTag, TelemetryAxisTag,
  IsaAxisTag, LayoutAxisTag, ReclamationAxisTag` mit konkreten Specializations
  (`resolve_baustein.hpp:51-61`).

> Dies ist der **Prüfling-gegen-Standard-Mechanismus**: liefert der Prüfling eine Achse selbst,
> gewinnt sie; sonst greift der Cache-Engine-Standard. Die Entscheidung fällt **vollständig zur
> Compile-Zeit** und materialisiert sich in der jeweiligen Binary.

### 3.2 Weitere Compile-Time-Weichen, die den Hop-Pfad bestimmen

| Concept / Weiche | Datei:Zeile | Wirkung |
|---|---|---|
| `StoreTraversableSearchAlgo<SearchAlgo>` | `axes/lookup/composable/store_traversable_search_algo.hpp` (genutzt `abi_adapter.hpp:693`, `:763`, `:813`) | **Weg A** (Suche über den `container_`-Store) vs. **Weg B** (separates `search_organ_`) |
| `container_traversal_t` = `std::conditional_t<…>` | `abi_adapter.hpp:1650-1653` | wählt das Traversal-Organ des Stores: `traversal_for_search_algo_t<SearchAlgo>` (Weg A) **oder** `SortedBinaryTraversal` (Fallback/Weg B) |
| `ObservableAxis<T>` | (Concept; z. B. `abi_adapter.hpp:915`, `:932`) | steuert, ob eine Achse im Observer (`fill_observer_v3`) aggregiert wird |
| `MementoAxis<T>` | (Concept; `abi_adapter.hpp:1501`, `:1538`) | save/restore-Fähigkeit (Warmlauf-Zustand, CoW, `tier_scan`) |
| `if constexpr (requires { … })` je Auto-Kopplung | je Aufrufstelle (`abi_adapter.hpp:709`, `:713`, `:717`, `:721`, `:724`, `:745`, `:750`, `:755`) | optionale Achsen-Kopplung: nackte vs. observable Strategie |

Alles `[CT]`. Der einzige **`[RT]`**-Anteil im Hot-Path ist (a) der Datenzugriff auf
`container_`/`search_organ_` (Slot-Inhalt zur Laufzeit), (b) der CoW-Materialisierungs-Check
`if (cow_armed_)` (`abi_adapter.hpp:684`) und (c) `descent_slot_for_(key)` (`abi_adapter.hpp:1630`),
das den real berührten Slot per Lower-Bound-Suche über die Laufzeit-Keys bestimmt.

---

## 4. Hop-Pfade pro Interface-Funktion (SearchAlgorithm / `std::map`-artig)

Alle Pfade ab `libs/cache_engine/anatomy/abi_adapter.hpp`. `[CT]` = Compile-Time-Entscheidung,
`[RT]` = Laufzeit-Zustand. **Wichtig:** die Observer-feeding Auto-Kopplungen (T1/T2/T3/T7/T8/T10/T17/T18
sowie die T11/T16/value_handle-Build-Pfade) stehen unter `#if COMDARE_MEASUREMENT_ON`
(`abi_adapter.hpp:701-756`, `:772-804`) — in der **funktional-only DLL (Messung-AUS)** existiert nur
der reine Daten-Pfad (T0 + Storage-Achsen über `container_`).

### 4.1 `insert(key,value) → bool`  ·  `tier_insert`  ·  `abi_adapter.hpp:678`

```
tier_insert(uint64 key, uint64 value)                                     abi_adapter.hpp:678
 ├─ [RT] CoW-Memento:  if (cow_armed_) cow_materialize_copy_()            :684   (#if MEASUREMENT_ON)
 ├─ [CT] StoreTraversableSearchAlgo<SearchAlgo> ?                          :693
 │    ├─ Weg A:  m8_new_flag = container_.insert(key,value)                :694   [T0+T4+T5+T6]
 │    │            → ObservableComposedSearch::insert
 │    │              → ComposedSearch::insert                              composable_search.hpp:125
 │    │                → Traversal::insert_into<Store>(store_,k,v)         composable_search.hpp:95
 │    │                  (SortedBinary: lower_bound_index + insert_slot_at)composable_search.hpp:96-98
 │    │                    → LayoutAwareChunkedStore::insert_slot_at       [T4 node / T5 layout / T6 alloc REAL]
 │    └─ Weg B:  search_organ_.insert(key,value)                          :697   [T0]
 │               + container_.insert(key,value)                           :698   [treibt T4/T5/T6]
 │               m8_new_flag via occupied_count()-Delta                    :696,:699
 └─ Auto-Kopplung (#if MEASUREMENT_ON, je `if constexpr (requires …)`):   :701-756
      [T10] telemetry_organ_.record_node_touch(true)                      :709
      [T1]  ct_organ_.register_entry(key,value)                           :713-716
      [T2]  map_organ_.register_slot(key,value)                           :717-720
      [T17] queuing_q1_organ_.put(value)                                  :721-723
      [T18] queuing_q2_organ_.should_flush(…)+on_flush_complete()         :724-727
      [T8]  cc_organ_.observe_critical_section()                          :732
      [T7]  pf_organ_.observe_prefetch_descent(container_.store(),
                                               descent_slot_for_(key))    :737   ([RT] descent_slot)
      [T3]  pc_organ_.compress(key, 0u)                                   :741
      [T16] flt_organ_.insert_key(key)            (Build-Phase)           :745
      [T11] vh_organ_.store_value(key,value)      (Build-Phase, nicht-inline) :750
      [T3]  pc_organ_.insert_key(key)             (Patricia-Trie-Build)   :755
 └─ return m8_new_flag                                                     :757
```

Anhang-F-Abdeckung: `insert` direkt; `insert_or_assign` / `emplace` / `try_emplace` /
`emplace_hint` / `insert_range` reduzieren laut Anhang F (`tab:if-decomp`) auf „Ordnungs-Suche +
Knoten verlinken" — im Code **dieselbe** `insert_into`-Kette (mit Update-Zweig bei vorhandenem
Schlüssel, `composable_search.hpp:97`). Die ABI exponiert sie **nicht** einzeln (§6).

### 4.2 `find(key)` / `lookup(key,out) → bool`  ·  `tier_lookup`  ·  `abi_adapter.hpp:760`

```
tier_lookup(uint64 key, uint64* out_value) const                          abi_adapter.hpp:760
 ├─ [CT] StoreTraversableSearchAlgo<SearchAlgo> ?                          :763
 │    ├─ Weg A:  container_.lookup(key)                                    :764   [T0 über Store]
 │    │            → ComposedSearch::lookup                                composable_search.hpp:126
 │    │              → Traversal::lookup_in<Store>                         composable_search.hpp:101
 │    │                (SortedBinary: lower_bound_index + key_at==key)     composable_search.hpp:102-104
 │    └─ Weg B:  search_organ_.lookup(key)                                 :768   [T0]
 ├─ std::optional<value> → (*out_value) bei Treffer                        :766 / :770
 └─ Auto-Kopplung (#if MEASUREMENT_ON):                                    :772-804
      [T10] telemetry_organ_.record_node_touch(true)                      :775
      [T1]  ct_organ_.resolve(key)                                        :778-780
      [T2]  map_organ_.resolve_offset(key)                                :781-783
      [T17] queuing_q1_organ_.get()                                       :784-786
      [T8]  cc_organ_.observe_critical_section()                          :788
      [T7]  pf_organ_.observe_prefetch_descent(container_.store(),
                                               descent_slot_for_(key))    :794 / :798  ([RT])
      [T3]  pc_organ_.compress(key, 0u)                                   :803
 └─ return m8_hit                                                          :805
```

Anhang-F-Abdeckung: `find` ≙ `lookup`+`optional`. `count` / `contains` reduzieren laut Anhang F auf
`find` — im Code dieselbe `lookup`-Kette (bool ⇒ Membership; bei Unikat-Map 0/1). `at(key)` =
`find` + Wurf `std::out_of_range` bei Fehlen: **auf ABI-Ebene nicht realisiert** (§6) — die ABI
liefert `bool`+`out`, keine Ausnahme. `operator[]` (map) = `lower_bound` + Default-Insert: **nicht
als eigener ABI-Call** (§6); der Lower-Bound-Teil existiert intern in
`SortedBinaryTraversal::lower_bound_index` (`composable_search.hpp:89-93`), aber als Insert-Primitiv,
nicht als `operator[]`-Hop.

### 4.3 `erase(key) → bool`  ·  `tier_erase`  ·  `abi_adapter.hpp:808`

```
tier_erase(uint64 key)                                                    abi_adapter.hpp:808
 ├─ [RT] CoW-Memento: if (cow_armed_) cow_materialize_copy_()             :810  (#if MEASUREMENT_ON)
 └─ [CT] StoreTraversableSearchAlgo<SearchAlgo> ?                          :813
      ├─ Weg A:  return container_.erase(key)                             :814
      │            → ComposedSearch::erase → Traversal::erase_from<Store>  composable_search.hpp:127 / :107
      │              (SortedBinary: lower_bound_index + erase_slot_at)     composable_search.hpp:108-109  [T6 real]
      └─ Weg B:  search_organ_.erase(key) + container_.erase(key)         :817-818
                 Ergebnis via occupied_count()-Delta                       :816,:819
```

Anhang-F-Abdeckung: `erase(key)` direkt (0/1 entfernt). `erase(iterator/range)`, `extract`, `merge`,
`swap` — **nicht** auf ABI (keine Iteratoren/Node-Handles über die Grenze; §6).

### 4.4 `clear()`  ·  `tier_clear`  ·  `abi_adapter.hpp:823`

```
tier_clear()                                                              abi_adapter.hpp:823
 ├─ [RT] CoW-Memento: if (cow_armed_) cow_materialize_copy_()             :827  (#if MEASUREMENT_ON)
 ├─ search_organ_.clear() + container_.clear()                            :829   → ComposedSearch::clear → store_.clear()  composable_search.hpp:129
 ├─ container_tier1_.clear()  (kalte 2. Migrations-Ebene)                 :832
 └─ Statistik-/Daten-Reset aller Organe (#if MEASUREMENT_ON, je if constexpr): :833-887
      [T16] flt_organ_.clear_filter()  :842 · [T11] vh_organ_.clear_slots() :846 ·
      [T3]  pc_organ_.clear_trie()     :850 · [T0] search_organ_.reset()    :856 ·
      [T1]  ct_organ_.clear()/reset()  :869-870 · [T2] map_organ_.clear()/reset() :871-872 ·
      [T17] q1.clear()/reset()         :873-874 · [T18] q2.reset()           :875 ·
      [T10] telemetry_organ_.reset()   :879 · [T7] pf_organ_.reset()         :882 ·
      [T8]  cc_organ_.reset()          :883 · [T3] pc_organ_.reset()         :886
```

Anhang-F-Abdeckung: `clear` direkt (geteilte Funktion map+vector).

### 4.5 `size() → uint64`  ·  `tier_size`  ·  `abi_adapter.hpp:890`

```
tier_size() const  →  static_cast<uint64>(container_.occupied_count())    abi_adapter.hpp:890-892
                       (EIN Speicher: container_ hält jeden Insert, Weg A + Weg B)
```

Anhang-F-Abdeckung: `size` direkt. `empty()` ist **nicht als eigener ABI-Call** vorhanden, aber
trivial aus `tier_size()==0` ableitbar (§6 — Backlog: dünner Wrapper). `max_size` — **nicht**
verdrahtet (theoretischer Wert, kein Organ-Hop).

### 4.6 Range-Scan  ·  `tier_scan`  ·  `abi_adapter.hpp:1519`  (`IScannableTier`, YCSB-E)

```
tier_scan(uint64 start_key, uint64 max_count, uint64* out_checksum) const  abi_adapter.hpp:1519
 ├─ [CT] StoreTraversableSearchAlgo<SearchAlgo> ?                           :1528
 │    ├─ Weg A:  snapshot = container_.save_state().data                    :1529   (key,value)-Liste
 │    │          std::sort(snapshot nach key)                               :1530-1531
 │    │          akkumuliere ab key>=start_key bis max_count                :1532-1537
 │    └─ [CT] else if MementoAxis<SearchAlgo>:                              :1538
 │            snapshot = search_organ_.save_state(); sort; akkumuliere      :1539-1547
 │    └─ else: ehrlich No-Op (nicht scanbar — z. B. Hash ohne Memento)      :1549
 └─ *out_checksum += sum; return visited                                    :1553-1554
```

**Ehrlicher Hinweis (geordnetes Iterieren / `lower_bound`/`upper_bound`).** `tier_scan` liefert die
in Anhang F geforderte **geordnete Bereichs-Iteration** (das Substrat hinter `begin/end`,
`lower_bound`, `upper_bound`, `equal_range`), **aber** über `save_state()` + `std::sort` —
**nicht** über einen nativen, ordnungserhaltenden Traversal-Cursor. Ein echtes
`SortedBinaryTraversal::lower_bound_index` existiert (`composable_search.hpp:89-93`), wird hier
aber **nicht** aufgerufen; der Kommentar `abi_adapter.hpp:1527` markiert die O(scan\_len)-Variante
ausdrücklich als **Refinement-Backlog**. Es gibt **keinen** `tier_upper_bound`/`tier_lower_bound`
auf der ABI. Wer geordnete Cursor-Semantik braucht, ergänzt sie hier (§6).

### 4.7 2-Ebenen-Migration  ·  `tier_migrate_step`  ·  `abi_adapter.hpp:1594`  (`IMigratableTier`)

```
tier_migrate_step(uint64 max_moves)                                        abi_adapter.hpp:1594
 ├─ [RT] CoW-Memento: if (cow_armed_) cow_materialize_copy_()              :1600
 └─ [CT] requires container_.store_migrate_step(mig_organ_, …) ?           :1603
      [T15] mig_organ_.reset()                                            :1604
      moved = container_.store_migrate_step(mig_organ_,
                                            container_tier1_.store_mut(),
                                            max_moves)                     :1605   (heiß→kalt)
      [T15] mig_organ_.add_tier_moves(moved)                              :1606
 └─ return moved                                                           :1611
```

Kein direktes `std::map`/`std::vector`-Pendant (engine-spezifisch); hier zur Vollständigkeit der
Tier-API dokumentiert. `tier_save_all`/`tier_rollback_all` (`abi_adapter.hpp:1369`/`:1405`,
memento\_all) sind der Warmlauf-Zwei-Phasen-Vertrag (CoW, #133) — Infrastruktur, kein
Interface-Funktions-Hop.

### 4.8 Observer  ·  `tier_observe`  ·  `abi_adapter.hpp:1312`  (alle 19 Achsen, Pfad B)

```
tier_observe(ComdareTierObserverSnapshot* out) const                       abi_adapter.hpp:1312
 ├─ SCHRITT 1: fill_observer_v3(out)   (axis_stats[19][…] je Achse)         :1317 → :907
 │    Instanz-Organe (.statistics()):  T0 :915 · T1 :932 · T2 :940 · …
 │    Pfad-B-Zustand-Scan (reset()+container_.store_observe_<axis>()):
 │       T4/T5/T9/T11/T12/T13/T14/T15/T16  (idempotenter const-Scan)
 ├─ SCHRITT 2: fill_segment_timing_v3(&seg)  (per-Achsen-ns T0..T18)        :1320 → :1136
 └─ seg_ns/Meta in den EINEN POD übertragen                                 :1321-1325
```

`tier_observe` ist **kein** `std::map`-Interface-Hop, sondern die **Mess-Sicht** (Kap. 2.3 /
Doc 31): er erhebt je Achse einen POD-Observer in fester Schema-Reihenfolge `kV3AxisSchema` (0–18).
Hier dokumentiert, weil er die **gleiche Organ-Kette** wie der Hot-Path nutzt und das Bindeglied zur
Messung ist.

---

## 5. Hop-Pfade: Set-Gattung (`std::set`-artig) und Sequence-Gattung (`std::vector`-artig)

### 5.1 Set (`set_abi_adapter.hpp`)

```
tier_set_insert(key)    → anatomy_.insert(key)        set_abi_adapter.hpp:40-41
tier_set_contains(key)  → anatomy_.contains(key)      set_abi_adapter.hpp:43-44
tier_set_erase(key)     → anatomy_.erase(key)         set_abi_adapter.hpp:46-47
tier_set_size()         → anatomy_.size()             set_abi_adapter.hpp:49-50
tier_set_clear()        → anatomy_.clear()            set_abi_adapter.hpp:52
tier_observe_set(out)   → anatomy_.observe_all()      set_abi_adapter.hpp:56
```

Default-Kern `SortedArrayKeySet` (`set_default_organ.hpp:18`): **hier** liegt eine **echte**
`std::lower_bound`-Suche zugrunde — `insert` (`:22`), `lookup` (`:26`), `erase` (`:30`) nutzen
`std::lower_bound` über den sortierten `keys_`-Vektor. Das ist das Set-Pendant zur map-`find`-Kette.

### 5.2 Sequence (`sequence_abi_adapter.hpp`)

```
tier_push_back(value)        → anatomy_.push_back(value)   sequence_abi_adapter.hpp:35-36   (Anhang F: push_back/emplace_back/append_range)
tier_at(index,out) → bool    → anatomy_.at(index)          sequence_abi_adapter.hpp:38-40   (bounds-checked → std::optional; Anhang F: at/operator[])
tier_size()                  → anatomy_.size()             sequence_abi_adapter.hpp:47
tier_clear()                 → anatomy_.clear()            sequence_abi_adapter.hpp:49
tier_observe_sequence(out)   → anatomy_.observe_all()      sequence_abi_adapter.hpp:53
```

Anhang-F-Abdeckung Sequence: `push_back`/`at`/`size`/`clear` direkt; `operator[]` ≙ `tier_at`
(geprüft statt UB); `emplace_back` ≙ `push_back`; `data`/`resize`/`reserve`/`capacity`/`front`/`back`/
`pop_back`/positions-`insert`/`erase`/Iteratoren — **nicht** auf ABI (§6).

---

## 6. NICHT auf ABI verdrahtete `std::map`/`std::vector`-Funktionen (ehrlicher Abgleich gegen Anhang F)

Die ABI reicht nur `uint64`-Primitiven + POD-Observer durch. Folgende Anhang-F-Funktionen sind
**bewusst nicht** als eigener ABI-Hop realisiert (teils intern vorhanden, aber nicht exponiert) —
das ist der natürliche **Erweiterungs-Backlog**. Wer eine ergänzt, trägt ihren Hop-Pfad hier und in
§4/§5 nach.

**`std::map` (Anhang F, `tab:if-map`):**

| Anhang-F-Funktion | Status auf ABI | Bemerkung / nächster Hop |
|---|---|---|
| `find` $\dagger$ | ✅ ≙ `tier_lookup` + `optional` | §4.2 |
| `insert` $\dagger$ | ✅ ≙ `tier_insert` | §4.1 |
| `insert_or_assign` $\dagger$ | ⚠️ über `tier_insert` (Update-Zweig `composable_search.hpp:97`); kein eigener Call | Decomp Anhang F `tab:if-decomp:217` |
| `emplace` $\dagger$ / `emplace_hint` / `try_emplace` $\dagger$ | ⚠️ semantisch über `tier_insert`; kein in-place/Hint über die ABI | uint64-Primitiven ⇒ kein echtes emplace |
| `insert_range` | ❌ nicht verdrahtet | Bereich ⇒ Schleife über `tier_insert` |
| `count` $\dagger$ / `contains` $\dagger$ | ⚠️ über `tier_lookup` (bool); kein eigener Call | Decomp `tab:if-decomp:218` |
| `at` $\dagger$ | ❌ nicht verdrahtet | wirft-Semantik fehlt; ABI liefert `bool`+`out` statt `std::out_of_range` |
| `operator[]` $\dagger$ | ❌ nicht verdrahtet | = `lower_bound`+Default-Insert; Lower-Bound-Primitiv existiert intern (`composable_search.hpp:89-93`), nicht als ABI-Op |
| `lower_bound` $\dagger$ / `upper_bound` $\dagger$ / `equal_range` $\dagger$ | ❌ **nicht** als ABI-Op | `lower_bound_index` existiert intern (`composable_search.hpp:89`), **`upper_bound` hat KEINE eigene Implementierung**; geordnetes Iterieren nur indirekt via `tier_scan` (§4.6, sort-basiert) |
| `begin`/`end` $\dagger$ / `rbegin`/`rend` / Iteratoren | ❌ nicht verdrahtet | keine Iteratoren über die binäre Grenze; geordnetes Lesen via `tier_scan` |
| `empty` $\dagger$ | ⚠️ ableitbar `tier_size()==0`; kein eigener Call | trivialer Wrapper-Backlog |
| `size` / `clear` $\dagger$ | ✅ `tier_size` / `tier_clear` | §4.5 / §4.4 |
| `erase` $\dagger$ (key) | ✅ `tier_erase`; Iterator-/Range-`erase` ❌ | §4.3 |
| `swap` / `extract` / `merge` | ❌ nicht verdrahtet | Node-Handles/Iteratoren nicht über ABI |
| `max_size` / `key_comp` / `value_comp` / `get_allocator` | ❌ nicht verdrahtet | Beobachter/Allokator-Typen nicht über ABI |
| `operator==` / `operator<=>` / `std::erase_if` / Deduktions-Leitfäden | ❌ nicht verdrahtet | Nicht-Member/CTAD; außerhalb der Tier-ABI |

**`std::vector` (Anhang F, `tab:if-vector`):**

| Anhang-F-Funktion | Status auf ABI | Bemerkung |
|---|---|---|
| `push_back` / `emplace_back` | ✅ ≙ `tier_push_back` | §5.2 |
| `at` | ✅ ≙ `tier_at` (geprüft → `optional`) | §5.2 |
| `operator[]` | ⚠️ ≙ `tier_at` (geprüft statt UB) | bewusst sicherer |
| `size` / `empty` / `clear` | ✅ `tier_size`/(ableitbar)/`tier_clear` | `empty` als Wrapper-Backlog |
| `data` | ❌ nicht verdrahtet | roher Zeiger ungültig über `.dll`-Grenze |
| `resize` / `reserve` / `capacity` / `shrink_to_fit` | ❌ nicht verdrahtet | Kapazitäts-API nicht exponiert |
| `front` / `back` / `pop_back` | ❌ nicht verdrahtet | Backlog (trivial über size/at) |
| positions-`insert` / `emplace` / `erase` / `assign`(`_range`) / `append_range` / `insert_range` | ❌ nicht verdrahtet | Positions-/Bereichs-Semantik nicht über ABI |
| Iteratoren (`begin`/`end`/`rbegin`/`rend`) | ❌ nicht verdrahtet | keine Iteratoren über die Grenze |
| `swap` / `operator==` / `operator<=>` / `std::erase`(`_if`) / CTAD | ❌ nicht verdrahtet | Nicht-Member; außerhalb der Tier-ABI |

> **Zentrale Anhang-F-Beobachtung (`asec:if-decomp`):** Sämtliche Map-Zugriffe/-Modifikatoren ruhen
> auf **einem** Ordnungs-Such-Primitiv (`lower_bound`/`find`). Im Code ist das
> `SortedBinaryTraversal::lower_bound_index` (`composable_search.hpp:89-93`) — eine Achse muss nur
> diese korrekte Ordnungs-Suche + Knoten-Verkettung liefern, der Rest ist feste Komposition. Genau
> diese **Austauschbarkeit** ist der zentrale Mess-Akt der Arbeit (Kap. 2.3).

---

## 7. Übersichtstabelle (Funktion → `tier_*`-Op → Achsen-Kette → CT/RT)

| Interface-Funktion (Anhang F) | `tier_*`-Op (`abi_adapter.hpp:Zeile`) | Achsen-Kette (T#, Aufruf-Reihenfolge) | CT / RT |
|---|---|---|---|
| `map::insert` / `emplace` / `insert_or_assign` | `tier_insert` `:678` | CoW`[RT]` → **T0** (`container_`/`search_organ_`) → T4/T5/T6 (Store) → T10→T1→T2→T17→T18→T8→**T7**`[RT-slot]`→T3→T16→T11→T3 | Weg-Wahl + alle Kopplungen `[CT]`; CoW-Check + Slot `[RT]` |
| `map::find` / `count` / `contains` | `tier_lookup` `:760` | **T0** → T10→T1→T2→T17→T8→**T7**`[RT-slot]`→T3 | `[CT]`; Slot/Daten `[RT]` |
| `map::erase(key)` | `tier_erase` `:808` | CoW`[RT]` → **T0** (+T4/T5/T6 Store) | `[CT]`; CoW `[RT]` |
| `map::clear` / `vector::clear` | `tier_clear` `:823` | CoW`[RT]` → **T0** (+tier1) → Reset T16/T11/T3/T0/T1/T2/T17/T18/T10/T7/T8 | `[CT]`; CoW `[RT]` |
| `map::size` / `vector::size` | `tier_size` `:890` | `container_.occupied_count()` | reiner `[RT]`-Zustand |
| geordnetes Iterieren / `lower_bound`/`upper_bound`/`begin..end`(Range) | `tier_scan` `:1519` | **T0**-Substrat via `save_state()`+`std::sort` (NICHT nativer Lower-Bound-Cursor — §4.6) | `[CT]` Weg-Wahl; Daten `[RT]` |
| 2-Ebenen-Migration (engine-spezifisch) | `tier_migrate_step` `:1594` | CoW`[RT]` → **T15** (`mig_organ_` + Store-Move heiß→kalt) | `[CT]`; Move `[RT]` |
| Mess-Sicht (alle 19 Achsen) | `tier_observe` `:1312` | T0..T18 (Instanz-`.statistics()` + Pfad-B-Store-Scan) | `[CT]` Schema; Erhebung `[RT]` |
| `set::insert`/`contains`/`erase`/`size`/`clear` | `tier_set_*` `set_abi_adapter.hpp:40-52` | `anatomy_.*` → `SortedArrayKeySet` (`std::lower_bound`, `set_default_organ.hpp:22/26/30`) | `[CT]` Gattung; Daten `[RT]` |
| `vector::push_back`/`emplace_back` | `tier_push_back` `sequence_abi_adapter.hpp:35` | `anatomy_.push_back` | `[CT]` Gattung; Daten `[RT]` |
| `vector::at`/`operator[]` | `tier_at` `sequence_abi_adapter.hpp:38` | `anatomy_.at(index)` (geprüft) | `[CT]` Gattung; Daten `[RT]` |

---

## 8. Compile-Time-Hops (statische Achsen = eigene Binary) vs. Runtime-Hops (dynamische Sub-Achsen)

**Compile-Time-Hops (`[CT]`) — eine eigene `.dll`/`.so` je Achsen-Wahl:**

- **Achsen-Auswahl Prüfling vs. Standard:** `resolve_baustein` (`resolve_baustein.hpp:66-83`,
  11 Tags `:38-48`) — `if constexpr (has_member_baustein…)` fixiert je Achse den Typ zur
  Übersetzungszeit. Kein Laufzeit-Switch.
- **Weg-A/Weg-B-Routing:** `StoreTraversableSearchAlgo<SearchAlgo>` als `if constexpr`
  (`abi_adapter.hpp:693/763/813`) und `container_traversal_t = std::conditional_t<…>`
  (`abi_adapter.hpp:1650-1653`).
- **Organ-Typen der Composition:** alle 19 Member sind `typename Composition::<axis>`-Typen
  (`abi_adapter.hpp:1663-1716`) — pro Permutation andere konkrete Typen ⇒ **anderes Binary**.
- **Optionale Auto-Kopplungen:** `if constexpr (requires { … })` an jeder Kopplungsstelle
  (`:709,:713,:717,:721,:724,:745,:750,:755`) — nackte vs. observable Strategie wird wegkompiliert,
  wenn die Strategie die Methode nicht trägt.
- **Mess- vs. funktional-only-Binary:** `#if COMDARE_MEASUREMENT_ON` (`abi_adapter.hpp:133-143`,
  `:701`, `:772`, `:833`) — die Release-DLL erbt nur `IDriveableTier` (reiner Antrieb, kein
  Observer-/Memento-/Scan-vtable-Slot).

**Runtime-Hops (`[RT]`) — Laufzeit-Schleife über *derselben* Binary:**

- **Daten-Zustand:** Inhalt von `container_`/`search_organ_`/`container_tier1_` (Slots), der je
  Operation zur Laufzeit gelesen/mutiert wird.
- **Dynamische Sub-Achsen (5):** `tier_apply_resource_control` (`abi_adapter.hpp:196-210`) klemmt
  zur Laufzeit thread\_count/prefetch\_distance/pool\_budget/batch\_size/inline\_threshold an die
  Caps (`tier_query_resource_caps :186-195`). Das ist die **Laufzeit-Schleife** des
  `RuntimeVariableLoop` **ohne** Reload — genau die „dynamische Sub-Achse" aus Kap. 2.3.
- **CoW-Memento-Zustand:** `cow_armed_`/`cow_materialized_` (`abi_adapter.hpp:1799-1800`) +
  `cow_materialize_copy_()` (`:1783`) — die erste Warmup-Mutation materialisiert lazy.
- **Descent-Slot:** `descent_slot_for_(key)` (`abi_adapter.hpp:1630-1638`) bestimmt den real
  berührten Store-Slot per Lower-Bound über die Laufzeit-Keys (T7-Prefetch-Ziel).

**Faustregel:** *Welches* Organ getroffen wird ⇒ `[CT]` (Binary). *Womit* es zur Laufzeit arbeitet
(Daten, geklemmte Steuerwerte, Memento) ⇒ `[RT]` (Schleife über demselben Binary).

---

## 9. Querverweise

- **Thesis Anhang F** — vollständige Soll-Semantik je `std::map`/`std::vector`-Funktion (C++23):
  `…/thesis/diplomarbeit/anhang/de/F_comparison_interfaces.tex` (Label `app:interfaces`, Tabellen
  `tab:if-map`, `tab:if-vector`, `tab:if-decomp`; EN-Spiegel: `anhang/en/F_comparison_interfaces.tex`).
  Anhang F nennt dieses Dokument explizit als „eigene Entwickler-Dokumentation der Cache-Engine"
  (`F_comparison_interfaces.tex:26-27` und `:230-232`).
- **`docs/architecture/35_function_handle_hops.md`** — kompaktere Schwester-Notiz (Doc 35); diese
  Datei ist die ausführliche AP-CE1-Fassung mit vollständiger Anhang-F-Abdeckung.
- **`docs/architecture/36_eine_architektur_lebewesen_ist_searchalgorithm.md`** — Lebewesen ≡
  `SearchAlgorithm`, die EINE achsentragende Anatomie (Hintergrund zu §0/§1).
- **`docs/ENTWICKLER-IDE-EINSTIEG.md`** — IDE-/Build-Einstieg im **Super-Repo** (übergeordnetes
  Projekt, nicht in diesem `external/comdare-cache-engine`-Submodul). Dort beginnt das Onboarding;
  dieses Dokument vertieft die ABI-/Achsen-Verdrahtung.
- **`docs/architecture/14_*` (Achsen-Komposition/Organ-Metapher)** und **Doc 18** (Paper-Code-Map) —
  Achsen-Definitionen je Organ.
- **`docs/architecture/31_observer_interface_konsolidierung_i1.md`** — Hintergrund zu §4.8
  (`tier_observe`, konsolidierter POD).

---

## 10. Einstieg für neue Entwickler:innen (Kurzpfad)

1. §0 (Definition) + §1 (Adapter) + §2 (19 Organe) lesen.
2. §3 verstehen: `[CT]` (eigene Binary, `resolve_baustein`) vs. `[RT]` (Laufzeit-Schleife).
3. `insert`/`lookup` in §4.1/§4.2 mit **offenem `abi_adapter.hpp`** Hop für Hop nachvollziehen.
4. Neue **Achse**: Concept + CRTP-Base + Observable-Wrapper nach Doc 14 (Goldstandard
   `axis_06_allocator`), dann hier den Hop-Pfad + die Übersichtstabelle §7 ergänzen.
5. Neue **Interface-Funktion**: §6-Backlog + Anhang-F-Soll-Semantik prüfen, ABI-Op + Hop-Pfad in
   §4/§5 anlegen und hier dokumentieren.

---

### Anhang-F-Abgleich: Status

**Anhang F gefunden und gespiegelt: JA.** Quelle `thesis/diplomarbeit/anhang/de/F_comparison_interfaces.tex`
(+ EN-Spiegel). **Alle** in `tab:if-map` und `tab:if-vector` gelisteten Standard-Funktionen sind in
§6 erfasst und je nach ABI-Realisierung als ✅ direkt / ⚠️ indirekt / ❌ nicht-verdrahtet markiert —
ohne Fabrikation. Funktionen ohne stützenden Code (`upper_bound`, `equal_range`, Iteratoren, `data`,
Kapazitäts-API, …) sind ausdrücklich als **nicht verdrahtet** ausgewiesen; das interne
Lower-Bound-Primitiv (`composable_search.hpp:89-93`) ist als vorhanden, aber **nicht ABI-exponiert**
vermerkt.
