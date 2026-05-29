# Session — Roadmap-1 (Observability auf weitere Achsen): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow · **Task:** #36 (User-Roadmap Schritt 1, Pflicht)
**Workflow:** `wqnbqbm3w` / Run `wf_254d1aaf-880` — 7 Agenten, ~716k Subagent-Tokens, 238 Tool-Uses.
**Zweck:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 24 §2.2/§3 (Säule 2). Vorgänger: Säule-2-Kern (observe_all real für search_algo, ce `8bc39fb`).

> **Frage:** Wie weiten wir den `observe_all`-ECHT-Pfad von `search_algo` auf weitere der 17 Achsen aus —
> strukturell vollständig (alle observablen Slots) UND mit mindestens einer ZWEITEN real-getriebenen Achse?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)

### 1.1 Anatomie + ObserverAggregate — nur 4 von 17 Achsen sind ObservableAxis
- `ObservableAxis` (snapshot_t + statistics()) erfüllen **nur 4**: **search_algo (03a)**, **cache_traversal (03b)**, **mapping (03m)**, **allocator (06)**. Die übrigen **13** sind nicht-observable → `snapshot_of_t<…>` = `EmptyAxisSnapshot` (POD-Fallback, `observer_aggregate.hpp:48-64`).
- Die Anatomie hält heute nur `axis_search_algo_` und füllt nur `agg.search_algo`. Der „protected-CRTP-Ctor"-Blocker ist **veraltet** (Datei-Kommentar `:59-61` selbst als stale markiert) — `axis_search_algo_` beweist, dass abgeleitete Wrapper holdbar sind.
- `observable_count()` (`observer_aggregate.hpp:115-135`) zählt bereits alle 17 auf ObservableAxis.

### 1.2 Treibbare Achsen — allocator ist die natürliche zweite
- **allocator (axis_06):** `AllocationStatistics` (7 Felder: total_bytes_allocated/in_use, allocation/deallocation/failure_count, ext/int_fragmentation), `statistics()`/`reset()` unter STATISTICS. **Real getrieben**, wenn der Container `ComposedStore<N,L,A>` nutzt: `std::vector<Slot, A::StdAllocatorAdapter<Slot>>` allokiert bei jedem insert/erase → allocation_count/total_bytes_in_use steigen.
- **Lifetime:** `ComposedStore` Copy/Move=delete (Adapter hält `Derived*`); `ObservableComposedSearch` hält Store by value, nie kopiert → **verträglich** (der in Inc2 als Folge-Increment vermerkte Lifetime-Punkt ist damit gelöst).
- **layout (axis_05):** nur `organ_scan_field_sum()` (One-Shot, kein Workload-Statistik-Reset) → nicht kontinuierlich getrieben. **node_type (axis_04):** nur `max_capacity()`, keine statistics(). → beide bleiben strukturell-0.

### 1.3 Test-Landschaft
- Bestehende Tests prüfen `total_slots()==17` (Typ-static_assert) + `observable_count()` (AllEmptyComposition==0). Müssen grün bleiben (ObserverAggregate-Layout unangetastet).
- `R5B_ObserveReal` (search_algo) + `Saeule2_ObserveAllReal` (treibt Anatomie direkt) bleiben unberührt.

---

## 2. Design-Phase (3 Linsen) + Korrekturen

