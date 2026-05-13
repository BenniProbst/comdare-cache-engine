# PROJECT_LAYER_MAP — comdare-cache-engine (2026-05-13)

Strategische Strukturübersicht für **manuelles Code-Review**.
Reihenfolge entspricht der **empfohlenen Lese-Reihenfolge**.

---

## 0. Repo-Rolle

`comdare-cache-engine` ist die **Werkzeug-Bibliothek** im
Drei-Schichten-System (REV 7 §4.2):

```
CacheEngine  →  ExecutionEngine  →  SearchEngine  ←  PRT-ART
   (this repo)    (this repo)        (this repo)     (consumer)
```

Beinhaltet:
- **Allokator-Bibliothek** (23 Family-Adapter A01–A23)
- **CacheEngine-Bausteine** (Page-Typen, ConcurrencyManager, TelemetryStrategy)
- **CacheEngineBuilder** (Permutations-Orchestrator mit XML-Config)
- **Workload-Generator + ResultAggregator** (Phase 7 Experiment-Loop)
- **External Tools** (ycsb_cli, latex_anhang, latex_toolchain)
- **External References** (`ext/`-Verzeichnis: 23 geklonte Allokator + Such-Repos)

---

## 1. Top-Layer-Hierarchie (10 Schichten von Anwender nach Hardware)

