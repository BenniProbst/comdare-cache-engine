# Session 2026-06-04 — Mess-Architektur: per-Achsen-Timer auf ALLE 19 SA-Achsen + Layout-Fix

## Stand: UMGESETZT + verifiziert + committet

Der per-Achsen-Timer (F15-Hybrid Pfad A) deckt jetzt **alle 19 SearchAlgorithm-Achsen** ab (Direktive
[[feedback_all_axes_driven_and_timed_per_tier_binary]]: jede Achse je Tier-Binary getrieben + gemessen, **kein `n/a`**).

**Verankert (Pflicht-Planungssession gegen Architektur+Doku+Ist, kein Raten) → implementiert (Ultracode-Workflow:
8 Achsen-Ops parallel → Kern → 3 adversariale Skeptiker, alle PASS) → finaler Lauf.**

- **ABI:** neu `ComdareSegmentLatencyV2 { int64 seg_ns[19]; total_ns; batches }` + `IMeasurableWorkloadV3`
  (additiv, **kein ABI-Bruch** — V1/V2 + IMeasurableWorkloadV2 unverändert; alte DLLs degradieren via dynamic_cast).
- **abi_adapter.hpp:** `run_workload_segmented_v2` mit 19 eigenen `steady_clock`-Segmenten (Setup je Achse einmal,
  Op-Schleife gemessen, sink gegen Wegoptimierung); alle 19 via `composition_t::<axis>` verdrahtet.
- **Layout-Fix (Befund 2):** `cache_line_aligned` echter `aligned_stride = round_up(record_size,64)` (vorher
  byte-identisch zu `aos_strict`); Pflicht-Kopplung `kRecordSize 64→48` + `kLbufBytes = kRecords*64` (OOB-Schutz).
- **CSV:** 19 `seg_<achse>_ns`-Spalten (single-source `kCompositionAxisNames`), `na_axes` ersatzlos entfernt.
- **Modul-Kopien (`modules/*`) sind tote Snapshots → NICHT angefasst.**

**Verifiziert (literal):** `tier150_measurements.csv` = 72 Zeilen (4 search_algo × 6 dyn × 3 Reps) × 39 Spalten;
**0 von 1368 seg-Zellen ≤0** (alle 19 Achsen real, kein n/a); `seg_search_algo_ns` differenziert interpolation 3,72M →
k_ary 16,58M → linear_scan 25,09M ns (Faktor ~6,7×); Layout-Fix CLA(64) ≠ aos_strict(48) −12,4%; `test_segment_timer_differentiation` (V1/V2) weiter grün.

## ⚠️ PERSISTIERTER FOLGEPUNKT (User 2026-06-04) — konkrete Per-Achsen-Nacharbeit

> **User verbatim:** „Das sollte konkret je Achse behoben werden … Ehrlichkeit vorab (transparent): 8 der 19 Achsen
> sind strukturell nur über eine synthetische, strategie-charakteristische Mess-Operation timebar (z.B. migration/io/
> filter ohne 2. Tier bzw. echtes IO). Die liefern reale, vom Organ bestimmte Laufzeiten (kein erfundener Wert), werden
> aber im Doc als Simulation gekennzeichnet — so erfüllen sie ‚alle Achsen gemessen, kein n/a' ohne Überzeichnung."

**Auftrag:** Jede der heute synthetisch/simulativ getriebenen Achsen bekommt eine **ECHTE, axen-eigene Operation**
(grounded im tatsächlichen Algorithmus der Achse), KEINE Simulation mehr. Aktueller Zustand → Ziel je Achse:

| Achse | Heute (Mindest-Op) | Konkret zu beheben (echte Op) |
|---|---|---|
| T3 path_compression (Patricia) | `path_descend_scan` = synthetischer 1-Bit-Descent über Roh-Puffer (kein echter Trie) | echten materialisierten Patricia-/Radix-Descent über die reale Trie-Struktur timen |
| T11 value_handle (ExternalPool/Versioned/ChainRef/Immutable) | Zugriffs-**Simulation** auf lbuf | echten Pool-/Version-/Chain-Deref gegen die reale Value-Handle-Struktur |
| T13 index_organization (NonClustered/IOT/Heap) | synthetischer LCG-Random-Hop | echten Index-Org-Zugriff über die reale Index-Struktur |
| T14 io_dispatch (Buffered/Direct/Mmap) | reine In-Memory-Dispatch-**Simulation** (kein echtes IO) | echten IO-Dispatch-Pfad (mmap/buffered) — ggf. gegen tmp-Datei/echtes mmap |
| T15 migration_policy (HotCold/TierBased/Adaptive) | Entscheidungs-Scan ohne 2. Tier | echte Migration über eine reale 2-Tier-Struktur |
| T16 filter (Bloom/Cuckoo/SuRF/Xor) | Probe-**Simulation** auf Pseudo-Bitmap | echte Filter-Probe gegen einen real aus den Daten gebauten Filter |
| T7 prefetch (None/Hardware) | Hint-Loop (kann near-no-op sein) | mikroarchitektur-wirksame Prefetch-Op mit messbarer Divergenz |
| T8 concurrency (None) | bewusste 0-Overhead-Baseline | (None bleibt Baseline; Blocking/RW/LockFree sind bereits ECHT — std::mutex/shared_mutex/atomic-CAS) |

