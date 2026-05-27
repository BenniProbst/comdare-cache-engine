# 16 — axis_05_memory_layout: CPU IMC Runtime-Heuristik (Doku-Only Sprint)

**Stand:** 2026-05-27
**Status:** OFFEN — Pflicht-Doku fuer R7.7-Backlog
**Verwandt:** Doku 15 §3 (R7.7 Skizze), axis_05_memory_layout
**Task:** #724

## §1 Kontext

User-Direktive 2026-05-27: "Im memory_layout Topic kann die CPU IMC
(Integrated Memory Controller) per Laufzeit verwaltet werden. Das laesst sich
heuristisch ausnutzen. NUR DOKU + Web-Recherche."

Diese Doku konsolidiert die Web-Recherche-Ergebnisse zu IMC-Runtime-Management
fuer eine spaetere Implementierung (Sprint nach R7.5/R7.6/R7.7).

## §2 Intel IMC Runtime-APIs

### §2.1 Hardware-Architektur

| CPU-Generation | IMC-Eigenschaften |
|----------------|-------------------|
| Sapphire Rapids (3. Gen Xeon Scalable) | 8 Kanaele DDR5, 4800 MHz, 8 IMCs/CPU |
| Granite Rapids (5. Gen Xeon Scalable) | bis 12 Kanaele DDR5-8800, 4 Kanaele/XCC-Tile |
| Alder Lake/Raptor Lake (Hybrid) | DDR4/DDR5 dual-mode, IMC pro Tile |
| Meteor Lake (Hybrid 2024) | DDR5 + LP5X, separate SoC-Tile-IMC |

### §2.2 DDR5 Sub-Channel Architecture

DDR5 splittet den 64-bit Memory-Pfad in **2x 32-bit unabhängige Sub-Kanaele**
(neue Architektur vs. DDR4). Das ermoeglicht hoehere Concurrent-Request-Throughput
bei kleineren Granularitaeten (cache-line size).

### §2.3 Sub-NUMA Clustering (SNC) + IMC Interleaving

Sapphire Rapids+ unterstuetzen:
- **Flat Mode + SNC4**: 4 Sub-NUMA-Cluster pro Socket
- IMC-Interleave-Settings muessen entsprechend konfiguriert werden fuer LLC-Effizienz
- Runtime-Tuning via MSR (siehe §4) NICHT empfohlen, Stabilitaet Hazard

### §2.4 Intel APIs (offiziell dokumentiert)

- **Intel CMT-CAT** (Cache Monitoring Technology + Cache Allocation Technology):
  `intel-cmt-cat/wiki` — `rdtset` CLI fuer MBA/SMBA Memory-Bandbreite-Allocation
- **RAPL** (Running Average Power Limit, ab Sandy Bridge):
  Domains Package/DRAM/Core/Uncore, MSR-Interface
- **MBM** (Memory Bandwidth Monitoring): perf-Events
  `llc_local_bw`, `llc_total_bw` (LLC-perspective)

## §3 AMD IMC Runtime-APIs

### §3.1 HSMP (Host System Management Port)

- Mailbox-basiertes Protokoll via `/dev/hsmp` (misc device)
- Funktionalitaet: Power Limits, Frequency Control, **Bandwidth Monitoring**,
  Thermal Management
- Verfuegbar ab EPYC Serverline, auch Threadripper (Kernel-Docs bestaetigen)

### §3.2 NPS (NUMA Per Socket) Configuration

| Mode | NUMA-Domains/Socket | Memory-Interleave |
|------|---------------------|-------------------|
| NPS0 | 1 (System-weit) | alle Kanaele interleaved |
| NPS1 | 1 pro Socket (Default) | 6 Kanaele/Node interleaved |
| NPS2 | 2 | 6 Kanaele/Domain (3 pro Domain) |
| NPS4 | 4 (HPC-Workloads) | 3 Kanaele/Domain |

**Wichtig:** NPS-Aenderung erfordert BIOS-Setting + Reboot. Runtime-Erkennung
ueber `libnuma`/`hwloc` ist moeglich.

### §3.3 AMD Tools

- **amd-hsmp**: Mailbox-Interface zu System-Management
- **amd-pstate**: Frequency Scaling Driver (Full MSR / Shared Memory Modes)
- **AMDuProf**: User-mode Profiler (RAS, IBS, Memory Profiling)

