# 29 — Experiment-B+-Baum: Generizität für weitere Achsen + Composition-Driver-Befund (2026-06-02)

> Autoritative Antwort auf die (früher gestellte) User-Frage „Ist die Experiment-B+-Baum-Engine generisch für
> weitere Achsen formuliert?" — mit Code-Beleg + dem konkreten ersten Schritt des V42-Composition-Drivers.

## §1 Antwort: JA — der Baum-KERN ist generisch (diese Session belegt es literal)

| Ebene | Generizität | Beleg (Datei) |
|-------|-------------|---------------|
| **Baum-Kern** | **VOLL generisch** — beliebige Achsenzahl, beliebige Achsen-Namen, static/dynamic gemischt, beliebige Gattung; NICHT auf 17/22 hartcodiert | `experiment_tree.hpp`: `build(std::vector<AxisLevel> const&)`, `AxisLevel{axis,values,is_static,variable,block_id}` (string/wert-basiert), `StaticBinaryView` (mixed-radix über beliebig viele Ebenen), `binary_count()` = ∏ über `static_levels_`, `for_each_node`/`for_each_binary`/`DynamicDim` |
| **Registry→Baum (BR-1)** | generisch — reflektiert beliebige Enabled-mp_lists zu AxisLevels | `registry_to_axis_levels.hpp` (`build_all_axis_levels`, block_id-getaggt) |
| **Gattungs-Bindung** | pro-Gattung-parametrisch + **erweiterbar** (neue Gattung = neue Spezialisierung) | `genus_binding_traits.hpp`: `GenusBindingTraits<G>` (slot_count + CompositionFor + axis_names), `GenusBound<G>`-Concept |
| **Selektion** | gattungs-agnostisch (∏-lazy → endliche K) | `coverage_selection.hpp` (BuildSelection) |

