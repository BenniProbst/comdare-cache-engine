# Goal-V6 Lücken-Ledger — SOLL (Doc 28) ⇄ IST (Code, literal) — Single-Source der offenen Punkte

> **Goal-V6 Phase B (Konsolidierung).** Lebendiges Ledger: gleicht die SOLL-Kartographie (Doc 28) gegen den
> literal verifizierten IST-Code ab. Jede Lücke: SOLL-Quelle (Doc:§) · IST-Code-Stand (Datei:Zeile) · literales
> Verifikations-Kriterium · Aufwand · Status. IST-Stände stammen aus 3 frischen adversarial-präzisen
> Planungs-Agenten dieser Session (a0c96bb #74 · a94f574 Gattungen · a8de276 Skalierung), je gegen den realen
> Code gegengeprüft. **Kein Eintrag wird „erledigt" ohne literale Tool-Ausgabe** ([[feedback_no_success_marks_without_literal_output]]).
> Reihenfolge der Abarbeitung + Sprints: siehe Planungssession `20260602-goal-v6-planungssession-gesamtplan.md`.

## §0 Solide IST (literal belegt, NICHT offen) — Referenz-Anker

| Gate | Bedingung | Beleg | Status |
|------|-----------|-------|--------|
| Gate-1 | `binary_count() == ∏ mp_size(Enabled_i)` | `test_br1_full22_count`: == 137.594.142.720.000 (22 Achsen, arithmetisch, OOM-sicher) | ✅ solide |
| Gate-2 | 22 Achsen als Baum-Ebene, volles Enabled-Inventar | `test_br1_full22_count`: 22 distinkte Blöcke, `block_id()==axis()` | ✅ solide |
| Gate-5 | inverse Signatur-Projektion (KF-15) über reale Kompositionen | `test_br_kf15_real`: 8 reale Blätter → gepinnte Signatur | ✅ solide |
| Gate-6 | belegt Doc 26/27 + Session-Doku | Doc 26/27/28 + Audit-Session-Doku | ✅ solide |
| BR-4 (SearchAlgorithm) | Blatt → reale DLL → Loader → `dynamic_cast<IObservableTier*>` + observe | `br4_load`: organ_count==17, observe 256/256/256 (1 Pilot-DLL) | ✅ solide (Pilot) |

**Gattungs-Bindung IST:** 2 von 5 (`test_genus_binding`/`test_container_genus`: `GenusBound<SearchAlgorithm>==true`, `GenusBound<Adapter>==true`, Set/Sequence/View==false). `GenusBindingTraits<Adapter>::slot_count==1`.

---

## §1 Lücken-Ledger (offene Punkte)

### L-73 — `provision_all` results-Vektor O(K) statt O(∏)  · Task #73 · Aufwand: S (klein)
- **SOLL:** Doc 28 §6 (#73 Tech-Debt) + Doc 26 §2 (OOM-Disziplin „nie alle ∏ vorhalten").
- **IST (literal):** `builder/build_orchestrator/build_orchestrator.hpp:153` `std::vector<BuildResult> results(view.size());` → bei `view.size()==1.4e14` sofortiger OOM. Worker indizieren `results[i]` über globalen View-Index (`:170/:188/:223`); `next.fetch_add` läuft bis `view.size()`.
- **Verifikations-Kriterium:** Unit-Test mit synthetisch großer `view.size()` (z.B. 1e12 Stub-Levels) + `BuildSelection` mit 5 Indizes → `results.size()==5`, kein OOM, Stub-CompileFn nur 5× aufgerufen; bestehender `test_kf16_build_orchestrator` bleibt grün (Rückwärts-Overload).
- **Abhängigkeit:** L-SEL (BuildSelection). **Status:** OFFEN.

### L-SEL — StaticBinaryView (∏-lazy) → endliche reale Build-Liste  · (Teil #73-Umfeld) · Aufwand: M
- **SOLL:** Doc 28 §5 („reale Cluster-Build-Menge ≪ ∏; BUILD-PROFIL/Enabled-Flags/Coverage wählt"), Doc 22 §4 (1-wise 25 statt 2125), messarchitektur_v5_drei_profile §1.
- **IST (literal):** ∏-Reduktion existiert HEUTE zweigleisig getrennt: (a) configure-time `Enabled*`-Flags (kleines ∏); (b) `anatomy/combinatorial_coverage.hpp` 1-wise — aber NUR im `apps/adhoc_emitter/main.cpp:106-178`-Pilot, NICHT am `StaticBinaryView`. Es fehlt EINE explizite Übersetzung „Lazy-View(∏) → endliche Index-Selektion".
- **Verifikations-Kriterium:** `select_one_wise(48-Pilot-View).size() == max(AxisLevel.values.size())`; jede Achse-Variante ≥1× im Index-Satz (Identitäts-Test gegen `combinatorial_coverage`). NIE `for_each_binary`/`pinned_signature_index()` auf vollem ∏ (beide O(∏)).
- **Neue Datei:** `builder/experiment_tree/coverage_selection.hpp` (`BuildSelection` POD: `vector<size_t> indices` + Provenienz; `select_full/one_wise/by_pinned_signature/explicit`). **Status:** OFFEN.

### L-77 — ceb_generator reale Anatomie ODER Anspruch präzisieren  · Task #77 · Aufwand: S (Doku) + M (Brücke)
- **SOLL:** Doc 28 §3 BR-4-Kriterium (generiert→gebaut→Loader→`dynamic_cast<IObservableTier*>`+`tier_observe` über REALE Komposition), Doc 28 §6 (#77), Doc 26 §7 KF-8.
- **IST (literal):** `builder/experiment_tree/ceb_generator.hpp:52-82` emittiert STUB: `#define COMDARE_PERM_<AXIS>_IS_<VALUE>` (von niemandem konsumiert) + leerer `extern "C" perm_<id>_run` (gibt `0.0`). KEIN Umbrella-Include, KEIN ADHOC-Makro. Der REALE BR-4-Pfad ist `builder/codegen/adhoc_emitter.hpp` (`render_adhoc_module_source` → `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ>)`). **Diskrepanz:** Task #70 `[completed]` betitelt „ceb_generator emittiert ADHOC" — trifft nur auf adhoc_emitter zu.
- **Verifikations-Kriterium:** (4.B) ceb_generator-Header-Doku + Doc 26 §7 KF-8 + Task #70-Titel korrigiert: ceb_generator = Pfad-Manifest/Diagnose-Shell; reale Anatomie = adhoc_emitter. (4.A) `generate_all_real<Engine>` delegiert an `emit_adhoc_modules<Engine>`.
- **Risiko:** NICHT zur vollen String→Typ-Tabelle ausbauen (Direktive „kein Runtime-Switch"). — eingehalten (Brücke ist typ-getrieben via Engine).
- **✅ ERLEDIGT (D3, test_d3_ceb_generator literal grün, min_free 9.34 GB):** (1) `generate_perm_source` = Diagnose-Shell — hat `COMDARE_PERM_`-#defines + `perm_<id>_run`-Stub (0.0), KEIN ADHOC-Makro. (2) `generate_all_real<FakeEngine>` emittiert 1 Modul mit `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC` + `all_axes_umbrella.hpp` + `anatomy_manifest.txt`. Beide Pfade output-disjunkt. Header-Doku + Doc 26 §7 KF-8 + Task #70-Titel präzisiert.

### L-75 — Container-Gattung vervollständigen: q2-Slot + DLL-Pfad  · Task #75 · Aufwand: M (3–5 Tage)
- **SOLL:** Doc 28 §2 (Adapter: 2 Slots q1+q2 + axis_inner), Doku 14 §26.4/§28/§32.2, Doc 24 §8.8 (neue Gattung = neues Sub-Interface + neuer V1-POD, NIE IAnatomyBase mutieren).
- **IST (literal):** `anatomy/container_anatomy.hpp` `ContainerComposition` = 1 Slot (Q1 buffer); `GenusBindingTraits<Adapter>::slot_count==1`; `ContainerDock` nur in-process (kein DLL-Pfad analog BR-4). q2-Inventar vorhanden: `topics/queuing/axis_q2_queuing/` 5 FlushPolicies (Eager/Watermark/Lazy/Timed/AdaptiveLsm).
- **Verifikations-Kriterium:** (1) `GenusBindingTraits<Adapter>::slot_count==2`; `ContainerAnatomy<ContainerComposition<FifoBuffer,WatermarkFlush>>::organ_count()==2`. (2) in-process: `observer.full_flush_count>0` bei Watermark. (3) **DLL-Round-Trip:** emittiert→gebaut→`AnatomyModuleLoader::load()==status_ok`→`anatomy()->genus()==Adapter`→`dynamic_cast<IContainerTier*>≠null`→put/get über DLL-Grenze→`tier_observe_container` liefert `put_count==N`.
- **Neue Dateien:** `anatomy/container_tier.hpp` (IContainerTier + ContainerObserverSnapshotV1), `anatomy/container_abi_adapter.hpp`, `COMDARE_DEFINE_CONTAINER_MODULE` in `include/cache_engine/abi/container_module_abi_v1.hpp`.
- **Loader bleibt UNVERÄNDERT** (gattungs-agnostisch, gibt `IAnatomyBase*`, Gattung runtime via `genus()`).
- **✅ ERLEDIGT (D4a in-process + D4b DLL, literal grün, min_free 9.07–9.50 GB):**
  - **D4a:** ContainerComposition<Q1,Q2> (Q2 Default ContainerNoFlushPolicy, entkoppelt), ContainerAnatomy organ_count()==2, put() treibt Flush; GenusBindingTraits<Adapter> slot_count==2. test_container_genus: WatermarkFlush 75%@cap8 → full_flush_count==3; test_container_dock + test_genus_binding rückwärts-kompatibel grün.
  - **D4b:** IContainerTier + ContainerObserverSnapshotV1 (ABI-POD, standard_layout) + ContainerAbiAdapter (erbt IAnatomyBase+IContainerTier) + COMDARE_DEFINE_CONTAINER_MODULE. test_d4b_container_adapter (in-process dynamic_cast). **test_d4b_container_dll (3-Phasen DLL-Round-Trip):** AnatomyModuleLoader::load==status_ok → genus()==Adapter, organ_count==2, `dynamic_cast<IContainerTier*>`≠null → put 20 + Q2-Watermark-Flush (full_flush=1) über die REALE .dll-Grenze. DERSELBE Loader wie SearchAlgorithm (keine Loader-Änderung, Doc 24 §8.8). **Status: ✅**

### L-74a — page_type/09b/12 als echte Build-Varianten DERSELBEN 17-Slot-Binary  · Task #74 · Aufwand: M (3–4 Tage)
- **SOLL:** Doc 27 §0.1 (Zeile 59: „SearchAlgorithm-Sub/Build-Varianten (3) … modifizieren DIESELBE Binary … NICHT eigene Gattung"), Doc 28 §1 Zeile 46-48 (Spalte „Build/Definition-SOLL"), Doc 15 §6 (Cross-Constraint 96→~25).
- **IST (literal):** Die 3 Achsen sind reine `static constexpr` Compile-Konstanten (`axis_01_page_type_concept.hpp:25-32`, `axis_09b_..._concept.hpp:18-28`, `axis_12_..._concept.hpp:19-28`). Der Emit-Pfad `adhoc_emitter.hpp:22-44` gibt NUR die 17 FQ-Typen → die Binary trägt 09b/12/page_type NICHT als Build-Parameter. BR-1 reflektiert sie bereits als Ebene (`registry_to_axis_levels.hpp:63-65,92-94`).
- **Entscheidung (Doc 27 §0.1 vorgegeben, NICHT neu):** KEINE `AdHocComposition<20>` (17 = Gattungs-Invariante bleibt). Die 3 als zusätzliche Codegen-Parameter (`using`-Aliase + `static_assert` Concept) in die Modul-`.cpp` einbacken; Build-Identität via 3 `extern "C"`-Inspection-Symbole (`comdare_build_page_kind/simd_width_bits/hw_cache_line`).
- **Verifikations-Kriterium:** `test_gate4_buildvariant_emit`: aus 1 Blatt emittiert → `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(<17+3 FQ>)` → DLL gebaut → `comdare_build_simd_width_bits()==256 (Avx2)` vs `==512 (Avx512)` (2 Varianten DERSELBEN 17-Komposition unterscheidbar). Cross-Constraint: `(4 ISA × 8 SE × 3 HW)=96 → ~25-30` literal via `mp_size`.
- **Neue Dateien:** `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT` (anatomy_module_abi_v1.hpp), `build_variant_args<PT,SE,HW>()` (adhoc_emitter.hpp), `builder/experiment_tree/hw_cross_constraint.hpp`. **Status:** OFFEN.

### L-74b — Eigener „Observer" der 3 Build-Achsen = je-Knoten-getragene + ABI-gezogene Definition  · Task #74 · Aufwand: M (2 Tage)
- **SOLL:** Doc 27 §3 (Zeile 156-159: „Hardware-Achsen = reine Build-Time-Konstanten … evtl. KEIN Laufzeit-Observer → ‚Achsen-Definition statt Observer' je Knoten, aber EXPLIZIT dokumentiert"), Doc 28 §4 Punkt 4 („22 Observer, NICHT 17").
- **IST (literal):** `axis_observer_classification.hpp:42-66` klassifiziert die 3 als `DefinitionOnly` — nur statische Tabelle, KEIN je-Knoten-Träger, KEIN ABI-Pull.
- **Empfehlung (begründet):** „Definition-statt-Observer" IST korrekt (kein Laufzeit-Zustand: alle API `static constexpr`; Build-Konstante = Identität, kein Mess-Ergebnis; Doc 27 §3 sieht es vor). Task-Titel „echter Observer" präzisiert zu „echte je-Knoten-getragene + ABI-gezogene Definition". KEIN Pseudo-Observer (Anti-Fake).
- **Verifikations-Kriterium:** `test_gate4_definition_per_node`: (a) `build_variant_definition<DenseByte,Avx512,X86_64>().vector_width_bits==512`; (b) gemessener Knoten `build_def_real==true`, ungemessener `==false` (SPARSE-Kontrast); (c) 22-Vollständigkeit: 17 Observer + 3 Definition + 2 Container == 22, keine fällt weg.
- **Neue Dateien:** `anatomy/build_variant_definition.hpp` (BuildVariantDefinitionV1 POD), `builder/experiment_tree/build_variant_definition_reader.hpp`; `NodeValue` += `BuildVariantDefinitionV1 build_def{}; bool build_def_real=false`. **Status:** OFFEN.

### L-74c — R5.B-Voll-Operativität: die 13 heute passiven der 17 Komposition-Achsen real messbar  · Task #74 · Aufwand: L (4–6 Tage)
- **SOLL:** Doc 24 §2.2/§5.5/§8.5, Doc 28 §1 Spalte „Observer-SOLL".
- **IST (literal):** `node_value_measurement.hpp:11-13` + `abi_adapter.hpp:268-298` (`tier_observe`): operativ getrieben sind nur **search_algo + allocator**; Pfad-A `run_workload` (`:215-218`) treibt zusätzlich memory_layout (`scan_field_sum`) + serialization (`serialize_scan`), aber diese fließen NICHT in `tier_observe`/`ObserverAggregate`. `observe_all()` (`search_algorithm_anatomy.hpp:62-72`) hält nur `axis_search_algo_`.
- **Befund:** R7.2/7.3/7.4 (#5/#6/#7 completed) = Organ-Body-Ausbau erledigt; was FEHLT = der **Composition-Driver** (Verdrahtung dieser Organe in tier_observe/observe_all).
- **Operativ machbar:** memory_layout (⚠️ PMC nötig, R5.D), serialization, telemetry (statistics-tragend), node_type (KF-6). Dauerhaft Deskriptor: path_compression/prefetch/concurrency/value_handle/isa/index_organization/io/migration/filter.
- **Verifikations-Kriterium:** `test_gate4_r5b_operative`: getriebenes Tier → `tier_observe` POD-Felder >0 für search+alloc UND ≥layout_scan_checksum>0 + telemetry_*>0 (Delta vor/nach Treiben, kein Stub); `observable_axis_count` == literal getriebene Zahl (≥4, exakt belegt); ungetriebene == 0. **R5.D PMC** ehrlich als „operativ-PMC-pending" markieren, NICHT als gemessen.
- **Ehrlicher Endzustand „22 Observer":** 17 SearchAlgorithmObserver (2 voll + 2-4 erweiterbar + Rest Deskriptor) + 3 Definition + 2 Container == 22. „alle 17 voll operativ" ist NICHT Ziel. **Status:** OFFEN.

### L-76a — Set-Gattung (Vogel, K-only, 14 Achsen)  · Task #76 · Aufwand: M–L (4–6 Tage)
- **SOLL:** Doc 28 §2 (Set: 14 ohne mapping/value_handle/inner), Doku 14 §27.2/§28/§32.2.
- **IST (literal):** 0 Treffer für SetAnatomy/SetComposition/SetDock im Code.
- **Pre-Sprint-Klärung K-A:** Doku 14 §32.2 + Doc 28 = 14; Doku 14 §28-Tabelle = 15 ✓ (filter strittig). Vor D-2 durch genaues Doku-14-Lesen auflösen (NICHT User fragen — „bei Unklarheit Planrunde").
- **Verifikations-Kriterium:** `IsSetComposition<…>` + `SetAnatomy::organ_count()==14` + `binary_count()==∏ mp_size(14 Enabled)` + DLL-Round-Trip (`genus()==Set`, `dynamic_cast<ISetTier*>≠null`, `contains` nach `insert`==true).
- **Neue Dateien:** `anatomy/set_{composition_concept,composition_factory,observer_aggregate,anatomy,tier,abi_adapter,permutation_engine}.hpp` + `GenusBindingTraits<Set>` + `pruef_dock/set_dock.hpp`. **Status:** OFFEN.

### L-76b — Sequence-Gattung (Reptil, V-indexed, 9 + axis_growth)  · Task #76 · Aufwand: L (6–9 Tage)
- **SOLL:** Doc 28 §2 (Sequence: 9 + axis_growth), Doku 14 §26.1/§28/§32.2.
- **IST (literal):** 0 Treffer; `axis_growth` existiert NICHT (muss als NEUE Achse nach Goldstandard-Checkliste angelegt werden, Vorlage axis_06_allocator).
- **Pre-Sprint-Klärung K-B:** 9 (§32.2) vs 10 (§28-Tabelle). Vor D-3 auflösen.
- **Verifikations-Kriterium:** `GrowthPolicy<DoublingGrowth>` + `SequenceAnatomy::genus()==Sequence` + DLL-Round-Trip (`push_back` N → `tier_size()==N`, `tier_at(i)` korrekt).
- **Neue Achse:** `topics/sequence/axis_growth/…` (10 Pflicht-Komponenten: GrowthPolicy-Concept `next_capacity/growth_factor`, Wrapper Doubling/GoldenRatio/FixedChunk/Exact, Registry, flags.hpp.in). + Sequence-Gattungs-Kette. **Status:** OFFEN.

### L-76c — View-Gattung (Pflanze, non-owning, 7 + axis_extent/layout/accessor)  · Task #76 · Aufwand: L (7–10 Tage)
- **SOLL:** Doc 28 §2 (View: 7 + axis_extent/layout/accessor; non-owning kein insert/erase/alloc/concurrency), Doku 14 §26.6/§28/§32.2.
- **IST (literal):** 0 Treffer; 3 neue Achsen (extent/layout/accessor) nötig (mdspan-Bezug).
- **Pre-Sprint-Klärung K-C:** 7 (Doc 28/§32.2) vs 4 geteilt (§28-Plant). Plausibelste Lesart: 7 = 4 geteilte (memory_layout/telemetry/value_handle/isa) + 3 eigene. Vor D-4 auflösen.
- **Verifikations-Kriterium:** `genus()==View` + Compile-Fehler-Test dass `tier_insert` NICHT existiert (API-Asymmetrie) + DLL-Round-Trip (`bind(extern_buf)`→`tier_read(i)`==extern_buf[i]).
- **Risiko:** non-owning-Lifetime über DLL-Grenze (Dock muss Puffer-Owner sein, Lebensdauer > View). **Status:** OFFEN.

### L-76d — Viren produktiv (IVirusExecutionEngine, Graph-Algos ohne Achsen)  · Task #76-Umfeld · Aufwand: M–L (5–7 Tage)
- **SOLL:** Doc 28 §2 (Viren: 0 Achsen, Kapsel, Schwester von AnatomyBase an Wurzel IExecutionEngine; Graph-Dock), Doku 14 §33-§40.
- **IST (literal):** `execution_engine/execution_engine_base.hpp` (`IExecutionEngine` + `ExecutionEngineKind::Virus` existieren); KEINE produktive Virus-Impl (nur Test-Stub).
- **Verifikations-Kriterium:** `GraphBfs::engine_kind()==Virus` + BFS über Referenzgraph korrekt (Algorithmus-Korrektheit) + (falls DLL) Virus-Loader-Round-Trip.
- **Risiko/Entscheid:** `IPruefDock`/`AnatomyModuleHandle` ist Anatomy-typisiert → Viren brauchen EIGENE Dock-/Handle-Familie (Empfehlung), NICHT in den Anatomy-Zweig pressen. Gate-1-∏-Logik gilt für Viren NICHT (0 Achsen). **Status:** OFFEN (niedrigste Prio, V42-nah).

### L-CLUSTER — ZIH-Voll-Pfad: lokal-zählen → Cluster-bauen → Dock-messen  · Aufwand: L (gate-frei-Teil) + GATE-MAXIMAL
- **SOLL:** Doc 28 §5 (Skalierungs-Gleichung, User-verbindlich), messarchitektur_v5_design §1/§9.
- **IST (literal):** kein durchgängiger Treiber/CLI verkettet `StaticBinaryView→provision_all→Loader→measure→NodeValue` (grep: 0 Aufrufe von provision_all/measure_composition außerhalb Unit-Tests). `slurm_launcher.hpp` emittiert nur sbatch-TEXT + erwartet `perm_*.bin` (Executable) — Mismatch zu `.dll`-Modulen (kein `main()`). KEIN `comdare-ce.sif`-Rezept, KEIN Webhook-EMPFANG.
- **Gate-frei (lokal-verifizierbar):** E2E-Treiber `e2e_pipeline.hpp` + `apps/experiment_pipeline`; `apps/perm_runner` (lädt 1 perm-DLL, fährt sequencer, schreibt JSON); `result_ingest.hpp` (JSON→NodeObserverSnapshot→set_node_value); `.def`/`build_sif.sh` Skript-TEXT-Korrektheit.
- **GATE-MAXIMAL (mit User absprechen, kein Python):** `apptainer build comdare-ce.sif` + ZIH-Upload; echte `sbatch`-Submission (array aus BuildSelection.size()); Webhook-Receiver VLAN-60. **Status:** OFFEN (gate-frei-Teil zuerst; Submission termin/user-gebunden).

### L-MEAS — RuntimeVariableLoop: dyn. Variablen <1s je Binary, voller Mess-Lauf  · Aufwand: M
- **SOLL:** Doc 28 §5 (je Binary ALLE dyn. Variablen + Testdaten <1s, RAM-resident), messarchitektur_v5_design §4/§5.
- **IST (literal):** `runtime_variable_loop.hpp` vollständig für Resource-Control-Variablen (thread_count/prefetch/pool_budget/batch_size via `tier_query_resource_caps`+`tier_apply_resource_control`), ABER der Visitor MISST nichts — er muss je Setting die Zwei-Phasen-Op-Schleife + Workload fahren + Snapshot erzeugen.
- **Verifikations-Kriterium:** 1 geladener Pilot-Tier × dyn. Kartesik (z.B. thread {1,2,4} × prefetch {0,8}) = 6 Settings × 3 Repeats = 18 Snapshots, EINE DLL, kein Reload, <1s, `observer_real==true`.
- **Neue Datei:** `builder/experiment_tree/runtime_measure_visitor.hpp` (verbindet RuntimeVariableLoop + `drive_two_phase_op_loop` (I7 done) + `WorkloadGenerator` (done)). **Status:** OFFEN.

### L-BACKLOG — Doku-only Backlog (NICHT Teil der 6 Gates)  · Aufwand: variabel
- Doc 16 axis_05-IMC-Wrapper; Doc 15 axis_09b-Schichten + 15 AVX-512-Sub-Flags (VNNI/BF16/FP16); Original*SearchAlgo s4-echtes-Linking (heute s2-Stubs); R5.D PMC (Per-Achsen-Differenzierung jenseits Wall-Clock). **Status:** OFFEN (V42-nah, nicht Gate-blockierend).

---

## §2 Pre-Sprint-Klärungen — AUFGELÖST (genaues Doku-14-§28-Lesen, 2026-06-02)

**Auflösung (Planrunde, NICHT User gefragt):** Die explizite Achsen-Matrix Doku 14 **§28** (jede Achse je Gattung einzeln ✓-markiert) ist maßgeblich; die zusammenfassende §32.2 hat bei Set/Sequence ein systematisches **Off-by-one** (−1). Exakte Zählung der ✓:

| ID | Frage | AUFLÖSUNG (§28-Zählung) |
|----|-------|--------------------------|
| K-A | Set 14 vs 15 | **15 geteilte Achsen** ✓ (search_algo, cache_traversal, path_compression, node_type, memory_layout, allocator, prefetch, concurrency, serialization, telemetry, isa, index_organization, io_dispatch, migration_policy, filter); **kein** mapping/value_handle (K=V). §32.2-„14" = Off-by-one. |
| K-B | Sequence 9 vs 10 | **10 geteilte** ✓ (memory_layout, allocator, prefetch, concurrency, serialization, telemetry, value_handle, isa, io_dispatch, migration_policy) **+ axis_growth** (eigene). §32.2-„9" = Off-by-one. |
| K-C | View 7 | **7 gesamt** = 4 geteilte (memory_layout, telemetry, value_handle, isa) + 3 eigene (axis_extent, axis_layout, axis_accessor). §28 bestätigt; non-owning → kein allocator/concurrency/insert. |

## §2.5 Phase-D-Fortschritt + Verifikations-Build-Rezeptur (Kontinuität)

**Erledigt (literal grün, committet):** D1 L-SEL · D2 L-73 · D3 L-77 · D4 L-75 (Container q2+DLL) · **#76 KOMPLETT: D9 Set (15) · D10 Sequence (10+axis_growth) · D11 View (7 non-owning) → ALLE 5 Anatomie-Gattungen GenusBound==true + in-process (test_genus_binding 5/5) + DLL-Round-Trip (test_dgenus_dll: Sequence/View/Set über gattungs-agnostischen Loader; Container via D4b; SearchAlgorithm via BR-4) · D12 L-76d Viren (IVirusExecutionEngine + GraphBfs, echte BFS, test_d12_virus).** Pre-Sprint K-A/B/C aufgelöst (§28 maßgeblich). **Tasks erledigt: #73, #75, #76, #77** (+ Phase A/B/C). **#74 TEIL-erledigt (ZURÜCKGESTUFT nach Phase-E-Zwischenaudit 20260602-goal-v6-phase-e-zwischenaudit.md):** **L-74b** ✅ (BuildVariantDefinitionV1 + Reader, test_d7) · **L-74c** ✅ (R5.B-Operabilitäts-Klassifikation 2+4+11==17, anti-Fake, test_d8) · **L-74a TEIL** (nur standalone `COMDARE_DEFINE_BUILD_VARIANT_INSPECTION`-Symbol, **stub-getestet** test_d7a — **OFFEN: `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT` 17+3-DLL + hw_cross_constraint.hpp 96→~25 + build_variant_args + reale comdare_build_*-Symbole NICHT gebaut**). **L-MEAS** ✅-Mechanik (test_d13 — **Caveat: gegen MockTier, nicht geladene DLL**). **L-CLUSTER gate-frei NUR result_ingest** ✅ (test_d14 — **OFFEN: e2e_pipeline/apps/perm_runner/build_sif.sh/comdare-ce.def NICHT gebaut**, ~1/4 des gate-freien Teils).

**Solide grün:** #73, #75, #76 (5 Gattungen je 1 reales Kern-Organ + int-Platzhalter-Slots, ehrlich deklariert; Viren echte BFS), L-74b, L-74c. **Echt verbleibend:** (a) **L-74a-Voll** (BUILDVARIANT-DLL + Cross-Constraint) · **L-CLUSTER-E2E** (perm_runner/e2e_pipeline/build_sif) · **CTest-Registrierung** der D-Tests (aktuell nur git-ignorierte Scratch-Skripte → nicht repo-reproduzierbar). (b) **V42-nah/schwer (Composition-Driver):** #74-Voll-Verdrahtung der 4 OperativeCapable-Achsen real in tier_observe + voller Build-Achsen-DLL-Einbau; per-Gattung Pruef-Docks + PermutationEngines (Doku 14 §32.6, „TODO V42"). (c) **GATE-MAXIMAL** (NICHT autonom, User-Freigabe): reale ZIH-sbatch-Submission + apptainer comdare-ce.sif + Webhook-Receiver-Deploy. (d) **Phase E Abschluss-Audit** (erst nach a+b+c). **Folge-Integration (Doku 14 §32.6, V42):** per-Gattung Pruef-Docks (Set/Sequence/View/Graph) + PermutationEngines. **Dann Phase E** (finaler adversarialer Audit). **Vorlage = set_/sequence_/view_-Ketten + Container/genus-DLL-Pfade.**

**Verifikations-Build (header-only Standalone-Tests, RAM-Watchdog):** `build/scratch_compile_test.ps1 -Test <name> [-Boost] [-Extra @("libs\cache_engine\src","build\generated")]` (cl via vcvars64+vswhere, response-file wegen Pfad-Spaces, cl-Kill < 2.5 GB frei). Include-Roots-Default: `libs/cache_engine`, `libs/cache_engine/include`, `libs/common`. Achsen-Tests (q1/q2 etc.) brauchen `-Boost` + `-Extra src,generated`. DLL-Round-Trip (3-Phasen) = `build/scratch_d4b_dll.ps1` (DLL `/LD /DCOMDARE_ANATOMY_MODULE_BUILD`; Loader-Test linkt `anatomy_module_loader.cpp`; /Fo-Verzeichnis braucht `\\"`). Scratch-Skripte sind git-ignoriert (build/), bei Verlust aus dieser Rezeptur reproduzierbar.

## §3 Verdikt Phase B

Gegenüber Doc 28 §6 ist das Ledger nun **lückenlos granular** (L-73, L-SEL, L-77, L-75, L-74a/b/c, L-76a/b/c/d, L-CLUSTER, L-MEAS, L-BACKLOG), je mit literalem IST-Code-Stand (Datei:Zeile) + Verifikations-Kriterium. **Solide grün** bleiben Gate-1/2/5/6 + SearchAlgorithm-BR-4 (Pilot). **„ABSOLUTE Vollständigkeit"** = Schließung aller obigen Lücken (Phase D), bestätigt durch Phase E (finaler adversarialer Audit). Reihenfolge/Sprints: → Planungssession.
