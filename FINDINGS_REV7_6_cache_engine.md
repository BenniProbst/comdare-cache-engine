# FINDINGS REV 7.6 — comdare-cache-engine (2026-05-13)

**Repo-Perspektive:** cache-engine-spezifische Findings + Korrekturen aus
der heutigen Drei-Repo-Architektur-Klaerung. Dieses Dokument enthaelt
den **vollstaendigen inhaltlichen Stand** aus cache-engine-Sicht (NICHT
nur eine Zusammenfassung).

**Schwester-Dokumente** (gleicher Sprint, andere Repos):
- Diplomarbeit-Master: `FINDINGS_REV7_6_diplomarbeit.md`
- comdare-prt-art: `FINDINGS_REV7_6_prt_art.md`
- Habich-Termin-Zusammenfassung: Diplomarbeit `20260508 Termin 7/HABICH_TERMIN7_ZUSAMMENFASSUNG_2026_05_13.md`

---

## §0 Repo-Rolle in der Drei-Repo-Architektur

cache-engine bestimmt **WIE gemessen wird** (User-Direktive 2026-05-13):
- Builder-Executable + ExperimentDriver-Library (Phase 1-7)
- ModuleLoader (LoadLibrary/dlopen)
- Demo-Workloads als Library (YCSB-A..F, Zipfian)
- Mikrobenchmark-Suite, ResultAggregator
- 23 Allokator-Familien-Adapter (A01-A23)
- 14 LEGACY_REIMPL Adapter
- Generische Werkzeuge (`tools/ycsb_cli`, `tools/latex_anhang`,
  `tools/latex_toolchain`, `tools/permutation_codegen`)

NICHT in cache-engine: die 3 Messreihen-Definitionen
(`Diplomarbeit/Code/experiment_config/`) + die Binary→CSV→LaTeX-PDF
Auswertungs-Pipeline (`Diplomarbeit/Code/`).

---

## §1 Drei-Repo-Architektur (User-Direktive, hoechste Praezision)

```
Diplomarbeit/Code/  =  WAS + AUSWERTUNG    (Anwender)
       │
       ▼ (Submodule, parallel)
comdare-prt-art      =  Test-Algorithmus  (Pruefling)
       │
       ▼ (Submodule)
comdare-cache-engine =  WIE gemessen wird  (THIS REPO — Werkzeug + Bibliothek)
```

### Konkrete Aufteilung (User-Direktive 2026-05-13 §14.5 verbatim)

> *"Die Diplomarbeit bestimmt WAS getestet wird, die CacheEngine bestimmt
> WIE es getestet wird. Die Demo workloads sind also Bibliothek
> Bestandteil der cache-engine. Die Diplomarbeit bestimmt nur die
> Compiletime Auspraegung ueber die Definition einer Konfigurationsdatei
> xml (siehe Plan) an die ExecutionEngine bzw deren Ausfuehrungsengine.
> Die Diplomarbeit wertet aber die durch die cache-engine beschriebenen
> Messungen im Binary Format selbst aus, daher hat die cache-engine
> echte Messmodule und Mikro-Benchmarks wie gehabt und laed precompiled
> C++23 modules adhoc, aber die Diplomarbeit verarbeitet die Ergebnisse
> weiter."*

| Aspekt | Repo | Begruendung |
|---|---|---|
| Mess-Mechanismus (Phase 1-7 Driver) | cache-engine | "Die CacheEngine bestimmt WIE getestet wird" |
| Builder-Executable | cache-engine | "Modul-loads und -builds sind Teil der Cache-Engine" |
| Demo-Workloads (YCSB-Library) | cache-engine | "Die Demo workloads sind Bibliothek-Bestandteil der cache-engine" |
| Mikrobenchmark-Suite | cache-engine | "echte Messmodule und Mikrobenchmarks wie gehabt" |
| Precompiled-C++23-Module-Loading | cache-engine | "laed precompiled C++23 modules adhoc" |
| XML-Configs (compile-time-Ausprägung) | Diplomarbeit | "Die Diplomarbeit bestimmt nur die Compiletime Auspraegung ueber die Definition einer Konfigurationsdatei xml" |
| Binary → CSV → LaTeX → PDF Auswertung | Diplomarbeit | "Die Diplomarbeit verarbeitet die Ergebnisse weiter" |
| 3 Messreihen (A/B/C) Definition | Diplomarbeit | "Die Diplomarbeit bestimmt WAS getestet wird" |

