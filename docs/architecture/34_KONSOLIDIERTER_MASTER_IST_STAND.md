# 34 — KONSOLIDIERTER MASTER-IST-STAND (Single-Source-of-Truth) — 2026-06-13

> **Zweck (Masterplan B1):** EIN umfassendes, aktuelles Architektur-Doc, das den IST-Stand über ALLE Quellen konsolidiert —
> ein neuer Bearbeiter kann daraus korrekt arbeiten, OHNE alle 48 Quell-Docs einzeln zu lesen. **Basis:** die zwei IST-Single-
> Sources (`architektur-ziele-offene-punkte-ledger.md` + `20260531-e2e-abnahme-audit-und-entscheidungen.md`) + die destillierten
> Lese-/Code-/Audit-Notizen (`20260613-A1-lesenotizen-ist-architektur.md` · `…-A2-code-pre-read-notizen.md` · `…-A3-audit-soll-
> abgleich.md`) — alle agent-verifiziert (A1-Coverage VOLLSTÄNDIG + Treue DESTILLAT-TREU) bzw. am Code gegengeprüft (A2).
> **Ort-Hinweis:** liegt bei den IST-Docs (cache-engine `docs/architecture/`, Nummer 34 nach 33). Masterplan B1 schlug
> alternativ Thesis-Basis 15 vor (User-Vorbehalt) — relozierbar; der IST lebt cache-engine-zentrisch, daher hier.
> **Quellen-Hierarchie bei Widerspruch:** Doc 30 §8.0 (3-Ebenen) > IST-Ledger > e2e-Abnahme > Code > diese Konsolidierung >
> Einzel-Docs 15–33 > Thesis-Basis 00–14 (SUPERSEDED). Phase D erweitert dieses Doc um die 85 Audit-Defekt-SOLL-Korrekturen (§9).

---

## §1 Das 3-Ebenen-Modell (Doc 30 §8.0, AUTORITATIV — code-verankert A2.5)

