# Session — Säule-2 (observe_all real): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow (Doku 24 §5.2/§5.3/§5.4)
**Workflow:** `wc6uuz57d` / Run `wf_166cf902-a80` — 7 Agenten, ~821k Subagent-Tokens, 272 Tool-Uses.
**Zweck dieser Doku:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 24 §5.2 (3-Pfad-Fragmentierung / observe_all NULL), §5.5 (Key-Type-Blocker), §6 (RESUME-PLAN);
Doku 14 §17.2/§20 (Anatomie = Organe + Observer). Vorgänger: Inc1 (`176c60b`) + Inc2 (`ccdbbde`).

> **Frage des Increments:** Wie liefert `AnatomyExecutionContext::observe_all()` ECHTE Per-Achsen-Statistik
> (statt NULL, Doku 24 §5.2), indem der Builder einen `ComposedSearch` (uint64) treibt — bei stabiler
> Context-API (21 builder_anatomy-Tests grün) und ausschaltbar via `COMDARE_CE_ENABLE_STATISTICS`?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)

### 1.1 Builder-Mess-Pfad + die NULL-Lücke
- **Defekt-Ort:** `anatomy_execution_context.hpp:83` hält `std::map<uint64,uint64> container_` (R5.B-Pilot); `:48-66` treiben **ihn**; `:69-71` `observe_all()` delegiert aber an `anatomy_.observe_all()` → liest das **ungetriebene** Anatomie-Organ → alle Zähler 0.
- **WICHTIGE KORREKTUR (Synthese):** Die Anatomie ist **nicht** inert — `search_algorithm_anatomy.hpp:101` hält `Composition::search_algo axis_search_algo_{}` real, und `:62-72` ruft bereits `axis_search_algo_.statistics()` unter `if constexpr (ObservableAxis<…>)`. Die Säule-2-Verdrahtung ist dort **schon eingebaut**; der einzige Defekt ist, dass der Context dieses Organ **nie treibt** (er fährt sein eigenes std::map).
- **Root-Cause (Doku 24 §5.5):** schmale Organ-Keys (Array256=uint8, BST/BTree=uint16) → das Anatomie-Organ als allgemeiner uint64-Container ist verlustbehaftet. Lösung: ein **eigener uint64-Container** im Context (ComposedSearch), getrennt vom narrow-key-Lebewesen-Wrapper.

### 1.2 Observer-Infrastruktur
- **`ObservableAxis`-Concept** (`observer_aggregate.hpp:40-44`): `typename A::snapshot_t; { a.statistics() } -> same_as<snapshot_t>`.
- **`snapshot_of_t<A>`** (`:46-64`): ObservableAxis → `A::snapshot_t`, sonst `EmptyAxisSnapshot` (POD) — Compile-Zeit, 0 Runtime-Overhead.
- **`SearchAlgoStatistics`** (`concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp:44-52`): total_lookup/hit/miss/insert/erase_count, peak_occupancy, avg_density. POD/trivially_copyable → ABI-stabil.
- **Präzedenz `Array256SearchAlgo`** (`array256:70-131`): statistics()/snapshot_t/observer() unter `#ifdef COMDARE_CE_ENABLE_STATISTICS` + `MeasurableObserver<Snapshot>` (`measurement/measurable_concept.hpp:49`).

### 1.3 Die 21 builder_anatomy-Command-Tests
- Sehen NUR die Context-Fassade: `insert→bool` (inserted-Flag, Tests erwarten true bei Neueinfügung), `lookup→optional`, `erase→bool`, `clear`, `size`, `empty`, `observe_all`.
- `ObserveAllDelegatesToAnatomy`/`…Snapshot`/`Full…ViaCommands` prüfen nur `total_slots()==17` (Typ-static_assert) — keine ECHT-Werte (heute alle 0).
- TYPED_TEST über 11 Compositions → Container muss Composition-unabhängig bleiben.

---

## 2. Design-Phase (3 Linsen, alle risk=medium)

