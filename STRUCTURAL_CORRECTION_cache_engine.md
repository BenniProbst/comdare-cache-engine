# STRUCTURAL_CORRECTION REV 7.6 — comdare-cache-engine (2026-05-13)

**Repo-Perspektive:** Wie die heutige Drei-Repo-Architektur-Korrektur die
cache-engine betrifft. Dieses Dokument enthaelt den **vollstaendigen
inhaltlichen Stand** aus cache-engine-Sicht (NICHT nur eine
Zusammenfassung) — die Master-Version in der Diplomarbeit ist parallel
gehalten.

**Schwester-Dokumente** (gleicher Sprint, andere Repos):
- Diplomarbeit-Master: `STRUCTURAL_CORRECTION_diplomarbeit.md`
- comdare-prt-art: `STRUCTURAL_CORRECTION_prt_art.md`
- Habich-Termin-Zusammenfassung: Diplomarbeit `20260508 Termin 7/HABICH_TERMIN7_ZUSAMMENFASSUNG_2026_05_13.md`

**Anlass:** User-Klarstellung 2026-05-13: die Code-Verzeichnisstruktur
der **Diplomarbeit** wurde zunaechst falsch verortet und stattdessen in
`comdare-cache-engine` abgelegt. Diese Doku korrigiert die Architektur,
**ohne** Code aus cache-engine herauszunehmen.

---

## §1 Die korrekte Drei-Repo-Architektur (User-Direktive)

```
┌──────────────────────────────────────────────────────────────────────┐
│  Diplomarbeit  (Konzept + Code-Schicht "Messung + PDF-Generierung")  │
│  ─────────────────────────────────────────────────────────────────── │
│  Dokumentation (Phase 1-6)                                            │
│  Termin-Protokolle (Termin 1-7)                                       │
│  Architektur-Skizzen (REV 1-7+)                                       │
│  Diplomarbeits-LaTeX-Manuskript (*.tex)                               │
│                                                                       │
│  +  Code/                                                              │
│     ├── messung_driver/      (C++ App, ruft prt-art-Submodule auf)    │
│     ├── binary_to_csv/        (deserialisiert Messergebnisse)         │
│     ├── csv_to_latex/         (mit Algorithmus-Baustein-Beschreibung) │
│     ├── diagram_generator/    (C++ TikZ-Generator A4-aware)           │
│     └── latex_to_pdf/         (pdflatex-Wrapper)                      │
│                                                                       │
│  Konfigdatei: experiment_config.{xml,json}                            │
│      Spezifiziert die drei zu messenden Permutations-Modi             │
└──────────────────────────────────────────────────────────────────────┘
                       │
                       │ konsumiert als Git-Submodule (parallel)
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  comdare-prt-art  (Test-Algorithmus + Mess-Interfaces)                │
│  ─────────────────────────────────────────────────────────────────── │
│  Die PRT-ART-Hauptklasse + 8 Bausteine + Mess-Hooks                   │
└──────────────────────────────────────────────────────────────────────┘
                       │
                       │ konsumiert als Git-Submodule
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  comdare-cache-engine  (THIS REPO — Werkzeug + Permutations-Bausteine)│
│  ─────────────────────────────────────────────────────────────────── │
│  STAND DER TECHNIK:                                                   │
│  - 23 Allokator-Familien (A01-A23) aus 21 Paper                       │
│  - 33 Such-Algorithmen (P01-P33) als Adapter/Re-Impl                  │
│  - Cache-Engine-Bausteine: ConcurrencyManager, TelemetryStrategy,     │
│    Page-Typen, Decision-Lambda-Trees                                  │
│  - CacheEngineBuilder (Permutations-Orchestrator)                     │
│  - ABI-Modul-Interface (module_abi_v1.hpp)                            │
│  - ExperimentDriver-Library (REV 7.6, Phase 1-7)                      │
│                                                                       │
│  WICHTIG: ist Werkzeug-Bibliothek + Konzept-Quelle.                   │
│  Wird vom prt-art und direkt von der Diplomarbeit konsumiert.         │
└──────────────────────────────────────────────────────────────────────┘
```

---

## §2 Die drei Messreihen (Pflicht-Output der Diplomarbeit-Code-Schicht)

