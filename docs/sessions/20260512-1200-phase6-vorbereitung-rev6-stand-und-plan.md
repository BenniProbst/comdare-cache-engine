# 2026-05-12 12:00 — Phase 6 Vorbereitung: REV 6 Stand und Plan

**Session-Typ:** Architektur-Konsolidierungs-Session (Phase-5-Abschluss + Phase-6-Vorbereitung)
**Hauptdokumente Referenz:** `Diplomarbeit - Datenbanken/20260508 Termin 7/Phase5_UML_Detail/` (REV 1-6 Architektur-Skizzen + drawios)
**Stand:** REV 6 in Praezisierungs-Korrektur (PRT-ART als Erweiterung, NICHT Ersatz der 6 Node-Typen)

---

## 1. Architektur-Stand (REV 1 -> REV 6)

### 1.1 Iterations-Historie

| REV | Datum | Hauptbeitrag | drawio-Tabs |
|-----|-------|--------------|-------------|
| REV 1-2 | 2026-05-08 / 2026-05-09 | Erste UML + ER-Modell + Habich-H1 Detail-Anforderung | 8 |
| REV 3 | 2026-05-10 | Kanonische Architektur-Skizze (12 Tabs), F1-F15-Beschluesse, Korrektur-Runde 3 K3.1-K3.4 | 12 |
| REV 4 | 2026-05-10 | Future-Proof-Prinzipien (Open/Closed, Modular, Encapsulation, SRP, SoC, API-First, DSM), Layer-Matrix-Hybrid | 13 |
| REV 5 | 2026-05-11 | Termin 1-6 Integration: 7-Quellen-Hybrid, ISearchEngine-Familien S1-S30, ICacheEngine-Familien C1-C12, Visitor+State+Pipeline | 22 |
| REV 5.1 | 2026-05-11 | 3-Schichten-Trennung (Konzept/Strategie/Physisch), INode-Korrektur (Slot-Eintrag), Facade-Pattern fuer Interpreter, Cache-Lines-Plattform-spezifisch (NIE 64 B hardcoded), ICacheEngine-Familien-Taxonomie K15+U09 | 24 |
| REV 5.2 | 2026-05-11 | U09 als klassisches UML mit State-Pattern (PressureState als std::variant) + Visitor-Pattern + Pipeline-Mediator + 12 Sub-Engines | 24 |
| REV 5.3 | 2026-05-11 | K07 + K08 fein gesplittet (K07a-d Discover/Measure/Classify/Concepts + K08a-d Cache/CPU/Bus/Live) | 32 |
| **REV 6** | **2026-05-11** | **PRT-ART-Praezisierung** aus 35 GPT-Antworten + 3 Fallbeispielen — als **chirurgische Ergaenzung** (NICHT Ersetzung) der bestehenden Cache-Engine-Planung | 40 |

### 1.2 Was die Cache-Engine ist (zentral)

Die `comdare-cache-engine` ist das **universelle Forschungs-Framework** der Diplomarbeit. Sie steht NICHT im Schatten eines einzelnen Algorithmus, sondern bietet:

1. **Permutations-Maschine** ueber **alle** Suchalgorithmen aus den 33 Papern (D14-Baselines)
2. **State-Pattern** ueber den aktuellen Plattform-Zustand (`PressureState` mit 5 Zustaenden Idle/Warmup/Saturated/CoherenceStorm/Recovery)
3. **Visitor-Pattern** zur Bedienung von ISearchPageStrategy-Anfragen mit `CacheRecommendation` als Rueckgabe
4. **Pipeline-Mediator** ueber 12 interne Sub-Engine-Familien C1-C12 (Layout/Pinning/Prefetch/Coherence/Telemetry/Allocation/Migration/Encoding/Heuristik/Topologie/Scheduler/Filter)
5. **Adaptive Plattform-Erkennung** ueber `IPlatformProbe` Auto-Discovery (5-Schritt Discover/Measure/Classify/Publish/Bind)
6. **Cross-Permutationen** Saeule A (ISearchEngine-Familien S1-S30) x Saeule B (ICacheEngine-Familien C1-C12) x Engine-Choice (V1-V4)

