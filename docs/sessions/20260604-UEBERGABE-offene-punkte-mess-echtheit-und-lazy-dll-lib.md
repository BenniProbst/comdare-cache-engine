# ÜBERGABE 2026-06-04 — offene Punkte: Mess-Echtheit (6 Achsen) + realer DLL-Lauf + lazy DLL-Bibliothek

> **Resume-Dokument** (Kontext lief aus). Rolle: **Implementierungs-Agent** cache-engine/Thesis (NICHT Cluster; Infra-Wünsche nur via K78).
> Ultracode AN. Direktiven: [[feedback_never_guess_always_lookup_state_of_art_and_docs]] (Planung-vor-Bau Pflicht),
> [[feedback_all_axes_driven_and_timed_per_tier_binary]], [[feedback_no_success_marks_without_literal_output]].
> Arbeitsweise diese Session: Planungs-Workflow → Implementierungs-Workflow (parallele Achsen-Agenten + Kern + **adversariale Skeptiker**) → unabhängige Verifikation → Commit.

## 0. AUFTRAG (User 2026-06-04): „gehe ALLE offenen Punkte vollständig an"
7 Punkte (s. §3). Pläne dafür liegen verankert vor (s. §4). NÄCHSTER SCHRITT = Implementierung je Punkt (memento-sicher), adversarial verifiziert, dann finaler realer DLL-Lauf.

## 1. STAND: was FERTIG + committet+gepusht ist (Mess-Architektur)
Alle 19 SearchAlgorithm-Achsen sind in **beiden** Dimensionen real gemessen (kein `n/a`, kein `EmptyAxisSnapshot`):
- **Timing (Pfad A):** `ComdareSegmentLatencyV2 { int64 seg_ns[19] }` + `IMeasurableWorkloadV3` (additiv); `abi_adapter.hpp run_workload_segmented_v2` = 19 eigene `steady_clock`-Segmente. Commit `c7c5f2e`/`fa0fda9`.
- **Observer (Pfad B):** `ComdareTierObserverSnapshotV3 { uint64 axis_stats[19][8] }` + `IObservableTierV3` + `fill_observer_v3` + Schema `kV3AxisSchema` (observable_tier.hpp). Phase A (10 Achsen) Commit `1fa6045`; Phase B (9 Achsen) + 2 Adversarial-Fixes Commit `840cf80`.
- **Layout-Fix:** `cache_line_aligned` echter `aligned_stride=round_up(record_size,64)`; `kRecordSize 64→48` + `kLbufBytes=kRecords*64` (OOB).
- **2 Adversarial-Fixes (Phase B):** (1) `tier_clear()` resettet jetzt `statistics()` von ct/map/q1/q2/telemetry-Organen (war kumulativ); (2) Multi-Achsen-Grid zeigt Differenzierung. Doku korrigiert.
- **Letzter HEAD:** cache-engine `840cf80`, Superprojekt `5dd2d80` (alle gepusht).
- **CSV:** `tests/unit/thesis_tiere/tier150_measurements.csv` = aktuell der In-Process-Multi-Achsen-Grid (21 Z × 134 Sp, 94 stat_, filled_axis_count==19). Timing-Grid (72 Z, search_algo 6,7×) in History `c7c5f2e`.
- **Ehrliche Grenzen (heute, vom User gebilligt, jetzt zu VERTIEFEN):** migration=decide-only (tier_moves=0), io=In-Memory-Sim, value_handle/filter/patricia teils synthetisch, prefetch near-no-op.

## 2. WICHTIGE INFRA-FAKTEN (verifiziert)
- **DLL-Ort:** `build/thesis_tiere/tiere/<stem>/` (perm.cpp/.dll/.obj/.cl.log/.version); `<stem>` hat MAX_PATH-Hash-Suffix (`orch_fnv1a_hex(binary_id)`, build_orchestrator.hpp:122/138) — das ist NUR Pfad-Hash über die Achsen-WAHL, KEIN Content-Hash (→ Punkt 7).
- **Modul-Kopien `modules/*` = tote V1-only-Snapshots — NIE anfassen.**
- **Harness:** `tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -SelectMode {index|search_algo_grid} -NRepeats 3 -BuildVersion <v>`; Host `run_lazy_150.cpp`.
- **In-Process-Grid trippt bei großem Kartesisch einen latenten Heap-Issue (allocation-history-abhängig); über die REALE DLL-Grenze (jede Binary eigener Prozess) tritt er NICHT auf** → realer DLL-Lauf ist der saubere Weg (Punkt 6).
- **memento/rollback (V5-I6/I7):** `tier_save_all`/`tier_rollback_all` (abi_adapter.hpp:1095-1116) sichern HEUTE nur `search_organ_` + `container_`. Jede NEUE persistente Struktur (migration tier1!) MUSS dort mitgesichert werden, sonst I6-Bruch.

