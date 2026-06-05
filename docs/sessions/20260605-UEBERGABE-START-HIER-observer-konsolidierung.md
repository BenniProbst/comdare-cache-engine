# ÜBERGABE / START HIER — Observer-Konsolidierung (I1) Resume @ I-C.2

> **Kontext lief knapp → Kompaktierung.** Dies ist das **autoritative Resume-Dokument**. Lies zuerst §1 (Pflicht-Pre-Read),
> dann §2 (Rolle), §3 (Stand), §4 (EXAKTER Resume-Punkt I-C.2/I-D). HEAD bei Übergabe: cache-engine `9ab328f`,
> Superprojekt (Diplomarbeit) `ee6ad54`. Alles committet+gepusht, durchgehend grün.

---

## §1 PFLICHT-PRE-READ (in dieser Reihenfolge, VOR jeder Code-Arbeit)

**A. Diese Übergabe-Kette (das WO-genau):**
1. **DIESES Dokument** (`docs/sessions/20260605-UEBERGABE-START-HIER-observer-konsolidierung.md`).
2. `docs/sessions/20260604-observer-konsolidierung-und-mess-echtheit.md` — die ausführliche Substanz-Doku (Feld-Map, I-C.2-Intricacy, must_not_break).
3. Plan-Datei (Host, NICHT im Repo): `C:\Users\benja\.claude\plans\dynamic-frolicking-truffle.md` — Schritt 2.5 (Konsolidierung).
4. Preflight-Workflow-Output (file:line-Edit-Liste, evtl. ephemer): `…\AppData\Local\Temp\claude\C--WINDOWS-system32\<sess>\tasks\wkqt7a0il.output` — falls weg: §4 hier ist die Essenz.

**B. Architektur-Dokumente (autoritativ für die Mess-/ABI-Architektur) — `docs/architecture/`:**
- **`24_messmodell_korrektur_zwei_dimensionen.md`** §8.1 — das **HYBRID-Modell**: **Pfad A** = isolierte Achsen-Algorithmen (run_workload, DLL-selbst); **Pfad B** = composite Tier über die REALE Komposition (observe_all/tier_observe). **seg_ns im EINEN POD = Pfad B** (User-Entscheid).
- **`messarchitektur_design_observer_handle_no_dynamic_cast.md`** — „kein **per-Op**-dynamic_cast" (1× kalt/Modul = ok); I1-Konsolidierungs-Richtung.
- **`abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz.md`** §5 — der EINE Mess-POD + ABI-Major-Bump (I1); Loader-Major-Reject; **alle 4 extern-C-Symbole + dynamic_cast-Stellen**.
- **`30_audit_achsen_delegation_pflichtachsen.md`** — Befund 2 (Such-Organe umgehen Speicher-Achsen) + §8.0/§8.1 (3-Ebenen-Modell: Gattung=Interface/Prüf-Dock; Achsen Pflicht/uniform).
- **`28_vollstaendigkeits-kartographie.md`** §1 — die 22 Achsen + kV3AxisSchema-Verortung.
- (Bei Bedarf: `26_permutations_bplus_baum…` B+-Baum; `27`/`29` Experiment-Baum-Bindung.)

**C. Session-Dokumente (Mess-Echtheit-Kontext):**
- `docs/sessions/20260604-mess-architektur-19-achsen-timer.md` — der „PERSISTIERTE FOLGEPUNKT" (8 synthetische Achsen → echte Op), der diese ganze Arbeit auslöste.