Drei strikt getrennte Ebenen (jede „Gattung"-Aussage in den Alt-Docs ist hiernach zu lesen):

1. **GATTUNG = ein Außen-INTERFACE = ein Prüf-Dock.** Es gibt **3**: **SearchAlgorithm / Container / Graph** (Doc 24 §8.8).
   Code: `AnatomyGattung`-Enum (real, A2.5 genus_binding_traits `gattung = AnatomyGattung::Container`).
2. **TIER-UNTERKLASSE = unter dem Interface, FESTER Achsen-Satz.** Hier lebt die feste Achsen-Konfiguration. 5 Tier-Unterklassen
   (Tier-Metapher): **SearchAlgorithm** (Säugetier, std::map-ähnlich, unter SearchAlgorithm-Interface) · **Set/Sequence/Adapter/
   View** (Vogel/Reptil/Wirbelloses/Pflanze, unter Container-Interface). Code: `AnatomyGenus`-Enum (historischer Name der Tier-
   Unterklassen-Aufzählung). **Invariante:** feste Slot-Zahl = ABI-Identität der Tier-Unterklasse (`AdHocComposition<19>`).
3. **ACHSEN = Organe der Tier-Unterklasse. KEINE Achse ist optional.** Die Interfaces ALLER Achsen werden in JEDEM Tier-Binary
   uniform getrieben; ein nicht-pufferndes/-prefetchendes Tier wählt einen KONKRETEN Durchreich-Algorithmus (NoBuffer/NoFlush/
   NonePrefetch/NoMigration/None) — KEIN „Achse weglassen".

**Per-Tier-Unterklassen-Slots (A2.5, code-verifiziert, GenusBound 5/5):** SearchAlgorithm **19** (17 Such-Achsen T0–T16 + queuing
q1/q2 T17/T18) (+3 Build-Achsen page_type/09b/12 außerhalb der Komposition) · Adapter **13** (12 delegiert/geteilt §28 + `inner_container`;
KEINE „ordering"-Achse, FIFO/LIFO = API-Nutzung §26.4) · Set **15** · Sequence **11** (10 + axis_growth) · View **7** (4 + extent/
layout/accessor). **Aktuell voll gebaut + verifiziert (BR-1..4): NUR die SearchAlgorithm-Tier-Unterklasse;** die übrigen 4 sind
GenusBindingTraits-Binding-Instanzen (`test_genus_binding` 5/5). queuing ist eine ACHSE (kein Interface, keine Gattung; Doku-14-§7
„q1/q2 = Organe" rehabilitiert, Doc-27-§0.1-Fehl-Präzisierung verworfen). **Befund 1 (Doc 30):** „22 Achsen" = 19 SA-Achsen + 3 Build.

## §2 Organ-Metapher + Permutation = Organ-Tausch (Doc 14, AUTORITATIV)

- **Achse = Organ** (Sub-Aufgabe jedes Algorithmus) · **Algorithmus (Tier) = Permutations-Konfiguration ALLER Achsen** ·
  **Permutation = genetisches Experiment** (Organe testweise gegeneinander tauschen). Bottom-Up: vom Tier (Algorithmus) zum Organ
  abstrahieren. Ein nicht-seziertes Tier steht AUSSERHALB des Systems (Doku 14 §3.1) — erst zerlegt bringt es seine Organe ein.
- **Reference-/Composition-Templates:** Original-Algorithmen = benannte Compositions = je 1 Punkt im Permutations-Raum
  (ArtComposition/HotComposition/…). `ArtComposition` vs `ArtPaperBindingComposition` unterscheiden sich in GENAU `search_algo`
  (16 Achsen identisch). Code: `AdHocComposition<T0..T18>` (A2.4, 19 named using-Slots, KEINE Template-Defaults).
- **Wurzel-Hierarchie (Doc 14 Teil 5):** `IExecutionEngine` (alles Ausmessbare; warm_up/reset/shutdown) → `IAnatomyBase` (Lebewesen,
  Topics/Achsen) **vs** `IVirusExecutionEngine` (Viren = Graphen/FFT/Crypto ohne Anatomie, Schwester nicht Tochter). Nur Lebewesen permutierbar.
- **Verantwortlichkeits-Trennung (Doc 14 §17, A2.3 code-verifiziert):** PermutationEngine = Anatomie-Generator (1 Anatomie = 1
  Permutation) · `SearchAlgorithmAnatomy<C>` = Organ-Container + `observe_all()`→ObserverAggregate (KEIN insert/lookup — die sind
  CEB-Commands via `AnatomyExecutionContext`) · CacheEngineBuilder = Mess-Orchestrierung + ABI-Loader + CMake-je-Permutation.
- **Technische-Identifier-Direktive (Doc 14 §41):** Code-Identifier technisch (`SearchAlgorithmAbiAdapter`, NICHT `MammalAbiAdapter`);
  Tier-Metapher NUR in Kommentaren/Doku.

## §3 B+-Experiment-Baum (Doc 26/27/29 + `experiment_tree.hpp`) — KERN des Achsen-Austauschs

- **Achsen = Baum-Ebenen** (`AxisLevel{axis, values[], is_static, …}`, static/dynamic gleichrangig). Pfad Wurzel→Blatt = `binary_id`
  = eine Tier-Binary (statische Rekombination). Blatt + dyn. Belegung = eine `ExperimentSetting` (Binary × eine dyn. Kombination).
- **Mixed-Radix-Bijektion `StaticBinaryView`:** `operator[](i)` ⇄ `flat_index(tuple)` = inverse Bijektionen. **ACHSEN-AUSTAUSCH =
  Ziffernwechsel:** im `tuple` NUR `tuple[d]` (Ebene d = Achse a) k→k' ändern → Geschwister-Tier (Diff in genau a), O(Geschwister).
  ⇒ **Achsen-Struktur/-Austausch gehört IN DEN BAUM (cache-engine), NIE flach im Eval-Tool** (zentrale Session-Lehre).
- **🔴 NIE voll materialisiert (C1060, empirisch):** Voll-`mp_product` über 19 Achsen sprengt den Compiler-Heap (eager build ~21 GB)
  → **nur EIN Blatt compile-time materialisiert** (on-demand BR-2/BR-4); Laufzeit-Baum zählt/strukturiert; Iteration = lazy Mixed-
  Radix-Odometer (O(Tiefe)); Build = nur K Pfade parallel. **Gate-1 (literal):** `binary_count() == ∏ mp_size(Enabled_i) ==
  PermutationEngine::count() == 137.594.142.720.000` (rein arithmetisch, Kardinalitäts-Identität, ohne Typ-Baum).
- **Die 4 Brücken (Doc 27, alle DONE; A2.6 code-präsent):** BR-1 registry→`AxisLevel`s (`build_all_axis_levels`) · BR-2 Blatt-Pfad↔
  reale `AdHocComposition<19>` (`composition_registry::register_from_engine` + EINE `serialize_composition_path<P>()`) · BR-3
  `NodeValue`→echter `NodeObserverSnapshot` (sparse value_map, nur GEMESSENE Knoten) · BR-4 generierte Binary→reale Anatomie
  (`render_adhoc_module_source` → `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC` → DLL → Loader → `dynamic_cast<IObservableTier*>`).
- **Knoten via Abstract Factory:** `StaticAxisNode` (compile-time → lädt EINE Binary) / `DynamicVariableNode` (Laufzeit-FOR-Schleife
  auf geladener Binary über `Algorithm_Resource_Control`, KEINE neue Binary). Inverse Paper-Signatur (KF-15): `multimap<Signatur,
  binary_id>` statt FNV1a-Fingerprint.
- **5 per-Gattung-PermutationEngines** (A2.6: set/sequence/view + SearchAlgorithm + anatomy_permutation_driver) — Cross-Genus
  type-unmöglich (disjunkte Achsen-Sets, Doku 14 §32 → getrennte Engines/Compositions/Observer).

## §4 4-Subsystem-Modell (Doc 10) + 3-Stufen-Prüfung (Doc 14/24 §8.9)

- **4 Subsysteme:** `messung_driver` (Diplomarbeit/Code, OUTER-LOOP/Auswertung) → **CacheEngineBuilder** (autonomes Plattform-
  Ausmess-System, App) → **CacheEngine** (Werkzeug-Bibliothek) ↔ **Prüfling/prt-art** (bidirektional: CE bindet Prüfling als
  Permutations-Struktur + Prüfling nutzt CE-Services). CEB+CE im selben Repo, 2 unabhängige Subsysteme. (M-Korrektur §0.4: CEB
  orchestriert CacheEngine UND prt-art als zwei ExecutionEngines via Command.) Orthogonal zum 3-Repo-Layer.
- **6-Stufen-Pipeline (kanonisch 16-Spalten-CSV, `workload_used`@idx3):** `01 sample → 02 messung_driver → 03 binary→csv → 04
  csv→latex → 05 diagram → 06 latex→pdf`. P1 done (E2E-Target `comdare_pipeline_e2e` → PDF, reale i7-1270P-Daten).
- **3-Stufen-Prüfung (Prüfling, Doc 24 §8.9):** Stufe 1 `comdare_perms_ce` (NUR CE-Achsen) · Stufe 2 `comdare_perms_<pf>` (Prüfling
  ERSETZT je Achse, sonst CE-Fallback) · Stufe 3 `comdare_perms_full_join` (Union non-redundant, A⋈B = „Schnabeltier"-Hybrid). **Regel
  der abstrakt-leeren Achse:** leere Prüfling-Achse reust ALLE CE-Algos → Stufe-2-Raum = ∏ über die LEEREN Achsen. EIN Prüf-Dock misst
  alle. **Prüfling ≠ Prüflings-Binary** (Prüfling = komplettes Achsen-Kompendium; Binary = EINE Rekombination). prt-art-Integration =
  CMake + Metaprog-Join (NICHT Header-Kopie); CE lädt Prüfling (`COMDARE_CE_PRUEFLINGE`), Richtung CE→lädt→Prüfling.

## §5 Mess-Modell (Doc 24 HYBRID + §8.8 Prüf-Dock) — 2 Pfade, 3 Dimensionen

- **HYBRID, ZWEI Pfade über DIESELBE Modul-Binary:** **Pfad A** = isolierte Achsen-Algos gegeneinander, IN der DLL via
  `IMeasurableWorkload::run_workload` (Synthetik-`lbuf`, A2.1 `do_seg19` :490) → host-seitige Aggregation + `f15_compare`. **Pfad B**
  = composite Tier, zentral host-seitig via ABI-Observer-Zugriff (`IObservableTier::tier_observe`, A2.1 :1143), zeit-/zustands-
  korreliert (Wall-Clock-Stempel je Snapshot). Mess-Konfiguration wählt den Pfad.
- **3 Mess-Dimensionen (Doc 24 §2.4):** §2.1 Tier-Wall-Clock (Füllstand-Kurven, r/w/d, RAM/Disk) · §2.2 Per-Achsen-Observer
  (`observe_all`) · §2.3 Achsen-Vergleich (Unit-Tests gegen vereinheitlichtes Interface vs. bekannte Algos, z.B. std::map). **Welche
  Achsen-Variante „besser" ist, entscheidet §2.3, NICHT der Latenz-Benchmark.**
- **PRÜF-DOCK (§8.8):** CacheEngineBuilder-SEITE für GENAU EINE Gattung — lädt + treibt Gattungs-API durch + misst Observer +
  persistiert. EIN Dock je Gattung (SearchAlgorithm/Container/Graph; KEIN Neubau, nur Benennung der `IObservableTier`+`Loader`+
  `drive_tier_observe_trace_abi`-Verdrahtung). MATCHING: Prüf-Dock↔Gattung (`genus()`) · Lastprofil↔Tierart (MEHRERE je Binary) ·
  Observer↔cmake-Messmodus.
- **3 PROFILE orthogonal:** (1) BUILD (statisch, configure-time: welche Binaries) ⊥ (2) LASTENPROFIL (runtime host-seitig: Testdaten+
  Op-Mix, YCSB A–F, KEINE CMake-Flags) ⊥ (3) COMPILE-RELEASE (`COMDARE_MEASUREMENT_MODE`, ob observe/memento einkompiliert). Build ⊥
  Lasten = kartesisches Kreuz (1 Binary × N Lastprofile ohne Rekompilation). Reproduzierbarkeit: `WorkloadGenerator` xorshift64,
  gleicher Seed → BIT-IDENTISCHE Op-Sequenz.
- **🔴 ZWEI-PHASEN-OP-SCHLEIFE (v5_design §4, PFLICHT für Mess-Gültigkeit):** pro Op GENAU 2×: `tier_save_all → op(warmup) →
  tier_rollback_all → op(measure, Wall-Clock + Observer)`. Eliminiert Pfad-Abhängigkeit der Latenz vom akkumulierten Zustand.
- **KONFORMITÄTS-GATE (v5_design §6):** import → **GATE gegen std::map** → (nur bei pass) messen; `conformance_gate.hpp` Runtime-Oracle.

## §6 Observer-Konsolidierung I1 + ABI-Konvergenz (Doc 31 + abhaengigkeitskette + design_observer; A2.2 code-verifiziert)

- **EIN konsolidierter POD `ComdareTierObserverSnapshot` (A2.2 :111, sizeof 1400):** `axis_stats[19][8]` (Per-Achsen-Observer, Schema
  `kV3AxisSchema` single-source) + `seg_ns[19]` (Pfad-B-Per-Achsen-Timing über die REALE Komposition) + 4 Meta. **Schema für ALLE 19
  Achsen befüllt** (Phase A+B 2026-06-04) — die „R5.B nur 2 Achsen"-Grenze ist im Schema überholt. `standard_layout` + `trivially_
  copyable` (memcpy). V1/V2/V3-PODs + Sub-IFs ENTFERNT; Versionierung via **ABI-Major 2→3** (Loader-Reject per Magic/Major-Mismatch).
- **🔴 ABI-Grund-Invariante:** Neue Mess-Fähigkeit NIE als virtuelle Methode ans vtable-Ende (alte DLL vs neuer Host → SEH 0xc0000005);
  immer eigenes Sub-Interface, das der Adapter ZUSÄTZLICH erbt (NICHT unter IAnatomyBase). Dreifach-Vererbung `SearchAlgorithmAbiAdapter
  : IAnatomyBase, IMeasurableWorkload, IObservableTier` (Reihenfolge eingefroren; A2.1 :116). **B1-Split:** `IObservableTier : public
  IDriveableTier` — Antrieb IMMER, `tier_observe`/Observer/Memento NUR unter `#if COMDARE_MEASUREMENT_ON` (Release-DLL = kein vtable-
  Slot, zero-overhead; A2.1 :121/A2.2). `dynamic_cast` 1× KALT je Modul (nie Hot-Loop) → no-dynamic-cast-Direktive bereits erfüllt.
- **🔴 Q1-SEQUENZ in der EINEN `tier_observe` (gegen Doppelzählung, A2.1 :1143):** (1) `fill_observer_v3` axis_stats-READ → (2)
  `fill_segment_timing_v3` seg_ns-Timing → (3) per-op-Reset. `destroy_fn` VOR `dlclose`. `host_compatible_with = major==host.major &&
  module.minor<=host.minor`.
- **Composition-Driver-Stand (Doc 29; A2.3):** `observe_all` hält **9 reale Achsen-Organe** (search_algo + telemetry + memory_layout +
  serialization + node_type via `ObservableXxx`-Hüllen, + cache_traversal/mapping/q1/q2) — der protected-CRTP-ctor-Block via Hüllen gelöst.

## §7 Memento Rev.2 (CoW) + Mess-Resume (Doc 33; A2.1 code-verifiziert)

- **CoW-Memento Rev.2** (ersetzte Undo-Log Rev.1 nach empirischer Widerlegung ~2× langsamer): `tier_save_all` = O(1) (2 Stat-POD-
  Snapshots); **lazy Vollkopie materialisiert VOR der ersten mutierenden Warmup-Op** (`cow_materialize_copy_`, A2.1 :640/:710/:722);
  `tier_rollback_all` = Kopie zurückspielen + Stat-Restore; Read-Perioden O(1). Deckt `search_organ_` + `container_` (T0+T6; T6-Allocator
  aus Datenzustand DERIVIERT). Sub-IF `IRollbackableTier` (nur Messung-AN). BuildVersion **cowmem-v1** (→ cowfix-v1 nächster Voll-Lauf).
- **Mess-Resume (Doc 33 §5; A2.6):** Granularität = Tier-Binary. Config-Stamp `result.csv.stamp` (BuildVersion + n_ops + seed + records +
  dims + rows) NUR wenn Binary VOLLSTÄNDIG gemessen. **BuildVersion im Stamp ⇒ copymem-v1-Ergebnisse werden im cowmem/cowfix-Lauf NIE als
  fertig gewertet** (Stale-Falle vermieden). Skip-Check vor Laden; Header-Schema-Drift-Schutz. `resume_completed_binaries` (Default AN).
- **memento_all-Vollständigkeit (v5_i8):** I8 gegenstandslos — KEIN echtes Disk-I/O im Achsen-Code; „Disk"-Achsen (io/serialization/
  migration) = stateless Compile-Time-Deskriptoren → `EmptyMemento` no-op = korrekt; Vorwärts-Kontrakt (MementoAxis) gebaut.

## §8 Pflicht-Achsen-Satz + F15-Mission + Statistik

- **19 SearchAlgorithm-Achsen (T0–T18, A2.4):** search_algo · cache_traversal · mapping · path_compression · node_type · memory_layout ·
  allocator · prefetch · concurrency · serialization · telemetry · value_handle · isa · index_organization · io_dispatch · migration_
  policy · filter · queuing_q1 · queuing_q2. (+3 Build-Achsen page_type/09b/12 = Codegen-Varianten DERSELBEN Binary.) Goldstandard-
  Achsen-Aufbau = `axis_06_allocator`-Vorbild (10 Pflicht-Komponenten je Achse). is_original-Klassen A (echtes Linking: allocator/
  search_algo/q1) vs B–E (Re-Impl + `is_original=false` ehrlich, Habich-konform; Paper-Map = Doc 18 autoritativ, 110 Algos).
- **F15-Mission:** schnellste Rekombination im Permutations-Raum finden + ALLE Achsen-Permutationen studieren. Autoritative F15-Quelle =
  6 named Observable-Organ-Compositions (Monolith-AdHoc-Pilot SUPERSEDED); real auf i7-1270P (p50 479–653 ns). V1–V4 Engine-Choice-Dim
  (NO-CE/Static/Informed/Adaptive). **NB:** Die 320-FullPilot-Bias-Matrix (Original-Mission) ist ein SEPARATER, größerer Mess-Aufbau —
  Verhältnis zur 6-Organ-F15-Quelle in E zu klären.
- **🎯 Bias-Bruch-Matrix (Original-Mission, Doc 32):** jedes Paper wählt ein Heimspiel-Lastprofil → um den Bann zu brechen, ALLE
  Lastprofile (14 LP01–LP14 → 21 XMLs: 14 + LP06-neg%×5 [+ LP11-read_ratio]; ⚠️ Audit-Lücke „Katalog ≠ 21 XMLs" in D klären) über ALLE
  Tiere (320). Workload = dynamische Achse 2 im Baum. 5 Bias-Kategorien (static-read-Trie/negsweep/zipfian-cache/write-update/prefetch-HW).
- **F15-Statistik-Triade (Doc 22, `f15_compare`):** Median-(p50)-Ranking + Mann-Whitney-U (robust, Holm-FWER, markiert `[DISKREPANZ]` vs
  Welch-t) + Cliff's δ (Effektmaß = „WIE VIEL bringt die Achse?"). **Ehrliche Mess-Limits:** Wall-Clock NICHT bit-reproduzierbar (Seed
  steuert Keys nicht CPU-Timing; Signifikanz+Extrem-Ordnung STABIL); allocator wall-clock-auflösbar (~2–3×); memory_layout SUB-NOISE
  → R5.D PMC (extern-gated).

## §9 Mess-Echtheit: DER eine echte Architektur-Defekt (Befund 2 / Doc 30 §6) — A2.1 + A3

- **Befund 2 (Doc 30, code-verifiziert A2.1):** `SearchAlgorithmAbiAdapter` hält ZWEI getrennte Speicher — `search_organ_` (Monolith
  `Composition::search_algo`, liefert SEARCH-Metriken A2.1 :788) + `container_` (NodeChunkedStore, liefert STORAGE-Achsen via
  `organ_observe_*` A2.1 :879). **Q2-Schritt-1-3 GEFIXT** (NodeChunkedStore → node-abhängig, `alloc_cnt=ceil(n/cap)`); **Q2-Schritt-4
  OFFEN:** SEARCH-Zähler kommen weiter aus dem Monolith → Such-Organ beschattet node_type/memory_layout, Tiere routen NICHT uniform
  durch alle Organe. **Das ist der dominante Mess-Echtheits-Defekt (Audit-K5+K6); Meta-Lehre #3 macht ihn mission-kritisch** (Achsen-
  Austauschbarkeits-Belege dürfen nicht Apparat-Artefakt sein). SOLL: search_organ_ entfällt; Such-Strategie ALS Traversal über DENSELBEN
  container_-Store; perm_runner→V2-POD (node_*-Felder). = E-Kern-Aufgabe (cowfix-v1, DLL-Neubau).