## 3. DIE 7 OFFENEN PUNKTE
1. **migration_policy** → echter 2-Tier-Store + `migrate_step()` (tier_moves real >0), memento-sicher.
2. **io_dispatch** → echtes IO als **Fixture** (NICHT Mess-Pfad; Portabilität + DRAM-Bench-Sauberkeit).
3. **value_handle / filter / path_compression(Patricia)** → echter Pool/Chain-Deref · real aus Daten gebauter Filter · materialisierter Patricia-Trie.
4. **prefetch** → mikroarchitektur-wirksam mit messbarer Divergenz.
5. **realer Multi-Achsen-DLL-Mess-Lauf** → Timing+Observer in EINER CSV, viele Achsen variiert (DLL-Prozess-Isolation).
6. (= Teil von 5)
7. **inhalts-abgeleitete Versionierung je Tier → lazy DLL-Bibliothek** (NEU, User-Hinweis): Version ändert sich nur, wenn der für ≥1 Achse gewählte Algorithmus im CODE aktualisiert wird → nur betroffene Tiere neu bauen.

## 4. VERANKERTE PLÄNE (gerade fertig — Essenz hier gesichert; volle Specs in den Temp-Task-Outputs)
**Plan-Temp-Dateien (evtl. ephemer — Essenz unten):**
- 6-Achsen+DLL-Lauf: `…/tasks/wdbx72gu1.output` (JSON `result`).
- Lazy-DLL-Bibliothek: Agent `affb40fd6cc4d2724` (im Transcript).

### 4.1 migration 2-Tier (VOLL verankert, §1 der wdbx72gu1-Spec)
- `axis_migration_observable.hpp`: neues **stateless Prädikat** `bool should_migrate_record(rec, record_size)` (if constexpr je Strategie: HotCold cold-Vote/TierBased %/Adaptive score&1/None false) — KEIN stats_-Inkrement.
- `axis_04_node_type_chunked_store.hpp`: `organ_migrate_step<MigOrgan>(org, NodeChunkedStore& tier1)` via verifiziertes `flatten_()`/`rebuild_()`-Pattern (2-Phasen: markieren → markierte in `tier1.append_slot`, Rest rebuilden; löst R4 Iteration-während-Mutation). `append_slot` = einzige Append-API; tier1 = voller Store via `container_.store()`.
- `abi_adapter.hpp`: Member `container_t container_tier1_{}` (nach ~Z.1175); neues Sub-Interface **`IMigratableTier { tier_migrate_step(max_moves)->uint64 }`** (additiv, dynamic_cast-Degrade); Impl: `mig_organ_.reset()` → `organ_migrate_step` → `add_tier_moves(n)` an der Hülle (stats_ privat → friend/Methode). **KRITISCH: NICHT in fill_observer_v3** (Observer muss idempotent bleiben) — echter Move im Treibe-Pfad.
- **Memento (load-bearing):** `tier_save_all`/`tier_rollback_all` symmetrisch `container_tier1_` mitsichern (MementoAxis bevorzugt, sonst std::optional-Kopie); `tier_rollback_all` zusätzlich `mig_organ_.reset()`; `tier_clear` + `container_tier1_.clear()`; `tier_rollback_is_exact` tier1_ok. **MementoAggregate UNVERÄNDERT** (tier1 ist impl-intern, migration-Slot bleibt EmptyMemento). Aufwand M. Risiken R1 Memento-Bruch (mitigiert), R3 Pfad-B (mitigiert), R4 (flatten-rebuild).

### 4.2 io_dispatch echtes IO (§2)
- **Entscheidung: Test-Fixture `tests/unit/.../test_v5_io_real_fixture.cpp`, NICHT der DLL-Mess-Pfad** (echtes IO würde alle 19 Segment-Timer verjittern + bricht Linux/ZIH-Portabilität der DLL). Kein ABI-Change.
- Win-MSVC: InMemory=Baseline; Buffered=`CreateFile`(tmp)+`ReadFile`; Direct=`FILE_FLAG_NO_BUFFERING`+`VirtualAlloc`-512-aligned; Mmap=`CreateFileMapping`+`MapViewOfFile`+`VirtualAlloc(MEM_RESET)` zw. Batches. Misst syscall/Align/Page-Fault — ehrlich.

### 4.3 value_handle / filter / patricia (§3 — Volltext in wdbx72gu1.output nachlesen)
Echter Weg je Achse: value_handle echter Pool/Version/Chain-Deref gegen reale Slot-Struktur; filter ein REAL aus den eingefügten Keys gebauter Bloom/Cuckoo/SuRF/Xor + Probe; Patricia materialisierter Radix-Trie + echter Descent. **static `*_scan`-Signaturen NICHT brechen** (seg19-Aufrufer). Details in der Temp-Spec.