```
┌─────────────────────────────────────────────────────────────────────┐
│ L1  TOOLS (Anwender-Frontend)                                        │
│     tools/                                                           │
│     ├── ycsb_cli/        (YCSB-A..F Generator, 3 Output-Formate)     │
│     ├── latex_anhang/    (CSV → LaTeX-booktabs-Tabelle, Phase 8)     │
│     ├── latex_toolchain/ (drawio + pdflatex pipeline, Habich H3)     │
│     └── permutation_codegen/ (CMake/sh/bat, kein Python — F-EXTRA-5) │
├─────────────────────────────────────────────────────────────────────┤
│ L2  CACHE-ENGINE-BUILDER (Permutations-Orchestrator)                 │
│     cache_engine/builder/                                            │
│     ├── main.cpp                  (Master-Driver Phase 1–7)          │
│     ├── xml_config_parser/        (Phase 1: XML → CacheEngineConfig) │
│     ├── permutation_loop/         (Phase 1: enumerate Descriptors)   │
│     ├── codegen/                  (Phase 2: cpp + CMakeLists pro Perm)│
│     ├── module_loader/            (Phase 4: LoadLibrary/dlopen)      │
│     ├── experiment_runner/        (Phase 5+6: workload-driver)       │
│     └── 14 Sub-Komponenten        (cache_engine_component,           │
│                                    decision_lambda_trees,            │
│                                    measurement_matrix, ...)          │
├─────────────────────────────────────────────────────────────────────┤
│ L3  ABI-INTERFACE (REV 7 §4.2)                                        │
│     cache_engine/include/cache_engine/abi/                           │
│     ├── module_abi_v1.hpp          (C-API für SHARED-Module)         │
│     ├── search_engine.hpp          (Layer 3 - oberste Such-Schicht)  │
│     ├── execution_engine.hpp       (Layer 2 - Provider-Rolle)        │
│     └── configuration_permutation.hpp (Variadic-Template-Magic)      │
├─────────────────────────────────────────────────────────────────────┤
│ L4  PERMUTATION-FLAGS + DECISION-LAMBDA-TREES                        │
│     cache_engine/include/cache_engine/                               │
│     ├── flags/permutation_flags.hpp (10 Banks à 64 Bit Identity)     │
│     ├── lambda/decision_lambda_trees.hpp (10 Familien)               │
│     ├── observer/observer_registry.hpp                               │
│     ├── concurrency/concurrency_manager.hpp (10 Disziplinen)         │
│     └── telemetry/telemetry_strategy.hpp (5 Strategien Kuehn)        │
├─────────────────────────────────────────────────────────────────────┤
│ L5  CACHE-ENGINE-MODE + HEURISTIK                                    │
│     cache_engine/include/cache_engine/                               │
│     ├── mode/cache_engine_mode.hpp (Scheduler + Heuristik)           │
│     ├── platform/platform_probe.hpp (Auto-Discovery)                 │
│     ├── hbm/hbm_hierarchy.hpp (HBM-Hybrid + Abstract Factory) NEU    │
│     └── command/hybrid_composition_command.hpp                       │
├─────────────────────────────────────────────────────────────────────┤
│ L6  PAGE-TYPEN + INTERNAL-SEARCH (REV 5 K05 - 6 Pflicht-Typen)       │
│     search_engine/page/                                              │
│     prt_art/include/prt_art/concepts/i_search_page_structure.hpp     │
│     6 Page-Types: ARTNode, BPlusPage, RedirectNode, ...              │
├─────────────────────────────────────────────────────────────────────┤
│ L7  ALLOKATOREN (23 Family-Adapter, Phase 6.2)                       │
│     cache_engine/include/cache_engine/allocators/families/           │
│     ├── a01_hoard/, a02_slab/, a03_michael_lockfree/,                │
│     ├── a04_mimalloc/, a05_jemalloc/, a06_tcmalloc/, a07_snmalloc/   │
│     ├── a08_scalloc/, a09_numalloc/, a10_rpmalloc/, a11_lrmalloc/    │
│     ├── a12_cama/, a13_starmalloc/, a14_tcmalloc_warehouse/          │
│     ├── a15_hmalloc/, a16_pim_malloc/, a17_crystalline/              │
│     ├── a18_exgen_malloc/, a19_buddy/, a20_dlmalloc/, a21_ptmalloc2/ │
│     ├── a22_n3916_pmr/, a23_vmem_magazines/                          │
│     └── concepts/i_allocation_strategy.hpp (gemeinsame Schnittstelle)│
├─────────────────────────────────────────────────────────────────────┤
│ L8  CONCEPTS + EVENTS + MEASUREMENT-BUFFER                           │
│     cache_engine/include/cache_engine/                               │
│     ├── concepts/  (pressure_state, cache_recommendation, cpuid)     │
│     ├── events/    (Event-Hierarchy + Observer)                      │
│     ├── buffer/in_memory_measurement_buffer.hpp                      │
│     └── sub_engines/  (C01..C12 Slots)                               │
├─────────────────────────────────────────────────────────────────────┤
│ L9  RECLAMATION (Aufgabe #104)                                       │
│     cache_engine/reclamation/rcu_reclaim/                            │
│     └── rcu.hpp (eigene RCU statt liburcu)                           │
├─────────────────────────────────────────────────────────────────────┤
│ L10 SUCCINCT (Aufgabe #105)                                          │
│     succinct/include/comdare/succinct/                               │
│     ├── bit_vector.hpp (rank/select mit 256-bit-Block-Index)         │
│     └── louds.hpp (Jacobson 1989, P09)                               │
├─────────────────────────────────────────────────────────────────────┤
│ L11 WORKLOAD + EXPERIMENT (Phase 7.1+7.3+7.4)                        │
│     workload_generator/  (YCSB-A..F + Zipfian + Uniform)             │
│     experiment/          (ResultAggregator + CSV/JSON Export)        │
│     benchmark_suite/     (No-deprecate-Wrapper + Mikrobenchmarks)    │
│     test_data_accumulation/ (TestDataSetAccumulationEngine)          │
└─────────────────────────────────────────────────────────────────────┘
```

### Lese-Reihenfolge (empfohlen, bottom-up)

| # | Schicht | Begründung |
|---|---|---|
| 1 | **L3 ABI** | `module_abi_v1.hpp` — der zentrale Vertrag aller Module |
| 2 | **L9 RCU** + L10 Succinct | Standalone-Bibliotheken, klein, isoliert (HEUTE NEU) |
| 3 | L4 Flags+Lambda | Fundament für L2 Builder + L3 ABI |
| 4 | L7 Allocators (Sample 3: tcmalloc, mimalloc, rpmalloc) | 23 Adapter ähnliches Muster — 3 Tier-1 reichen für Verständnis |
| 5 | L6 Page-Typen | 6 Pflicht-Page-Typen + Concept |
| 6 | L11 Workload+Experiment | Phase-7-Logik: Workload → Run → Aggregator |
| 7 | **L2 Builder** | `main.cpp` Phase 1–7 (HEUTE: Phase 4-7 ergänzt) |
| 8 | L1 Tools | ycsb_cli, latex_anhang, latex_toolchain (HEUTE NEU) |
| 9 | L5+L8 (optional) | Mode/Heuristik/Concepts/SubEngines — Detail-Layer |