### 1.3 PRT-ART im richtigen Verhaeltnis zur Cache-Engine

**WICHTIGE KLARSTELLUNG (User-Direktive 2026-05-12):**

PRT-ART ist:
- Eine **eigene Idee fuer einen Suchalgorithmus** (Permutations-Trie-ART)
- **Nicht** ein eigenstaendiger Anwendungsfall, sondern ein **konkretes weiteres Beispiel**, das die 33-Paper-Baselines **erweitert**
- Liefert je **Suchalgorithmus-Baustein-Familie** (Allokatoren, Fanouts, Node-Typen, Konzepte) **experimentelle Permutations-Optionen**
- Diese Optionen werden im **gleichen Permutations-Setup** der Cache-Engine getestet wie die existierenden Algorithmen aus dem Stand der Technik

Die Cache-Engine ist die **universelle Forschungs-Schicht**. PRT-ART zeigt:
1. **Erweiterbarkeit des Systems** — neue Algorithmen koennen problemlos hinzugefuegt werden
2. **Vergleichbarkeit** — PRT-ART kann gegen alle 33 Paper-Baselines unter denselben Bedingungen gemessen werden
3. **Acceleration-Mechanismus** — die Cache-Engine beschleunigt PRT-ART wie jeden anderen Algorithmus

### 1.4 KORREKTUR (2026-05-12) — Falsche Praezisierung in REV 6

Die ursprueliche REV-6-Sektion §5.2 hatte die "6 Node-Typen aus Termin 4" durch "2 PRT-ART-spezifische Node-Typen (Redirect + B+)" **ersetzt**. Das war **FALSCH** in der allgemeinen Cache-Engine-Perspektive:

| Aussage | Korrektheit |
|---------|-------------|
| "PRT-ART hat 2 Node-Typen" (im Spezifik-Sinne) | KORREKT — fuer den PRT-ART selbst |
| "Die Cache-Engine hat 2 Node-Typen" (im Generellen-Sinne) | **FALSCH** — sie muss alle 6+ Node-Typen aus Termin 4 als permutierbare Baselines unterstuetzen |

**Korrektur-Aktion:** REV 6 md §5.2/§5.3 werden zurueckgesetzt auf den REV-5-Stand (6 Seitentypen mit P0/P1/P2-Prioritaet aus Termin 4 Scope-Freeze). Die PRT-ART-spezifischen 2 Node-Typen werden als **hinterer Anhang** (NEU §5.17-§5.24) angefuegt, klar markiert als "PRT-ART-spezifische Erweiterung der allgemeinen Baseline-Familie".

Genauso wird **K05b** im drawio umgestaltet von "Korrektur" auf "PRT-ART-Erweiterung". Die anderen 7 neuen Tabs K05c-K05h + U10 bleiben unveraendert (sie ergaenzen ohnehin nur).

---

## 2. Architektur-Komponenten-Uebersicht

### 2.1 Drei-Ebenen-Architektur (REV 5 K01)

```
EBENE 1 - IExecutingEngine
   Wurzel-Abstraktion fuer alle Engines, die CacheEngine konsumieren
   ISearchEngine (Diplomarbeit-Fokus - alle Baselines D14)
   IFutureEngine (Architektur-Slot fuer ICompactingEngine, ISortingEngine, etc.)
                                  ▼ konsumiert
EBENE 2 - Suchalgorithmus-Datenstruktur (Saeule A)
   IPage / IRootPage / IFanout / INode / ISearchPage / ISearchPageStructure /
   ISearchPageStructureInterpreter (Facade-Singleton) / ICachePage / ICacheStrategy /
   ISearchPagesStrategy / ISearchPagesStrategyPattern / ISearchPageStrategy /
   IStrategyCommand / HybridCompositionCommand / 3 Pflicht-Iteratoren
                                  ▼ optimiert via
EBENE 3 - CacheEngine + Plattform-Modell (Saeule B)
   12 Sub-Engine-Familien C1-C12 (Layout/Pinning/Prefetch/Coherence/Telemetry/
   Allocation/Migration/Encoding/Heuristik/Topologie/Scheduler/Filter)
   PressureState-Pattern (5 Zustaende) + Visitor-Pipeline-Mediator
   IPlatformProbe Auto-Discovery (5-Schritt) + PlatformPropertySet (19 Properties)
```

