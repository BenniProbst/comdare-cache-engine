# Goal-V6 Session-Abschluss + V42-Scope-Vorbereitung (2026-06-02)

> **Zweck:** Diese Session-Doku sichert den vollständigen Stand des frisch-von-vorn durchlaufenen Goal-V6-Zyklus
> (Phase A→E) und bereitet das **nächste Thema** sauber vor: der vom User freigegebene **V42-Scope** („V42-Scope
> jetzt autonom angehen", AskUserQuestion 2026-06-02) — die schwere #74-Composition-Driver-Voll-Verdrahtung +
> L-74a-BUILDVARIANT-DLL im vollen Achsen-Umbrella. Single-Source-Querverweise: `goal-v6-luecken-ledger.md` (§2.5
> Fortschritt) · `20260602-goal-v6-phase-e-zwischenaudit.md` (ehrliches Audit-Verdikt) · `28_vollstaendigkeits-kartographie.md` (SOLL).

## §1 Was diese Session erreicht hat (frischer Goal-V6-Durchlauf von vorn)

Phase A (Kartographie) · B (Lücken-Ledger) · C (Planungssession) abgeschlossen + committet. Phase D (Umsetzung)
zu großen Teilen, jeder Schritt **RAM-Watchdog-verifiziert (min_free ≥8.5 GB, kein OOM), committet, 3-Repo-
Submodul-synchronisiert**, plus ein adversarialer Phase-E-Zwischenaudit mit ehrlicher Selbst-Korrektur.

**Solide grün (literal verifiziert + audit-bestätigt + großteils CTest-registriert):**
- **#73** provision_all O(K) (`build_orchestrator.hpp` provision_core; `coverage_selection.hpp` BuildSelection) — test_d1_d2 (100 Builds aus 1e12-View).
- **#75** Container q2-Slot + DLL-Round-Trip (`container_anatomy/tier/abi_adapter` + `container_module_abi_v1.hpp`) — test_d4b.
- **#76** ALLE 5 Anatomie-Gattungen gebunden (SearchAlgorithm/Adapter/Set/Sequence/View, je Composition/Anatomie/AbiAdapter/GenusBindingTraits) + in-process + DLL-Round-Trip (test_genus_binding 5/5; test_dgenus_dll Sequence/View/Set; test_d9/d10/d11) + **Viren** (IVirusExecutionEngine + GraphBfs echte BFS, test_d12). Je Gattung EIN reales Kern-Organ + int-Platzhalter-Slots (EHRLICH deklariert).
- **#77** ceb_generator generate_all_real (test_d3).
- **#74 L-74b** BuildVariantDefinition (test_d7) · **L-74c** Operabilitäts-Klassifikation 2+4+11==17 (test_d8) · **L-74a-Teil** Build-Variant-Inspection-ABI (test_d7a, stub-getestet).
- **L-MEAS** RuntimeMeasureVisitor (test_d13).
- **L-CLUSTER gate-frei** result_ingest (test_d14) + perm_runner (test_d14b) + e2e_pipeline (test_d14c, O(K): 3 aus 1e9).
- **CTest-Registrierung** der 15 D-Tests (`tests/unit/CMakeLists.txt`) — cmake-configure grün + test_d8 im echten CMake/MSBuild-Kontext gebaut+gelaufen ALLE OK (behebt Audit-Reproduzierbarkeits-Vorbehalt).
- Pre-Sprint-Klärungen K-A/B/C aufgelöst (Set=15/Sequence=10+axis_growth/View=7; Doku 14 §28 maßgeblich).

**Commit-Range (cache-engine):** `3b466da` (Phase A) … `21c7e48` (L-CLUSTER-Ledger). DA-Submodul mitgezogen.

## §2 Ehrlich offen (Phase-E-Zwischenaudit-Verdikt — NICHT über-behaupten)

Der adversariale Audit stufte 3 Headlines zurück (Details: `20260602-goal-v6-phase-e-zwischenaudit.md`). Offen bleibt:
1. **#74-Composition-Driver-Voll-Verdrahtung** (V42-nah) — die 4 OperativeCapable-Achsen real in tier_observe.
2. **L-74a-Voll** (V42-nah) — COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT (17+3-DLL) + hw_cross_constraint + reale comdare_build_*-Symbole.
3. **per-Gattung Prüf-Docks + PermutationEngines** (Doku 14 §32.6 „TODO V42").
4. **GATE-MAXIMAL ZIH** — reale sbatch-Submission + apptainer comdare-ce.sif + Webhook-Deploy (User-Freigabe nötig, NICHT autonom) + perm_runner-CLI-App + comdare-ce.def/build_sif-Text.
5. **vorbestehender Tech-Debt:** gtest-discover-Artefakt `test_pressure_state[1]_include` (blockiert den ctest-Voll-Lauf der Alt-Tests; nicht die D-Tests).
6. **Phase-E-Abschluss-Audit** (erst nach 1–4).

## §3 NÄCHSTES THEMA — V42-Scope-Sprint (User-freigegeben, autonom)

### §3.1 #74-Composition-Driver-Voll-Verdrahtung (PRIORITÄT 1) — exakter Ansatzpunkt

**Ist-Stand (verifiziert gelesen):** `anatomy/search_algorithm_anatomy.hpp:62-72` `observe_all()` hält NUR
`axis_search_algo_` (Z.101) und füllt nur `agg.search_algo` (Z.69), wenn `ObservableAxis<search_algo>`. Die
allocator-Achse wird separat im `abi_adapter.hpp` (container_t = ObservableComposedSearch + ComposedStore) real
getrieben (R6 Inkrement 2b). Die 4 OperativeCapable-Achsen (node_type/memory_layout/serialization/telemetry,
s. `axis_operability_classification.hpp`) sind als Organe vorhanden (R7.2/7.3/7.4 done), aber NICHT in observe_all/
tier_observe verdrahtet.

**Umsetzungs-Plan (Composition-Driver):**
1. **SearchAlgorithmAnatomy weitere Organe halten:** in `search_algorithm_anatomy.hpp` neben `axis_search_algo_`
   auch `typename Composition::telemetry axis_telemetry_{}` (am leichtesten — statistics-tragend), dann
   memory_layout/serialization/node_type. observe_all() je Achse `if constexpr (ObservableAxis<...>) agg.<achse> = axis_<achse>_.statistics();`.
   ⚠️ Key-Type-Sauberkeit (Doku 24 §5.5): alle Organe denselben uint64-Key (Umstufung-B) — sonst latenter Mismatch.
2. **abi_adapter.hpp tier_observe erweitern** (`:268-298`): nach search+alloc die neuen Achsen-statistics() in den
   Cross-ABI-POD flachen. **POD-Erweiterung** `observable_tier.hpp` ComdareTierObserverSnapshotV1: NEUE uint64-
   Felder NUR APPEND-only (telemetry_*/layout_*/serialize_*) → Minor-ABI-Bump (alte DLLs bleiben kompatibel); KEIN
   Umordnen (Major-Bruch). `NodeObserverSnapshot` (experiment_tree.hpp) + result_ingest/perm_runner-Format parallel ergänzen.
3. **observable_axis_count ehrlich hochzählen** (heute 2 → real getriebene Zahl); `axis_operability_classification.hpp`
   Operative-Liste nachziehen (von 2 auf die neu-verdrahteten). **KEINE Marke ohne Delta-vor/nach-Treiben-Test.**
4. **R5.D-Grenze dokumentieren:** memory_layout-Wall-Clock ist sub-noise → als „operativ-PMC-pending" markieren,
   NICHT als voll-gemessen (Doc 28 §1 #6).
- **Verifikations-Kriterium:** ein getriebenes Lebewesen → tier_observe POD-Felder >0 für telemetry (+ ggf. layout/serial/node), Delta vor/nach Treiben; `observable_axis_count` == literal getriebene Zahl. Schwerer Compile (Umbrella + Boost + `${COMDARE_ALL_AXIS_GENERATED_DIRS}`); Test analog `test_v41_tier_observe_trace` registrieren.
- **Risiken:** (R1) Umbrella-Compile RAM-intensiv → RAM-Watchdog Pflicht, ggf. einzelne Achse pro Commit. (R2) observe_all hält heute 1 Organ; mehr Organe → Key-Type-Mismatch-Reaktivierung (uint64 erzwingen). (R3) ABI nur append-only (Minor), sonst alle Permutations-DLLs neu.

### §3.2 L-74a-Voll: BUILDVARIANT-DLL + Cross-Constraint (PRIORITÄT 2)
- **COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(<17 FQ>, PT, SE, HW)** in `anatomy_module_abi_v1.hpp`:
  wie COMDARE_DEFINE_ANATOMY_MODULE_ADHOC, aber zusätzlich `using ComdarePageTypeVariant=PT; …` + die 3
  `comdare_build_page_kind/simd_width_bits/hw_cache_line`-extern-C-Symbole (aus build_variant_inspection.hpp-Muster).
- **`hw_cross_constraint.hpp`** (neu, builder/experiment_tree): mp_remove_if-Predicate über (09=ISA × 09b=SE × 12=HW),
  `is_compatible<SE,HW,ISA>` → 96 → ~25-30 (Doc 15 §6); via mp_size literal belegen.
- **`build_variant_args<PT,SE,HW>()`** in `adhoc_emitter.hpp` (3 zusätzliche FQ-Typnamen via type_name).
- **Verifikation:** 1 Blatt → BUILDVARIANT-DLL gebaut (scratch_d4b-Muster) → `comdare_build_simd_width_bits()==256 (Avx2)` vs `==512 (Avx512)` DERSELBEN 17-Komposition; Cross-Constraint 96→~25 via mp_size. **L-74b BuildVariantDefinitionV1 + L-74a Inspection-Symbol sind bereits da** (nur in-DLL-Einbau + echte axis_09b/12-Wrapper statt Stubs fehlen).

### §3.3 per-Gattung Prüf-Docks + PermutationEngines (PRIORITÄT 3)
- SetDock/SequenceDock/ViewDock (analog `search_algorithm_dock.hpp`/`container_dock.hpp`, je I*Tier-dynamic_cast) + GraphDock (eigene Familie, Doku 14 §35.4).
- SetPermutationEngine/SequencePermutationEngine/ViewPermutationEngine (Doku 14 §32.6, je nur-Gattungs-Slots) — Voraussetzung für Baum-Bindung der Rest-Gattungen über BR-2/BR-3.

## §4 GATE-MAXIMAL (NICHT autonom — User-Freigabe)
Reale ZIH-sbatch-Submission + apptainer comdare-ce.sif-Build + Webhook-Receiver-Deploy (VLAN-60). Memory-Direktive:
kritische Cluster-Manöver IMMER mit User absprechen (Strafen/Account-Sperre). Gate-frei vorbereitbar (Text, ohne
Ausführung): perm_runner-CLI-App (apps/perm_runner/main.cpp, dünner Wrapper um AnatomyModuleLoader + perm_runner.hpp)
+ comdare-ce.def (Apptainer-Rezept) + build_sif.sh + sbatch-Array-Gen aus BuildSelection.size().

## §5 Verifikations-Build-Rezeptur (Kontinuität)
- **Standalone-Tests (RAM-Watchdog):** `build/scratch_compile_test.ps1 -Test <name> [-Boost] [-Extra @("libs\cache_engine\src","build\generated")]` (cl via vcvars64+vswhere, response-file wegen Pfad-Spaces, cl-Kill <2.5 GB). Default-Roots: libs/cache_engine(+include/common). Achsen-/Umbrella-Tests: `-Boost -Extra src,generated` + ggf. `${COMDARE_ALL_AXIS_GENERATED_DIRS}`.
- **DLL-Round-Trip (3-Phasen):** `build/scratch_d4b_dll.ps1` (Container) · `build/scratch_dgenus_dll.ps1` (Set/Sequence/View). /Fo-Verzeichnis braucht `\\"`; DLL `/LD /DCOMDARE_ANATOMY_MODULE_BUILD`; Loader-Test linkt `anatomy_module_loader.cpp`. Scratch-Skripte git-ignoriert (build/), aus dieser Rezeptur reproduzierbar.
- **CTest (repo-reproduzierbar):** `tests/unit/CMakeLists.txt` Ende — 15 D-Tests via add_executable+add_test (kein gtest). Build-Dir `build/msvc-release` konfiguriert. Gezielt: `cmake --build build/msvc-release --target <test>` → `.../tests/unit/Debug/<test>.exe`. (Voller ctest blockiert durch vorbestehendes gtest-discover-Artefakt — separater Tech-Debt.)

## §6 Disziplinen (für den V42-Sprint unverändert gültig)
OOM-Lazy (nie ganzer Baum, RAM-Watchdog jeder Compile) · bauen parallel/messen seriell · 17/22 schrumpfen nie ·
KEINE Erfolgsmarke ohne literale Tool-Ausgabe · KEINE declare-victory-by-reclassification · commit+push+Submodul-
Sync je Schritt · Doku nie löschen · kein Python in Buildchain · Compile-Time-Only/kein Runtime-Switch im Hot-Path ·
destruktive Ops nur mit Tag+Commit+Push · GATE-MAXIMAL ZIH immer mit User absprechen.

**Empfohlene V42-Sprint-Reihenfolge:** §3.1 (Composition-Driver, eine Achse pro Commit, telemetry zuerst) → §3.2
(BUILDVARIANT-DLL) → §3.3 (Docks/PermutationEngines) → dann GATE-MAXIMAL-Vorbereitung (Text) → Phase-E-Abschluss-Audit.