---

## §2 FINDING 1 — Diagnose-Output beim Library-Refactoring verloren (cache-engine intern)

**Schwere:** HOCH (kritische Mess-Mechanik-Doku).

### Was war im alten main.cpp (vor REV 7.6 Library-Refactoring)?

Pro Pipeline-Phase ein detaillierter `std::cout`-Output:

```
==== CacheEngineBuilder Phase 7.2 ====
Config:   /path/to/example_configs
Output:   /path/to/output
Mode:     full pipeline
Comdare-Root: /path/to/repo

[Phase 1] Parsed XML configs:
  cache_engine_permutations:     3
  search_algorithm_permutations: 3
  allocator_permutations:        3
  test_data_sets:                 2
  Enumerated 54 permutations.

[Phase 2] Generating 54 modules ...
  Generated: ce_lockfree:art:tcmalloc:ycsb_c
  Generated: ce_hardened:hot:mimalloc:commoncrawl
  ... (54 Lines)
  Aggregator: /path/to/generated/CMakeLists.txt

[Phase 3] Configuring cmake subbuild under /path/to/build-perms ...
  $ cmake -S "..." -B "..."
  $ cmake --build "..." --target comdare_all_permutations
[Phase 3] OK — all 54 permutation modules built.

[Phase 4] Loading permutation modules from /path/to/Debug ...
[Phase 4] Loaded 54 of 54 modules.

[Phase 5+6] Running workload (500 ops) against 54 modules ...
[Phase 5+6] Collected 54 measurement records.

[Phase 7] Wrote /path/to/measurements.csv
[Phase 7] Wrote /path/to/measurements.json
[Phase 7] Unloaded all modules.
```

### Was im REV 7.6 Wrapper (nach Library-Refactoring) verblieb:

```
==== CacheEngineBuilder Demo Wrapper (REV 7.6) ====
Demo pipeline OK.
```

**ALLE Phase-1-bis-7 Diagnose-Outputs waren verloren.**

### Root-Cause

Beim Refactoring wurde die Phase-Logik in `ExperimentDriver`-Methoden
verschoben, aber die `std::cout`-Calls **nicht** in die Methoden mit-
verschoben. Sie wurden im main.cpp **stillschweigend geloescht**.

### Korrektur (commit `d1e79f0`)

- `experiment_driver.hpp`: `ExperimentDriverOptions::verbose = true`
  (Default)
- `experiment_driver.cpp`: Jede Phase-Methode echoes ihre Diagnose
  (Counts der XML-configs, Per-Module "Generated"-Lines, cmake commands,
   Load-Status, Record-Counts, Persist-Pfade, Unload-Confirmation)
- `main.cpp`: Mode-Banner ("full pipeline" / "enumerate-only" /
  "generate-only") + `--quiet`-Flag + finales OK-Banner

### Verifikation

E2E mit 54 Permutationen: alle 7 Phasen-Diagnose-Outputs sichtbar
(Phase 1 XML-counts + Enumerated 54 / Phase 2 Generated 54x + Aggregator
/ Phase 3 cmake-commands + OK / Phase 4 Loaded 54 / Phase 5+6 Collected 54
/ Phase 7 CSV+JSON+Unloaded).

---

## §3 FINDING 2 — `*.cmake`-gitignore-Pattern hat 2 wichtige Files verschluckt

**Schwere:** HOCH (Build-blockierend bei Submodule-Konsum durch Diplomarbeit).

### Was war broken?

`.gitignore` enthielt:
```
*.cmake
!cmake/*.cmake
```

→ `tools/latex_toolchain/latex_toolchain.cmake` und
  `tools/permutation_codegen/codegen.cmake` waren silent von Git
  ignoriert.

Im Hauptrepo: Files lagen im Working-Tree. Build OK.
Im Submodule-Konsum (`Diplomarbeit/Code/external/comdare-cache-engine/`):
Files fehlten → CMake-Error "File does not exist".

### Korrektur (commit `e2dc290`)

```
*.cmake
!cmake/*.cmake
!tools/permutation_codegen/*.cmake   # NEU REV 7.6
!tools/latex_toolchain/*.cmake       # NEU REV 7.6
```

Whitelist erweitert + beide Files jetzt committed.

