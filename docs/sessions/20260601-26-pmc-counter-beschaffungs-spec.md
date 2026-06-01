# #26 — Beschaffungs-/Setup-Spezifikation: Reale PMC-Hardware-Counter als IPmcSource-Drop-ins (Windows/i7-1270P + Linux/ZIH)

**Status: Beschaffungs-Spec (Umsetzung extern / durch User).** Beschreibt, WIE reale Performance-Monitoring-Counter beschafft und als `IPmcSource`-Subklassen eingehängt werden. Hardware-/Treiber-/Rechte-gebunden → nicht hier ausgeführt. Das CMake-Gating-Muster (ENABLE/HAVE/USE + `#cmakedefine01`-Flag-Template + `if constexpr`-Selektion) ist **real bewiesen** an der Allocator-Achse axis_06 (mimalloc/snmalloc/dlmalloc) — PMC ist ein 1:1-Nachbau dieses verifizierten Musters, kein Neuland.

---

## 1. Ausgangslage (repo-verifiziert)

Die Abstraktion ist **fertig** und der Drop-in-Vertrag steht: `libs/cache_engine/builder/pmc_source.hpp` definiert `IPmcSource` (`begin()` / `end()→PmcCounters` / `available()` / `name()`) und `NullPmcSource` (`available()==false`). Reale Quellen sind ein reiner Add-on: eine weitere `IPmcSource`-Subklasse, eingehängt in `measurement_from_workload_result(r, pid, pmc)` (`measurement_snapshot.hpp` Z.108–121). **Keine POD-/Pipeline-/PDF-Änderung nötig** — `pmc_available` spiegelt ehrlich `pmc.available`.

> **WICHTIG — die 6 Counter sind NICHT die generische perf-Default-Liste.** Der reale POD (`measurement_snapshot.hpp` Z.44–50, identisch in `PmcCounters`) hat:
> `cache_misses_l1`, `cache_misses_l2`, `cache_misses_l3`, `dtlb_misses`, `coherence_invalidations`, `energy_micro_joules`.
> Die generische Liste (cycles/instructions/cache-references/branch-instructions/branch-misses) trifft **nicht** zu: `total_cycles` ist eine eigene Spalte (kein PMC-Feld), und es gibt **keine** Felder für instructions/branches. Jede Implementierung MUSS exakt diese 6 Felder befüllen, sonst ist `available=true` eine Erfolgsmarke ohne Deckung ([[feedback_no_success_marks_without_literal_output]]).

---

## 2. CMake-Gating analog axis_06 (ENABLE/HAVE/USE)

Spiegelt das verifizierte Allocator-Muster (`CMakeLists.txt` Z.85–116 ENABLE; Z.484–506 USE=ENABLE&&HAVE; `configure_file` **nach** `add_subdirectory(ext)`; Template-Mechanik wie `axis_06_allocator_flags.hpp.in`, `#cmakedefine01` + `inline constexpr bool …_enabled`):

- **Optionen:** `COMDARE_PMC_ENABLE_INTEL_PCM`, `COMDARE_PMC_ENABLE_LINUX_PERF`, `COMDARE_PMC_ENABLE_PAPI`, `COMDARE_PMC_ENABLE_LIKWID`. Default **OFF** (anders als axis_06=ON) — PMC ist HW-/Rechte-gated, kein Default-Bestandteil.
- **Detection (`COMDARE_HAVE_<X>`):** Win+MSVC+`x86_64` → `HAVE_INTEL_PCM` möglich; `CMAKE_SYSTEM_NAME STREQUAL "Linux"` → `HAVE_LINUX_PERF` (Header `linux/perf_event.h` via `check_include_file`); `find_package(PAPI)`/`find_library(likwid)` für die HPC-Pfade.
- **USE = ENABLE && HAVE** → neues Template `libs/cache_engine/builder/pmc_flags.hpp.in` (gleiche `#cmakedefine01`-Mechanik) → `inline constexpr bool intel_pcm_enabled` usw. Quellen kompilieren immer; die `IPmcSource`-Subklasse selektiert per `if constexpr (flags::…_enabled)` (kein `#ifdef`-Wildwuchs), exakt wie die vendor_includes-Shims.
- Eingecheckter Vendor-Tree analog `ext/allocator/A0x-…`: `ext/pmc/intel-pcm/` (PCM-Quellen + LICENSE/MODIFICATIONS.md/sha256_locked.txt wie bei den Allocatoren). Linux-perf braucht **kein** ext/ (Kernel-UAPI-Header `<linux/perf_event.h>`); PAPI/likwid sind System-Libs (nur auf ZIH).