## §4 Linux Tools & Runtime-Monitoring

### §4.1 msr-tools (gefaehrlich — nur Dev/Debug)

```bash
# MSR lesen (CPU 0, MSR-Adresse hex)
rdmsr -p 0 0x1A

# MSR schreiben — KANN KERNEL-STATE BESCHAEDIGEN!
wrmsr -p 0 0x1A 0x42
```

**Warnung:** wrmsr kann Kernel-Writes clobbern. Nur in kontrollierten Umgebungen
(BIOS-locked, single-purpose Test-Maschinen) verwenden.

### §4.2 perf PMU (Performance Monitoring Unit)

**perf c2c (Cache-to-Cache):**
- Analyse Cacheline-Contention, HITM (Hit-to-Modified)
- Intel: Load-Latency + Precise-Store Events
- AMD: IBS-op PMU (NICHT auf Zen3 verfuegbar)
- ARM64: SPE (Statistical Profiling Extension)

**perf mem:** Memory-Access-Profiling pro Cache-Level

**Intel Uncore IMC PMU:** pro-IMC Messung, mehrere summieren fuer Socket-Total

**AMD UMC Events:** `amd_umc/umc_cas_cmd.all/`, pro-UMC Monitoring

### §4.3 hwloc / numactl

- **hwloc**: System-Topologie-Discovery (NUMA-Nodes, LLC, Caches), API + CLI (lstopo)
- **numactl**: NUMA-Policies (first-touch, interleave, restricting)
- **libnuma**: Programmgesteuert NUMA-Queries + Page-Migration zur Laufzeit
- **hwloc-bind**: Logical-Object Binding (vs. physikalische Indices)

## §5 DBMS-relevante IMC-Heuristiken

### §5.1 Verifizierte Patterns

1. **Page Coloring / LLC-Level**: OS kontrolliert Virtual→Physical Mapping fuer
   Cacheline-Locality (mehrere Papers)
2. **Adaptive Data Placement**: SAP HANA + andere In-Memory-DBs tracken
   Utilization pro Socket/Task, migrieren Tables dynamisch bei Imbalance
3. **First-Touch Policy + Dynamic Migration**: Intelligente Allocation auf
   "Home-Node" + Runtime-Defragmentation bei NUMA-Contention
4. **NUMA-aware Scheduler**: Workload-bewusste Task/Thread-Placement auf
   lokalen Cores + Daten
5. **Bandwidth-Sensing Heuristic**: IMC-Counter auslesen (perf/RAPL) → wenn
   Bandbreite gesaettigt → Workload-Throttling oder Prefetch-Policy anpassen

### §5.2 Wissenschaftliche Papers (Pflicht-Vollangaben)

1. **Psaroudakis, P. et al.** "Adaptive NUMA-aware Data Placement and Task
   Scheduling for Analytical Workloads in Main-Memory Column-Stores."
   *Proceedings of the VLDB Endowment*, Vol. 10, No. 1, 2016.
   URL: http://www.vldb.org/pvldb/vol10/p37-psaroudakis.pdf

2. **SAP HANA-Team.** "NUMA-aware Memory Management with In-Memory Databases."
   ResearchGate / SAP HANA Architecture, 2016.

3. **Drebes, A. et al.** "NUMA-aware Scheduling and Memory Allocation for
   Data-flow Task-parallel Applications."
   *ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming
   (PPoPP)*, 2016.
   DOI: 10.1145/2851141.2851193

4. **Yamada, K. et al.** "HMB: Scheduling PREM-Like Real-Time Tasks at High
   Memory Bandwidth." *Dagstuhl 2024.*
   (Real-time-spezifisch, aber Memory-Throttling adaptiv)

## §6 Implementierungs-Hinweise fuer axis_05_memory_layout

### §6.1 Vorgeschlagene neue Wrapper-Klassen

**Naming-Konvention** (R7.5.k Goldstandard): `[Strategy]MemoryLayout`

