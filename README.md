# comdare-cache-engine

**Active Cache-Aware Hardware Adaptation Cache Engine for Trie-Based Index Structures**

Diplomarbeit · TU Dresden · Datenbankentwicklung mit Spezialisierung in Suchalgorithmen
Pruefungsordnung 2010 · Hauptbetreuer: Prof. Dr. Dirk Habich, Dr. Alexander Krause
Lizenz: Apache 2.0 · BEP Venture UG / Marke Comdare

---

## Derzeitiger Status: Kern + Mess-Pfad lauffähig; Voll-Permutations-Build teils skelettiert

Dieses Repository enthaelt die Cache-Engine-Implementation: der Kern (Such-/Anatomie-/
Komposition + Cross-ABI) und der lokale Mess-Pfad (`perm_runner` + Tier-DLLs, s. „Messwerte
erzeugen") sind lauffähig; der Voll-Permutations-Build (alle Vendor-Allokatoren + Massen-
Permutationen) und die ZIH-Skalierung folgen phasenweise.

## Projekt-Domaenen (gem. Domaenenmodell v3 + v4)

| Domaene | Zweck | Verzeichnis |
|---------|-------|-------------|
| 1 — Search Engine | Trie/B-Tree-Container, Page/Node/Traversal/ValueHandle als Concepts, std::map-API | `search_engine/` |
| 2 — Cache Engine | CacheEngineBuilder + CacheEngine-Singleton (Heap, im Builder), Observer/Visitor, ConcurrencyManager | `cache_engine/` |
| 3 — Measurement | In-Memory MeasurementBuffer, perf/PAPI/Advisor/HdrHistogram-Wrapper, Mikrobenchmarks | `measurement/` |
| 4 — Hardware/ISA | ISA-Dispatch, Hybrid-Core-Pinning, Memory-Type-Detector, Hugepages | `hardware_isa/` |
| 5 — Engine Choice | StaticEngine vs CacheEngine als Compile-Time Template-Parameter | `engine_choice/` |
| 6 — Publication | LaTeX-Renderer (in Builder integriert) → Anhang Diplomarbeit | `cache_engine/builder/latex_renderer/` |

## Bearbeitungsdirektiven der Diplomarbeit (F-EXTRA-1)

- Originalcode aller fremden Algorithmen (`ext/<paper>/`) wird **EXAKT KOPIERT**
  uebernommen (wissenschaftliche Anforderung für die Optimierung der Vergleichbarkeit)
- Anbindung erfolgt ueber Adapter-Pattern (`adapters/<paper>/`).
- Hauptcompiler ist IMMER C++23 (Schale, Adapter, Modul-Wrapper).
- Originalcode-Bausteine werden mit IHREM Original-Compiler kompiliert,
  als Object-Files statisch in das C++23-Modul gelinkt.
- PRT-ART (`prt_art/`) ist die einzige Stelle, in die wir frei eingreifen.

## Wichtige Dokumente (in `docs/`)

| Dokument | Quelle |
|----------|--------|
| Architekturentscheidungen F1-F15 + F-EXTRA-1-8 | `docs/architekturentscheidungen/` |
| Domaenenmodell v3 + v4 | `docs/domaenenmodell/` |
| Begriffsglossar v3-v6 | `docs/glossare/` |
| Bausteine-Matrix | `docs/bausteine/` |
| Flag-System (CPUID-Vorbild) | `docs/architecture/flag_system.md` |

## Build-Anleitung (Kern + Mess-Pfad lauffähig; Voll-Permutations-Build skelettiert)

