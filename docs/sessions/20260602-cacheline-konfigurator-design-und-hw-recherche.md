# Diplomarbeit-Cache-Line-Konfigurator — Design & HW-Recherche (2026-06-02)

> Geerdet durch die Kartierungs-Workflows `wf_438b567b` (Kardinalität) + `wf_6bd2ac54` (Konfigurator-Grundlage)
> und Web-Recherche zu HW-Cache-Line-Settings. Vorgänger: `20260601-kartesik-kardinalitaet-ladebalken-mess-scope.md`.
> Status: DESIGN freigegeben (User-Entscheidungen 2026-06-01/02), Umsetzung als TODO-Liste unten.

## 0. Zweck

Umstellung der bisher in `tools/permutation_codegen/codegen.cmake` **hartcodierten** Permutations-Profile
(smoke/medium/full) auf einen **Diplomarbeit-ansteuerbaren XML-Profil-Konfigurator**. Kern: das Cache-Line-aware
Experiment — bekannte Paper-Suchalgorithmen als Basis-Tiere in Originalkonfiguration, gegen die nur die
cache-line-relevanten Achsen permutiert werden.

## 1. Leit-Prinzipien (User-Direktiven 2026-06-01/02) — VERBINDLICH

1. **Alle physischen Achsen/Organe bleiben erhalten.** Der Konfigurator ist eine reine **Konfigurations-Auswahlschicht**;
   nichts wird entfernt/abgeschaltet. Das Experiment bleibt generisch für andere Fachbereiche nutzbar (andere Aspekte
   bestehender Paper-Algorithmen).
2. **Basis-Permutationen = Paper-Originale per Konfiguration.** Jedes Basis-Tier fixiert alle Achsen auf die
   Paper-Standardwerte → die Originale entstehen per Definition der Konfiguration.
3. **Nur cache-line-relevante Achsen werden permutiert** (+ die neue Cache-Line-Unterachse, §4).
4. **3 Permutationsmodi pro Achsen-Teilmenge beschränkbar** (Stufe 1 ce-only / Stufe 2 Prüfling-Replace / Stufe 3 Full-Join).
5. **Concurrency 1/2/4 Kerne** — echte Mess-Dimension (nicht fixiert).
6. **Telemetrie konfigurierbar** (nicht still aus, §8).
7. **Wiederholungen Default 3** — alle Läufe vollständig + separat dokumentiert, **NIE interpoliert**, höchstens separat
   im Diagramm überlagert (§9).
8. **Codegen via C++/CEB-Generator-Tool** (kein Python in der Buildchain).
9. **Skala: zweistellige Millionen Rekombinationen akzeptiert** (ZIH: tausende Nodes, Prof. Habich).

## 2. Achsen-Inventar (drei Zählweisen)

| Ebene | Zahl | Bedeutung |
|---|---|---|
| Physische Achsen-Organe | **23** (15 Topics, 230+ Varianten) | Implementierungs-Auswahlmenge — bleibt vollständig erhalten |
| Logische Registry-Achsen | **14** primär (+ Sub-Achsen) | `axis_library_registry.hpp:20` |
| **XML-Konfigurator-Achsen** | **11** (`permutation_axes.xml`) | **autoritative Oberfläche** — alle 30 SOTA-Profile + prt-art serialisieren darauf |

### Cache-Line-Relevanz der 11 XML-Achsen

| Achse | physisch | Var. | Relevanz | Rolle im Experiment |
|---|---|---|---|---|
| **layout** | axis_05_memory_layout | 6 | HIGH | Kernfokus; einzige, die der Run-Body heute schon real verzweigt |
| **node** | axis_04_node_type | 8 | HIGH | **aktuell TOTE Achse** (nur String-`#define`) → echt zu verdrahten |
| **page** | axis_01_page_type | 10 | HIGH | Seiten-Topologie (Knoten/Line) |
| **traversal** | axis_03a_search_algo (19 SOTA) | 5 | HIGH | Algorithmus + Besuchssequenz |
| **prefetch** | axis_07_prefetch | 6 | HIGH | L1/L2-Platzierung vor Nutzung |
| **isa** | axis_09/09b SIMD | 6 | HIGH | AVX-512 = 64 B = 1 Line; real divergierend |
| value_handle | axis_14_value_handle | 5 | MEDIUM | fixiert (Tier-Original) |
| **allocator** | axis_06_allocator | 16 | MEDIUM | **Teilmenge std/jemalloc/mimalloc (×3)** permutiert |
| **concurrency** | axis_08_concurrency | 9 | NONE→**Mess-Dim** | **1/2/4 Kerne** (User) |
| telemetry | axis_11_telemetry | 4 | NONE | konfigurierbar (§8) |
| reclamation | (logisch) | 5 | NONE | fixiert (NONE) |

