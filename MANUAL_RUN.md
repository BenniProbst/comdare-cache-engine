# MANUAL_RUN — CacheEngineBuilder manuell bauen & ausführen

Diese Anleitung beschreibt, wie die Lösung **von Hand** gebaut und gestartet wird
(Selbst-Test ohne CI). Ergänzt `README.md` (Build/IDE-Einstieg) um den **Laufzeit**-Teil.

> **Architektur-Einordnung:** Die `cache-engine` bestimmt **WIE** gemessen wird
> (Builder, Codegen, ModuleLoader, Workload-Generator). Die eigentlichen 3 Messreihen
> (A/B/C) orchestriert `Diplomarbeit/Code/messung_driver/` bzw. der `thesis_tiere`-Harness.
> Der hier beschriebene `comdare-cache-engine-builder` ist der **Demo-Treiber** für den
> Schnell-Selbst-Test der Permutations-/Codegen-Pipeline.

---

## 0. Voraussetzungen

- C++23-Compiler (MSVC 19.4x / GCC 13+ / Clang 17+ — Details: `README.md` §Compiler-Anforderungen)
- CMake ≥ 3.24, Ninja **oder** Visual Studio 17 2022
- Boost (Offline-Prerequisite-Mechanismus s. `prerequisites/README.md`)

## 1. Konfigurieren & bauen

Der Demo-Treiber ist hinter der CMake-Option `COMDARE_BUILD_BUILDER` (Default AN) registriert.

```bash
# aus dem Repo-Wurzelverzeichnis (comdare-cache-engine/)
cmake -B build -DCOMDARE_BUILD_BUILDER=ON
cmake --build build --target cache_engine_builder
```

- **Visual Studio (Multi-Config):** zusätzlich `--config Debug` (oder `Release`) anhängen.
  Die Exe landet unter `build/apps/cache_engine_builder/<Config>/comdare-cache-engine-builder.exe`.
- **Ninja (Single-Config):** Exe unter `build/apps/cache_engine_builder/comdare-cache-engine-builder`.
- Alternativ die geprüften Presets aus `CMakePresets.json` verwenden (s. `README.md`).

## 2. Hilfe anzeigen (`--help` / `-h`)

```bash
comdare-cache-engine-builder --help
```

Gibt die vollständige Aufruf-Syntax nach `stdout` aus und beendet mit Status `0`.

## 3. Demo-Preset auflisten (schnell, ohne Codegen/Build)

Das mitgelieferte Preset `libs/cache_engine/builder/example_configs/` enthält die 4
Pflicht-XMLs (`cache_engine_permutations.xml`, `search_algorithm_permutations.xml`,
`allocator_permutations.xml`, `test_data_sets.xml`). Mit `--enumerate-only` wird nur die
Permutations-Enumeration ausgeführt (kein Codegen, kein cmake-Aufruf):

```bash
# aus dem Repo-Wurzelverzeichnis ausführen — --comdare-root fällt dann korrekt
# auf das aktuelle Verzeichnis zurück (Repo-Wurzel):
comdare-cache-engine-builder libs/cache_engine/builder/example_configs /tmp/ce_out --enumerate-only
```

Erwartete Ausgabe: eine Liste `ce_<engine>:<search>:<allocator>:<dataset> (fp=0x…)`-Deskriptoren,
abgeschlossen mit `==== CacheEngineBuilder OK ====` und Status `0`.

> Wird der Builder aus einem anderen Verzeichnis gestartet, den Repo-Root explizit setzen:
> `--comdare-root=/pfad/zu/comdare-cache-engine`. Die SOTA-Profil-Auflösung
> (`algorithm_profiles/sota`) findet das Verzeichnis seit #193-A CWD-unabhängig
> (mehrere Layout-Varianten + Aufwärts-Suche).

## 4. Optionen

| Option | Wirkung |
|--------|---------|
| `-h`, `--help` | Aufruf-Syntax nach stdout, Status 0 |
| `--enumerate-only` | Nur Deskriptoren enumerieren (kein Codegen/Build) |
| `--skip-build` | Quellen + Aggregator generieren, aber cmake nicht aufrufen |
| `--comdare-root=DIR` | Repo-Wurzel (Default: aktuelles Verzeichnis) |
| `--quiet` | Phasen-Diagnose unterdrücken |

## 5. Vollständige Messung (Codegen + Build + Lauf)

Für den **echten** Multi-Achsen-Mess-Lauf (Codegen der Permutations-Module, Kompilation,
Ausführung, CSV) ist der getestete Pfad der `thesis_tiere`-Harness bzw. `messung_driver`:

- `tests/unit/thesis_tiere/README.md` — Harness-Aufruf (Pilot/150er/320er-Lauf, Resume)
- Der Demo-Treiber-Voll-Lauf (ohne `--enumerate-only`/`--skip-build`) generiert Module über
  `codegen`; die standalone-Codegen-Pfad-Auflösung wird zusammen mit dem 320-DLL-Neubau
  (#215) final abgeglichen — für den Schnell-Selbst-Test genügen `--help` + `--enumerate-only`.

## 6. Plattform-Hinweise

- **Windows:** aus Git-Bash die `C:/…`-Schreibweise für Pfad-Argumente verwenden.
- **Linux:** Standard-POSIX-Pfade; Exe ohne `.exe`-Endung.
- Cache-Wurzel: Windows `C:\temp\comdare`, Linux `/tmp/comdare/`.