> Der Kern-Build (`msvc-release`) + die lokale Mess-Kette (`perm_runner` + Tier-DLLs, s. „Messwerte erzeugen") sind
> lauffähig. Der `-DCOMDARE_BUILD_PERMUTATIONS=ON`-Voll-Build (alle Vendor-Allokatoren + Massen-Permutationen) bleibt
> teils skelettiert (s. `docs/sessions/20260601-19-vendor-allokatoren-beschaffungs-spec.md`).

```bash
# Standard Build (Compile-Time SIMD-Detection)
cmake -B build -S . -DCOMDARE_DETECTION_MODE=COMPILE_TIME
cmake --build build -j

# ZIH-Modus (Runtime SIMD-Detection)
cmake -B build -S . -DCOMDARE_DETECTION_MODE=RUNTIME
cmake --build build -j

# Pre-Build aller Modul-Permutationen (Stunden bis Tage!)
cmake -B build -S . -DCOMDARE_BUILD_PERMUTATIONS=ON
cmake --build build -j

# Builder ausfuehren (nach erfolgreichem Permutations-Build)
./build/cache_engine/builder/cache_engine_builder run \
  --architecture=x86_avx512 --dataset=YCSB-A
```

### Reproduzierbare Builds über CMake-Presets (`CMakePresets.json`)

Statt der manuellen `-D`-Flags oben kapselt `CMakePresets.json` (Schema v6) die geprüften
Konfigurationen. Verwendung: `cmake --preset <name>` → `cmake --build --preset <name>` → `ctest --preset <name>`.

| Preset | Generator / Compiler | Zweck | Plattform-Bedingung |
|--------|----------------------|-------|---------------------|
| `msvc-release` | Visual Studio 17 2022, x64, Release | **Daily-Dev** unter Windows (Standard; `COMDARE_EXPERIMENT_MODE=ON`) | `hostSystemName == Windows` |
| `msvc-debug`   | VS 17 2022, Debug (erbt `msvc-release`) | **Debugging** unter Windows (Asserts/Statistics aktiv) | `hostSystemName == Windows` |
| `gcc-release`  | Ninja, `g++`, Release | **CI-Linux / Cluster-Build** (ZIH Barnard/Capella GCC-Toolchain) | `hostSystemName == Linux` |
| `clang-release`| Ninja, `clang++`, Release | **Cluster-/Profiling-Alternative** (Clang-Toolchain) | `hostSystemName == Linux` |

Gemeinsame Basis (`_base`, hidden): `CMAKE_CXX_STANDARD=23` (Required, keine GNU-Extensions) +
`CMAKE_EXPORT_COMPILE_COMMANDS=ON` (für clangd/IDE). Es gibt je 4 `configurePresets`/`buildPresets`
und 2 `testPresets` (`msvc-release`, `gcc-release` mit `outputOnFailure`).

> Hinweis: Die `condition`-Klauseln blenden nicht-passende Presets je Host automatisch aus
> (`cmake --list-presets` zeigt nur die auf dem aktuellen System gültigen).

## Messwerte erzeugen (Kommandozeile) — Schnellstart

Diese Kette erzeugt **echte Messwerte aus echten SearchAlgorithm-DLL-„Tieren"** lokal (Windows/MSVC, gate-frei).
Ein „Tier" = eine kompilierte Permutations-DLL (19 SearchAlgorithm-Slots T0..T18 = 17 Kern-Achsen + queuing q1/q2, plus 3 Build-Achsen); `perm_runner` lädt sie,
treibt den Mess-Workload (`n` Inserts + `n` Lookups) und gibt eine `result_ingest`-Zeile aus.

**0) Einmalig konfigurieren** (erzeugt `build/msvc-release/generated/` + baut `perm_runner.exe`):
```powershell
cmake --preset msvc-release
cmake --build build/msvc-release --target perm_runner --config Release
```

**1) Mess-Lauf über mehrere Tiere** (baut 8 Tiere + misst, schreibt CSV):
```powershell
pwsh tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1
# Ergebnis: build/thesis_tiere/thesis_measurements.csv
#   Spalten: organ;n_ops;search_lookup;hit;miss;insert;erase;peak;bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail;obs_axes;fill
```

