# Session-Übergabe — Goal-V6 L-74c Cross-ABI-Abschluss + Cluster-Fahrplan (2026-06-03)

> Erster Pre-Read der Folge-Session (mit `goal-v6-luecken-ledger.md` + `docs/architecture/29_*.md`).
> Hält den verifizierten L-74c-Abschluss, das wiederverwendbare Muster und den präzisen nächsten
> Schritt je verbleibendem Cluster fest. Single-Source bleibt das Ledger §2.5.

## 0 — Aktiver Auftrag (unverändert)
Goal V6 (Stop-Hook): Permutations-B+-Experiment-Baum auf absolute, literal-belegte Vollständigkeit gegen alle 22 Achsen + 5 Lebewesen-Gattungen. **Phase D (Umsetzung) läuft.** Goal V6 ist — wie die Planungssession (#79) es anlegte — ein **Mehr-Cluster-/Mehr-Session-Programm**; kein einzelner Kontext schließt es. Disziplin: pro Zyklus den nächsten Cluster **real, abgegrenzt, literal verifiziert**; NIE Erfolgsmarke ohne literale Ausgabe.

## 1 — DIESER ZYKLUS ERLEDIGT: L-74c Composition-Driver + Cross-ABI (20 verifizierte Commits)
Von 1/4 OperativeCapable-Achsen bis zum realen DLL-Round-Trip — jeder Schritt mit literaler Ausgabe, alle regress-verifiziert:
1. **Member-Hold-Sackgasse revertiert** (Aggregat-`{}`-Init-Artefakt, nicht CRTP-Block).
2. **4 OperativeCapable-Achsen voll** (telemetry/memory_layout/serialization/node_type) als `ObservableXxx<Strategy>`-Hüllen in `observe_all()` — `test_v41_anatomy_observer` 5/5 Säule-2-Tests `[ OK ]`, alle 11 Reference-Compositions umgestellt.
3. **Cross-ABI:** `ComdareTierObserverSnapshotV2` + `fill_tier_observer_v2`-Brücke; telemetry-Auto-Kopplung in `tier_insert/lookup`; **`IObservableTierV2`** (eigenständiges Sub-Interface, ABI-robust); **realer DLL-Round-Trip 4/4 grün** (`tier_observe_v2` über die `.dll`-Grenze).
4. **CMake-Fix:** `RUNTIME/LIBRARY_OUTPUT_DIRECTORY_DEBUG` für codegen-DLLs (Multi-Config-Bug).

**Wiederverwendbares Muster (Doc 29 §3c–§3f) — gilt für ALLE künftigen Achsen-Observer:**
- Pro OperativeCapable-Achse: `ObservableXxx<Strategy>`-Hülle = Mess-Mechanik + gegated `statistics()`/`snapshot_t`, transparenter Decorator (`name()`/`topic_tag`/Strategie-Methoden durchreichen). **Pflicht-Probe ZUERST mit dem REAL verwendeten Wrapper** (Lehre aus der Sackgasse — `ArtComposition::<achse>` ermitteln).
- static-genutzte Achsen (memory_layout `L`, node_type `N` in `ComposedStore<N,L,A>`): Hülle bietet **static Pass-Through + Instanz `observe_*`** + reicht den Concept (`topic_tag`/`max_capacity`/`cache_line_size`) durch.
- Cross-ABI: **neuer POD-Typ V2** (nie append an V1) + **eigenständiges Sub-Interface** (nie vtable-Append → sonst SEH-Crash über DLL) + codegen-DLL braucht `OUTPUT_DIRECTORY_DEBUG`.

## 2 — VERBLEIBENDE CLUSTER (präziser nächster Schritt je Cluster)

### L-74c-Rest (klein, 2 Punkte)
- **telemetry-über-DLL = 0:** die AdHoc/Permutations-DLL trägt die NACKTE Registry-Strategie (`axis_11_telemetry_registry.hpp:18-23` `AllTelemetries = mp_list<LeafOnlyCounter,…>`), nicht die Hülle → `if constexpr(requires record_node_touch)` greift nicht. **Schritt:** die telemetry-Enabled-Liste (und analog memory_layout/serialization/node_type) auf `ObservableXxx<…>`-Hüllen umstellen, damit `TopicConfigSet::StaticAxisVariants` (BR-1 `registry_to_axis_levels`) die Hüllen liefert. **Breit** (berührt BR-1/BR-2/BR-3 + Baum + DLL-Rebuild) → eigener Sprint, je Achse messen.
- **scan-Achsen-Auto-Kopplung (Pfad A):** memory_layout/serialization/node_type sind scan-basiert (Daten-Buffer), passen zu `abi_adapter.hpp:run_workload` (lbuf), nicht zu `tier_insert` (key/value). **Schritt:** in `run_workload` die `MemLayout::scan_field_sum`/`Serializer::serialize_scan` (Z.215/218) durch gehaltene Organ-`observe_*` ersetzen + in `fill_observer_v2` sammeln.

### L-74a / L-74b — page_type(01)/simd(09b)/hw(12) als echte Build-Varianten
- IST (Ledger L-74a): nur Stub-Inspektionssymbol; `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT` + `hw_cross_constraint.hpp` + reale `comdare_build_*`-Symbole FEHLEN. Diese 3 Achsen stehen AUSSERHALB der 17-Slot-Komposition (Gattungs-Invariante) → sie sind Build-Varianten DERSELBEN Binary, kein Observer-Slot. **Schritt:** Build-Variant-Makro + je-Knoten-getragene Definition (L-74b) nach `build_variant_definition.hpp`-Vorbild.

### L-76a/b/c/d — per-Gattung Docks + PermutationEngines (Set/Sequence/View)
- IST: Gattungen GEBUNDEN (GenusBindingTraits, #76 done), aber **Docks + PermutationEngines fehlen**. Vorbild = `SearchAlgorithmDock` (`builder/pruef_dock/`) + Container-Dock (#75). Pre-Sprint-Klärungen K-A/B/C im Ledger §127-129 bereits aufgelöst (Set=15, Sequence=10+growth, View=7). **Schritt je Gattung:** `<genus>_dock.hpp` (analog SearchAlgorithmDock, `dock_genus()==<Genus>`) + PermutationEngine; DLL-Round-Trip analog R8RestA.

### L-CLUSTER — perm_runner-CLI + Webhook (~1/4, nur result_ingest existiert)
- **Schritt:** `perm_runner`-CLI-App (kein Python — CMake+C++), die den Baum-Selektions→Build→Measure-Pfad als Binary fährt + Webhook-POST. GATE-MAXIMAL (ZIH sbatch/apptainer) bleibt USER-ABSPRACHE.

### L-MEAS — PMC (Per-Achsen jenseits Wall-Clock)
- Wall-Clock sub-noise für die Deskriptor-Achsen → PMC nötig (R5.D). Ehrlich „operativ-PMC-pending". Plattform-spezifisch, niedrigste Prio.

## 3 — Build-/Verifikations-Verfahren (bewährt)
- Umbrella via CMake-Target + RAM-Watchdog (`< 2.5 GB` → cl/cmake/link killen). Muster in jedem Build-Block dieser Session.
- Leichte Einzeltests: `build/scratch_compile_test.ps1 -Test <name> -Boost -Extra @("build\generated","libs\cache_engine\src")`. STATISTICS/MEASUREMENT via `#define` in der Quelle.
- DLL-Round-Trip: CMake-Target `comdare_r5g_adhoc_perm` + `test_v41_anatomy_adhoc_dll_load`; bei DLL-Rebuild-Problemen obj-dir + DLL gezielt löschen, dann bauen (DLL landet jetzt dank `OUTPUT_DIRECTORY_DEBUG` direkt im Lade-Ort).
- **3 Repos:** nach jedem ce-Push DA-Submodul-Pointer bumpen (committet+gepusht beide Remotes); prt-art unberührt.

## 4 — Standing Constraints (verbatim, gelten weiter)
Deutsch + Diakritika; NIE Erfolgsmarke/„done" ohne literale Ausgabe; KEINE declare-victory-by-reclassification; keine Quick-Fixes; Doku nie löschen (nur git mv); destruktive Ops in den 3 Thesis-Repos OHNE Rückfrage NUR mit Tag+Commit+Push, Remotes NIE löschen; Submodul-Sync nach jedem ce-Push; Keys nie committen; KEIN Python in der Buildchain; Compile-Time-Only/kein Runtime-Switch im Hot-Path; FÜR SUCHE IMMER BÄUME; RAM-Watchdog bei jedem Compile; bei Unklarheit Planrunde; autonom arbeiten; GATE-MAXIMAL ZIH IMMER mit User absprechen.