**Literaler Beleg dieser Session (genau die frühere Anforderung „verkraftet andere AxisBlocks mit anderen static/dynamic-Achsen + baut die andere Gattung"):** ALLE 5 Anatomie-Gattungen sind über DENSELBEN Baum-Kern gebunden, mit UNTERSCHIEDLICHER Achsenzahl — SearchAlgorithm **17**, Adapter/Container **2**, Set **15**, Sequence **10+axis_growth=11**, View **7** — je in-process + DLL-Round-Trip verifiziert (`test_genus_binding` 5/5; `test_dgenus_dll`; `test_d9/d10/d11/d12`). Der Baum ist also nachweislich generisch für verschiedene Achsenzahlen UND Gattungen.

## §2 Grenze (bewusste Invariante, ehrlich)

Die **Gattungs-Komposition + ObserverAggregate sind fest-N pro Gattung** (`observer_aggregate.hpp`: 17 named Member, `total_slots()==17`; `composition_factory.hpp`: `AdHocComposition<17>` mit `static_assert sizeof...(Vs)==17`). Das ist die **Gattungs-Invariante** (die N ist die Gattungs-Identität, Doku 14 §32) — KEIN variadisches N. Konsequenz:
- **Neue GANZE Gattung** hinzufügen = generisch (GenusBindingTraits-Spezialisierung + eigene fest-N-Komposition; diese Session 3× getan: Set/Sequence/View).
- **Neue Achse INNERHALB einer bestehenden Gattung** (z.B. SearchAlgorithm 17→18) = erfordert das Anpassen ihrer Komposition + ObserverAggregate + die 5 Außen-/Build-Achsen-Konventionen. Bewusst nicht generisch (sonst verlöre die Gattung ihre stabile ABI-Identität).

## §3 Composition-Driver-Befund (der konkrete erste V42-Schritt für #74-L-74c-Voll)

**Ziel (Doc 28 §1, R5.B):** mehr als search_algo+allocator real in `observe_all`/`tier_observe` treiben+beobachten (die 4 OperativeCapable-Achsen node_type/memory_layout/serialization/telemetry, `axis_operability_classification.hpp`).

**KRITISCHER BLOCK (verifiziert, `search_algorithm_anatomy.hpp:59-61` + Z.62-72/99-101):** Heute hält die Anatomie NUR `axis_search_algo_` als Member; `observe_all()` sammelt nur `agg.search_algo`. Der Wrapper-Kommentar nennt den Grund wörtlich: *„Achsen-Wrappers haben protected CRTP-Constructor — direktes Halten als Anatomie-Member blockiert. Wird durch public-Constructor-Fix oder Tuple-basierte Komposition spaeter geloest."* `search_algo` funktioniert NUR, weil es über die Observable-Hülle (`ObservableComposedSearch`, #42) default-konstruierbar gemacht wurde; allocator wird separat über den `ComposedStore`-Vector im `abi_adapter` getrieben (R6-Inkrement-2b).

**Daraus folgt die präzise V42-Composition-Driver-Reihenfolge (ersetzt die grobe §3.1 der Session-Doku):**
1. **CRTP-ctor-Entsperrung je OperativeCapable-Achse** (Vorarbeit, der eigentliche Block): den protected CRTP-Konstruktor der telemetry/memory_layout/serialization/node_type-Wrapper public machen ODER je eine Observable-Hülle analog `ObservableComposedSearch` (#42) bereitstellen ODER die Anatomie auf **Tuple-basierte Organ-Komposition** umstellen (hält alle Achsen-Organe in einem `std::tuple`, default-konstruiert). Tuple-Weg ist der generischste (löst es für ALLE Achsen auf einmal).
2. **Organe halten + observe_all sammeln:** je Achse `if constexpr (ObservableAxis<...>) agg.<achse> = organ_.statistics();` (ObserverAggregate trägt die Felder bereits via `snapshot_of_t<>`).
3. **Kopplung (Treiben):** der Tier-insert/lookup muss die gekoppelten Achsen-Organe mit-treiben (telemetry-record beim insert etc.) — sonst bleiben ihre statistics() leer. Das ist der eigentliche „Driver".
4. **Cross-ABI-POD:** `ComdareTierObserverSnapshotV1` (observable_tier.hpp) um die konkreten Achsen-uint64-Felder **append-only** erweitern (Minor-Bump) + `tier_observe` (abi_adapter.hpp) + `NodeObserverSnapshot`/result_ingest/perm_runner-Format parallel; je Achsen-snapshot_t auf die POD-Felder mappen (heterogene telemetry-snapshots → Metrik-Auswahl).
5. **observable_axis_count ehrlich hochzählen** + Delta-vor/nach-Treiben-Test (kein Fake).

**Empfehlung:** Schritt 1 über die **Tuple-basierte Organ-Komposition** lösen (generisch, löst den CRTP-Block für alle 17 Achsen zugleich) — das ist zugleich die saubere Grundlage, um die Anatomie wirklich „Composition-driven" (Doku 14 §11.3/§12) statt search_algo-only zu machen. Schwerer Umbrella-Eingriff → eine Achse pro Commit, RAM-Watchdog, CTest-registrieren.

## §3a EMPIRISCHER BEFUND (test_d_v42_probe, literal grün 2026-06-02) — präzisiert §3

Direkt am Code verifiziert (telemetry-Achse, ohne `COMDARE_CE_ENABLE_STATISTICS`):

| Wrapper | default_constructible | ObservableAxis |
|---------|:---------------------:|:--------------:|
| InsertCounter / DensityTracker / LatencyHistogram | **1 (JA)** | **0 (NEIN)** |

**Zwei Korrekturen am §3-Befund:**
1. **Der protected CRTP-Base-ctor (`TelemetryStrategyBase`) ist KEIN Block für die telemetry-Derived** — `InsertCounter` hat einen impliziten public default-ctor (ruft den protected Base-ctor zulässig, da abgeleitet), `t::InsertCounter ic;` kompiliert. Der search_algorithm_anatomy.hpp:59-Block ist also **achsen-spezifisch** (gilt für search_algo, das NICHT trivial default-konstruierbar ist → braucht die ComposedStore-Komposition + #42-Hülle), **NICHT pauschal für alle Achsen**. → Tuple/Hülle ist für telemetry NICHT nötig; telemetry ist direkt als Member haltbar.
2. **ObservableAxis ist an `COMDARE_CE_ENABLE_STATISTICS` gebunden** (statistics()/snapshot_t sind flag-gekapselt, wie bei FIFOQueueBuffer/WatermarkFlush). Ohne Flag → kein Observer (Release-Pfad, korrekt). observe_all sammelt telemetry also nur im STATISTICS-Build (konsistent mit dem search_algo-`if constexpr (ObservableAxis<>)`-Muster).

**Daraus der konkrete, machbare telemetry-Composition-Driver (nächster Code-Schritt):**
(a) `typename Composition::telemetry axis_telemetry_{}` als Member in SearchAlgorithmAnatomy (kein Block) + `telemetry_organ()`-Accessor;
(b) observe_all `if constexpr (ObservableAxis<typename Composition::telemetry>) agg.telemetry = axis_telemetry_.statistics();` (greift im STATISTICS-Build);
(c) **Kopplung (der eigentliche Driver):** der Tier-insert/lookup im abi_adapter muss `axis_telemetry_.record(...)` mit-treiben (sonst leer) — der Workload-Treiber koppelt search_algo-Op + telemetry-record;
(d) Cross-ABI-POD nur falls telemetry-Metriken über die DLL-Grenze sollen (append-only).
Verifikation: Test MIT `-DCOMDARE_CE_ENABLE_STATISTICS`, Delta telemetry-statistics vor/nach Treiben > 0. Achsen-Reihenfolge memory_layout/serialization/node_type analog prüfen (Probe je Achse zuerst).

## §3b KORREKTUR (2026-06-02, literaler Build-Fehler widerlegt §3a) — Member-Hold IST blockiert

**Der in §3a (a)+(b) vorgeschlagene Schritt wurde umgesetzt und schlug am realen Build fehl.** `cmake --build --target test_v41_anatomy_observer` (STATISTICS-Umbrella, min_free 11 GB) brach mit folgendem ab:

```
error: TelemetryStrategyBase<LeafOnlyCounter>::TelemetryStrategyBase ist nicht zugreifbar
  in der vom Compiler generierten SearchAlgorithmAnatomy<ArtComposition>::SearchAlgorithmAnatomy(void)
```

**Ursache + warum die Probe (§3a) falsch lag:** `test_d_v42_probe` testete `InsertCounter`/`DensityTracker`/`LatencyHistogram` — aber **`ArtComposition::telemetry` = `LeafOnlyCounter`**, ein anderer Wrapper. Der protected `TelemetryStrategyBase`-ctor blockiert das **direkte Member-Halten** (`axis_telemetry_{}` in einer fremden Klasse, die NICHT von `LeafOnlyCounter` erbt → kann den protected Base-ctor nicht aufrufen). Bei `t::InsertCounter ic;` im Probe-`main()` ist `main` zwar auch keine Derived, aber der Befund war schlicht **wrapper-unvollständig** — `LeafOnlyCounter` hat (anders als InsertCounter) keinen extern zugänglichen default-ctor-Pfad als Member. **§3a-Schlussfolgerung „telemetry direkt als Member haltbar" ist damit FALSCH für die reale Composition.**

**Konsequenz (Revert, Repo wieder grün — 13/13 test_v41_anatomy_observer):** Der Member-Hold-Ansatz ist verworfen. Der **korrekte Pfad ist der ursprüngliche §3 Schritt 1** — und zwar NICHT pro-Achse-stückeln, sondern an der Wurzel lösen:
- **Option T (empfohlen, generisch): Tuple-basierte Organ-Komposition.** Die Anatomie hält alle Achsen-Organe in einem `std::tuple<...>`, **default-konstruiert via `emplace`/Aggregat-Init mit Zugriffsfreundlichkeit**, ODER — sauberer — über je eine `ObservableComposed*`-Hülle pro Achse (analog `ObservableComposedSearch` #42), die den protected ctor kapselt. Tuple löst den Block für ALLE 17 Achsen zugleich; das ist der eigentliche „Composition-driven"-Umbau (Doku 14 §11.3/§12).
- **Option H (punktuell): pro OperativeCapable-Achse eine Observable-Hülle** mit public default-ctor, die das Organ kapselt (wie search_algo es via ObservableComposedSearch bereits tut). Weniger invasiv, aber 4×-Wiederholung.
- **Option C (Wurzel-Fix): protected CRTP-Base-ctor → public** in den Telemetry/MemoryLayout/Serialization/NodeType-Strategy-Bases. Am einfachsten, aber berührt die CRTP-Disziplin (protected-ctor verhindert Slicing/Fremd-Instanziierung) — nur nach Prüfung, ob das eine bewusste Invariante ist.

**Empirische Lehre für die nächste Runde:** Probe IMMER mit dem **real in der Composition verwendeten Wrapper** (hier `LeafOnlyCounter`, via `ArtComposition::telemetry` ermitteln), nicht mit beliebigen Vertretern der Achse. Der Default-Wrapper pro Achse steht in der jeweiligen `compositions/*_composition.hpp`.

## §3c VERIFIZIERTER BEFUND + LÖSUNG (2026-06-02, test_d_v42_probe2 + test_d_v42_telemetry_observable, beide literal grün)

**Probe2 (`test_d_v42_probe2`, REALER Wrapper LeafOnlyCounter) — literal:**

| Wrapper | is_aggregate | default_constructible | brace_ok `T{}` | ObservableAxis | HoldPlain (Member ohne `{}`) |
|---------|:---:|:---:|:---:|:---:|:---:|
| LeafOnlyCounter | **1** | 1 | **0** | **0** | **konstruiert** |
| InsertCounter | **1** | 1 | **0** | **0** | konstruiert |

**Zwei harte Korrekturen am §3b-Befund:**
1. Der „CRTP-ctor-Block" ist in Wahrheit ein **`{}`-Aggregat-Init-Artefakt**: Die Strategie-Wrapper sind **Aggregate** (`is_aggregate=1`); `T{}` (Aggregat-Init) spricht die protected Base **direkt** an (`brace_ok=0`) — aber **default-init OHNE `{}` klappt** (`HoldPlain<LeafOnlyCounter>` konstruiert). Der Build-Fehler kam vom Member-Initializer `axis_telemetry_{}` (Brace), nicht von genereller Nicht-Konstruierbarkeit. Member-Hold ist also möglich, nur ohne Brace bzw. über eine Nicht-Aggregat-Hülle.
2. **Aber die Strategie-Wrapper sind selbst NICHT `ObservableAxis`** (`ObservableAxis=0`, auch unter STATISTICS — sie haben kein `statistics()`/`snapshot_t`, nur `static constexpr` name/family/is_leaf_only). Member-Halten allein bringt also nichts; observe_all hätte weiter `EmptyAxisSnapshot` gesammelt.

**LÖSUNG = Hülle (Option H, exakt das search_algo-Vorbild):** `axes/telemetry_axis/axis_11_telemetry_observable.hpp` → `ObservableTelemetry<Strategy>`. Sie trägt die **Mess-Mechanik** (eigener Counter + `statistics()`/`snapshot_t`/`reset()`, gegated), parametrisiert durch die Strategie (`Strategy::is_leaf_only()`): leaf-only-Strategien verwerfen Inner-Node-Touches, non-leaf zählen sie. Die Hülle ist **kein Aggregat** → direkt als Anatomie-Member `{}`-haltbar. **`test_d_v42_telemetry_observable` (MIT STATISTICS) literal grün:**
```
LeafOnlyCounter : total=3 leaf=2 node=0 peak=2   (leaf-only verwirft Inner-Touch)
InsertCounter   : total=3 leaf=2 node=1 peak=3   (non-leaf zählt Inner-Touch)
```
Belegt: `ObservableAxis<ObservableTelemetry<...>> == true`, Delta vor/nach Treiben > 0 (kein Stub), Strategie parametrisiert die Mechanik (der messbare Achsen-Unterschied), Snapshot `standard_layout`+`trivially_copyable` (ABI-tauglich).

**Verdrahtungs-Plan (Option A, belegt durch das search_algo-Vorbild `art_reference.hpp:60` `using search_algo = ...::ObservableArtTrieOrgan`):**
1. Alias je Strategie: `using ObservableLeafOnlyCounter = ObservableTelemetry<LeafOnlyCounter>;` etc. (analog wie search_algo eine observable Hülle als Slot-Typ trägt).
2. Reference-Compositions (`art_reference.hpp:77` u.a.): `using telemetry = ...::ObservableLeafOnlyCounter;` statt der nackten Strategie. `IsComposition` fordert nur Typ-Existenz (`composition_concept.hpp:38`) → zulässig.
3. `search_algorithm_anatomy.hpp`: `typename Composition::telemetry axis_telemetry_{};` als 2. Organ (jetzt Hülle = kein Aggregat → `{}` klappt) + `telemetry_organ()`-Accessor + `observe_all` `if constexpr (ObservableAxis<...>) agg.telemetry = axis_telemetry_.statistics();`. Dann `snapshot_of_t<Composition::telemetry> == TelemetrySnapshot`, `observable_count()` steigt um 1.
4. **Kopplung (der eigentliche Driver):** der Tier-insert/lookup (`abi_adapter`/`anatomy_execution_context`) ruft `telemetry_organ().record_node_touch(is_leaf)`.
5. Cross-ABI-POD (`ComdareTierObserverSnapshotV1`) append-only um telemetry-uint64-Felder erweitern (nur wenn über die DLL-Grenze nötig).
6. **Risiko-Prüfung vor Schritt 2:** alle Stellen finden, die `Composition::telemetry` als `TelemetryStrategy` voraussetzen (memento_aggregate/codegen/Gattungs-Compositions) — der Slot-Typ-Wechsel darf sie nicht brechen. Umbrella-Build (`test_v41_anatomy_observer`) muss grün bleiben.