---

## 2. Modul-Abhängigkeits-Diagramm (CMake-Targets)

```
cache_engine_builder (executable)
 ├── comdare_builder_xml_config_parser
 ├── comdare_builder_codegen
 │    └── comdare_builder_xml_config_parser
 ├── comdare_builder_permutation_loop
 │    └── comdare_builder_xml_config_parser
 ├── comdare_builder_experiment_runner
 ├── comdare_module_loader (NEU)
 │    └── (Unix: -ldl, Windows: kernel32 implicit)
 ├── comdare::workload_generator
 │    └── (header-only abi)
 └── comdare::experiment
      └── comdare::workload_generator
      └── comdare::benchmark_suite

comdare::cache_engine_concepts (INTERFACE)
 └── alle CacheEngine-Header

comdare::prt_art_concepts (INTERFACE)
 └── comdare::cache_engine_concepts

comdare::succinct (INTERFACE, NEU)
comdare::rcu      (INTERFACE, NEU)

comdare_perm_<fp> (54x SHARED, gebaut vom Builder)
 └── module_abi_v1 (header-only)

comdare_legacy_p{11..27}_* (14x INTERFACE, NEU)
 └── eigenständig — keine inter-Module Deps

Tools:
comdare_ycsb_cli (executable)
 └── comdare::workload_generator

comdare_latex_anhang (executable)
 └── (standalone)
```

---

## 3. Test-Map (Verzeichnis-Layout entspricht Schicht-Layout)

```
tests/unit/  (alle MSVC-Debug ctest verifiziert, 100% pass)
├── test_pressure_state.cpp                  L8 concepts
├── test_cache_recommendation.cpp            L8 concepts
├── test_cpuid_probe.cpp                     L5 platform
├── test_concepts_compile.cpp                L8
├── test_value_handle.cpp                    L6 page (PRT-ART concept)
├── test_three_layer_audit.cpp               L3 ABI
├── test_permutation_flags.cpp               L4 flags
├── test_event_hierarchy.cpp                 L8 events
├── test_observer_registry.cpp               L8 events
├── test_decision_trees.cpp                  L4 lambda
├── test_decision_lambda_registry.cpp        L4 lambda
├── test_concurrency_disciplines.cpp         L4 concurrency
├── test_telemetry_strategies.cpp            L4 telemetry
├── test_platform_concepts.cpp               L5 platform
├── test_cache_hierarchy_and_mode.cpp        L5 mode
├── test_strategy_command.cpp                L5 command
├── test_measurement_buffer.cpp              L8 buffer
├── test_sub_engines.cpp                     L8 sub_engines
├── test_six_page_structures.cpp             L6 pages
├── test_allocator_concepts.cpp              L7 allocators
├── test_abi_interface.cpp                   L3 ABI
├── test_test_data_accumulation.cpp          L11 test_data
├── test_benchmark_suite.cpp                 L11 benchmarks
├── test_allocator_full_matrix.cpp           L7 (Container-Matrix × 23 Adapter)
├── test_workload_and_experiment.cpp         L11 workload+experiment
├── test_ycsb_cli.cpp                NEU      L1 tools
├── test_builder_codegen.cpp         NEU      L2 builder
├── test_latex_anhang.cpp            NEU      L1 tools
├── test_rcu.cpp                     NEU      L9 RCU
├── test_succinct.cpp                NEU      L10 succinct
├── test_hbm_hierarchy.cpp           NEU      L5 hbm
├── test_module_loader.cpp           NEU      L2 builder (Phase 4)
└── mock_permutation_module.cpp      NEU      L2 builder (Test-DLL)

prt_art/legacy_reimpl/P{11..27}/tests/*  NEU  L7 reimpl
```

