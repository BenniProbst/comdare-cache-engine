# 2026-05-12 15:00 — Phase 6 INK-1 bis INK-8 Migration aus Termin 7 (REV 3+5+6)

**Session-Typ:** Inkrementelle Header-Skelett-Migration (8 Inkremente, autonom freigegeben)
**Vorgaenger:** `20260512-1300-phase6-migration-aus-diplomarbeit.md` (initiale 29 Dateien)
**Quelle:** Termin 7 / Phase5_UML_Detail (00-23) — vollstaendig eingelesen
**Ziel:** Termin-7-Konzept-Schichten in Header-Skelette + Tests ueberfuehren

---

## 1. Ausgangslage

Nach dem User-Hinweis vom 2026-05-12, dass die Termin-7-Historie (00-23) zwingend zu lesen sei,
wurde festgestellt, dass die initiale Migration (8 cache_engine + 11 prt_art Concept-Header)
nur die Basis-Schicht abdeckt. Die folgenden Termin-7-Konzept-Schichten fehlten:

- F10-K PermutationFlags (10 Banks inkl. telemetry_bank)
- F2 Event-Hierarchie + ObserverRegistry
- F-EXTRA-6 DecisionLambdaTrees (10+ Familien) + Bundle + Registry
- F6 ConcurrencyManager (10 Disziplinen + 3 Mechaniken)
- Achse 11 TelemetryStrategy (Kuehn 2026-05-09, 5 Konkretisierungen)
- REV 3 K3.2 IPlatformProbe (Auto-Discovery, generisch ohne CPU-Spezialisierung)
- REV 5 K07 28 fehlende Saeule-B-Concepts (RankSelect, BranchPredictor, Storage, etc.)
- F4 CacheHierarchyManager (generisch)
- Block AP CacheEngineMode (HEURISTIC_STATIC | INFORMED_KALIBRIERT | AUTOMATIC_ADAPTIVE)
- REV 5 K03 Permutationsdimension (4 Varianten)
- REV 3 K3.4 HybridCompositionCommand + 4 CompositionRules
- F5 InMemoryMeasurementBuffer + ThreadArena + 32-Byte MeasurementRecord
- F1 Measure<Cat,Detail> constexpr-Template + F-EXTRA-7 + F11

---

## 2. Inkrement-Plan (8 INK)

| INK | Thema | Header | Tests |
|-----|-------|--------|-------|
| INK-1 | PermutationFlags + Event + Observer | 3 | 23 |
| INK-2 | DecisionLambdaTrees (10+ Familien) | 13 | 29 |
| INK-3 | ConcurrencyManager (10+3) | 14 | 16 |
| INK-4 | TelemetryStrategy Achse 11 (5+1) | 7 | 15 |
| INK-5 | Plattform-Auto-Discovery (28 Concepts) | 11 | 22 |
| INK-6 | CacheHierarchy + LiveModel + Scheduler + Heuristik + Mode | 4 | 9 |
| INK-7 | HybridCompositionCommand (Command-Pattern) | 3 | 10 |
| INK-8 | InMemoryMeasurementBuffer + Measure-Matrix | 5 | 16 |
| **Summe** | | **60 neue Header** | **140+ neue Tests** |

---

## 3. Wichtige Architektur-Direktiven (REV 3+5)

- **REV 3 K3.2:** Plattform-Auto-Discovery statt CPU-Spezialisierung. KEINE `RyzenX3DProbe`,
  `IntelHybridProbe`, `X3DAwareFactory` etc. Stattdessen generische Properties + Heuristiken
  (`LargestL3CcdPinningHeuristic`, `HotPathOnHighIpcCoreHeuristic`).
- **REV 3 K3.4:** Hybrid-Familien IMMER als Command-Pattern aufgliedern (`HybridCompositionCommand`
  + `ICompositionRule` mit Sequential/Parallel/Conditional/Recursive).
- **REV 5 K02:** Pro `ISearchPage` gibt es eine `ISearchPageStructure` + `ISearchPageStructureInterpreter`
  (Invariante I4 aus Termin 2). Bereits in initialer Migration enthalten.
- **REV 5 K03:** `ICacheStrategy` ist eine eigene Permutationsdimension. `CacheEngineMode` mit
  4 Varianten (BASELINE_NO_ENGINE / HEURISTIC_STATIC / INFORMED_KALIBRIERT / AUTOMATIC_ADAPTIVE)
  steuert pro Search-Engine-Permutation 4 Builds (V1-V4) — F15-Vergleich.
