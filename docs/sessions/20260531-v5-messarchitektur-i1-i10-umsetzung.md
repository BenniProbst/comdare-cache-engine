# V5 Mess-Architektur — Umsetzung I1–I10 (Session 2026-05-31)

> **Handoff-Doku.** Single-Source-of-Truth bleibt das Ledger
> `docs/sessions/architektur-ziele-offene-punkte-ledger.md` (Block (a.V5)). Diese Notiz fasst den
> Umsetzungs-Strom dieser Session zusammen, damit nach einer Kontext-Summierung lückenlos fortgesetzt werden kann.

## /goal V5 (Stop-Hook, verbatim Kern)

„Mess-Architektur (Memento_all + Observer_all · 3 Profile · host-seitige Workloads · Konformitäts-Gate) +
Erhalt der E2E-Abnahme. Fahre autonom fort, bis umgesetzt + Audit-Workflow-bestätigt, OHNE Regress der
abgenommenen P1–P5 (V3/V4)."

GRUNDMODELL (zwei Seiten):
- **TIER-BINARY (.dll):** exportiert IMMER `IDriveableTier` (funktionaler Gattungs-Antrieb, ABI-stabil);
  `observer_all` + `memento_all` NUR bei Messung-AN compile-time einkompiliert (reine Metaprogrammierung,
  KEIN dynamic_cast zur Entfernung). Messung-OFF = funktional-only, auslieferbar, kein Overhead. KEINE
  Workloads in der DLL (run_workload-in-DLL war V3-Designfehler → host-seitig relokalisiert).
- **HOST (CacheEngineBuilder):** Prüf-Dock je Gattung; `IMeasurableWorkload` rein generisch host-seitig;
  mehrere Lastprofile je Binary; Lebenszyklus import→Konformität→messen→abstoßen.
- **ZWEI-PHASEN pro Op (Default):** memento-save-all → op (warmup, verworfen) → rollback-all → op (measure).
- **DREI PROFILE:** Build (statisch, welche DLLs) ⊥ Lastenprofil (host) · Compile-Release (cmake, Default Messung-AN).

## Inkremente I1–I10 — Stand Ende Session (alle Commit-Refs in comdare-cache-engine)

| Ink. | Inhalt | Commit | Verifikation |
|------|--------|--------|--------------|
| I1 | 3-Profil-cmake-Flags `COMDARE_MEASUREMENT_MODE`/`RELEASE_MODE`→`MEASUREMENT_ON` | (frühere Session) | 3 Szenarien |
| I2 | ABI-Split `IDriveableTier`/`IObservableTier`, konditionale Adapter-Vererbung, ABI Major 1→2 | (frühere Session) | Voll-Build 0 C++-Fehler; Tags `pre-v5-i2-abi-break`/`post-v5-i2.2-abi-major2` |
| I4 | Konformitäts-Gate `run_conformance_gate` (RF1–RF7 vs std::map-Orakel) + Dock-Integration | (frühere Session) | Standalone 4249/4249 |
| I5 | `MementoAxis`-Concept + `EmptyMemento` + `MementoAggregate` | `99a10b4` | Round-Trip ALL PASS; gtest `test_v5_memento_axis` |
| I6 | `IRollbackableTier`-ABI (`rollbackable_tier.hpp`) + Adapter-Wiring + exakter In-Memory-Memento; ABI Minor 0→1 | `ded1969`+`7e88a26` | Standalone gg. ArtComposition EXITCODE=0, Organe kopierbar→exakt; Tags `pre-v5-i6-abi-rollback`/`post-v5-i6-abi-minor1` |
| I7 | Zwei-Phasen-Treiber `drive_two_phase_tier_trace_abi` + `detail::two_phase_measure` | `93dadb0` | Invariante 2phase==1phase==cold ALL PASS; gtest `test_v5_two_phase_driver` |
| I8 | Disk-Achsen-Memento (R1) | `6dc071e` | **aufgelöst durch Ist-Stand-Feststellung**: kein Disk-I/O, Achsen stateless → memento_all bereits vollständig; `messarchitektur_v5_i8_memento_vollstaendigkeit.md` |
| I9 | `WorkloadOrchestrator` (host-seitig generisch) + `MeasurementPlan` + YCSB-C/D + Serialisierung | `7f2b08f` | Standalone ALL PASS (5 Checks); gtest `test_v5_workload_orchestrator` |
| I10 (Teil) | Pruef-Dock misst zwei-phasig per Default (memento_all eingehängt) | `10b2213` | Standalone-Compile EXITCODE=0 |
| I10 (Teil) | 3-Profil-Doku `messarchitektur_v5_drei_profile.md` | `57f939a` | — |