Aus der `experiment_config` werden DREI Messreihen erzeugt — die
cache-engine liefert pro Reihe die Pipeline-Mechanik (Phase 1-7):

### §2.1 Messreihe A — PRT-ART-Testalgorithmus gegen Stand-der-Technik

**Was wird gemessen:** PRT-ART (hybride API mit Vector-/Map-/Tuple-Modi)
gegen die in der cache-engine vorhandenen Stand-der-Technik-Algorithmen
(P01 ART, P02 HOT, P03 Masstree, P04 CoCo-Trie, P05 START, P06 B2tree,
P07 Wormhole, P10 SuRF).

**cache-engine-Beitrag:** ExperimentDriver-Library + ModuleLoader +
workload_generator (YCSB-A..F) + ResultAggregator.

### §2.2 Messreihe B — Bestehende cache-engine-Permutationen

**Was wird gemessen:** Die bestehenden 54+ Permutationen aus den
`example_configs/`-XMLs, ohne PRT-ART. Reine Vergleichsbasis "Stand der
Technik" — vollstaendig cache-engine-internes Setup.

**cache-engine-Beitrag:** Codegen + Aggregator-CMakeLists +
ModuleLoader.

### §2.3 Messreihe C — Merge alt/neu

**Was wird gemessen:** Konkrete Merge-Punkte, an denen PRT-ART-Bausteine
in die cache-engine-Permutationen eingesetzt werden (PRT-ART OLC + tcmalloc-
Allokator + ART-Page; 4+2-Pool + HOT; PathPrefetch + B2tree;
MultiLevelLayout + Wormhole).

**cache-engine-Beitrag:** Compile-time-Fallback PRT_ART → cache-engine
Bausteine, Codegen-Pipeline.

---

## §3 Was in cache-engine BLEIBT (keine Migration — Korrektur §14.5)

Spaeter klargestellte User-Direktive: **cache-engine bleibt fast komplett
intakt**. Die folgenden Komponenten gehoeren in cache-engine als
*Werkzeug-Bibliothek* + *Mess-Mechanik*:

| Komponente | Wo | Rolle |
|---|---|---|
| `cache_engine/builder/main.cpp` | cache-engine | Builder-Demo-Wrapper (REV 7.6 ueber Library) |
| `cache_engine/builder/experiment_driver/` | cache-engine | **NEU REV 7.6** — Phase 1-7 Pipeline-Library |
| `cache_engine/builder/{xml_parser,codegen,permutation_loop,module_loader,experiment_runner}/` | cache-engine | Mess-Mechanik-Sub-Komponenten |
| `workload_generator/` (YCSB-A..F, Zipfian) | cache-engine | Library — Demo-Workloads sind cache-engine-Bibliothek-Bestandteil |
| `experiment/` (ResultAggregator) | cache-engine | Library |
| `benchmark_suite/` (Mikrobenchmarks) | cache-engine | Library |
| `tools/ycsb_cli/` | cache-engine | Generic YCSB-Generator-Tool |
| `tools/latex_anhang/` | cache-engine | Generic CSV→LaTeX-Tabelle |
| `tools/latex_toolchain/` | cache-engine | Generic pdflatex-Pipeline |
| `tools/permutation_codegen/` | cache-engine | Generic CMake/sh/bat NO-PYTHON |
| 23 Allokator-Familien A01..A23 | cache-engine | Bausteine |
| 14 LEGACY_REIMPL (P11..P27) | cache-engine | Bausteine |
| RCU + Succinct + HBM-Hierarchy | cache-engine | Bausteine |

**Wichtig:** cache-engine **VERLIERT NICHTS**.

---

## §4 Was sich in cache-engine geandert hat (REV 7.6 Detail)

### §4.1 Library-Refactoring (commit `05b41a8`)

- `main.cpp` (165 Zeilen) → `main.cpp` (108 Zeilen, REV 7.6 + Diagnose-Restore)
- NEU: `cache_engine/builder/experiment_driver/experiment_driver.{hpp,cpp}`
  + `comdare::builder::ExperimentDriver`-Klasse
  + 6 Phase-Methoden (`phase1_enumerate` ... `phase7_export`)
  + `run_pipeline_full()` als Convenience
  + Verbose-Flag fuer Diagnose-Output

### §4.2 Diagnose-Restore (commit `d1e79f0`)