**Prinzip:** nach der Nacharbeit ist KEINE Achse mehr „Simulation"; jede misst ihren echten Organ-Algorithmus. Bis dahin
bleiben die Werte ehrlich als reale (organ-bestimmte) Laufzeiten der jeweiligen Mindest-Op gekennzeichnet — kein erfundener Wert.
Pro Achse Goldstandard-konform + verifiziert (per-Achsen-Differenzierung in der CSV nachweisen). Planung-vor-Bau Pflicht.

---

## Adversarial-Fix (2026-06-04, später): zwei Defekte im Phase-B-Per-Achsen-Observer

Zwei adversarial gefundene Defekte am Per-Achsen-Observer-Pfad (Phase B, `axis_stats[19][8]`) behoben. **NICHT committet** (nur `libs/` + `tests/thesis_tiere`; Modul-Kopien sind tot).

### DEFEKT 1 (Konsistenz) — `tier_clear()` resettet die kumulativen statistics der auto-gekoppelten Instanz-Achsen

**Befund:** `abi_adapter.hpp tier_clear()` rief für die auto-gekoppelten Instanz-Achsen T1 cache_traversal (`ct_organ_`), T2 mapping (`map_organ_`), T17 queuing_q1 (`queuing_q1_organ_`) nur `clear()` (= nur DATEN: `entries_`/`mappings_`/Puffer) — NICHT das separate `reset()` (= `stats_ = {}`). T18 queuing_q2 (`queuing_q2_organ_`, `LazyFlush`) hat gar kein `clear()` → wurde NIE zurückgesetzt. Folge: deren `statistics()` und damit die V3-`axis_stats` AKKUMULIERTEN über die 3 Wiederholungen je (Binary×Setting) — ein kumulatives Artefakt, das der V1-Delta-Block (search_algo/allocator via post−pre) eliminiert, der V3-Block aber NICHT.

