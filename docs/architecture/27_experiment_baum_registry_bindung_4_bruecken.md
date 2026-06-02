# 27 — Experiment-B+-Baum: vollständige Registry-Bindung (die 4 Brücken) (2026-06-02)

> **Companion zu Doc 26.** Doc 26 definiert das B+-Baum-Modell; dieses Dokument ist der verbindliche
> Umsetzungsplan, der den heute STRING-getriebenen Baum **registry-getrieben + vollständig** gegen ALLE
> realen Achsen + Observer + Definitionen macht (aktives /goal 2026-06-02, „absolute Vollständigkeit").
> Provenienz: Code-Verifikation 2026-06-02 (permutation_engine.hpp, composition_factory.hpp, alle
> *_registry.hpp + *_config_set.hpp, all_axes_umbrella.hpp, observer_aggregate.hpp).

## 0. KORREKTUR des Achsen-Inventars — 22 Achsen, nicht 18

Ein früherer Explore-Agent zählte **18** und übersah `queuing q1/q2`, `simd_extension 09b`, `general_hardware 12`.
Rigorose Zählung der `*_registry.hpp` (dedupliziert axes/ vs topics/-Spiegel): **15 Topics, 22 Achsen** — exakt
Doku 22 „15 Topics · 22 Achsen". Die User-Angabe „22/23" war korrekt; „18" ist verworfen.

### 22 Achsen, gruppiert nach Topic + Enabled-Liste + Komposition-Zugehörigkeit

| # | Achse | Topic | Enabled-Liste | AdHoc-Slot |
|---|-------|-------|---------------|-----------|
| 1 | axis_03a_search_algo | traversal | `EnabledStrategies` | **T0** |
| 2 | axis_03b_cache_traversal | traversal | `EnabledStrategies` | **T1** |
| 3 | axis_03m_mapping | traversal | `EnabledStrategies` | **T2** |
| 4 | axis_02_path_compression | nodes | `EnabledCompressions` | **T3** |
| 5 | axis_04_node_type | nodes | `EnabledNodeTypes` | **T4** |
| 6 | axis_05_memory_layout | memory_layout | `EnabledLayouts` | **T5** |
| 7 | axis_06_allocator | allocator | `EnabledVendors` | **T6** |
| 8 | axis_07_prefetch | prefetch | `EnabledPrefetchers` | **T7** |
| 9 | axis_08_concurrency | concurrency | `EnabledStrategies` | **T8** |
| 10 | axis_10_serialization | serialization | `EnabledSerializers` | **T9** |
| 11 | axis_11_telemetry | telemetry | `EnabledTelemetries` | **T10** |
| 12 | axis_14_value_handle | value_handle | `EnabledHandles` | **T11** |
| 13 | axis_09_isa | hardware | `EnabledIsas` | **T12** |
| 14 | axis_01_index_organization | search_engine | `EnabledOrganizations` | **T13** |
| 15 | axis_io (io_dispatch) | io | `EnabledIos` | **T14** |
| 16 | axis_migration (policy) | migration | `EnabledMigrations` | **T15** |
| 17 | axis_filter | filter | `EnabledFilters` | **T16** |
| 18 | axis_01_page_type | nodes | `EnabledPageTypes` | — (außerhalb) |
| 19 | axis_09b_simd_extension | hardware | `EnabledExtensions` | — (außerhalb) |
| 20 | axis_12_general_hardware | hardware | `EnabledPlatforms` | — (außerhalb) |
| 21 | axis_q1_queuing | queuing | `EnabledStrategies` | — (außerhalb) |
| 22 | axis_q2_queuing | queuing | `EnabledPolicies` | — (außerhalb) |

**17 Achsen = AdHocComposition-Slots T0..T16** (composition_factory.hpp:41-66, feste Reihenfolge) → bilden EIN
baubares SearchAlgorithm-Tier-Binary. **5 Achsen außerhalb** (page_type, 09b, 12, q1, q2) sind separate
Sub-Achsen/andere Gattungen (queuing = Container/Queue-Gattung; 09b/12 = Hardware-Sub; page_type = nodes-Sub).

### 0.1 AUTORITATIV: „22" ist die Gesamtzahl, „17" ist NUR die SearchAlgorithm-Gattung (User 2026-06-02)

> **Kritische Klarstellung (User 2026-06-02): die „17" darf die „22" NICHT wegschrumpfen.** „17" ist KEINE
> Achsen-Gesamtzahl, sondern die **Slot-Zahl der SearchAlgorithm-Gattungs-Komposition** (`AdHocComposition<17>`,
> `ObserverAggregate<17>`, `sizeof...(Vs)==17` — eine GATTUNGS-Invariante, bleibt). Wo Doku „15 Topics / 17 Achsen"
> o.ä. als Bibliotheks-GESAMTZAHL schreibt, ist das FALSCH → **22 Achsen / 15 Topics**. Verifiziert 2026-06-02:
> 22 `axis_*_registry.hpp` (Liste in Session-Doku); `test_br1_full22_count` bindet alle 22 als Baum-Ebene.

**Differenzierte Bindung der 22 (User-Entscheidung „gattungs-korrekt", 2026-06-02) — KEINE Achse schrumpft weg:**

| Gruppe | Achsen | Bindung an Binary/Messung | Observer |
|--------|--------|----------------------------|----------|
| **SearchAlgorithm-Komposition (17)** | T0..T16 (search_algo … filter) | `AdHocComposition<17>` = EIN SearchAlgorithm-Tier-Binary (Gattungs-Invariante) | `ObserverAggregate<17>` |
| **SearchAlgorithm-Sub/Build-Varianten (3)** | page_type/01 (nodes-Sub), simd_extension/09b + general_hardware/12 (Hardware) | modifizieren DIESELBE SearchAlgorithm-Binary (zusätzliche Baum-Ebene + Build-/Codegen-Variante je Wert) — NICHT eigene Gattung | je eigener Observer/Definition (BR-3-OBS-22) |
| **Container-Gattung (2)** | queuing q1 + q2 | EIGENE Container-Gattungs-Komposition + eigenes **Container-Prüf-Dock** (Doku 24 §8.8) — NICHT in die SearchAlgorithm-17-Komposition (Cross-Genus type-unmöglich, Doku 14 §32) | eigener Container-Observer |

**Konsequenz:** Alle **22** erscheinen als volle Baum-Ebene (BR-1 ✓, registry-getrieben). Die 17 SearchAlgorithm-
Slots + die 3 Sub/Build-Achsen binden an die SearchAlgorithm-Tier-Binary; q1/q2 an eine getrennte Container-Tier-
Binary (eigene Gattung). JEDE der 22 trägt einen eigenen Observer + eigene Definition (kein Wegschrumpfen). Die
„17"-Stellen im Code/Doku, die `AdHocComposition`/`ObserverAggregate`/„17 Organe einer Komposition" meinen, sind
KORREKT und bleiben; nur Bibliotheks-GESAMTZAHL-Angaben werden auf 22 korrigiert.

## 1. Verifizierter Permutations-Mechanismus (an den der Baum bindet)

- `PermutationEngine<TopicConfigSets...>` (src/permutations/permutation_engine.hpp): `AllPermutations =
  mp_product<PermTuple, TopicConfigSets::StaticAxisVariants...>`; `count() = mp_size<AllPermutations>`;
  `for_each_permutation(v)` ruft `v.operator()<PermTuple<...>>()` je Permutation.
- `AdHocComposition<T0..T16>` (anatomy/composition_factory.hpp): 17 named using-Slots; `CompositionFromPermTuple<P>`
  = `AdHocComposition<Vs...>` aus `PermTuple<Vs...>` (static_assert sizeof...==17).
- `SearchAlgorithmAnatomy<Composition>` → `observe_all()` → `ObserverAggregate<C>` (anatomy/observer_aggregate.hpp,
  17 Snapshot-Slots; `ObservableAxis`-Concept: Achse hat `snapshot_t` + `statistics()`).
- `TopicConfigSet`s (topics/*/topic_*_config_set.hpp) exponieren je Achse `StaticAxisVariants_<id>` (= die
  Enabled-Liste, namespace-korrekt) — **der namespace-sichere Reflektions-Anker** (nicht die rohen Registry-NS).
- `all_axes_umbrella.hpp` (builder/codegen) inkludiert alle 17 Komposition-Registries + composition_factory.

## 2. Befund: heutige Lücke (warum „String-Gerüst")

- `profile_to_tree.hpp` baut `AxisLevel`s aus `ThesisProfile` + externer `AxisRegistry = std::map<string,
  vector<string>>` (XML) → **Strings, NICHT die echten mp_lists**.
- `NodeValue` (experiment_tree.hpp) = 4-uint64-Stub → **kein echter ObserverAggregate**.
- Keine Datei verbindet den Baum mit `PermutationEngine`/`for_each_permutation`/`ObserverAggregate`.

## 3. DIE 4 BRÜCKEN (Plan)

### BR-1 — Registry → Baum-Levels (registry-getriebene Permutation)
**Neu:** `builder/experiment_tree/registry_to_axis_levels.hpp`. Reflektiert die 17 Komposition-Achsen-Enabled-
Listen (Anker: `TopicConfigSet::StaticAxisVariants_<id>` bzw. die `Enabled*` über all_axes_umbrella) via
`mp_for_each` → je Achse ein `AxisLevel{axis_name, [W::name() …], is_static=true}` in T0..T16-Reihenfolge.
+ die statische Sub-Achse cacheline (45/Organ) für die 5 betroffenen Achsen + die dynamischen Sub-Achsen
(thread_count/hw_prefetcher) als `DynamicDim`. **Verifikation (Gate-1):** `tree.binary_count()` ==
`PermutationEngine<…17 ConfigSets…>::count()` == `∏ mp_size(Enabled_i)` — exakte Gleichheit. Die 5 Achsen
außerhalb werden als eigene Genus-/Sub-Achsen-Levels reflektiert (separater Teilbaum, NICHT im 17-Slot-Binary).

### BR-2 — Baum ↔ AdHocComposition (Blatt → reale Komposition)
**Neu:** `builder/experiment_tree/tree_to_anatomy_adapter.hpp` + `composition_registry.hpp`. Eine
`CompositionRegistry`, die via `PermutationEngine::for_each_permutation` jeden `PermTuple<17>` aufnimmt, keyed
über seinen **serialisierten Pfad** (axis=W::name() je Slot, T0..T16) → Factory, die `SearchAlgorithmAnatomy<
CompositionFromPermTuple<P>>` instanziiert. Der Baum-Blatt-`binary_id` (= derselbe serialisierte Pfad) schlägt
die Komposition nach. **Verifikation:** jeder Blatt-Pfad round-trippt auf genau eine reale Komposition; die
Pfad-Serialisierung von BR-1 (Level-Namen) == die von BR-2 (PermTuple-Namen).

> ✅ **BR-2 ERLEDIGT + VERIFIZIERT (2026-06-02, `test_br2_roundtrip`, cl, RAM-Watchdog 16.7 GB frei):** Umgesetzt
> als `composition_registry.hpp` (`CompositionRegistry::register_from_engine<PilotEngine>` → je `for_each_permutation`-
> Permutation eine reale `CompositionFromPermTuple<P>` = `AdHocComposition<17>`) + `axis_path_serialization.hpp`
> (DIE eine zentrale Pfad-Konvention `serialize_composition_path<P>()` == experiment_tree.hpp `binary_id`-Format).
> C1060-sicher über ein PILOT-Engine (schwere Achsen ×1, node_type/memory_layout ×2 → ∏=4); Doc 27 §6: nur die
> Pilot-Blätter werden compile-time materialisiert, NIE der Voll-Typ-Baum. Literale Belege: `reg.size() ==
> PilotEngine::count() == 4`; `tree.binary_count() == 4`; **alle 4 Baum-Blätter `lookup`-en genau eine reale
> Komposition**; Round-Trip `path == slot_path` (P→Composition Slot-Reihenfolge verlustfrei); jede Komposition
> trägt die 17-Achsen-Definition; Pfad-Mengen BR-1 == BR-2 identisch. `SearchAlgorithmAnatomy`-Instanziierung +
> observe_all gehören zu BR-3/BR-4 (hier nur AdHocComposition<17> + Definition).

### BR-3 — NodeValue → echter ObserverAggregate
**Erweitern:** `NodeValue` um einen ABI-stabilen Observer-Snapshot-Block (uint64-Slots je Achse, memcpy-fähig;
KEIN komposition-typisiertes Member → standard_layout bleibt). Der Mess-Treiber zieht je gemessenem Blatt den
realen Snapshot via `observe_all()`/`IObservableTier::tier_observe` und legt ihn im Knoten ab. Definitionen
(Wrapper-Identität/Properties) je Knoten über die `BinarySpec.axes` + die CompositionRegistry read-only abrufbar.

> ✅ **BR-3 (SearchAlgorithm-Kern) ERLEDIGT + VERIFIZIERT (2026-06-02, `test_br3_observer`, cl /DCOMDARE_MEASUREMENT_ON=1
> /DCOMDARE_CE_ENABLE_STATISTICS, RAM-Watchdog 15.9 GB frei):** `NodeValue` trägt jetzt `NodeObserverSnapshot`
> (flacher uint64-POD, umbrella-unabhängig, Layout == `ComdareTierObserverSnapshotV1`) + `observer_real`. Treiber
> `node_value_measurement.hpp`: `measure_composition<P>` instanziiert den REALEN genus-Adapter `SearchAlgorithmAbiAdapter<
> SearchAlgorithmAnatomy<CompositionFromPermTuple<P>>>`, treibt das echte Such-Organ + allocator-`ComposedStore`
> (tier_insert/tier_lookup), zieht `tier_observe` → POD → `NodeValue.observer`; `set_node_value`/`node_value` legen
> ihn in der SPARSE value_map ab (nur GEMESSENE Knoten). Literale Belege: `search_insert_count == 256` (ECHT
> getrieben, kein Stub), ungemessener Knoten == 0 (Kontrast), `measured_node_count == 1`, 17-Achsen-Definition
> read-only via CompositionRegistry. **R5.B-Grenze EHRLICH:** `observable_axis_count` macht transparent, wie viele
> Achsen real beobachtet sind (operativ search_algo + allocator; Rest passive Compile-Time-Deskriptoren → Default 0).
> **VERBLEIBT (BR-3-OBS-22, #72):** die 22-Observer-Differenzierung — die 5 außerhalb-Achsen (page_type/09b/12 =
> Definition-statt-Observer; q1/q2 = eigener Container-Gattungs-Observer) sind hier NOCH NICHT getragen.

> **AUDIT-TODO BR-3-OBS-22 (User 2026-06-02):** `ObserverAggregate<C>` (observer_aggregate.hpp) hat heute nur
> **17 Snapshot-Slots** (= die 17 Komposition-Achsen T0..T16). Die 22-Achsen-Bindung (BR-1) braucht aber **22
> Observer-Strukturen** — die **5 Achsen außerhalb** (page_type/01, simd_extension/09b, general_hardware/12,
> queuing_q1, queuing_q2) haben KEINEN Observer-Slot. Vor der Done-Marke für Gate-4 ist je außerhalb-Achse
> bewusst zu klären (nicht stillschweigend auf 17 reduzieren):
> - **Hardware-Achsen (09b simd_extension, 12 general_hardware, sowie 09 isa):** sind reine Build-Time-Konstanten
>   (Definition/Properties), evtl. KEIN Laufzeit-Observer → dann „Achsen-Definition statt Observer" je Knoten, aber
>   EXPLIZIT dokumentiert (nicht implizit weggelassen).
> - **queuing_q1/q2:** andere Gattung (Container/Queue, Doku 24 §8.8 → eigenes Prüf-Dock) → eigener Gattungs-
>   Observer-Aggregate (NICHT der SearchAlgorithm-`ObserverAggregate<17>`).
> - **page_type/01:** nodes-Sub-Achse → entweder Teil des node-Observers oder eigener Slot.
> Ergebnis: entweder ein erweiterter/parallel-gehaltener Observer je Gattung ODER eine dokumentierte
> „Definition-statt-Observer"-Klassifikation je außerhalb-Achse. Bis dahin gilt zusätzlich die R5.B-Grenze
> (Doku 21/24 §5.5: operativ misst real nur search_algo (+ allocator); die übrigen Komposition-Achsen sind
> heute passive Compile-Time-Deskriptoren) — der „volle" 17-Observer-Snapshot ist selbst noch nicht voll
> operativ. BR-3 muss BEIDE Grenzen (22-vs-17-Slots UND R5.B-Operativität) ehrlich abbilden.

### BR-4 — Generierte Binary → reale Anatomie
**Erweitern KF-8 (ceb_generator):** statt nur `#define`-Hülle emittiert `perm_<id>.cpp` jetzt
`#include <…/all_axes_umbrella.hpp>` + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ-Typnamen aus dem Pfad>)` →
eine echte ladbare Tier-Binary mit `comdare_create_anatomy` + `observe_all`. **Verifikation:** generiert → via
BuildOrchestrator (KF-16) gebaut → via `AnatomyModuleLoader` geladen → `dynamic_cast<IObservableTier*>` +
`tier_observe` über die reale Komposition liefert echte Achsen-Statistik.

## 4. Vollständigkeits-Gate (Done-Bedingung, literal)

1. `tree.binary_count() == PermutationEngine::count()` (exakt, über die 17 Komposition-Achsen).
2. ALLE 22 Achsen erscheinen als Baum-Ebene mit vollem Enabled-Inventar (17 im SearchAlgorithm-Teilbaum +
   5 in separaten Genus-/Sub-Achsen-Teilbäumen).
3. Jedes statische Blatt → reale `AdHocComposition`, als Tier-Binary baubar (BR-4).
4. Jeder gemessene Knoten → realer `ObserverAggregate`-Snapshot + Achsen-Definition (BR-3). **22 Observer-
   Strukturen, NICHT 17** (Audit-TODO BR-3-OBS-22, §3): die 5 außerhalb-Achsen (page_type/09b/12/q1/q2)
   brauchen eigene Observer bzw. eine bewusst dokumentierte „Definition-statt-Observer"-Klassifikation —
   keine stillschweigende Reduktion auf die 17 Komposition-Slots.
5. Inverse Signatur-Projektion (KF-15) über die REALEN Kompositionen.
6. Belegt hier (Doc 27) + Doc 26 + finaler Session-Doku; finaler Audit bestätigt die Gleichheit.

## 5. Reihenfolge + Risiken

BR-1 → BR-2 → BR-4 → BR-3 (Observer zuletzt, da auf reale geladene Binary gestützt). Risiken: (R1) mp_product-
Compile-Explosion bei „alle Wrapper enabled" — Enabled-Set wird durch Flags klein gehalten (Pilot: ∏≈klein);
„voll" = volles ENABLED-Inventar (Experiment-Konfig wählt). (R2) Namespace-Auflösung der Enabled-Listen →
Anker = TopicConfigSets (namespace-korrekt). (R3) Pfad-Serialisierung muss BR-1↔BR-2↔BR-4 identisch sein →
EINE zentrale `serialize_path(axis,value)`-Konvention. (R4) ABI: Observer-Block als flacher uint64-POD.

## 6. EMPIRISCHER BEFUND C1060 + aufgelöste Knoten-Klassen-Architektur (User 2026-06-02)

**C1060 (Compiler-Heap-Erschöpfung) — belegt:** das Instanziieren von `PermutationEngine<…17 Achsen…>::count()`
(= `mp_size<mp_product<PermTuple, Enabled_03a, …, Enabled_filter>>`) **sprengt den Compiler-Heap**. Der VOLLE
kartesische **Typ-Raum** über die 17 Achsen mit ihren realen Enabled-Inventaren (18×3×…×25×…) ist astronomisch
und **NICHT als Typen materialisierbar**. Das bestätigt empirisch den Kern von Doc 26: man **zählt** die
Kardinalität, materialisiert aber NICHT den Typ-Baum. Ein **voll compile-time Baum** (type-level über alle
Achsen) ist damit nachweislich **infeasible**.

**Konsequenz für Gate-1 (Mess-Methode korrigiert):** `tree.binary_count()` wird gegen
`composition_binary_count() = ∏ mp_size(Enabled_i)` geprüft (constexpr, OHNE `mp_product`-Materialisierung).
Das IST der Wert von `PermutationEngine::count()` per Kardinalitäts-Identität `mp_size<mp_product<L…>> = ∏|L|`
— literal belegbar, ohne den infeasiblen Typ-Baum zu bauen.

**Aufgelöste Knoten-Klassen-Architektur (User-Entscheidung „Compile-time Deskriptor-Typen + Laufzeit-Baum"):**
- **Per-Achse COMPILE-TIME Spezial-Deskriptor-Klassen** (CRTP, typsicher, KEIN struct): Hierarchie
  Head-Concept `AxisNodeDescriptor` → `StaticAxisDescriptorBase<D>` / `DynamicAxisDescriptorBase<D>` →
  per-Achse-SPEZIAL (`AllocatorAxisDescriptor`, `ConcurrencyDynamicDescriptor`, …), je mit den achsen-eigenen
  typsicheren Properties (Doc 26 §4 „nicht generisch-flach"). Registrier-/erweiterbar (mp_list-Registry).
- **Laufzeit-Baum-Manager** hält LEICHTE Knoten (`StaticAxisNode`/`DynamicVariableNode`, bereits da), die per
  `block_id`/Achsen-Name auf den compile-time Deskriptor **zurück-verweisen** (Bidirektionalität). Der Baum
  zählt/strukturiert den riesigen Raum zur LAUFZEIT (ohne Typ-Materialisierung); statische und dynamische
  Knoten sind GLEICHRANGIG.
- **Nur EIN Blatt = compile-time:** je zu bauender Binary wird genau EINE `AdHocComposition<17>` /
  `SearchAlgorithmAnatomy` materialisiert (on-demand, BR-2/BR-4) — nie der ganze Baum.
- Bidirektionalität: Laufzeit-Knoten → `block_id` → compile-time Deskriptor (typsichere Properties read-only
  abrufbar); Deskriptor-Registry → flache AxisLevels/DynamicDims (BR-1). So vereint: Doc-26-§4-Typsicherheit
  + Materialisierungs-Grenze.

**Verkettungs-Formalik (User 2026-06-02 — Block = typsicherer Teilbaum-Einheit):** Ein `AxisBlock` ist formal
EINE Einheit = ein eigener, typsicherer **C++-Deskriptor-Teilbaum** (CRTP-Subklasse: statische Achse + ihre
Sub-Achsen als TYPISIERTE Descriptor-Knoten). Die **Wurzel** der statischen node eines Blocks wird **an das
ENDE (Tail) der vorangegangenen Achse** angehängt — NICHT an ein Blatt (Blätter = finale Experiment-Einstellungen;
Zwischen-Achsen verketten sich Wurzel-an-Tail). Die Block-Teilbäume fügen sich so zum EINEN zusammenhängenden
Gesamtbaum (= die Achsen-Verkettung in `build_recursive`: jede Achsen-Ebene = Kinder der vorigen). **Typsicherheit
sitzt auf der Block-Ebene** (jeder Block ist ein kleiner, voll type-checkter Deskriptor-Teilbaum, materialisierbar);
die **Gesamtbaum-Assemblierung** der Block-Teilbäume ist LAUFZEIT (Wurzel-an-Tail-Verkettung), da das volle
Typ-Produkt C1060-infeasible ist. Hauptzweck: Typsicherheit der Deskriptoren je Achse.
