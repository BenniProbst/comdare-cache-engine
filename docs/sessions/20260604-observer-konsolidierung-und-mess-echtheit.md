# Session 2026-06-04 — Mess-Echtheit (Re-Grounding) + Observer-Interface-Konsolidierung (I1)

> **Resume-Dokument.** Rolle: **Implementierungs-Agent** cache-engine/Thesis. Plan-Datei (Host):
> `C:\Users\benja\.claude\plans\dynamic-frolicking-truffle.md`. Preflight-Workflow-Output (file:line-Edit-Liste):
> `…/tasks/wkqt7a0il.output`. Direktiven: [[always-use-trees-for-search]], [[feedback_one_consistent_observer_interface_pruefdock]],
> [[feedback_never_guess_always_lookup_state_of_art_and_docs]], [[feedback_no_success_marks_without_literal_output]].

## 0. Auslöser-Kette (wie wir hierher kamen)
1. Synthetischer 8-MiB-prefetch-Puffer fabriziert → **User-Stopp** („Was für einen setup puffer? Wir haben ein riesiges Projekt").
2. Elaborate Planungssession + „lies die letzten 10 Architektur-Dokumente und update deinen Plan entsprechend deiner Verstöße".
3. Befund (Doc 24 §8.1 / Doc 30 Befund 2 / abhaengigkeitskette / no-dynamic-cast): mein erster Plan hatte **mehrere Verstöße** (synthetischer Scratch-Puffer durch die Hintertür; Pfad-A/Pfad-B-Verwechslung; Befund-2-Replikation; layout-Achse unecht).
4. User-Entscheide: Per-Achsen-Timing in **Pfad B** (reale Komposition), **layout-honorierender Store**, **N=131072**, dann **EINE konsistente Observer-Schnittstelle** (I1-Konsolidierung).

## 1. FERTIG + committet + gepusht (je Schritt grün verifiziert)
Alle Commits in `comdare-cache-engine` (main); Superprojekt-Submodul je gebumpt+gepusht.

| Schritt | Substanz | Beleg (literal) | ce-Commit |
|---|---|---|---|
| **S1** | `LayoutAwareChunkedStore<N,L,A>` (`axes/node/axis_04_node_type_layout_aware_store.hpp`) — Byte-Backing am Layout-`eff_stride` (CLA 64 / aos 16); memory_layout-Achse ECHT, `organ_observe_layout`-OOB behoben, alloc-Bytes layout-abhängig | `test_layout_aware_store` exit 0: CLA bytes=4× aos; eff_stride CLA=64/aos=16 | `f767325` |
| **S2** | Pfad-B Per-Achsen-Timing `fill_segment_timing_v3` (`abi_adapter.hpp`) über die REALE befüllte Komposition; `container_t`→LayoutAwareChunkedStore; `IObservableTierV4` (transitorisch) | `test_pathb_segment_timer` exit 0: 19 seg_ns>0, DATA-Neutralität, Korrelation, Lebewesen-Variation | `bee798d` |
| **S2.5 I-A** | EIN POD `ComdareTierObserverSnapshot` {axis_stats[19][8]; seg_ns[19]; obs_axes; fill; filled; batches} + EINE `IObservableTier::tier_observe(unified)`; Adapter-Impl mit **fixer Q1-Sequenz** (Observer-READ → seg_ns-Timing → per-op-Reset). Additiv (Major bleibt 2). | `test_pathb`: unified axis_stats == V3; `test_obs_phaseA` exit 0 | `d1cdb29` |
| **I-B.1** | `tier_clear()` ruft `search_organ_.reset()` (ObservableComposedContainer/Search HABEN `reset()` Z.90/80 — „nicht resetbar"-Kommentar war VERALTET) → T0 frisch je Messung, KEIN post−pre-Delta mehr. perm_runner `PermResult.unified` (additiv) | `test_obs_phaseA` exit 0 | `9f48e8d` |
| **I-B.2/2b** | Iterator-CSV `stat_*`+`seg_*`+`filled` aus `row.unified` = **Pfad B**; `format_csv_row` rückwärtskompatibel (Fallback v3/seg, entfällt I-C); test_obs_phaseA verifiziert unified==V3 | `test_obs_phaseA` exit 0 ("unified axis_stats == V3" ×3) | `a76e673`/`928ae8c` |

**Projektweit (cmake):** `test_d14b_perm_runner` **exit 0** (Round-Trip mit additiver Konsolidierung).

**Erreicht:** Thesis-Mess-Pfad (perm_runner→iterator→CSV) läuft über die EINE `tier_observe` + den EINEN POD, mit **Pfad-B-Timing über reale Strukturen** (keine synthetischen Puffer). Q1-Doppelzählung adversarial ausgeschlossen+verifiziert.

## 2. PREFLIGHT-WORKFLOW (wkqt7a0il, 116 Konsumenten, 8 Agenten) — verifizierte Grundlage
- **Feld-Subsumption NULL POD-Orphans:** axis_stats[19][8]+Meta trägt JEDES V1- (13) + V2-Feld (26). V1 search→`[0][0..5]`, alloc→`[6][0..4]`, obs_axes/fill→Meta. V2 telemetry→`[10]`, layout→`[5]`, serialization→`[9]`, node_type→`[4]`. **V2-Pfad host-seitig TOT + redundant zu V3 → löschbar.**
- **ABI-Layout OK:** unified POD `sizeof==1400`, alignof 8, kein Padding, standard_layout+trivially_copyable. Loader-Reject via **Makro**-Magic + Major-Vergleich (KEIN hartkodiertes Hex) → Major 2→3 + Magic `.A2.→.A3.` lehnt Alt-DLLs sauber ab. Mehrfach-Vererbungs-Abbau sicher (alle Casts `dynamic_cast`, offset-agnostisch).
- **2 Mess-Semantik-Blocker:** (Q1) fixe Sequenz Observer-READ→Timing→Reset (in I-A umgesetzt+verifiziert). (Q3) seg_* kam aus Pfad A; `fill_segment_timing_v3`/V4 war host-seitig toter Code → Konsolidierung schaltet seg_ns real auf Pfad B (User-Fork: **Pfad B**).
- **3 Host-V1-Orphans (umhängen, kein Datenverlust):** NodeObserverSnapshot (experiment_tree), `format_perm_result`↔`ingest_result_line` 13-Feld-Wire-Zeile, `tier_observe_trace_abi`-CSV/JSON-Emitter.

**User-Forks entschieden:** seg_ns = **Pfad B** (reale Struktur; Thesis-seg_* werden neu baseliniert). Baum/Wire-Format **VOLL** auf axis_stats[19][8] (bricht 13-Feld-Cluster-Protokoll bewusst). **Klein entschieden:** V2-Tests auf axis_stats-Indizes umschreiben (Coverage halten); Schema-Konstanten kV3*-Namen behalten.

## 3. VERBLEIBEND (all-or-nothing, je cmake-verifiziert) — file:line in tasks/wkqt7a0il.output

> **I-B.3 DONE** (ce `3ceddf4`): Tree/Wire-Format voll auf axis_stats[19][8] (minimal-Ripple: NodeObserverSnapshot
> bekam die volle Matrix als FLACHE Felder + behielt die 13 Legacy-Felder als Projektion → nur experiment_tree +
> perm_runner (format_perm_result volle Matrix, run_observable_perm OHNE V1-Delta) + result_ingest (parse 175 + 13
> ableiten) + test_d14b/d14c-Mocks geändert). Verifiziert cmake: test_d14b exit 0, test_d14c exit 0, test_obs_phaseA exit 0.
> **Letzter V1-Blocker (format_perm_result) entfernt → I-C unblocked.**

### I-B.3 — Tree/Wire-Format VOLL auf axis_stats[19][8] (~8 Host-Dateien SYNCHRON) [DONE]
- `experiment_tree.hpp`: `NodeObserverSnapshot` (heute 13 V1-Felder Z.42-49) → `using NodeObserverSnapshot = anatomy::ComdareTierObserverSnapshot;` (+ `#include observable_tier.hpp` — leichter ABI-Header, ok).
- `perm_runner.hpp` `format_perm_result` (heute 13 ';'-Felder aus V1) + `result_ingest.hpp` `ingest_result_line` (heute 13 Felder parsen, `f.size()<14`-Check) → **volle Matrix** (axis_stats 152 + seg_ns 19 + Meta 4 = 175 Felder). **SYNCHRON ändern, sonst Round-Trip-Bruch** (test_d14b/d14c).
- Konsumenten der 13 Felder auf `axis_stats[0]/[6]`+Meta: `cache_engine_builder_iterator.hpp` (13 `o.*`-CSV-Spalten Z.181-193), `runtime_measure_visitor.hpp`, `node_value_measurement.hpp`, `workload_orchestrator.hpp`, `tier_observe_trace_abi.hpp` (Emitter Z.267-312).
- Tests: `test_d14b_perm_runner` (assertions search_insert==100 → axis_stats[0][3]), `test_d14c_e2e_pipeline`, `test_d13_runtime_measure`, `test_d13_dll_runtime_measure`.
- **Verifikation:** cmake `test_d14b_perm_runner` + `test_d14c_e2e_pipeline` grün.

### I-C — Entfernung + ABI-Major-Bump
> **I-C.1 DONE** (ce `c0012c7`): 5 Mock-Tests (test_d13_runtime_measure, test_d13_dll_runtime_measure, test_v5_two_phase_driver, test_v5_workload_orchestrator, test_v5_ycsb_op_set) additiv um `tier_observe(ComdareTierObserverSnapshot*)` ergänzt (search→axis_stats[0]) → nach V1-Entfernung nicht abstrakt. Verifiziert cmake exit 0. **User-Entscheid 2026-06-05: VOLLSTÄNDIG LÖSCHEN** (echte EINE-Schnittstelle, NICHT deprecated behalten).
> **I-C.2 = atomarer Block (zusammen kompilieren, dann I-D Voll-Rebuild):** observable_tier.hpp (V1/V2/V3-PODs + asserts + kTierObserverSnapshotVersion(V1/V2/V3) löschen; **kV3AxisCount/kV3FieldCount/kV3AxisSchema/kV3FilledAxisCount BEHALTEN** = Schema; IObservableTierV2/V3/V4 löschen; `tier_observe(unified)`=0 als EINZIGE pure-virtual, `tier_observe(V1)` weg). abi_adapter.hpp (Vererbung nur IObservableTier; fill_observer_v3→Body schreibt direkt out->axis_stats; fill_observer_v2/tier_observe_v1/v2/v3/timed_v3 weg). tier_observer_v2_bridge.hpp DELETE. anatomy_module_abi_v1_decl.hpp Major 2→3/Minor→0/Magic .A2.→.A3. perm_runner (r.v3/v3_real + v3-Param weg). iterator (.v3/.seg/obs3/segw/Fallback weg). **~10 Tests mit V1-POD-als-Typ** (test_v41_anatomy_observer/_adhoc_dll_load/_f15_measurement/_module_abi/_loader/_codegen, br4_load, test_obs_phaseA/B*, test_pathb, tier150_axis_grid) + runtime_measure_visitor/node_value_measurement/workload_orchestrator/tier_observe_trace_abi (V1-POD→unified). DANN voller cmake-Build + iterieren bis grün. **Empfehlung: in EINEM Block mit frischem Budget (mehrere Voll-Build-Zyklen).**

> **KRITISCHE Intricacy:** Die EINE `tier_observe(unified)` RUFT heute `fill_observer_v3(&v3)` (V3-POD) + `fill_segment_timing_v3(&seg)` (SegV2) und KOPIERT in den unified POD. Vor dem Löschen von V3-POD: `fill_observer_v3` auf `ComdareTierObserverSnapshot*` umstellen (schreibt `out->axis_stats`/Meta DIREKT — identisches Layout) + `fill_segment_timing_v3` schreibt `out->seg_ns` direkt. `ComdareSegmentLatencyV2` BLEIBT (Pfad A `run_workload_segmented_v2` nutzt sie). DANN V3-POD + V2/V3/V4-Interfaces löschen.
> **~12 verbleibende Test-Mocks** mit `tier_observe(ComdareTierObserverSnapshotV1*)`-Override → auf `tier_observe(ComdareTierObserverSnapshot*)` + axis_stats[0]/[6] umstellen (Muster wie test_d14b: search→[0], alloc→[6]). Liste: test_d13_runtime_measure, test_d13_dll_runtime_measure, test_v5_two_phase_driver, test_v5_workload_orchestrator, test_v5_ycsb_op_set, br4_load, test_v41_anatomy_f15_measurement(+memcpy), test_v41_anatomy_observer, test_v41_anatomy_adhoc_dll_load. V3/V4-Tests (test_obs_phaseA/B*, test_pathb, tier150_axis_grid) auf EINEN POD. Fallback in format_csv_row entfernen.
- `observable_tier.hpp`: ComdareTierObserverSnapshotV1/V2/V3 + IObservableTierV2/V3/V4 + kTierObserverSnapshotVersion(V2/V3) löschen; `tier_observe(unified)` als EINZIGE pure-virtual (V1-Overload weg).
- `abi_adapter.hpp`: IObservableTierV2/V3/V4 aus Vererbung (Z.120-124); fill_observer_v2/tier_observe_v2 (Z.783-854), fill_observer_v3/tier_observe_v3 (Body in unified gemergt), fill_segment_timing_v3/tier_observe_timed_v3 löschen. (IMeasurableWorkload*-Vererbung = Pfad A, BLEIBT.)
- `tier_observer_v2_bridge.hpp` LÖSCHEN (0 Includer verifiziert).
- `anatomy_module_abi_v1_decl.hpp`: `COMDARE_ANATOMY_ABI_MAJOR 2→3`, MINOR 2→0, Magic `0x434F4D444141322E→0x434F4D444141332E` (.A3.).
- Iterator: Fallback in format_csv_row entfernen (nur unified); LazyMeasuredRow .v3/.seg + segw/drive_segment_latencies-Pfad-A entfernen (oder Pfad A separat lassen — run_workload bleibt, run_workload_segmented_v2 als isolierter Vergleich optional).
- Tests: `test_v41_anatomy_module_abi` (3/0/.A3.), `test_v41_anatomy_module_loader` (Major-2-Reject-Fall), `test_v41_anatomy_codegen`; V2-Tests (`test_d_v42_abi_telemetry_coupling`, V2-Teil von `test_v41_anatomy_observer`/`_adhoc_dll_load`) auf axis_stats[10]/[5]/[9]/[4]; V3/V4-Tests (`test_obs_phaseA/B*`, `test_pathb`, `tier150_axis_grid`) auf EINEN POD.
- **Verifikation:** Grep zeigt 0 Vorkommen ComdareTierObserverSnapshotV1|V2|V3 / IObservableTierV2|V3|V4 / fill_observer_v2|v3 / fill_segment_timing_v3 / tier_observer_v2_bridge in libs/+tests/+apps/.

### I-D — DLLs neu + realer Lauf
- Alle perm-/genus-DLLs neu (Major-Bump erzwingt; COMDARE_DEFINE_*_MODULE erbt automatisch). Re-Configure CMake.
- Realer Lauf (run_lazy / test_lazy_static_dynamic_driver N=8-12): loaded>0, measured>0, CSV aus EINEM POD; Alt-DLL (Major 2) → status_abi_major_mismatch literal verifizieren.

## 4. WICHTIGE INVARIANTEN (must_not_break)
- **modules/*** = TOTE Spiegel — NIE anfassen; realer Build inkludiert NUR `libs/cache_engine`.
- **Q1-Sequenz** in der EINEN tier_observe zwingend: axis_stats-READ → seg_ns-Timing → per-op-Reset (sonst Doppelzählung T0/T1/T2/T3/T7/T8/T10/T17/T18).
- **Pfad A** (`IMeasurableWorkload*` / `run_workload`, measurable_workload.hpp) BLEIBT (isolierter Achsen-Bench) — getrennt von Pfad B; seg_ns im POD = Pfad B.
- **kV3AxisSchema** = Single-Source Schreiber↔CSV-Spaltenname; Honest-0 (Baseline-Strategien echte 0-Teilfelder, KEIN „Fehler").
- ABI-POD `is_standard_layout && is_trivially_copyable` (memcpy über DLL-Grenze).

## 5. BUILD/VERIFIKATION-KOMMANDOS
- Standalone cl-Tests: `build/scratch_compile_obs_phaseA.ps1`, `…_pathb.ps1`, `…_all19.ps1`, `…_layout_store.ps1` (generated/ aus CMake-Configure nötig).
- cmake-Tests: `cmake --build build/msvc-release --target <test> --config Release` → `build/msvc-release/tests/unit/Release/<test>.exe`.
- Je Increment grün → commit (ce) + Superprojekt-Submodul-Bump (`Code/external/comdare-cache-engine`) + push.
