# 27 — Experiment-B+-Baum: vollständige Registry-Bindung (die 4 Brücken) (2026-06-02)

> ⚠️ **KORREKTUR (2026-06-03, maßgeblich: `30_audit_achsen_delegation_pflichtachsen.md` §8):** Die in §0.1 eingeführte
> Einordnung „queuing q1/q2 = EIGENE Container-Gattung" ist ein **KATEGORIENFEHLER** — queuing ist eine **Achse/ein
> Organ**, KEINE Gattung. Doku 14 §7 („q1/q2 = Organe") war KORREKT; §0.1 ist die Fehl-Reinterpretation (§32 Cross-Genus
> zweckentfremdet). Korrektes Modell: queuing = reguläre, mandatorische SearchAlgorithm-Achse (`NoBuffer` = durchreichender
> Algorithmus, kein „optional"); die Adapter-Gattung ist eine echte Container-Datenstruktur (axis_inner + ordering),
> nutzt queuing NICHT. Alle „queuing = Container-Gattung"-Aussagen unten sind hierdurch revidiert.

> ⚠️ **KORREKTUR — VERBINDLICHES 3-EBENEN-MODELL (korr. 2026-06-03, s. Doc 30 §8.0; Doc 24 §8.8 + Doku 14 §25):**
> In diesem Dokument bezeichnet „Gattung" an vielen Stellen fälschlich die Ebene mit dem FESTEN Achsen-Satz (z.B.
> „SearchAlgorithm-Gattung", „SearchAlgorithm-Gattungs-Komposition", „Gattungs-Invariante", „derselben Gattung",
> „Adapter-Gattung", „2 Gattungen"). Verbindlich gelten DREI getrennte Ebenen:
> 1. **GATTUNG = ein INTERFACE für die Außenwelt (= ein Prüf-Dock)** — es gibt nur **3**: **SearchAlgorithm / Container / Graph**.
>    (Doc 24 §8.8 „Prüf-Dock je Gattung — für Search Algorithm oder Container oder Graphen"; §8.6 „ABI-Interface der API der Gattung".)
> 2. **TIER-UNTERKLASSE = liegt UNTER dem Gattungs-Interface und verwendet einen FESTEN Achsen-Satz.** HIER lebt die
>    feste Achsen-Konfiguration — NICHT auf der Gattungs-Ebene. Die 17/19-Achsen-Komposition (`AdHocComposition<17>`,
>    std::map-ähnlich) ist die **SearchAlgorithm-Tier-Unterklasse** (die einzige bisher gebaute), UNTER dem SearchAlgorithm-
>    **Interface**. Set/Sequence/Adapter/View sind **Tier-Unterklassen UNTER dem Container-Interface**.
> 3. **ACHSEN = Organe der Tier-Unterklasse**; KEINE ist optional — ein nicht-pufferndes/nicht-prefetchendes Tier wählt
>    einen KONKRETEN Durchreich-Algorithmus (`NoBuffer`/`NoFlush`/`NonePrefetch`/`NoMigration`/`None`), NICHT „Achse weglassen".
> Folge: queuing q1/q2 = **Pflicht-Achsen der SearchAlgorithm-Tier-Unterklasse** (kein Interface, keine Gattung). Wo unten
> „SearchAlgorithm-Gattung" die 17/19-Achsen-Komposition meint, ist die **SearchAlgorithm-Tier-Unterklasse** gemeint; wo
> „Adapter-Gattung" steht, ist eine **Tier-Unterklasse der Container-Gattung** gemeint. Wo „Gattung" das Außen-Interface
> bzw. „Prüf-Dock je Gattung" meint (z.B. Doc 24 §8.8), ist es KORREKT und bleibt. Die feste Slot-Zahl (`AdHocComposition<17>`
> als ABI-Identität) bleibt als **Tier-Unterklassen-Invariante** korrekt.

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