- **ZWEI Observe-Mechanismen (A2.3, in E zu vereinheitlichen):** `SearchAlgorithmAnatomy::observe_all` (in-process, 9 Organe) vs
  `abi_adapter::fill_observer_v3` (ABI-POD aus search_organ_+container_) — der gemessene ABI-Pfad nutzt NICHT die Anatomie-observe_all.
- **85 Audit-Befunde** (K1–K10 + 36 Major + 17 Minor + 8 Meta-Lehren) sind in `20260613-A3-audit-soll-abgleich.md` je Architektur-Konzept
  + IST-Status (K2/K5a/K7a code-gefixt) + E-Wellen-Saat verankert. **Phase D arbeitet sie Befund-für-Befund gegen die JSONs durch +
  erweitert dieses §9 um die vollständige SOLL-Korrektur-Tabelle.**

## §10 IST-Substanz-Landkarte (Ledger §e/§b/§c/§d) + offene/gated Punkte

- **§e VERIFIZIERT-ERLEDIGT (Großteil):** V5 I1–I10 (§a.V5) · F.2 17/17 physische Achsen · F.3 17 Concepts · F.4 Tools-Facade · R6.1–6.5
  (IObservableTier+POD, Prüf-Dock) · R7.2/7.3/7.4 · R8 (Prüfling 3-Stufen) · Umstufung-A/B (EnabledStrategies = 4 Organe, 13 Monolithen
  USE=0) · s4 · BR-1..4 · KF-9..16b · G.1 · E10. ⇒ Architektur-Substanz steht; Mess-Pfad real.