## 3. Achsen-Auswahl (ENTSCHIEDEN, User 2026-06-02)

- **Permutiert (compile-time, eigenes Binary):** layout, isa, traversal, **node (echt verdrahtet)**, page, prefetch,
  allocator(×3), **Cache-Line-Unterachse (size/align/sw-hint, §4)**, **workload (YCSB A–F)**.
- **Permutiert (architektonische Laufzeit-Ausnahme, Launcher-gesetzt, §7):** concurrency-Core-Anzahl (1/2/4 via Affinität),
  HW-Prefetcher-Zustand (MSR).
- **Orchestrierung:** Wiederholungen ×3 (§9).
- **Fixiert (Tier-Original):** value_handle, reclamation; telemetry konfigurierbar.

## 4. NEUE Cache-Line-Unterachse (Kern der Anlage)

**User-Auftrag:** Eine neue Unterachse bei **allocator, page, traversal, node**, die die Cache-Line-Einstellung
permutierbar macht; **ALLE betroffenen Tier-Achsen-Algorithmen werden erweitert**, diese Einstellung zu unterstützen.

### 4.1 HW-Recherche: einstellbare Cache-Line-Settings je Architektur (Web, 2026-06-02)

| Architektur | Cache-Line-Größe | HW-einstellbar | Mechanismus |
|---|---|---|---|
| **Intel x86** | 64 B (fix) | HW-Prefetcher per-Core | **MSR `0x1A4`**, Bits 0–3: (0) L2 DPL-Streamer, (1) L2 Adjacent-Line, (2) DCU Next-Line (L1), (3) DCU-IP. Bit=1 → Prefetcher AUS |
| **AMD Zen** | 64 B (fix) | HW-Prefetcher per-Core | MSR-Region `0xC0011020–0xC001102B` (DC/L2-Prefetch-Config) |
| **ARM** | **64 / 128 / 256 B** (variabel!) | Größe je Kern; Prefetch IMP_DEF | `CTR_EL0` (CWG) meldet Größe; 64 B Cortex/Neoverse, 128 B Apple M/ThunderX, **256 B A64FX** |
| **IBM POWER** | 128 B | Prefetch-Tiefe/Richtung | `dcbt`/`dcbtst` mit Direction/Depth/Units + Partial-Block |
| **RISC-V** | implementierungsabhängig | Zicbop-Prefetch | `PREFETCH.R/W/I` (Zicbop), `CBO.*` (Zicbom/Zicboz) |

**Software-Seite (architekturübergreifend, compile-time + runtime):**
- **Alignment:** `alignas(64/128/256)` auf Knoten/Pages → Struktur an Cache-Line-Grenze.
- **False-Sharing-Padding:** Felder auf eigene Line padden (`hardware_destructive_interference_size`).
- **SW-Prefetch-Hints:** `_mm_prefetch` mit `_MM_HINT_T0` (alle Levels), `T1` (L2+), `T2` (L3+), `NTA` (non-temporal,
  cache-pollution-arm) — analog `__builtin_prefetch`, POWER `dcbt`, RISC-V `PREFETCH.R`.
- Empirisch: 128-B-Tuning brachte +85 % bei `copy_page` auf ThunderX → Cache-Line-Größen-Annahme ist messbar relevant.

### 4.2 Werte-Modell der Unterachse (Vorschlag)

Die Unterachse `cacheline` ist ein **strukturiertes, architekturübergreifendes** Knob-Tupel:

| Sub-Knob | Werte | Compile/Runtime |
|---|---|---|
| `line_size_target` | 64 / 128 / 256 B | **compile-time** (Alignment/Packing der Struktur) |
| `alignment_policy` | none / cacheline_aligned / padded_no_false_share | **compile-time** |
| `sw_prefetch_hint` | none / T0 / T1 / T2 / NTA | **compile-time** (`_mm_prefetch` mit konstantem Hint, gebacken) |
| `hw_prefetcher_state` | all_on / adjacent_off / dcu_off / all_off | **Laufzeit-Ausnahme** (MSR 0x1A4 / AMD; Launcher, §7) |