### Lesson Learned (cache-engine spezifisch)

Vor jedem Submodule-Pin-Bump verifizieren: `git ls-tree -r HEAD <dir>`
muss alle erwarteten Files anzeigen. Silent gitignore-Konflikte
verstecken sich oft hinter generischen Patterns. **Empfehlung:**
zukuenftige `.cmake`-Files muessen explizit zur Whitelist hinzugefuegt
werden, sobald sie ausserhalb von `cmake/` liegen.

---

## §4 FINDING 3 — ExperimentDriver-Library + main.cpp Wrapper-Pattern (REV 7.6 Q4)

### Library (`cache_engine/builder/experiment_driver/`)

```cpp
namespace comdare::builder {
    class ExperimentDriver {
    public:
        explicit ExperimentDriver(ExperimentDriverOptions opts);

        int phase1_enumerate(std::vector<PermutationDescriptor>&);
        int phase2_generate(std::vector<PermutationDescriptor> const&);
        int phase3_compile();
        int phase4_load(std::vector<ModuleHandle>&);
        int phase5_run_workload(std::span<ModuleHandle>,
                                 WorkloadOptions const&,
                                 std::vector<PermutationDescriptor> const&,
                                 ResultAggregator&);
        int phase7_export(ResultAggregator const&);

        int run_pipeline_full(WorkloadOptions const&);
    };
}
```

### Library-Konsumenten

1. **cache-engine main.cpp** — Demo-Wrapper, ruft `run_pipeline_full()`
2. **Diplomarbeit/Code/messung_driver/main.cpp** — 3-Messreihen-Loop,
   ruft `run_pipeline_full()` separat pro Messreihe (A/B/C)

### Konsequenz fuer Diagnose-Output

Diagnose muss in der Library sein (nicht in main.cpp), damit BEIDE
Konsumenten den gleichen Output bekommen.

---

## §5 FINDING 4 — Submodule-Layout aus cache-engine-Sicht

cache-engine wird in der Diplomarbeit als **paralleles Submodule** auf
Tiefe 1 eingebunden:

```
Diplomarbeit/Code/external/
├── comdare-prt-art/         (Submodule)
└── comdare-cache-engine/    (Submodule, DIESES Repo, parallel)
```

### Konsequenz fuer cache-engine

- cache-engine wird direkt konsumiert (nicht transitiv via prt-art)
- Pin-Bumps in cache-engine erfordern Pin-Bump in der Diplomarbeit
  (nicht in prt-art, weil Diplomarbeit ihre OWN cache-engine-Submodule-
  Kopie hat)
- prt-art hat IN seinem eigenen Repo ein
  `external/comdare-cache-engine/` Submodule — das wird NICHT von
  Diplomarbeit konsumiert (Diplomarbeit hat eigene Kopie)

---

## §6 FINDING 5 — Hybride PrtArtSearchEngine (REV 7.1) bleibt fuer cache-engine unveraendert

Die heutige Architektur-Korrektur betrifft NICHT die hybride API:

- 1 Param = std::vector-API
- 2+ Params = std::map-API (mit tuple-Magic fuer N>2)
- Schreiboperatoren = `status_t` (errno-style, 0=ok)

Verbleibt in `comdare-prt-art/prt_art/include/prt_art/identity/`.
cache-engine ABI (`cache_engine/abi/search_engine.hpp`) sollte
weiterhin die Basis liefern (siehe Verifikations-Punkt V6).

---

## §7 FINDING 6 — TikZ statt matplotlib fuer Diagramme (Q3-Entscheidung)

User-Entscheidung: **TikZ-Code generieren**. Aus cache-engine-Sicht:

- **Kein Python** (F-EXTRA-5-konform)
- Diagram-Generator gehoert in Diplomarbeit/Code/diagram_generator/,
  **NICHT** in cache-engine
- cache-engine `tools/latex_anhang` (CSV→LaTeX) ist hier weiterhin
  nuetzlich, aber nicht direkt verantwortlich fuer Diagramme

---

## §8 KONSOLIDIERTER STAND DER 3 REPOS

