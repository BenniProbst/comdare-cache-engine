# Session-Übergabe 2026-06-10 — Undo-Log-Memento + Mess-Resume + Voll-Lauf-Neustart

> **Zweck:** Kontext-Übergabe an frischen Kontext. Enthält ALLE offenen Punkte. Der aktive Auftrag (User
> 2026-06-10): **(1) das geplante Refactoring (Undo-Log-Memento) durchführen, (2) Mess-Wiedereinstieg
> (Resume) implementieren, (3) den Voll-Lauf erneut fahren.** Anlass: Windows-Update hat die Maschine beim
> Voll-Lauf (~50%) neu gestartet.

---

## 0. Repo-Stand / Sync (alles gepusht)
- **cache-engine HEAD:** `d2f7bde` (origin/main). Superprojekt `\…\Diplomarbeit - Datenbanken` HEAD `5b3ce58`.
- Branch `main`. Arbeitskopie CLEAN (getrackte CSVs = committete 5761er-Version; die 47/140-MB-Roh-CSVs sind ungetrackt).
- 3 Diplomarbeit-Repos (Diplomarbeit/cache-engine/prt-art): destruktive Autonomie mit Commit+Push erlaubt (aktives /goal).

## 1. Was diese Session erreicht wurde (chronologisch)
1. **78 FullPilot-Build-Fails behoben** (`DistanceEstimatorPrefetch`: Concept `HasPrefetchTracker` verlangte nur `impl_type`, nicht `enqueue`) → **320/320 Lebewesen gebaut+gemessen** (ce `1fc3f56`, 5760 Zeilen, committet).
2. **Verfehlte Pflichtaufgabe entdeckt:** der 320-Lauf maß nur EINEN fixen Workload. Die **Workload/YCSB-Dimension = Achse 2** des kartesischen Mess-Kreuzes (messarch-v5 §73) war Pflicht, aber nie verdrahtet (parallel zur YCSB-„implementiert-aber-unverdrahtet"-Lücke).
3. **Achse-2 verdrahtet (INC-0..3c):** Workload als innerste dynamische B+-Baum-Ebene. `perm_runner::run_workload_perm` treibt den bestehenden Interpreter `workload_driver::run_workload_profile` (statt hartgecodetem insert/lookup-Loop). **Zwei-Phasen-Cache-Warmup ist PFLICHT** (Gültigkeit, nicht Geschwindigkeit — User-Direktive). + YCSB-**Load-/Run-Phasen-Trennung** (records vorab einfügen, sonst read-heavy = 100% Miss = ungültig).
4. **KOSTEN-BLOCKER gefunden + behoben:** Zwei-Phasen-Rollback war O(n²)/Op (`tier_rollback_all` → `restore_state` via Re-Insert). Empirisch: 10k/10k-Probe lief 17h ohne 1 Binary fertig → Voll-Lauf 11× über ZIH-Kontingent. **Fix = Copy-Memento bevorzugen** (`abi_adapter.hpp:1163-1184`: `saved=organ`/`organ=saved` O(n) statt Re-Insert O(n²); MementoAxis nur Fallback). Verifiziert: 10k/10k-Probe 63s statt 17h, Observer-Zähler SAUBER (Vollkopie restauriert auch Counter). ce `c8182ff`. **320 DLLs mit copymem-v1 neu gebaut** (alle vorhanden, reusable).
5. **CoCo-Trie-Mess-Direktive adoptiert (P04):** statischer Build + read-only look_up, Sweep `QUERY_NOT_IN_SET_PERCENTAGE` ∈ {0,25,50,75,100}. → `WorkloadConfig.negative_query_pct` + Generator-Support (`workload_generator.cpp next()`: Lookup/Scan mit Wahrsch. neg% auf `[key_max+1,2·key_max]` = garantierter Miss).
6. **XML-Lastprofil-Achse (User-Direktive: Lastprofile als XML, runtime-interpretiert, ALLE über ALLE Achsen = Bias-Bruch):**
   - **Schema** `algorithm_profiles/load_profiles/SCHEMA.md` (`comdare_load_profile`).
   - **Parser** `workload_driver/load_profile_parser.hpp`: `parse_load_profile` (XML→WorkloadConfig via self-contained `comdare::common::xml`) + `discover_load_profiles(dir)`. Workload-CHARAKTERISTIK aus XML, SKALA (records/n_ops) vom Aufrufer.
   - **Verdrahtung** (#135): `run_workload_perm` nimmt Registry `map<id,WorkloadConfig>` (XML primär / `profile_by_name`-env-Fallback). `LazyRunConfig.workload_configs`. `run_lazy_150` entdeckt `COMDARE_LOAD_PROFILE_DIR` → Registry + workload_values.
   - **21 Lastprofile** geschrieben (`load_profiles/*.xml`): CoCo-Sweep ×5 + YCSB a/b/c/d/e/f + ih/lh + 8 Paper-Archetypen. Alle parsen + valid.
   - ce `d2f7bde`.
7. **33-Paper-Lastprofil-Katalog** (Workflow `wn7b2fu44`, 34 Agenten) → **Doc 32** `docs/architecture/32_lastprofil_katalog_und_paper_bias.md`: 14 distinkte Lastprofile (LP01-LP14) + 13 Archetypen + **Paper-Bias-Befund** (jede Algo-Familie rahmt Heim-Workload zum Eigenvorteil — Rechtfertigung des Cross-Runs).
8. **Lokaler Test-Voll-Lauf** (n_ops=1000): **40.320/40.320** (320×6×21), 0 Fehlmessungen, alle `two_phase_valid=1`, Negativ-Query-Sweep verifiziert (Miss-Rate coco_neg0=0 → coco_neg100=500, monoton). Referenz `build/thesis_tiere/testfull_xml_40320_nops1000.csv` (ungetrackt).
9. **Finaler Voll-Lauf gestartet** (n_ops=10k, n_repeats=3, alle 21 Profile): 320×18×21 = **120.960 Messungen**. Reale Kosten ~3s/Messung → ~90-103h (nicht meine 50h-Schätzung). **VOM WINDOWS-UPDATE GEKILLT bei ~161/320 (~50%).**

## 2. AKTUELLER ZUSTAND (nach Reboot)
- `run_lazy_150.exe`: **tot** (0 Prozesse).
- **161/320 Lebewesen fertig** — je in `build/thesis_tiere/tiere/<stem>/result.csv` (378 Zeilen = 18 dyn × 21 Profile, 21 distinkte Profile/Datei). **Überlebten den Reboot** (lokal auf Disk). Erkennung: `result.csv` mit `LastWriteTime > 2026-06-08 13:32` (Lauf-Start). Die übrigen ~159 `result.csv` sind STALE (vom Test-Lauf 2026-06-08 / copymem-Rebuild) — NICHT verwechseln (Cutoff exakt am Lauf-Start prüfen!).
- Globale CSV `tier150_measurements.csv` = noch die alte 5761er (der Lauf schreibt die globale CSV erst am Ende → ging verloren; die 161 per-Binary-CSVs sind die Rettung).
- 320 copymem-v1-DLLs vorhanden + gültig (`build/thesis_tiere/tiere/<stem>/perm.dll`, `.version`=copymem-v1).

## 3. OFFENER PUNKT A — Undo-Log-Memento-Refactoring (#133, das „geplante Refactoring")
**Ziel:** Copy-Memento O(n)/Op → Undo-Log O(1)/Op, v.a. damit **read-only-Ops keinen O(n)-Vollkopie-Overhead** zahlen (die 21 Profile sind großteils read-heavy → größter Hebel). Erwartung: Voll-Lauf von ~90h auf ~10-20h (load-Phase O(records²) bleibt als Floor — s.u.).

**Mechanik heute (`abi_adapter.hpp:1163-1184` + `tier_observe_trace_abi.hpp:106` `two_phase_measure`):**
- `two_phase_measure(rb, lambda)`: `rb->tier_save_all()` → warmup-op → `rb->tier_rollback_all()` → measure-op.
- Copy-Memento: save = `saved_search_.emplace(search_organ_)` (O(n)), rollback = `search_organ_ = *saved_search_` (O(n)). Restauriert Daten UND Observer-Zähler (Vollkopie) → sauber, aber O(n) je Op AUCH bei read-only.

**Der schwierige Teil (counter-clean Undo-Log) — VOR Umsetzung klären/lösen:**
- Op-aware Undo am Interpreter-Level (`run_workload_profile`): read-only = kein Undo (O(1)); write = Einzel-Key-Inverse (insert→erase / erase→re-insert; O(1) Daten). ABER: Warmup-Op + Undo-Op erhöhen Observer-Zähler → **Verschmutzung der ~9 auto-gekoppelten Achsen** (T0 search lookup/hit/miss/insert, T6 allocator, T1/T2/T3/T7/T8/T10/T17/T18). Die 13 Scan-Achsen (T4/T5/T9/T11..T16) sind in `fill_observer_v3` idempotent (reset+scan je Observe) → NICHT verschmutzt.
- **Counter-clean braucht O(1)-Stat-Snapshot/Restore.** ERSTER SCHRITT FRISCHER KONTEXT: prüfen, ob `observable_composed_search.hpp` (+ die stateful Organe) ihre **Stats SEPARAT vom Daten-Substrat** halten (kleiner POD-Member). Falls JA → `tier_stats_save()`/`tier_stats_restore()` im Adapter (O(1) Snapshot/Restore nur der Stat-PODs) + op-aware Daten-Undo. Falls NEIN → entweder Stat-Restore je Organ ergänzen (invasiv, ~9 Organe) ODER pragmatisch: Copy-Memento NUR für write-Ops, read-only ohne save/rollback + dokumentierte 2×-Counter (Ratios+Timing bleiben korrekt) — **das ist die offene Sub-Entscheidung, mit User abstimmen** (er ist sehr genau bei Mess-Gültigkeit).
- **ABI-kritisch:** Adapter-Änderung → alle 320 DLLs neu bauen (neuer BuildVersion, z.B. `undolog-v1`). KEIN ABI-Major-Bump nötig, wenn nur die Memento-Implementierung (gleiche Schnittstelle/POD) ändert — verifizieren.
- **Wichtig (Kosten-Realität):** Die **Load-Phase** (records Inserts vor jeder Messung, ungemessen) ist für O(n)-Insert-Strukturen (k_ary/sorted/linear) **O(records²)** → ~1s/Messung Floor, vom Undo-Log NICHT adressiert. Überlegung für später: Load EINMAL je Binary + für read-only-Profile wiederverwenden (read-only mutiert nicht). Memory `project_two_phase_undolog_cost_blocker` hat die volle Analyse.

## 4. OFFENER PUNKT B — Mess-Wiedereinstieg / Resume (#139)
**Ziel (User):** „Wiedereinstieg bei einem bestimmten nicht-fertigen Tier mit vorigem Cache-Warmup". Beim Neustart sollen **fertige Lebewesen übersprungen** und nur die unfertigen (neu) gemessen werden; der Zwei-Phasen-Cache-Warmup gilt auf Re-Entry weiterhin (intrinsisch je Op).
**Einstiegsstelle:** `cache_engine_builder_iterator.hpp:299` `for (BuildResult const& b : builds)` (die Mess-Schleife je Binary). Plan:
1. VOR dem Messen je Binary prüfen: existiert `bin_dir/result.csv` und ist sie **vollständig + aktuell** für die laufende Config? Kriterium: Zeilenzahl == erwartete (dyn_settings × |workloads|) UND die Profil-ids/setting-Labels decken die aktuelle Workload-Menge. (Robust: einen kleinen Header-/Config-Stempel in die result.csv schreiben — z.B. erste Kommentarzeile mit build_version+n_ops+workload-set-hash — und beim Resume vergleichen.)
2. Wenn vollständig+aktuell → **Binary überspringen** (nicht neu messen), aber seine `result.csv`-Zeilen in die globale CSV/`result.csv_rows` AUFNEHMEN (einlesen + ingest), damit die globale CSV vollständig wird.
3. Wenn fehlend/unvollständig/veraltet → wie bisher messen (mit Zwei-Phasen-Warmup).
4. `LazyRunConfig` Flag `resume=true` (Default an?) + ggf. `expected_rows_per_binary`. Harness `-Resume`-Switch.
**Achtung:** Stale `result.csv` (vom Test-Lauf n_ops=1000) NICHT als „fertig" werten → der Config-Stempel (n_ops/records/workload-set) muss matchen, sonst neu messen. Das adressiert genau die aktuelle Verwechslungsgefahr (161 echt vs ~159 stale).

## 5. OFFENER PUNKT C — Voll-Lauf erneut + NAS + Auswertung
- **Re-Run** nach A+B: 320 × 18 × 21 = 120.960 Messungen, n_ops=10k/records=10k, BuildVersion = der nach dem Undo-Log neu gebaute (`undolog-v1`), `COMDARE_LOAD_PROFILE_DIR` = `libs/cache_engine/algorithm_profiles/load_profiles`, `COMDARE_WORKLOAD_RECORDS=10000`, n_repeats=3, **mit Resume**. User-Wahl: **lokal** (über Nacht/Wochenende).
- **NAS-Ablage (User-Direktive, REFERENCE):** Roh-CSV (~140 MB, zu groß für git) → `\\backup1.comdare.de\Cluster_NFS\experiment results` (UNC, nur bash/UNC — CLAUDE.md). **Verbindung INSTABIL** (hängt zeitweise) → IMMER robust: Retry-Schleife (mehrere Versuche + Pause + Ziel-Größen-Verifikation), benannte lokale Kopie bleibt IMMER als Fallback. In git nur Aggregat/Sample + PDFs.
- **Wichtig (Output-Pfad):** Der Repo liegt unter OneDrive → Sync kann Locks/Delays geben (früherer `perm.cpp`-C1083-Lock). Bei künftigem Lauf erwägen, den Output auf einen Nicht-OneDrive-Pfad (z.B. `C:\comdare_results`) zu legen. Für die Mess-Phase (keine DLL-Neubauten) bisher unkritisch.
- **Auswertungs-Pipeline `04_csv_to_latex`** (im Superprojekt-Ordner `…/Diplomarbeit - Datenbanken/04_csv_to_latex`): muss auf das neue **`Lebewesen × workload`-Schema** (175 Spalten inkl. `workload`-Achse + `negative_query_pct`) angepasst werden → Thesis-Tabellen/Plots. NOCH OFFEN (angeboten, nicht gemacht).

## 6. Lauf-Kommando (Referenz, Harness)
PowerShell (Sandbox aus, Hintergrund), `repo = …\Code\external\comdare-cache-engine`:
```
$env:COMDARE_LOAD_PROFILE_DIR = "$repo\libs\cache_engine\algorithm_profiles\load_profiles"
$env:COMDARE_WORKLOAD_RECORDS = "10000"
Remove-Item Env:COMDARE_WORKLOADS -EA SilentlyContinue   # XML-Discovery, nicht env-String
pwsh tests\unit\thesis_tiere\build_and_measure_150_tiere.ps1 -MaxBinaries 320 -BuildVersion <undolog-v1> -NOps 10000 -NRepeats 3 -RebuildHost
```
- `-RebuildHost` Pflicht nach jeder Host-relevanten Code-Änderung (run_lazy_150/iterator/perm_runner). Host-Exe = `%TEMP%\comdare_lazy150\run_lazy_150.exe`.
- DLL-Neubau erzwingt der NEUE BuildVersion (nach Undo-Log: alle 320 neu). Reuse via gleicher BuildVersion (Resume-Skip der Builds).
- Per-Binary-Output: `build/thesis_tiere/tiere/<stem>/result.csv`. Globale CSV: `build/thesis_tiere/tier150_measurements.csv` (am Ende). Harness kopiert auch → `tests/unit/thesis_tiere/tier150_measurements.csv`.

## 7. Schlüssel-Dateien
| Zweck | Datei |
|---|---|
| Adapter-Memento (Undo-Log-Ziel) | `libs/cache_engine/anatomy/abi_adapter.hpp:1163-1184` (save/rollback_all) |
| Zwei-Phasen-Treiber | `libs/cache_engine/builder/anatomy_commands/tier_observe_trace_abi.hpp:106` (`two_phase_measure`) |
| Such-Organ (Stat-Separierbarkeit prüfen) | `libs/cache_engine/axes/lookup/composable/observable_composed_search.hpp` |
| Interpreter (op-aware Undo) | `libs/cache_engine/builder/workload_driver/workload_orchestrator.hpp:64` (`run_workload_profile`) |
| Mess-Runner | `libs/cache_engine/builder/experiment_tree/perm_runner.hpp` (`run_workload_perm`, Load-Phase, Registry) |
| Mess-Loop (Resume-Einstieg) | `libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp:299-388` |
| XML-Parser | `libs/cache_engine/builder/workload_driver/load_profile_parser.hpp` |
| Lastprofile | `libs/cache_engine/algorithm_profiles/load_profiles/*.xml` (21) + `SCHEMA.md` |
| Host-Treiber | `tests/unit/thesis_tiere/run_lazy_150.cpp` (env COMDARE_LOAD_PROFILE_DIR) |
| Harness | `tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1` |
| Workload-Config + Generator | `…/workload_driver/workload_config.hpp` (negative_query_pct), `workload_generator.cpp` |
| Lastprofil-Katalog + Bias | `docs/architecture/32_lastprofil_katalog_und_paper_bias.md` |

## 8. Relevante Memories (diese Session geschrieben)
- `project_two_phase_undolog_cost_blocker` — Kostenanalyse + Undo-Log-Entscheidung + Counter-Sub-Frage.
- `feedback_two_phase_warmup_mandatory_validity` — Zwei-Phasen-Warmup = Gültigkeit, nicht Geschwindigkeit (NIE weglassen).
- `feedback_all_papers_loadprofiles_xml_all_axes` — alle Paper-Lastprofile als XML, alle über alle Achsen (Bias-Bruch).
- `project_workload_achse2_dynamic_bplus` — Workload = Achse 2.
- `project_biasmatrix_fullrun_and_nas` — Voll-Lauf-Zustand + NAS-Pfad + Robustheit.

## 9. EMPFOHLENE REIHENFOLGE (frischer Kontext)
1. **Resume (B) zuerst** — geringeres Risiko, löst direkt das Killed-Run-Problem; danach könnte man SOFORT den copymem-v1-Lauf resumen (161 da) und müsste nur ~159 Lebewesen nachmessen. (Pragmatischer Quick-Win, falls Undo-Log sich verzögert.)
2. **Undo-Log (A)** — erst Stat-Separierbarkeit in `observable_composed_search.hpp` prüfen → Design wählen (counter-clean vs Sub-Entscheidung mit User) → implementieren → 320 DLLs `undolog-v1` neu → SmallPilot verifizieren (Op-Mix+Counter sauber, Negativ-Sweep, two_phase_valid, **Kosten messen** vs copymem).
3. **Re-Run (C)** mit Undo-Log + Resume, lokal, robuste NAS-Ablage. Dann `04_csv_to_latex` auf neues Schema.
4. Inkrementell committen + Superprojekt-Bumpen + pushen (3 Repos synchron, Memory `feedback_submodule_sync_3repos`).

## 10. Offene Backlog/Scope (nicht für diese Re-Run-Welle)
- Doc-32-Scope: String-Corpora/LP07 + lognormal out-of-scope (uint64-Modell); value_length-Achse; threads falten in concurrency-Dim (KEINE eigene Workload-Variante).
- #122 P3 io-Fixture, #123 P4 migration 2-Ebenen, #124 P5 filter train-then-probe, #125 P6 lazy DLL content-versioning — separat, niedrige Prio.
- Load-once-reuse für read-only-Profile (Load-Phase-Floor senken) — Optimierungsidee.