> **Trigger-Caveat (wie bei axis_06):** `configure_file` für die Flags muss **nach** `add_subdirectory(ext)` laufen, sonst greift die HAVE-Detection nicht. Solange alle USE=0 → `NullPmcSource` bleibt aktiv, Build unverändert grün.

---

## 3. Pfad A — Windows / i7-1270P lokal

### A1. Intel PCM → `IntelPcmPmcSource`
- **Quelle/Build:** github `intel/pcm`, CMake + MSVC v143 (VS 2022): `cmake -B build` → `PCM.sln` → `build/bin/Release`. Als Library gegen die PCM-Klasse linken (`PCM::getInstance()` + **zwingend** `program(...)`, nicht nur `getInstance()`).
- **Treiber/Rechte (harter Blocker, ehrlich):** PCM braucht auf Windows einen MSR-Kernel-Treiber. Entweder Intels `msr.sys` nach `C:\Windows\System32` (Admin) **oder** der Drittanbieter-`WinRing0.sys`/`WinRing0x64.sys` (aus OpenHardwareMonitor/RealTemp). Folgen: (a) **Admin-Rechte** bei Treiber-Setup und Mess-Lauf; (b) WinRing0 ist bekannt **unsigniert/als verwundbar geflaggt** → Secure-Boot/HVCI/Defender kann das Laden blocken (`Failed to load winring0.dll/winring0.sys`, intel/pcm #183); (c) auf Hybrid-CPUs ist die Per-Core-Zuordnung P/E-Core-abhängig.
- **Counter-Mapping → die 6 realen POD-Felder:**

  | POD-Feld | PCM-Quelle | Status / Ehrlichkeit |
  |---|---|---|
  | `cache_misses_l2` | `getL2CacheMisses(before,after)` | Built-in, direkt |
  | `cache_misses_l3` | `getL3CacheMisses(before,after)` | Built-in, direkt |
  | `cache_misses_l1` | **kein** Built-in | nur via `CUSTOM_CORE_EVENTS`/`ExtendedCustomCoreEventDescription`, Alder-Lake-Raw-Event (z.B. `MEM_LOAD_RETIRED.L1_MISS`) — modellspezifisch |
  | `dtlb_misses` | **kein** Built-in | nur via Custom-Event (`DTLB_LOAD_MISSES.*`), Alder-Lake-spezifisch |
  | `coherence_invalidations` | **kein** sauberes Built-in | Custom-Event (Snoop/`OFFCORE`/Invalidate); auf 1-Socket-Laptop semantisch ~0 → ehrlich 0 lassen statt erfinden |
  | `energy_micro_joules` | `getConsumedJoules(before,after)` ×1e6 | RAPL via PCM; Werte ggf. socket-/package-weit, nicht reine Workload-Energie |

  Sicher direkt: **L2/L3 + Energy**. L1/dTLB nur über Custom-Event-Programmierung (CPU-Manual Vol.3B, modellabhängig). `coherence_invalidations` auf 1-Socket-Laptop realistisch nicht aussagekräftig → ehrlich 0 + Kommentar.

### A2. Alternative Windows-PDH/ETW → NICHT geeignet
PDH/ETW liefern OS-Zähler, aber **keine** echten Core-PMU-Events (L1/L2/L3-Miss/dTLB/Coherence). ETW kann CPU-Sampling, nicht die geforderten Delta-Hardware-Counter. → höchstens `total_cycles`-Quervalidierung. **PDH/ETW NICHT als PMC-Quelle markieren** (sonst falsches `available=true`).

**Fazit A:** `IntelPcmPmcSource` ist machbar, aber **Admin + Treiber-Beschaffung (extern)** und nur 3 der 6 Felder ohne Custom-Event-Arbeit. Bis Treiber steht: `available()` bleibt false (kein Schein-0).

---

## 4. Pfad B — Linux / ZIH-Cluster (Barnard CPU)

### B1. `LinuxPerfPmcSource` (perf_event_open(2), keine Lib)
- Kein glibc-Wrapper → direkt `syscall(SYS_perf_event_open, &attr, …)`; `begin()`=`ioctl(RESET+ENABLE)`, `end()`=`ioctl(DISABLE)`+`read()` der Gruppe. Header `<linux/perf_event.h>` (HAVE via `check_include_file`).
- **Mapping** (`config = cache_id | (op<<8) | (result<<16)`):

  | POD-Feld | perf-Event |
  |---|---|
  | `cache_misses_l1` | `PERF_TYPE_HW_CACHE`, `L1D` \| `OP_READ` \| `RESULT_MISS` |
  | `cache_misses_l2` | i.d.R. **kein** generischer Code → `PERF_TYPE_RAW` mit Modell-Event |
  | `cache_misses_l3` | `PERF_TYPE_HW_CACHE`, `LL` \| `OP_READ` \| `RESULT_MISS` (LLC≈L3) |
  | `dtlb_misses` | `PERF_TYPE_HW_CACHE`, `DTLB` \| `OP_READ` \| `RESULT_MISS` |
  | `coherence_invalidations` | nur `PERF_TYPE_RAW` (Snoop/HITM), modellspezifisch → wenn unbekannt: 0 + ehrlich |
  | `energy_micro_joules` | **nicht** perf-CACHE; RAPL via `PERF_TYPE_POWER` (`/sys/bus/event_source/devices/power/`) ODER getrennt (B4) |

  Ehrlich: L1D/LL/dTLB-Miss generisch ok; L2-Miss + Coherence brauchen Raw-Events (Barnard-CPU-Modell prüfen). Nicht gemappte Felder bleiben 0; `available()` nur true, wenn die belegten Felder real gelesen wurden.

### B2. Rechte (entscheidend auf HPC)
- `/proc/sys/kernel/perf_event_paranoid`: ab Linux 4.6 Default **2** (nur User-Space, **kein** HW-Cache-Counter ohne Weiteres). Für die HW-Events braucht es **≤1** (bzw. 0 für per-CPU) **oder** `CAP_SYS_ADMIN` — als normaler ZIH-User **nicht** setzbar. ⚠ Versionsabhängig.
- ZIH-üblicher Weg: **SLURM-Prolog setzt paranoid=0 nur bei exklusivem Node**, Epilog zurück auf 2; praktisch an die likwid-Freigabe gekoppelt (`#SBATCH --constraint=hwperf` / `-C hwperf`). → **Vor erstem Versuch HPC-Support fragen, ob `-C hwperf`/paranoid-Prolog auf Barnard existiert** (nicht annehmen).

### B3. `PapiPmcSource` (PAPI) — robusteste HPC-Variante (Primärziel)
- PAPI über perf_event-Backend; `PAPI_L1_DCM`/`PAPI_L2_TCM`/`PAPI_L3_TCM`/`PAPI_TLB_DM` mappen direkt auf 4 der 6 Felder. Energy via PAPI-`rapl`-Komponente (`rapl:::PACKAGE_ENERGY:PACKAGE0`) → `energy_micro_joules`. `coherence_invalidations` nur über native Events. Auf ZIH typischerweise `module load papi` → `find_package`/`find_library` (HAVE); gleiche paranoid-Einschränkung.

### B4. likwid / `perf stat` (Fallback ohne eigene Lib)
- `likwid-perfctr` (Marker-API `LIKWID_MARKER_START/STOP` um den Mess-Workload) liest L2/L3/TLB + RAPL-Energy (`likwid-powermeter`); braucht **`-C hwperf`** im SLURM-Job. Out-of-Process-Variante: `perf stat -e …` um den Mess-Treiber, CSV parsen (= `IPmcSource`-Adapter, der das Kind-Resultat liest). Energy bei likwid/RAPL ist **package-weit** → nicht reine Workload-Energie (ehrlich dokumentieren).

**Fazit B:** Auf Barnard `PapiPmcSource` als Primärziel (modulverfügbar, sauberes Mapping für 4–5 der 6 Felder), `LinuxPerfPmcSource` als lib-freier Direktweg, likwid/`perf stat` als Fallback. **Alle** hängen an `paranoid`/`-C hwperf` → ohne exklusiven Node + Freigabe `available()=false`.

---

## 5. Was faktisch NICHT geht (kein Faken)

- **Windows ohne Admin/Treiber:** keine echten Core-PMU-Counter. PDH/ETW ersetzen sie NICHT. WinRing0 kann durch Secure-Boot/HVCI/Defender geblockt sein.
- **`coherence_invalidations` auf 1-Socket-i7-1270P:** vermutlich nicht sinnvoll messbar → Kandidat für dauerhaft-ehrliche 0 (mit Kommentar), nicht erfinden.
- **`cache_misses_l1` / `dtlb_misses` über PCM:** kein Built-in → nur Custom-Event-Programmierung (Alder-Lake-modellspezifisch, ungetestet).
- **ZIH ohne `-C hwperf`/exklusiven Node:** `perf_event_paranoid=2` blockt HW-Cache-Counter → `available()=false`.
- **Energy (RAPL) überall:** package-/socket-weit, nicht reine Workload-Energie → so dokumentieren, nicht als exakte Workload-Energie ausgeben.

In allen Sperrfällen gilt: `available()` bleibt **false** statt Schein-0. Eine Erfolgsmarke nur bei literaler Counter-Ausgabe.

---

## 6. Zuordnung: Quelle → IPmcSource-Drop-in → Plattform

| Drop-in-Klasse | Plattform | Lib/Toolchain | HAVE-Detection | Felder real abgedeckt |
|----------------|-----------|---------------|----------------|------------------------|
| `IntelPcmPmcSource` | Windows/i7 | intel/pcm + MSR-Treiber (msr.sys/WinRing0), Admin | Win+MSVC+x86_64, `ext/pmc/intel-pcm/` | L2,L3,Energy direkt; L1/dTLB via Custom-Event; Coherence ehrlich 0 |
| `LinuxPerfPmcSource` | Linux/ZIH | keine (Syscall + `<linux/perf_event.h>`) | `check_include_file` | L1D,LL(=L3),dTLB; L2/Coherence raw; Energy via PERF_TYPE_POWER |
| `PapiPmcSource` | Linux/ZIH (Primär) | PAPI (`module load papi`) | `find_package(PAPI)` | L1_DCM,L2_TCM,L3_TCM,TLB_DM + rapl-Energy; Coherence nativ |
| likwid / `perf stat`-Adapter | Linux/ZIH (Fallback) | likwid (`find_library`) bzw. `perf` CLI | `find_library(likwid)` | L2,L3,TLB + RAPL package-weit |

---

## 7. Konkrete nächste Schritte (umsetzbar, gate-bewusst)

1. `pmc_flags.hpp.in` + 4 ENABLE-Optionen (Default OFF) + HAVE-Detection + USE-Berechnung **nach** `add_subdirectory(ext)` anlegen (1:1 axis_06-Muster). Solange alle USE=0 → `NullPmcSource` aktiv, Build grün.
2. `LinuxPerfPmcSource` zuerst (lib-frei, deckt L1D/LL/dTLB + RAPL); unter `if constexpr (flags::linux_perf_enabled)` in den Mess-Treiber einhängen; bei `read()`-Fehler/paranoid-Block → `available=false`.
3. `IntelPcmPmcSource` als zweite Subklasse (L2/L3/Energy direkt; L1/dTLB/Coherence via Custom-Events ODER ehrlich 0). Treiber/Admin = extern, bis dahin nicht aktivieren.
4. `PapiPmcSource` als ZIH-Primärquelle vorbereiten (HAVE via `find_package(PAPI)`); erst auf Barnard scharf schalten, **nach** Klärung `-C hwperf`/paranoid mit HPC-Support.
5. Pro Quelle ein Test: wenn `available()`, dann sind die belegten Felder >0 in einem Last-Lauf; sonst skip. Keine Erfolgsmarke ohne literale Counter-Ausgabe.

---

## 8. Offene Unsicherheiten (ehrlich)
- Barnard-CPU-Modell (→ Raw-Event-Codes für L2-Miss/Coherence) noch unbekannt.
- ZIH-`paranoid`/`-C hwperf`-Policy unbestätigt (HPC-Support fragen).
- Intel-PCM-Custom-Events für i7-1270P (Alder-Lake-Hybrid, P/E-Core) modellspezifisch und ungetestet.
- `coherence_invalidations` auf 1-Socket-Hardware vermutlich nicht sinnvoll messbar → dauerhaft-ehrliche 0.

---

## 9. Relevante Datei-Pfade (absolut)
- `C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine\libs\cache_engine\builder\pmc_source.hpp` (IPmcSource / NullPmcSource — Drop-in-Vertrag)
- `...\comdare-cache-engine\libs\cache_engine\builder\measurement_snapshot.hpp` (Z.44–50 PmcCounters-POD; Z.108–121 `measurement_from_workload_result` — Einhängepunkt)
- `...\comdare-cache-engine\libs\cache_engine\builder\pmc_flags.hpp.in` (NEU anzulegen, Vorbild `libs/cache_engine/axes/alloc/axis_06_allocator_flags.hpp.in`)
- `...\comdare-cache-engine\CMakeLists.txt` (Z.85–116 ENABLE-Muster; Z.484–506 USE; configure_file nach ext)
- `...\comdare-cache-engine\ext\pmc\intel-pcm\` (NEU anzulegender Vendor-Tree, Vorbild `ext/allocator/A0x-…`)

**Quellen:** intel/pcm (Build, getInstance/program, RAPL getConsumedJoules) · intel/pcm issue #183 (WinRing0-Load-Fehler) · Linux `perf_event_open(2)` man-page (PERF_TYPE_HW_CACHE, paranoid) · PAPI (PAPI_L1_DCM/L2_TCM/L3_TCM/TLB_DM, rapl-Komponente) · likwid (Marker-API, `-C hwperf`, likwid-powermeter) · Intel SDM Vol.3B (Custom-Core-Events Alder Lake).