---

## 4. Paper-Code-Referenzen (33 Such-Paper + 23 Allokator-Paper)

### Such-Algorithmen (`ext/P*-*/`)

| Tier | Repo | Paper |
|---|---|---|
| 1 | `ext/P01-ART/` | Leis/Kemper/Neumann 2013 |
| 1 | `ext/P02-HOT/` | Binna/Zangerle/Pichl/Specht/Leis 2018 |
| 1 | `ext/P03-Masstree/` | Mao/Kohler/Morris 2012 |
| 1 | `ext/P04-CoCo-trie/` | Boffa/Ferragina/Tosoni/Vinciguerra 2024 |
| 1 | `ext/P05-START/` | Fent/Jungmair/Kipf/Neumann 2020 |
| 1 | `ext/P06-B2tree/` | Schmeisser/Schuele/Leis/Neumann/Kemper 2022 |
| 1 | `ext/P07-Wormhole/` | Wu/Ni/Jiang 2019 |
| 1 | `ext/P10-SuRF/` | Zhang/Lim/Leis/Andersen et al. 2018 |
| 2 | `ext/P20-BTreesAreBack/` | Mueller/Benson/Leis 2025 |
| 3 | `ext/P25-Mahling/` | Mahling/Weisgut/Rabl 2025 |
| 3 | `ext/P29-RCU/` | McKenney et al. RCU 2001 OLS (→ jetzt eigene impl) |
| 3 | `ext/P30-HazardPointers/` | Michael 2004 |

**Re-Implementations** unter `prt_art/legacy_reimpl/` (P11-P14, P16-P19, P21-P24, P26-P27):
- P11-CSS-tree → `css_node_page.hpp`
- P12-CSB-tree → `csb_node_group_page.hpp`
- ... (siehe Delta-Doku 26, Diplomarbeit-Repo)

### Allokatoren (`ext/A*-*/`)

| Tier | Repo | Family-Adapter (Header-Pfad) |
|---|---|---|
| 1 | `ext/A04-mimalloc/` | `cache_engine/include/cache_engine/allocators/families/a04_mimalloc/` |
| 1 | `ext/A06-tcmalloc/` | `.../a06_tcmalloc/` |
| 1 | `ext/A10-rpmalloc/` | `.../a10_rpmalloc/` |
| 1 | `ext/A05-jemalloc/` | `.../a05_jemalloc/` |
| 1 | `ext/A07-snmalloc/` | `.../a07_snmalloc/` |
| ... | (23 insgesamt — siehe `Allokator_Matrix.txt` in Diplomarbeit) | |

**Code-Quality-Audit:** `docs/quality_audit/HABICH_H2_CODE_QUALITY_2026_05_13.md`
mit Tier-1 bis Tier-4 Empfehlungen.

---

## 5. Cross-Projekt-Referenzen

| Ziel | Wo | Form |
|---|---|---|
| **prt-art** | `prt_art/` (Sub-Tree, NICHT submodule) | Header-only Concepts + Tests + LEGACY_REIMPL |
| → prt-art-Repo | Verwendet als externes Git-Submodule des prt-art Repos | Inverser Pfeil! prt-art konsumiert cache-engine |
| **Diplomarbeit** | Cross-References in Code-Kommentaren | REV 6/7 §X.Y, Termin 7 Phase5_UML_Detail |
| ↔ Bausteine_Matrix | `docs/sources/` und Diplomarbeit-Repo | Mapping P-IDs ↔ Code-Pfade |

**Wichtige Asymmetrie:**
`comdare-cache-engine` enthält ein `prt_art/` Unterverzeichnis (Sub-Tree, kein
submodule). Das ist die *Werkzeug-Seite* der PRT-ART-Concepts. Der echte
*Prüfling* lebt im separaten `comdare-prt-art` Repo, das wiederum
`comdare-cache-engine` als Submodule unter `external/` konsumiert.

```
comdare-cache-engine ─────────── prt_art/ (Concepts + Skelette)
                                          ↑
                                          │ (logische Spiegelung)
                                          │
comdare-prt-art (separates Repo) ─── prt_art/ (volle Impl + Tests)
                                          │
                                          ↓ submodule
                            external/comdare-cache-engine/ (Werkzeug)
```

