# 28 — Vollständigkeits-Kartographie: SOLL-Zustand für ALLE 22 Achsen + 5 Tier-Unterklassen + 4 Brücken + Skalierung (2026-06-02)

> ⚠️ **KORREKTUR (korr. 2026-06-03, s. Doc 30 §8.0):** Im gesamten Dokument bezeichnet „Gattung" an vielen Stellen
> fälschlich die Ebene mit dem FESTEN Achsen-Satz (z.B. „5 Gattungen", „SearchAlgorithm-Gattung", „Adapter-Gattung",
> „Set/Sequence/View-Gattung"). Verbindliches 3-Ebenen-Modell (Doc 30 §8.0; Doc 24 §8.8 + Doku 14 §25):
> (1) **GATTUNG = ein INTERFACE für die Außenwelt (= ein Prüf-Dock)** — es gibt nur **3**: **SearchAlgorithm / Container / Graph**.
> (2) **TIER-UNTERKLASSE = liegt UNTER dem Gattungs-Interface und verwendet einen FESTEN Achsen-Satz.** SearchAlgorithm (std::map-ähnlich, 17/19-Achsen) ist die einzige bisher gebaute Tier-Unterklasse (unter dem SearchAlgorithm-**Interface**); Set/Sequence/Adapter/View sind Tier-Unterklassen UNTER dem **Container-Interface**.
> (3) **ACHSEN = Organe der Tier-Unterklasse**, keine ist optional (Durchreich-Algorithmus statt Weglassen).
> Daher: „5 Gattungen" = **5 Tier-Unterklassen** (verteilt auf 2 Interfaces SearchAlgorithm + Container); die feste Slot-/Achsen-Zahl pro Zeile lebt auf der Tier-Unterklassen-Ebene. Wo „Gattung" das **Außen-Interface/Prüf-Dock** meint (z.B. „Prüf-Dock je Gattung", „Interface der Gattung", „ABI-Interface der API der Gattung"), ist es KORREKT und bleibt. Die Gattungs-Invariante (feste Slot-Zahl, `AdHocComposition<17>` als ABI-Identität) bleibt sachlich richtig — sie ist eine **Tier-Unterklassen**-Invariante.

> ⚠️ **KORREKTUR (2026-06-03, maßgeblich: `30_audit_achsen_delegation_pflichtachsen.md` §8):** Die Einordnung der Achsen
> 21/22 (axis_q1/q2_queuing) als „EIGENE Container-Gattung (Adapter), Slot 1/2" (Tabelle §1 + §2 + §67) ist ein
> **KATEGORIENFEHLER** — queuing ist eine **Achse**, keine Gattung. **§67 ist FALSCH herum:** Doku 14 §7 („q1/q2 =
> stabile Organe") war KORREKT; nicht Doku 14, sondern Doc 27 §0.1 ist die (revidierte) Fehl-„Präzisierung". Korrektes
> Modell: queuing = reguläre mandatorische SearchAlgorithm-Achse; Adapter-Gattung = echte Container-Datenstruktur
> (axis_inner + ordering), NICHT queuing. „22" = 19 SA-Achsen (inkl. queuing) + 3 Build-Achsen, KEINE „2 Container-Gattungs-Slots".

> **Korrektur 2026-06-03 (Doc 30 §8.1):** Die in den Notizen oben + im Body genannte Formel „axis_inner + **ordering**
> (FIFO/LIFO/Priority)" für die Adapter-Tier-Unterklasse war noch teilweise GERATEN und ist hiermit VERWORFEN. Autoritativ
> (Doku 14 **§28 Invertebrate** + **§26.4**, in Doc 30 §8.1 umgesetzt+verifiziert): die Adapter-Tier-Unterklasse hat
> **KEINE „ordering"-Achse**, sondern **13 Achsen** = 9 delegiert (search_algo, cache_traversal, memory_layout, allocator,
> prefetch, concurrency, isa, io_dispatch, migration_policy) + 3 aktiv (serialization, telemetry, value_handle) +
> **1 spezifisch `inner_container`** (NEU axis_inner, die EINZIGE Adapter-spezifische Achse). Die **Disziplin FIFO/LIFO ist
> API-Nutzung** (push/pop/top vs. front/back, §26.4), KEINE Achse. Code-Realisierung (Doc 30 §8.1, real+verifiziert):
> `AdapterComposition<T0..T11, Inner>` + `AdapterAnatomy` + `AdapterObserverSnapshot`; die Typen/Dateien heißen seit dem
> Rename 2026-06-03 **`Adapter*`/`adapter_*`** (vorher `Container*`/`container_*`) — `AnatomyGenus`/`AnatomyGattung::Container`
> bleiben. Wo unten „axis_inner + ordering", „Eigene Achsen: axis_inner, ordering" oder `Container*`-Typnamen als AKTUELLE
> Architektur stehen, gilt stattdessen das §8.1-Modell.

> **Goal-V6 Phase A (Kartographie).** Dieses Dokument konsolidiert den SOLL-Zustand des Permutations-B+-Experiment-
> Baums aus den letzten 18 Architektur-Dokumentationen — gefunden, kartografiert, konsolidiert (NICHT neu erfunden).
> Quelle je Aussage: Doc:§ bzw. Datei:Zeile. Provenienz via 3 paralleler adversarial-präziser Kartographie-Agenten
> (ad722f5 Achsen · afab8aa Gattungen · ab233e3 Brücken/Skalierung), je gegen Doku + realen Code gegengeprüft.
> Phase B (Lücken-Ledger) gleicht dies gegen den IST-Code ab; Phase C (Planungssession) verabschiedet den Umsetzungsplan.

## §0 Provenienz + Achsen-/Gattungs-Grundgesetz

- **Doku-Orte:** `docs/architecture/15..27` + `messarchitektur_v5_*` + `abhaengigkeitskette_*` liegen im **cache-engine-Repo**. **Doku 11/14** (Achsen-vs-Strategien / Organ-Metapher + Gattungen) liegen im **Diplomarbeit-Hauptrepo** `docs/architektur/14_achsen_komposition_organ_metapher.md` (§25-§40 Gattungen/Viren).
- **Autoritative Gesamtzahl (Doc 27 §0/§0.1):** **15 Topics / 22 Achsen** (frühere „18" verworfen). „17" = NUR die SearchAlgorithm-Komposition (`AdHocComposition<17>`/`ObserverAggregate<17>`, Invariante der SearchAlgorithm-**Tier-Unterklasse** — korr. 2026-06-03, s. Doc 30 §8.0: feste Achsen-Zahl lebt auf der Tier-Unterklassen-Ebene, NICHT auf der Gattungs-/Interface-Ebene) — NIEMALS als Gesamtzahl missverstehen (kein Wegschrumpfen).
- **Achsen-Set-Constraint (vormals „Gattungs-Constraint", Doku 14 §32; korr. 2026-06-03, s. Doc 30 §8.0):** Joins zwischen disjunkten Tier-Unterklassen sind type-system-mathematisch unmöglich (disjunkte Achsen-Sets, inkompatible Concepts). Jede **Tier-Unterklasse** = eigene Komposition/Anatomie/ABI-Adapter/Observer + eigene `PermutationEngine`-Subklasse; das **Prüf-Dock + ABI-Interface der API** liegt auf der **Gattungs-(Interface-)Ebene** (SearchAlgorithm/Container/Graph), Doku 14 §29.2/§32.6.

## §1 Die 22 Achsen (15 Topics) — SOLL je Achse

Spalten: # · axis-id · Topic · Enabled-Liste · AdHoc-Slot · Pflicht-API (Concept) · Observer-SOLL · Build/Definition-SOLL.
Quelle: Doc 27 §0, Doc 15-18/22/23, Doku 14, die 22 `axis_*_registry.hpp` + Concept-Header + `observer_aggregate.hpp`/`axis_observer_classification.hpp`.

**17 SearchAlgorithm-Komposition-Slots (T0..T16, composition_factory.hpp:41-66):**

| # | axis-id | Topic | Enabled-Liste | Slot | Pflicht-API | Observer-SOLL |
|---|---------|-------|---------------|------|-------------|---------------|
| 1 | axis_03a_search_algo | traversal | EnabledStrategies | T0 | SearchAlgoVariant: insert/lookup/erase/occupied_count/density_percent/clear | **ObservableAxis** (SearchAlgoStatistics: insert/lookup/hit/miss/peak); R5.B real |
| 2 | axis_03b_cache_traversal | traversal | EnabledStrategies | T1 | CacheTraversalVariant + uniform | observable-fähig |
| 3 | axis_03m_mapping | traversal | EnabledStrategies | T2 | MappingVariant (+PoolRebasable) | snapshot vorhanden |
| 4 | axis_02_path_compression | nodes | EnabledCompressions | T3 | PathCompressionStrategy + uniform | passiv (Default 0) |
| 5 | axis_04_node_type | nodes | EnabledNodeTypes | T4 | uniform; reales Storage-Organ (ComposedStore) | node-Observer; KF-6 Run-Body-Divergenz |
| 6 | axis_05_memory_layout | memory_layout | EnabledLayouts | T5 | MemoryLayoutStrategy: cache_line_size(); scan_field_sum() | runtime-operativ, ABER wall-clock-sub-noise → PMC nötig (R5.D) |
| 7 | axis_06_allocator | allocator | EnabledVendors | T6 | AllocatorStrategy: allocate/deallocate/== + 6 Sub-Concepts | **ObservableAxis** real (AllocationStatistics); **Goldstandard** |
| 8 | axis_07_prefetch | prefetch | EnabledPrefetchers | T7 | uniform | passiv |
| 9 | axis_08_concurrency | concurrency | EnabledStrategies | T8 | achsenspez. + uniform | passiv |
| 10 | axis_10_serialization | serialization | EnabledSerializers | T9 | uniform; serialize_scan() | runtime-operativ |
| 11 | axis_11_telemetry | telemetry | EnabledTelemetries | T10 | uniform (DensityTracker/InsertCounter/LatencyHistogram/LeafOnlyCounter) | statistics-tragend |
| 12 | axis_14_value_handle | value_handle | EnabledHandles | T11 | uniform | passiv |
| 13 | axis_09_isa | hardware | EnabledIsas | T12 | uniform (Aarch64/Amd64/PowerPc/RiscV) | Definition (Build-Konstante) |
| 14 | axis_01_index_organization | search_engine | EnabledOrganizations | T13 | uniform | passiv |
| 15 | axis_io | io | EnabledIos | T14 | IoStrategy: is_in_memory_only() | passiv |
| 16 | axis_migration | migration | EnabledMigrations | T15 | uniform | passiv |
| 17 | axis_filter | filter | EnabledFilters | T16 | achsenspez. (Bloom/Cuckoo/Xor/RangeSurf) | passiv |

**5 Achsen außerhalb der 17 composition_factory-Slots (T0..T16) — alle weiterhin Achsen der SearchAlgorithm-Tier-Unterklasse:** 3 Build-Achsen (page_type/09b/12 = Build-/Definition-Variante) + 2 reguläre mandatorische SearchAlgorithm-Achsen (q1/q2_queuing). Zusammen mit den 17 Slot-Achsen = **19 SearchAlgorithm-Achsen** (+ 3 Build-Achsen = 22). Weder eigene Gattung (Interface) noch eigene Tier-Unterklasse (korr. 2026-06-03, s. Doc 30 §8.0).

| # | axis-id | Topic | Enabled-Liste | Pflicht-API | Observer-SOLL | Build/Definition-SOLL |
|---|---------|-------|---------------|-------------|---------------|------------------------|
| 18 | axis_01_page_type | nodes | EnabledPageTypes | PageTypeStrategy: page_kind() (6 Pflichttypen DenseByte/ExtendedDense/SparsePatricia/Redirect/CustomCache/BPlus), is_branch/is_leaf | eigener Observer SOLL (#74); heute nur DefinitionOnly | **Build-/Codegen-Variante** DERSELBEN SearchAlgorithm-Binary (nodes-Sub) |
| 19 | axis_09b_simd_extension | hardware | EnabledExtensions | uniform + provides_sse/avx/avx512*() (15+ Sub-Flags), units_per_socket(), accessible_from_efficiency_cores() | Definition (Build-Konstante) | **Build-/Codegen-Variante**; Schichten-Modell+Sub-Flags = Backlog (Doc 15 §4-5) |
| 20 | axis_12_general_hardware | hardware | EnabledPlatforms | uniform (X86_64/Aarch64/Generic HW-Profile) | Definition (Build-Konstante) | **Build-/Codegen-Variante**; Cross-Constraint mit 09/09b (96→~25, R5.C.3) |
| 21 | axis_q1_queuing | queuing | EnabledStrategies | BufferStrategy: put/get/emplace/peek_front/peek_back/size/is_empty/clear | **ObservableAxis** (put/get/peak_occupancy) | **reguläre mandatorische Achse der SearchAlgorithm-Tier-Unterklasse** (queuing-Organ; nicht-pufferndes Tier wählt `NoBuffer`, durchreichend); Achse — weder Gattung noch Tier-Unterklasse |
| 22 | axis_q2_queuing | queuing | EnabledPolicies | FlushPolicy: should_flush()→{NoFlush/Partial/Full}, on_flush_complete() | **ObservableAxis** | **reguläre mandatorische Achse der SearchAlgorithm-Tier-Unterklasse** (queuing-Organ; nicht-flushendes Tier wählt `NoFlush`, durchreichend); Achse — weder Gattung noch Tier-Unterklasse (#75) |

**is_original-Linking-Klassen (Doc 17 §4.5):** A (echtes Linking) = allocator/search_algo/q1_queuing (20 eligible; 6 Allocators aktiv: mimalloc/jemalloc/snmalloc/rpmalloc/dlmalloc/lrmalloc; Original*SearchAlgo = s2-Stubs, echtes Linking SOLL s4). B-E (Re-Impl + is_original=false ehrlich): prefetch/value_handle/memory_layout/index_organization/telemetry/migration/io. axis_06 separat in `ext/allocator/A01-A23`.

## §2 Die 5 Tier-Unterklassen (+ Viren) — SOLL je Tier-Unterklasse

> **(korr. 2026-06-03, s. Doc 30 §8.0):** Diese Tabelle listet **5 Tier-Unterklassen** (NICHT „5 Gattungen"), verteilt auf **2 Gattungen/Interfaces**: SearchAlgorithm-Zeile = einzige Tier-Unterklasse unter dem **SearchAlgorithm-Interface**; Adapter/Set/Sequence/View = Tier-Unterklassen unter dem **Container-Interface**. Viren liegen außerhalb (Graph-Interface bzw. Nicht-Lebewesen). Die Spalten „Slots SOLL / Eigene Achsen / Anatomie / ABI / Observer" beschreiben die jeweilige **Tier-Unterklasse**; nur das **Dock** (+ ABI-Interface der API) gehört zur Gattungs-/Interface-Ebene. „AnatomyGenus" im Code ist die historische Bezeichnung dieser Tier-Unterklassen-Aufzählung.

Quelle: `anatomy_base.hpp:44-50` (AnatomyGenus — historischer Name der Tier-Unterklassen-Enum), Doku 14 §26-§40, Doc 24 §8.8, abhaengigkeitskette.

| Tier-Unterklasse | Gattung/Interface | Tier (Doku 14 §27) | K/V-Modell | Slots SOLL | Eigene Achsen | Anatomie / ABI / Observer / Dock | IST |
|---------|---------|--------------------|-----------|-----------|---------------|----------------------------------|-----|
| **SearchAlgorithm** | **SearchAlgorithm** (eigenes Interface) | Säugetier (map) | K→V | **17** | — | SearchAlgorithmAnatomy ✓ / SearchAlgorithmAbiAdapter ✓ / ObserverAggregate<17> ✓ / SearchAlgorithmDock ✓ (Dock = Interface-Ebene) | **voll** (BR-1..4) |
| **Adapter** | **Container** (gemeinsames Interface) | Wirbelloses (queue/stack/priority_queue) | Wrapper über Inner | **13 Achsen** (§28: 9 delegiert + 3 aktiv + `inner_container`); KEINE „ordering"-Achse (korr. 2026-06-03, Doc 30 §8.1; FIFO/LIFO = API-Nutzung §26.4) | `inner_container` (axis_inner) — die EINZIGE spezifische | AdapterAnatomy ✓ / AdapterAbiAdapter ✓ / AdapterObserverSnapshot ✓ / AdapterDock ✓ (Rename 2026-06-03 vorher `Container*`; in-process, DLL #75; Dock = Interface-Ebene) | **teilw.** (Dock; DLL offen #75). Nutzt queuing NICHT — queuing (q1/q2) sind SearchAlgorithm-Achsen, keine Adapter-Organe |
| **Set** | **Container** (gemeinsames Interface) | Vogel (set) | K-only (K=V) | **15** (ohne mapping/value_handle; §28-Zählung, K-A aufgelöst) | — | — | **FUTURE** (#76) |
| **Sequence** | **Container** (gemeinsames Interface) | Reptil (vector/list) | V-indexed | **10** geteilte + axis_growth (§28, K-B aufgelöst) | axis_growth | — | **FUTURE** (#76) |
| **View** | **Container** (gemeinsames Interface) | Pflanze (span/mdspan) | non-owning (kein insert/erase/alloc/concurrency) | **7** + axis_extent/layout/accessor | axis_extent/layout/accessor | — | **FUTURE** (#76) |
| (Viren) | **Graph** (eigenes Interface) | Nicht-Lebewesen (Graph/FFT/Crypto) | keine Achsen | 0 (Kapsel) | — | IVirusExecutionEngine (Schwester von AnatomyBase an Wurzel IExecutionEngine); Graph-Dock | **FUTURE** (nur Test-Stub; R6/V42) |

**queuing-Einordnung (präzise, korrigiert; verfeinert korr. 2026-06-03, s. Doc 30 §8.0):** q1/q2 sind reguläre **mandatorische SearchAlgorithm-Achsen** (Organe), nicht Organe einer eigenen Tier-Unterklasse/Gattung — sie liegen lediglich außerhalb der 17 composition_factory-Slots (T0..T16), gehören aber zur **SearchAlgorithm-Tier-Unterklasse** (unter dem SearchAlgorithm-**Interface**) und werden in JEDEM Tier-Binary uniform getrieben (nicht-pufferndes/-flushendes Tier wählt `NoBuffer`/`NoFlush`, durchreichend — keine Achse ist optional). Die **Doku-14-§7-Sicht („q1/q2 = stabile Organe") war KORREKT** (hiermit rehabilitiert); **nicht Doku 14, sondern Doc 27 §0.1 ist die revidierte Fehl-„Präzisierung"**, die queuing fälschlich zur Container-Gattung erhob. Die **Adapter-Tier-Unterklasse** (`std::queue/stack/priority_queue`, unter dem **Container-Interface**) besteht aus den **13 §28-Achsen** (9 delegiert + 3 aktiv + `inner_container`; **KEINE „ordering"-Achse** — FIFO/LIFO = API-Nutzung §26.4, korr. 2026-06-03 s. Doc 30 §8.1) und nutzt queuing NICHT; §32 (Disjunkte-Achsen-Sets, vormals „Cross-Genus") verbietet den Join disjunkter Tier-Unterklassen, macht aber keine Achse zur Gattung/Tier-Unterklasse. Grep-verifiziert: **kein** Set/Sequence/View-Anatomy/Composition/Dock im Code (0 Treffer) → die 3 weiteren Container-Tier-Unterklassen (Set/Sequence/View) + produktiver Viren-Pfad (Graph-Interface) sind nur Doku-SOLL.

## §3 Die 4 Brücken — SOLL + Verifikations-Kriterium (Doc 27 §3)

- **BR-1 Registry→Baum** (`registry_to_axis_levels.hpp` + `axis_reflect.hpp`): alle Enabled-mp_lists → AxisLevels (Anker `TopicConfigSet::StaticAxisVariants_<id>`), block_id-getaggt; + cacheline-Sub-Achse + dynamische thread_count/hw_prefetcher als DynamicDim. **Kriterium:** `tree.binary_count() == ∏ mp_size(Enabled_i) == PermutationEngine::count()`.
- **BR-2 Baum↔Komposition** (`composition_registry.hpp` + `axis_path_serialization.hpp`): Blatt-Pfad → PermTuple<17> → `CompositionFromPermTuple` → `AdHocComposition<17>`. **Kriterium:** jeder Blatt-Pfad round-trippt auf genau EINE reale Komposition; `serialize_composition_path<P>()` (BR-1-Namen) == `serialize_composition_from_slots<C>()` (Slot-Namen).
- **BR-3 NodeValue→Observer** (`node_value_measurement.hpp` + `NodeObserverSnapshot`): je gemessenem Blatt realer Snapshot via `observe_all()`/`IObservableTier::tier_observe`; flacher uint64-POD (kein komposition-typisiertes Member); SPARSE value_map (nur gemessene). **Kriterium:** jeder gemessene Knoten → realer Observer + Definition, **22 Observer-Strukturen** (keine stillschweigende 17-Reduktion).
- **BR-4 generierte Binary→reale Anatomie** (`adhoc_emitter.hpp render_adhoc_module_source`): `perm_<id>.cpp` = `#include all_axes_umbrella.hpp` + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ-Typen>)`. **Kriterium:** generiert → gebaut → `AnatomyModuleLoader` → `dynamic_cast<IObservableTier*>` + `tier_observe` über reale Komposition.
- **Zentrale Pfad-Konvention (Doc 27 §5 R3):** `axis_path_serialization.hpp` (`kCompositionAxisNames` 17 + Format `"axis=value"`/`/`-join, identisch zu `StaticAxisNode::serialize()`) — der Round-Trip-Garant BR-1↔BR-2↔BR-4. R4: Observer-Block flacher uint64-POD.

## §4 Die 6 Gates (Doc 27 §4, wörtlich) — Soll-Kriterium

1. `tree.binary_count() == PermutationEngine::count()` (exakt; via ∏ mp_size constexpr, OHNE mp_product — Kardinalitäts-Identität, Doc 27 §6 C1060).
2. ALLE 22 Achsen als Baum-Ebene mit vollem Enabled-Inventar (17 SearchAlgorithm-Teilbaum + 5 separate).
3. Jedes statische Blatt → reale `AdHocComposition`, als Tier-Binary baubar (BR-4).
4. Jeder gemessene Knoten → realer `ObserverAggregate` + Definition; **22 Observer, NICHT 17**.
5. Inverse Signatur-Projektion (KF-15) über REALE Kompositionen.
6. Belegt Doc 26/27 + Session-Doku; finaler adversarialer Audit bestätigt die Gleichheit.

## §5 Skalierungs-Architektur — lokal zählen / ZIH bauen / Prüf-Dock messen

- **Statisch/dynamisch-Trennung (Doc 26 §2+§4):** STATISCHE Knoten = je distinkter Static-Pfad EINE Tier-Binary (compile-time-Identität, inkl. cacheline line_size/alignment/sw_hint eingebacken → StaticAxisNode); DYNAMISCHE Knoten = FOR-Schleife auf EINER geladenen Binary über `Algorithm_Resource_Control` (thread_count/hw_prefetcher → DynamicVariableNode), KEINE neue Binary. Blatt = EIN Mess-Lauf (Binary × Laufzeit-Einstellung). `binary_count` = Zahl distinkter Static-Pfade; dyn. Kartesik wird NICHT aufgefächert (`experiment_setting_count = binary_count × ∏ dyn`).
- **Skalierungs-Gleichung (Goal-V6, User-verbindlich):** (a) STATISCHE Binary-Rekombinatorik → ZIH-Cluster (10000 Nodes × 32 Kerne, OHNE RAM-Limit; KF-12/13 SLURM-Array + Singularity); (b) je Binary ALLE dynamischen Variablen + Testdaten <1 s (Xeon Platinum/Epyc, Testdaten RAM-resident in CacheEngineBuilder). `binary_count == ∏ mp_size` = Kardinalität des statischen Teilbaums; statisch/dynamisch-Trennung macht Größenordnungen kompatibel. Reale Cluster-Build-Menge ≪ ∏ (BUILD-PROFIL/Enabled-Flags/Coverage wählt; Doc 22 §4 z.B. 1-wise 25 statt 2125).
- **3 Profile (messarchitektur_v5_drei_profile):** (1) BUILD-PROFIL (welche Binaries, configure-time = static_filter) ⊥ (2) LASTENPROFIL (Testdaten/Op-Mix runtime, host-seitig = dyn. loops) ⊥ (3) COMPILE-RELEASE-PROFIL (ob observe/memento einkompiliert; ABI Major 2). Build ⊥ Lasten = kartesisches Kreuz (1 Binary × N Lastprofile ohne Rekompilation).
- **WO gebaut:** lokal/orchestriert = `BuildOrchestrator::provision_all(StaticBinaryView)` (KF-16b, multithreaded, inkrementell-resumierbar, RAM-Admission, Default 4 Kerne, keine Oversubscription). Cluster = `slurm_launcher.hpp` (KF-12 arch-Ausnahmen taskset/numactl/MSR-Prefetcher/Governor/SMT; KF-13 sbatch-Array + Singularity + Webhook) — **emittiert nur Skript-TEXT, submittet NICHTS; Submission GATE-MAXIMAL (mit User absprechen), kein Python**.
- **WO gemessen:** Prüf-Dock je Gattung (Doc 24 §8.8) = `AnatomyModuleLoader` + `IObservableTier` + `drive_tier_observe_trace_abi` + `pruef_dock_sequencer` (gleiche Gattung sequentiell). Default: Binaries einer Gattung seriell, dann nächste Gattung.
- **OOM-/Lazy-Disziplin (Doc 26 §2-Korrektur):** NIE der ganze Baum — immer nur EIN Pfad Wurzel→Blatt (lazy mixed-radix Odometer, O(Tiefe)); `binary_count` arithmetisch; `StaticBinaryView::operator[](i)` dekodiert on-demand; per-node Observer SPARSE (nur gemessene). **Bauen = parallel (K = zulässige DLL-Build-Prozesse); Messen = Unikat-Prozess je Binary (seriell, nur Binary-intern multithreaded); jeder Compile RAM-Watchdog (cl-Kill < 2.5 GB frei).**

## §6 SOLL-vs-IST-Synthese (Überleitung Phase B)

**Solide IST (literal verifiziert, CE cbe3971):** Gate-1 (∏ mp_size==137.594.142.720.000), Gate-2 (22 Ebenen), Gate-5 (inverse Signatur), Gate-6 (Doku); SearchAlgorithm-**Tier-Unterklasse** voll (korr. 2026-06-03, s. Doc 30 §8.0; BR-1/2/3/4 + DLL-Round-Trip); KF-9..16b; lokaler BuildOrchestrator + SearchAlgorithm-Prüf-Dock (Dock = Gattungs-/Interface-Ebene, korrekt).

**Offene Lücken (SOLL gefordert, IST fehlt — Tasks):**
- **#74 (Gate-4-real):** page_type/09b/12 = nur DefinitionOnly-Klassifikation → SOLL eigener Observer + gebaute Build-Variante DERSELBEN SearchAlgorithm-Binary; „22 Observer" real nur 4/17 operativ (R5.B → volle Achsen-Operativität).
- **#75 (Container-Gattung/Interface — Adapter-Tier-Unterklasse; korr. 2026-06-03, s. Doc 30 §8.0/§8.1):** echte Adapter-Komposition (`AdapterComposition<T0..T11, Inner>` der **Adapter-Tier-Unterklasse** aus den **13 §28-Achsen** = 9 delegiert + 3 aktiv + `inner_container` — KEINE „ordering"-Achse, FIFO/LIFO = API-Nutzung §26.4; NICHT queuing) + Adapter-DLL-Pfad (`AdapterAbiAdapter` + COMDARE_DEFINE-Analogon + Loader + observe über DLL-Grenze; Dock + ABI-Interface = Container-Gattungs-/Interface-Ebene). (Typnamen `Adapter*`/`adapter_*` seit Rename 2026-06-03, vorher `Container*`/`container_*`; in Doc 30 §8.1 umgesetzt+verifiziert. queuing q1/q2 = SearchAlgorithm-Achsen, gehören nicht hierher → AdHocComposition 17→19.)
- **#76 (weitere Container-Tier-Unterklassen; korr. 2026-06-03, s. Doc 30 §8.0):** Set (14 Achsen) / Sequence (9+axis_growth) / View (7+axis_extent/layout/accessor) — je eine **Tier-Unterklasse** unter dem Container-Interface — je Anatomie+Composition+ABI+Observer+PermutationEngine (+ gemeinsames Container-Dock auf Interface-Ebene) + GenusBindingTraits-Spezialisierung; (Viren-Pfad IVirusExecutionEngine produktiv, Graph-Interface).
- **#77 (ceb_generator):** reale Anatomie via Name→Typ ODER Anspruch präzisieren (reale Binary läuft über adhoc_emitter).
- **#73 (Tech-Debt):** provision_all results-Vektor batchen (O(K) statt O(∏)).
- **Cluster (GATE-MAXIMAL):** KF-12/13-Skript-Emit erledigt; reale ZIH-Submission user-/termingebunden.
- **Backlog (Doku-only):** Doc 16 axis_05-IMC-Wrapper; Doc 15 axis_09b-Schichten+15-AVX512-Sub-Flags; Original*SearchAlgo s4-Linking; R5.D PMC.

**Gate-3/4 gelten heute nur unter Pilot- bzw. Klassifikations-Vorbehalt.** „ABSOLUTE VOLLSTÄNDIGKEIT gegen ALLE 22 Achsen + alle 5 Tier-Unterklassen (verteilt auf die Gattungen/Interfaces SearchAlgorithm + Container, plus Graph für Viren; korr. 2026-06-03, s. Doc 30 §8.0)" = Phase D, bestätigt durch Phase E (finaler adversarialer Audit).
