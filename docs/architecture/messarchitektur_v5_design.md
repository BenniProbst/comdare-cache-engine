# Mess-Architektur V5 — Memento_all + Observer_all, 3 Profile, host-seitige Workloads, Konformitäts-Gate

> Status: Architektur-Spezifikation (bindend), 2026-05-31. Provenienz: Grounding-Workflow `wlw1w69eg` (5 IST-Explore + Synthese).
> Knüpft an den IST-Code der drei Repos an. **Anfassen, nicht neu erfinden.** Neu-Bausteine **[NEU]**, Annahmen **[ANNAHME]**.
> Bindende User-Entscheidungen: `messarchitektur_v5_entscheidungen.md`.
>
> Korrigiert ggü. V3 zwei Designfehler: (a) `IMeasurableWorkload`/Pfad A war fälschlich reine In-DLL-Last — Pfad A war
> **immer** host-seitig im Prüfdock geplant, wird **nicht** gestrichen, sondern host-seitig **relokalisiert**; (b) eine reine
> Lebewesen-Binary ist **nicht generisch** auslieferbar — das generische Mess-Interface gehört **host-seitig in die CacheEngineBuilder**.

---

## 1. Zielbild + ASCII-Diagramm der zwei Seiten

Genau **zwei Seiten**, getrennt durch **eine** ABI-Grenze (die `.dll`/`.so`-Modul-Grenze). Oberhalb: Builder-Binary (eine vtable
erlaubt, NICHT Hot-Path). Unterhalb: Lebewesen-Binary, compile-time-monomorphisiert (Hot-Path, kein `virtual`/`dynamic_cast`).

```
╔══════════════════════════════════════════════════════════════════════════════════════╗
║  HOST-SEITE  —  CacheEngineBuilder-Binary  (Builder-vtable erlaubt, NICHT Hot-Path)    ║
║   ┌───────────────────────────── Lebenszyklus je Lebewesen-Binary ──────────────────────┐  ║
║   │  import (dlopen/LoadLibrary)  →  KONFORMITÄTS-GATE  →  MESSEN  →  abstoßen       │  ║
║   └─────────────────────────────────────────────────────────────────────────────────┘  ║
║   PruefDockRegistry::select_for(handle)  ── pro GATTUNG genau 1 Dock ──┐               ║
║     pruef_dock_registry.hpp:28-33                                       │               ║
║     ┌──────────────────────────────────────────────────────────────────▼────────────┐ ║
║     │ IPruefDock (pruef_dock.hpp:54)  z.B. SearchAlgorithmDock (search_algorithm_dock │ ║
║     │   .hpp:22) — dünner Orchestrierungs-Wrapper, KEIN Neubau                        │ ║
║     │   (1) [NEU] Konformitäts-Gate gegen std::map  ── conformance_gate.hpp           │ ║
║     │   (2) IMeasurableWorkload-Host-Orchestrator  ── [NEU host-seitig] Pfad A relok. │ ║
║     │         · mehrere Lastprofile je Binary  (YCSB A/B/C/D/E/F × OP-1..6)           │ ║
║     │   (3) Zwei-Phasen-Op-Schleife  ── [NEU] drive_two_phase_op_loop                 │ ║
║     │         pro Op GENAU 2×:  memento-save-all → op → rollback-all → op-measure     │ ║
║     │   (4) Latenz-/Protokoll-State (host-only): ns-Vektoren + Op-Protokoll+Ergebnis  │ ║
║     │         tier_observe_trace_abi.hpp (Wall-Clock) + Op-Trace [NEU]                │ ║
║     └─────────────┬───────────────────────────────────────────────────┬─────────────┘ ║
║   Abstract-Factory-ähnlicher Handle-Übergang (ABI-stabil, POD + vtable des Sub-IF):    ║
║   AnatomyModuleHandle (anatomy_module_loader.hpp) ── liefert IAnatomyBase* ──┐         ║
╚═════════════════════════════════════════════════════════════════════════════╪═════════╝
                  ║  ABI-GRENZE (die einzige)                                   ║
                  ║  Übergang: IDriveableTier  [+ observer_all + memento_all NUR Messung-AN]
╔═════════════════╪═══════════════════════════════════════════════════════════╪═════════╗
║  LEBEWESEN-BINARY-SEITE  —  geladene .dll/.so  (compile-time monomorph, Hot-Path) ║         ║
║                                                                              ▼         ║
║   SearchAlgorithmAbiAdapter<A>  (abi_adapter.hpp:75-77)  — final, exportiert:          ║
║     ├─ IAnatomyBase           (Lifecycle: warm_up()/run(), abi_adapter.hpp:97-104)     ║
║     ├─ IDriveableTier         [NEU = Umbenennung des Antriebs-Teils von IObservableTier]║
║     │     tier_insert/lookup/erase/clear/size  (uint64-Key-Raum)                       ║
║     ├─ observer_all  [Sub-IF, nur Messung-AN einkompiliert]  ── tier_observe(POD V…)   ║
║     │     observe_all() (search_algorithm_anatomy.hpp:53-72) → ObserverAggregate       ║
║     └─ memento_all   [NEU Sub-IF, nur Messung-AN einkompiliert]  (Etikett "hybrider Visitor"  ║
║          NICHT umgesetzt: kein accept()/HostSink-Besuch; reines save/rollback-vtable)         ║
║           save_all()/rollback_all() über ALLE stateful Achsen (9 von 17)               ║
║   STATE LEBT HIER, NICHT HOST-SEITIG: search_organ_ · ComposedStore<N,L,A> · Disk      ║
║   RELEASE-DLL (Messung-AUS): observer_all+memento_all COMPILE-TIME ENTFERNT            ║
║     → reine funktional-getriebene Lebewesen-Binary OHNE Overhead, an Forschung auslieferbar ║
╚════════════════════════════════════════════════════════════════════════════════════════╝
```