| Repo | Aktueller HEAD | Aenderungen heute (REV 7.6) |
|---|---|---|
| comdare-cache-engine main | `d1e79f0` | F2+F3+F4 Diagnose-Restore + LAYER_MAP; davor `e2dc290` (gitignore-Fix), `05b41a8` (ExperimentDriver-Lib) |
| comdare-prt-art development | `3e8044b` + `01989b9` | nur LAYER_MAP-Update, keine Code-Aenderungen |
| probst-Diplomarbeit-cache-engine main | `21aa4d8` | Code/-Skelett + Submodules + Delta 30 + STRUCTURAL_CORRECTION |

### Noch ausstehende Korrekturen (cache-engine):

1. **V6:** PRT-ART erbt von cache-engine search_engine (Verdrahtung)
2. **V7:** test_data_set_accumulation_engine bei SearchEngine-init
3. **Bugfix-Backlog:** `gtest_discover_tests`-MSB3073-Warnings (kosmetisch)
4. **Module-Bodies:** echte Permutations-Logic statt Mock

---

## §9 Original-Nachricht des Users (verbatim, fuer cache-engine relevante Abschnitte)

> *Anlass: Phase 6 INK-1 bis INK-8 Migration REV 6 (2026-05-12 1500),
> Pre-Habich-Sprechstunde von 2026-05-08, mit nachgelegter Praezisierung.*

### §9.1 Custom Allokation (cache-engine Pflicht-Disziplin)

> *"Ich denke wir haben die wichtigste Basis-Disziplin der Cache Engine
> vergessen: Custom Allokation. Die Cache Engine ist ebenfalls dafuer da,
> nach bekannten weitreichenden Allokationsmethoden Speicher zu verwalten.
> Dazu brauchen wir eine umfassende wissenschaftliche Analyse und Suche
> nach google scholar papers zum Thema Allokationsmethoden."*

### §9.2 std-Container-Vollintegration

> *"Die Anforderungen an die Allokation sind optionale thread safe
> Implementierungen der std C++ Bibliothek fuer concurrency und allocation,
> bitte recherchiere ausfuehrlich wie die einzelnen permutierten
> Implementierungen der Allokator-Algorithmus-Bausteine den Standard
> typsicher und strukturell korrekt erweitern koennen."*

### §9.3 ABI-stabiles C++23-Interface (cache-engine-eigene Implementation)

> *"Jedes kompilierte Experiment eine bestimmte Execution Engine -> Search
> Engine Rekombination ist, also formal ein zusammengesetzer Custom
> Suchalgorithmus einer definierten Baustein-Permutation als"*
>
> ```cpp
> std::variant<
>     comdare::search_engine<
>         search_algorithm_type_collection<key, value>,
>         configuration_permutation_type
>     > : comdare::execution_engine<processing_strategy_type>(...)
> >();
> ```

### §9.4 Variadic 1/2/N>2 Params

> *"Wenn nur ein Parameter angegeben wird, dann ist dieser typ die value
> und wir fuellen den key typ implizit mit einem hochzaehlenden 64bit
> unsigned long. Werden mehr typ Parameter angegeben, dann ist der erste
> typ der key, alle folgenden Typen formal ein Tupel mit allen
> zusammengesetzten values."*

### §9.5 Drei-Schichten-Hierarchie + Visitor (cache-engine als Basis)

> *"Die execution_engine erbt von der CacheEngine selbst, die als Visitor
> Pattern durch Abkoemmlinge initialisiert und bei Algorithmus-
> Entscheidungen je Fall um Rat gefragt werden kann."*

### §9.6 CacheEngineBuilder als eigenstaendiges Programm

> *"Der CacheEngineBuilder ist ein eigenstaendiges Programm welches ueber
> einen Satz xml definierter Konfigurationen einerseits alle zulaessigen
> CacheEngine Rekombinationen definiert, von denen in direkter
> Abhaengigkeit die konfigurierten Custom Suchalgorithmen gebaut werden."*

### §9.7 PRT-ART Compile-Time-Fallback auf cache-engine

> *"Wir erben aus der CacheEngine die Permutations-Struktur-Hierarchie
> der Algorithmus-Bausteine, wenn die Typen aus
> configuration_permutation_type im Prueflings-Algorithmus wie PRT_ART
> nicht gefunden werden, findet zur compile time automatisch ein fallback
> auf die Bausteine der Cache-Engine Bibliothek statt."*

### §9.8 Test-Daten-Akkumulation-Engine (cache-engine-Library)

> *"Test_data_set_accumulation_engine_type als Klasse, welche die Daten
> geladen hat und die Test-Algo-Interfaces kennt, bereitstellt."*

