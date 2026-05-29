# Session — Roadmap-3 (Säule-2 Mess-Pfad vollenden): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow · **Task:** #38 (User-Roadmap Schritt 3, Pflicht)
**Workflow:** `wyp5eu78l` / Run `wf_99d032c4-f40` — 7 Agenten, ~756k Subagent-Tokens, 203 Tool-Uses.
**Zweck:** Agenten-Ergebnisse für spätere Konsultation festhalten, BEVOR implementiert wird.
**Bezug:** Doku 24 §2.1 (Tier-Wall-Clock als Akkumulation), §5.2/§5.3 (observe_all-Lücke), §5.4.3 (DLL-Pfad=R6). Vorgänger: Roadmap-2 (ce `7155cae`).

> **Frage:** Wie vollenden wir Säule 2 — pro Permutation/Füllstand den observe_all-Trace erheben (search_algo+allocator real)
> UND die Tier-Wall-Clock anreichern (Latenz-Kurven über Füllstand, read/write/delete getrennt, RAM/Disk)?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)

### 1.1 Ist-Mess-Pfad
- **`abi_adapter::run_workload`** (`abi_adapter.hpp:136-200`): misst Wall-Clock eines 3-Segment-Batch (search/allocator/layout) via `steady_clock` **direkt in der DLL** — `noexcept`, vtable-fixiert; treibt lokale Organe, **umgeht** AnatomyExecutionContext + observe_all. F15-CLI (`apps/f15_compare/main.cpp:137`) ruft es via `dynamic_cast<IMeasurableWorkload*>`.
- **WICHTIGE KORREKTUR (Synthese):** Die §5.2-Lücke ist **Builder-seitig bereits geschlossen** (Roadmap-1): `AnatomyExecutionContext::observe_all()` (`:75-91`) liefert search_algo (`:81`) **und** allocator (`:87`) real (Test `R5B_ObserveMultiAxes`). Die Lücke besteht **nur** im DLL-internen `run_workload` → observe_all durch die .dll-Grenze = **R6** (additive ABI-Methode nötig). Dieser Increment misst **in-process** über die vorhandene Context-Kette.

### 1.2 Tier-Metriken
- `ExecuteEngineCommand` misst Gesamt-Latenz (op_latencies_ns, p50/p99 via `latency_stats::percentile_ns`) + Welch/MWU/Cliff's-δ (`welch_t_test`/`mann_whitney_u_test`/`multi_compare`), CSV/JSON via `result_aggregator`. **Fehlt:** Latenz getrennt nach read/write/delete + Kurven über Element-Füllstand (Doku 24 §2.1). `OperationOutcome` hat kein Op-Typ-Feld; latency_samples_ns ist undifferenziert.

### 1.3 RAM/Disk-Portabilität
- Allocator-Achse liefert bereits `total_bytes_in_use` (AllocationStatistics, Roadmap-1) → **portabler RAM-Proxy ohne OS-API** (MSVC/Windows, kein psapi-Bruch). Prozess-RSS/WorkingSet (psapi) + echte Disk-Persistenz (axis_10-Serializer sind struktur-only) = V42-Folge.

---

## 2. Design-Phase (3 Linsen, alle risk=low) + gewählt

| Linse | Kern | Verdikt |
|-------|------|---------|
| A — observe_all-Trace am Permutations-Ende | minimal | Graft (Brücke `make_execution_result`) |
| B — r/w/d-Segment-Mechanik | Op-Typ-Trennung | Graft (r/w/d-Phasen) |
| **C — einheitlicher Füllstands-Treiber** | EIN Lauf liefert beide Dimensionen pro Füllstand-Checkpoint | **GEWÄHLT** |

LINSE C ist überlegen: ein Treiberlauf erhebt pro Füllstand-Checkpoint observe_all (search_algo+allocator) UND Tier-Wall-Clock r/w/d — erfüllt Doku-24-§2.1 direkt (Kurven über Füllstand). Entspricht dem bewiesenen `R5B_ObserveMultiAxes`-Muster (ein observe_all → 2 reale Achsen).