| Linse | Kern | Verdikt |
|-------|------|---------|
| **A — alle 17 Slots, Anatomie iteriert** | 16 zusätzliche Organ-Member + observe_all über alle | überdimensioniert (15 Member bleiben 0; unnötig) |
| **B — allocator real via ComposedStore** | Container nutzt Composition-Allocator → agg.allocator real | **GEWÄHLT** (+ Graft) |
| **C — Context treibt Parallel-Organ** | künstliche Op-Spiegelung in Anatomie-Organe | verworfen (verstößt gegen „std::map = Interface, Achse = INNEN-Verhalten"; basierte zudem auf veraltetem Checkout) |

**Synthese-Korrekturen (gegen veraltete Design-Annahmen):** ComposedStore + ObservableComposedSearch existieren bereits (Inc1/Inc2 done); search_algo ist bereits die 1. reale Achse; der protected-Ctor ist kein Blocker. **Latenter Bug entdeckt:** `composed_store.hpp` deklariert `slot_t`/`slot_alloc` **doppelt** (Z. 57-58 + 107-108; aus Inc2, MSVC-toleriert, GCC/Clang-brechend) → Cleanup in diesem Increment.

---

## 3. Gewählter Blueprint (LINSE B, gegraftet)

**Ergebnis:** 2 real-getriebene Achsen (`search_algo` + `allocator`) aus EINEM Workload; die übrigen 15 bleiben bewusst strukturell-Default (`EmptyAxisSnapshot`), alle durchlaufen `snapshot_of_t` — die ehrlichste „Verdrahtung aller 17 Slots". Anatomie-Klasse + ObserverAggregate **unangetastet**.

**Änderungen (4 Produktiv + 1 Test):**
1. **`composed_store.hpp`** — additiv `allocator_statistics()` (gegated, NICHT-Vertrags-Methode: `A::statistics()` ist unter STATISTICS Pflicht-API; StorageOrgan-Kernvertrag fordert es bewusst nicht). **+ Doppel-`slot_t`/`slot_alloc`-Deklaration bereinigen** (späte Z.107-108 entfernen, frühe behalten).
2. **`composable_search.hpp`** — additiv `Store const& store() const noexcept` (read-only Accessor).
3. **`observable_composed_search.hpp`** — gegateter, `requires`-detektierter `store_allocator_statistics()` + `store_has_allocator_stats<S>` (KEIN Runtime-Switch; ObservableAxis-Vertrag über search_algo unverändert).
4. **`anatomy_execution_context.hpp`** — `container_t` = `ObservableComposedSearch<SortedBinaryTraversal, ComposedStore<Composition::node_type, ::memory_layout, ::allocator>>`; `observe_all()` füllt zusätzlich `agg.allocator = container_.store_allocator_statistics()` (doppeltes `if constexpr`-Gate). insert/lookup/erase/clear/size/empty byte-identisch.
5. **Test** `R5B_ObserveMultiAxes.SearchAlgoAndAllocatorBothDrivenFromOneWorkload` — beweist beide Achsen real aus einem Workload (search_algo-Zähler + allocator allocation_count>0/bytes_in_use>0), 2 verschiedene Snapshot-Typen, `observable_count()>=2`, Idempotenz. + Compile-Selbstbeweis `ObservableAxis<ArtComposition::allocator>`.

**Typkompatibilität:** alle 11 Compositions nutzen `MimallocAllocator` (snapshot_t=AllocationStatistics) → `agg.allocator`-Zuweisung für alle gültig.

---

## 4. Build-grüne Reihenfolge + Hinweise
Dateien 1→2→3→4→5 (Datei 4 = Container-Wechsel = riskantester Schritt, erst nach 1-3 grün). STATISTICS=ON ist Default (`CMakeLists.txt:53`) → der neue `#ifdef`-Test läuft. Zusätzlich ein OFF-Lauf als Negativ-Beweis. Nur `>0`/`>=` asserten (vector-Growth-abhängig), keine exakten Byte-Zahlen.

## 5. Scope-Grenze (Folge-Increments)
Die übrigen 15 Achsen observable machen (axis_05/04 bräuchten statistics()-API); `statistics()` in StorageOrgan/TraversalOrgan-Kernvertrag ziehen (verboten); Anatomie/ObserverAggregate-Layout; bounded ComposedArrayStore; echte Layout-Slot-Anordnung; abi_adapter/Tier-Wall-Clock (Task #38); Tier-Wrapper-Umstufung (#40).