**Asymmetrie des State (bindend):** Der Lebewesen-Binary-State lebt **in** der Binary (`search_organ_`, `ComposedStore`, Disk-Files),
nicht host-seitig. Der Host hält nur (a) Latenz-Mess-State (ns-Vektoren) + (b) Operations-Protokoll + Ergebnisse.

> **[G5-Audit-Korrektur, w289llo0o] „hybrider Visitor" ist NICHT umgesetzt — Etikett gestrichen.** Der real implementierte
> Pfad ist **kein** Visitor: `IRollbackableTier` (`rollbackable_tier.hpp`) exponiert nur zwei void-Methoden
> `tier_save_all()`/`tier_rollback_all()` als reine ABI-vtable; es gibt **kein** `accept()`, **keinen** `HostSink`/Element-Besuch
> und die Rollback-Klassen „besuchen" den Host nicht. Es ist ein klassisches **Memento** (Capture/Restore über die Modulgrenze),
> kein hybrider Visitor. Die folgenden „hybrider Visitor"-Erwähnungen sind als historisches Entwurfs-Etikett zu lesen, nicht als Ist.

---

## 2. Die DREI Profile (Build-Profil ⊥ Lastenprofil ⊥ Compile-Release-Profil)

Streng getrennte, orthogonale Dimensionen. Build-Profil ⊥ Lastenprofil = die zwei **Haupt-Experiment-Achsen**
(User/Thesis-geliefert). Compile-Release-Profil = nicht-experimentelle Schaltachse.