**D. Memory-Direktiven (Host `…\.claude\projects\C--WINDOWS-system32\memory\`):**
- `feedback_one_consistent_observer_interface_pruefdock.md` — **GENAU EINE Observer-Schnittstelle** (der I1-Auftrag).
- `feedback_always_use_trees_for_search.md` — reale Strukturen, keine synthetischen Scans.
- `feedback_never_guess_always_lookup_state_of_art_and_docs.md` — Raten verboten, teure Planung.
- `feedback_no_success_marks_without_literal_output.md` — keine Erfolgsmarken ohne literale Tool-Ausgabe.

---

## §2 ROLLE + ARBEITSWEISE
Ich bin der **Implementierungs-Agent cache-engine/Thesis** (NICHT Cluster — Infra-Wünsche nur via K78). Ultracode ist AN.
Arbeitsweise: Planung-vor-Bau; je Increment grün baubar → committen (ce) + Superprojekt-Submodul-Bump (`Code/external/comdare-cache-engine`) + push. Destruktive Autonomie in den 3 Diplomarbeit-Repos mit Commit+Push erlaubt.

---

## §3 STAND (committet + gepusht + grün)

**Auslöser:** synthetischer prefetch-Puffer → User-Stopp („Was für einen setup puffer?") → Lektüre der 10 Architektur-Dokus → Plan korrigiert. **3 User-Entscheide:** (1) Per-Achsen-Timing in **Pfad B**; (2) **layout-honorierender Store**; (3) **EINE konsistente Observer-Schnittstelle** (I1), Alt-Typen **VOLLSTÄNDIG LÖSCHEN** (nicht deprecated).

| Schritt | Substanz | ce-Commit |
|---|---|---|
| S1 | `LayoutAwareChunkedStore<N,L,A>` (memory_layout-Achse echt, OOB behoben) | `f767325` |
| S2 | Pfad-B-Per-Achsen-Timing `fill_segment_timing_v3` über reale Komposition | `bee798d` |
| I-A | EIN POD `ComdareTierObserverSnapshot` + EINE `tier_observe` (Q1-Sequenz) | `d1cdb29` |
| I-B.1 | `tier_clear`→`search_organ_.reset()` (kein Delta) + perm_runner unified | `9f48e8d` |
| I-B.2/2b | Iterator-CSV aus EINEM POD = Pfad B; rückwärtskompatibler Fallback | `a76e673`/`928ae8c` |
| I-B.3 | Tree/Wire-Format VOLL auf axis_stats[19][8] (13→175 Felder) | `3ceddf4` |
| I-C.1 | 5 Mock-Tests additiv um `tier_observe(unified)` ergänzt | `c0012c7` |

**Grün verifiziert (literal):** standalone `test_layout_aware_store`/`test_pathb_segment_timer`/`test_obs_phaseA`/`test_all19_segment_timer` exit 0; cmake `test_d14b_perm_runner`/`test_d14c_e2e_pipeline`/`test_d13_runtime_measure`/`test_v5_two_phase_driver` exit 0.

**Funktional erreicht:** Der GANZE Host-Mess-Pfad (perm_runner→iterator→CSV **+** Cluster-Wire-Format→Baum) läuft über die EINE `tier_observe` + den EINEN `ComdareTierObserverSnapshot` (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta). Alt-Typen V1/V2/V3-PODs + IObservableTierV2/V3/V4 existieren NUR NOCH additiv (vom Live-Pfad ungenutzt). Mocks bereit.

**Preflight-Workflow `wkqt7a0il` (116 Konsumenten):** NULL POD-Feld-Orphans (axis_stats+Meta subsumiert V1/V2 voll); ABI-Layout OK (unified POD sizeof==1400, standard_layout+trivially_copyable); Loader-Reject via Makro-Magic+Major (kein Hardcode-Hex).

---

## §4 EXAKTER RESUME-PUNKT: I-C.2 (atomare Typ-Löschung) → I-D (DLL-Rebuild)

> **WICHTIG:** I-C.2 ist ein **atomarer ~15–20-Datei-Block** — er muss als Ganzes kompilieren. **Strategie:** Edits machen → **voller cmake-Build** (`cmake --build build\msvc-release --config Release`) → Fehlerliste abarbeiten bis grün. Der committete Stand `9ab328f`/`c0012c7` ist die grüne Rückfall-Basis (I-C.1).

### I-C.2 Edits (geordnet)
**1. `libs/cache_engine/anatomy/abi_adapter.hpp`** (ZUERST — der Adapter ist die Kern-Implementierung):
- `fill_observer_v3`: Signatur `ComdareTierObserverSnapshotV3*` → `ComdareTierObserverSnapshot*` (schreibt `out->axis_stats[t][f]` + `out->observable_axis_count/tier_fill_level/filled_axis_count` DIREKT — **identisches Layout**, nur Typname ändern).
- Die EINE `tier_observe(ComdareTierObserverSnapshot*)`: statt temp-V3+Kopie jetzt `fill_observer_v3(out)` direkt + `fill_segment_timing_v3(&seg)` (SegV2 bleibt — Pfad A nutzt sie) + `out->seg_ns[t]=seg.seg_ns[t]` + `out->batches_measured`.
- Vererbungsliste (`#if COMDARE_MEASUREMENT_ON`): **IObservableTierV2/V3/V4 entfernen** (nur `IObservableTier` bleibt; IMeasurableWorkload*/IRollbackableTier/IScannableTier/IResourceControllableTier BLEIBEN).
- Overrides löschen: `tier_observe(V1*)`, `tier_observe_v2`, `tier_observe_v3`, `tier_observe_timed_v3`, `fill_observer_v2`. (`fill_segment_timing_v3` BLEIBT als privater Helfer.)

**2. `libs/cache_engine/anatomy/observable_tier.hpp`:**
- `ComdareTierObserverSnapshotV1` + `V2` + `V3` Structs + ihre static_asserts + `kTierObserverSnapshotVersion`(V1/V2/V3) **löschen**.
- **BEHALTEN:** `kV3AxisCount`/`kV3FieldCount`/`kV3AxisSchema`/`kV3FilledAxisCount` (= Schema, von CSV+POD genutzt) + `ComdareTierObserverSnapshot` + `kTierObserverSnapshotVersionUnified`.
- `IObservableTierV2`/`V3`/`V4` Klassen **löschen**.
- `IObservableTier`: `tier_observe(ComdareTierObserverSnapshot*)` → **pure virtual `= 0`** (Default-Body weg); `tier_observe(ComdareTierObserverSnapshotV1*)` **löschen**.

**3. `libs/cache_engine/anatomy/tier_observer_v2_bridge.hpp`** → **DATEI LÖSCHEN** (0 Includer verifiziert; ggf. aus CMake-GLOB/Umbrella entfernen).

**4. `libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp`:**
- `COMDARE_ANATOMY_ABI_MAJOR` 2→**3**; `COMDARE_ANATOMY_ABI_MINOR` 2→**0**; `COMDARE_ANATOMY_ABI_MAGIC` `0x434F4D444141322E` → **`0x434F4D444141332E`** (.A2.→.A3.). Versions-Doc-Kommentar nachziehen.

**5. `libs/cache_engine/builder/experiment_tree/perm_runner.hpp`:** `PermResult.v3`/`v3_real` + den `IObservableTierV3* v3`-Param von `run_observable_perm` + den `tier_observe_v3`-Aufruf entfernen (nur noch `tier.tier_observe(&r.unified)`).

**6. `libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp`:** `LazyMeasuredRow.v3/v3_real/seg/seg_real` entfernen; `obs3`(IObservableTierV3)+`segw`(IMeasurableWorkloadV3)-dynamic_casts + `drive_segment_latencies`-Aufruf (Pfad A) entfernen; in `format_csv_row` den **Fallback** entfernen (nur `row.unified`).

**7. `libs/cache_engine/builder/.../{runtime_measure_visitor,node_value_measurement,workload_orchestrator}.hpp` + `anatomy_commands/tier_observe_trace_abi.hpp`:** alle `ComdareTierObserverSnapshotV1`-als-Typ + `tier_observe(V1)`-Aufrufe → `ComdareTierObserverSnapshot` + `tier_observe(unified)`; die Emitter (tier_observe_trace_abi CSV/JSON) lesen search_*/alloc_* aus `axis_stats[0]`/`[6]`+Meta.

**8. Tests (V1-POD-als-Typ / V3 / V4):** Mocks: in den 5 I-C.1-Mocks die `tier_observe(V1*)`-Overrides **löschen** (nur unified bleibt); `test_d14b` (V1-Mock-Override weg). V1-Typ-Nutzer: `test_v41_anatomy_observer`, `test_v41_anatomy_adhoc_dll_load`, `test_v41_anatomy_f15_measurement` (+memcpy), `br4_load` → unified. V3/V4-Tests: `test_obs_phaseA` (V3-Cast+POD→unified), `test_obs_phaseB_pilot`, `test_obs_phaseB_t11_t12`, `test_pathb_segment_timer` (V4→unified), `tier150_axis_grid`. ABI-Tests: `test_v41_anatomy_module_abi` (static_assert 3/0/.A3.), `test_v41_anatomy_module_loader` (Major-2-Reject-Fall), `test_v41_anatomy_codegen` ((3<<32)|0 + Magic).
- **Verifikations-Grep (muss 0 ergeben in libs/+tests/+apps/):** `ComdareTierObserverSnapshotV1|V2|V3`, `IObservableTierV2|V3|V4`, `fill_observer_v2`, `tier_observer_v2_bridge`.

### I-D — DLLs neu + realer Lauf
- Re-Configure CMake (Makro erbt Major-Bump) → alle perm-/genus-DLLs neu bauen. `COMDARE_DEFINE_*_MODULE` ziehen Major/Magic automatisch.
- Realer Lauf: `tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1` ODER cmake `test_lazy_static_dynamic_driver` (N=8–12): loaded>0, measured>0, CSV aus EINEM POD (stat_* + seg_* Pfad B).
- **Alt-DLL-Reject literal:** eine vor-dem-Bump gebaute Major-2-DLL gegen neuen Loader → `status_abi_major_mismatch` (nicht annehmen — messen).

---

## §5 INVARIANTEN (must_not_break)
- **modules/*** = TOTE Spiegel — NIE anfassen; realer Build inkludiert NUR `libs/cache_engine`.
- **Q1-Sequenz** in der EINEN `tier_observe`: axis_stats-READ → seg_ns-Timing → per-op-Reset (sonst Doppelzählung T0/T1/T2/T3/T7/T8/T10/T17/T18).
- **Pfad A** (`IMeasurableWorkload*`/`run_workload*` + `ComdareSegmentLatencyV2`, measurable_workload.hpp) BLEIBT (isolierter Achsen-Bench); seg_ns im POD = **Pfad B**.
- **kV3AxisSchema** = Single-Source Schreiber↔CSV-Spaltenname (NICHT umbenennen; Namen kV3* bewusst behalten).
- **Honest-0** (Baseline-Strategien echte 0-Teilfelder, KEIN „Fehler").
- POD `is_standard_layout && is_trivially_copyable` (memcpy über DLL-Grenze).
- **Wire-Format-Symmetrie:** `format_perm_result`↔`ingest_result_line` (175 Felder) MÜSSEN synchron bleiben (test_d14b/d14c).

## §6 BUILD/VERIFY
- Standalone cl: `build/scratch_compile_{obs_phaseA,pathb,all19,layout_store}.ps1` (braucht `build/msvc-release/generated/`).
- cmake (VS 2022): `cmake --build "<repo>\build\msvc-release" --target <test> --config Release` → `build\msvc-release\tests\unit\Release\<test>.exe`.
- Voll: `cmake --build "<repo>\build\msvc-release" --config Release` (alle Targets) → für I-C.2-Fehlerliste.
- Je grünem Increment: `git add` (ce) + commit + `git push` + Superprojekt `git add Code/external/comdare-cache-engine` + commit + push. CSVs ggf. force-add (gitignored).

## §7 NACH I-C/I-D — die verbleibenden offenen Mess-Echtheits-Punkte (separat, niedrigere Prio)
migration 2-Tier (`tier_moves>0`, memento-sensibel) · filter Train-then-Probe · io echtes Fixture · lazy DLL-Bibliothek (Content-Hash) · finaler realer Multi-Achsen-DLL-Lauf. Details: Plan-Datei „Danach"-Abschnitt.