**17 Achsen = AdHocComposition-Slots T0..T16** (composition_factory.hpp:41-66, feste Reihenfolge) → bilden den
Kern des SearchAlgorithm-Tier-Binary (= der SearchAlgorithm-**Tier-Unterklasse**, korr. 2026-06-03, s. Doc 30 §8.0).
**5 Achsen außerhalb dieser 17 Slots** (page_type, 09b, 12, q1, q2)
gliedern sich (korr. 2026-06-03, s. Doc 30 §8.0) so: **q1/q2 (queuing) sind reguläre, mandatorische
SearchAlgorithm-Achsen** (Organe DERSELBEN Tier-Unterklasse; ein nicht-pufferndes Tier wählt `NoBuffer`/`NoFlush`, einen
durchreichenden Algorithmus — analog NonePrefetch/NoMigration) — also zusammen mit den 17 Slots → **19
SearchAlgorithm-Achsen**; **page_type/09b/12 = 3 Build-Achsen** (Codegen-/Build-Varianten der DERSELBEN
SearchAlgorithm-Binary). queuing ist KEINE eigene Gattung (und kein Interface) — die Gattungen sind die 3 Außen-
Interfaces SearchAlgorithm/Container/Graph, vgl. Top-Banner.

### 0.1 AUTORITATIV: „22" ist die Gesamtzahl, „17" ist NUR die SearchAlgorithm-Tier-Unterklasse (korr. 2026-06-03 — vorher „…SearchAlgorithm-Gattung", s. Doc 30 §8.0) (User 2026-06-02)