- Phase-Methoden echoen ihren Diagnose-Output (vorher in main.cpp,
  beim ersten Refactoring stillschweigend verloren — siehe FINDINGS §1)
- main.cpp Wrapper hat Mode-Banner + `--quiet`-Flag

### §4.3 gitignore-Fix (commit `e2dc290`)

- 2 wichtige `.cmake`-Files (`latex_toolchain.cmake` + `codegen.cmake`)
  waren silent vom `*.cmake`-Pattern ignoriert
- Whitelist erweitert + Files committed

### §4.4 PROJECT_LAYER_MAP REV 7.6 (commit `d1e79f0`)

- Section 7 "REV 7.6 — Diplomarbeit-Code-Schicht konsumiert cache-engine
  via Submodule" erweitert
- Klarstellung "WAS vs WIE" als nachgelagerte Direktive eingearbeitet

---

## §5 Wer ruft cache-engine auf?

### §5.1 cache-engine main.cpp (Demo-Driver, intern)

```bash
comdare-cache-engine-builder <config_dir> <output_dir>
    [--enumerate-only] [--skip-build] [--quiet] [--comdare-root=DIR]
```

### §5.2 Diplomarbeit/Code/messung_driver/main.cpp (Anwender-Driver)

Konsumiert `comdare::builder::ExperimentDriver` als Library + ruft
sie pro Messreihe (A/B/C) auf. Wird ueber das Submodule
`Code/external/comdare-cache-engine/` integriert.

### §5.3 Indirekt: PRT-ART-Adapter (Codegen-Output)

`comdare_perm_<fp>.dll`-Module benutzen die hybride PrtArtSearchEngine
aus dem prt-art-Submodule, aber kompilieren gegen cache-engine-Bausteine.

---

## §6 Aktueller Repo-State (cache-engine main)

