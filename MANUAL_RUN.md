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
  den Sentinel `unbelegt` statt einer erfundenen Zahl. Dasselbe gilt, wenn es **nichts zu summieren** gibt
  (Plan nicht erhoben, Walk ohne Zelle, oder alle Zellen fremd): eine leere Summe ist keine `0`. Sprengt die
  Summe den Wertebereich, steht der eigene Sentinel `uebergelaufen` -- nie eine umgeklappte Zahl.
- Fortschritts-Cursor: `done=` ist eine Aussage ueber die **zuletzt** gelesene Zeile. Schreibt ein
  Folge-Fenster hinter das `done` weiter, faellt `done=nein` zurueck. Ein bei einem Abbruch halb
  geschriebenes `[progress] perm=N axes_changed=` zaehlt als `abgebrochene_zeile=`, nicht als Fortschritt;
  dasselbe gilt fuer einen Wert mit angehaengtem Muell (`axes_changed=1x`). Die `[status-cursor]`-Zeile
  traegt die Fenster-Zugehoerigkeit **selbst** (`im_fenster=ja|nein`): `done=` gilt nur fuer das eigene
  Fenster, das Fertig-Signal einer fremden Perm steht als `done_fremd=` daneben. Hat sich eine vorhandene
  `progress.cursor` **nie** vollstaendig gemeldet (nur Fragmente/fremde Zeilen), ist `letzte_perm=unbelegt` --
  nicht `0`, denn `0` ist ein echter Cursor-Wert.
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

## 8. Mess-Ergebnisse rendern: `comdare-mess-report render|plan|version` (A9-S4)

Der **Konsument** des A9-S3-xlsx-Writers (`libs/cache_engine/builder/lager_ablage/`): liest
offizielle Mess-CSV(s) und schreibt sie als xlsx- (Default) oder csv-Baum. Eigene Binary
(`tools/mess_report`, Target `comdare_mess_report`) -- kein internes Bau-Werkzeug, sondern eine
Nutzer-CLI fuer die Auswertung.

```bash
cmake --build build --target comdare_mess_report
build/tools/mess_report/comdare-mess-report plan   --realm-root=<dir> --out=<ziel-dir>
build/tools/mess_report/comdare-mess-report render --realm-root=<dir> --out=<ziel-dir>
```

- Quelle: **genau eine** von `--csv=<pfad>` (eine Datei) **oder** `--realm-root=<dir>` (rekursiver,
  tiefen- und eintrags-begrenzter Scan nach `*.result.csv`). `.stale`-Bestaende werden dabei **weder
  gelesen noch angefasst** -- sie werden gezaehlt (`stale_uebersprungen=`), nie eingelesen
  (Messdaten-nie-loeschen-Doktrin).
- `--out=<dir>` ist Pflicht fuer beide Subkommandos (Ziel-Verzeichnis der Mappe).
- `--format=xlsx|csv` -- Default `xlsx`, `csv` ist der Fallback derselben Factory.
- `plan` ist eine **deterministische** Vorschau (schreibt nichts): der Dateiname traegt den fest
  gepinnten Zeitstempel-Platzhalter `19700101-000000` (dieselbe Epochen-Pin-Doktrin wie die
  ZIP-Zeitstempel im xlsx-Backend) -- zwei Laeufe ueber dieselben Quellen liefern byte-gleichen Text.
  `render --dry-run` verhaelt sich gleich, aber mit der echten Wall-Clock-Zeit dieses Aufrufs.
- Sheet-Gruppierung: **eine Gruppe je `binary_id`** (Pflichtspalte im Quell-Header; fehlt sie, bricht
  das Kommando ehrlich ab statt zu raten). Bei `--realm-root` entspricht das "eine Datei = eine
  Gruppe" (jede `per_binary/*.result.csv` traegt bereits nur eine `binary_id`); bei `--csv` gegen eine
  Aggregat-Datei splittet derselbe Algorithmus sie in dieselben Gruppen.
- Dateiname (A9-Doc Abschnitt 5): nur **dynamische** Unter-Achsen-Kandidaten (die ueber die Gruppen
  hinweg mehr als einen Wert tragen) erscheinen im Namen, als `<achse>=sweep`. Konstante/leere
  Kandidaten landen ausschliesslich als Meta-Eintrag im INFO-Blatt, nie im Dateinamen.
- `--fassung3`: verlangt die 8 checkpoint_measure-Pflichtspalten (`prozess`/`thread`/`mess_ebene`/
  `ziel`/`aufrufer`/`checkpoint`/`zeitpunkt`/`messwert`, SOLL-Design ce `cc028e1d`). checkpoint_measure
  ist noch nicht gebaut -- eine Quelle ohne diese Spalten bricht **laut** ab (Exit 3), es gibt **keinen**
  stillen Fassung-1/2-Ruckfall und **keine** Platzhalter.
