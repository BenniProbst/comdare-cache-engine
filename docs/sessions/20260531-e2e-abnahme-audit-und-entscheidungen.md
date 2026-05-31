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

### P1 ✅ done-verified (2026-05-31, DA `362a5cc`)
- `comdare_pipeline_e2e` erzeugt über CMake-Build einen GESCHLOSSENEN Lauf: 01 `measurements.csv` (10 Perm., 16-col) → 04 `04_table.tex` (10 rows, kein Wurf) → 05 `05_diagram.tex` (10 rows by workload) → 06 **`pipeline_demo.pdf` (2 Seiten)**. PDF enthält die generierte Tabelle + TikZ-Diagramm.
- D1/D2-Grenze respektiert: `pipeline_demo.tex` ist Build-Artefakt; die Integration in `thesis/main.pdf` (Auskommentieren der `\input{tabellen/…}`/`\input{tikz/…}` in `chapters/06_auswertung.tex`) bleibt user-manueller D1-Schritt.
- **Verbleibend P3:** die synthetischen 10-Perm-Sample-Daten durch reale i7-1270P-Messung ersetzen (nach P2).

### P2 — Mess-Pfad-Korrektur (Umstufung-B, cache-engine) — IN ARBEIT (Investigation)
Audit-Gap G3: `axis_03a::EnabledStrategies`/`AllStrategies` + `adhoc_emitter` SA0..SA11 fahren weiterhin 17 monolithische Tiere (Array256..BTree) als search_algo-Achsenwert statt sezierter Organe.

**IST verifiziert** (`axis_03a_search_algo_registry.hpp:45-77`): `AllStrategies = mp_list<Array256SearchAlgo, …17 Monolith-Wrapper… BTreeSearchAlgo>`; `EnabledStrategies = mp_filter<is_enabled, AllStrategies>`. Diese 17 Wrapper werden BREIT referenziert (perm_engine, named Compositions, adhoc_emitter SA0..SA11, equivalence-Tests).

