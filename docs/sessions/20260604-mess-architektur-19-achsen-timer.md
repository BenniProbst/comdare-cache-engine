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