Primär-permutiert für die Thesis (compile-time): `line_size_target` × `alignment_policy` (= 3×3 = **9** strukturelle
Cache-Line-Varianten), optional × `sw_prefetch_hint`. Der `hw_prefetcher_state` ist die EINZIGE Laufzeit-Ausnahme dieser
Unterachse (CPU-Register, nicht einkompilierbar) und wird vom Launcher gesetzt (§7).

### 4.3 Querschnitt-Charakter

`cacheline` ist EINE globale Einstellung, die die betroffenen Organe (allocator/page/traversal/node) **gemeinsam**
honorieren (nicht 4 unabhängige → würde `9^4` explodieren). Alle betroffenen Tier-Algorithmen (per Benennung: ART-Node256,
HOT, B+-Page, Allocator-Pools …) werden um die Unterstützung erweitert (Goldstandard-Achsen-Vorlage: Concept + CEPS +
Subaxes + CRTP-Base + Wrapper + flags.hpp.in + Registry, vgl. `axis_06_allocator`).

## 5. XML-Schema `comdare_thesis_profile` (Entwurf V1)

```xml
<comdare_thesis_profile id="cacheline_study_v1" schema_version="1">
  <base_tiers>                          <!-- Paper-Originale, referenzieren sota/*.profile.xml -->
    <tier id="art"  profile_ref="sota/art.profile.xml"  paper_ref="P01"/>
    <!-- hot, masstree, coco, start, b2tree, wormhole, surf ... -->
  </base_tiers>

  <permute_axes>                        <!-- nur cache-line-relevante Struktur-Achsen (compile-time) -->
    <axis ref="layout"/> <axis ref="isa"/> <axis ref="traversal"/>
    <axis ref="node"/> <axis ref="page"/> <axis ref="prefetch"/>
    <axis ref="allocator"><use>std</use><use>jemalloc</use><use>mimalloc</use></axis>
    <axis ref="cacheline">              <!-- NEUE Unterachse, §4 -->
      <line_size>64</line_size><line_size>128</line_size><line_size>256</line_size>
      <alignment>none</alignment><alignment>cacheline_aligned</alignment><alignment>padded</alignment>
    </axis>
  </permute_axes>

  <!-- workload + telemetry sind COMPILE-TIME (je Binary gebacken, kein Runtime-Switch) -->
  <compile_dims>
    <workloads>A B C D E F</workloads>
    <telemetry mode="off"/>             <!-- §8: off|leaf_sampled|all; Pfad B host-seitig separat -->
  </compile_dims>
  <runtime_exceptions>                  <!-- HW/OS-Zustand, Launcher-gesetzt — NICHT einkompilierbar (§7) -->
    <affinity_cores>1 2 4</affinity_cores>                     <!-- permutiert via taskset/numactl -->
    <hw_prefetcher>all_on adjacent_off all_off</hw_prefetcher>  <!-- permutiert via MSR 0x1A4 -->
    <fixed_conditions turbo="off" smt="off" aslr="off" numa="local" governor="performance"/>
    <repetitions count="3" interpolate="false" overlay_in_chart="true"/>
  </runtime_exceptions>

  <modes>                               <!-- 3 Stufen, je Achsen-Teilmenge -->
    <mode name="ce_only"          merge="Stufe1_CeOnly"          active_axes="layout isa traversal node page prefetch allocator cacheline"/>
    <mode name="pruefling_replace" merge="Stufe2_PrueflingReplace" active_axes="isa cacheline"
          pruefling="prtart" replaces_axes="page node traversal value_handle allocator prefetch layout"/>
    <mode name="full_join"        merge="Stufe3_FullJoin"        active_axes="layout isa traversal node page prefetch allocator cacheline"/>
  </modes>

  <fixed_axes>
    <axis ref="value_handle" inherit="tier"/> <axis ref="reclamation">NONE</axis>
  </fixed_axes>
  <constraints>
    <require axis="isa" host_supported="true"/>
    <pin tier="art" axis="node" to="SPARSE_NODE4_ART"/>
  </constraints>
</comdare_thesis_profile>
```

## 6. Paper-Basis-Tiere (Originalkonfig, aus `sota/*.profile.xml`)