### 4.4 prefetch (§4) + realer DLL-Lauf (§4)
- prefetch: echter `_mm_prefetch` über großen Working-Set mit/ohne Distanz → seg_prefetch_ns + PrefetchStatistics differieren messbar.
- **Realer DLL-Lauf:** neuer SelectMode in `run_lazy_150.cpp` (z.B. `one_wise` via `coverage_selection::select_one_wise` ODER Multi-Achsen-Grid) → variiert viele Achsen über REALE DLLs (Prozess-Isolation, kein In-Process-Heap-Issue); Timing(seg_ns[19])+Observer(axis_stats[19][8]) in EINER CSV. Harness `-SelectMode` erweitern.

### 4.5 LAZY DLL-BIBLIOTHEK (Punkt 7 — VOLL verankert)
**Ziel:** per-Tier-Version = `hash(schema_version || content_hash(W0) || … || content_hash(W18))`; ändert sich nur bei Code-Update einer GENUTZTEN Achse → nur betroffene Tiere rebuilden.
- **Heute:** `build_orchestrator.hpp` `dll_is_current(output, build_version)` + `write_version_sidecar` nutzen EINEN GLOBALEN String (`-BuildVersion`); `orch_fnv1a_hex` hasht nur den binary_id (Wahl, nicht Inhalt). SHA256-System (`src/sha256/ctsha.hpp`, `is_original`-Makro) ist nur Habich-Paper-Validierung, KEIN Build-Versions-Hash.
- **Empfohlen (Option A, Build-Zeit-Datei-Hash):**
  1. Codegen (CMake Custom Target, **kein Python**): SHA256 je Wrapper-Header in `topics/`+`axes/` → `build/msvc-release/generated/axis_content_hashes.hpp` (`constexpr kAxisContentHash_<wrapper_name> = "..."`).
  2. Neuer `tier_version_computer.hpp`: `compute_tier_content_hash(BinarySpec&)->string` (Lookup je `spec.axes`-Wrappername + FNV1a-Aggregat).
  3. `BuildConfig` + `TierVersionFn tier_version_fn` (optional, Default nullptr=rückwärtskompat); `provision_core`: `effective_version = build_version + "|" + tier_version_fn(spec)`.
  4. `run_lazy_150.cpp`: `cfg.tier_version_fn = &compute_tier_content_hash`. `-BuildVersion` bleibt Schema-Version (nur bei ABI/Define-Bruch hochzählen).
- **Offene Entscheidungen:** E1 Granularität (direkte .hpp vs `topics/<achse>/`-Ordner-SHA vs `cl /P`-Expansion für transitive Includes), E2 Aggregat (FNV1a-Kette reicht), E4 Erst-Lauf rebuildet alles einmal (erwartet, dokumentieren), E5 später consteval-`content_digest()` je Wrapper als präziser 2. Kanal.

## 5. RESUME-REIHENFOLGE (NÄCHSTE SESSION)
1. Volle Specs lesen: `…/tasks/wdbx72gu1.output` (§1-§6) + Agent affb40fd (lazy DLL). Falls Temp weg: §4 hier ist die Essenz.
2. Implementieren (je Punkt eigener Implementierungs-Workflow, memento-sicher, adversarial verifiziert, je Schritt grün baubar): **Reihenfolge** = (a) lazy DLL-Bibliothek (Infra, beschleunigt alle Folge-Builds) → (b) value_handle/filter/patricia/prefetch (Wrapper-lokal, parallelisierbar) → (c) migration 2-Tier (memento-sensibel, einzeln) → (d) io-Fixture (eigenständig) → (e) realer Multi-Achsen-DLL-Lauf (SelectMode one_wise) als FINALE Messung.
3. Je Punkt: unabhängig verifizieren (literal), committen+pushen, Superprojekt-Submodul-Bump.
4. Bestehende Tests grün halten: `test_obs_phaseA`, `test_all19_segment_timer`, `test_d_v42_abi_telemetry_coupling`, `test_segment_timer_differentiation`.
5. ABI-Regel: jede neue Fähigkeit als EIGENSTÄNDIGES Sub-Interface (dynamic_cast-Degrade), V1/V2/V3 nie append-mutieren.

## 6. COMMIT-DISZIPLIN
Nach jedem cache-engine-Push: Superprojekt `git add Code/external/comdare-cache-engine` + commit + push (Submodul-Sync). Force-add CSVs (`*.csv` gitignored, wie thesis/tier150). Co-Authored-By-Trailer.