- **REV 5 K07:** 28 fehlende Saeule-B-Concepts (RankSelect, BranchPredictor, BitManipulation,
  Storage, PageCache, ConcurrencyProtocol, Rebuild, WorkloadModel, Cell/WordProbeModel).
- **REV 5 K08:** Plattform-Modell mit NUMA-Awareness, TLB-Modell, NVRAM, 6-Flavor-RCU.

---

## 4. Verzeichnisstruktur

```
cache_engine/include/cache_engine/
├── concepts/
│   ├── permutation_flags.hpp                           (INK-1)
│   ├── event.hpp                                       (INK-1)
│   ├── i_observer.hpp                                  (INK-1)
│   ├── i_decision_lambda_tree.hpp                      (INK-2)
│   ├── decision_lambda_tree_bundle.hpp                 (INK-2)
│   ├── decision_lambda_tree_registry.hpp               (INK-2)
│   ├── decision_trees/                                 (INK-2)
│   │   ├── page_relocation_tree.hpp
│   │   ├── page_type_change_tree.hpp
│   │   ├── prefetch_adjustment_tree.hpp
│   │   ├── hot_path_recognition_tree.hpp
│   │   ├── allocator_rebalance_tree.hpp
│   │   ├── concurrency_discipline_switch_tree.hpp
│   │   ├── value_handle_selection_tree.hpp
│   │   ├── sampling_rate_adjustment_tree.hpp           (Block AM Kuehn)
│   │   ├── coherence_aware_write_decision_tree.hpp     (Block AN Kuehn)
│   │   └── cache_coherence_detection_tree.hpp          (Block AI Kuehn)
│   ├── i_concurrency_discipline.hpp                    (INK-3)
│   ├── i_concurrency_mechanic.hpp                      (INK-3)
│   ├── concurrency_manager.hpp                         (INK-3)
│   ├── disciplines/                                    (INK-3, 10 Disziplinen)
│   │   ├── page_discipline.hpp
│   │   ├── node_discipline.hpp
│   │   ├── array_discipline.hpp
│   │   ├── data_structure_discipline.hpp
│   │   ├── path_discipline.hpp
│   │   ├── memory_read_discipline.hpp
│   │   ├── memory_write_discipline.hpp                 (Block AN CacheCoherenceCost)
│   │   ├── memory_read_write_discipline.hpp
│   │   ├── simd_thread_discipline.hpp
│   │   └── simd_flow_discipline.hpp
│   ├── mechanics/                                      (INK-3, 3 Mechaniken)
│   │   ├── olc_mechanic.hpp                            (P01/P08)
│   │   ├── rowex_mechanic.hpp                          (P02/P08)
│   │   └── comdare_rcu_mechanic.hpp                    (P29 / Task #104)
│   ├── i_telemetry_strategy.hpp                        (INK-4 Achse 11)
│   ├── telemetry/                                      (INK-4, 6 Strategien)
│   │   ├── per_node_counter.hpp                        (P28 + WARNING Block AI)
│   │   ├── leaf_only_counter.hpp                       (Kuehn NEU 2026-05-08 Block AJ)
│   │   ├── leaf_only_sampled_counter.hpp               (Kuehn NEU 2026-05-08 Block AL)
│   │   ├── retroactive_aggregation.hpp                 (Kuehn NEU 2026-05-08 Block AK BARRIER)
│   │   ├── path_read_counter.hpp                       (P26 Zhang FGCS)
│   │   └── probability_hints_header.hpp                (PRT-ART eigen T4)
│   ├── cache_engine_mode.hpp                           (INK-6 Block AP)
│   ├── cache_hierarchy_manager.hpp                     (INK-6 F4 generisch)
│   ├── cache_engine.hpp / cache_recommendation.hpp / ...    (initial)
│   └── ...
├── platform/                                           (INK-5 + INK-6 — REV 3 K3.2)
│   ├── i_platform_probe.hpp                            (3 Top-Level + PropertySet)
│   ├── cache_topology.hpp                              (4 Concepts)
│   ├── core_layout.hpp                                 (4 Concepts, generisch)
│   ├── isa_features.hpp                                (2 Concepts)
│   ├── interconnect.hpp                                (3 Concepts)
│   ├── primitives.hpp                                  (3 Concepts: RankSelect, BranchPred, BitManip)
│   ├── storage.hpp                                     (2 Concepts: Storage, PageCache)
│   ├── concurrency_protocol.hpp                        (1 Concept + 6-Flavor-RCU)
│   ├── rebuild.hpp                                     (2 Concepts)
│   ├── workload_model.hpp                              (3 Concepts: Workload, Cell, Word)
│   ├── live_platform_model.hpp                         (2 Concepts)
│   ├── i_scheduler.hpp                                 (INK-6)
│   └── i_heuristic.hpp                                 (INK-6)
├── strategy_command/                                   (INK-7)
│   ├── i_strategy_command.hpp
│   ├── i_composition_rule.hpp                          (4 Konkretisierungen)
│   └── hybrid_composition_command.hpp
└── measurement/                                        (INK-8)
    ├── measurement_category.hpp                        (16 + 22 enum)
    ├── measurement_record.hpp                          (32-Byte aligned)
    ├── thread_arena.hpp                                (cache-line aligned)
    ├── in_memory_measurement_buffer.hpp                (F5)
    └── measure.hpp                                     (F1 + F-EXTRA-7 + F11)
```