### 2.2 ISearchEngine-Familien S1-S30 (REV 5 K06)

**21 Patterns (Ebene A):**
- S1-S10: Komposition (AllPagesUniform, LayerMix, InnerVsLeaf, HotVsCold, DynamicRebalance, HierarchicalFractal, OrthogonalRuntimeParametrized, MultiStrategyOrchestration, HeterogeneousAdaptive, BoundaryNodePartition)
- S11-S16: Adaptivitaet (OfflineSelfTuning, OnlineProbabilityRebalance, OnEveryStructureModification, PeriodicGlobalRelocation, BulkLoadLevelByLevel, CompoundFastBuild)
- S17-S21: Aufbauverfahren (BfsLevelByLevelPagePacking, RecursiveDfsBlockPacking, ProbabilityGreedyPagePacking, PostOrderDp, RecursiveCommonPrefixDecomposition)

**5 Plural-Familien (Ebene B):** S22 BPlus (12) / S23 Trie (8) / S24 Hybrid (8) / S25 Layout-Theorie (14) / S26 Adaptive-Layout-Selectors (3)

**4 Singular-Cluster (Ebene C):** S27 Intra-Page Search (22) / S28 Insert/Update (15) / S29 Layout/Encoding (28) / S30 Concurrency (9)

**Plus 17 + 5 = 22 HybridCompositionCommands** zur Aufloesung monolithischer Hybride

### 2.3 ICacheEngine-Familien C1-C12 (REV 5.1 K15)

| ID | Familie | Atoms |
|----|---------|-------|
| C1 | Cache-Layout-Engine | ~26 |
| C2 | Cache-Pinning-Engine | ~10 |
| C3 | Cache-Prefetch-Engine | ~25 |
| C4 | Cache-Coherence-Engine | ~12 |
| C5 | Cache-Telemetry-Engine | ~22 |
| C6 | Cache-Allocation-Engine | ~17 |
| C7 | Cache-Migration-Engine | ~12 |
| C8 | Cache-Encoding-Engine | ~28 |
| C9 | Cache-Heuristik-Engine | ~73 (orthogonal) |
| C10 | Cache-Topologie-Engine | ~17 |
| C11 | Cache-Scheduler-Engine | ~13 |
| C12 | Cache-Filter-Engine | ~9 |

**Gesamt:** ~264 atomare Bausteine + ~30 Hybrid-Composition-Commands

### 2.4 6 Pflicht-Seitentypen (Termin 4 Scope-Freeze, KORRIGIERT 2026-05-12)

Alle 6 Seitentypen bleiben PERMUTIERBAR in der Cache-Engine als Baseline-Familie:

| Klasse | Literaturanker | Prioritaet | ISearchPageStructure | Interpreter |
|--------|----------------|-----------|---------------------|-------------|
| Redirect-Knoten | CoCo-trie | P0 | RedirectStructure | RedirectInterpreter |
| Dense-Byte-Page | ART | P0 | DenseByteStructure | DenseByteInterpreter |
| Sparse-Patricia-Page | HOT | P0/P1 | SparsePatriciaStructure | PatriciaInterpreter |
| Multilevel-Dense-Page | START | P1 | MultilevelDenseStructure | MultilevelInterpreter |
| Decision-Span-Page | B^2-Tree | P2 | DecisionSpanStructure | B2Interpreter |
| Custom-Aligned-Page | PRT-ART intern | P2 | CustomAlignedStructure | CustomInterpreter |

**WICHTIG:** Diese 6 Seitentypen sind die **gemeinsame Baseline-Famile** fuer ALLE Algorithmen. Die Cache-Engine permutiert sie als Achse D5 (Seitentyp-Scheduler) in F15.

### 2.5 PRT-ART-Erweiterung (NEU 2026-05-11, KORRIGIERT als Anhang 2026-05-12)