> **Kritische Klarstellung (User 2026-06-02): die „17" darf die „22" NICHT wegschrumpfen.** „17" ist KEINE
> Achsen-Gesamtzahl, sondern die **Slot-Zahl der SearchAlgorithm-Tier-Unterklassen-Komposition** (`AdHocComposition<17>`,
> `ObserverAggregate<17>`, `sizeof...(Vs)==17` — eine **Tier-Unterklassen-Invariante** [korr. 2026-06-03, vorher
> „GATTUNGS-Invariante", s. Doc 30 §8.0 — die Gattung ist das Außen-Interface, nicht der feste Achsen-Satz], bleibt). Wo Doku „15 Topics / 17 Achsen"
> o.ä. als Bibliotheks-GESAMTZAHL schreibt, ist das FALSCH → **22 Achsen / 15 Topics**. Verifiziert 2026-06-02:
> 22 `axis_*_registry.hpp` (Liste in Session-Doku); `test_br1_full22_count` bindet alle 22 als Baum-Ebene.

**Differenzierte Bindung der 22 (User-Entscheidung „gattungs-korrekt", 2026-06-02) — KEINE Achse schrumpft weg:**

| Gruppe | Achsen | Bindung an Binary/Messung | Observer |
|--------|--------|----------------------------|----------|
| **SearchAlgorithm-Komposition (17)** | T0..T16 (search_algo … filter) | `AdHocComposition<17>` = Kern des SearchAlgorithm-Tier-Binary (Tier-Unterklassen-Invariante; korr. 2026-06-03 vorher „Gattungs-Invariante", s. Doc 30 §8.0) | `ObserverAggregate<17>` |
| **SearchAlgorithm-Achsen queuing (2)** | queuing q1 + q2 | reguläre, **mandatorische SearchAlgorithm-Achsen** (Organe DERSELBEN Tier-Unterklasse — korr. 2026-06-03 vorher „derselben Gattung", s. Doc 30 §8.0); jedes Tier treibt ihr Interface — ein nicht-pufferndes wählt `NoBuffer`/`NoFlush` (durchreichender Algorithmus, analog NonePrefetch/NoMigration); zusammen mit den 17 Slots → **19 SearchAlgorithm-Achsen** | je eigener Achsen-Observer/Definition |
| **SearchAlgorithm-Build-Varianten (3)** | page_type/01, simd_extension/09b + general_hardware/12 | modifizieren DIESELBE SearchAlgorithm-Binary (zusätzliche Baum-Ebene + Build-/Codegen-Variante je Wert) — KEINE eigene Gattung | je eigener Observer/Definition (BR-3-OBS-22) |

> ✅ **Gattungs-Generik ERLEDIGT (2026-06-02, User-Option-B Schritt 2+3):** Die Bau-Brücke ist jetzt
> gattungs-PARAMETRISCH (`genus_binding_traits.hpp` `GenusBindingTraits<G>`): SearchAlgorithm = verifizierter
> Spezialfall (17 Slots, `test_genus_binding`), **die Adapter-Tier-Unterklasse als 2. Instanz** (korr. 2026-06-03
> vorher „Adapter-Gattung", s. Doc 30 §8.0 — Adapter ist eine **Tier-Unterklasse UNTER der Container-Gattung/dem
> Container-Interface**, keine eigene Gattung) (`container_anatomy.hpp`,
> genus()==Adapter; `test_container_genus`: real getriebener Inner-Container mit put/get/size, EIGENER
> Adapter-Observer put_count/get_count/peak_occupancy, GenusBound<Adapter>==true).
> ⚠️ **(korr. 2026-06-03, s. Doc 30 §8.0):** Die Adapter-Tier-Unterklasse ist eine **echte Container-Datenstruktur**
> (`std::queue/stack/priority_queue`) = `axis_inner` (Inner-Container) + ordering/discipline (FIFO/LIFO/Priority) +
> delegierte Standard-Achsen — sie nutzt die queuing-Achsen (q1/q2) NICHT. Die in der ursprünglichen Fassung als
> Adapter-Instanz herangezogene `ContainerComposition<Q1>` (queuing-getrieben) war Ausdruck des Kategorienfehlers
> (queuing ≠ Gattung); q1/q2 bleiben reguläre SearchAlgorithm-Achsen.
> Der EINE Experiment-Baum baut damit Tier-Unterklassen unter 2 Gattungs-Interfaces (korr. 2026-06-03 vorher
> „2 Gattungen", s. Doc 30 §8.0: SearchAlgorithm-Tier-Unterklasse unter dem SearchAlgorithm-Interface, Adapter-Tier-
> Unterklasse unter dem Container-Interface; Cross-Genus type-getrennt, Doku 14 §32). Folge: Container-/Adapter-
> Prüf-Dock (dünner ABI-Wrapper analog SearchAlgorithmDock), priority-Ordering als 2. Adapter-Variante, Graph-Gattung.

**Konsequenz:** Alle **22** erscheinen als volle Baum-Ebene (BR-1 ✓, registry-getrieben). Die 17 SearchAlgorithm-
Slots + die 2 queuing-Achsen (q1/q2) + die 3 Build-Achsen binden alle an die SearchAlgorithm-Tier-Binary (= die
SearchAlgorithm-Tier-Unterklasse); q1/q2
sind reguläre SearchAlgorithm-Achsen (KEINE eigene Gattung, kein eigenes Interface — korr. 2026-06-03, s. Doc 30 §8.0). JEDE der 22 trägt
einen eigenen Observer + eigene Definition (kein Wegschrumpfen). Die
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
außerhalb der 17 AdHoc-Slots werden als eigene Levels reflektiert (korr. 2026-06-03, s. Doc 30 §8.0): q1/q2 =
reguläre SearchAlgorithm-Achsen (gehören zur SearchAlgorithm-Tier-Unterklasse [korr. 2026-06-03 vorher
„SearchAlgorithm-Gattung", s. Doc 30 §8.0], KEIN getrennter Genus-Teilbaum),
page_type/09b/12 = Build-/Codegen-Varianten derselben SearchAlgorithm-Binary.

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
> ✅ **BR-3-OBS-22 ERLEDIGT + VERIFIZIERT (2026-06-02, `test_br3_obs22`, Voll-22-Include, RAM-Watchdog 11.7 GB):**
> `axis_observer_classification.hpp` klassifiziert ALLE 22 Achsen (kein Wegschrumpfen): **19
> SearchAlgorithmObserver** (die 17 ObserverAggregate<17>-Slots BR-3 real + die 2 queuing-Achsen q1/q2 als reguläre
> SearchAlgorithm-Achsen-Observer — korr. 2026-06-03, s. Doc 30 §8.0: q1/q2 sind Organe DERSELBEN Tier-Unterklasse
> [vorher „derselben Gattung"], KEINE Container-Gattung und keine eigene Gattung) + **3 DefinitionOnly** (page_type/09b/12 = Build-Konstanten → read-only Definition statt
> Laufzeit-Observer, EHRLICH) = 22. Literal belegt gegen die ECHTEN
> 22 Achsen (BR-1 `build_all_axis_levels`): jede der 22 ist observer-klassifiziert UND trägt ihre read-only
> Definition (reale Wrapper-Namen via reflect_names); Summe 19+3==22 (keine doppelt/fehlend).

> **AUDIT-TODO BR-3-OBS-22 (User 2026-06-02):** `ObserverAggregate<C>` (observer_aggregate.hpp) hat heute nur
> **17 Snapshot-Slots** (= die 17 Komposition-Achsen T0..T16). Die 22-Achsen-Bindung (BR-1) braucht aber **22
> Observer-Strukturen** — die **5 Achsen außerhalb** (page_type/01, simd_extension/09b, general_hardware/12,
> queuing_q1, queuing_q2) haben KEINEN Observer-Slot. Vor der Done-Marke für Gate-4 ist je außerhalb-Achse
> bewusst zu klären (nicht stillschweigend auf 17 reduzieren):
> - **Hardware-Achsen (09b simd_extension, 12 general_hardware, sowie 09 isa):** sind reine Build-Time-Konstanten
>   (Definition/Properties), evtl. KEIN Laufzeit-Observer → dann „Achsen-Definition statt Observer" je Knoten, aber
>   EXPLIZIT dokumentiert (nicht implizit weggelassen).
> - **queuing_q1/q2:** reguläre, mandatorische SearchAlgorithm-Achsen (Organe DERSELBEN Tier-Unterklasse [korr.
>   2026-06-03 vorher „derselben Gattung", s. Doc 30 §8.0]; `NoBuffer`/`NoFlush`
>   = durchreichender Algorithmus bei nicht-pufferndem Tier) → SearchAlgorithm-Achsen-Observer; der
>   `ObserverAggregate` wächst hier von 17 auf 19 Slots (korr. 2026-06-03, s. Doc 30 §8.0 — KEINE eigene Gattung,
>   KEIN getrenntes Gattungs-Observer-Aggregate; die echte Adapter-Tier-Unterklasse [vorher „Adapter-Gattung";
>   eine Tier-Unterklasse unter dem Container-Interface, keine eigene Gattung] = `std::queue/stack/priority_queue`
>   = axis_inner + ordering, nutzt queuing NICHT).
> - **page_type/01:** Build-Achse (Codegen-Variante der SearchAlgorithm-Binary, korr. 2026-06-03 s. Doc 30 §8.0) →
>   entweder Teil des node-Observers oder eigener Slot bzw. „Definition-statt-Observer".
> Ergebnis (korr. 2026-06-03, s. Doc 30 §8.0): der SearchAlgorithm-`ObserverAggregate` wird um die queuing-Achsen
> q1/q2 auf 19 Slots erweitert (alle 19 Achsen DERSELBEN Tier-Unterklasse [vorher „derselben Gattung"]), und für die 3 Build-Achsen (page_type/09b/12) gilt eine
> dokumentierte „Definition-statt-Observer"-Klassifikation. Bis dahin gilt zusätzlich die R5.B-Grenze
> (Doku 21/24 §5.5: operativ misst real nur search_algo (+ allocator); die übrigen Komposition-Achsen sind
> heute passive Compile-Time-Deskriptoren) — der „volle" 17-Observer-Snapshot ist selbst noch nicht voll
> operativ. BR-3 muss BEIDE Grenzen (22-vs-17-Slots UND R5.B-Operativität) ehrlich abbilden.

### BR-4 — Generierte Binary → reale Anatomie
**Erweitern KF-8 (ceb_generator):** statt nur `#define`-Hülle emittiert `perm_<id>.cpp` jetzt
`#include <…/all_axes_umbrella.hpp>` + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ-Typnamen aus dem Pfad>)` →
eine echte ladbare Tier-Binary mit `comdare_create_anatomy` + `observe_all`. **Verifikation:** generiert → via
BuildOrchestrator (KF-16) gebaut → via `AnatomyModuleLoader` geladen → `dynamic_cast<IObservableTier*>` +
`tier_observe` über die reale Komposition liefert echte Achsen-Statistik.

> ✅ **BR-4 ERLEDIGT + VERIFIZIERT (2026-06-02, 3-Phasen-Round-Trip `br4_emit`/`br4_load`, RAM-Watchdog 14.3 GB frei):**
> - **Phase 1 (emit):** aus EINEM Baum-Blatt (reale Pilot-Komposition via `for_each_permutation`→`CompositionFromPermTuple<P>`)
>   schreibt `render_adhoc_module_source(0, adhoc_macro_args<Comp>())` (builder/codegen/adhoc_emitter.hpp) die ECHTE
>   Anatomie-Quelle: `#include all_axes_umbrella.hpp` + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ-Typnamen via type_name>)`.
>   Pfad-Round-Trip `serialize_composition_path<P>() == serialize_composition_from_slots<Comp>()` (BR-1↔BR-2-Identität).
> - **Phase 2 (DLL):** `cl /LD /DCOMDARE_ANATOMY_MODULE_BUILD /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS`
>   baut die reale Anatomie als SHARED-DLL (kein OOM).
> - **Phase 3 (load+observe):** `AnatomyModuleLoader::load` → `status_ok`; `composition_name=="AdHocComposition"`,
>   `organ_count()==17`, `genus()==SearchAlgorithm`; `dynamic_cast<IObservableTier*>` → `tier_insert`×256 → `tier_observe`
>   liefert über die REALE DLL-Grenze echte Werte: **search_insert=256, lookup=256, fill=256, observable_axes=4** (R5.B).
>   Der Baum-Blatt-Pfad (`search_algo=k_ary/.../filter=filter_bloom`) IST die Identität der gebauten Binary.
>
> Anmerkung: der reale Anatomie-Emit läuft über den compile-time `render_adhoc_module_source<C>` (type_name → FQ-Typen);
> der string-getriebene `ceb_generator::generate_perm_source` bleibt die leichte `#define`-Hülle für die Build-Orchestrierung
> ohne Typ-Auflösung — die ECHTE ladbare Anatomie kommt über den Komposition-typisierten Emit (BR-2 ordnet Blatt↔Komposition zu).

## 4. Vollständigkeits-Gate (Done-Bedingung, literal)

1. `tree.binary_count() == PermutationEngine::count()` (exakt, über die 17 Komposition-Achsen).
2. ALLE 22 Achsen erscheinen als Baum-Ebene mit vollem Enabled-Inventar: 19 SearchAlgorithm-Achsen (17 AdHoc-Slots
   + queuing q1/q2 als reguläre Achsen DERSELBEN Tier-Unterklasse [korr. 2026-06-03 vorher „derselben Gattung", s.
   Doc 30 §8.0]) + 3 Build-Achsen (page_type/09b/12) als Codegen-Varianten
   derselben SearchAlgorithm-Binary — KEIN getrennter Genus-Teilbaum für queuing (korr. 2026-06-03, s. Doc 30 §8.0).
3. Jedes statische Blatt → reale `AdHocComposition`, als Tier-Binary baubar (BR-4).
4. Jeder gemessene Knoten → realer `ObserverAggregate`-Snapshot + Achsen-Definition (BR-3). **22 Observer-
   Strukturen, NICHT 17** (Audit-TODO BR-3-OBS-22, §3, korr. 2026-06-03, s. Doc 30 §8.0): die queuing-Achsen q1/q2
   sind reguläre SearchAlgorithm-Achsen (Organe DERSELBEN Tier-Unterklasse) und erweitern den `ObserverAggregate`
   von 17 auf 19 Slots (KEINE eigene Gattung, kein eigenes Interface); die 3 Build-Achsen (page_type/09b/12) tragen eine bewusst dokumentierte „Definition-statt-Observer"-
   Klassifikation — keine stillschweigende Reduktion auf die 17 Komposition-Slots.
5. Inverse Signatur-Projektion (KF-15) über die REALEN Kompositionen. ✅ **VERIFIZIERT (2026-06-02,
   `test_br_kf15_real`):** Baum aus ECHTEN Enabled-Listen (search_algo gepinnt = Paper-Signatur + 3 freigegebene
   Achsen → 8 Blätter); `ReadOnlyResultView` projiziert alle 8 realen Komposition-Pfade auf die gepinnte
   Signatur (registry-getrieben, `search_algo=<realer Wrapper>`); `aggregate_for_signature` sieht den echten
   gesetzten NodeValue (BR-3). Lineare Signatur-Filter-Projektion (Doc 26 §3), KEINE Fingerprints.
6. Belegt hier (Doc 27) + Doc 26 + finaler Session-Doku; finaler Audit bestätigt die Gleichheit. ✅ **ERLEDIGT
   (2026-06-02):** finaler Audit `docs/sessions/20260602-experiment-baum-4-bruecken-final-audit.md` konsolidiert
   die literalen Test-Belege + Commit-Refs aller 6 Gates. **Alle 6 Gates literal grün** (Gate-1 Gleichheit
   `binary_count==∏ mp_size==137.594.142.720.000` belegt). Verbleibend (NICHT in den 6 Gates): Gattungs-Generik
   (Bau-Brücke der echten Adapter-Tier-Unterklasse [korr. 2026-06-03 vorher „Adapter-Gattung" — Tier-Unterklasse
   unter dem Container-Interface, s. Doc 30 §8.0] = `std::queue/stack/priority_queue` via axis_inner + ordering — NICHT
   queuing-getrieben) + #73 provision_all-Batching.

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