ART (P01), HOT (P02), Masstree (P03), CoCo-Trie (P04), START (P05), B2Tree (P06), Wormhole (P07), SuRF (P10)
+ **PRT-ART** (Prüfling, nur Stufe 2/3). Beispiel ART:
`page=DENSEBYTE_ART256, node=SPARSE_NODE4_ART, traversal=STANDARD, value_handle=EXTERNAL, allocator=MIMALLOC,
layout=CACHE_LINE_ALIGNED, isa=X86_AVX2, prefetch=NONE` (verifiziert `art.profile.xml`). Set anpassbar.

## 7. Compile-Time-Everything + architektonische Laufzeit-Ausnahmen

**KORREKTUR (User 2026-06-02):** Es gilt die Kern-Direktive *Compile-Time-Only, kein Runtime-Switch im Hot-Path*
(statische Metaprogrammierung, `if constexpr`, kein `std::variant`/`virtual`/Runtime-Branch). **ALLE Achsen werden
compile-time in distinkte Binaries gebacken** — inkl. workload-Typ, telemetry-Modus und cacheline-Struktur. Das
**ZIH-Sonderbudget** (Prof. Habich, „wir dürfen das") trägt die resultierende Binary-Zahl; die Compile-Zahl ist KEIN
limitierender Faktor mehr (anders als das reguläre 20.000-core-h-Kontingent).

**Architektonische Laufzeit-Ausnahmen** — nur HW/OS-Zustand, der sich physisch NICHT einkompilieren lässt; vom
SLURM-Launcher je Lauf gesetzt (kanonische reproduzierbare-Mikrobench-Bedingungen, web-recherchiert, Quellen §12):

| Laufzeit-Ausnahme | Rolle | Mechanismus |
|---|---|---|
| **Core-Affinität / -Anzahl (1/2/4)** | PERMUTIERT (concurrency-Werte) | `taskset` / `numactl --physcpubind` |
| **HW-Prefetcher-Zustand** | PERMUTIERT (cacheline-Knob) | MSR `0x1A4` (Intel) / `0xC001102x` (AMD) via `wrmsr` |
| **NUMA-Bindung** | fixe Bedingung | `numactl --membind`, Auto-Balancing aus |
| **Turbo / Frequenz** | fixe Bedingung | `performance`-Governor, no-turbo |
| **SMT / Hyperthreading** | fixe Bedingung | BIOS / sysfs aus |
| **ASLR** | fixe Bedingung | `setarch -R` / `kernel.randomize_va_space=0` |
| **Huge Pages** | optional permutiert | `madvise` / THP-sysfs |
| **Wiederholungen (×3)** | Orchestrierung | Launcher führt jedes Binary N× aus (§9) |

→ **Permutierte Laufzeit-Faktoren** (cores × hw_prefetcher × ggf. hugepages) und **Wiederholungen** multiplizieren die
**Lauf-Zahl je Binary**, NICHT die Compile-Zahl. **Fixe Bedingungen** (turbo/SMT/ASLR/NUMA/governor) sind für ALLE Läufe
konstant — Reproduzierbarkeit.

**Kombinations-Schätzung (compile-time-strukturell):**
layout 5 × isa 3 × traversal 5 × node 8 × page 10 × prefetch 3 × allocator 3 × cacheline 9 × workload 6 =
**~87,5 Mio Binaries** bei voll geöffnetem Achsenraum. Mit **Tier-Pinning** (jedes Paper-Tier fixiert die meisten Achsen
auf sein Original und öffnet nur eine im Profil deklarierte Wolke) realistisch **10⁴–10⁶ Binaries je Tier × 8 Tiere**.
× **Laufzeit-Faktoren** (cores 3 × hw_prefetcher ~4 × Wiederholungen 3 = 36 Läufe/Binary) → **Mess-Läufe in den
zweistelligen Millionen+**. Stellschraube = Tier-Öffnungsgrad (je `<tier>` deklariert, §11). ZIH: SLURM-Array über
FNV1a-Fingerprint je `PermutationDescriptor` (existiert bereits), Singularity-Container, Ergebnis-Webhook (VLAN 60).

## 7-A. Algorithm_Resource_Control — Laufzeit-Steuerschnittstelle am Prüf-Dock

**User-Direktive 2026-06-02:** Einige Achsen bieten Parameter, die ZUR LAUFZEIT steuerbar sein müssen (nicht nur
compile-time). Dafür eine NEUE Steuerungsschnittstelle am Prüf-Dock: **`Algorithm_Resource_Control`** — getrennt vom
Mess-Pfad und **aktiv auch bei abgeschalteter Messung** (eigene Kontroll-Ebene, kein Heisenberg-Effekt auf die Messung).
Dies ist die DRITTE Property-Kategorie neben (1) compile-time-gebacken (§3/§7) und (2) HW/OS-Laufzeit-Ausnahmen (§7).

**Aufbau:**
- Besteht aus **Sub-Structs je laufzeit-einstellbarem Achsen-Interface** (ein Control-Struct pro betroffener Achse).
- **ALLE Organ-Algorithmen einer Achse** müssen die Control-Operationen mit-implementieren (Pflicht-API, analog den
  Goldstandard-Achsen-Properties; Achsen-CRTP-Base liefert No-op-Defaults für nicht-betroffene Organe).
- ABI: **additives Sub-Interface** (analog `IObservableTier`/`IScannableTier`) via `dynamic_cast` aus dem geladenen
  Permutations-DLL — NIEMALS in-place an bestehenden Interfaces ändern (SEH-0xc0000005-Lektion).

**Laufzeit-steuerbare Eigenschaften (Beispiele, je Achse als Sub-Struct):**

| Achse | runtime-Control-Property | Operation (Beispiel) |
|---|---|---|
| concurrency | **Thread-Anzahl** (ursprünglich Laufzeit-Variable) | `set_thread_count(n)` |
| prefetch | Prefetch-Distanz / -Tiefe | `set_prefetch_distance(d)` |
| allocator | Arena-/Pool-Größe, Growth-Policy | `set_pool_budget(bytes)` |
| traversal | Batch-Größe / Working-Set | `set_batch_size(n)` |
| value_handle | Inline/External-Schwelle | `set_inline_threshold(bytes)` |

**Abgrenzung zu §7:** §7-Laufzeit-Ausnahmen = HW/OS-Zustand (Launcher/MSR/Affinität, AUSSERHALB des Algorithmus).
`Algorithm_Resource_Control` = algorithmus-INTERNE Laufzeit-Properties (IM DLL, vom Prüf-Dock gesetzt). Die
**Thread-Anzahl** ist beides: OS-Affinität (welche Kerne, §7) + algorithmus-interne Thread-Spawn-Zahl (hier).

## 8. Telemetrie — warum „aus" vorgeschlagen wurde + konfigurierbare Lösung

Telemetrie-AN fügt **pro Operation** Beobachter-Overhead ein, der genau die Latenz **verfälscht**, die Pfad A
(in-DLL `run_workload`) misst (Heisenberg-Effekt). Daher Default `off` für reine Latenzläufe. **ABER:** die
Achsen-Observer-Daten (Pfad B) liefern gerade die per-Achsen-Korrelation des Hybrid-Messmodells. Lösung:
`telemetry` ist eine **konfigurierbare Mess-Dimension** (`off | leaf_sampled | all`); Pfad B erhebt die Observer
**host-seitig** (CEB, ABI-stabiler `observe_all`) entkoppelt vom gemessenen Binary, sodass volle Observability
OHNE Latenz-Verfälschung möglich ist. Default-Wahl bleibt User-Entscheidung (offen, §11).

## 9. Wiederholungen / Mess-Integrität

`<repetitions count="3" interpolate="false">` (Default 3, konfigurierbar). **Jeder** der N Läufe wird **vollständig
+ separat** persistiert (eigene Zeile/Datei je Wiederholung, eigener Wall-Clock-Stempel) — **nie gemittelt/interpoliert**.
Im Diagramm höchstens **separat überlagert** (N Kurven). Erfordert: Mess-Treiber-Schleife `for rep in 1..N`,
CSV-Schema um `repetition_index` erweitern, Diagramm-Generator Overlay-Modus. (Konsistent mit Direktive
„keine Erfolgsmarken ohne literale Ausgabe".)

## 10. Migrations-/Build-Plan (→ TODO-Liste)

0. Additiv: `permutation_axes.xml` bleibt Single-Source der Wertebereiche; `comdare_thesis_profile` NEU unter
   `Code/experiment_config/`; KEINE `sota/*.profile.xml` ändern (Rückwärts-Kompatibilität).
1. **tinyxml2-Migration** des `XmlConfigParser` (verschachtelte Elemente: modes>active_axes, constraints, cacheline).
2. **Cache-Line-Unterachse** anlegen (Goldstandard-Vorlage) — Concept/CEPS/Subaxes/Wrapper/Registry.
3. **Betroffene Tier-Algorithmen** (allocator/page/traversal/node) um Cache-Line-Support erweitern.
4. **node** echt verdrahten (Run-Body-Divergenz je Node-Format statt String-`#define`).
5. **Algorithm_Resource_Control** (§7-A): Laufzeit-Steuerschnittstelle am Prüf-Dock + Sub-Structs je Achse; alle Organe
   einer Achse implementieren die Control-Ops (additives ABI-Sub-Interface).
5b. **concurrency:** Thread-Anzahl via `Algorithm_Resource_Control` (Runtime) + Core-Affinität via SLURM-Launcher.
5c. **SLURM-Launcher** (§7): HW/OS-Ausnahmen setzen (Affinität, HW-Prefetcher-MSR, governor, SMT/ASLR/NUMA, Hugepages).
6. **C++/CEB-Generator-Tool** (kein Python): liest Profil-XML → generiert `perm_<id>.cpp`; bricht den
   5-Achsen-Deckel von `codegen.cmake`.
7. **PermutationLoop::enumerate** dynamisch über `cfg.axes_profiles[active_axes]` (Fallback: alte 4-Achsen-Logik).
8. **3-Modi-Achsen-Maske** beim Enumerate anwenden (Stufe2 = nur nicht-ersetzte Achsen; Stufe3 = Union).
9. **Wiederholungen** (§9) im Treiber + CSV + Diagramm-Overlay.
10. **Telemetrie konfigurierbar** (§8).
11. **ZIH-Skalierung:** SLURM-Array über Fingerprints + Singularity + Webhook.
12. **Thesis-Anbindung:** Profil → `generate_measurement_appendix` mit 3-Wiederholungs-Overlay.

## 11. Offene Mikro-Entscheidungen

- **Cacheline-Werte-Set:** reicht `line_size{64,128,256} × alignment{none,aligned,padded}` (=9), oder zusätzlich
  `sw_prefetch_hint` (T0/T1/T2/NTA) und `hw_prefetcher_state` (MSR) mit aufnehmen?
- **Cacheline global vs per-Organ:** global (1 Einstellung, alle Organe honorieren) — bestätigen (vs per-Organ-Explosion).
- **Tier-Öffnungsgrad:** wie weit fächert jedes Paper-Tier seine gepinnten Achsen auf (steuert die Compile-Zahl)?
- **Telemetrie-Default:** `off` (reine Latenz) + Pfad-B-Observer host-seitig — bestätigen?

## 12. Quellen (Web, 2026-06-02)

- Intel HW-Prefetch-Control / MSR 0x1A4: Intel Community „How to control the four hardware prefetchers", Intel Atom
  Hardware-Prefetch-Controls (357930), `deater/uarch-configure`.
- ARM Cache-Line-Größen: GCC-Patch „Armv9-A generic L1 cache line size 64B", ThunderX 128B `copy_page`-Patch (linux-arm-kernel),
  A64FX 256B; Lemire „Measuring the size of the cache line empirically".
- AMD Zen Prefetch-MSR: WikiChip Zen/Zen2, XMRig MSR-Guide.
- SW-Prefetch: felixcloutier `PREFETCHh`, `_mm_prefetch`-Hints (Rust core::arch Doku / Intel), LWN „Memory part 5".
- POWER: PowerISA 2.06 Stride & Prefetch, „Under the Hood: POWER7 Caches".
- RISC-V: RISC-V CMO-Spec (Zicbom/Zicbop/Zicboz), OpenJDK Zicbop-Support.
- Reproduzierbare CPU-Mikrobench-Laufzeit-Bedingungen (§7-Ausnahmen): Google Benchmark `reducing_variance`,
  Easyperf „consistent results benchmarking on Linux", Linux-Kernel `cpufreq`/Performance-Governor, V. Stinner
  „Intel CPUs: Turbo Boost"; Affinität/NUMA: `taskset`/`numactl` (ADMIN-Magazine Processor & Memory Affinity Tools).