PRT-ART liefert je Baustein-Familie **zusaetzliche** Permutations-Optionen:

| Baustein-Familie | PRT-ART-Erweiterung | Permutationsachse |
|------------------|---------------------|-------------------|
| Node-Typ-Variante | 2 Node-Typen-Mix (Redirect + B+) mit dynamischem Layout | D5 erweitert |
| Internes B+ Layout | 4 Suchtypen (Array[256] / Array[65535] / Vector<u8,u8> / Vector<u16,u16>) mit Dichte-Schwellen 25/50/75% | D10 NEU |
| Virtuelle Adressierung | TLB-inspired Offset-Berechnung statt Pointer-Chasing | D11 NEU |
| Value-Buffer | Linearer Append-only mit Lazy Deletion | C6 Allocation-Familie ergaenzt |
| Allokator-Pools | 4+2 spezielle Pools (A/B/C/D + R + V-static/V-dynamic) | C6 erweitert |
| Serialisierung | Signaling-Bits-Sparse-Length-Serialisierung | C8 Encoding ergaenzt |
| Concurrency | OLC + reservierte Value-Bloecke pro Writer + parallele Index-Updates auf disjunkten Nodes | S30 erweitert + C4 ergaenzt |
| Compile-Time-Metaprogrammierung | 11 Template-Parameter (Serializer, 6 Pools, TransitionPolicy, Fingerprint, Concurrency) | D12+D13 NEU |

**Forschungs-Aussage:** PRT-ART ist der **Test-Algorithmus** zum Beweis, dass die Cache-Engine als universelles Framework neue Algorithmen aufnehmen und gegen Bestehende messen kann.

---

## 3. Stand des comdare-cache-engine Repos

### 3.1 Was existiert (Phase 4.B abgeschlossen)

- 56 INTERFACE-Library-Stubs in den 7 Domaenen-Subdirectories
- 12 geklonte Original-Repos in `ext/` (P01, P02, P03, P04, P05, P07, P10, P20, P25, P29, P30 + P06)
- 14 LEGACY_REIMPL-Skelette in `prt_art/legacy_reimpl/` (P11-P14, P16-P19, P21-P24, P26, P27)
- Lizenz-Doku komplett (NOTICE, LICENSE, docs/lizenzen/)
- Master CMakeLists.txt (C++23, CMake 3.28+)
- 5 Python-Setup-Skripte (one-shot, OK gemaess F-EXTRA-5)
- `.gitmodules.template` fuer 6 COMDARE-Module (wartet auf Cluster-GitLab)

### 3.2 Was fehlt (Phase 6 Implementation)

