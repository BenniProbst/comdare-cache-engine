# End-zu-Ende-Abnahme-Audit (3 Repos) + User-Entscheidungen

**Datum:** 2026-05-31 · **Audit-Workflow:** `w9iy2dhrc` (8 Leser exhaustiv über alle Architektur-Docs + Code-E2E-Verifikation + Synthese) · **Verdikt:** BEDINGTE ABNAHME (Modell korrekt, 2 Code-Blocker + 1 wissenschaftlicher Vorbehalt)

## 1. 3-Repo-Modell — CODE-VERIFIZIERT KORREKT

| Repo | Rolle (IST-belegt) |
|------|--------------------|
| **cache-engine** | Achsen-Bibliothek (15 Topics / 17 Achsen, `axes/`-Rename physisch) + Anatomie-Generator (`AdHocComposition<T0..T16>` → PermutationEngine → `adhoc_emitter` → je Permutation eine SHARED-DLL, 48 real gebaut) + EINHEITLICHES Prüf-Dock (`SearchAlgorithmDock`: `dynamic_cast<IObservableTier*>` über reale DLL-Grenze; Hybrid Pfad A `run_workload`+`f15_compare` / Pfad B `IObservableTier`-POD). Konsumiert prt-art als Prüfling via `COMDARE_CE_PRUEFLINGE`. |
| **prt-art** | PRÜFLING der Gattung SearchAlgorithm — reines Plugin (kein nested Submodul), `comdare_pruefling.cmake` + `register_prt_art_pruefling`; 4/17 Achsen-Slots gefüllt; 3-Stufen-Join compile-time (`pruefling_merge` static_assert). |
| **Diplomarbeit** | 6-Stufen-LaTeX-Pipeline `01 sample → 02 messung_driver → 03 binary→csv → 04 csv→latex → 05 diagram → 06 latex→pdf`; konsumiert CE korrekt (lädt Permutations-DLLs + nutzt CE-`welch_t_test`). |

**Richtungsregel** prt-art-konsumiert-CE im Code erfüllt. **Datenfluss strukturell vorhanden, aber NICHT geschlossen.**

## 2. Befunde (16 Gaps, code-verifiziert)

**BLOCKER (reiner Code-Fix, nicht extern-gated):**
- **G1 CSV-Schema-Bruch:** Stufe 03/04 = 15 Spalten (kein `workload_used`), Stufe 01/05/ResultAggregator = 16 Spalten (`workload_used` an Index 3). Stufe 04 wirft `std::stoull('YCSB_A')` auf 16-Spalten-Input. Kein CSV fliesst sauber durch 03→04→05.
- **G2 keine E2E-Orchestrierung:** kein Target verkettet die 6 CLIs; kein Cross-Stage-Test → der Schema-Bruch blieb unentdeckt.

**WISSENSCHAFTLICHER VORBEHALT:**
- **G3 Umstufung-B im Mess-Pfad nicht vollzogen:** `axis_03a::EnabledStrategies` + `adhoc_emitter` (SA0..SA11) fahren weiterhin 17 monolithische Tiere (Array256..BTree) als search_algo-Achsenwert, KEINE sezierten Organe — entgegen der Direktive „Achsen = NUR Organe". #42 ist „completed" markiert, im Mess-Pfad aber NICHT umgesetzt → F15-Headline-Resultate basieren auf dem Anti-Pattern.

**EXTERN/TERMIN/USER-GATED:**
- G4 reale HW-Messreihe (V21.2, nur Sample-Daten) · G5 prt-art `run()` = `std::unordered_map`-Platzhalter (TODO E6) · G6 PMU-Felder hart 0 / `total_cycles` approximiert · G7 2 real / 5 Vendor-Allokatoren std-Fallback · G8 zwei parallele Plugin-ABIs (alt `comdare_perm_descriptor` von Stufe 02 / neu `comdare_anatomy_perm_*` CE-intern) · G9 `observe_all` real ~2/17 Achsen · G10 `op_type_filter` fehlt · G11 WorkloadKind `Custom_BulkInsert` fehlt · G12 **massive Doku-Drift** (Docs 00–09, Y/Z, prt-art-5-Doks, REV7.7, Doku 23/23a vom Code überholt; nur /goal-V2-Ledger IST-treu) · G13 alte PrtArtSearchEngine koexistiert ungenutzt · G14 Slot-Abdeckung 4/~10 · G15 K-H/K-I-Doku · G16 F.6-Phase-C deprecated (termin-gated).

## 3. User-Entscheidungen (2026-05-31, AskUserQuestion)

| # | Frage | Entscheidung |
|---|-------|--------------|
| 1 | Pipeline-Blocker (CSV + Orchestrierung) | **16-Spalten kanonisch + Orchestrierung** — Stufe 03 + Binary-Record/Writer um `workload_used` erweitern, Stufe 04 auf 16; `add_custom_target` verkettet 6 CLIs + Cross-Stage-Test |
| 2 | Umstufung-B im Mess-Pfad | **Vollenden + F15 neu erheben** — `EnabledStrategies`+`adhoc_emitter` auf Organe, Monolithen deregistrieren, F15 neu messen |
| 3 | Mess-Daten | **i7-1270P lokal als Mindestmessung** — echte lokale Mess-Reihe vor Abnahme |
| 4 | Gated-Punkte | **Vendor-Allokatoren/PMC vorher beschaffen** — jemalloc/tcmalloc echt linken + PMC-Counter vor Abnahme (Beschaffung/Zugang nötig) |