| Profil | Definition | Inhalt | Wo gesetzt | Wer liefert |
|---|---|---|---|---|
| **(1) BUILD-PROFIL (statisch)** | Parameter der zu erzeugenden Lebewesen-Binaries + Permutations-Gesetzmäßigkeiten interner+externer abstrakter Lebewesen | Achsen-Vendor aktiv; Cartesian-Expansion; Profile-Filter (smoke/medium/full); Codegen-Modus; Prüflinge | `CMakeLists.txt`, `cmake/permutations.cmake`, prt-art | Diplomarbeit/User (Achse 1) |
| **(2) LASTENPROFIL** | Testdaten UND Operationsabläufe + Pausen+Testzeiten, je Gattung über das ABI-stabile Interface; host-seitig generisch | YCSB A–F, record/operation_count, seed, OP-1..6, Pausen, Checkpoints | **Runtime host-seitig** `workload.hpp:34-40`, `tier_observe_trace_abi.hpp:32-42`, `op_type_filter.hpp` | Diplomarbeit/User (Achse 2) |
| **(3) COMPILE-RELEASE-PROFIL** | Build-Flags (Messung-bauen vs Release ohne), cmake-Config IN der Cache Engine, **DEFAULT = Messung eingebaut** | Master-Messschalter, ISA-Detection, STATIC/SHARED, Experiment-Mode | `CMakeLists.txt`, `cmake/comdare_add_library.cmake`, `cmake/anatomy_codegen.cmake` | Cache Engine selbst |

**Ablauf (bindend):** Profil baut **zuerst den HOST** (`COMDARE_BUILD_BUILDER=ON`) → Host permutiert eine Lebewesen-Binary-Config →
**baut zuerst alle DLLs** → **misst danach** (Build vor Mess-Phase, nicht verschränkt).

### 2.2 Alle gefundenen CMake-Flags → 3 Profile (Auszug; Vollliste im Workflow-Inventar)

