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

- C++23-Compiler: **GCC ≥ 15.3** (Toolchain-Floor V-6iii/Ledger §73.4, hart per CMake-Assert; kein
  Versions-Pin) / MSVC 19.4x / Clang 17+ — Details: `README.md` §Compiler-Anforderungen
- CMake **≥ 3.28** (reales Minimum, `CMakeLists.txt`/`CMakePresets.json`), Ninja **oder** Visual Studio 17 2022
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

## 6. Stand abfragen: `comdare-experiment-planner status` (W5)

Der **Experiment-Planer** ist eine eigene Binary (`apps/experiment_planner`, Target
`comdare_experiment_planner`). Sein Subkommando `status` ist ein **reiner Rueck-Leser**: es baut nichts,
misst nichts und reserviert nichts -- es berichtet den Stand von CEB-Bauten, Tier-Binaries und Messwerten
und beendet mit `0`.

```bash
cmake --build build --target comdare_experiment_planner
build/apps/experiment_planner/comdare-experiment-planner status --root=Code/measure_out
```

- `--root=<dir>` -- Wurzel des Mess-Ausgabe-Baums. Default: `Code/measure_out`, ersatzweise `measure_out`.
  Der **aufgeloeste** Wert steht immer in der Kopfzeile (`root=...`, `root_vorhanden=ja|nein`).
- Fenster: `COMDARE_GOLDEN_N_RANGE="start:count"`. Der `start` **filtert** die Erhebung: nur Perms in
  `[start, start+count)` gehen in die Bilanz. Perm-Verzeichnisse ausserhalb gehoeren einem **anderen**
  Fenster und erscheinen als eigene `[status-fremdfenster]`-Zeile (`im_fenster=nein` in der Zell-Zeile),
  nie als Fortschritt des aktuellen Fensters. `offen=` der Gesamt-Zeile ist die **Summe** der
  Zell-Offenstaende (je Zelle `count - gemessen`), nicht ein `count` minus aller Messungen.
  **Ohne** gepinntes Fenster (`count=0` oder Env leer) gibt es kein Binary-SOLL -- `offen=` traegt dann
  den Sentinel `unbelegt` statt einer erfundenen Zahl.
- Fortschritts-Cursor: `done=` ist eine Aussage ueber die **zuletzt** gelesene Zeile. Schreibt ein
  Folge-Fenster hinter das `done` weiter, faellt `done=nein` zurueck. Ein bei einem Abbruch halb
  geschriebenes `[progress] perm=N axes_changed=` zaehlt als `abgebrochene_zeile=`, nicht als Fortschritt.
- Bestandslog (Aggregat-Quelle): `COMDARE_BESTANDSLOG=true` + `COMDARE_BESTANDSLOG_DOC_KEY` + erreichbare
  Ebene B. Fehlt eines davon, erscheint **eine** ehrliche `keine Daten`-Zeile -- kein Abbruch, keine
  Reservierung (`status` bindet **keinen** `planer_block`). Wirft der Transport, ist auch das
  Berichts-Inhalt: `[status-bestand] quelle=fehler (<what>)` mit rc `0`, nie rc `2`.
- Scan-Kappen: der Perm-Baum wird tiefen- **und** breiten-begrenzt gelesen. Greift die Breiten-Kappe,
  sagt die Zell-Zeile `scan_gekappt=ja` -- die Bilanz ist dann ausdruecklich unvollstaendig.
- Fehlende Quellen sind Berichts-Inhalt, kein Fehler. Exit-Codes: `0` Bericht, `1` Usage,
  `2` Konfig-Fehler (z.B. kaputte `COMDARE_GOLDEN_N_RANGE`). Kein `watch`/`follow` -- jeder Aufruf ist ein
  Schnappschuss.

Detail-Hilfe: `comdare-experiment-planner help status`.

## 7. Plattform-Hinweise

- **Windows:** aus Git-Bash die `C:/…`-Schreibweise für Pfad-Argumente verwenden.
- **Linux:** Standard-POSIX-Pfade; Exe ohne `.exe`-Endung.
- Cache-Wurzel: Windows `C:\temp\comdare`, Linux `/tmp/comdare/`.