---

## 5. Verifikation pro Inkrement

Jeder Inkrement-Schritt verlaeuft nach diesem Schema:
1. Concept-Header in `cache_engine/include/cache_engine/...` anlegen
2. Test-Datei in `tests/unit/test_<komponente>.cpp` anlegen
3. `tests/unit/CMakeLists.txt` mit `comdare_add_test(...)` erweitern
4. Configure: `cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64`
5. Build: `cmake --build build-msvc --config Debug --target <test_target>`
6. Tests: `ctest -C Debug` (aus `build-msvc/tests/unit/`)

Plattform: i7-1270P (Block-AO-Liste) + MSVC 19.39 + Debug + C++23.

---

## 6. Was diese Migration NICHT umfasst

- Konkrete CMake-Generator-Logik fuer Permutations-Builds (Phase 7)
- ABI-stabiles C++23-Modul-Interface (in 23_architektur_skizze_REV6 dokumentiert,
  noch nicht implementiert — F12 Builder-Lifecycle Phase 1-7)
- `CacheEngineBuilder` selbst (main.cpp) — Skelett im builder/CMakeLists.txt vorhanden
- 12 Sub-Engine-Slots (C02 pinning, C04 coherence, C08 encoding, C09 heuristik, C10 topologie,
  C12 filter — 6 fehlend, 6 bestehend mit Umbenennung c03_prefetch_engine, etc.)
- 6 Pflicht-Seitentypen (Redirect-CoCo, Dense-ART, Sparse-HOT, Multilevel-START,
  Decision-B², Custom-Aligned) — Stub-Header existieren, echte Impl. in PRT-ART-Phase
- 14 LEGACY_REIMPL Skelette (P11/P12/P13/P14/P16/P17/P18/P19/P21/P22/P23/P24/P26/P27)
- F-EXTRA-5 NO-PYTHON Codegen-Driver (urspr. `tools/permutation_codegen/codegen.py`
  + `cmake/permutations.cmake` Python-Aufruf — muss durch sh/bat ersetzt werden)
- 17 Hybrid-Aufloesungen aus 11_md §10 + 12_md §9 (Beispiele in Tests, Vollkatalog in
  Phase 7)
- Adapter-Skelette in `adapters/<paper>/` pro Repo
- 29 ICacheStrategy-Familien F1-F29 aus 11_md (nur Concept-Wurzel migriert,
  29 konkrete Familien-Header in Phase 7)
- 4-Ebenen-Strategien aus 12_md (~21 Patterns + ~41 Plural + ~74 Singular + ~80 Heuristiken)

---

## 7. Verifikations-Ergebnis i7-1270P MSVC Debug C++23 (Stand 2026-05-12 Abend)

```
183/183 tests passed (100%)
```

Aufschluesselung:
- 26 initial migrierte Tests (Cache-Engine + PRT-ART Concept-Header)
- 23 INK-1: PermutationFlags + Event + Observer
- 29 INK-2: DecisionLambdaTrees + Bundle + Registry
- 16 INK-3: ConcurrencyManager (10 Disziplinen + 3 Mechaniken)
- 15 INK-4: TelemetryStrategy Achse 11 Kuehn
- 22 INK-5: Plattform-Auto-Discovery (28 Saeule-B-Concepts)
-  9 INK-6: CacheHierarchy + Mode + Scheduler + Heuristik
- 10 INK-7: HybridCompositionCommand + 4 CompositionRules
- 16 INK-8: InMemoryMeasurementBuffer + Measure-Matrix + Hooks
-  8 Sub-Engines C01-C12 (12 Slots, semantische Namespaces)
-  9 6 Pflicht-Seitentypen (REV 5 K05) + 6 Singleton-Interpreter