---

## 3. Gewählter Blueprint (`TierObserveTrace`, Builder-seitig, in-process)

**NEU** `builder/anatomy_commands/tier_observe_trace.hpp` (header-only):
- `enum class TierOpType {Read,Write,Delete}`; `TierTraceConfig{fill_checkpoints{10,100,1000}, lookups/deletes_per_checkpoint, seed}`.
- `FillLevelSnapshot{fill_level, load_factor, read_ns/write_ns/delete_ns (Rohsamples), ram_bytes_in_use, #ifdef STATISTICS: search_algo_at_checkpoint + allocator_at_checkpoint}`.
- `template<IsComposition C> TierObserveTrace<C> drive_tier_observe_trace(cfg)`: treibt `AnatomyExecutionContext<C>` (→ `ObservableComposedSearch<…, ComposedStore<N,L,A>>`, breiter uint64). Pro Checkpoint: WRITE-Phase (insert bis Füllstand, je steady_clock-umklammert) · READ-Phase (lookups, ~50% Hit/Miss, volatile sink) · DELETE-Phase (erase+reinsert → Füllstand stabil) · dann **ein** `observe_all()` → search_algo+allocator real + `ram_bytes_in_use = allocator.total_bytes_in_use`.
- Auswertung p50/p99 pro Op-Typ via vorhandenem `latency_stats::percentile_ns` (kein neuer Perzentil-Code). Optionale Brücke `make_execution_result` → multi_compare/CSV (ExecutionResult unangetastet).
- Alle STATISTICS-Felder/Zugriffe unter `#ifdef COMDARE_CE_ENABLE_STATISTICS`; **kein** Runtime-Switch (Präprozessor + `if constexpr`).

**NEU** `tests/unit/test_v41_tier_observe_trace.cpp` + **EDIT** `tests/unit/CMakeLists.txt` (additive Test-Registrierung, Block-Muster wie test_v41_builder_anatomy_commands + Boost::mp11 + ALL_AXIS_GENERATED_DIRS).
**Unangetastet:** abi_adapter.hpp, f15_compare/main.cpp, execution_result.hpp, AnatomyExecutionContext.

---

## 4. Build-grüne Reihenfolge + Test-Plan
Inc-3a Header (nur verifiziert vorhandene Includes) → Inc-3b Test + CMake → Build/ctest (STATISTICS-ON Default). Tests: T1 Kurven-Vollständigkeit (3 Checkpoints, fill_level={10,100,1000}, load_factor monoton) · T2 r/w/d-Trennung · T3 (STATISTICS) observe_all real pro Checkpoint (insert/lookup/allocation_count>0, kumulativ) · T4 RAM-Monotonie (bytes_in_use steigt mit Füllstand) · T5 Idempotenz · T6 (STATISTICS OFF) Gating · T7 latency_stats-Reuse (p50≤p99) · T8 make_execution_result-Brücke · T9 TYPED_TEST über {Art,Hot}Composition.

## 5. Scope-Grenze (R6/V42-Folge, NICHT jetzt)
- observe_all **durch die .dll-Grenze** (additive ABI `IObservableWorkload`, run_workload-Context-Umstellung) = R6.
- Echte Welch/MWU/Holm-Auswertung je (fill_level × op_typ) = Inc-3c. CSV/JSON-Erweiterung (fill_level/r-w-d-Spalten) + F15-CLI + LaTeX = ausgelagert (result_csv_header-Vertrag nicht brechen).
- OS-RSS via psapi (MSVC-Infra-Risiko → RAM bleibt allocator-bytes_in_use); echte Disk-Persistenz (axis_10); .so/.dll-Permutations-Builds (hier in-process); Tier-Wrapper-Umstufung (#40); Säule-3-Achsenvergleich (#39).