- Exit-Codes: `0` Erfolg, `1` Usage, `2` Quelle/Gruppierung/Dateiname-Fehler, `3` `--fassung3` ohne
  Pflichtspalten. Ausgaben: Daten/Emissionen -> stdout, Diagnose/Fehler -> stderr (clig.dev).

Detail-Hilfe: `comdare-mess-report help render` bzw. `help plan`.

## 9. Vor dem Push: `vor-push-gate` -- welche CI-Jobs sind lokal gefahren? (2026-08-10)

Beantwortet **eine** Frage, und zwar mit **beiden** Mengen: *welche der CI-Jobs habe ich lokal
gefahren, welche nicht, und was war ihr Ergebnis?* Anlass: die ce-Sammellandung fiel mit 4 von 24
Jobs rot -- und von diesen 24 war vor dem Push **keiner** lokal gefahren worden. Zwei der vier
roten Jobs (`lint:format`, `lint:static`) waren ohne jeden Bau erkennbar.

Das Werkzeug braucht **kein Bauverzeichnis** -- das ist seine tragende Eigenschaft, denn es laeuft
*vor* dem Bau. Es zieht nur die Standardbibliothek:

```bash
# Gebrauchsweg (kein konfigurierter Baum noetig):
g++ -std=c++23 -O1 -o vor-push-gate tools/vor_push_gate/main.cpp
./vor-push-gate                       # voller Lauf aus der Repo-Wurzel
./vor-push-gate --nenner              # nur die Job-Liste mit Bau-Klassifikation
./vor-push-gate --ausfuehrlich        # auch die Ausgabe gruener Jobs zeigen

# Hausweg, wenn ohnehin ein Baum konfiguriert ist:
cmake --build build --target comdare_vor_push_gate
```

- **Der Nenner kommt aus `.gitlab-ci.yml` selbst**, nicht aus einer Liste im Werkzeug. Kommt ein Job
  in die Datei, waechst der Nenner von allein und der neue Job steht automatisch in der Menge der
  *nicht* gefahrenen. Ein Nenner, den man nicht bewegen kann, ist eine Konstante und kein Nenner.
- **Die Ausgabe nennt immer beide Mengen**: `N von M CI-Jobs lokal gefahren; die uebrigen M-N
  namentlich: ...`. Ein Gate, das nur "alles gruen" meldet, ohne zu sagen *welche* Jobs es gefahren
  hat, waere genau der Fehler, den dieses Werkzeug verhindern soll -- eine Ebene hoeher.
- **Fail-closed**: Jobs, deren Skript in einer `ci-templates`-Vorlage liegt, und Jobs, die ein
  Bauverzeichnis brauchen, gelten als *nicht gefahren* und werden namentlich aufgelistet -- nie
  stillschweigend uebersprungen.
- **Werkzeuge** ueber `COMDARE_CLANG_FORMAT` / `COMDARE_CPPCHECK` / `COMDARE_GITLEAKS`
  ueberschreibbar. Fehlt eines, ist das betroffene Gate *offen* (nicht gruen) und wird so gemeldet.
- Exit-Codes: `0` die hier gefahrenen Jobs sind gruen (**keine** Aussage ueber die anderen),
  `1` mindestens ein gefahrener Job rot, `2` Abbruch (kein Repo / keine `.gitlab-ci.yml` /
  Nenner 0 -- ein Nenner 0 ist niemals ein Gruen).

**Verhaeltnis zu `scripts/vor_push_alle_wachen.sh`**: die beiden stellen verschiedene Fragen und
ersetzen einander nicht. Das Skript misst die **Diff-Menge** (`$BASIS...HEAD`, die beruehrten
Dateien) und traegt als einziges die Diff-Hygiene (ASCII + Spaltenbreite) -- die ist gar kein
eigener CI-Job. `vor-push-gate` misst die **CI-Menge**: `lint:format`/`lint:static` fahren im Job
ueber `git ls-files` (am 10.08.2026 waren das 1849 Dateien gegen 55 im Diff), und genau in dieser
Luecke kann das Skript gruen sein, waehrend die CI rot ist. Vor einer Landung beide fahren.

**Grenze, ausdruecklich**: es ersetzt die CI nicht. Es faengt die Fehlerklasse, die *ohne Bau*
erkennbar ist -- am 10.08.2026 waren das 2 der 4 roten Jobs.