**D1/D2-Volltext** bleibt strikt user-manuell (Thesis-Autorenschaft), sofern nicht anders angewiesen.

## 4. Abgeleitetes Arbeitsprogramm (Basis für neues /goal)

1. **P1 Pipeline-Schließung** (Diplomarbeit, Code): CSV 16-col kanonisch (Record+Writer+Stufe 03/04) + E2E-Orchestrierungs-Target + Cross-Stage-Test → geschlossener Lauf an EINEM Datensatz.
2. **P2 Mess-Pfad-Korrektur** (cache-engine, Code): Umstufung-B vollenden (Organe statt Monolithen in EnabledStrategies+adhoc_emitter) → F15 neu erheben.
3. **P3 Reale Messung** (i7-1270P): Mindest-Messreihe durchfahren → Pipeline → LaTeX/PDF mit echten Zahlen.
4. **P4 Vendor/PMC** (extern-beschaffung): jemalloc/tcmalloc echt linken + PMC-Counter — Beschaffungs-Anforderung an User.
5. **P5 Konsistenz** (Code+Doku): zwei-Plugin-ABI-Konsolidierung, prt-art `run()` (E6)+Slot-Abdeckung, `op_type_filter`+`Custom_BulkInsert`, Doku-Drift (SUPERSEDED-Banner, niemals löschen).

**Abgegrenzt (kein Agenten-Scope):** D1/D2-Volltext (User), F.6-Phase-C-Löschung (nach Habich-Termin~9), Cluster C1/C2 (extern/Termin).

## 5. Fortschritt (Goal V3)

### P1 — Pipeline-Schließung
- **P1-A CSV-16-Spalten-Schema ✅ done-verified** (Diplomarbeit `d19c2de`): Container-v2 (`measurement_writer.hpp` + `workload_used` längen-präfixiert), Stufe 02 `workload_used="micro"`, Stufe 03 liest v1/v2 + emittiert 16-col, Stufe 04 parst 16-col (cols[3]=workload_used, Rest +1). Bestehende Tests + `test_pipeline_cross_stage` (Gap G2) nachgezogen. **Verifiziert: standalone MSVC 03→04 ALLE CHECKS PASSED** (16-col, Stufe 04 kein Wurf, workload+op_count an korrekt verschobenen Spalten).
- **P1-B Orchestrierungs-Target ✅ angelegt** (Diplomarbeit `4794b94`): `comdare_pipeline_e2e` (`add_custom_target`) treibt 01 Sample-CSV → 04 LaTeX + 05 TikZ (Sample-Pfad, DLL-frei).
- **P1-RUN (voller E2E-Lauf → thesis/main.pdf) ⏳ GE-GATET auf Superprojekt-Build.** `pdflatex` (MiKTeX) lokal vorhanden ✓.
  - **GATEWAY-BLOCKER diagnostiziert (P5-Cross-Repo-Build-Bug):** `cache-engine/cmake/anatomy_codegen.cmake` (+ `adhoc_emitter.cmake`, `is_original_codegen.cmake`, evtl. boost/gtest/compiler_cache) nutzen `${CMAKE_SOURCE_DIR}/libs/cache_engine/...` — im Superprojekt-Kontext zeigt das auf die SUPERPROJEKT-Wurzel (`Code/`), nicht die cache-engine-Wurzel → `Template not found`-FATAL_ERROR beim Configure. Standalone gebaut funktioniert es.
  - **Fix-Muster (etabliert, schon in `permutations.cmake:24`):** `set(_ce_root "${CMAKE_CURRENT_LIST_DIR}/..")` + `${CMAKE_SOURCE_DIR}/libs/cache_engine` → `${_ce_root}/libs/cache_engine`. ~22 Vorkommen über 9 cmake-Dateien klassifizieren (echte Pfad-Bugs vs. legitime Container-Caches), fixen, dann voller Superprojekt-Configure+Build → `comdare_pipeline_e2e` → PDF.

### P1-Gateway + E2E-Lauf ✅ (2026-05-31)
- **Gateway-Build-Fix done-verified** (cache-engine `cec3a85`): `CMAKE_SOURCE_DIR`→`CMAKE_CURRENT_FUNCTION_LIST_DIR/..` (anatomy_codegen+adhoc_emitter) bzw. `CMAKE_CURRENT_LIST_DIR`-relativ (boost/gtest/compiler/tools). **Superprojekt-Configure EXIT 0 + Build EXIT 0.**
- **Pipeline-Tests grün (CMake-Build):** `test_pipeline_cross_stage` 1/1 (03→04→**05**-Kohärenz), binary_to_csv 5/5, csv_to_latex 5/5, diagram_generator 6/6.
- **E2E-Orchestrierung läuft real:** `comdare_pipeline_e2e` → 01 `measurements.csv` (10 Perm., 16-col) → 04 „10 rows → 04_table.tex" (kein Wurf!) → 05 „10 rows by workload → 05_diagram.tex".
- **PDF baut:** `pdflatex thesis/main.tex` → `main.pdf` (29 Seiten, 511 KB, EXIT 0).
- **OFFEN für volle P1-Schließung:** Thesis bindet die generierten Pipeline-Artefakte (04_table/05_diagram) noch NICHT ein („No anhang/*.tex") → anhang-Verdrahtung (Pipeline-Output → `thesis/anhang/` → PDF inkludiert sie) + build_thesis-Pfad-Fix (`..\..\thesis`).

### Nächster Schritt: anhang-Verdrahtung (P1-Schließung: Pipeline-Tabelle/Diagramm IM PDF) → dann P2 (Umstufung-B Mess-Pfad).