---

## 6. Heutige Änderungen (zum priorisierten Review)

### HIGH PRIORITY (echte funktionale Codeänderungen)

| Datei | Kritikalität |
|---|---|
| `cache_engine/builder/main.cpp` Phase 4-7 | **HOCH** — voller E2E-Pipeline-Wire-up |
| `cache_engine/builder/codegen/codegen.cpp` Aggregator + dllexport | **HOCH** — Cross-Platform-Codegen |
| `cache_engine/builder/module_loader/{hpp,cpp}` | **HOCH** — Cross-Platform DLL-Loading |
| `succinct/include/comdare/succinct/{bit_vector,louds}.hpp` | MITTEL — SDSL-Lite-Ersatz |
| `cache_engine/reclamation/rcu_reclaim/rcu.hpp` | MITTEL — liburcu-Ersatz |
| `cache_engine/include/cache_engine/hbm/hbm_hierarchy.hpp` | MITTEL — Abstract Factory |
| `prt_art/legacy_reimpl/P{11..27}/include/*.hpp` | MITTEL — 14 Algorithmen, je 1 file |

### MEDIUM PRIORITY (Tools)

| Datei | Kritikalität |
|---|---|
| `tools/ycsb_cli/main.cpp` | MITTEL — Standalone-CLI |
| `tools/latex_anhang/main.cpp` | MITTEL — CSV-Parser + LaTeX-Writer |
| `tools/latex_toolchain/latex_toolchain.cmake` | NIEDRIG — CMake-Skript |

### LOW PRIORITY (Tests, Doc, Config)

- Alle 16 neuen `tests/unit/test_*.cpp`
- `docs/quality_audit/HABICH_H2_CODE_QUALITY_2026_05_13.md`
- `RELEASE_PREP_2026_05_13.md`
- `.idea/`-Konfiguration

---

## 7. Bekannte Schwachstellen / TODO

1. **Mock-Modul-Bodies:** Die 54 generierten `comdare_perm_<fp>.dll` haben
   leere `run_workload` (setzt nur Defaults). Phase 8 ersetzt durch echte
   Permutations-spezifische Implementation.
2. **`comdare_cache_engine_v1*` nicht verdrahtet:** `create_instance(nullptr)`
   im aktuellen Code. Sollte Shared-CacheEngine-Service liefern.
3. **CMake gtest_discover_tests MSB3073-Warnings:** kosmetisch, EXEs gebaut +
   laufen direkt. Phase 8 erwägt `--no-pretty-values`-Workaround.
4. **Pfad `dll_dir/Debug`** in main.cpp hardcoded — Single-Config-Generator
   (Ninja/Make) findet DLLs unter `build_dir/` direkt.

---

## 8. Build-Verifikation

```bash
cd comdare-cache-engine
cmake -B build-msvc                           # MSVC VS 17 2022
cmake --build build-msvc --config Debug
cd build-msvc && ctest -C Debug --output-on-failure
# → alle Tests grün

# E2E-Builder (54 Permutationen):
./build-msvc/cache_engine/builder/Debug/comdare-cache-engine-builder.exe \
    ./cache_engine/builder/example_configs \
    ./_runs/test \
    --comdare-root=$(pwd)
# → 54/54 DLLs gebaut, 54 measurement_records exportiert
```

CLion: `Open Project` auf Repo-Root → 4 CMake-Profile erkannt:
**Debug**, **Release**, **MSVC-Debug** (build-msvc), **Permutations**.
RunConfigurations: 7 vorkonfiguriert.

---

## 9. Querverweis zu Delta-Dokumenten

Die Detail-Begründungen pro Aufgabe stehen in der **Diplomarbeit** (separates
Repo `probst-Diplomarbeit-cache-engine`):

- `20260508 Termin 7/Phase5_UML_Detail/25_…_hybrid_search_engine_…md`
- `26_…_legacy_reimpl_…md`
- `27_…_ycsb_cli_…md`
- `28_…_cmake_pipeline_…md`
- `29_…_phase4_7_loader_…md`