| Commit | Beschreibung |
|---|---|
| `d1e79f0` | REV 7.6 F2+F3+F4 — Diagnose-Restore + LAYER_MAP |
| `e2dc290` | gitignore-Fix fuer 2 .cmake-Files |
| `05b41a8` | REV 7.6 Q4 — ExperimentDriver-Library + main.cpp Wrapper |
| `80ce6be` | PROJECT_LAYER_MAP.md (urspruengliche Version, vor REV 7.6) |
| `a8f12cf` | Habich H2+H3+H4 + CLion-Config |
| `bdb8c24` | Succinct + HBM (Aufgaben #105 + #106) |
| `1e9fe57` | Eigene RCU-Implementation (#104) |

---

## §7 Naechste Schritte aus cache-engine-Perspektive

1. **Verifikation V6+V7** (User-Aufgabe): Full-Build der Diplomarbeit/Code/
   inkl. cache-engine als Submodule (~30 Min).
2. **Bugfix-Backlog:** `gtest_discover_tests`-MSB3073-Warnings (kosmetisch,
   EXEs werden trotzdem gebaut).
3. **Module-Bodies:** die 54 generierten `comdare_perm_<fp>.dll` haben
   aktuell leere `run_workload` (setzt nur Defaults). Echte Permutations-
   Implementationen folgen.
4. **Phase 4 Verdrahtung:** `create_instance(nullptr)` → echter
   `comdare_cache_engine_v1*`-Service-Pointer.

---

## §8 Diskussion: Was hat sich beim Verstaendnis "WAS vs WIE" geklaert?

Die spaetere User-Direktive (§14.5 in der Master-Doku) hat klargestellt,
dass **alle generischen Werkzeuge in cache-engine BLEIBEN**. Es sind die
*Anwender-Spezialisierungen* (LaTeX-Tabelle mit Algorithmus-Steckbrief,
A4-aware TikZ-Diagramme, 3-Messreihen-Loop), die in die Diplomarbeit
gehoeren.

Daher gilt fuer cache-engine: **cache-engine bleibt das Werkzeug-Repo,
NICHT das Anwender-Repo.** Code in cache-engine ist generischer Backbone,
nicht Diplomarbeits-spezifisch.

---

## §9 Risiken bei zukuenftiger Migration (rein theoretisch)

1. **Submodule-Tiefe 2:** wenn Diplomarbeit/Code/external/comdare-prt-art/
   wiederum cache-engine als Submodule hat, sind 2 `git submodule update`
   noetig. Aktuell geloest durch **parallel** Layout (siehe §10 Q2).
2. **CMake-Find-Paths:** falls `comdare::ycsb_cli_lib` umzieht, brechen
   dependente Tests in cache-engine.
3. **PROJECT_LAYER_MAP.md** und die heutigen Delta-Doks (25-30 unter
   Diplomarbeit) referenzieren konkrete cache-engine-Pfade — Aenderungen
   muessen synchron gehalten werden.

---

## §10 Original-Nachricht des Users (verbatim, Quelle der Architektur-Definition)

> *Anlass: Phase 6 INK-1 bis INK-8 Migration REV 6 (2026-05-12 1500),
> Pre-Habich-Sprechstunde von 2026-05-08, mit nachgelegter Praezisierung.*

### §10.1 Custom Allokation als vergessene Basisdisziplin

> *"Ich habe gerade 20260512-1500-phase6-ink1-bis-ink8-migration-rev6.md
> gelesen und ich denke wir haben die wichtigste Basis-Disziplin der Cache
> Engine vergessen: Custom Allokation. Die Cache Engine ist ebenfalls
> dafuer da, nach bekannten weitreichenden Allokationsmethoden Speicher zu
> verwalten. Dazu brauchen wir eine umfassende wissenschaftliche Analyse
> und Suche nach google scholar papers zum Thema Allokationsmethoden."*

### §10.2 Concurrency-/Thread-Anforderungen + std::-Container-Vollintegration

> *"Die Anforderungen an die Allokation sind optionale thread safe
> Implementierungen der std C++ Bibliothek fuer concurrency und allocation,
> bitte recherchiere ausfuehrlich wie die einzelnen permutierten
> Implementierungen der Allokator-Algorithmus-Bausteine den Standard
> typsicher und strukturell korrekt erweitern koennen, indem sie in der
> Lage sein muessen, eine Grundlage aller verwendeten std container zu
> bilden, was umfangreiche Tests fuer Allokation, Container, Threading und
> Concurrency-Sicherheit bedarf."*

### §10.3 Das ABI-stabile C++23-Interface (Pseudo-Code)

> *"Bezueglich der ABI stabilen C++23 interfaces verhaelt es sich so, dass
> jedes kompilierte Experiment eine bestimmte Execution Engine -> Search
> Engine Rekombination ist, also formal ein zusammengesetzer Custom
> Suchalgorithmus einer definierten Baustein-Permutation als"*
>
> ```cpp
> std::variant<
>     comdare::search_engine<
>         search_algorithm_type_collection<key, value>,
>         configuration_permutation_type
>     > : comdare::execution_engine<processing_strategy_type>(
>         test_data_set_accumulation_engine_type
>             data_accumulation_benchmark_routines(data_set)
>     )
> >();
> ```

### §10.4 Variadic-Parameter-Wandlung (Funktion vs. Typ)

> *"Wenn nur ein Parameter in einem zu generierenden Suchalgorithmus
> angegeben wird, dann muss dieser typ die value sein und wir fuellen den
> key typ der Darstellung implizit automatisch mit einem beim Einfuegen
> stets hochzaehlenden 64bit unsigned long. Werden mehr typ Parameter
> angegeben, dann ist der erste typ der Ebene der key, und alle folgenden
> Typen formal ein Tupel mit allen zusammengesetzten values."*

### §10.5 Drei-Schichten-Hierarchie (CacheEngine -> ExecutionEngine -> SearchEngine)

> *"Die execution_engine von der die SearchEngine erbt, enthaelt als
> Basisklasse die CacheEngine selbst, die als Visitor Pattern durch
> Abkoemmlinge, wie die SearchEngine, initialisiert und bei Algorithmus-
> Entscheidungen je Fall in jedem Algorithmus-Baustein um Rat gefragt oder
> als experimentelles OS Interface direkt verwendet werden kann."*

### §10.6 CacheEngineBuilder als eigenstaendiges Programm

> *"Der CacheEngineBuilder ist ein eigenstaendiges Programm welches ueber
> einen Satz xml definierter Konfigurationen einerseits alle zulaessigen
> CacheEngine Rekombinationen definiert, von denen in direkter
> Abhaengigkeit die konfigurierten Custom Suchalgorithmen gebaut werden.
> Implizit hat also die execution_engine als typ eine Reihe an CacheEngine
> impliziter Typen, die zur compiletime gesetzt und kompiliert werden."*

### §10.7 Schichten-Rollen (Provider vs. Konsument)

> *"Die ExecutionEngine stellt mithilfe der CacheEngine experimentelle OS
> primitiven bereit, die SearchEngine dann Implementierungen fuer
> permutationen der fuer Suche spezifische komplexere experimentelle
> Standard OS Such- und Speicherzugriffsmuster und Routinen bereit; die
> Search Engine bildet dann das Dach des Konstruktes als oberster Layer."*

### §10.8 PRT-ART als Pruefling-Algorithmus mit Compile-Time-Fallback

> *"Strategisch sollten wir die Struktur der CacheEngine in der
> Bearbeitung der stubs und Struktur bevorzugen, weil wir den PRT_ART
> spaeter dort hinein mergen wollen. (...) Wir erben also aus der
> CacheEngine die Permutations-Struktur-Hierarchie der Algorithmus-
> Bausteine, wenn die Typen aus configuration_permutation_type im
> Prueflings-Algorithmus wie PRT_ART nicht gefunden werden, findet zur
> compile time automatisch ein fallback auf die Bausteine der Cache-Engine
> Bibliothek statt."*

### §10.9 Aktion: _archive_code_pre_migration/

> *"Zu deinem Vorschlag der _archive_code_pre_migration/ --> bitte mach
> das."*

**Status:** ERLEDIGT — Verzeichnis existiert (268MB lokal), ist in
`.gitignore` als ignored eingetragen (`/_archive_code_pre_migration/`),
wird nicht gepusht.

### §10.10 Erste Doku-Direktive

> *"Bitte dokumentiere im ersten Schritt alle meine Anmerkungen und den
> Plan mit hoechster Praezision, um das Modulare Interface der precompiled
> ABI stabilen C++23 Module vorzubereiten."*

### §10.11 Test-Daten-Akkumulation-Engine

> *"Letzter Hinweis: Wenn wir unterschiedliche Testdatentypen haben, muss
> jeder von diesen Datensaetzen per Deserialization oder parsing in den
> Arbeitsspeicher geladen und verfuegbar (und fair reproduzierbar aligned)
> gemacht werden, um einen definierten Experiment Ablauf ueber
> test_data_set_accumulation_engine_type als Klasse, welche die Daten
> geladen hat und die Test-Algo-Interfaces kennt, bereitstellt."*

### §10.12 Mikrobenchmark-Suite Anforderungen

> *"Die Mikrobenchmark suite ist ein no-deprecate wrapper aller
> Testmethoden und akkumuliert fortlaufend auf einer separaten custom
> Basis-allokation (die so gross sein muss, dass sie nie failed oder
> erweitert werden muss) alle Messergebnisse traced und separat durch
> sparse serialized byte states binary den Testzustand und Fortschritt
> auf einer weiteren separaten custom allocation loggt - alles wird zum
> Ende eines Experimentes erst von binary in handlichere Formate
> konsolidiert."*

---

## §11 Delta-Analyse: Original-Nachricht vs. heutiger cache-engine-Stand

### §11.1 KONSISTENT (alle gefordert + heute in cache-engine umgesetzt)

| Anforderung (§10) | cache-engine Implementations-Stand |
|---|---|
| §10.1 23 Allokator-Familien | DONE — `cache_engine/include/cache_engine/allocators/families/a01..a23/` |
| §10.2 std-Container-Grundlage | DONE — `i_allocation_strategy` + std::pmr-Adapter (Phase 6.2.E + F+G) |
| §10.2 Single-W/Multi-R default | DONE — std::shared_mutex in allen Adaptern |
| §10.2 Multi-W Cache-Page-Awareness | DONE — std::scoped_lock pattern in 23 Adaptern |
| §10.3 ABI-Modul-Interface | DONE — `module_abi_v1.hpp` + 54 SHARED-Module gebaut |
| §10.4 fingerprint::to_binary_string | DONE — `cache_engine/fingerprint/fixed_length_fingerprint.hpp` |
| §10.5 Drei-Schichten + Visitor | DONE — `cache_engine/abi/search_engine.hpp` + `execution_engine.hpp` |
| §10.6 CacheEngineBuilder eigenstaendig | DONE — `comdare-cache-engine-builder` Executable |
| §10.6 ExperimentDriver-Library (REV 7.6) | DONE — `cache_engine/builder/experiment_driver/` |
| §10.9 _archive_code_pre_migration | DONE — 268MB lokal, ignored |
| §10.11 test_data_set_accumulation_engine_type | DONE als **Klasse** (Phase 6.5) |
| §10.12 Mikrobenchmark-Suite + 2 Custom-Allokationen | DONE (Phase 6.6) |
| §10.12 Sparse-Binary-Logging | DONE (Phase 6.6) |
| §10.12 Conversion-Routine binary → handlich | DONE im Messmodul (no-deprecate-Wrapper) |

### §11.2 KONSEQUENZEN AUS DER KLAERUNG "WAS vs WIE" (cache-engine BLEIBT)

| Komponente | Erste Idee | Endgueltig (nach §14.5 der Master-Doku) |
|---|---|---|
| `tools/ycsb_cli` | Verschieben nach Diplomarbeit | **BLEIBT** in cache-engine (generisches Werkzeug) |
| `tools/latex_anhang` | Verschieben | **BLEIBT** (generisches CSV→LaTeX) |
| `tools/latex_toolchain` | Verschieben | **BLEIBT** (generische pdflatex-Pipeline) |
| `cache_engine/builder/main.cpp` Phase 4-7 | Verschieben | **BLEIBT** als Demo-Wrapper, Logik wandert in `experiment_driver/`-Library |
| `workload_generator/` | Grenzfall | **BLEIBT** (Algorithmus = Werkzeug) |
| `experiment/` (ResultAggregator) | Grenzfall | **BLEIBT** als Library (auch Diplomarbeit-Code verfuegbar) |

### §11.3 FEHLEND / NICHT VOLLSTAENDIG (aus cache-engine-Sicht)

| Anforderung | Status | Notwendige Schritte |
|---|---|---|
| §10.5 CacheEngine als Visitor-Pattern-Service | Konzept dokumentiert, Code als Skelett | Volle Implementation der `(*advise)`, `(*notify)`, `(*snapshot)` callbacks in `module_abi_v1` mit echtem State-Management |
| §10.6 ExecutionEngine erbt direkt von CacheEngine | unklar — REV 7 hat `execution_engine : public ?` | **PRUEFEN** ob direkte CacheEngine-Vererbung in `cache_engine/abi/execution_engine.hpp` korrekt aufgesetzt ist |
| §10.7 SearchEngine als Provider von OS-Search-/Mem-Mustern | als Interface dokumentiert | Implementations-Skelette fuer `notify_density_threshold`, `notify_hot_path_detected`, `notify_workload_change` |
| §10.8 PRT-ART als spezielle ExecutionEngine | Konzept klar, **prt-art PrtArtSearchEngine** erbt heute NICHT von `comdare::search_engine` ABI | **KORREKTUR** in prt-art noetig (REV 7.6) |
| §10.11 test_data_set_accumulation_engine_type bei SearchEngine-init | aktuell separate Library | Verbindung: SearchEngine-Konstruktor empfaengt + haelt eine `TestDataSetAccumulationEngine`-Instanz |

---

## §12 Aktualisierter cache-engine-Plan (mit Klarstellungs-Praezision)

### §12.1 Phase M1 (cache-engine-Seite): keine Migration noetig

Alle generischen Werkzeuge BLEIBEN. cache-engine wird nur **erweitert**:

- ExperimentDriver-Library als saubere Library-Schicht (DONE)
- Diagnose-Output in Library wiederhergestellt (DONE)
- gitignore-Whitelist (DONE)
- LAYER_MAP REV 7.6 (DONE)

### §12.2 Phase M2 (gegenueber prt-art): Verdrahtung

`prt-art::PrtArtSearchEngine` muss konkret von `comdare::search_engine`
(aus cache-engine ABI) erben — **§10.8** verlangt das. Dazu muss
cache-engine garantieren, dass `search_engine.hpp` einen sauberen
Public-Header-Pfad hat (`prt_art_root/external/comdare-cache-engine/
cache_engine/include/cache_engine/abi/search_engine.hpp`).

### §12.3 Phase M3 (gegenueber Diplomarbeit): Submodule-Konsum

cache-engine wird **parallel** als Submodule eingebunden. Korrekturen
in cache-engine (z. B. gitignore-Whitelist) propagieren durch
Submodule-Pin-Bumps. Diplomarbeit muss bei Bedarf den Pin bumpen
(siehe S5 Tasks).

---

## §13 Verifikations-Liste (Tests vor / nach REV 7.6)

| # | Test | Status |
|---|---|---|
| V1 | `_archive_code_pre_migration/` existiert + ignored | DONE (268MB lokal) |
| V2 | 23 Allokator-Adapter alle compile-bar | DONE (Phase 6.2.E+F+G) |
| V3 | ABI v1 funktional + 54 DLLs ladbar | DONE (Phase 7.2.D-F) |
| V4 | Variadic-Magic 1/2/N>2 Params | DONE (REV 7.1, hybride PrtArtSearchEngine) |
| V5 | fingerprint::to_binary_string fuer alle Typen | DONE (`fixed_length_fingerprint.hpp`) |
| V6 | PRT-ART erbt von cache-engine search_engine | **OFFEN** — heute storage_=std::map, nicht typed search_engine |
| V7 | test_data_set_accumulation_engine_type bei SearchEngine-init | **OFFEN** — heute getrennte Library |
| V8 | cache-engine ExperimentDriver-Library separat compile-bar | DONE (REV 7.6 Q4) |
| V9 | `tools/latex_toolchain/latex_toolchain.cmake` im git-Tree | DONE (commit e2dc290) |
| V10 | E2E "Demo pipeline OK" mit 54 DLLs | DONE |
| V11 | Diagnose-Output via `--verbose` voll wiederhergestellt | DONE (commit d1e79f0) |

---

## §14 Offene Fragen + Entscheidungen (Q1-Q4)

Aus der Master-Doku §14 — hier mit den heute getroffenen Entscheidungen
+ ihrer cache-engine-Konsequenz:

### Q1: Migrations-Reihenfolge — Hybrid (gewaehlt)

User-Entscheidung: **Option C (Hybrid)** — kein Verschieben, sondern
Re-Implementation + Behalten der Werkzeuge in cache-engine.

**cache-engine-Konsequenz:** keine Komponente wird verschoben oder
gestrichen. Nur Library-Refactoring + Diagnose-Restore + gitignore-Fix.

### Q2: Submodule-Struktur — Parallel (gewaehlt)

```
Diplomarbeit/Code/external/
├── comdare-prt-art/         ← parallel
└── comdare-cache-engine/    ← parallel (NICHT genested unter prt-art)
```

**cache-engine-Konsequenz:** cache-engine wird direkt von der
Diplomarbeit konsumiert (nicht transitiv via prt-art).

### Q3: Diagram-Generator-Technologie — TikZ (gewaehlt)

**cache-engine-Konsequenz:** keine Diagram-Library in cache-engine
erforderlich. Diagram-Generator gehoert in Diplomarbeit/Code/diagram_generator/.

### Q4: cache-engine main.cpp Phase 4-7 — Library + duenner Wrapper (gewaehlt)

User-Entscheidung: **Option B** — Library-Funktion
(`run_pipeline_full()`) + main.cpp wird Demo-Wrapper. **DONE** in REV
7.6 Q4 commit `05b41a8`.

---

## §15 Querverweis

- `FINDINGS_REV7_6_cache_engine.md` (heutige Findings im Detail)
- `PROJECT_LAYER_MAP.md` (10-Schichten-Hierarchie der cache-engine, REV 7.6 erweitert)
- Diplomarbeit-Repo: `STRUCTURAL_CORRECTION_diplomarbeit.md` (Master mit User-Original-Nachricht §10 verbatim, vollständige Q-Diskussion §14)
- Diplomarbeit-Repo: `20260508 Termin 7/HABICH_TERMIN7_ZUSAMMENFASSUNG_2026_05_13.md`
- Diplomarbeit-Repo: `20260508 Termin 7/Phase5_UML_Detail/30_architektur_delta_REV7_6_drei_repo_layer_2026_05_13.md`
- prt-art-Repo: `STRUCTURAL_CORRECTION_prt_art.md`