| Wrapper | Beschreibung | Aufwand SP |
|---------|--------------|------------|
| `AdaptiveChannelInterleavingMemoryLayout` | Runtime-Tuning Sub-Channel + IMC-Interleave | 8 |
| `NumaPageColoringMemoryLayout` | Page-Coloring auf LLC/Bank/Controller-Ebenen | 5 |
| `BandwidthAdaptiveMemoryLayout` | IMC-Counter-Sensing + Workload-Throttling | 8 |
| `FirstTouchDynamicMigrationMemoryLayout` | SAP-HANA-Pattern: Adaptive Umverteilung | 5 |
| `SncFlatMemoryLayout` | Intel SNC4 Flat-Mode (Sapphire Rapids+) | 3 |
| `NpsAwareMemoryLayout` | AMD NPS0/1/2/4 Domain-Aware | 3 |

**Gesamtaufwand:** ~32 SP (kleiner Sprint im Vergleich zu R7.6 Paper-Identifikation).

### §6.2 Concept-Erweiterungen

```cpp
// Pseudo: Optional Sub-Concept fuer Runtime-IMC-Awareness
namespace axis_05_memory_layout::concepts {

template <typename L>
concept RuntimeImcAware = MemoryLayoutStrategy<L> && requires {
    { L::supports_runtime_tuning() } noexcept -> std::convertible_to<bool>;
    { L::imc_monitoring_interface() } noexcept -> std::convertible_to<std::string_view>;
    // z.B. "perf-pmu", "msr-tools", "hsmp"
};

template <typename L>
concept BandwidthSensitive = MemoryLayoutStrategy<L> && requires {
    { L::estimated_bandwidth_overhead_pct() } noexcept -> std::convertible_to<float>;
    { L::target_workload_class() } noexcept -> std::convertible_to<std::string_view>;
    // z.B. "OLAP", "OLTP", "Streaming"
};

}
```

### §6.3 Integration mit anderen Achsen

- **axis_09 (ISA)**: NUMA-Topologie + CPU-Sockel-Count beeinflussen IMC-Strategie
- **axis_09b (SIMD)**: Bandbreiten-Throttling kann SIMD-Throughput beeinflussen
- **axis_12 (Hardware)**: `numa_capable=true` Voraussetzung fuer NumaPageColoring*
- **PermutationEngine**: `mp_remove_if` Cross-Constraint
  (z.B. `NumaPageColoring` nur wenn `HardwareProfile::numa_capable()`)

### §6.4 Tool-Submodule-Anforderungen

Fuer echte Implementierung waeren neue comdare-Submodule sinnvoll:
- `comdare-hwloc-wrapper`: hwloc C-API Bindings
- `comdare-perf-pmu`: perf PMU Counter via libperf
- `comdare-msr-tools`: rdmsr/wrmsr via /dev/cpu/X/msr (CAP_SYS_RAWIO)
- `comdare-amd-hsmp`: HSMP Mailbox-Protocol

## §7 Herausforderungen + Risiken

| Risiko | Mitigation |
|--------|------------|
| MSR-Write-Stabilitaet | Nur in Dev/Test, Production via BIOS-Settings |
| Hardware-Spezifika (Intel vs AMD) | Abstrahieren via Concepts, Pro-Vendor Wrapper |
| Portabilitaet perf-PMU | hwloc-basierte Abstract-Topology |
| IMC-Counter Overhead | Sampling-Strategie (alle N Cycles, nicht jeder Access) |
| Privileges (CAP_SYS_RAWIO) | Container-Setup dokumentieren, Fallback-Heuristiken |

## §8 Naechste Schritte

1. **R7.6 Paper-Identifikation** (#723) hat hoehere Prio — Habich-Compliance fuer
   die R7.5-Achsen.
2. **R7.7.d** (NEU): Skelett-Implementation der 6 Wrapper als Sprint nach R7.6.
3. **R7.7.e** (NEU): Tool-Submodule (comdare-hwloc-wrapper etc.) als 4-Phasen-Sprint.
4. **R7.7.f** (NEU): Test-Fixtures mit Synthetic Workloads (BenchmarkSuite mit
   hoher IMC-Auslastung).

## §9 Cross-Refs

- Doku 15 §3 (R7.7 axis_05 IMC-Heuristik — Initial-Sketch, durch Doku 16
  ersetzt/erweitert)
- axis_05_memory_layout Wrappers (4 aktuell: CacheLineAligned/AoSStrict/SoA/
  PackedBitmap MemoryLayout)
- Memory: [[reference-isa-layered-extensions-and-avx512-subflags]] (axis_09b
  Topologie-Pattern als Template fuer axis_05)