**2) Ein einzelnes Tier messen** (direkt mit perm_runner):
```powershell
perm_runner <tier.dll> <binary_id> <n_ops>
# z.B.: build/msvc-release/apps/perm_runner/Release/perm_runner.exe `
#       build/thesis_tiere/thesis_sa_btree.dll thesis_sa_btree 2000
# Ausgabe (1 Zeile): thesis_sa_btree;2000;2000;0;2000;0;2000;115200;38368;20;19;0;4;2000
```
Format = `binary_id` + 13 Observer-Felder (`builder/experiment_tree/result_ingest.hpp`):
`search_lookup;hit;miss;insert;erase;peak_occupancy | bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail | observable_axes;fill`.

**3) Eigenes Tier definieren:** eine `.cpp` mit dem Makro `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, <17 Achsen>)`
anlegen (Vorlage: `tests/unit/thesis_tiere/thesis_sa_array256.cpp`), dann via `cl /LD /DCOMDARE_MEASUREMENT_ON=1`
+ vollem Include-Satz bauen (das Skript aus Schritt 1 zeigt den exakten Aufruf). **Wichtig:** ohne
`/DCOMDARE_MEASUREMENT_ON=1` baut die DLL ohne `IObservableTier` → `perm_runner` meldet „keine Mess-Ebene" (exit 1).

> ⚠️ **Architektur-Hinweis (ehrlich, Stand 2026-06-03):** Die aktuellen `axis_03a`-Such-Organe (Array256, BST, …) sind
> noch **Monolithen mit Eigenspeicher** und delegieren NICHT an die Speicher-Achsen (node_type/layout/allocator) →
> Variation von axis_04/05 ist derzeit wirkungslos. Details + Fix-Plan: `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md`.
> Der **korrekte, delegierende** Aufbau ist als vertikaler Beleg vorhanden und ausführbar:
> ```powershell
> pwsh tests/unit/thesis_tiere/build_node_delegation_proof.ps1
> # zeigt: gleiche Semantik, aber chunk_count = ceil(n/node_capacity) je Node verschieden -> node_type wirkt
> ```

**Cluster-Skalierung (GATE-MAXIMAL, ZIH):** dieselbe Kette läuft via Apptainer + SLURM-Array über `perm_runner` →
Webhook → `result_ingest`; siehe `deploy/comdare-ce.def` + den Wunsch-Katalog in der Infra-K78 (CE-D1…D5).

## Compiler-Anforderungen

| Komponente | Compiler |
|------------|----------|
| Hauptcompiler (Schale + Adapter + PRT-ART) | GCC 14+ / Clang 17+ / MSVC 19.39+ (C++23) |
| Original-Bausteine | siehe `ext/<paper>/STATUS.md` (pro Paper individuell) |
| Cross-Compile-Toolchain | `tools/compiler_provisioning/` (F-EXTRA-4) |

## Plattform-Workflow (F13)

| Plattform | Modus | Lieferung |
|-----------|-------|-----------|
| Pi 5 / VisionFive 2 | Compile-Time, Self-Built Compiler | Direkt nativ bauen |
| ODROID H4 Ultra / x86-Server | Compile-Time, CI/CD | GitLab CI |
| ZIH (Barnard / Capella / Grace Hopper) | Runtime SIMD-Detection | Cross-compiled, SOCKS5-Lieferung |

## Submodule-Konvention (Ausnahme zur globalen Regel)

Entgegen der globalen "KEINE Git Submodules"-Regel werden in diesem Projekt
ausnahmsweise die COMDARE-Module unter `modules/` als feste Submodules
eingebunden. Sie sind sonst privat und werden nur statisch im
comdare-cache-engine-Projekt veroeffentlicht.

Geplant unter `modules/` (zu integrieren nach Cluster-Migration):
- `comdare-search-engine/`
- `comdare-cache-engine-core/`
- `comdare-measurement/`
- `comdare-isa-dispatch/`
- `comdare-build-tools/`
- `comdare-test-system/`

## Status der Originalcode-Integration

Siehe `ext/STATUS_UEBERSICHT.md` und parallel:
- `Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md` (geklonte Repos: 11 von 33)
- `Forschungsarbeiten/code/<paper>/STATUS.md` (pro Paper)

**11 Repos geklont:** P01, P02, P03, P04, P05, P07, P10, P20, P25, P29, P30
**14 Re-Implementations noetig:** P11-P14, P16-P19, P21-P24, P26, P27
**6 Email-Anfragen noetig:** P06, P08, P28, P31, P32, P33 (siehe `docs/email/20260508-1800-email_kontakte.md`)
**2 Originalpaper-Konzept:** P09 (LOUDS, SDSL als Referenz), P15 (Survey)

## Naechste Phasen

- ⏳ **Phase 4.B Detail:** Cluster-Migration abschliessen, COMDARE-Modules
  via Submodules einbinden, Email-Anfragen senden
- 🔒 **Phase 5:** drawio UML — gesperrt bis explizites GO
- ⏳ **Phase 6:** Implementation (nach UML-GO)
- ⏳ **Phase 7:** Permutations-Builds + Experimente
- ⏳ **Phase 8:** LaTeX-Anhang fuer Diplomarbeit

## Lizenz

Apache License 2.0 — siehe [LICENSE](LICENSE).
Externe Originalcode-Bausteine unter `ext/<paper>/` behalten ihre Originallizenz.
