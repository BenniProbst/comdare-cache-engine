# 33 — Undo-Log-Memento (O(1)/Op) + Mess-Resume (2026-06-11)

> **Kontext:** Der Zwei-Phasen-Cache-Warmup (`save_all → op-warmup → rollback_all → op-measure`, je Op;
> messarchitektur_v5_design §4, PFLICHT für Mess-Gültigkeit) lief bis copymem-v1 über eine **Organ-Vollkopie
> O(n)/Op** (`abi_adapter.hpp` tier_save_all/tier_rollback_all). Bei records=10k zahlte damit JEDE Op — auch
> read-only — zwei O(n)-Kopien; der 320×18×21-Voll-Lauf brauchte ~4 s/Messung (~90-103 h) und wurde am
> 2026-06-10 von einem Windows-Update-Reboot bei ~50 % gekillt. Dieses Dokument beschreibt die zwei daraus
> abgeleiteten, vom User beauftragten Mechaniken: **(A) Undo-Log-Memento** (User-Entscheidung 2026-06-08
> „Beides: Kopie jetzt, Undo-Log später"; ausgeführt 2026-06-11) und **(B) Mess-Resume** (User 2026-06-10
> „Wiedereinstieg bei einem bestimmten nicht fertigen Tier mit vorigem Cache-Warmup").

## §1 Undo-Log-Memento (#133) — Design

**Unverändert:** das ABI (`IRollbackableTier::tier_save_all()/tier_rollback_all()`, vtable/POD identisch →
KEIN ABI-Major-Bump), das Host-Protokoll (`two_phase_measure`, tier_observe_trace_abi.hpp) und der Vertrag
(„End-Zustand + Observer-Zähler identisch zur Einphasen-Messung"). Es ändert sich NUR die Implementierung im
Adapter → alle Tier-DLLs werden mit neuer BuildVersion (`undolog-v1`) neu gebaut.

**Mechanik (abi_adapter.hpp):**
1. `tier_save_all()` = **O(1)**: Stat-POD-Snapshots von `search_organ_` (T0) und `container_` (Allocator-
   Messpfad) ziehen (`statistics()`), Undo-Log leeren, Recording armen (`undo_armed_`). Die Daten werden
   NICHT mehr kopiert.
2. `tier_insert`/`tier_erase` zeichnen — NUR bei `undo_armed_` (Warmup-Phase) — **vor** der Mutation die
   Einzel-Key-Inverse auf: `{key, old_value, had_old}` via direktem `search_organ_.lookup(key)` (berührt nur
   den T0-Stat-POD, der beim Rollback restauriert wird; die auto-gekoppelten Mess-Organe sehen den internen
   Lookup nicht). `tier_lookup`/`tier_scan` mutieren weder search noch container → kein Log-Eintrag.
3. `tier_rollback_all()` = **O(#writes der Periode)** (im Protokoll 0–2): das Log RÜCKWÄRTS über die
   **Organ-API** abspielen (`had_old → insert(k, old)` sonst `erase(k)`, auf search_organ_ UND container_ —
   die Delegationskette node/layout/allocator bleibt gewahrt, Doc 30), danach beide Stat-PODs restaurieren
   (`restore_statistics`, neue O(1)-Methode der Hüllen `ObservableComposedSearch`/`ObservableComposedContainer`).
   Danach `undo_armed_ = false` → die **Mess-Op läuft log-frei** (einziger Mess-Pfad-Overhead: ein
   vorhersagbarer bool-Branch je tier_insert/tier_erase).
4. **Eskalation:** `tier_clear` in der Warmup-Phase ist nicht O(1)-invertierbar → für DIESE save-Periode wird
   auf die Vollkopie eskaliert (`escalate_undo_to_copy_`: erst Log-Replay → Organe auf save-Stand, dann
   Kopie). Ebenso bei Log-OOM. Der Rollback einer eskalierten Periode = Vollkopie-Restore + Stat-Restore.
5. **Fallback-Kaskade (requires-detektiert, `undo_log_capable_`):** Organe ohne map-API/`restore_statistics`
   oder ohne Copy-Fähigkeit → unverändert der bisherige Copy-/MementoAxis-Pfad. Diagnose compile-time:
   `tier_memento_is_undo_log()`.
6. **Idempotenz** (IRollbackableTier-Vertrag): nach dem Replay ist das Log leer; ein erneuter
   `tier_rollback_all` wiederholt nur den Stat-Restore (bzw. den Vollkopie-Restore) → derselbe Stand.

**T6-Allocator-Exaktheit ohne Snapshot:** Die Allocator-Statistik des Mess-Stores (`LayoutAwareChunkedStore::
allocator_statistics()`) ist vollständig aus dem Datenzustand DERIVIERT (`chunk_allocs_`≡ceil(size/cap) nach
jedem Rebuild, `size_`) → nach korrektem Daten-Undo automatisch exakt (literal belegt: T6 elementweise
identisch im Einphasen-Vergleich, test_undolog_memento).

## §2 Counter-Abdeckung (Klarstellung, identisch zu copymem-v1)

Das Memento deckte SCHON IMMER genau `search_organ_` + `container_` ab (T0 + T6/Allocator-Pfad). Die
auto-gekoppelten Instanz-Achsen (T1/T2/T3/T7/T8/T10/T17/T18) sehen Warmup- UND Mess-Op (systematisch exakt
2× je Op, deterministisch identisch über alle Tiere/Profile — Ratios/Vergleiche unverzerrt); die Scan-Achsen
(T4/T5/T9/T11..T16) sind idempotent (reset+scan je Observe). Der Undo-Log repliziert diese Abdeckung EXAKT —
die CSV-Semantik aller 19 stat_*-Blöcke ist mit copymem-v1 identisch (Determinismus-Beweis: stat_*-Spalten-
Diff copymem-v1 ↔ undolog-v1 bei gleichen Seeds, §4).

## §3 Verfahrens-Nuance (dokumentierte Semantik)

Nach dem inversen Replay ist der Zustand **logisch exakt** der save-Stand (gleiche key→value-Map, gleiche
Zähler), das **physische Substrat** bleibt „warm": z.B. schrumpfen Knoten-Splits oder gewachsene Kapazitäten
des Warmup-Inserts nicht zurück (das Copy-Memento via copy-assign behielt die Ziel-Kapazitäten ebenfalls).
Die Mess-Op läuft damit auf logisch identischem, physisch warm-strukturiertem Zustand — konsistent über alle
Tiere/Profile und im Sinne des Cache-Warmup-Zwecks (die Op wurde unmittelbar zuvor einmal gespielt). Diese
Nuance ist inhärent im vom User gewählten Verfahren (Einzel-Key-Inverse, 2026-06-08) und gilt gleichermaßen
für jede Komposition.

## §4 Verifikation (literal)

1. **test_undolog_memento.cpp** (standalone, Art/Hot/Masstree): 42/42 OK — Undo-Pfad compile-time aktiv;
   Daten-Exaktheit (insert-neu/-update, erase, lookup, tier_clear-Eskalation); Idempotenz; Mess-Fortschritt
   (Periode-Disziplin); **Counter-Clean**: Einphasen-Lauf == Zwei-Phasen-Lauf in tier_size + T0/T6
   elementweise; `rollback_is_empirically_exact == true`.
2. **Bestandstests:** test_v5_two_phase_driver (3/3), test_v5_organ_memento (2/2) grün.
3. **E2E/Kosten:** 2 FullPilot-Tiere (Index 0/1, k_ary) als undolog-v1-DLLs neu gebaut + mit der
   Voll-Lauf-Konfiguration gemessen (n_ops=10k, records=10k, n_repeats=3, 21 XML-Profile = 378 Messungen je
   Tier); Determinismus-Diff der stat_*-Spalten gegen die gesicherten copymem-v1-result.csv derselben Tiere
   (`result.copymem-v1.csv`); Kosten copymem ~25 min/Tier als Referenz (Zeitstempel 2026-06-08 14:00→14:25).
   → Ergebnisse in der Session-Doku.

## §5 Mess-Resume (#139) — Design

**Granularität = Tier-Binary** (User: „Wiedereinstieg bei einem bestimmten nicht fertigen Tier"). Der
Zwei-Phasen-Cache-Warmup gilt auf Re-Entry intrinsisch je Op (er ist Teil jeder Messung, nicht des Laufs).

**Mechanik (cache_engine_builder_iterator.hpp + run_lazy_150.cpp + Harness):**
1. **Config-Stempel** `result.csv.stamp` neben der per-Binary `result.csv`, geschrieben NUR wenn die Binary
   VOLLSTÄNDIG gemessen wurde (jede besuchte dyn-Einstellung lieferte eine Zeile). Inhalt:
   `resume-v1|build=<BuildVersion>|n_ops|seed|records|dims=<JEDE dynamische Dimension mit voller
   Werte-Liste>|rows=<N>` — deckt n_repeats (repetition-Dim), das Workload-Set (XML-Lastprofil-ids) und alle
   Resource-Control-Dims ab. **BuildVersion im Stempel ⇒ copymem-v1-Ergebnisse werden in einem
   undolog-v1-Lauf NIE als fertig gewertet** (genau die Stale-Falle des gekillten Laufs: ~159 Test-Lauf-CSVs
   mit n_ops=1000).
2. **Skip-Check** je Binary VOR dem Laden (`lazy_try_resume_binary`): result.csv + Stamp existieren, Stamp ==
   aktueller Config-Stempel, **Header == lazy_csv_header()** (Schema-Drift-Schutz), Datenzeilen-Zahl ==
   rows-Angabe. Alles erfüllt → Binary komplett übersprungen (kein DLL-Load, keine Messung), die Zeilen
   fließen UNVERÄNDERT in die globale CSV (`LazyRunResult::resumed_csv_rows`, vor den frischen Zeilen).
   Jede Abweichung → Neu-Messung (keine stillen Teil-Übernahmen). Unvollständige Binaries (Reboot mitten im
   Tier) haben keinen/inkonsistenten Stamp → komplett neu.
3. **Steuerung:** `LazyRunConfig::resume_completed_binaries` (Default AN), CLI argv[11] (`1|0`), Harness
   `-Resume` (Default `$true`; `-Resume:$false` = alles neu messen).
4. **Bewusste Grenze:** resumierte Zeilen werden NICHT erneut in den Experiment-B+-Baum ge-ingestet (die
   per-Binary-CSV-Zeile ist das WIDE-Format, nicht das Wire-Format von `ingest_result_line`); die Auswertung
   (04_csv_to_latex, Aggregat) läuft über die globale CSV. Der Baum trägt die in DIESEM Lauf frisch
   gemessenen Knoten.

## §6 Betroffene Dateien

| Zweck | Datei |
|---|---|
| Undo-Log-Memento (save/rollback/Hooks/Helfer) | `libs/cache_engine/anatomy/abi_adapter.hpp` |
| O(1)-Stat-Restore der Hüllen | `axes/lookup/composable/observable_composed_search.hpp` + `observable_composed_container.hpp` |
| Semantik-Test | `tests/unit/test_undolog_memento.cpp` (standalone, ADHOC-Include-Satz) |
| Resume (Stamp + Skip + resumed_csv_rows) | `libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp` |
| Host (argv[11], globale CSV inkl. resumiert) | `tests/unit/thesis_tiere/run_lazy_150.cpp` |
| Harness (`-Resume`) | `tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1` |
