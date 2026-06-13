# A1-Lesenotizen + IST-Architektur-Destillat (Masterplan-Phase A1 → B-Entwurf)

> **Zweck:** Durable Destillation der Architektur-Vollektüre (Masterplan A1), kompaktierungs-fest. Jede Zeile =
> verifizierter Stand aus den gelesenen Docs/Code. Wird in Phase B zum konsolidierten Master-Doc poliert.
> **Quellen-Status:** Thesis-Basis `docs/architektur/` 00–14 = **SUPERSEDED-Banner (2026-05-31)** = Begriffs-/
> Konzept-Kontext, NICHT IST. **IST-Single-Source = `docs/sessions/architektur-ziele-offene-punkte-ledger.md`
> (Stand 2026-06-01, HEAD 292aef9) + `20260531-e2e-abnahme-audit-und-entscheidungen.md`.**

## 1. Konzept-Modell (aus Docs 09/10/11/14 — Begriffs-Fundament, gilt weiter)

**3-Ebenen-Modell (Doc 30 §8.0 / Doc 14 §0, AUTORITATIV):**
- **GATTUNG = ein INTERFACE / Prüf-Dock:** `SearchAlgorithm` / `Container` / `Graph` (die Außen-API, je Gattung ein Dock).
- **TIER-UNTERKLASSE = unter dem Gattungs-Interface, FESTER Achsen-Satz:** die 5 Klassen SearchAlgorithm/Set/
  Sequence/Adapter/View (Säugetier/Vogel/Reptil/Wirbelloses/Pflanze) sind **Tier-Unterklassen, KEINE Gattungen**.
- **ACHSEN = Organe der Tier-Unterklasse. KEINE Achse optional** — jede in JEDEM Tier-Binary uniform getrieben;
  „kein Puffer/Prefetch" = konkreter Durchreich-Algorithmus (NoBuffer/NonePrefetch/…), NICHT „Achse weggelassen".
- **queuing q1/q2 = Pflicht-Achsen** der SearchAlgorithm-Tier-Unterklasse (→ AdHocComposition 17→**19**).

**Organ-Metapher (Doc 14 §1/§9, Master-Statement):** Achse=Organ · Algorithmus=Permutations-Konfiguration ALLER
Achsen · **Permutation = genetisches Experiment am Tier (Organe testweise gegeneinander tauschen)** · Bottom-Up:
abstrahiere vom Algorithmus (Tier) zum Organ. Original-Algorithmen = Reference-Compositions (1 Punkt im Raum).
ArtComposition vs ArtPaperBindingComposition = unterscheiden sich in GENAU `search_algo`, 16 Achsen identisch.

**4 Konzept-Ebenen (Doc 11 axes_vs_strategies — Anti-Vermischung):** I Bausteine-Achsen (Permutations-Dim) ·
II CE-Sub-Engines C1-C12 (CE-interne Services) · III Cache-Strategien F1-F29 (Impl. der Sub-Engines) ·
IV Such-Engine-Familien S1-S30 (Impl. der Achsen). Achse ≠ C-Sub-Engine ≠ F-Strategie ≠ S-Familie.

> **⚠️ SUPERSEDED-KONZEPT-VOKABULAR (Docs 03/04/09, alle 2026-05-31-Banner):** Das **29-ICacheStrategy-Familien-
> (F1–F29) / 30-Such-Engine-Familien- (S1–S30) / 4-Ebenen-Strategie- (A IPattern/B IPlural/C ISingular/D ICommand) /
> 12-CE-Sub-Engine-(C1–C12)-Modell** ist der ALTE Planungsstand (Säule A/B, Doc 02 REV7.7). Es ist NICHT das aktuelle
> Achsen/Organ-IST (= 17/19-Achsen-Komposition, B+-Baum, 3-Stufen-Prüfling, V5-Mess-Architektur). **Für B NICHT
> vermischen** — das IST ist das Achsen-Organ-Modell (§1 oben + Doc 30); F1–F29 etc. dienen nur als Begriffs-/
> Paper-Mapping-Historie (33-Paper → Familien). 33 Paper P01–P33 sind den Achsen/Compositions zugeordnet.

**4-Subsystem-Modell (Doc 10):** messung_driver (OUTER LOOP/Auswertung, Diplomarbeit/Code) → **CacheEngineBuilder**
(autonomes Plattform-Ausmess-System, App) → **CacheEngine** (Werkzeug-Bibliothek) ↔ **Prüfling (prt-art)**
(bidirektional: CE bindet Prüfling als Permutations-Struktur + Prüfling nutzt CE-Services). CEB+CE im selben Repo,
2 unabhängige Subsysteme. Orthogonal zum 3-Repo-Layer (Diplomarbeit/prt-art/cache-engine).

## 2. B+-Experiment-Baum (Doc 26/27/29 + `builder/experiment_tree/experiment_tree.hpp` — KERN für Achsen-Austausch)

- Achsen = **Baum-Ebenen** (`AxisLevel{axis, values[]}`, static/dynamic gleichrangig als Knoten-Eigenschaft).
  Ein Pfad Wurzel→Blatt = ein `binary_id` = eine Tier-Binary (statische Rekombination). Blatt + dyn. Belegung
  = eine ExperimentSetting.
- **Nie voll materialisiert** (∏ astronomisch): lazy Mixed-Radix-Odometer, O(Tiefe) Speicher. `binary_count()` =
  ∏ arithmetisch. `StaticBinaryView`: `operator[](i)` ⇄ `flat_index(tuple)` = **inverse Bijektionen**.