## Kern-Dateien (V5)

- `libs/cache_engine/anatomy/idriveable_tier.hpp` — IDriveableTier (5 Ops, immer)
- `libs/cache_engine/anatomy/observable_tier.hpp` — IObservableTier : IDriveableTier (+ tier_observe)
- `libs/cache_engine/anatomy/rollbackable_tier.hpp` — IRollbackableTier (tier_save_all/tier_rollback_all)
- `libs/cache_engine/anatomy/memento_aggregate.hpp` — MementoAxis-Concept + EmptyMemento + MementoAggregate
- `libs/cache_engine/anatomy/abi_adapter.hpp` — SearchAlgorithmAbiAdapter: konditionale Vererbung (#if MEASUREMENT_ON), In-Memory-Memento via Tiefkopie
- `libs/cache_engine/builder/pruef_dock/conformance_gate.hpp` — std::map-Konformitäts-Gate
- `libs/cache_engine/builder/anatomy_commands/tier_observe_trace_abi.hpp` — Einphasen- + Zwei-Phasen-Treiber
- `libs/cache_engine/builder/workload_driver/workload_orchestrator.hpp` — Host-Lastprofil-Orchestrator
- `libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp` — ABI Major 2 / Minor 1

## ABI-Stand

- `COMDARE_ANATOMY_ABI_MAJOR = 2` (immer-präsenter IDriveableTier-Kontrakt), `MINOR = 1` (MESSUNG-AN +
  IRollbackableTier), Magic `0x434F4D444141322E` (".A2.", kodiert Major).
- Messung-AN-Adapter erbt: IAnatomyBase + IObservableTier(→IDriveableTier) + IMeasurableWorkload + IRollbackableTier.
- Messung-AUS-Adapter erbt: IAnatomyBase + IDriveableTier (nur).
- Loader-`dynamic_cast` auf fehlende Sub-Interfaces → null → grazile Degradation (Kalt-Messung).

## VERBLEIBEND (I10-Rest, multi-session)

1. **Voll-Build + Regression grün** (P1–P5 V3/V4 + neue V5-gtests `test_v5_conformance_gate`,
   `test_v5_memento_axis`, `test_v5_two_phase_driver`, `test_v5_workload_orchestrator`). ⚠️ braucht
   Reconfigure (~10min configure-Zeit-Codegen über OneDrive) + Build. **Diese Integrations-Verifikation
   steht noch aus** — bisher ist jedes Inkrement nur STANDALONE verifiziert (cl /c gegen echte Header).
2. **End-to-End-Mess-Kette**: `f15_compare`/Dock real laufen lassen → Konformität→Zwei-Phasen→16+6-CSV→PDF
   mit ECHTER Two-Phase-Messung (nicht nur Mock). CLI-Einhängung `run_measurement_plan` in `f15_compare`.
3. **Finaler Audit-Workflow** + Bestätigung P1–P5 unverändert grün.
4. R-Refinement (optional, Korrektheit unberührt): inkrementeller Undo-Log-Memento statt Voll-Organ-Kopie
   für große Füllstände (Performance der Mess-Harness, NICHT der gemessenen Größe).

## Git-Stand

comdare-cache-engine HEAD ≈ `57f939a`, **~33 Commits vor origin/main, UNPUSHED** (FortiGate-SSL-Inspection
blockt git push; ein anderer Agent kümmert sich um Cert+Push — KEINE konkurrierenden Pushes). DA-Superprojekt-
Pointer NOCH NICHT auf neuen CE-HEAD gebumpt (nach Push). Tags lokal: `pre/post-v5-i2-*`, `pre/post-v5-i6-*`.

## Disziplin-Notizen

- Kein „done" ohne literale Tool-Ausgabe ([[feedback_no_success_marks_without_literal_output]]) — daher jedes
  Inkrement standalone cl-verifiziert (EXITCODE/RESULT ALL PASS zitiert).
- I8 zeigt die Ist-State-Disziplin: KEIN Fake-Disk-Checkpoint für zustandslose Achsen erfunden.
- Destruktive Ops (ABI-Bruch) mit Tag+Commit ([[feedback_destructive_autonomy_3repos_with_tag]]).
