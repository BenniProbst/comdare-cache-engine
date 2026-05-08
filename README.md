# comdare-cache-engine

**Active Cache-Aware Hardware Adaptation Cache Engine for Trie-Based Index Structures**

Diplomarbeit · TU Dresden · Datenbankentwicklung mit Spezialisierung in Suchalgorithmen
Pruefungsordnung 2010 · Hauptbetreuer: Prof. Dr. Dirk Habich
Lizenz: Apache 2.0 · BEP Venture UG / Marke Comdare

---

## Status: Phase 4.B Skelett (KEINE Implementation)

Dieses Repository enthaelt die **strukturelle Vorbereitung** der Diplomarbeit-
Implementation. Die eigentliche Implementation wird in Phase 5 (drawio UML)
und folgenden Phasen durchgefuehrt — nach explizitem GO durch den Architekten.

## Projekt-Domaenen (gem. Domaenenmodell v3 + v4)

| Domaene | Zweck | Verzeichnis |
|---------|-------|-------------|
| 1 — Search Engine | Trie/B-Tree-Container, Page/Node/Traversal/ValueHandle als Concepts, std::map-API | `search_engine/` |
| 2 — Cache Engine | CacheEngineBuilder + CacheEngine-Singleton (Heap, im Builder), Observer/Visitor, ConcurrencyManager | `cache_engine/` |
| 3 — Measurement | In-Memory MeasurementBuffer, perf/PAPI/Advisor/HdrHistogram-Wrapper, Mikrobenchmarks | `measurement/` |
| 4 — Hardware/ISA | ISA-Dispatch, Hybrid-Core-Pinning, Memory-Type-Detector, Hugepages | `hardware_isa/` |
| 5 — Engine Choice | StaticEngine vs CacheEngine als Compile-Time Template-Parameter | `engine_choice/` |
| 6 — Publication | LaTeX-Renderer (in Builder integriert) → Anhang Diplomarbeit | `cache_engine/builder/latex_renderer/` |

## Habich-Direktive (F-EXTRA-1)

- Originalcode aller fremden Algorithmen (`ext/<paper>/`) wird **EXAKT KOPIERT**
  uebernommen.
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

## Build-Anleitung (Skelett — noch nicht funktionsfaehig)

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
**6 Email-Anfragen noetig:** P06, P28, P31, P32, P33 (siehe `docs/email/20260508-1800-email_kontakte.md`)
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