### §9.9 Mikrobenchmark-Suite (cache-engine)

> *"Die Mikrobenchmark suite ist ein no-deprecate wrapper aller
> Testmethoden und akkumuliert fortlaufend auf einer separaten custom
> Basis-allokation alle Messergebnisse traced und separat durch sparse
> serialized byte states binary den Testzustand und Fortschritt auf
> einer weiteren separaten custom allocation loggt."*

---

## §10 Delta-Analyse: Original-Nachricht vs. cache-engine-Stand

### §10.1 KONSISTENT (alle gefordert + heute umgesetzt)

| Anforderung | cache-engine Implementations-Stand |
|---|---|
| §9.1 23 Allokator-Familien | DONE — A01-A23 |
| §9.2 std-Container-Grundlage | DONE — `i_allocation_strategy` + std::pmr-Adapter |
| §9.2 Single-W/Multi-R default | DONE — std::shared_mutex |
| §9.2 Multi-W Cache-Page-Awareness | DONE — std::scoped_lock pattern |
| §9.3 ABI-Modul-Interface | DONE — `module_abi_v1.hpp` + 54 SHARED-Module |
| §9.3 fingerprint::to_binary_string | DONE |
| §9.5 Drei-Schichten + Visitor | DONE |
| §9.6 CacheEngineBuilder eigenstaendig | DONE — Executable + REV 7.6 Library |
| §9.8 TestDataSetAccumulationEngine | DONE als Klasse |
| §9.9 Mikrobenchmark-Suite + 2 Custom-Allokationen | DONE (Phase 6.6) |
| §9.9 Sparse-Binary-Logging | DONE (Phase 6.6) |
| §9.9 Conversion-Routine binary → handlich | DONE im Messmodul |

### §10.2 OFFEN aus cache-engine-Sicht

| Anforderung | Status | Notwendige Schritte |
|---|---|---|
| §9.5 CacheEngine als Visitor-Pattern-Service | Konzept, Skelett | Volle Implementation der `(*advise)`, `(*notify)`, `(*snapshot)` callbacks in `module_abi_v1` |
| §9.5 ExecutionEngine erbt direkt von CacheEngine | unklar | **PRUEFEN** in `cache_engine/abi/execution_engine.hpp` |
| §9.7 PRT-ART als spezielle ExecutionEngine | Konzept | **prt-art**-Korrektur (nicht cache-engine selbst) |
| §9.8 TestDataSet bei SearchEngine-init | aktuell separate Lib | Verdrahtung in SearchEngine-Konstruktor |

---

## §11 Verifikations-Liste (cache-engine Perspektive)

| # | Test | Status |
|---|---|---|
| V1 | cache-engine `comdare-cache-engine-builder` baut + E2E "Demo pipeline OK" | DONE |
| V2 | cache-engine ExperimentDriver-Library separat compile-bar | DONE |
| V3 | cache-engine latex_toolchain.cmake im git getrackt | DONE (e2dc290) |
| V4 | 23 Allokator-Adapter alle compile-bar | DONE (Phase 6.2.E+F+G) |
| V5 | ABI v1 funktional + 54 DLLs ladbar | DONE (Phase 7.2.D-F) |
| V6 | Diagnose-Output via `--verbose` voll wiederhergestellt | DONE (commit d1e79f0) |
| V7 | PRT-ART erbt von cache-engine search_engine | OFFEN |
| V8 | TestDataSetAccumulationEngine bei SearchEngine-init verdrahtet | OFFEN |

---

## §12 Querverweis

- `PROJECT_LAYER_MAP.md` (cache-engine root, REV 7.6 erweitert)
- `STRUCTURAL_CORRECTION_cache_engine.md` (Schwester-Dokument im selben Repo)
- Diplomarbeit-Repo:
  - `STRUCTURAL_CORRECTION_diplomarbeit.md` (Master mit User-Original-Nachricht)
  - `FINDINGS_REV7_6_diplomarbeit.md` (Master-Findings)
  - `20260508 Termin 7/Phase5_UML_Detail/30_architektur_delta_REV7_6_drei_repo_layer_2026_05_13.md`
  - `20260508 Termin 7/HABICH_TERMIN7_ZUSAMMENFASSUNG_2026_05_13.md`
- prt-art-Repo: `FINDINGS_REV7_6_prt_art.md`