---

## 8. Folge-Inkremente nach INK-1..INK-8

### 8.1 — 12 Sub-Engine-Slots C01-C12 (RBMM-Konvention)

Verzeichnisse mit `cNN_*_engine`-Praefix (Sortierreihenfolge), Namespaces semantisch
(User-Korrektur 2026-05-12 — `c01..c12` nicht nachvollziehbar). Mapping:

| Slot | Verzeichnis | Namespace |
|------|-------------|-----------|
| C01 | `c01_cost_engine`         | `cost`       |
| C02 | `c02_pinning_engine`      | `pinning`    |
| C03 | `c03_prefetch_engine`     | `prefetch`   |
| C04 | `c04_coherence_engine`    | `coherence`  |
| C05 | `c05_telemetry_engine`    | `telemetry`  |
| C06 | `c06_allocation_engine`   | `allocation` |
| C07 | `c07_migration_engine`    | `migration`  |
| C08 | `c08_encoding_engine`     | `encoding`   |
| C09 | `c09_heuristik_engine`    | `heuristik`  |
| C10 | `c10_topologie_engine`    | `topologie`  |
| C11 | `c11_scheduler_engine`    | `scheduler`  |
| C12 | `c12_filter_engine`       | `filter`     |

5 Umbenennungen (prefetch_controller → c03, etc.), 1 Re-Mapping (cost_model → c01),
6 neue Slots (c02, c04, c08, c09, c10, c12).

### 8.2 — F-EXTRA-5 NO-PYTHON-Direktive (H6)

`tools/permutation_codegen/codegen.py` entfernt. Drei synchron gepflegte Pendants:
- `codegen.cmake` (Standard via `cmake -P`)
- `codegen.sh` (POSIX shell)
- `codegen.bat` (Windows cmd)

`cmake/permutations.cmake` umgestellt — Backend per `COMDARE_PERMUTATION_CODEGEN_BACKEND`
waehlbar (Default `cmake`). Kein `find_package(Python3)` mehr.

### 8.3 — 6 Pflicht-Seitentypen (REV 5 K05)

Pro Pflicht-Seitentyp:
- `prt_art/include/prt_art/page_structures/<name>_structure.hpp` (ISearchPageStructure-Konkretisierung)
- `prt_art/include/prt_art/interpreters/<name>_interpreter.hpp` (Singleton-Facade)

| Seitentyp | Prio | Page-Structure | Interpreter |
|-----------|------|----------------|-------------|
| Redirect (CoCo)         | P0    | `RedirectStructure`        | `RedirectInterpreter`    |
| Dense-Byte (ART)        | P0    | `DenseByteStructure`       | `DenseByteInterpreter`   |
| Sparse-Patricia (HOT)   | P0/P1 | `SparsePatriciaStructure`  | `PatriciaInterpreter`    |
| Multilevel-Dense (START)| P1    | `MultilevelDenseStructure` | `MultilevelInterpreter`  |
| Decision-Span (B^2)     | P2    | `DecisionSpanStructure`    | `B2Interpreter`          |
| Custom-Aligned (PRT-ART)| P2    | `CustomAlignedStructure`   | `CustomInterpreter`      |

---

## 9. Naechste Schritte

1. PRT-ART-Erweiterung (2 Node-Typen + 4 internal search types)
2. Datasets Phase 4.A Implementation
3. CacheEngineBuilder main.cpp (F12 Lifecycle Phase 1-7)
4. ABI-stabiles Modul-Interface (POD structs + function pointers)
5. 14 LEGACY_REIMPL Skelette ausfuellen
6. 17 Hybrid-Aufloesungen aus 11_md §10 + 12_md §9 (Vollkatalog)
7. 29 ICacheStrategy-Familien F1-F29 als konkrete Header
8. 4-Ebenen-Strategien aus 12_md (~21 Patterns + ~41 Plural + ~74 Singular + ~80 Heuristiken)
9. Habich H2/H3/H4 Workitems
10. Eigene RCU-Impl (Task #104), SDSL-Lite-Portierung (Task #105), HBM-Hybrid (Task #106)
