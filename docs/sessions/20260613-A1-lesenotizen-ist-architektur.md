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
- ✅ Thesis **03_konzepte_saeule_a + 04_konzepte_saeule_b** (SUPERSEDED-Konzept-Vokabular: F1–F29 / 4-Ebenen-Strategie
  A-B-C-D / 33-Paper-Map / Säule-B Plattform-Auto-Discovery [Discover→Measure→Classify→Publish→Bind] + 28 Concept-Klassen
  + ~80 Heuristiken + Block-AO-Maschinen Ryzen-9950X3D/i9-14900KS — Kontext, NICHT IST)
- ⬜ OFFEN: Thesis 01,05,06,07,08,12,13 + Rest 11/14 · cache-engine **24 (Messmodell 2-Dim) · 26 (B+-Baum-Prosa) ·
  27 (Baum-4-Brücken) · 29 (Baum-Generik) · 31 (Observer-Konsol.) · abhaengigkeitskette · messarchitektur_design_observer ·
  messarchitektur_v5_design/_entscheidungen/_drei_profile/_i8** + 15–23/25/28/32 · A2 Rest-Code-Pre-Read
  (anatomy/composition/permutation_engine/perm_runner/iterator) · A3 Audits-Soll-Abgleich.
  (Beide IST-Docs + Doc 30 + Doc 33 ✅ — die Konsolidierungs-Basis B steht; Rest = Konzept-/Detail-Kontext.)