- **(1) Build:** `COMDARE_BUILD_BUILDER` (CMakeLists:30, Pflicht zuerst), `_BENCHMARKS`/`_TESTS`/`_PERMUTATIONS` (31-33), `COMDARE_CE_PRUEFLINGE` (41, externe Lebewesen), `COMDARE_AXIS_06_ENABLE_*` (62-93, 23×), `_Q1_*` (97-116), `_Q2_*` (118-123), `_03A_*` (140-159, 17× #42), `_03B_*`/`_03M_*`/`_12_*` (162-171), `COMDARE_TARGET_ISA`/`_PERMUTATION_PROFILE`/`_MODE` (permutations.cmake:10-42), prt-art-Pendants, `COMDARE_V32_ENABLE`, `COMDARE_DA_BUILD_TESTS`.
- **(3) Compile-Release:** `COMDARE_CE_ENABLE_STATISTICS` (51-55, **heutiger Mess-Master-Annäherung**), `COMDARE_DETECTION_MODE` (23-25), `COMDARE_EXPERIMENT_MODE` (285-289), `COMDARE_BUILD_SHARED_LIBS` (comdare_add_library:20-21), `COMDARE_<PROJECT>_BUILD_SHARED_LIBS` (38-41), `COMDARE_LIBRARY_TYPE` (anatomy_codegen:12,55-61).
- **(sonstiges):** `COMDARE_PERMUTATION_CODEGEN_BACKEND`, `_USE_COMPILER_CACHE`, `_FETCHCONTENT_USER_CACHE`, `_QUIET_SUBMODULE_CHECK`, `_BUILD_LEGACY_REIMPL`/`_BUILD_DEPRECATED`.

### 2.3 NEU fehlende Flags (bindend zu schließen)

| **[NEU] Flag** | Profil | Begründung | Default |
|---|---|---|---|
| `COMDARE_MEASUREMENT_MODE` (ON/OFF) | (3) | **Master-Schalter Messung.** IST hat nur `CE_ENABLE_STATISTICS` (#ifdef), keinen funktional-only/Release-Modus. Bindet observer_all **+** memento_all ein/aus → `add_compile_definitions(COMDARE_MEASUREMENT_ON=1)`. | **ON** (Default = Messung eingebaut) |
| `COMDARE_RELEASE_MODE`/`_PRODUCTION_MODE` | (3) | Komplement: Release-DLL **ohne** Messung; aktiviert §7-Compile-Time-Entfernung. | OFF |
| `COMDARE_AXIS_05/10/14/FILTER/IO_DISPATCH/MIGRATION_ENABLE_*` | (1) | Vendor-Flags der übrigen **stateful** Achsen (§3) — Voraussetzung, dass memento_all über genau die einkompilierten Achsen läuft. | je ON |
| `COMDARE_AXIS_12_ENABLE_ARM64/_AARCH64` | (1) | IST nur GENERIC+X86_64 (170-171). | OFF (kein Cross) |

> **Lastenprofil hat absichtlich KEINE neuen CMake-Flags** — es ist host-seitig zur Laufzeit (Runtime-Matrix-Achse).

---

## 3. Memento_all-System (parallel zu observe_all)

**Grundsatz (bindend):** `memento_all` ist Lebewesen-Binary-**einkompiliert**, nur bei Messung-AN (zusammen mit der IO/memory-Achse).
Rollt den **gesamten** Zustand nach Warmup über **alle stateful Achsen** via einkompiliertes Memento-Pattern zurück + wiederholt
die Op. **Parallel** zu observe_all. (~~nach dem Observer-Pattern = **hybrider Visitor**: die Lebewesen-Binary-Rollback-Klassen
**besuchen den Host**~~ — **dieses „hybrider Visitor"-Etikett ist NICHT umgesetzt, gestrichen [G5-Audit w289llo0o]**: real ist es
ein **Memento** über zwei void-vtable-Methoden, ohne `accept()`/Host-Besuch.) **Einfacher Snapshot reicht NICHT**, weil
IO-Disk-Persistenz der Such-Algorithmen möglich ist (`ComdareTierObserverSnapshotV1` = nur uint64-Counter → unzureichend).

### 3.2 observe_all ↔ memento_all

| | observe_all (IST, teilweise) | memento_all **[NEU]** |
|---|---|---|
| Richtung | Host **zieht** POD (read-only) | Host **ruft** save/rollback-vtable (Memento; KEIN Host-Besuch — „hybrider Visitor" gestrichen, G5-Audit w289llo0o) |
| Zweck | Per-Achsen-Counter | Gesamt-State **speichern + zurückrollen** |
| Daten | uint64-Counter (in-process) | vollständiger Achsen-State **inkl. Disk** |
| Datei | observer_aggregate.hpp, observe_all() search_algorithm_anatomy.hpp:53-72 | **[NEU]** memento_aggregate.hpp, memento_all(), AnatomyMementoCommand |
| Gating | `COMDARE_CE_ENABLE_STATISTICS` | **[NEU]** `COMDARE_MEASUREMENT_MODE` |

### 3.3 Einheitliche Memento-API — `MementoAxis`-Concept [NEU]

Analog `ObservableAxis<A>` (⟺ „hat statistics()") ein **[NEU] `MementoAxis<A>`** (⟺ „hat save/restore"):
```cpp
template <class A>
concept MementoAxis = requires(A& a, typename A::memento_t m) {
    { a.save_state() }            -> std::same_as<typename A::memento_t>;  // capture
    { a.restore_state(m) }        -> std::same_as<void>;                   // rollback
    { a.memento_persist(/*io*/) } -> std::same_as<void>;                   // Disk-Sonderfall
};
```

**9 STATEFUL Achsen (Memento erforderlich) / 8 STATELESS (`EmptyMemento`-Fallback):**

| Achse | Stateful | Memento-API | Disk-Sonderfall |
|---|---|---|---|
| axis_03a search_algo | **JA** | Tree-Topologie+Belegung+Rebalancing; serialize_state | ART Node4/16/48/256 + Wormhole hash+trie → Typ-Discriminator |
| axis_04 node_type | **JA** | serialize_allocated_nodes, occupancy-Bitmaps, slot-Offsets | externe Pointer, rekursive Nodes |
| axis_06 allocator | **JA** | capture_free_list/serialize_arena (Offsets+occupancy) | **EXTREM SCHWER**: je Allokator andere Struktur, kein generisches Format |
| axis_08 concurrency (RCU/HP) | **JA** | capture_grace_period/hazard_lists | thread-verteilt, Epoch-Sync, Leak-Gefahr |
| axis_10 serialization | **JA** | capture_codec_state (Dictionary/LOUDS) | **ZENTRAL**: Disk-Persistierung selbst |
| axis_14 value_handle | **JA** | serialize_value_pool/version_epochs | MVCC-Generationen |
| axis_filter | **JA** | serialize_filter_bitmap/bucket | Cuckoo-Displacement-History |
| axis_io_dispatch | **JA** | flush_buffers + checkpoint_file_position | **ZENTRAL**: Disk-Interface (File-Pointer+Buffers) |
| axis_migration_policy | **JA** | capture_tier_placement + heat_metadata | RAM/NVM/SSD-Checkpoint-Konsistenz |
| 03b/03m/05/07/09/09b/11/12 | nein | `EmptyMemento` (Ausnahmen: HashLookup probe-chain, prefetch Stride-Kalibrierung) | — |

### 3.4 Hybrider Visitor (Memento-Klassen besuchen Host) — **[G5-Audit w289llo0o: NICHT umgesetzt, Entwurf verworfen]**

> **NICHT umgesetzt (Entwurf, kein Ist).** Das Modul `builder/algorithm_visitor/` ist **bis heute leer** (`.gitkeep`); ein
> `memento_visitor.hpp` mit `accept(HostSink&)`/Host-Besuch **existiert nicht** (grep-belegt). Der real gebaute Pfad ist ein
> **Memento** über `IRollbackableTier::tier_save_all()/tier_rollback_all()` (zwei void-vtable-Methoden, kein Visitor). Der
> folgende Abschnitt beschreibt einen **verworfenen Entwurf** und ist nicht als Ist zu lesen.

Das heute **leere** `algorithm_visitor`-Modul (nur `.gitkeep`) **bleibt leer (NICHT umgesetzt)**. ~~wird gefüllt. **[NEU]** `memento_visitor.hpp`:~~
- Lebewesen-Binary: `MementoAggregate<Composition>` **[NEU]** (analog ObserverAggregate, 17 Slots, conditional via `MementoAxis`); `memento_all()`/`rollback_all(m)` in `SearchAlgorithmAnatomy`.
- Hybrider Visitor: einkompilierte Rollback-Klassen erhalten beim Übergang einen Host-Steuerungs-Handle (`accept(HostSink&)`), besuchen den Host (z.B. „rollback fertig, Op wiederholbar"). Host iteriert NICHT in die Achsen.
- ABI-Übergang: `memento_all` quert als eigenständiges Sub-IF `IRollbackableTier` **[NEU]** (wie IMeasurableWorkload/IObservableTier — **nicht** an IAnatomyBase → kein vtable-Bruch). Disk-State wird **nicht** als POD kopiert, sondern in der Binary referenziert; Host steuert nur save/rollback.
```cpp
// [NEU] anatomy/rollbackable_tier.hpp — NUR bei MEASUREMENT_MODE=ON
class IRollbackableTier {
public:
    virtual ~IRollbackableTier() = default;
    virtual void tier_save_all()     noexcept = 0;  // alle stateful Achsen + Disk
    virtual void tier_rollback_all() noexcept = 0;  // inkl. flush/checkpoint-Rückrollung
};
```

**Disk-Sonderfall (ehrlich):** kein kanonisches generisches Format (axis_06 allein ~20 Allokatoren). → Memento ist
**pro-Achse/pro-Wrapper** spezifisch; `memento_all` aggregiert nur die **einheitliche API**. `builder/disk_serializer/` als Senke wiederverwenden.

---

## 4. Zwei-Phasen-Op-Schleife (pro Op GENAU 2×, bindend)

```
für jede Op im Lastenprofil:
   1) tier_save_all()          [memento-save-all, IRollbackableTier]
   2) op  (erste Ausführung)   [echte Last-Op, echte Daten, echter Lebewesen-State]
   3) tier_rollback_all()      [rollback-all: Lebewesen-State inkl. Disk auf Stand vor (2)]
   4) op  (op-measure)         [zweite Ausführung, JETZT Wall-Clock-umklammert + Observer]
```
Zweite Ausführung misst gegen **denselben** Vor-Zustand → eliminiert Pfad-Abhängigkeit der Latenz vom akkumulierten Zustand.
**Host-Verdrahtung:** [NEU] `drive_two_phase_op_loop()` ersetzt die heute **harte** WRITE/READ/DELETE-Schleife in
`tier_observe_trace_abi.hpp:82-137`; Wall-Clock-Umklammerung (`detail::abi_dur_ns`, :132-133) auf Schritt (4).
**Mess-OFF (Release-DLL):** observer_all+memento_all compile-time entfernt → Op läuft **einmal**, ohne save/rollback; Host misst
nackte Op-Latenz via Wall-Clock; Lebewesen-Binary trägt **keinen** Overhead.

---

## 5. Host-seitiges IMeasurableWorkload (Pfad A relokalisiert) + IDriveableTier-Handle

**V3-Designfehler-Korrektur:** `IMeasurableWorkload` als Orchestrator ist **rein generisch host-seitig** (reine Lebewesen-Binary
nicht generisch auslieferbar). Pfad A **nicht gestrichen**, war immer host-seitig geplant. — **Bleibt in DLL:** lebewesen-art-
angepasstes `run_workload()` (DLL-interne Ausführung der eigenen Such-Struktur). — **[NEU] host-seitig:**
`IMeasurableWorkloadHost`-Orchestrator (mehrere Lastprofile je Binary, YCSB A–F × OP-1..6 aus `op_type_filter.hpp:62-113`).

**IDriveableTier (Split):** heutiges `IObservableTier` (observable_tier.hpp:80-106) vermischt Antrieb + Beobachtung →
Antrieb (`tier_insert/lookup/erase/clear/size`, :87-99) wird **`IDriveableTier`** (immer einkompiliert); `tier_observe()` (:105)
bleibt im **observer_all**-Sub-IF (nur Messung-AN). Dock-Pfad bleibt: `select_for()` → `SearchAlgorithmDock::measure()` (:34-49),
zieht künftig `IDriveableTier*` + (nur Messung-AN) observer_all/memento_all. Einheitliche Observer-Abfrage über POD bleibt.

**Lebenszyklus:** import (`AnatomyModuleLoader.load`) → **Gate** (§6) → `select_for` → **je Lastprofil** measure (Zwei-Phasen §4)
→ unload. Warmup-Hooks im Adapter (`warm_up()` :97, `run()` :102). **Lastprofil-Persistenz [NEU]:** materialisierte Sequenz
(nicht nur Seed) in `disk_serializer/`; die zwei unkoordinierten Generatoren über **eine** `IMeasurableWorkloadHost`-Schnittstelle koordinieren.

---

## 6. Konformitäts-Gate gegen std::map je Gattung (NEU)

Jede Lebewesen-Binary **bei Verwendung zuerst** durch dieselben std::map-Hüllen-Tests (alle Randfälle valide, egal wie die Hülle
gebaut ist). Experiment misst nur Performance, aber jede Binary muss nach ihrer Gattung Testdaten konform speichern+wiedergeben.

**IST-Lücke:** Compile-Time-Konformität existiert (`verify_matches_std_map` std_map_equivalence_harness.hpp:28-52, 3-Stufen-Tests),
aber Template-only, nicht Runtime-ABI-generisch. Runtime nur `phase4b_functional_tests()` (experiment_driver.cpp:249-276) =
ABI-Vertrags-Check ohne std::map-Oracle.

**[NEU]** `builder/pruef_dock/conformance_gate.hpp` — Runtime-Host-Oracle über `IDriveableTier`: deterministische Randfall-Sequenz
(leer/single/Doppel-Insert/Update/erase-nichtvorhanden/clear-dann-lookup/full-sweep) **gegen `std::map<uint64,uint64>`**; pro
Gattung ein Gate-Profil; liefert Konformitäts-Quoten; arbeitet ausschließlich auf dem ABI-uint64-Raum (nach Umstufung-B).
**Reihenfolge bindend: import → GATE → (nur bei pass) messen.** Erweitert `phase4b_functional_tests()` um den Oracle-Vergleich.

---

## 7. Observer + Memento-Entfernung rein compile-time (bindend)

```cpp
template <AnatomyConcept A>
class SearchAlgorithmAbiAdapter final
    : public IAnatomyBase
    , public IDriveableTier                       // IMMER (Antrieb, auch Release)
#if COMDARE_MEASUREMENT_ON
    , public IObservableTier_ObserverPart         // nur Messung-AN: observer_all
    , public IRollbackableTier                     // nur Messung-AN: memento_all
    , public IMeasurableWorkload                    // nur Messung-AN: Pfad A in-DLL
#endif
{ ... };
```
Release-DLL: observer_all/memento_all **nicht vererbt**, vtable-Slots existieren nicht, Aggregate `#if`-eliminiert — **kein**
`dynamic_cast` zur Entfernung, die Typen sind schlicht **nicht da**. Host-`dynamic_cast` (search_algorithm_dock.hpp:42) bleibt
nur Probing-Pfad bei Messung-AN. Hot-Path bleibt compile-time-monomorph.

---

## 8. Konsequenz für I1 / die ABI — revidierte Anfass-Liste

| Aktion | Datei | Änderung |
|---|---|---|
| **Split** IObservableTier→IDriveableTier | `anatomy/observable_tier.hpp:80-106` | Antrieb (:87-99) → neues `IDriveableTier` (immer); `tier_observe()` (:105) → Observer-Teil |
| **[NEU]** observer_all-Sub-IF | `anatomy/observable_tier.hpp` | Observer-Teil + POD (:40-59), nur Messung-AN-vererbt |
| **[NEU]** memento_all-Sub-IF | `anatomy/rollbackable_tier.hpp` | `IRollbackableTier`, nur Messung-AN |
| **[NEU]** MementoAggregate | `anatomy/memento_aggregate.hpp` | analog observer_aggregate.hpp, 17 Slots, conditional |
| **[NEU]** memento_all() | `anatomy/search_algorithm_anatomy.hpp` (neben :53-72) | save/restore aller stateful Achsen |
| **[NEU]** MementoAxis+EmptyMemento | `topics/axis_base.hpp` | analog ObservableAxis/EmptyAxisSnapshot |
| **Adapter** multi-inherit conditional | `anatomy/abi_adapter.hpp:75-77` | IDriveableTier immer; observer/memento/MeasurableWorkload `#if COMDARE_MEASUREMENT_ON` |
| **Dock** Probing umstellen | `builder/pruef_dock/search_algorithm_dock.hpp:42-45` | `dynamic_cast<IDriveableTier*>` + Messung-AN-Zweige; Gate vor measure() |
| **[NEU]** conformance_gate | `builder/pruef_dock/conformance_gate.hpp` | §6 |
| **[NEU]** Zwei-Phasen-Treiber | `builder/anatomy_commands/two_phase_op_loop.hpp` | §4, ersetzt harte Schleife tier_observe_trace_abi.hpp:82-137 |
| ~~**[NEU]** memento_visitor~~ | `builder/algorithm_visitor/` (**bis heute leer, NICHT umgesetzt** — G5-Audit w289llo0o) | ~~hybrider Visitor~~ (verworfen; real = Memento-vtable) |
| **[NEU]** Host-Workload-Orchestrator | `builder/workload_driver/measurable_workload_host.hpp` | §5 |
| **[NEU]** Master-Flags | `CMakeLists.txt`, `cmake/*` | `COMDARE_MEASUREMENT_MODE`/`_RELEASE_MODE` + axis_05/10/14/filter/io/migration/ARM64 |

**ABI-Major-Bump Pflicht:** Der Split ändert das Adapter-vtable-Layout → **`ANATOMY_ABI_MAJOR`-Bump** → **alle DLLs neu**
(Pilot-Permutations + prt-art-Prüflinge `COMDARE_CE_PRUEFLINGE`). Datei `anatomy_module_abi_v1.hpp`→`_v2`. (observer_all+memento_all
allein wären additiv; der **Split** ist der Bump-Auslöser.)

---

## 9. Inkrement-Reihenfolge + Risiken

> **Vorbedingung:** `observe_all()` ist für die **15 nicht-getriebenen** Achsen Stub (nur search_algo+allocator real). `memento_all`
> erbt denselben Bedarf real-getriebener Achsen. Umstufung-B (Task #42, EnabledStrategies=4 Organe — Symbol-Ebene verifiziert done)
> ist die Grundlage; reale Mehr-Achsen-Treibung ist die eigentliche I0-Arbeit.

1. **I0** Mehr-Achsen real treiben (Grundlage observe_all/memento_all über >2 Achsen).
2. **I1** Flags (`MEASUREMENT_MODE`/`RELEASE_MODE` + fehlende axis-Vendor-Flags). Reine CMake, low-risk.
3. **I2** Interface-Split + `ANATOMY_ABI_MAJOR`-Bump, **alle DLLs neu**. **Destruktiv 3 Repos → Tag+Commit+Push.**
4. **I3** observer_all gating + compile-time-Entfernung verifizieren (Release-DLL ohne vtable-Slot).
5. **I4** Konformitäts-Gate + Verdrahtung vor measure().
6. **I5** MementoAxis-Concept + EmptyMemento (stateless trivial).
7. **I6** memento_all für In-Memory-Achsen (03a/04/06-RAM) + Visitor füllen.
8. **I7** Zwei-Phasen-Op-Schleife auf In-Memory-Achsen.
9. **I8** memento_all Disk-Achsen (io_dispatch/10/migration) — höchstes Risiko.
10. **I9** Host-Workload-Orchestrator + Lastprofil-Persistenz + Generator-Koordination.
11. **I10** `measure_genus_sequential()` in `V32Orchestrator` verdrahten (heute nur CLI `--observe`).

### Risiken (ehrlich)
- **R1 Memento IO/Disk (höchstes):** kein generisches Format. Optionen: (a) pro-Wrapper-Memento (vollständig, teuer) vs (b) [ANNAHME] Lebewesen-weiter Disk-Checkpoint (gesamtes Backing-File kopieren) — umgeht per-Allokator-Serialisierung, kostet Disk-IO pro Op.
- **R2 ×2 + save/rollback pro Op:** bei Disk-Achsen Größenordnungen Overhead → ggf. sample-basiert statt jede Op.
- **R3 Visitor-ABI-Stabilität:** `IRollbackableTier` POD-frei + schmal; Visitor lebt IN der DLL; Host-Sink als Funktionszeiger-POD (ABI-sicher), nicht vtable.
- **R4 Gate-Umfang:** „alle Randfälle" begrenzt halten (läuft vor jeder Binary); Kern-Set + optional `--exhaustive`; zunächst nur SearchAlgorithm-Gattung.
- **R5 axis_08 RCU/HP Rollback-Leak:** retired nodes nach restore; GC-Safe-Point-Bewusstsein nötig.
- **R6 Pfad A↔B Konsistenz:** nach Relokalisierung sollte Pfad A host-seitig dasselbe Lebewesen treiben → Konsistenz-Assertion [NEU].
- **R7 STATISTICS vs MEASUREMENT_MODE:** MEASUREMENT_MODE = Master; STATISTICS Sub/ersetzt; RELEASE_MODE erzwingt beide OFF.

**Vollständige Datei-Referenzen + neu-anzulegende Header:** s. Workflow-Ausgabe `wlw1w69eg` (im Transkript) — alle [NEU]-Header
unter §3/§5/§6/§8 gelistet.