| Bereich | Status | Aktion |
|---------|--------|--------|
| Concept-Header REV 5.3 (PressureState, ICacheEngine, ISubEngine, etc.) | **Liegen in Diplomarbeit/code/cache_engine/include/** | Migration in `cache_engine/include/cache_engine/concepts/` mit Namespace-Refactor `prt_art::cache_engine::*` -> `comdare::cache_engine::*` |
| Concept-Header REV 5.3 PRT-ART | **Liegen in Diplomarbeit/code/prt_art/include/** | Migration in `prt_art/include/prt_art/concepts/` |
| Multi-OS-CMake-Module (4 Stueck) | **Liegen in Diplomarbeit/code/cmake/** | Migration in `cmake/` mit `PRT_ART_*` -> `COMDARE_*` Refactor |
| F-EXTRA-5 Verletzung | `tools/permutation_codegen/codegen.py` ruft Python im Build auf | Umbau auf reine CMake-Funktion + sh/bat Helper |
| 5 fehlende Sub-Engine-Slots | C02 Pinning, C04 Coherence, C08 Encoding, C09 Heuristik, C10 Topologie, C12 Filter fehlen als Verzeichnisse unter `cache_engine/subsystems/` | NEU anlegen + 5 bestehende umbenennen (C03/C05/C06/C07/C11) |
| Echte Implementierung der 12 Sub-Engines | nichts vorhanden | Phase 6 Inkrement |
| Echte Implementierung der 6 Seitentypen | nichts vorhanden | Phase 6 Inkrement |
| Echte Implementierung PRT-ART-Erweiterung | nichts vorhanden | Phase 6 Inkrement (nach 6-Seitentypen-Baseline) |
| YCSB-Daten-Provider | nichts vorhanden | Phase 7 Vorbereitung |
| LaTeX-Toolchain | nichts vorhanden | Phase 8 (Habich H3) |

### 3.3 Wichtige Pfade

```
comdare-cache-engine/
├── README.md                                ✓ vorhanden
├── CMakeLists.txt                           ✓ vorhanden (Phase 4.B, ergaenzungsbeduerftig)
├── NOTICE / LICENSE                         ✓ vorhanden
├── .gitmodules.template                     ✓ vorhanden
├── cmake/
│   ├── check_submodules.cmake               ✓ vorhanden
│   ├── permutations.cmake                   ✗ F-EXTRA-5 Verletzung, umbauen
│   ├── platform_detection.cmake             ✗ FEHLT (aus Diplomarbeit/code migrieren)
│   ├── compiler_flags.cmake                 ✗ FEHLT (aus Diplomarbeit/code migrieren)
│   ├── isa_features.cmake                   ✗ FEHLT (aus Diplomarbeit/code migrieren)
│   └── gtest_setup.cmake                    ✗ FEHLT (aus Diplomarbeit/code migrieren)
├── docs/
│   ├── architecture/                        ✓ vorhanden
│   ├── architekturentscheidungen/           ✓ leer-Slot
│   ├── bausteine/                           ✓ leer-Slot
│   ├── domaenenmodell/                      ✓ leer-Slot
│   ├── glossare/                            ✓ leer-Slot
│   ├── email/                               ✓ vorhanden
│   ├── lizenzen/                            ✓ vorhanden
│   ├── status/                              ✓ vorhanden
│   └── sessions/                            ★ NEU 2026-05-12 (diese Datei)
├── cache_engine/                            ✓ 31 Sub-Komponenten-Stubs
│   ├── builder/                             (15 Sub-Komponenten)
│   ├── concurrency_manager/                 (8)
│   ├── reclamation/                         (1)
│   └── subsystems/                          (7 Sub-Engines vorhanden, 5 fehlen)
├── search_engine/                           ✓ 8 Sub-Komponenten-Stubs
├── measurement/                             ✓ 7 Sub-Komponenten-Stubs
├── hardware_isa/                            ✓ 4 Sub-Komponenten-Stubs
├── engine_choice/                           ✓ Top-Level CMakeLists
├── prt_art/                                 ✓ 9 Sub-Komponenten + 14 LEGACY_REIMPL
├── adapters/                                ✓ 11 Paper-Verzeichnisse (leer, F-EXTRA-1 Pflicht)
├── benchmarks/                              ✓ 3 Sub-Komponenten-Stubs
├── tests/                                   ✓ 4 Sub-Komponenten-Stubs
├── tools/                                   ✓ 5 Tool-Skelette (codegen.py F-EXTRA-5 Verletzung)
├── ext/                                     ✓ 12 geklonte Original-Repos
├── modules/                                 ✓ leer (Wartet auf GitLab)
└── datasets/                                ✓ leer (Phase 4.A pending)
```

---

## 4. Beziehung zu den 3 Projekten

| Projekt | Pfad | Zweck | Beziehung zu cache-engine |
|---------|------|-------|---------------------------|
| **comdare-cache-engine** | `Projekte/Research/comdare-cache-engine/` | **Universelles Framework** (Diese Doku, Hauptprojekt) | Code + Tests + Doku |
| comdare-prt-art | `Projekte/Research/comdare-prt-art/` | PRT-ART als zukuenftige Grundlage fuer comdare-db | Aktuell nur GPT-Diskussions-Archiv; Phase 8+ ggf. Separation |
| Diplomarbeit - Datenbanken | `Diplomarbeit - Datenbanken/` | NUR die Diplomarbeit selbst (LaTeX-Textbausteine + Diplomarbeit-spezifischer Glue-Code) | Konsument des cache-engine-Outputs (Phase 8 LaTeX-Anhang) |

### 4.1 Workflow zwischen den 3 Projekten

```
1. comdare-cache-engine (Code + Tests + Phase 7 Experimente)
       |
       | Phase 7 MeasurementBuffer-Disk-Dump
       v
2. Diplomarbeit - Datenbanken (Phase 8 LaTeX-Anhang)
       |
       | spaetere Verwendung
       v
3. comdare-prt-art (Phase 8+ Separation als comdare-db-Grundlage)
```

### 4.2 Was in der cache-engine programmiert wird

- 6 Seitentyp-Implementierungen (Redirect, Dense-ART, Sparse-HOT, Multilevel-START, Decision-Span-B^2, Custom-Aligned)
- 12 Sub-Engine-Familien C1-C12
- PressureState-Pattern + Visitor-Pipeline
- IPlatformProbe Auto-Discovery
- PRT-ART als zusaetzliche permutierbare Variante (im `prt_art/`-Unterverzeichnis des cache-engine-Repos)
- Adapter zu 12 ext-Repos (Habich-F-EXTRA-1)
- LEGACY_REIMPL fuer 14 Paper ohne Original-Code
- MeasurementBuffer + Permutations-Loop + Disk-Dump
- Multi-OS-CMake-Setup (Windows i7-1270P / Linux / macOS / ARM / RISC-V)
- GoogleTest 1.15.2 ueber FetchContent (KEINE Submodules)
- KEIN Python in Build-Pipeline (F-EXTRA-5)

### 4.3 Was NICHT in der cache-engine programmiert wird

- LaTeX-Textbausteine der Diplomarbeit (-> `Diplomarbeit - Datenbanken/latex/`)
- Diplomarbeit-spezifischer Glue-Code (-> `Diplomarbeit - Datenbanken/code/`)
- Phase-8 LaTeX-Anhang-Generator (-> nutzt cache-engine output, lebt aber in Diplomarbeit)
- comdare-db-Schnittstellen (-> spaetere Phase, vermutlich `comdare-prt-art` oder eigenes Repo)

---

## 5. Architektur-Highlights aus REV 5.3 + REV 6

### 5.1 PressureState als std::variant (REV 5.2)

```cpp
namespace comdare::cache_engine::state {
    struct Idle           { uint64_t last_check_ns; };
    struct Warmup         { double mpki; size_t pages_loaded; };
    struct Saturated      { double bandwidth_util; uint32_t mshr_pressure; };
    struct CoherenceStorm { uint32_t inter_socket_msgs_per_us; uint8_t affected_sockets; };
    struct Recovery       { std::chrono::milliseconds remaining; };

    using PressureState = std::variant<Idle, Warmup, Saturated, CoherenceStorm, Recovery>;
}
```

**Begruendung:** Modern-C++23-Variante mit Compile-Time-Exhaustiveness, value-Semantik, keine vtable-Hierarchie. Praezedenz: Iglberger Guideline 18 (Acyclic Visitor performt schlechter als Variant-Visit).

### 5.2 ICacheEngine als Visitor + State + Pipeline (REV 5.2 U09)

```cpp
namespace comdare::cache_engine {
    class ICacheEngine {
    public:
        virtual CacheRecommendation advise(const RequestContext& ctx) = 0;
        virtual PlatformSnapshot snapshot() const = 0;
        virtual void notify(const SubEngineEvent& event) = 0;
        virtual void register_sub_engine(SubEngineSlot slot,
                                        std::unique_ptr<ISubEngine> engine) = 0;
        virtual void on_pressure_transition(const state::PressureState& from,
                                           const state::PressureState& to) = 0;
    };
}
```

**Pipeline (deterministische Reihenfolge):**
```
1. C05 Telemetry    -> 2. C10 Topologie    -> 3. C09 Heuristik
4. C01 Layout       -> 5. C08 Encoding     -> 6. C06 Allocation
7. C02 Pinning      -> 8. C07 Migration    -> 9. C03 Prefetch
10. C04 Coherence   -> 11. C12 Filter      -> 12. C11 Scheduler
```

### 5.3 PermutationEngine fehlt noch (NEU IDENTIFIZIERT 2026-05-12)

User-Direktive: "Jede ExecutionEngine muss durch eine PermutationEngine fuer die Feature Permutation, gesteuert durch die CacheEngineBuilder, gesteuert werden, aber das ist noch nicht sichtbar und fuer den speziellen Fall des PRT_ART noch nicht gezeigt."

**Konsequenz:** REV 6 wird um die `IPermutationEngine`-Modellierung erweitert (U10 ergaenzt).

```
IExecutingEngine        <--konfiguriert durch--   CacheEngineBuilder
       |                                                |
       |                                                | steuert via
       v                                                v
ISearchEngine           <--steuert Permutationen--  IPermutationEngine
       |                                                |
       v                                                v
ISearchPagesStrategy ...               4-Ebenen-Permutationen + Engine-Choice-Varianten
```

Die `IPermutationEngine` wird im `cache_engine/builder/permutation_engine/`-Slot implementiert (existiert bereits als CMakeLists-Stub).

---

## 6. Phase-6-Plan (Naechste Schritte)

### 6.1 Schritt 0 — REV 6 Korrektur (heute 2026-05-12)

- [x] Diese Session-Doku
- [ ] REV 6 md korrigieren: §5.2/§5.3 zurueck auf 6 Seitentypen; PRT-ART-Spezifika als NEU §5.17+ Anhang
- [ ] drawio REV 6 korrigieren: K05b umgestalten als "PRT-ART-Erweiterung" statt "Korrektur"
- [ ] U10 erweitern um `IPermutationEngine` + Beziehung zu CacheEngineBuilder

### 6.2 Schritt 1 — Multi-OS-CMake-Migration

- [ ] `cmake/platform_detection.cmake` aus Diplomarbeit/code migrieren (Refactor `PRT_ART_*` -> `COMDARE_*`)
- [ ] `cmake/compiler_flags.cmake` migrieren
- [ ] `cmake/isa_features.cmake` migrieren
- [ ] `cmake/gtest_setup.cmake` migrieren
- [ ] Master `CMakeLists.txt` ergaenzen um `include(cmake/platform_detection)` etc.
- [ ] Lokal i7-1270P MSVC Configure-Test

### 6.3 Schritt 2 — Concept-Header-Migration (REV 5.3 + REV 6)

- [ ] `cache_engine/include/cache_engine/concepts/` anlegen + 8 Header migrieren mit Namespace-Refactor:
  - `pressure_state.hpp`, `i_cache_engine.hpp`, `i_sub_engine.hpp`, `cache_recommendation.hpp`, `platform_snapshot.hpp`, `request_context.hpp`, `i_cache_engine_visitor.hpp` (NEU), `cache_engine.hpp` (Aggregator)
- [ ] `cache_engine/include/cache_engine/platform_probe/cpuid_probe.hpp` migrieren
- [ ] `search_engine/include/search_engine/concepts/` anlegen + Concepts dorthin verschieben (analog REV 5.1 K02 3-Schichten-Audit)
- [ ] `prt_art/include/prt_art/concepts/` anlegen + 10 Header migrieren

### 6.4 Schritt 3 — 12 Sub-Engine-Slots vervollstaendigen

- [ ] `cache_engine/subsystems/c02_pinning_engine/` NEU anlegen
- [ ] `cache_engine/subsystems/c04_coherence_engine/` NEU anlegen
- [ ] `cache_engine/subsystems/c08_encoding_engine/` NEU anlegen
- [ ] `cache_engine/subsystems/c09_heuristik_engine/` NEU anlegen
- [ ] `cache_engine/subsystems/c10_topologie_engine/` NEU anlegen
- [ ] `cache_engine/subsystems/c12_filter_engine/` NEU anlegen
- [ ] 5 bestehende umbenennen: `prefetch_controller` -> `c03_prefetch_engine`, `telemetry_aggregator` -> `c05_telemetry_engine`, `allocator_manager` -> `c06_allocation_engine`, `relocation_manager` -> `c07_migration_engine`, `page_type_scheduler` -> `c11_scheduler_engine`
- [ ] `cache_engine/subsystems/c01_layout_engine/` ergaenzen (implizit in allocator_manager - separat machen)

### 6.5 Schritt 4 — F-EXTRA-5 Fix

- [ ] `tools/permutation_codegen/codegen.py` ersetzen durch CMake-Logik + sh/bat-Helper (synchron gepflegt)
- [ ] `cmake/permutations.cmake` umbauen: kein `find_package(Python3)` mehr

### 6.6 Schritt 5 — Tests-Migration

- [ ] 6 vorhandene Tests aus Diplomarbeit/code/{cache_engine,prt_art}/tests/ nach `tests/unit/` migrieren
- [ ] GoogleTest 1.15.2 ueber FetchContent setup verifizieren
- [ ] 3 kanonische PRT-ART-Fallbeispiele aus REV 6 K05h als Tests anlegen

### 6.7 Schritt 6 — Diplomarbeit/code aufraeumen

- [ ] `Diplomarbeit - Datenbanken/code/` nach `_archive_code_pre_migration/` archivieren
- [ ] Diplomarbeit-Verzeichnis enthaelt dann nur noch: 7 Termine + Phase5_UML_Detail + Forschungsarbeiten + Anmeldung-Dokumente + spaeter `latex/` (Phase 8)

### 6.8 Schritt 7 — Phase-6-Inkrementelle Implementation

Reihenfolge:
1. C10 Topologie-Engine (Auto-Discovery, IPlatformProbe vollstaendig)
2. C05 Telemetry-Engine (HwCounters-Lesen)
3. C09 Heuristik-Engine (Entscheidung)
4. C01 Layout-Engine (Geometrie)
5. C06 Allocation-Engine (Pool-Manager)
6. ... (Pipeline-Reihenfolge folgen)
7. 6 Seitentypen-Baseline-Implementierungen
8. PRT-ART-Erweiterung (2 Node-Typen + 4 Suchtypen)
9. Adapter zu 12 ext-Repos
10. LEGACY_REIMPL fuer 14 Paper

---

## 7. KRITISCHE Manoever / Pending

| Pri | Aufgabe | Status |
|-----|---------|--------|
| P0 | Email-Antworten 5 Anfragen (P06, P28, P31, P32, P33) | WARTEN |
| P0 | Cluster-Migration: Fortigate-31G + GitLab-Push | OFFEN (wartet auf Wochenende) |
| P1 | Habich H2 Code-Qualitaets-Bewertung pro Bausteine-Quelle | OFFEN |
| P1 | Habich H3 LaTeX-Toolchain | OFFEN (Phase 8) |
| P1 | Habich H4 Repo-Separation + GitHub-Push | OFFEN |
| P2 | Datasets Phase 4.A Implementation | OFFEN |
| P2 | Talos OS Java-Runtime-Provisioning fuer YCSB | OFFEN |
| P3 | Eigene RCU-Implementation (statt liburcu) | Phase 6+ |
| P3 | SDSL-Lite C++23-Portierung | Phase 6+ |
| P3 | HBM-Hybrid-Cache-Hierarchie | Phase 6+ |

---

## 8. Naechste Aktion

**JETZT (gleicher Tag):**
1. REV 6 md korrigieren — 6 Seitentypen wiederherstellen, PRT-ART hinten anhaengen
2. drawio REV 6 K05b umgestalten, U10 um IPermutationEngine ergaenzen
3. Anschliessend: Phase-6-Implementation-Migration starten (Schritte 1-6 oben)

**Nach Cluster-Freischaltung (Wochenende):**
- ZIH-Tests + Block-AO-Plattform-Vermessung
- GitLab-Push + COMDARE-Submodules-Init

---

## Ende der Session

Diese Session-Doku ist die kanonische Stand-zum-2026-05-12-Referenz fuer die Cache-Engine-Forschungsarbeit. Sie wird bei jeder kommenden Architektur-Aenderung als Bezugspunkt herangezogen.

Wortzahl: ca. 3000.