**Mit-Befund (über den Auftrag hinaus, für „alle 19 Achsen bei 0" nötig):** T10 telemetry (`telemetry_organ_`, `ObservableTelemetry`) wird ebenfalls per `tier_insert/lookup` (`record_node_touch`) auto-gekoppelt und in `fill_observer_v3` DIREKT gelesen → akkumulierte identisch. `ObservableTelemetry::reset()` existiert (`axes/telemetry_axis/axis_11_telemetry_observable.hpp:76`), wurde aber nicht aufgerufen.

**Fix (`libs/cache_engine/anatomy/abi_adapter.hpp`, `tier_clear()`):** zusätzlich zu den bestehenden `clear()`-Aufrufen jetzt `reset()` für `ct_organ_` (T1), `map_organ_` (T2), `queuing_q1_organ_` (T17), `queuing_q2_organ_` (T18) **und** `telemetry_organ_` (T10), alle if-constexpr-geschützt. T7/T8/T3 riefen `reset()` bereits. Ziel erreicht: nach `tier_clear()` sind ALLE 19 Achsen-statistics bei 0 → je Messung frisch, konsistent mit dem V1-Delta.

**Doku-Korrektur (ehrlich, [[no-success-marks-without-literal-output]]):** die falsche „warmup-frei/delta-äquivalent/konsistent"-Behauptung in `perm_runner.hpp` (run_observable_perm-Kommentar) und `cache_engine_builder_iterator.hpp` (stat_*-Semantik) auf den TATSÄCHLICHEN Stand korrigiert: vor dem Fix akkumulierten T1/T2/T17 (clear-only) + T18/T10 (gar nicht); seit dem Fix gilt die Behauptung wieder.

**Verifikation (literal, in-process, 3 Reps je Binary):**
```
T1 cache_traversal reps=[4000,4000,4000]
T2 mapping          reps=[4000,4000,4000]
T17 queuing_q1      reps=[2000,2000,2000]
T18 queuing_q2      reps=[3000,3000,3000]
```
→ über die 3 Reps GLEICH (kein 1000→2000→3000-Artefakt). Vor dem Fix wären es 4000→8000→12000 gewesen.

### DEFEKT 2 (Sichtbarkeit) — die Differenzierung der 18 Nicht-search-Achsen im Pilot nicht sichtbar

**Befund:** die `statistics()` differenzieren real je STRATEGIE (belegt), aber der `search_algo_grid`-Pilot (run_lazy_150 / test_obs_phaseA) variiert NUR `search_algo`; alle FullPilot-Referenz-Kompositionen (Art/Hot/Masstree/Start/Surf/Wormhole) tragen IDENTISCHE Nicht-search-Achsen (Node256·CacheLineAligned·PathCompressionNone·BloomFilter·…) → die 18 anderen stat_*-Spalten sind über die Binaries KONSTANT.

**Demonstration (`tests/unit/thesis_tiere/tier150_axis_grid.cpp` + `build/scratch_compile_tier150_grid.ps1`):** ein Multi-Achsen-Grid (Art-Basis, NUR node_type × path_compression × filter variiert), je Binary 3 Reps, schreibt die WIDE-Schema-CSV nach `build/thesis_tiere/tier150_measurements.csv` (+ committe-bare Kopie `tests/unit/thesis_tiere/tier150_measurements.csv`). Kuratiertes 7-Binary-1-wise-Grid (alle 3 Werte jeder Achse), node256/none/bloom = Anker.

**Verifikation (literal, per-Achsen-stat-Summe je Binary):**
```
node_type   T4: node256=125973  node4=6949   node48=130277   (stat_node_type_keys: 256/4/48)
path_comp   T3: none=32007      bytewise=32007  patricia=34007 (stat_path_compression_prefix_len: 14000/14000/16000)
filter      T16: bloom=2329     cuckoo=2025  xor=2009         (stat_filter_pos: 71/12/4)
```
→ ALLE DREI nicht-search-Achsen differenzieren über die Binaries (Auftrag: ≥2). (T3 none==bytewise im Zähler ist korrekt — beide erzeugen denselben `common_prefix_len`-Aufwand; nur Patricia addiert die 1-Bit-Descent-Schritte.)

**Mess-Architektur-Doku (ehrlich, im CSV-Kopf-Kommentar + hier):** Op-ZÄHLER-statistics differenzieren je STRATEGIE (algorithmus-internes Verhalten: prefix_len, find/keys, probe/pos …). ALGORITHMEN unter IDENTISCHER deterministischer Last (z.B. Art vs Hot, beide 1000 lookups) differenzieren NICHT im Zähler, sondern in der ZEIT (`seg_*_ns`, geliefert von `test_all19_segment_timer` → `all19_pilot.csv`). Beide Dimensionen zusammen = das vollständige per-Achsen-Bild (F15-Hybrid).

**Scope-Hinweis (ehrlich):** das volle 3·3·3=27-Kreuzprodukt trifft im in-process-Stand-in einen latenten, allokations-historie-abhängigen Heap-Engpass (reproduzierbar layout-abhängig, NICHT durch eine einzelne Komposition; jede Komposition läuft isoliert sauber). GETRENNTES Problem, NICHT Teil dieser zwei Defekte; über die echte .dll-Grenze (run_lazy_150, je Binary ein eigener Prozess) entfällt es. Das kuratierte 1-wise-Grid (7 Binaries, alle Achsen-Werte abgedeckt) genügt für den Beleg.

### Geänderte Dateien (NICHT committet)
- `libs/cache_engine/anatomy/abi_adapter.hpp` — `tier_clear()`: reset() für T1/T2/T17/T18 + T10
- `libs/cache_engine/builder/experiment_tree/perm_runner.hpp` — run_observable_perm-Kommentar ehrlich korrigiert
- `libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp` — stat_*-Semantik-Kommentar ehrlich korrigiert
- `tests/unit/thesis_tiere/tier150_axis_grid.cpp` (neu) + `build/scratch_compile_tier150_grid.ps1` (neu)
- `build/thesis_tiere/tier150_measurements.csv` + `tests/unit/thesis_tiere/tier150_measurements.csv` (Beleg-CSV)

### Bestehende Tests weiter grün (literal)
- `test_obs_phaseA` → `==== Phase A Per-Achsen-Observer-V3: ALLE OK ====` (run_exit=0)
- `test_all19_segment_timer` → `==== 19-Segment-Timer + Layout-Fix: ALLE OK ====` (run_exit=0)
- `test_d_v42_abi_telemetry_coupling` → `OK: abi_adapter ... koppelt telemetry AUTOMATISCH` (run_exit=0)
