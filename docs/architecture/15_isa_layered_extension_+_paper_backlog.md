# 15 — ISA-Schichten-Modell + Paper-Backlog (R7.5/R7.6/R7.7)

**Stand:** 2026-05-27
**Status:** OFFEN — Pflicht-Nacharbeit fuer V41.F.6.1
**Verwandt:** Doku 13 (Paper-Legacy-Architektur), Doku 14 (Achsen-Komposition Organ-Metapher)

## §1 Kontext

User-Direktive 2026-05-27 fuer die Achsen-Reifung nach R7.5 (Optional-Topics) erfasst vier Pflicht-Themen, die nach R7.5 noch nicht abgedeckt sind und in spaeteren Sprints (R7.6 / R7.7 / R7.8) implementiert werden muessen.

## §2 R7.6 — Paper-Identifikation + Original-Code-Validation fuer ALLE R7.5-Achsen

**Hintergrund:** Die 9 R7.5-Achsen (prefetch, telemetry, serialization, value_handle, filter, io, migration, index_organization, isa+simd_extension) sind nicht optional sondern **Pflicht-Achsen**. Sie wurden in R7.5 als Goldstandard-Skelette implementiert (Sub-Achsen + Concepts + flags.hpp.in + Strategy-Base + Wrappers + Registry + TopicConfigSet + CMakeLists), aber:

- Es fehlen **Paper-Quellen-Verlinkungen** pro Wrapper (analog axis_06_allocator mit mimalloc/jemalloc/tcmalloc/snmalloc Pilots)
- Es fehlt **Original-Code-Validation** via `ctsha` SHA256 + `is_original_module()`
- Es fehlt **legacy_code/paper_<id>_<name>/** Skelette mit LICENSE + manifest.txt

**Pattern aus Doku 13 (4-Schichten-Paper-Legacy-Architektur):**

| Schicht | Beschreibung |
|---------|--------------|
| Schicht 1 | legacy_code/paper_<id>_<name>/ mit Original-Source + LICENSE + manifest.txt |
| Schicht 2 | apps/is_original_validator generiert Compile-Time SHA256 pro Function-Body |
| Schicht 3 | Wrapper-Klasse erbt AxisBase-Mixin mit get_compiler() + is_original_module() |
| Schicht 4 | Habich-Compliance: Paper-Code als statisches Linken, KEIN Body-Copy in Wrapper |

**Beispiel-Mapping (R7.6 Phase 1 — filter axis):**

| Wrapper | Paper-Quelle |
|---------|--------------|
| BloomFilter | Bloom CACM 1970 "Space/Time Trade-offs in Hash Coding with Allowable Errors" |
| CuckooFilter | Fan et al. CoNEXT 2014 "Cuckoo Filter: Practically Better Than Bloom" |
| XorFilter | Graf+Lemire 2020 "Xor Filters: Faster and Smaller Than Bloom and Cuckoo Filters" |
| RangeFilter | Zhang et al. SIGMOD 2018 "Surf: Practical Range Query Filtering with Fast Succinct Tries" |

**Aufwand:** 40-60 SP gesamt fuer alle 9 R7.5-Achsen (4-7 SP pro Achse).
**Task:** #723

## §3 R7.7 — axis_05_memory_layout: CPU IMC Runtime-Heuristik

**Hintergrund:** Die CPU IMC (Integrated Memory Controller) ist zur Laufzeit per Kernel-Interface oder MSR (Model Specific Registers) verwaltbar. Das laesst sich heuristisch ausnutzen fuer adaptive Memory-Layout-Strategien.

**Pflicht-Recherche (NUR DOKU + Web-Recherche, KEIN Code in diesem Sprint):**

| Plattform | API/Tool | Beschreibung |
|-----------|----------|--------------|
| Intel | MSR (Model Specific Registers) | Memory-Channel-Interleaving, DDR5 Sub-Channel-Mode |
| AMD | Infinity Fabric IMC | Memory-Controller-Steuerung |
| Linux | perf-mem, msr-tools | Runtime-Monitoring |
| Intel Sapphire Rapids+ | DDR5 Sub-Channel Mode | Heuristik fuer Bandbreite-Optimierung |

**Doku-Ziel:** Anhang an `axis_05_memory_layout` README oder neue Datei `axis_05_memory_layout/IMC_RUNTIME_HEURISTIC.md`.

**Verwandt:** axis_05_memory_layout (Layout-Strategie kann von Runtime-IMC-Info profitieren). Vorerst nur Hinweis-Notiz — Implementierung spaeter wenn Forschungs-Mission das benoetigt.

**Task:** #724

## §4 R7.7.b — axis_09b ISA-Schichten-Modell (SSE/AVX/AVX-512-Flags)

**Hintergrund:** SSE/AVX/AVX-512 sind in Schichten **rueckwaerts-kompatibel** und bauen systematisch aufeinander auf. AVX-512 hat zusaetzliche Sonderbefehl-Flags. Pro Architektur darf nur die freigeschaltete Schicht + freigeschaltete Sub-Flags verfuegbar sein.

### §4.1 SSE-Schichten (jede umfasst Vorgaenger)

| Schicht | Jahr | Einfuehrung | Beschreibung |
|---------|------|-------------|--------------|
| SSE | 1999 | Pentium III | 70 SIMD-Instruktionen, 128-bit XMM-Register |
| SSE2 | 2001 | Pentium 4 | 144 neue Instruktionen, integer + double-precision |
| SSE3 | 2004 | Prescott | 13 neue (DSP, Thread-Synchronisation) |
| SSSE3 | 2006 | Core 2 | 16 neue (Permutation/Multiply-Accumulate) |
| SSE4.1 | 2007 | Penryn | 47 neue (Dot-Product, Streaming-Load) |
| SSE4.2 | 2008 | Nehalem | 7 neue (String-Compare, PCMPESTR/PCMPISTR) |

### §4.2 AVX-Schichten (jede umfasst Vorgaenger)

| Schicht | Jahr | Einfuehrung | Vektor-Breite |
|---------|------|-------------|---------------|
| AVX | 2011 | Sandy Bridge | 256-bit YMM-Register |
| AVX2 | 2013 | Haswell | 256-bit integer + FMA3 |
| AVX-512F | 2016 | Skylake-X / Zen 4 | 512-bit ZMM-Register (Foundation) |

### §4.3 AVX-512 Sub-Flags (Pflicht-Liste, 15+ separate Flags)

| Flag | Name | Hardware | DBMS-Relevanz |
|------|------|----------|---------------|
| AVX-512F | Foundation | Skylake-X+ / Zen 4+ | Pflicht-Basis |
| AVX-512CD | Conflict Detection | Skylake-X+ | Loop-Vectorization |
| AVX-512ER | Exponential+Reciprocal | NUR Xeon Phi (Knights Landing) | HPC-spezifisch |
| AVX-512PF | Prefetch | NUR Xeon Phi (Knights Landing) | HPC-spezifisch |
| AVX-512BW | Byte+Word | Skylake-X+ | Text-Processing |
| AVX-512DQ | Doubleword+Quadword | Skylake-X+ | Floating-Point + Integer |
| AVX-512VL | Vector Length (128/256-bit AVX-512) | Skylake-X+ | Backwards-Compatibility |
| AVX-512IFMA | Integer Fused Multiply-Add | Cannon Lake+ | Crypto/Bignum |
| AVX-512VBMI | Vector Byte Manipulation | Cannon Lake+ | Permutation |
| AVX-512VBMI2 | Vector Byte Manipulation 2 | Ice Lake+ | Compress/Expand |
| AVX-512VNNI | Vector Neural Network | Cascade Lake+ | **DBMS: Vector-Indexes, ML** |
| AVX-512BITALG | Bit Algorithms | Ice Lake+ | Popcount/Compress |
| AVX-512VPOPCNTDQ | Vector Population Count | Ice Lake+ | Bitmap-Indexes |
| AVX-512BF16 | Brain-Float 16-bit | Cooper Lake+ / Sapphire Rapids+ | ML/AI-Workloads |
| AVX-512FP16 | Half-Precision Float | Sapphire Rapids+ | ML/AI-Workloads |

### §4.4 Implementierungs-Vorschlag

```cpp
// axis_09b Schicht-Hierarchie als template-Properties:
class Avx2Extension {
    static constexpr bool provides_sse()     { return true; }
    static constexpr bool provides_sse2()    { return true; }
    static constexpr bool provides_sse3()    { return true; }
    static constexpr bool provides_ssse3()   { return true; }
    static constexpr bool provides_sse4_1()  { return true; }
    static constexpr bool provides_sse4_2()  { return true; }
    static constexpr bool provides_avx()     { return true; }
    static constexpr bool provides_avx2()    { return true; }
    static constexpr bool provides_avx512f() { return false; }
    // ... alle 15+ AVX-512 Sub-Flags = false
};

class Avx512Extension {
    // alle SSE/AVX/AVX2 = true
    static constexpr bool provides_avx512f() { return true; }
    static constexpr bool provides_avx512cd() { return true; }
    // Pro Wrapper-Subtype variabel: AVX-512F-only vs AVX-512-Full
};
```

**Aufwand:** 13-20 SP. **Task:** #726

## §5 R7.7.c — axis_09b CPU-Sockel-Count + P/E-Cores Topologie

**Hintergrund:** Pro CPU-Sockel hat man N Erweiterungs-Einheiten. AVX-256 default 2x pro CPU-Sockel. CPU-Kerne **teilen** sich diese Erweiterungseinheiten. Performance vs Efficiency Cores (Intel Hybrid-Architecture) — nur P-Cores haben Zugriff auf bestimmte Erweiterungen.

### §5.1 Erweiterungs-Properties pro axis_09b-Wrapper

| Property | Beispiel | Beschreibung |
|----------|----------|--------------|
| units_per_socket() | AVX2: 2, AVX-512: 1 | Anzahl SIMD-Einheiten pro CPU-Sockel |
| shared_among_cores() | true | Ob mehrere Kerne sich Einheit teilen |
| accessible_from_efficiency_cores() | false bei AVX-512 (Intel Alder Lake) | Ob E-Cores Zugriff haben |

### §5.2 Hardware-Spezifika

| Architektur | AVX-512 Verfuegbarkeit | Notes |
|-------------|------------------------|-------|
| Intel Alder Lake (Hybrid) | P-Cores: AVX-512 NEIN (BIOS-disabled) | E-Cores: kein AVX-512 |
| Intel Raptor Lake (Hybrid) | P-Cores: NEIN | E-Cores: NEIN |
| Intel Meteor Lake (Hybrid) | NEIN | hybrid mit weiteren Compute-Tiles |
| AMD Zen 4 | JA (alle Cores) | nativ 512-bit |
| AMD Zen 5 (kommend) | JA (alle Cores) | erweitert |
| ARM Neoverse V2 (NVIDIA Grace) | SVE2 scalable 128-bit | alle Cores gleich |
| ARM Cortex (mobile) | NEON 128-bit baseline | big.LITTLE moeglich |
| Apple M-Series (Hybrid) | NEON+SVE | P-Cores+E-Cores beide NEON, SVE2 in M4+ |

### §5.3 NUMA-Effects

- Cross-Socket SIMD-Latency: erste Vector-Load nach Wechsel ~20-50ns Penalty
- SMT (Hyper-Threading) impact auf Shared-Unit-Throughput: ~30% Reduktion bei vollem SMT-Pressure
- DDR5 Memory-Channels pro Sockel: Sapphire Rapids 8x, Granite Rapids 12x

### §5.4 Implementierungs-Hinweis

- Per-Wrapper static constexpr accessor functions
- Optional: Composition mit `axis_12_general_hardware` (NUMA-Topologie + CPU-Klassen)

**Aufwand:** 8-13 SP. **Task:** #725

## §6 Cross-Constraint-Filter (verwandt R5.C.3)

Aktuell produziert `CartesianIsa09xExt09bxPlatform12 = mp_product<mp_list, _09, _09b, _12>` 4 x 8 x 3 = **96 unfiltered Permutationen** — viele davon sind Hardware-mathematisch unmoeglich (z.B. Avx2Extension + Aarch64Isa).

**Loesung (R5.C.3):** PermutationEngine via `mp_remove_if` mit Compat-Predicate filtert auf Compile-Time:

```cpp
template <typename Perm>
using is_compatible = mp_bool<
    is_x86_isa<Perm>::value ? Perm::ext::compatible_with_x86() :
    is_arm_isa<Perm>::value ? Perm::ext::compatible_with_arm() :
    is_riscv_isa<Perm>::value ? Perm::ext::compatible_with_riscv() :
    is_powerpc_isa<Perm>::value ? Perm::ext::compatible_with_powerpc() : false>;

using ValidPermutations = mp_remove_if<CartesianIsa09xExt09bxPlatform12, mp_not_fn<is_compatible>>;
```

**Erwartete Reduktion:** 96 → ~25-30 gueltige Permutationen.

## §7 Cross-Refs

- Doku 13 — Paper-Legacy-Architektur (4-Schichten)
- Doku 14 — Achsen-Komposition Organ-Metapher (§26 Gattungen)
- axis_06_allocator — Goldstandard fuer alle Achsen-Implementierungen
- mimalloc-Pilot — einziger aktueller Wrapper mit echtem Original-Code-Linking

## §8 Naechste Schritte (Priorisiert)

1. **R7.6 Phase 1:** Paper-Identifikation fuer filter + index_organization + serialization
2. **R7.6 Phase 2:** legacy_code Skelette + is_original_validator Lauf
3. **R7.7 Phase 1:** axis_09b Schichten-Properties (provides_sse/avx/avx512*)
4. **R7.7 Phase 2:** AVX-512 Sub-Flags vollstaendig (15+ Flags)
5. **R7.7 Phase 3:** P/E-Cores Topologie-Properties
6. **R5.C.3:** Cross-Constraint-Filter PermutationEngine
