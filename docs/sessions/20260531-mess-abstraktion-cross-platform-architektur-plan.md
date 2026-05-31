# Architektur-Plan: Einheitliche Mess-Abstraktion, arch-adaptierte Builder-Werkzeuge, Plattform-Fingerprint + ZIH-Precompile

> **Status:** PLAN (keine Marke „erledigt/verifiziert"). **Provenienz:** Inventar+Synthese-Workflow `w403jsmd2`
> (4 Explore-Agenten über den IST-Code + 1 Architektur-Synthese, 2026-05-31). **User-Auftrag:** „solide Messverfahren
> unter derselben Abstraktion, PCM als EINE Implementierung unter allen Chip-Vendors/ISA … CacheEngineBuilder gibt
> arch-adaptierte Werkzeuge fürs Prüf-Dock … Build mit Metaprogrammierung … CMake-Check ob Binaries zur Plattform
> passen, sonst neu bauen … manueller Precompile-Modus für ZIH (run+measure, kein Compile)".
>
> **Intel-Implementierung (IntelPcmSource) im Detail:** `20260531-p4-pmc-intel-pcm-plan.md` (PCM-Setup + Live-Geräte-Status:
> Secure Boot AUS, BitLocker AUS, HVCI/Memory-Integrity AN = einziger Blocker). **Ledger-Bezug:** §b R5.D / P4-PMC.
>
> **Kernbefund (anknüpfen, NICHT neu erfinden):** `comdare_hw_counters_v1` (Struct) + `pull_live_counters` (Funktionszeiger)
> existieren bereits in `libs/cache_engine/include/cache_engine/abi/module_abi_v1.hpp:45-51,114`, sind aber **nie befüllt**.
> Ebenso existieren zwei divergierende Mess-Pfade (A `IMeasurableWorkload::run_workload`, B `IObservableTier::tier_observe`)
> und die 6 CSV-HW-Spalten als Platzhalter `0` in `comdare_measurement_record_v1` (Felder 7-11).

Pfade relativ zu `Code/` (= `C:/Users/benja/OneDrive/Desktop/Diplomarbeit - Datenbanken/Code/`). Externe Engine = `ce/` (= `Code/external/comdare-cache-engine/`).

---

## 1. Zielbild + Schichtenbild

**Zielbild.** Unter den zwei vorhandenen Mess-Pfaden + dem ungenutzten HW-Counter-Kanal wird EINE vendor-neutrale
Mess-Quelle `IMeasurementSource` eingeführt, unter der Intel PCM / AMD uProf / ARM-PMU·PAPI / Wall-Clock-Fallback
**gleichrangige** Implementierungen sind. Ihre Werte mappen auf die **bereits vorhandenen** Record-Spalten
(`comdare_measurement_record_v1` Felder 7-11). Der `CacheEngineBuilder` erhält über das Prüf-Dock arch-bezogene
Bewertungs-Werkzeuge; dazu ein Plattform-Fingerprint-Fit-Check (R5) und ein manueller Precompile-Modus (R6) für den
ZIH-Fall (kein Admin, kein Compile auf dem Rechenknoten).

```
                 ┌──────────────────────────── F15 / CSV / Pipeline ───────────────────────────┐
   AUSWERTUNG    │ apps/f15_compare (Pfad A/B/multi)  →  02_messung_driver/measurement_writer   │
                 │ ce/.../result_aggregator.hpp (CSV/JSON, 12→16+6 Spalten)                     │
                 └───────────────────▲─────────────────────────────────▲───────────────────────┘
                                     │ ExecutionResult / Snapshot       │ comdare_measurement_record_v1
   KONSOLIDIERUNG ┌─────────────────┴─────────────────────────────────┴───────────────────────┐
   (P5: 2 ABIs    │   ComdareMeasurementSnapshotV1 (NEU, 1 autoritativer POD, V1-erweiterbar)   │
    → 1 Quelle)   │   speist:  ExecutionResult  +  ComdareTierObserverSnapshotV1  +  record_v1   │
                 └───────────────────────────────────▲───────────────────────────────────────┘
                                                     │ read_delta() / read_aggregate()
   BUILDER-       ┌─────────────────────────────────┴───────────────────────────────────────┐
   WERKZEUGE      │  PruefDock / SearchAlgorithmDock  →  drive_tier_observe_trace_abi          │
   (R3 Dock +     │  + ArchEvaluator (arch-bezogene Einordnung: cost_model × IIsaFeatureSet)   │
    Bewertung)    └───────────────────────────────────▲───────────────────────────────────────┘
                                                     │ IMeasurementSource* (DI, am Dock injiziert)
   VENDOR-IMPLS   ┌─────────────────┬─────────────────┴────┬──────────────────┬────────────────┐
   (R1, gleich-   │ IntelPcmSource  │ AmdUprofSource       │ ArmPmuPapiSource │ WallClockSource │
    rangig)       │ (PCM/RAPL/perf) │ (uProfPcm/perf core) │ (PAPI/perf_event)│ (Observer-only) │
                 └─────────────────┴──────────────────────┴──────────────────┴────────────────┘
                                                     ▲
   ABSTRAKTION    ┌─────────────────────────────────┴───────────────────────────────────────┐
   (R1 Wurzel)    │ IMeasurementSource / IPmcSource  — open(events), read_delta() → 16+6 POD │
   Compile-Time   │ COMDARE_MEASUREMENT_VENDOR-Selektor (mp_if über CMake-Detektion)          │
                 └───────────────────────────────────────────────────────────────────────────┘
```

---

## 2. R1 — Einheitliche Mess-Abstraktion

### 2.1 Der eine autoritative Record (löst zugleich die 16+6-CSV-Forderung)

Neuer POD `ComdareMeasurementSnapshotV1`, abgelegt **neben** dem bestehenden Snapshot in
`ce/libs/cache_engine/anatomy/observable_tier.hpp` (gleiche ABI-Disziplin: nur `uint64`,
`static_assert(standard_layout && trivially_copyable)`, `kMeasurementSnapshotVersion`). Er ist die gemeinsame
Schnittmenge aus `ExecutionResult` (host-seitig), `ComdareTierObserverSnapshotV1` (DLL-seitig) und
`comdare_measurement_record_v1` (Binary-Pipeline) — die P5-Lücke „keine gemeinsame Schema-Deklaration".

22 `uint64`-Felder (= 16 CSV-Kern + 6 HW-Counter):
- Meta (4): `engine_name_hash`, `throughput_ops_per_sec`, `latency_p50_ns`, `latency_p99_ns`
- Observer search_algo (6): die 6 Felder aus `ComdareTierObserverSnapshotV1` (lookup/hit/miss/insert/erase/peak)
- Observer allocator + Speicher (6): `alloc_bytes_allocated`, `alloc_bytes_in_use`, `alloc_allocation_count`, `alloc_failure_count`, `bytes_in_use_peak`, `tier_fill_level`
- **6 HW-Counter** (die Lücke): `hw_cycles`, `hw_instructions`, `hw_l1d_miss`, `hw_l3_miss`, `hw_branch_miss`, `hw_mem_stall` — plus `hw_energy_micro_joules` und `hw_dtlb_miss` als Erweiterungsspalten (mappen auf `record_v1.energy_micro_joules`/`dtlb_misses`).

Die 6 HW-Counter sind **layout-kompatibel** zum bereits existierenden `comdare_hw_counters_v1`
(`module_abi_v1.hpp:45-51`: `cycles`, `instructions`, `cache_misses[3]`, `tlb_misses[2]`, `timestamp_ns`). Annahme:
wir behalten `comdare_hw_counters_v1` als den **DLL-seitigen Roh-Transport** (über den schon deklarierten
`pull_live_counters`-Funktionszeiger) und projizieren ihn host-seitig in `ComdareMeasurementSnapshotV1`. Kein
paralleles ABI — der Kanal existiert, er ist nur nie befüllt worden.

### 2.2 Interface-Skizze `IMeasurementSource` / `IPmcSource`

Neuer Header `ce/libs/cache_engine/include/cache_engine/measurement/i_measurement_source.hpp`. Bewusst **host-seitig**
(nicht über die DLL-Grenze): PMC-Zugriff braucht OS-Privilegien/Treiber, die ein Tier-Modul nicht haben soll. Das Tier
liefert nur seinen In-Modul-Roh-Counter via `pull_live_counters`; die echte PMC-Erhebung umklammert host-seitig den
`run_workload`/`tier_*`-Aufruf am Dock.

```cpp
namespace comdare::cache_engine::measurement {

enum class MeasuredEvent : uint32_t {          // vendor-neutrale Semantik, nicht Event-Code
  Cycles, Instructions, L1dMiss, L2Miss, L3Miss,
  DtlbMiss, BranchMiss, MemStall, CoherenceInval, EnergyUj
};

enum class SourceStatus : int {                 // errno-style wie PruefDock
  Available=0, NotImplemented=1, DriverMissing=2,
  PermissionDenied=3, EventUnsupported=4, MultiplexRequired=5
};

struct MeasurementSourceCaps {                  // was diese Quelle KANN (Gate-Info)
  uint32_t max_concurrent_events;               // Intel Skylake GP: 4 → Multiplex-Flag
  bool     has_energy;                          // RAPL/uProf vorhanden?
  bool     needs_admin;                         // ZIH-Gate: true → auf Rechenknoten gesperrt
  bool     hybrid_core_aware;                   // P/E-Core separate Event-Codes
  std::array<bool, 10> event_supported;         // pro MeasuredEvent
};

class IMeasurementSource {                       // == IPmcSource (PMC ist EINE Familie davon)
public:
  virtual ~IMeasurementSource() = default;
  virtual std::string_view vendor_id() const noexcept = 0;          // "intel-pcm"/"amd-uprof"/"arm-papi"/"wallclock"
  virtual MeasurementSourceCaps capabilities() const noexcept = 0;
  virtual SourceStatus open(std::span<const MeasuredEvent>) noexcept = 0;  // Event-Auswahl + Multiplex-Plan
  virtual void         begin() noexcept = 0;     // Counter-Snapshot vor Workload
  virtual void         end()   noexcept = 0;     // Counter-Snapshot nach Workload
  virtual void read_delta(ComdareMeasurementSnapshotV1* out) const noexcept = 0; // (end-begin) → 6+ HW-Spalten
  virtual void close() noexcept = 0;
};
}
```

`read_delta()` ist exakt der im Inventar geforderte Andock-Punkt für `record_v1`-Spalten 7-11. `WallClockSource`
implementiert alle Methoden, füllt aber nur `Cycles`-Proxy aus Wall-Clock und setzt HW-Misses=0 (heutiges Verhalten,
jetzt aber **explizit** als degraded source statt Platzhalter).

### 2.3 Vendor/ISA-Implementierungs-Matrix

| Vendor | ISA / Plattform | Counter-Quelle | L1/L3-Miss-Event (Annahme, web-zu-verifizieren) | Energie | Verfügbarkeit / Gate |
|---|---|---|---|---|---|
| Intel | x86_64 (Skylake…Raptor) | Intel PCM / `perf_event` | `MEM_LOAD_RETIRED.L1_MISS`, `LONGEST_LAT_CACHE.MISS` | RAPL Package (DRAM nur Server; i7-1270P Package-only) | Treiber/Admin → `needs_admin=true`; ZIH-Knoten i.d.R. gesperrt |
| Intel Hybrid | x86_64 P+E (Alder/Raptor) | PCM + intel/perfmon JSON | P/E-separate Event-Codes; AVX-512 evtl. BIOS-disabled | RAPL Package | `hybrid_core_aware=true`, Core-Pinning nötig |
| AMD | x86_64 (Zen2…Zen5) | AMD uProf / `AMDuProfPcm` / core-perf | `LS_DC_REFILLS_FROM_L2` (L1-Refill), L3 via PMC/df | RAPL `energy-pkg` (Zen) | `needs_admin` für uProf-Treiber; perf-user-level teilw. ok |
| ARM | aarch64 (Neoverse/GH200, Cortex-A76/Pi5, Apple M) | PAPI / `perf_event` PMU | `L1D_CACHE_REFILL`, `L2D/LL_CACHE_MISS` (PMUv3) | meist KEINE RAPL-Analogie → `has_energy=false` | Apple M: PMU stark eingeschränkt; PAPI braucht oft Kernel-Recht |
| generisch | alle | Wall-Clock + `IObservableTier` | keine (HW-Misses=0) | nein | **immer** verfügbar, `needs_admin=false` — der garantierte Fallback |

Die Event-Code-Zuordnung pro Vendor ist **Annahme aus dem Inventar** und gehört vor Implementierung pro Event
web-verifiziert (Direktive „Web-Recherche pro Algorithmus", hier analog pro Event-Code). Die generische Wall-Clock-Zeile
knüpft direkt an `IObservableTier`/`run_workload` an und ist der ZIH-sichere Default.

### 2.4 Compile-Time-Selektor (kein Runtime-switch im Hot-Path)

CMake setzt aus `platform_detection.cmake` (`COMDARE_BLOCK_AO_PLATFORM`, `COMDARE_ARCH_*`) eine einzige Cache-Variable
`COMDARE_MEASUREMENT_VENDOR ∈ {intel_pcm, amd_uprof, arm_papi, wallclock}` + ein `AUTO`-Default, das auf `wallclock`
zurückfällt, wenn die Vendor-Toolchain nicht gefunden wird. Im Code wählt ein `mp_if`/`if constexpr`-Alias
`using SelectedMeasurementSource = …;` die Implementierung **zur Compile-Zeit** — konform zu „Kein Runtime-Switch im
Hot-Path". Die einzige Laufzeit-Verzweigung ist die einmalige `capabilities()`/`open()`-Gate-Prüfung **außerhalb** der
Messschleife (begründete Ausnahme: HW-Verfügbarkeit ist erst zur Laufzeit bekannt; sie liegt nicht im Hot-Path).

---

## 3. R2 — Anknüpfung an vorhandene Mess-Apps + ABI-Konsolidierung (2 ABIs → 1 Quelle)

**Konkrete Andock-Dateien (kein Parallel-Neubau):**

1. `ce/libs/cache_engine/anatomy/observable_tier.hpp` — `ComdareMeasurementSnapshotV1` + `kMeasurementSnapshotVersion` **neben** `ComdareTierObserverSnapshotV1` (gleiche `static_assert`-Disziplin). Die 13 vorhandenen Snapshot-Felder 1:1 eingebettet → keine Bruch-Migration.
2. `ce/libs/cache_engine/include/cache_engine/abi/module_abi_v1.hpp` — **nichts neu erfinden**: `comdare_hw_counters_v1` (Z. 45-51) + `pull_live_counters` (Z. 114) endlich verkabeln; `SearchAlgorithmAbiAdapter` implementiert `pull_live_counters` statt es leer zu lassen.
3. `ce/libs/cache_engine/anatomy/abi_adapter.hpp` — `SearchAlgorithmAbiAdapter` erhält die `pull_live_counters`-Implementierung (In-Modul-Roh-Counter, statistik-basiert); die echte PMC-Klammer bleibt host-seitig.
4. `ce/libs/cache_engine/builder/pruef_dock/search_algorithm_dock.hpp` + `…/pruef_dock.hpp` — `PruefDockMeasureOptions` um `IMeasurementSource*` (DI, default `WallClockSource`); `SearchAlgorithmDock::measure()` umklammert den `drive_tier_observe_trace_abi`-Lauf mit `source->begin()/end()` + füllt die 6 HW-Spalten via `read_delta()`. **Zentraler Merge-Punkt der zwei Pfade**: Observer + HW-Counter in EINEM `ComdareMeasurementSnapshotV1` korreliert.
5. `ce/libs/cache_engine/builder/anatomy_commands/tier_observe_trace_abi.hpp` — `drive_tier_observe_trace_abi` + `AbiFillLevelSnapshot` tragen zusätzlich HW-Counter-Delta pro Checkpoint (Wall-Clock-korreliert, wie heute r/w/d-Wall-Clock).
6. `ce/libs/cache_engine/builder/commands/execution_result.hpp` + `execute_engine_command.hpp` — `ExecutionResult` um optionales `HardwareCounters`-Aggregat (6 `uint64`) + `MeasurementResolution`-Enum (`Host_Simulation`/`RunWorkload_DLL`/`HybridBoth`); `OperationOutcome` trägt optional Per-Op-PMC-Delta. P5-Frage „welche Felder sind autoritativ?" → `resolution_mode` deklariert die Quelle.
7. `ce/libs/cache_engine/builder/commands/result_aggregator.hpp` — CSV-Header 12 → 16+6 Spalten; Mapping aus `ComdareMeasurementSnapshotV1`. Sample-/Counter-Dateien referenzieren die Result-Zeile via `(engine_name_hash, timestamp_ns)`-Fremdschlüssel.
8. `ce/apps/f15_compare/main.cpp` — `--observe`-Pfad erhält das injizierte `IMeasurementSource`; neuer Schalter `--measure-source=auto|intel|amd|arm|wallclock`. `--pipeline-csv` schreibt **reale** Zahlen statt `mean-ns→total_cycles`-Approximation.
9. `Code/02_messung_driver/measurement_writer.hpp` — `make_record_from_run()` (Z. 96-118) bekommt Überladung `make_record_from_snapshot(ComdareMeasurementSnapshotV1 const&)`, die Felder 7-11 aus echten Werten statt `0` füllt. Approximations-Variante bleibt als Fallback (kennzeichnet sich über `resolution_mode`).
10. `Code/02_messung_driver/v32_orchestrator.hpp` + `op_type_filter.hpp` — der `telemetry_hint`/`simd_hint` aus `op_type_filter` (OP-1..OP-6) parametrisiert `IMeasurementSource::open(events)` (welche Events sind für diese OP-Klasse sinnvoll). Reine Verkabelung, keine neue Tabelle.

**Sub-Interface-Fehlermodell (P5-Lücke):** `dynamic_cast` zu `IMeasurableWorkload`/`IObservableTier` wird in ein
`SubInterfaceStatus`-Enum (`Available/NotImplemented/VersionMismatch/RuntimeUnavailable`) gekapselt; der
`AnatomyModuleLoader` liefert nach `load()` eine `InterfaceCapabilities`-Query (`has_measurable_workload()`,
`has_observable_tier()`, `hw_counters_available()`). Der Host degradiert deterministisch.

---

## 4. R3 — Arch-adaptierte Builder-Werkzeuge + Bewertung am Dock

Der `CacheEngineBuilder` gibt am Prüf-Dock nicht nur Messwerte, sondern eine **arch-bezogene Einordnung** aus. Neues
Werkzeug `ArchEvaluator` in `ce/libs/cache_engine/builder/pruef_dock/arch_evaluator.hpp`, das vorhandene Bausteine
kombiniert — nichts neu erfunden:

- Eingang 1: `ComdareMeasurementSnapshotV1` (gemessen am Dock).
- Eingang 2: `IIsaFeatureSet::cost_model` pro `HardwareExtension` (Lookup/Insert/Compare/Prefetch/Atomic/Sort/HashCompute) aus `ce/libs/cache_engine/include/cache_engine/platform/isa_features.hpp` — das **erwartete** Kostenmodell der aktuellen ISA.
- Eingang 3: `core_layout.hpp` (`CoreClass` P/E, `has_hybrid_cores()`, `PinningPolicyId`) + `axis_09b_simd_extension_*` (`units_per_socket`, `accessible_from_efficiency_cores`).

`ArchEvaluator::evaluate()` liefert ein `ArchVerdict`:
- **Effizienz-Quotient** = gemessene L3-Misses / vom `cost_model` erwartete Misses für diese ISA → „nutzt das Tier die Architektur aus?"
- **SIMD-Ausnutzung**: erfüllt die Binary die `provides_avx*`-Versprechen des Wrappers vs. `cpuid_probe` Ist? (Lücke „promised vs actual").
- **Hybrid-Gate**: lief der Messlauf auf P- oder E-Core (Pinning-Empfehlung aus `PinningPolicyId::HotPathOnHighIpc`).
- **Vendor-Einordnung**: AMD vs Intel vs ARM relative Bewertung über `COMDARE_BLOCK_AO_PLATFORM`.

Registry-Erweiterung: `PruefDockRegistry` bekommt Docks für Set/Sequence/Adapter/View (heute nur `SearchAlgorithmDock`)
über `IPruefDock`-Polymorphie — jedes Dock injiziert dieselbe `IMeasurementSource` + `ArchEvaluator`. Der `ArchEvaluator`
ist das R3-Werkzeug „AMD/Intel-spezifisch durchtesten + gegen die Architektur bewerten".

---

## 5. R4 + R5 + R6 — Build-Metaprogrammierung, Plattform-Fit-Check, manueller Precompile-/Cross-Modus (ZIH)

### 5.1 R4 — Build-Metaprogrammierung des Builders selbst (Annahme: Build auf Ziel-Testplattform)

Erweiterung von `ce/cmake/isa_features.cmake` (hat bereits `check_cxx_source_compiles` für AVX2/AVX512/BMI2/NEON/SVE2 +
`COMDARE_apply_simd_flags()`): der `CacheEngineBuilder` + die Tools (`anatomy_codegen_tool`, `adhoc_emitter`) werden mit
den **zur Host-CPU passenden** ISA-Flags gebaut, abgeleitet aus `cpuid_probe`/`platform_detection.cmake`. Neuer dritter
`COMDARE_DETECTION_MODE`-Wert `AUTO_DETECT` (Andock `CMakeLists.txt:23-25`), der zur Configure-Zeit Host-CPUID prüft +
Binary-zu-Host-Compat-Flags einbäckt — ohne Plattform-Hardcoding. Filtert zugleich die kartesische Permutationsmenge
(Doku 15 §6: 96 → ~25-30) via `mp_remove_if` mit `is_compatible`-Predicate im `PermutationEngine` (heute manuell).

### 5.2 R5 — Plattform-Fingerprint-Fit-Check (neuer Pipeline-Zwischenschritt)

**Fingerprint-Erzeugung (Build-Zeit).** Neue Funktion in `ce/cmake/platform_detection.cmake`:
`COMDARE_PLAT_FINGERPRINT := SHA256(COMDARE_OS + COMDARE_ARCH + ISA-Feature-Set + Compiler-ID+Version + CMAKE_CROSSCOMPILING)`.
Wichtig: das ist **nicht** `permutation_fingerprint` aus `fixed_length_fingerprint.hpp` (= Achsen-Kombi-ID) — es ist ein
**Host-Plattform**-Fingerprint. Beide koexistieren.

**Stempelung der Artefakte.** `anatomy_codegen.cmake` / `adhoc_emitter.cmake` schreiben pro erzeugter DLL/SO einen
Manifest-Eintrag (`compatibility_matrix.json` / `manifest.json`) mit `{perm_id, permutation_fingerprint, plat_fingerprint,
isa_baseline, vendor_affinity, library_type, sha256}`. Das ABI-Feld `comdare_platform_snapshot_v1`
(`module_abi_v1.hpp:77-91`) wird um `vendor_id` + `fallback_isa_chain` erweitert + in jede DLL eingebacken.

**Fit-Check (neue Phase, Configure-Zeit).** Neues Modul `ce/cmake/platform_fit_check.cmake`:
```
if(NOT EXISTS plat_fingerprint.stamp)            → erster Build
elseif(stored_plat_fingerprint != COMDARE_PLAT_FINGERPRINT)
     message(STATUS "Plattform geändert → CacheEngineBuilder + Tier-Binaries werden NEU gebaut")
     → invalidiere .build_complete.marker der betroffenen Targets, trigger Rebuild
else  → Binaries passen, kein Rebuild
```
Ergänzt `anatomy_codegen_runner.cmake` (heute nur Timestamp) um den Plat-Hash-Vergleich. **Laufzeit-Variante**: beim
`dlopen` im `ModuleLoader` ruft `runtime_compatibility_check(loaded_snapshot, cpuid_probe())` — falls die Binary aus dem
Manifest nicht zur Live-Probe passt: sauberer Skip statt Crash (`live_platform_model.hpp::is_permutation_compatible(fingerprint)`).
Das ist der R5-Zwischenschritt: „passen die Binaries zur aktuellen Plattform? wenn nein → neu bauen (lokal) bzw.
überspringen (Messlauf)".

### 5.3 R6 — Manueller Precompile-/Cross-Modus (ZIH-Fall)

Neues Modul `ce/cmake/precompile_mode.cmake` mit drei Modi (eine CMake-Konfiguration, manuell umschaltbar):

| Modus | Was passiert | Wo |
|---|---|---|
| `NORMAL` | Build + Run + Measure am selben Host (heute) | Entwicklungs-Laptop |
| `PRECOMPILE` | nur kompilieren für `COMDARE_PRECOMPILE_FOR_TARGET_TRIPLE` (z.B. `x86_64-linux-gnu` Barnard, `aarch64-linux-gnu` GH200), exportiert **CacheEngineBuilder + ALLE Tier-Binaries** + `.lib`/`.so` + `manifest.json` (mit `plat_fingerprint` + sha256) nach `COMDARE_PRECOMPILE_TARGETS_DIR/precompiled_dist/` | Laptop/CI baut FÜR ZIH |
| `PRECOMPILED` | **kein Compile** — importiert Binaries aus `manifest.json`, validiert `plat_fingerprint` gegen Live-Host, dann nur `load + measure` | ZIH-Rechenknoten |

Treiber-CLI `ce/apps/precompile_batch_builder` (Inventar-Andock 10): `--target-triples x86_64-linux-gnu,aarch64-linux-gnu`
ruft CMake je Triple auf, exportiert SOURCES+BINARY-Tarball für ZIH-Upload (passt zum `/projects/p_llm_compile`- bzw.
`/data/horse`-Workflow). `f15_compare` erhält `--precompiled-manifest <json>` + `--target-triple` + `--skip-codegen`:
auf dem ZIH-Knoten läuft NUR load+measure, kein `adhoc_emitter`-Aufruf. `ModuleLoader::load_precompiled(manifest, triple)`
validiert vor `dlopen` + fällt auf `LD_LIBRARY_PATH` zurück (behebt die „Pfade hart auf Filesystem"-Lücke).

### 5.4 ZIH-Realität (ehrlich benannt)

- **Compile.** ZIH-Rechenknoten erlauben nur user-level run+measure, kein Compile (CLAUDE.md: „Nur vorkompilierte/mitgebrachte Binaries"). Der `PRECOMPILED`-Modus deckt genau das ab. Barnard = x86_64 (gleicher Triple wie Laptop-Linux-Target → unproblematisch); **GH200 = aarch64 → echtes Cross-Compile** nötig, das `compiler_cache.cmake`/`tools_cache.cmake` heute nur auf UNIX bauen — für aarch64-Cross ist ein Cross-Toolchain-File Pflicht (offene Entscheidung).
- **HW-Counter.** **Ehrlich:** Intel PCM + AMD uProf brauchen Treiber/Admin-Rechte, die es auf einem fremden HPC-Rechenknoten **nicht** gibt (`needs_admin=true`-Gate). Auf ZIH fällt die Mess-Quelle daher auf **PAPI / `perf_event` user-level** zurück (sofern `perf_event_paranoid` das erlaubt — sonst nur Wall-Clock/Observer). Energie via RAPL ist auf ZIH-Knoten i.d.R. gesperrt → `has_energy=false`, `energy_micro_joules` bleibt 0, **im Manifest als „nicht erhoben" markiert** (nicht „0 gemessen"). Genau deshalb sind `WallClockSource`/`ArmPmuPapiSource` **gleichrangige** Implementierungen, nicht Notlösungen.
- **PIKA.** Für ZIH-Energie/PMC kann ergänzend das vorhandene PIKA-Job-Monitoring (CLAUDE.md-Link) als externe Quelle dienen — als separate `IMeasurementSource`-Implementierung `PikaJobMonitorSource` (Annahme, später).

---

## 6. Inkrement-Reihenfolge + Risiken

| Inc | Deliverable | Verifikation |
|---|---|---|
| **I1** | `ComdareMeasurementSnapshotV1` + `kMeasurementSnapshotVersion` in `observable_tier.hpp`; `static_assert`s | Unit-Test: `sizeof`-Erwartung, `standard_layout && trivially_copyable`; bestehende Tests grün |
| **I2** | `IMeasurementSource`-Interface + `WallClockSource` (garantierter Fallback) | `f15_compare --measure-source=wallclock` läuft, CSV hat 16+6 Spalten (HW=0, `resolution_mode=Host`) |
| **I3** | Dock-Verkabelung: `IMeasurementSource*`-DI in `SearchAlgorithmDock` + `drive_tier_observe_trace_abi` HW-Klammer; `pull_live_counters` im `abi_adapter.hpp` implementiert | `--observe` schreibt korrelierte Observer+HW-Spalten; FK konsistent |
| **I4** | `result_aggregator` 16+6 CSV + `measurement_writer::make_record_from_snapshot`; P5-Konsolidierung (Sub-Interface-Status-Enum, `MeasurementResolution`) | end-to-end: `f15_compare --pipeline-csv` → `binary_to_csv` mit realen Feldern 7-11 |
| **I5** | `IntelPcmSource` ODER `ArmPmuPapiSource` (eine echte HW-Quelle) + Compile-Time-Selektor `COMDARE_MEASUREMENT_VENDOR` | auf i7-1270P: echte L1/L3-Misses ≠ 0; Vendor-Event-Codes **vorher web-verifiziert** |
| **I6** | `ArchEvaluator` + `ArchVerdict` am Dock (R3) | Verdict-CSV-Spalte: Effizienz-Quotient + SIMD-promised-vs-actual plausibel auf 2 Tieren |
| **I7** | `COMDARE_PLAT_FINGERPRINT` + `platform_fit_check.cmake` + Manifest-Stempelung (R5) | Build A, Plattform-Var ändern → Configure meldet „Rebuild nötig"; `runtime_compatibility_check` skippt inkompatible DLL |
| **I8** | `precompile_mode.cmake` 3 Modi + `precompile_batch_builder` CLI + `ModuleLoader::load_precompiled` (R6) | `PRECOMPILE` exportiert dist-Tarball; `PRECOMPILED` auf zweitem Host: nur load+measure, kein Compile |
| **I9** | restliche Vendor-Sources (AMD uProf, ARM PAPI, ggf. PikaJobMonitorSource) + Set/Sequence/Adapter/View-Docks | je Source `needs_admin`-Gate korrekt; Multiplex bei >4 Events |

### Risiken / offene Entscheidungen (ehrlich)

1. **PMC braucht Admin — Kernkonflikt mit ZIH.** Auf dem ZIH-Rechenknoten sind PCM/uProf/RAPL voraussichtlich gesperrt; nur PAPI/perf-user-level oder Wall-Clock bleiben. → Volle PMC-Tiefe (Energie/Miss) realistisch nur auf dem eigenen Laptop/kontrollierten Host; ZIH liefert Durchsatz/Latenz + ggf. PAPI-Subset. **Muss im Mess-Kapitel ehrlich dokumentiert werden.**
2. **Multiplexing >4 Events** (Skylake 4 GP-Counter): Zweit-Lauf oder Zeit-Multiplex mit Genauigkeitsverlust — Entscheidung: lieber 4 Events sauber als 6 gemultiplext?
3. **RAPL <1ms unzuverlässig** → Mindest-Workload-Dauer / Sub-Sekunden-Integration; Warm-up-Verwerfer von Batch-Zähler auf **sekundenbasiert** umstellen.
4. **aarch64-Cross-Compile (GH200)** heute ungelöst (Toolchain-File fehlt, Windows-Cross-TODO). Annahme bis dahin: ZIH-Precompile nur für x86_64-Linux-Targets (Barnard/Capella), GH200 später.
5. **Vendor-Event-Code-Korrektheit** = **Annahme** (Tabelle 2.3), pro Event web-/doku-verifizieren, bevor Zahlen belastbar gelten (Direktive „keine Erfolgsmarke ohne literale Ausgabe").
6. **`comdare_hw_counters_v1` (6 Felder) vs. neuer 22-Feld-POD**: bewusste Trennung — schmaler C-POD = DLL-Roh-Transport, breiter Snapshot = host-seitige Konsolidierung. Falls die Diplomarbeit EINEN ABI-POD bevorzugt, wäre `comdare_hw_counters_v1` per Major-Bump zu erweitern (ABI-Bruch) — offene Designentscheidung.

### Relevante Dateien (absolut)
- `…/ce/libs/cache_engine/anatomy/observable_tier.hpp` (neuer Snapshot)
- `…/ce/libs/cache_engine/include/cache_engine/abi/module_abi_v1.hpp` (`comdare_hw_counters_v1`/`pull_live_counters` vorhanden — verkabeln)
- `…/ce/libs/cache_engine/anatomy/abi_adapter.hpp`
- `…/ce/libs/cache_engine/builder/pruef_dock/{pruef_dock,search_algorithm_dock,pruef_dock_sequencer}.hpp` (+ neu `arch_evaluator.hpp`)
- `…/ce/libs/cache_engine/builder/anatomy_commands/tier_observe_trace_abi.hpp`
- `…/ce/libs/cache_engine/builder/commands/{execution_result,execute_engine_command,result_aggregator}.hpp`
- `…/ce/apps/f15_compare/main.cpp`
- `…/Code/02_messung_driver/{measurement_writer,v32_orchestrator,op_type_filter}.hpp`
- `…/ce/cmake/{platform_detection,isa_features,anatomy_codegen,anatomy_codegen_runner,adhoc_emitter}.cmake` (+ neu `platform_fit_check.cmake`, `precompile_mode.cmake`)
- `…/ce/CMakeLists.txt` (Z. 23-25: `AUTO_DETECT`-Modus)
- Neu: `…/ce/libs/cache_engine/include/cache_engine/measurement/i_measurement_source.hpp` + Vendor-Impls; `ce/apps/precompile_batch_builder/`