- **ACHSEN-AUSTAUSCH = tree-nativ:** `tuple`, NUR `tuple[d]` (Ebene d=Achse a) k→k' ändern, `flat_index` →
  Geschwister-Tier (diff in genau a). O(Geschwister), NICHT flach-quadratisch. ⇒ **Achsen-Struktur/-Austausch
  gehört in den Baum (cache-engine), NIE flach im Eval-Tool** ([[feedback_axis_exchange_belongs_in_bplus_tree]]).
- Mess-Werte sparse in `value_map_` (key=binary_id → NodeValue mit axis_stats[19][8]+seg_ns[19]). KF-15 inverse
  Auswertung = multimap pinned_signature→binary_id. Knoten via Abstract Factory (Static/Dynamic).

## 3. IST-Stand der Mess-Architektur (Ledger 2026-06-01 + V5-Design — AUTORITATIV)

- **3-Säulen (Doc 02, alt aber Struktur gilt):** IExecutingEngine → ISearchEngine/IFutureEngine → Säule A
  (Suchalgo-Datenstruktur) → Säule B (CacheEngine+Plattform-Modell).
- **V5-Mess-Architektur (Ledger §a.V5, I1–I10 done-verified):** 2 Phasen (save→op-warmup→rollback→op-measure) ·
  3 Profile · `memento_all`/`observe_all` parallel · **Konformitäts-Gate gegen std::map** (import→GATE→messen,
  in ALLEN 3 Mess-Pfaden) · **Observer/Memento NUR compile-time entfernbar (kein dynamic_cast im Hot-Pfad)** ·
  `IDriveableTier` (Antrieb, immer) + `IObservableTier` (nur MEAS_ON) + `IRollbackableTier` + `IScannableTier`.
- **ABI:** ANATOMY_ABI_MAJOR (Loader-Reject per Magic-Mismatch); I1-Observer-Konsolidierung = EINE IObservableTier
  + EIN POD (axis_stats[19][8]+seg_ns[19]) + Major 2→3 (Doc 31). status_t errno-style (Schreib-Ops int, Lese-Ops
  natürlicher Typ).
- **17/17 Achsen physisch unter `libs/cache_engine/axes/<axis>/`** (F.2, `topics/`=Forwarder). Allocator=28 Wrapper.
- **3-Stufen-Prüfung (R8 done):** Stufe1 CE-only · Stufe2 Prüfling-einzeln (ERSETZT-mit-Fallback; leere Prüfling-
  Achse reust ALLE CE-Algorithmen) · Stufe3 full-join (Union non-redundant). **Prüfling ≠ Prüflings-Binary**
  (Prüfling = komplettes Achsen-Kompendium/Projekt; Binary = EINE Rekombination; CEB orchestriert Zehntausende).
- **Verantwortlichkeit (Doc 14 §17):** PermutationEngine=Anatomie-Generator (1 Anatomie=1 Permutation) ·
  SearchAlgorithmAnatomy=Organ-Container + ABI-Observer-Aggregat (KEINE insert/lookup — die sind CEB-Commands) ·
  CacheEngineBuilder=Mess-Orchestrierung + ABI-Loader (CMake je Permutation → .dll, dlopen, measure).
- **CEB-Pipeline (Doc 02 §6):** 1 enumerate · 2 codegen · 3 compile (Stage-1/2) · 4 load · 5 run · 6 measure · 7 export.
- **F15-Mission:** schnellste Rekombination im Permutations-Raum finden + alle Achsen-Permutationen studieren;
  V1–V4 Engine-Choice-Dimension (NO-CE/Static/Informed/Adaptive).

## 3a. VERTIEFUNG Mess-Modell (Doc 24 vollständig, gelesen 2026-06-13) — das HYBRID + Prüf-Dock + Prüfling