- **§b EXTERN/TOOLCHAIN-GATED:** Vendor-Allokatoren (jemalloc/tcmalloc/hoard/scalloc — lokal nicht baubar, Specs geliefert) · R5.D/#26
  PMC-HW-Counter · C1/C2 Cluster/Grace-Hopper · E11-Facade (gated #22/V42) · F.6-Phase-C (23 Legacy-Header nach Habich-Termin löschen).
- **§c USER-MANUELL:** D1 Diplomarbeit-Kapitel-Text · D2 Bausteine-Matrix. **§d V42-FUTURE:** Set/Sequence/View/Graph produktiv ·
  R7.6.c is_original-Linking · Naming-Refactor-Backlog · #22 Submodule-Option-B.
- **Mission baut AUF dieser fertigen Substanz auf;** die Audit-Befunde (D) sind Korrekturen AM bestehenden Mess-Pfad, keine Neubauten.

---

## §11 SUPERSEDED-Auflösung (Masterplan B2) — was NICHT mehr gilt

- **Thesis-Basis 00–14 (SUPERSEDED-Banner 2026-05-31):** das **F1–F29-ICacheStrategy- / S1–S30-Such-Engine- / C1–C12-Sub-Engine- /
  4-Ebenen-Strategie-(A/B/C/D)- / alte-11-Achsen- (PAGE/NODE/…/TELEMETRY) / 3-Säulen-(IExecutingEngine/Säule-A/B)-Vokabular** ist ALTER
  Planungsstand. IST = das Achsen/Organ-Modell (§1–§9). F1–F29 etc. dienen nur als Begriffs-/Paper-Mapping-Historie. In keiner B-/E-Arbeit vermischen.
- **„5 Gattungen" (Doc 27/28/29/14 alt)** = 5 **Tier-Unterklassen** (Gattung = Interface, §1). **„queuing = Container-Gattung" (Doc 27
  §0.1)** = KATEGORIENFEHLER, verworfen (queuing = SA-Achse). **„Adapter = inner+ordering"** = verworfen (13 §28-Achsen, KEINE ordering).
- **V110/120/130-Merge (CLAUDE.md-Block)** betrifft das COMDARE-CLUSTER (anderer Workstream), NICHT diese Thesis-Architektur.
- **Monolith-AdHoc-48-DLL-Pilot** = SUPERSEDED (autoritative F15-Quelle = 6 named Organ-Compositions). **Undo-Log Rev.1** = verworfen (CoW Rev.2).

## §12 Original-Mission gegen dieses Modell verankert (Masterplan B3)

Die Original-Mission (Goal-A0: **Bias-Bruch-Matrix Messung → Audit-Abarbeitung → interpretierbarer LaTeX-Appendix**) — jede Teilaufgabe
hat einen architektur-konformen SOLL-Ort:

| Mission-Teilaufgabe (User-Detail) | Architektur-Soll-Ort (dieses Doc) | Direktiven-Korrektur |
|---|---|---|
| **Ausgabe = Testdaten-Konfig × Tier** (Matrix 320 Tiere × 18 dyn × 21 Lastprofile) | **Build ⊥ Lastenprofil = kartesisches Kreuz (§5)** über den **B+-Baum (§3)**: Tier = Static-Pfad/binary_id, Lastprofil = dynamische Achse 2 (DynamicVariableNode, FOR-Schleife auf geladener Binary) | NICHT flach im Eval-Tool — die Matrix lebt im Baum |
| **Je Interface-Funktion Verarbeitungsdauer (ns/op) auf z-Achse eines 3D-Diagramms** | **`seg_ns[19]` Pfad-B-Per-Achsen-Timing (§6)** + Tier-Wall-Clock r/w/d (§5 §2.1); Diagramm-Gen Stufe 05 (§4-Pipeline) | z = ns/op je Achse/Interface-Fn aus dem konsolidierten POD |
| **🎯 Achsen-Austauschbarkeits-Belege** (Wechsel EINER Achse → Diff gegen alle anderen Tiere als Tabelle) | **B+-Baum-Ziffernwechsel (§3): `flat_index`-Diff in genau einer Achse = Geschwister-Tier.** Diff-Auswertung = inverse Signatur-Projektion (KF-15 multimap) über die REALEN Compositions | **DIE Session-Kern-Lehre: gehört IN DEN BAUM, NIE flache Tupel-Kombinatorik** (L1/L2-Fehler, Phase C revert). **Cross-Achsen-Constraint (§3/e2e-Abnahme): `AdHoc<Organ,Default>` ill-formed — Organe brauchen kompatible Begleit-Achsen (named Compositions)** |
| **Belege müssen ECHT sein (nicht Apparat-Artefakt)** | **Meta-Lehre #3 (A3): Diff-Beweise brauchen Nachweis VERSCHIEDENER Pfade.** ⇒ blockiert auf **Befund-2/Q2-Schritt-4 (§9):** solange `search_organ_`-Monolith node/layout beschattet, sind Achsen-Diffs teils Apparat-Artefakt | **Mission-kritischer E-Kern: Q2-Schritt-4 (volle Such-Delegation) MUSS vor den finalen Austauschbarkeits-Belegen** |
| **Appendix IMMER alle Werte + ehrliche Limitierungs-Tabelle** | sparse `value_map_` (§3, nur gemessene Knoten) → Stufe 04 csv→latex (§4); **ehrliche Mess-Limits (§8): WC nicht bit-reprod., layout sub-noise→PMC, RC nominal/K1, cowmem=Copy-Pfad** als Daten-Vorbehalte | Done-Kriterium (b): jeder ungefixte Audit-Befund (A3/§9) als Limitierung ausgewiesen |
| **ZIH-diplominf-Vorlage · NUR relative Pfade · EIN Experiment → fertige bilinguale PDF** | 6-Stufen-Pipeline (§4) `comdare_pipeline_e2e` → thesis `build.ps1 -Lang` (EN≡DE); ZIH `zihpub.cls` unangetastet | Goal §0.4 — git-clone-fest |

**E-Auslegung (B3→E1, audit-fundiert A3 §4):** Die Mission ist EINE geordnete E-Aufgaben-Liste, jede gegen Doc 34 + die 85 Befunde
verankert: **E-Welle-A2 (Apparat-Reinheit: Q2-Schritt-4 §9 + K5/K6 + CoW-für-320/K3 + seg_ns-n>1/K9 → cowfix-v1, DLL-Neubau) = das
Herzstück** (ohne echte Achsen-Pfade keine gültigen Austauschbarkeits-Belege). Dann E-Welle-A1 (Resume/Stamp/K8), E-Welle-A3 (RC/
Scrambling/Gate/SelectMode), E-Welle-A4 (Pattern-Hygiene/K10). Reihenfolge Goal §2.5.5. **Jede E-Aufgabe = eine 1M-Session, gegen
beide teuren Audits gegengeprüft, Achsen-Austausch IM Baum.**

---

> **Status:** Phase B erfüllt — **B1** (dieses Doc §1–§10) + **B2** (SUPERSEDED-Auflösung §11) + **B3** (Mission-Verankerung §12) +
> **B4** (Goal-Doc verweist auf dieses Doc 34, separat). **Phase D** erweitert §9 um die vollständige 85-Befund-SOLL-Korrektur-Tabelle.
> Ein neuer Bearbeiter kann aus §1–§12 korrekt arbeiten, OHNE die 48 Quell-Docs einzeln zu lesen. **Nächste Masterplan-Phasen: C
> (Aufräumen: L1/L2 revert + A2a-re-eval + Memory-Clean) → D (85 Audit-Befunde Befund-für-Befund) → E (Mission, 1 Aufgabe/1M-Session).**
