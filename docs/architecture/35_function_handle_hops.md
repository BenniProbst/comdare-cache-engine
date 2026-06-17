# Doc 35 — function-handle-hops (Entwickler-Einstieg)

> **Zweck / Anforderung C02-5.** Dieses zentrale Entwickler-Dokument erklärt, **wie jede äußere
> Interface-Funktion intern verdrahtet ist** — den exakten Pfad vom ABI-Eintritt über die Achsen-Handles
> („Hops") bis zum Ergebnis. Es ist das in der Diplomarbeit (Kap. 2.3 / 2.3.1) angekündigte
> Entwickler-Dokument und der Einstiegspunkt für neue Entwickler:innen.
>
> **Quelle/Stand:** erstellt durch Code-Sichtung (read-only) am 2026-06-17; Datei-/Zeilenangaben sind ein
> Snapshot — bei Abweichung gilt der Code. Spiegel der vollständigen Interface-Tabelle: **Anhang F** der
> Thesis. Achsen-Definitionen: Doc 14; Paper-Code-Map: Doc 18.

---

## 0. Thesis-T-Nummer ↔ Code-`axis_NN` (WICHTIG — sie unterscheiden sich!)

Die Thesis nummeriert Achsen T0–T18; der Code nummeriert `axis_NN` historisch abweichend. Diese Tabelle
ist die autoritative Brücke:

| Thesis | Achse | Code-Header (Repräsentant) |
|---|---|---|
| T0  search_algo        | `axes/lookup/composable/` (axis_03a) + `axis_03a_search_algo_registry.hpp` |
| T1  cache_traversal    | `axes/cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp` |
| T2  mapping            | `axes/mapping/axis_03m_mapping_direct_placement.hpp` |
| T3  path_compression   | `axes/path_compression/axis_02_path_compression_observable.hpp` |
| T4  node_type          | `axes/node/axis_04_node_type_composed_store.hpp` |
| T5  memory_layout      | `topics/memory_layout/axis_05_memory_layout/…` |
| T6  allocator          | `axes/alloc/axis_06_allocator_*.hpp` |
| T7  prefetch           | `axes/prefetch_axis/axis_07_prefetch_observable.hpp` |
| T8  concurrency        | `axes/concurrency_axis/axis_08_concurrency_observable.hpp` |
| T9  serialization      | `topics/serialization/axis_10_serialization/…` |
| T10 telemetry          | `topics/telemetry/axis_11_telemetry/…` |
| T11 value_handle       | `axes/value_handle_axis/axis_14_value_handle_observable.hpp` |
| T12 isa                | `axes/simd/axis_09_isa_observable.hpp` |
| T13 index_organization | `axes/index_organization/axis_01_index_organization_observable.hpp` |
| T14 io_dispatch        | `axes/io_dispatch/axis_io_dispatch_observable.hpp` (+ `axis_io_mmap.hpp`, `axis_io_direct.hpp`) |
| T15 migration_policy   | `axes/migration_policy/axis_migration_observable.hpp` |
| T16 filter             | `axes/filter_axis/axis_filter_observable.hpp` |
| T17 queuing_q1         | `topics/queuing/axis_q1_queuing/…` |
| T18 queuing_q2         | `topics/queuing/axis_q2_queuing/…` |

> Merke: Code-`axis_14` = **value_handle** (T11), NICHT io_dispatch (T14). Code-`axis_09` = isa (T12),
> NICHT prefetch.

---

## 1. ABI-Eintrittspunkte (drei Gattungs-Adapter)

| Gattung | Adapter | Datei | Öffentliche Tier-API |
|---|---|---|---|
| SearchAlgorithm (map-artig) | `SearchAlgorithmAbiAdapter<A>` | `anatomy/abi_adapter.hpp` | `tier_insert/tier_lookup/tier_erase/tier_clear/tier_size` + `tier_observe` |
| Container/Set (set-artig) | `SetAbiAdapter<A>` | `anatomy/set_abi_adapter.hpp` | `tier_set_insert/contains/erase/size/clear` + `tier_observe_set` |
| Container/Sequence (vector-artig) | `SequenceAbiAdapter<A>` | `anatomy/sequence_abi_adapter.hpp` | `tier_push_back/tier_at/tier_size/tier_clear` + `tier_observe_sequence` |

Die ABI-Schicht exportiert ausschließlich `uint64`-Primitiven (key/value/index) und einen POD-Observer —
das ist die binär-stabile Modul-Grenze. Reichhaltige `std::map`/`std::vector`-Semantik (Iteratoren,
`operator[]`, Bereichsabfragen) wird hier **bewusst nicht** durchgereicht (siehe §6).

---

## 2. Achsen-Auflösung: Prüfling vs. Standard (`resolve_baustein`)

**Datei:** `include/cache_engine/abi/resolve_baustein.hpp`

Pro Achsen-Tag wählt `resolve_baustein<Algo, Tag>` zur **Übersetzungszeit**:

```
1. if constexpr (has_member_baustein<Algo, Tag>)  ->  Algo::baustein_t<Tag>   (Prüfling-Implementierung)
2. else                                           ->  cache_engine::baustein_t<Tag>::type  (CE-Standard)
3. sonst                                           ->  void
```

11 Achsen-Tags: `PageAxisTag, NodeAxisTag, TraversalAxisTag, ValueHandleAxisTag, ConcurrencyAxisTag,
AllocatorAxisTag, PrefetchAxisTag, TelemetryAxisTag, IsaAxisTag, LayoutAxisTag, ReclamationAxisTag`.
Das ist der Kern des Prüfling-gegen-Standard-Mechanismus (Tag-Dispatch + `std::conditional_t`).

---

## 3. Compile-Time-Klassifikationen, die den Pfad bestimmen

| Concept/Test | Datei | Wirkung |
|---|---|---|
| `StoreTraversableSearchAlgo<S>` | `axes/lookup/composable/store_traversable_search_algo.hpp` | **Weg A** (Traversal über `ComposedStore`) vs. **Weg B** (separates Such-Organ) |
| `ObservableAxis<T>` | (concepts) | besitzt die Achse einen Observer? steuert `tier_observe`-Aggregation |
| `MementoAxis<T>` | (concepts) | save/restore-Fähigkeit (Warmlauf-Zustand, CoW) |
| `if constexpr (requires { … })` | je Aufrufstelle | optionale Auto-Kopplung der Achsen-Handles |

---

## 4. Hop-Pfade pro Interface-Funktion (SearchAlgorithm / map-artig)

Alle Pfade ab `anatomy/abi_adapter.hpp`. `[CT]` = Compile-Time-Entscheidung, `[RT]` = Runtime-Zustand.

### `insert(key, value) -> bool`  (`tier_insert`)
```
tier_insert(uint64,uint64)
 ├─ [RT] CoW-Memento: if (cow_armed_) cow_materialize_copy_()
 ├─ [CT] StoreTraversableSearchAlgo<SearchAlgo>?
 │    ├─ Weg A:  container_.insert(k,v)
 │    │           -> ObservableComposedSearch::insert        (axes/lookup/composable/observable_composed_search.hpp)
 │    │              -> ComposedSearch::insert
 │    │                 -> Traversal::insert_into<Store>      (SortedBinary | LinearScan)
 │    │                    -> ComposedStore<N,L,A>::insert_slot_at   (axis_04_node_type_composed_store.hpp)
 │    │                       -> slots_.emplace(...)   [T6 Allocator REAL über StdAllocatorAdapter]
 │    └─ Weg B:  search_organ_.insert(k,v)  +  container_.insert(k,v)
 └─ Auto-Kopplung (je `if constexpr (requires …)`):
      [T3] pc_organ_.compress · [T10] telemetry_organ_.record_node_touch ·
      [T1] ct_organ_.register_entry · [T2] map_organ_.register_slot ·
      [T17] queuing_q1.put · [T18] queuing_q2.should_flush/on_flush_complete ·
      [T7] pf_organ_.observe_prefetch · [T8] cc_organ_.observe_critical_section
```

### `lookup(key, out) -> bool`  (`tier_lookup`)
```
 ├─ [CT] Weg A: container_.lookup(k)  -> ObservableComposedSearch::lookup -> ComposedSearch::lookup
 │              -> Traversal::lookup_in<Store> -> lower_bound_index()+compare  (SortedBinary)
 │       Weg B: search_organ_.lookup(k)
 ├─ Ergebnis std::optional<value> -> *out
 └─ Auto-Kopplung: [T1] resolve · [T2] resolve_offset · [T10] record_node_touch ·
      [T17] get · [T7] observe_prefetch · [T8] observe_critical_section · [T3] compress
```

### `erase(key) -> bool`  (`tier_erase`)
```
 ├─ [RT] CoW-Memento
 └─ [CT] Weg A: container_.erase(k) -> … -> Traversal::erase_from<Store> -> store_.erase_slot_at(i) [T6 real]
         Weg B: search_organ_.erase(k) + container_.erase(k)
```

### `clear()`  (`tier_clear`)
```
 ├─ [RT] CoW-Memento
 ├─ search_organ_.clear() + container_.clear() -> ComposedSearch::clear -> store_.clear()
 └─ Statistik-Reset aller Organe (je if constexpr): T0,T1,T2,T3,T7,T8,T10,T17,T18 .reset()
```

### `size() -> uint64`  (`tier_size`)  → `search_organ_.occupied_count()` / `container_` Größe.

---

## 5. Hop-Pfade: Set- und Sequence-Gattung

**Set** (`set_abi_adapter.hpp`): `tier_set_insert→anatomy_.insert` · `tier_set_contains→contains` ·
`tier_set_erase→erase` · `tier_set_size→size` · `tier_set_clear→clear`; `tier_observe_set` mappt
`observe_all()` auf den Set-POD.

**Sequence** (`sequence_abi_adapter.hpp`): `tier_push_back→anatomy_.push_back` · `tier_at→at(index)`
(bounds-checked, `std::optional`) · `tier_size→size` · `tier_clear→clear`; `tier_observe_sequence`
deckt 2 beobachtbare Achsen (V-Speicher + Growth) ab.

---

## 6. NICHT verdrahtete `std::map`/`std::vector`-Funktionen (Stand 2026-06)

Die ABI reicht nur Primitiven durch; folgende Standard-Funktionen sind **bewusst nicht** auf ABI-Ebene
realisiert (intern teils vorhanden, z. B. `lower_bound` in `SortedBinaryTraversal`):

- **map:** `find` (→ `lookup`+`optional`), `insert_or_assign`/`emplace`/`emplace_hint` (→ `insert`),
  `operator[]`, `at(key)`, `lower_bound`/`upper_bound`/`equal_range`, Iteratoren/Range-Iteration.
- **vector:** `operator[]` (→ `at`), `data()`, positions-`insert`/`erase`, `resize`/`reserve`/`capacity`,
  `emplace_back` (→ `push_back`), Iteratoren.

Diese sind der natürliche Erweiterungs-Backlog; Anhang F der Thesis listet die **vollständige**
Soll-Semantik je Funktion. Wer eine dieser Funktionen ergänzt, dokumentiert ihren Hop-Pfad hier nach.

---

## 7. Observer-Aggregation (`tier_observe`, 19 Achsen) — Pfad B

`fill_observer_v3()` (`abi_adapter.hpp`) erhebt je Achse einen Observer in fester Schema-Reihenfolge
(`kV3AxisSchema`, Indizes 0–18). Zwei Erhebungs-Arten:
- **Instanz-Organe** (member, direkt): T0,T1,T2,T3,T6,T7,T8,T10,T17,T18 → `.statistics()`.
- **Pfad-B-Zustand-Scan** (`reset()` + `container_.store_observe_<axis>()` const-scan): T4,T5,T9,T11,T12,
  T13,T14,T15,T16.

Im Builder-Kontext füllt `AnatomyExecutionContext::observe_all()`
(`builder/anatomy_commands/anatomy_execution_context.hpp`) Slot 0 (search_algo) + Slot 6 (allocator) mit
den **echten** getriebenen Organ-Werten (Säule-2).

---

## 8. Standard-Komposition als Referenz (ART)

`compositions/art_reference.hpp` belegt alle 19 Achsen + Build-Achsen (Auszug): `search_algo =
ObservableArtTrieOrgan`, `node_type = ObservableNodeType<Node256>`, `memory_layout =
CacheLineAlignedMemoryLayout`, `allocator = MimallocAllocator`, `concurrency = OlcOptimisticConcurrency`,
`telemetry = LeafOnlyCounter`, `value_handle = InlineValueHandle`, `io_dispatch = InMemoryOnly`, … —
das ist die Vorlage, an der man die Achsen-Slots eines neuen Lebewesens abliest.

---

## 9. Einstieg für neue Entwickler:innen (Kurzpfad)
1. Lies §1 (Adapter) + §2 (`resolve_baustein`) + §3 (Klassifikationen).
2. Folge `insert`/`lookup` in §4 mit offenem `abi_adapter.hpp`.
3. Für eine neue Achse: Concept + CRTP-Base + Observable-Wrapper nach Doc 14 (Goldstandard
   `axis_06_allocator`), dann hier den Hop-Pfad nachtragen.
4. Für eine neue Interface-Funktion: §6-Backlog + Anhang-F-Sollsemantik, dann Pfad in §4/§5 ergänzen.