- **HYBRID = ZWEI Mess-Pfade über DIESELBE Modul-Binary** (Doc 24 §8.1, User-verbatim-tragend). Gemeinsame Basis:
  jede Permutation wird als SHARED-DLL gebaut (`adhoc_emitter` → `comdare_build_adhoc_modules` → `AnatomyModuleLoader`).
  Die **Mess-KONFIGURATION** wählt den Pfad:
  - **Pfad A — isolierte Achsen-Algos gegeneinander:** läuft **IN der DLL selbst** via `IMeasurableWorkload::run_workload`
    (DLL fährt eigenen Mess-Workload, liefert Batch-Latenzen) → host-seitige Aggregation + Statistik (`f15_compare`,
    Welch/MWU/Cliff's δ). Dimension §2.3 (Achsen-Vergleich). **NICHT verworfen** (Korrektur einer früheren Fehlaussage).
  - **Pfad B — composite Tier:** läuft **zentral host-seitig über CacheEngineBuilder** via ABI-stabilen **Observer-Zugriff**
    (`IObservableTier::tier_observe` → flacher POD über DLL-Grenze). Dimensionen §2.1 (Tier-Wall-Clock: Füllstand-Kurven,
    r/w/d getrennt, RAM/Disk) **+** §2.2 (Per-Achsen-`observe_all` → ObserverAggregate). **Zeit-/zustands-KORRELIERT**
    (§8.7): jeder Observer-Snapshot trägt Wall-Clock-Stempel → Zeitreihe `[(t,ObserverAggregate)]`; 2 Trigger-Modi
    (a Zeitschritt-Sync / b Zustands-Manipulation = Füllstands-Checkpoints). R6 done-verified (`R8RestA_DockMeasuresRealDll`).
- **3 Mess-DIMENSIONEN (Doc 24 §2.4):** §2.1 Tier-Wall-Clock (CacheEngineBuilder) · §2.2 Achsen-Observer (`observe_all`,
  Anatomie) · §2.3 Achsen-Vergleich (Unit-Tests gegen vereinheitlichtes Interface vs. bekannte Algos, z.B. std::map) —
  **welche Achsen-Variante „besser" ist, entscheidet §2.3, NICHT der Latenz-Benchmark.**
- **PRÜF-DOCK (Doc 24 §8.8):** = die CacheEngineBuilder-SEITE für GENAU EINE Gattung (= Außen-Interface) — lädt +
  treibt Gattungs-API durch + misst Observer + persistiert. EIN Dock je Gattung: **SearchAlgorithm / Container / Graph**
  (NICHT je Tier-Unterklasse — Set/Sequence/Adapter/View teilen das EINE Container-Dock). **KEIN Neubau** — nur Benennung
  der vorhandenen `IObservableTier`+`AnatomyModuleLoader`+`drive_tier_observe_trace_abi`-Verdrahtung (`pruef_dock/`).
- **PRÜFLING-Integration (Doc 24 §8.9/§8.9.1) = CMake + Metaprog-Join, NICHT Header-Kopie:** prt-art = abstrakte
  Tier-Permutation, liefert für EINIGE Achsen neue Algos, per C++23-Metaprog mit CE-Achsen gejoint (`optional_prt_art_impl`-
  Slot, ERSETZT-mit-Fallback). **3 Join-Stufen, EIN Dock misst alle:** Stufe1 `comdare_perms_ce` (A) · Stufe2
  `comdare_perms_<pf>` (B) · Stufe3 `comdare_perms_full_join` (A⋈B, dedupliziert = „Schnabeltier"-Hybrid). **Regel der
  abstrakt-leeren Achse:** leere Prüfling-Achse reust ALLE CE-Algos → Stufe-2-Raum = kartesisches Produkt ÜBER die leeren
  Achsen (`B = 1^|belegt| × ∏_j|A_j|`), NICHT eine Komposition; Dock misst alle, um die schnellste Prototyp-Rekombination zu finden.
- **§5.5-Key-Type-Blocker (historisch, AUFGELÖST):** Such-Organe hatten schmale Keys (Array256=uint8 etc.) → durch
  Umstufung-A/B alle auf gemeinsamen **uint64** → Builder treibt Composition-Organ verlustfrei (Pfad B).

## 2a. VERTIEFUNG B+-Baum (Doc 26 + 27 vollständig, gelesen 2026-06-13) — 22 Achsen, 4 Brücken, Gate-1

- **22 Achsen / 15 Topics (Doc 27 §0, AUTORITATIV; „17"≠Gesamtzahl):** **17 AdHocComposition-Slots T0–T16** (search_algo…
  filter) = Kern der **SearchAlgorithm-Tier-Unterklasse** · **q1/q2 queuing = reguläre mandatorische SA-Achsen** (→ **19**;
  NoBuffer/NoFlush = Durchreich-Algo) · **page_type/09b/12 = 3 Build-/Codegen-Achsen** DERSELBEN SA-Binary. KEIN getrennter
  Genus-Teilbaum für queuing. Adapter-Tier-Unterklasse (unter Container-Interface) = 13 Achsen (9 delegiert + 3 aktiv +
  `inner_container`; KEINE „ordering"-Achse — FIFO/LIFO = API-Nutzung §26.4); nutzt queuing NICHT.
- **4 BRÜCKEN (Doc 27, alle DONE+verifiziert):** **BR-1** registry→`AxisLevel`s (`registry_to_axis_levels.hpp`, alle 22
  als Baum-Ebene) · **BR-2** Blatt-Pfad↔reale `AdHocComposition<17>` (`composition_registry.hpp` + EINE zentrale
  `serialize_composition_path<P>()`) · **BR-3** `NodeValue`→echter `NodeObserverSnapshot` (flacher uint64-POD ==
  `ComdareTierObserverSnapshotV1`, sparse value_map nur GEMESSENE Knoten; `node_value_measurement.hpp` treibt realen
  Adapter) · **BR-4** generierte Binary→reale Anatomie (`render_adhoc_module_source` → `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC`
  → DLL → Loader → `dynamic_cast<IObservableTier*>`). Reihenfolge BR-1→2→4→3.
- **Gate-1 (literal verifiziert):** `tree.binary_count() == ∏ mp_size(Enabled_i) == PermutationEngine::count() ==
  137.594.142.720.000` — REIN ARITHMETISCH (Kardinalitäts-Identität `mp_size<mp_product<L…>>=∏|L|`), OHNE den Typ-Baum.
- **🔴 C1060 (empirisch, Doc 27 §6):** Voll-`mp_product` über 17 Achsen sprengt den Compiler-Heap (eager `tree.build`
  zog ~21 GB) ⇒ Voll-Typ-Baum INFEASIBLE. ⇒ **nur EIN Blatt compile-time materialisiert** (on-demand BR-2/BR-4);
  Laufzeit-Baum-Manager zählt/strukturiert den Raum, **per-Achse CRTP-Deskriptor-Klassen** (typsicher) + **AxisBlock**
  (typsicherer Teilbaum, Wurzel-an-Tail-Verkettung). Iteration = lazy Mixed-Radix-Odometer (O(Tiefe)); Build = nur K Pfade
  parallel (`StaticBinaryView::operator[](i)` dekodiert EINEN Pfad). **§b R5.B-Operativität-Grenze ehrlich:** operativ
  misst real nur search_algo (+ allocator), Rest passive Compile-Time-Deskriptoren (`observable_axis_count` macht es transparent).

## 3b. VERTIEFUNG Observer/ABI-Konvergenz (Doc 29/31 + abhaengigkeitskette + messarchitektur_design_observer, gelesen 2026-06-13)

- **LEBEWESEN→PRÜF-DOCK-Kette (abhaengigkeitskette, 8 Schichten):** `IExecutionEngine` (Wurzel; Schwester `IVirusExecutionEngine`
  = Graphen-Algos ohne Anatomie) → `AnatomyGenus` (Dock-Diskriminator) → `Composition` (17 using-Slots) → `SearchAlgorithmAnatomy<C>`
  (`observe_all`→`ObserverAggregate<C>`) → **`SearchAlgorithmAbiAdapter<A>` = DREIFACH-Vererbung `: IAnatomyBase, IMeasurableWorkload,
  IObservableTier`** → 4 extern-C-Symbole (create/destroy/version/magic) → Snapshot-POD (memcpy) → `AnatomyModuleLoader` → Prüf-Dock.
  Slot-Reihenfolge T0..T16 UNVERRÜCKBAR (search_algo, cache_traversal, mapping, path_compression, node_type, memory_layout, allocator,
  prefetch, concurrency, serialization, telemetry, value_handle, isa, index_organization, io_dispatch, migration_policy, filter).
- **🔴 ABI-Grund-Invariante (warum Sub-Interfaces):** Eine neue Mess-Fähigkeit darf NIE als virtuelle Methode ans vtable-Ende von
  `IAnatomyBase`/`IObservableTier` — alte DLL gegen neuen Host springt über fremde vtable → **SEH 0xc0000005**. Stattdessen:
  jede Fähigkeit als eigenes Sub-Interface, das der Adapter ZUSÄTZLICH erbt (NICHT unter IAnatomyBase → dessen vtable bleibt stabil);
  Host fragt via `dynamic_cast<…*>` ab. **Dreifach-Vererbungs-REIHENFOLGE eingefroren** (bestimmt Sub-Object-Layout + cast-Offsets).
  `destroy_fn` VOR `dlclose` (sonst UAF). `host_compatible_with = major==host.major && module.minor<=host.minor` (Modul alt erlaubt, nie zukünftig).
- **🔴 dynamic_cast ist KALT, nicht heiß (verifiziert):** 1× pro `measure()`/Modul, gecacht + per Referenz; Hot-Loop
  (`drive_tier_observe_trace_abi`) nimmt `IObservableTier&` → NULL cast, nur Virtual-Calls. ⇒ **Die „no-dynamic-cast-im-Hot-Pfad"-
  Direktive ist im IST-Code bereits ERFÜLLT** (kein Fix nötig — nur dokumentieren). Op+Param+Observer in EINER Schnittstelle vereint.
- **I1-Observer-KONSOLIDIERUNG (Doc 31, DONE 2026-06-05, ABI-Major 2→3):** GENAU EINE `IObservableTier::tier_observe(
  ComdareTierObserverSnapshot*)` (pure virtual) + EIN versionierter POD = **`axis_stats[19][8] + seg_ns[19] + Meta`**. Subsumiert
  V1(13)+V2(26)-Felder verlustfrei (search→[0][0..5], alloc→[6][0..4], telemetry→[10], memory_layout→[5], serialization→[9],
  node_type→[4], Timing→seg_ns[0..18]). **V3 = konzeptionelle Wende** (generische schema-stabile `axis_stats[T][f]`-Matrix,
  `kV3AxisSchema` single-source → neue Achse = nur weitere Zeile, kein neuer POD); V4 ergänzte orthogonal `seg_ns` (Pfad-B-Timing
  über die REALE Komposition, KEIN synthetischer Puffer). V1/V2/V3/V4-Sub-Interfaces + Bridge ENTFERNT; Versionierung jetzt auf
  ABI-Major-Ebene (Loader-Reject per Major-Mismatch = der saubere Ort für einen echten Layout-Bruch). I1 erzwang Neubau ALLER Perm-DLLs.
- **🔴 Q1-SEQUENZ in der EINEN `tier_observe` (gegen Doppelzählung, ZWINGEND):** (1) `axis_stats`-READ → (2) `seg_ns`-TIMING
  (`fill_segment_timing_v3` treibt per-op-Organe) → (3) per-op-RESET. Sonst T0/T1/T2/T3/T7/T8/T10/T17/T18 doppelt.
- **Composition-Driver-Stand (Doc 29, R5.B-Grenze):** Anatomie hält real nur `axis_search_algo_` als Member; weitere Organe blockt der
  **protected CRTP-Base-ctor** (`{}`-Aggregat-Init spricht die protected Base an). LÖSUNG = **`ObservableXxx`-Hülle pro Achse** (Option H,
  search_algo-Vorbild `ObservableComposedSearch`): trägt die Mess-Mechanik (`statistics()/snapshot_t/reset()`, gegated), kein Aggregat
  → als Member `{}`-haltbar + transparenter `name()`-Decorator. **telemetry + memory_layout als 2./3. voll getriebene Achse verdrahtet**
  (`test_v41_anatomy_observer` 15/15 grün). VERBLEIBT (E-Kandidat): Auto-Kopplung am echten Tier-insert + restliche Reference-/AdHoc-
  Compositions + serialization/node_type nach demselben Hüllen-Muster.
- **Baum-Generik (Doc 29 §1):** Baum-KERN voll generisch (`build(vector<AxisLevel>)`, mixed-radix beliebig viele Ebenen, `genus_binding_traits`
  parametrisch); **fest-N pro Tier-Unterklasse = bewusste Invariante** (neue Achse INNERHALB = Composition + ObserverAggregate anfassen,
  NICHT generisch — sonst verliert die Tier-Unterklasse ihre ABI-Identität).
- **B1-Split (messarchitektur_design_observer, auf Major-Bump mitgeritten):** `IObservableTier` → `IDrivableTier` (Ops, Pflicht) +
  `IObservableTier` (nur `tier_observe`); 2 Build-Profile MESS (STATISTICS=ON) vs REIN (=OFF). Ledger §a.V5 listet `IDriveableTier`/
  `IObservableTier`/`IRollbackableTier`/`IScannableTier` als existent → B1 umgesetzt.

## 3c. VERTIEFUNG V5-Mess-Spezifikation + Vollständigkeits-Karte (messarchitektur_v5_design + Doc 28, gelesen 2026-06-13)

- **ZWEI Seiten, EINE ABI-Grenze (v5_design §1, bindend):** HOST = CacheEngineBuilder-Binary (Builder-vtable erlaubt, NICHT
  Hot-Path) · TIER-BINARY = geladene .dll (compile-time monomorph, Hot-Path, kein virtual/dynamic_cast). **State lebt IN der Binary**
  (`search_organ_`, `ComposedStore<N,L,A>`, Disk-Files), NICHT host-seitig; der Host hält nur Latenz-ns-Vektoren + Op-Protokoll.
  **Release-DLL (MEASUREMENT_OFF):** observer_all + memento_all COMPILE-TIME entfernt (`#if COMDARE_MEASUREMENT_ON`) → vtable-Slots
  existieren nicht, KEIN Overhead, an Forschung auslieferbar (kein dynamic_cast zur Entfernung — die Typen sind schlicht nicht da).
- **🔴 3 PROFILE orthogonal (v5_design §2 / Doc 28 §5):** (1) **BUILD-PROFIL** (statisch, configure-time: welche Binaries, Achsen-
  Vendor, Cartesian, smoke/medium/full, = `static_filter`) ⊥ (2) **LASTENPROFIL** (runtime host-seitig: Testdaten + Op-Mix + Pausen,
  YCSB A–F, KEINE CMake-Flags = dyn. loops) ⊥ (3) **COMPILE-RELEASE-PROFIL** (ob observe/memento einkompiliert; `COMDARE_MEASUREMENT_MODE`
  Default ON; ABI-Major). **Build ⊥ Lasten = kartesisches Kreuz** (1 Binary × N Lastprofile OHNE Rekompilation) — die zwei Haupt-Experiment-Achsen.
- **🔴 ZWEI-PHASEN-OP-SCHLEIFE (v5_design §4, PFLICHT für Mess-Gültigkeit, = Goal §0.2):** pro Op GENAU 2×: (1) `tier_save_all()` →
  (2) op (erste Ausführung, warmt) → (3) `tier_rollback_all()` (Tier-State inkl. Disk auf Vor-Zustand) → (4) op (op-measure, JETZT
  Wall-Clock-umklammert + Observer). Eliminiert die Pfad-Abhängigkeit der Latenz vom akkumulierten Zustand. `drive_two_phase_op_loop`.
- **memento_all-System (v5_design §3, parallel zu observe_all):** **HYBRIDER VISITOR** (Tier-Binary-Rollback-Klassen besuchen den Host
  — weil der State drüben lebt); `MementoAxis`-Concept (`save_state/restore_state/memento_persist`); **9 stateful** Achsen (search_algo/
  node_type/allocator/concurrency/serialization/value_handle/filter/io_dispatch/migration) / **8 stateless** (`EmptyMemento`). Sub-IF
  `IRollbackableTier` (nicht an IAnatomyBase). **Disk-Sonderfall ehrlich:** kein generisches Format (axis_06 ~20 Allokatoren) → Memento
  pro-Wrapper-spezifisch; R1 = höchstes Risiko. (Ledger: memento via CoW Rev.2 umgesetzt, Doc 33.)
- **🔴 KONFORMITÄTS-GATE gegen std::map je Gattung (v5_design §6):** Reihenfolge ZWINGEND **import → GATE → (nur bei pass) messen**.
  `conformance_gate.hpp` = Runtime-Host-Oracle über `IDriveableTier`: deterministische Randfall-Sequenz (leer/single/Doppel-Insert/
  Update/erase-nichtvorhanden/clear-dann-lookup/full-sweep) gegen `std::map<uint64,uint64>`; jede Binary muss gattungs-konform speichern+wiedergeben.
- **5 TIER-UNTERKLASSEN SOLL/IST (Doc 28 §2):** **SearchAlgorithm** (17, unter SearchAlgorithm-Interface — **VOLL** BR-1..4) · **Adapter**
  (13 Achsen unter Container-Interface — **teilw.**: Dock+in-process, DLL offen #75; KEINE „ordering"-Achse, `inner_container` die einzige
  spezifische, FIFO/LIFO=API-Nutzung) · **Set** (15) / **Sequence** (10+axis_growth) / **View** (7+axis_extent/layout/accessor) = **FUTURE**
  (#76, je unter Container-Interface) · **Viren** (Graph-Interface, `IVirusExecutionEngine` Schwester an IExecutionEngine — FUTURE-Stub).
- **Skalierungs-Architektur (Doc 28 §5):** lokal ZÄHLEN (∏ arithmetisch) / ZIH BAUEN (SLURM-Array + Singularity, GATE-MAXIMAL, nur
  Skript-Emit kein Submit) / Prüf-Dock MESSEN (gleiche Gattung sequentiell). `binary_count` = distinkte Static-Pfade; dyn. Kartesik NICHT
  aufgefächert (`experiment_setting_count = binary_count × ∏ dyn`). Reale Cluster-Build-Menge ≪ ∏ (Coverage/Enabled-Flags wählt).
- **SOLL-vs-IST offene Lücken (Doc 28 §6, für D/E):** #74 (page_type/09b/12 real-Observer + Build-Variante; „22 Observer" real nur 4/17
  operativ = R5.B) · #75 (Adapter-DLL-Pfad) · #76 (Set/Sequence/View + Viren produktiv) · #77 (ceb_generator) · Cluster GATE-MAXIMAL ·
  Backlog Doku-only (Doc 16 IMC, Doc 15 09b-Schichten/AVX512-Sub-Flags, Original*SearchAlgo s4-Linking, R5.D PMC).
- **is_original-Klassen (Doc 17/28):** A=echtes Linking (allocator [6 aktiv: mimalloc/jemalloc/snmalloc/rpmalloc/dlmalloc/lrmalloc]/search_algo/
  q1) · B–E=Re-Impl + `is_original=false` ehrlich (prefetch/value_handle/memory_layout/index_org/telemetry/migration/io).

## 4. Offene Punkte / Vorbehalte aus dem IST-Ledger (für D/E relevant)
- Vendor-Allokatoren (#19, jemalloc/tcmalloc/hoard/scalloc) + reale PMC (#26) = **extern/toolchain-gated**
  (lokal nicht baubar; Beschaffungs-Specs geliefert; erst ZIH/Cluster). Mechanik an mimalloc/snmalloc/dlmalloc bewiesen.
- #22 Submodule-Repos: Kern-Befüllung done (Option A); Option-B-Konsumptions-Migration gated auf GitLab/DependencyManager.
- V5-I-Drive-Vollausbau (IDriveableTier auf volle std::map/std::vector-API) = offener Strang (Voraussetzung volle Gate-Äquivalenz).
- YCSB A–F treu+zitiert done; ehrliche Lücke „update=Upsert" dokumentiert.

## 4b. IST-Ledger §b/c/d/e (vollständig gelesen, Z.148–226) — Status-Landkarte für C/D/E

- **§e VERIFIZIERT-ERLEDIGT (der Großteil!):** V5 I1–I10 · F.2 17/17 Achsen · F.3 17 Concepts · F.4 Tools-Facade ·
  R6.1–6.5 (IObservableTier+POD, Wall-Clock+Observer-Trace, Loader, Pfad-B-2D, Prüf-Dock) · R7.2/7.3/7.4-Body ·
  R8 (Prüfling 3-Stufen) · Umstufung-A/B · s4 · Cross-Constraints · G.1 (messung_driver axis_tree) · E10.x ·
  **#42-Phase-2: `EnabledStrategies = mp_filter<is_enabled, AllStrategies>` = 4 Such-ORGANE (K_ARY/INTERPOLATION/
  EYTZINGER/LINEAR_SCAN, USE=1), 13 Monolith-Tiere USE=0 deregistriert** (konfig. Flags-Header; Direktive
  no_whole_tier_axes auf EnabledStrategies-Ebene erfüllt). ⇒ Architektur-Substanz steht; Mess-Pfad real.
- **§b EXTERN/TOOLCHAIN-GATED:** A1/A2.1 Vendor-Allokatoren (jemalloc/tcmalloc/hoard/scalloc — lokal nicht baubar) ·
  R5.D/#26 PMC-HW-Counter (Intel PCM/MSR) · C1/C2 Cluster/Grace-Hopper · E10.6/7 ZIH-Verteilung ·
  E11-Facade-Impl (gated auf #22/V42) · F.6-Phase-C (23 Legacy-Header erst NACH Habich-Termin löschen) ·
  **Doku-11/14-Verif „nicht im Repo" — HEUTE AUFGELÖST: per Junction `docs/architektur` (Thesis-Basis 00–14) zugänglich.**
- **§c USER-MANUELL:** D1 (Diplomarbeit-Kapitel-Text) · D2 (Bausteine-Matrix-Update) — User schreibt.
- **§d V42-FUTURE:** Gattungs-Docks Set/Sequence/Adapter/View (Blueprint=SearchAlgorithmDock) · R7.6.c is_original-
  Linking · Naming-Refactor-Backlog (axis_12/04/03a/q1q2/08) · E9 raw-string · #22 Submodule-Option-B.
- **Implikation für E:** Die Original-Mission (Bias-Matrix-Messung) baut AUF dieser fertigen Substanz auf; die
  Audit-Befunde (D) sind Korrekturen AM bestehenden Mess-Pfad, keine Neubauten. Achsen-Austausch-Auswertung
  gehört in Baum/CEB (nicht Eval-Tool).

## 4c. IST-Doc `20260531-e2e-abnahme-audit-und-entscheidungen.md` (vollständig) — Mess-Pfad-Architektur

- **3-Repo-Modell code-verifiziert korrekt:** cache-engine = Achsen-Bibliothek (15 Topics / **22 Achsen** [„17" = NUR
  SearchAlgorithm-Komposition-Slots; 5 außerhalb: page_type/09b/12/q1/q2]) + Anatomie-Generator (`AdHocComposition<T0..T16>`
  → PermutationEngine → `adhoc_emitter` → je Permutation eine SHARED-DLL) + EINHEITLICHES Prüf-Dock
  (`SearchAlgorithmDock`, `dynamic_cast<IObservableTier*>` über reale DLL-Grenze; Pfad A `run_workload`+`f15_compare`
  / Pfad B `IObservableTier`-POD). prt-art = Prüfling-Plugin. Diplomarbeit = 6-Stufen-LaTeX-Pipeline.
- **6-Stufen-Pipeline (kanonisch 16-Spalten-CSV, `workload_used`@idx3):** `01 sample → 02 messung_driver →
  03 binary→csv → 04 csv→latex → 05 diagram → 06 latex→pdf`. P1 done (E2E-Target `comdare_pipeline_e2e` → PDF).
- **🔴 ZENTRALER ARCHITEKTUR-CONSTRAINT (G3/P2-Planrunden-Befund, hart verifiziert):** Der naive Achsen-Tausch
  **`AdHoc<Organ, Default-Achsen>` ist ILL-FORMED** (SearchAlgorithmAbiAdapter-CTAD scheitert C2780/C2514) — sezierte
  Organe (Art/Hot/Start/Surf/Wormhole/Masstree) brauchen ihre **KOMPATIBLEN Begleit-Achsen** (node_type/path_compression/…
  wie in `*_reference.hpp`), NICHT beliebige Defaults. ⇒ **Achsen-Austausch ist CROSS-ACHSEN-CONSTRAINT-behaftet**
  (Doc 14 §4.3 Rekombinations-Test); freie Permutation aller Achsen ist NICHT uneingeschränkt gültig. **Kritisch für
  die Original-Mission (Achsen-Austauschbarkeits-Belege): der Austausch muss Organ-Begleit-Achsen-Kompatibilität
  respektieren** — exakt das, was im B+-Baum/named-Compositions modelliert ist, NICHT in einer flachen Tupel-Kombinatorik.
- **Autoritative F15-Mess-Quelle = 6 named Observable-Organ-Compositions** (via `comdare_codegen_anatomy_module_list`,
  `test_v41_anatomy_multi_codegen` 7/7); der **Monolith-AdHoc-48-DLL-Pilot ist SUPERSEDED**. F15 real auf i7-1270P
  erhoben (p50 479–653 ns). **NB:** Die spätere 320-FullPilot-Bias-Matrix (Original-Mission/M-Lauf) ist ein
  SEPARATER, größerer Mess-Aufbau — Verhältnis zu dieser 6-Organ-F15-Quelle ist in B zu klären.
- **P-Status:** P1 (Pipeline) ✅ · P2 (Organ-F15) ✅ · P3 (i7-Realmessung) ✅ · P4 Vendor lokal-baubar (Quellen
  vendored, nur HAVE-Flags nicht gesetzt)/PMC extern · P5 Doku-Drift+op_type_filter teils.

## 4d. Doc 30 (audit_achsen_delegation_pflichtachsen — AUTORITÄT des 3-Ebenen-Modells + DER ECHTE VERSTOSS)

- **3-Ebenen-Modell (§8.0, AUTORITATIV, verbatim):** (a) **Gattung = Außen-Interface** SearchAlgorithm/Container/Graph
  (= Prüf-Dock, Doc 24 §8.8/§8.6) · (b) **Tier-Unterklasse = unter dem Interface, FESTER Achsen-Satz** (5 code-Genera
  SearchAlgorithm/Set/Sequence/Adapter/View; aktuell NUR die SearchAlgorithm-Tier-Unterklasse gebaut) · (c) **Achsen =
  Organe, ALLE Pflicht, in JEDEM Tier-Binary uniform getrieben** (NoBuffer/NonePrefetch/NoMigration = konkrete Durchreich-
  Algorithmen, NICHT abwesende Achsen). Per-Gattung-Slots: **SearchAlgorithm 17(+3 Build), Container/Adapter 13, Set 15,
  Sequence 11, View 7**. queuing q1/q2 = **Pflicht-SA-Achsen** (AdHocComposition **17→19**, `c9f051b`); Adapter-Gattung =
  echte Tier-Unterklasse (`AdapterComposition<T0..T11,Inner>` 13 Achsen: 9 delegiert + 3 aktiv + 1 inner_container; `18adc08`).
- **🔴 BEFUND 2 — DER ECHTE ARCHITEKTUR-VERSTOSS (kritisch, = Audit-K5/K6, Kontext meiner A2a-Arbeit):** Der
  `SearchAlgorithmAbiAdapter` hält **ZWEI getrennte, unverbundene Speicher**: `search_organ_` = monolithisches Such-Organ
  (`Composition::search_algo`, z.B. KArySearchAlgo mit eigenem Substrat, nimmt KEINEN node/layout/allocator-Slot) +
  `container_` = SEPARATER `ObservableComposedSearch<SortedBinaryTraversal[hart-verdrahtet, NICHT Composition::search_algo!],
  Store>` nur „um die Allocator-Achse zu messen". `tier_insert` treibt BEIDE (Doppel-Buchführung); Such-Metriken aus dem
  Monolith, alloc_* aus dem Store. ⇒ **Tiere routen NICHT uniform durch alle 17 Organe; Such-Organ beschattet node_type/
  memory_layout** (Sezierungs-Prinzip Doku 14 §3.1 im Mess-Pfad verletzt).
  - **§6 Q2 Schritt 1–3 GEFIXT+verifiziert:** `container_` von unbounded ComposedStore → **`NodeChunkedStore<N,L,A>`**
    (cap=N::max_capacity()) → node_type wirkt real (alloc_cnt = ceil(n/node_cap): Node4=250/Node256=4 @n=1000; vorher
    konstant 18). **§6 Q2 Schritt 4 OFFEN:** SEARCH-Zähler kommen WEITER aus dem Monolith `search_organ_`; volle
    Such-Organ-Delegation (search_organ_ entfällt, Such-Strategie ALS Traversal über DENSELBEN Store) + perm_runner→V2-POD
    (node_*-Felder) = verbleibend. **Das ist die SOLL-Korrektur für die Achsen-Echtheit (C1/C3/C4/C5 müssen ECHT, nicht umgangen).**
- **Befund 3 (Literatur-Treue):** 17-Zerlegung GiST-fundiert; Pflicht-Kern C1 Such/Navigation · C3 Key→Position-Mapping ·
  C4 Node/Storage · C5 Value/Payload (dürfen NICHT umgangen werden); Rest = Pflicht-Dim mit Trivial-Default. 2 Lücken:
  G1 Key-Normalisierung (keine eigene Achse), G2 Split-Merge/Reorganisations-Policy (axis_02 deckt nur Pfad-Kompression).
- **Verknüpfung Original-Mission/Audit:** Befund 2 = K5 (Apparat dominiert) + K6 (Phantom-Allocator) der teuren Audits.
  Meine A2a/K3-Arbeit (restore_statistics in die Wrapper) betraf `search_organ_`/`container_`-Memento — orthogonal, aber
  im selben Adapter. Der Q2-Schritt-4-Fix (volle Such-Delegation) ist ein Kandidat für eine E-Aufgabe (Mess-Echtheit).

## 5. A1-Lese-Fortschritt (Checklist)
- ✅ Thesis-Basis: 00_INDEX · 02_master_REV7_7 · 09_taxonomien · 10_schichten_modell_M · 11_axes_vs_strategies
- ◐ 11_konzept_extension_visitor (§1–§11 von §… ; 4 Patterns + 3-Stufen + CRTP+Concept + Prüfling-Namespace gelesen)
- ◐ 14_organ_metapher (§0–§20 von §53; Organ-Metapher + 3-Schichten + Verantwortlichkeit + ObserverAggregate)
- ✅ IST-Ledger (vollständig, 226 Z.; §a/§a.V5/§a.P/§b/§c/§d/§e)
- ✅ `20260531-e2e-abnahme-audit-und-entscheidungen.md` (2. IST-Doc, vollständig)
- ✅ cache-engine **Doc 30** (audit_achsen_delegation_pflichtachsen — 3-Ebenen-Autorität + Befund-2-Verstoß)
- ✅ cache-engine **Doc 33** (Memento Rev.2 CoW + Resume — bestätigt §3: Memento deckt search_organ_+container_ =
  T0+T6; Zwei-Phasen-Warmup PFLICHT; Resume je Tier-Binary via Config-Stamp [BuildVersion+dims+rows]; CoW =
  Rev.1-Eskalation generalisiert auf alle Mutationen, Read-Perioden O(1))
- ✅ (Code, frühere Session) `experiment_tree.hpp` (= Substanz von Doc 26/27/29 B+-Baum) · `abi_adapter.hpp` (CoW-Teil)
- ✅ cache-engine **Doc 24** (Mess-Modell 2-Dim — vollständig: HYBRID Pfad A/B + §8.7 korrelierte Erhebung + §8.8 Prüf-Dock
  + §8.9/§8.9.1 Prüfling-3-Join + leere-Achse-Regel; → §3a)
- ✅ cache-engine **Doc 26** (B+-Baum-Prosa, vollständig) + **Doc 27** (4 Brücken BR-1..4 + 22-Achsen-Inventar + Gate-1
  137.594.142.720.000 + C1060-Infeasibility; → §2a)
- ✅ cache-engine **Doc 29** (Baum-Generik + Composition-Driver-Stand: ObservableXxx-Hülle, telemetry+memory_layout getrieben)
  + **Doc 31** (Observer-Konsolidierung I1, EIN POD axis_stats[19][8]+seg_ns[19], ABI-Major 2→3, Q1-Sequenz) + **abhaengigkeitskette**
  (8-Schicht-Kette + Dreifach-Vererbung + SEH-vtable-Invariante) + **messarchitektur_design_observer** (dynamic_cast KALT, B1-Split); → §3b
- ✅ cache-engine **Doc 28** (Vollständigkeits-Kartographie: 22-Achsen-SOLL + 5 Tier-Unterklassen + 6 Gates + Skalierung + Lücken
  #74/75/76/77) + **messarchitektur_v5_design** (bindende V5-Spec: 2 Seiten/1 ABI, 3 Profile, Zwei-Phasen-Op-Schleife PFLICHT,
  memento_all hybrider Visitor, Konformitäts-Gate import→GATE→messen, compile-time-Removal); → §3c
- ✅ Thesis **03_konzepte_saeule_a + 04_konzepte_saeule_b** (SUPERSEDED-Konzept-Vokabular: F1–F29 / 4-Ebenen-Strategie
  A-B-C-D / 33-Paper-Map / Säule-B Plattform-Auto-Discovery [Discover→Measure→Classify→Publish→Bind] + 28 Concept-Klassen
  + ~80 Heuristiken + Block-AO-Maschinen Ryzen-9950X3D/i9-14900KS — Kontext, NICHT IST)
- ⬜ OFFEN: Thesis 01,05,06,07,08,12,13 + Rest 11/14 · cache-engine **messarchitektur_klarstellungen ·
  messarchitektur_v5_entscheidungen/_drei_profile/_i8** + 32 (Lastprofil-Katalog) + 15–23/23a/25(×2) ·
  A2 Rest-Code-Pre-Read (registry_to_axis_levels/profile_to_tree/composition_registry/composition_factory/
  search_algorithm_anatomy/observable_tier/perm_runner/iterator/permutation_engine/genus_binding_traits) · A3 Audits-Soll-Abgleich.
  (Beide IST-Docs + Doc 24/26/27/28/29/30/31/33 + abhaengigkeitskette + design_observer + v5_design ✅ — die Mess-/Baum-/Observer-/
  Vollständigkeits-Architektur ist jetzt VOLLSTÄNDIG erfasst; Konsolidierungs-Basis B steht solide; Rest = v5-Profil-Details/Lastprofile/historischer Kontext.)