| Linse | Kern | Verdikt |
|-------|------|---------|
| **A — ObservableComposedSearch-Wrapper** als Context-Container; observe_all liest den getriebenen Store | Wrapper trägt statistics(); Anatomie/composable_search unangetastet | **GEWÄHLT** (+ C-Rewire) |
| **B — self-counting ComposedSearch** (statistics am ComposedSearch-Level) | ändert `composable_search.hpp` invasiv | verworfen (legt statistics auf falsche Schicht, verletzt „ComposedSearch bleibt unverändert") |
| **C — Context treibt Organ; Anatomie-observe_all liest automatisch** | direktester Lücken-Schluss, berührt aber Anatomie+Context | Graft: der **observe_all-Rewire** aus C übernommen, ohne die Anatomie zu ändern |

---

## 3. Gewählter Blueprint (Linse A + C-Rewire) — Anatomie & composable_search UNANGETASTET

**NEU** `…/composable/observable_composed_search.hpp`: `ObservableComposedSearch<Traversal, Store>`
- hält `ComposedSearch<Traversal,Store> search_{}` by value; `key_type/value_type = Store::… (uint64)`.
- `insert→bool` rekonstruiert das inserted-Flag via `lookup` vor `insert` (ComposedSearch::insert ist void).
- **Unter `#ifdef COMDARE_CE_ENABLE_STATISTICS`:** `snapshot_t = SearchAlgoStatistics`, `statistics()`, `reset()` (= Statistik-Reset, NICHT Container-clear — Memory-Regel), `observer()` (MeasurableObserver). Zählt insert/lookup/hit/miss/erase/peak_occupancy. Bei OFF: nackter Pass-Through, 0 Footprint, `ObservableAxis` = false → `EmptyAxisSnapshot`.

**GEÄNDERT** `anatomy_execution_context.hpp`:
- `#include <map>` entfernen; `observable_composed_search.hpp` includen.
- `container_t = ObservableComposedSearch<SortedBinaryTraversal, RawSlotStore>` (uint64 → §5.5 gelöst; RawSlotStore vector-backed → **keine** Allocator-Lifetime-Falle wie bei ComposedStore; Composition-unabhängig → für alle 11 identisch).
- insert/lookup/erase/clear/size/empty: Signaturen byte-identisch, an `container_` delegiert.
- **Kern-Rewire `observe_all()`:** `agg = anatomy_.observe_all()` (Default für die 16 nicht-getriebenen Achsen), dann unter STATISTICS + doppeltem `if constexpr (ObservableAxis<container_t>)` **und** `(ObservableAxis<Composition::search_algo>)`: `agg.search_algo = container_.statistics();`.
- **Typkompatibilität (verifiziert):** `agg.search_algo` ist `snapshot_of_t<Composition::search_algo>` = `SearchAlgoStatistics` für alle 11 Compositions (Präzedenz `array256:124` + Concept-Pflicht). `container_.statistics()` liefert ebenfalls `SearchAlgoStatistics`. Das innere `if constexpr` schützt den hypothetischen Non-Observable-Fall.

**GEÄNDERT** `test_v41_builder_anatomy_commands.cpp`: 21 bleiben grün (uint64-Keys → `lookup(999)` ist sauberer Miss statt narrow-key-Kollision) + neuer `R5B_ObserveReal.SearchAlgoCountersReflectDrivenOps` (unter STATISTICS): beweist insert/lookup/hit/miss/erase-Zähler + Idempotenz → schließt §5.2-Lücke (scheitert mit der alten std::map). + Compile-Selbstbeweis `static_assert(ObservableAxis<ObservableComposedSearch<…>>)`.

### Kritische Korrekturen des Blueprints (gegen Findings/Designs)
1. Anatomie ist **nicht** inert (observe_all bereits verdrahtet, `:62-72,:101`) — nur das Organ wird nicht getrieben.
2. `agg.search_algo`-Zuweisung ist typkompatibel (beide `SearchAlgoStatistics`), abgesichert durch doppeltes `if constexpr`.
3. Container = **RawSlotStore** (nicht ComposedStore) → vermeidet die Allocator-Adapter-Copy/Move-Lifetime-Falle aus Inc2.

---

## 4. Implementierungs-Plan + Build-Hinweis
- A: `observable_composed_search.hpp` (additiv, beide Flags). B: Context-Swap + observe_all-Rewire. C: Test.
- **Build:** beide Konfigurationen — Default + explizit `COMDARE_CE_ENABLE_STATISTICS=ON`, damit der ECHT-Wert-Test wirklich läuft (er steht unter `#ifdef`; bei OFF kompiliert er zu nichts). **Offen zu prüfen:** Default-Zustand des Flags in der msvc-release-Preset (falls OFF → Test-Target braucht `target_compile_definitions(... PRIVATE COMDARE_CE_ENABLE_STATISTICS=1)` analog F15).
- Verifikation: ctest beide Konfigurationen; 21 + 1 grün, 0 Regressionen.

## 5. Scope-Grenze (Folge-Increments)
- Lebewesen-Wrapper-Umstufung (search_algo bleibt narrow-key Lebewesen-Wrapper, vom Context NICHT getrieben).
- Übrige 16 Observer-Slots bleiben Default/EmptyAxisSnapshot — nur `search_algo` echt.
- abi_adapter/run_workload-Mess-Last + Lebewesen-Wall-Clock-Anreicherung (eigene Säule).
- ComposedStore<N,L,A> als Context-Store (Allocator-Lifetime) — bewusst RawSlotStore-Pilot.
- `statistics()` in StorageOrgan/TraversalOrgan-Kernvertrag ziehen — bleibt verboten (Observable nur auf Wrapper-Ebene).

**Risiko:** medium — einzige nicht-triviale Stelle ist die Typkompatibilität über 11 Compositions + sauberes STATISTICS=OFF-Auskompilieren, beides durch doppeltes `if constexpr`-Gate strukturell abgesichert.