**Offene Architektur-Frage (vor dem Eingriff zu klären — Planrunde):** Umstufung-B Phase 1 (#42, `e31541a`) hat die named CE-Compositions auf sezierte Organe via `ObservableComposedContainer` umgestellt, ABER die Registry/adhoc_emitter NICHT. Korrekter Ziel-Zustand zu bestimmen:
- (a) `adhoc_emitter` variiert search_algo über die ORGAN-Compositions (named Observable*-Compositions) statt SA0..SA11-Monolithen → Mess-Permutationsraum direktiven-konform; ODER
- (b) `AllStrategies` selbst von Monolithen auf Organ-Compositions umstellen (breiter Eingriff, viele Konsumenten).
**Risiko:** Monolithen sind breit referenziert; der gerade reparierte Superprojekt-Build darf nicht brechen. Vorgehen: erst `ObservableComposedContainer`/named-Compositions + alle `AllStrategies`-Konsumenten kartieren, dann inkrementell + test-gedeckt umstellen, perm_engine/topic_traversal/cross_variant nach jedem Schritt grün halten, Rollback-Tag vor dem Eingriff.

### P2 — PLANRUNDEN-BEFUND (2026-05-31, naiver Ansatz widerlegt, Build grün gehalten)
**Versuch (Tag `v41-p2-pre-adhoc-organ-migration`):** `adhoc_emitter` SA0..SA11 von Monolithen auf die 6 Observable-Organe (`ObservableArtTrieOrgan`…`ObservableMasstreeOrgan`) umgestellt. Emitter kompiliert + emittiert organ-basierte Module (Manifest = 24× `ObservableComposedContainer<…>`, 0 Monolithen).
**ABER beim DLL-Build hart widerlegt:** `SearchAlgorithmAnatomy<AdHocComposition<Organ, AdHoc-DEFAULT-Achsen>>` ist **ill-formed** → `SearchAlgorithmAbiAdapter`-CTAD scheitert (C2780/C2514). Betrifft NICHT nur Wormhole/Masstree, sondern AUCH ART/HOT/START/SuRF (auto_3 ART scheitert). **Ursache:** die sezierten Organe benötigen ihre KOMPATIBLEN Begleit-Achsen (node_type/path_compression/… wie in ihren named Compositions, z.B. `art_reference.hpp`), NICHT die AdHoc-Defaults (Node256Layout/PathCompressionNone/…). Der naive „nur search_algo tauschen, Rest Default" funktioniert architektonisch nicht. **→ Emitter zurückgerollt (`git checkout`), 48-Monolith-Build wieder grün.**

**Korrekter Lösungsweg (für die test-gedeckte Ausführung):** Der Mess-Permutationsraum muss über die NAMED Organ-Compositions (ArtComposition/HotComposition/StartComposition/SurfComposition/Wormhole/Masstree — jede mit organ-kompatiblen Achsen) variieren, NICHT über `AdHoc<organ, defaults>`. D.h. der `adhoc_emitter` (oder ein neuer `composition_emitter`) iteriert die named Compositions als Mess-Einheiten (search_algo+companion-axes als EINE direktiven-konforme Organ-Komposition), variiert allocator/layout orthogonal. Alternativ: pro Organ die kompatiblen Default-Achsen ermitteln + organ-spezifische AdHoc-Tupel emittieren. **Risiko/Umfang:** mittel-groß (Emitter-Restrukturierung); braucht eigene Charge mit frischem Kontext + perm_engine/autobuilt-Test-Deckung nach jedem Schritt.

### P2 — TEILWEISE done-verified (2026-05-31, korrekter Weg umgesetzt, CE `1f57e90`)
**✅ Direktiven-konforme Organ-Messung erreicht + verifiziert:** Der Mess-Raum `comdare_codegen_anatomy_module_list` (R5.D.3-Pilot) variiert search_algo jetzt über ALLE **6 sezierten Observable-Organe** (Art/Hot/Start/Surf/Wormhole/Masstree — je `search_algo = Observable*Organ` mit organ-KOMPATIBLEN Begleit-Achsen). Verifiziert: `test_v41_anatomy_multi_codegen` **7/7** (6 Organ-DLLs bauen+laden+messen polymorph; `genus=SearchAlgorithm`, `organ_count=17`, composition-set = alle 6). F15-Mess-Loop über die Organe läuft (`PolymorphicMeasurementLoopOverAllHandles`). Wormhole/Masstree bauen als NAMED Composition (anders als im AdHoc-Default-Pfad) — bestätigt den Planrunden-Befund.

**🔄 VERBLEIBEND (koppelt an G8/P5 — EINE autoritative Mess-Quelle):** Der ALTE `adhoc_emitter`-Pilot (R5.G, 48 DLLs) emittiert weiterhin AdHoc<Monolith, Default-Achsen> (Array256..BTree) — der Planrunden-Befund zeigte, dass dieser AdHoc-Default-Pfad organ-architektonisch NICHT umstellbar ist. `axis_03a::EnabledStrategies` listet weiter 17 Monolithen (breit referenziert: adhoc-Pilot + Äquivalenz-Tests). Um G3 VOLL zu schließen: die organ-basierte named-Composition-Messung zur AUTHORITATIVEN F15-Quelle machen (f15_compare/Pipeline laden die Organ-DLLs) + den Monolith-AdHoc-Pilot als legacy/superseded markieren. Das ist Teil der P5-Plugin-ABI-/Mess-Quellen-Konsolidierung (G8).

### P2-„F15 neu" + P3-Mindestmessung ✅ (2026-05-31, reale i7-1270P-Hardware)
`comdare-f15-compare` über die **6 sezierten Organ-Composition-DLLs** auf dieser **i7-1270P-Maschine** ausgeführt → echtes F15-Head-to-Head (Welch/Holm/Mann-Whitney/Cliff's δ + p50/mean-Ranking). 64-Batch-Lauf: p50 479.800–653.100 ns, Spanne 1,36×. **Beleg: `docs/sessions/20260531-f15-organ-messung-i7-1270p.txt`.** Damit ist die F15-Messung **direktiven-konform über Organe** (kein Monolith) auf **realer lokaler HW** erhoben — deckt P2-„F15 neu erheben" + P3-„i7-1270P-Mindestmessung". (Signifikanz braucht höhere Sample-Zahl = Skalierung; Infrastruktur + reale Zahlen belegt.)

### VERBLEIBEND zur Voll-Schließung:
- **Bridge organ-F15 → Pipeline-CSV (16-col):** f15_compare-Ranking in das Pipeline-Mess-CSV-Format überführen, damit das PDF die ECHTEN Organ-Zahlen statt Sample-Daten zeigt (P3-Vollintegration; ties G8).
- **Monolith-Supersession (G3/G8/P5):** Monolith-AdHoc-Pilot als legacy markieren; named-Composition-Organ-Messung = autoritative F15-Quelle.
- **P4** Vendor/PMC (beschaffungs-gated) · **P5** Doku-Drift/SUPERSEDED-Banner · **finaler Abnahme-Audit** über 3 Repos.

### P3-Bridge ✅ done-verified (2026-05-31, CE `4b3354b`) — REALE i7-Organ-Daten im PDF
`comdare-f15-compare --pipeline-csv` exportiert die per-Organ-Messung als 16-Spalten-Pipeline-CSV
(`total_cycles` = reale ns-Latenz). **Volle geschlossene Kette an REALEN i7-1270P-Daten verifiziert:**
f15 misst 6 sezierte Organ-Compositions → 16-col-CSV → Stufe 04 „6 rows" (LaTeX-Tabelle trägt reale Zahlen
740176/825053 ns + Organ-Namen) → Stufe 05 → **`pipeline_demo.pdf` (102 KB) mit echten Organ-Mess-Zahlen**.
Damit ist Abschluss-Kriterium **C(a)** (geschlossener E2E-Lauf an realen i7-Daten → PDF) + **C(b)**
(F15 direktiven-konform über sezierte Organe) literal belegt. Beleg: `20260531-organ-pipeline-csv-beleg.md`.

### VERBLEIBEND (Voll-Abnahme):
- **Monolith-Supersession (G3/G8/P5):** Monolith-AdHoc-Pilot als legacy markieren; die named-Composition-Organ-Messung ist die autoritative F15-Quelle (jetzt belegt). EnabledStrategies-Monolithen bleiben für Äquivalenz-Tests, sind aber NICHT mehr die F15-Headline-Quelle.
- **P4** Vendor/PMC — Beschaffungs-Anforderung an User formulieren (jemalloc/tcmalloc-Libs; Intel-PCM/MSR-Zugang).
- **P5** Doku-Drift (SUPERSEDED-Banner) · op_type_filter · Custom_BulkInsert.
- **Finaler Abnahme-Audit-Workflow** über alle 3 Repos (Kriterium C(d)).

### Nächster Schritt: P4-Beschaffungs-Anforderung formulieren + P5-Doku-Drift, dann finaler Abnahme-Audit.
