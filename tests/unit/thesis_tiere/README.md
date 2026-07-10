# `thesis_tiere/` — ADHOC-Standalone-Tests & Mess-Treiber (Reproduktion)

Stand: 2026-06-20 (#155 + #171). Dieses Verzeichnis enthält **Standalone-Tests und Mess-Treiber**, die
bewusst **NICHT** über den normalen CMake-/CTest-Pfad (`comdare_add_test` in `tests/unit/CMakeLists.txt`)
gebaut werden, sondern über committete PowerShell-Skripte mit einem **eigenen, schweren ADHOC-Include-Satz**.

## Warum NICHT in CMake/CTest registriert?

Die `comdare_add_test`-Targets nutzen einen schlanken, stabilen Include-Satz
(`libs/cache_engine{,/include}`, `libs/common`, ggf. `generated/` + `Boost::mp11`). Die Treiber hier
brauchen dagegen ein **anderes, viel breiteres Build-Rezept**, das sich nicht ohne Risiko in reguläre
CMake-Targets gießen lässt:

1. **Voller rekursiver `generated/`-Include-Tree.** Die Skripte glob­ben `build/msvc-release/generated`
   **rekursiv** (`Get-ChildItem -Recurse -Directory`) als Include-Roots. Diese Verzeichnisse entstehen
   erst zur Configure-Zeit per `configure_file()` (NACH `add_subdirectory(tests)`). Ein CMake-`file(GLOB)`
   des Binary-Trees beim 1. Configure ist leer — exakt der in `tests/unit/CMakeLists.txt` (Z. 4–10)
   dokumentierte **„2×-configure"-Bug**. Eine Target-Verdrahtung müsste diese Fragilität reproduzieren.
2. **`cl` direkt + `/Od /bigobj /std:c++latest`.** Die m3v2-Achsen-ConfigSets sind mp11-schwer; ohne
   `/bigobj` bricht der MSVC-Übersetzer mit zu vielen Sektionen. Das ist ein ADHOC-`cl`-Aufruf, **nicht**
   der MSBuild-/Ninja-Toolchain-Pfad von CMake.
3. **Manuelles Mitkompilieren von Nicht-Header-Quellen** (z. B. `xml_config_parser.cpp`).
4. **Teilweise echter DLL-/Tier-Bau** (die Mess-Treiber, s. u.) — zu schwer und zu langsam für `ctest`.

Würde man diese als `ctest`-Einträge führen, die zur Test-Zeit `vcvars64` + `pwsh` + den schweren
ADHOC-`cl`-Lauf auslösen, wäre `ctest` langsam, umgebungs-sensitiv (vcvars/pwsh) und nicht plattform-
portabel (ZIH-Linux ohne MSVC). Das widerspricht der Direktive „der sauberste, nicht der einfachste Weg":
ein langsamer, fragiler ctest-Eintrag ist KEIN Gewinn gegenüber der dokumentierten, committeten Reproduktion.

> **Hinweis (Gegenbeispiel):** Drei *andere* Phase-E-Standalone-Tests liegen NICHT hier, sondern direkt
> unter `tests/unit/` und nutzen den **normalen** Anatomie-/Boost-Include-Satz — `test_migration_two_tier`,
> `test_seg_coverage` und `test_v5_io_real_fixture`. Diese SIND seit #155 als reguläre CMake-Targets +
> `add_test` registriert (s. `tests/unit/CMakeLists.txt`, Block „#155 Phase-E-Standalone-Tests").

## Reproduktion der ADHOC-Tests (read-only / leicht — kein DLL-Bau)

Jeder Test hat ein committetes Build-+Run-Skript. `cmake --preset msvc-release` (Configure) muss einmal
gelaufen sein (erzeugt `build/msvc-release/generated/`). Dann je Test:

> **Limits-Entkopplung (2026-07-10):** `profile_run_entry.hpp` konsumiert seit der Produktivumschaltung den
> BUILD-zeitigen `generated_source_catalog.hpp`. Die Skripte `build_run_profile_union.ps1` und
> `build_and_measure_150_tiere.ps1` bauen das Codegen-Target `comdare_limits_generated_source_catalog`
> deshalb selbst idempotent mit (regeneriert auch bei geaendertem `m3v2_study.profile.xml`).

| Test (`*.cpp`)              | Build-/Run-Skript                | Inhalt | DLL-Bau |
|-----------------------------|----------------------------------|--------|---------|
| `test_validate_profile`     | `build_validate_profile.ps1`     | rein-lesende `--validate`-Gate (m3v2-Profil ok / Tippfehler gefangen) | nein |
| `test_profile_roundtrip`    | `build_profile_roundtrip.ps1`    | Profil → parse → re-emit → Round-Trip-Gleichheit | nein |
| `test_run_profile_union`    | `build_run_profile_union.ps1`    | Vereinigung mehrerer Mess-Profile (CEB::run_profile) | nein |
| `test_axis_sweep_pilot`     | `build_axis_sweep_pilot.ps1`     | Achsen-Sweep-Pilot (per-Achse) | nein |
| `test_sota_series_pilot`    | `build_sota_pilot.ps1`           | SOTA-Mess-Serie (A/B/C-Reihen-Pilot) | nein |
| `test_node_delegation_proof`| `build_node_delegation_proof.ps1`| node-Achse delegiert echt (alloc_cnt node-abhängig) | nein |
| `test_layout_aware_store`   | (kein eigenes Skript)            | LayoutAwareChunkedStore (soa/aosoa/packed) — siehe `build_and_measure_thesis_tiere.ps1` | nein |
| `test_pruefling_type_pilot` | `build_pruefling_type_pilot.ps1` | #171 additive `pruefling_type`-Spalte (full/abstract/-) durch die REALE `lazy_csv_header`/`format_csv_row`/`build_sota_passes`-Kette; PRT-ART in BEIDEN Ausprägungen | nein |

Beispiel:

```powershell
# 1) Configure (einmalig) erzeugt generated/
cmake --preset msvc-release
# 2) Leichten read-only-Test bauen + laufen (cl /Od /bigobj, KEIN DLL):
pwsh tests/unit/thesis_tiere/build_validate_profile.ps1   # Exit 0 = grün
pwsh tests/unit/thesis_tiere/build_profile_roundtrip.ps1  # Exit 0 = grün
```

## Mess-Treiber (schwer — echter DLL-/Tier-Bau, NICHT als Test gedacht)

Diese erzeugen die Thesis-Messreihen (CSV) und bauen reale Tier-DLLs — bewusst außerhalb ctest:

| Treiber-Quelle                         | Orchestrierungs-Skript                  | Zweck |
|----------------------------------------|-----------------------------------------|-------|
| `run_lazy_150.cpp` / `tier150_axis_grid.cpp` | `build_and_measure_150_tiere.ps1` | 150-Tier-Achsengitter → `tier150_measurements.csv` |
| `thesis_sa_*.cpp` / `thesis_nt_*.cpp`  | `build_and_measure_thesis_tiere.ps1`    | 8 SA- + 3 NT-Tiere → `thesis_measurements.csv` |
| `measure_adapter_tiere.cpp`            | `build_and_measure_adapter_tiere.ps1`   | Adapter-Gattung → `adapter_measurements.csv` |
| `gen_golden_fullpilot.cpp`             | (`golden_fullpilot_320_binary_ids.txt`) | 320-Golden-Binary-IDs (FullPilot) |

Diese Treiber bauen pro Tier echte DLLs (`adhoc_emitter` + `cl`/`link`) und laufen Minuten bis Stunden;
sie gehören in den Mess-Workflow (lokal / ZIH-SLURM), nicht in die Unit-Test-Suite.

## NICHT als ctest registriert — Begründung (Audit-ehrlich)

- **8 ADHOC-Tests** (`test_validate_profile`, `test_profile_roundtrip`, `test_run_profile_union`,
  `test_axis_sweep_pilot`, `test_sota_series_pilot`, `test_node_delegation_proof`, `test_layout_aware_store`,
  `test_pruefling_type_pilot`): brauchen den schweren rekursiven-`generated/`-Include-Satz + `cl /Od /bigobj`
  + manuelle Nicht-Header-Quellen. Reproduzierbar via die committeten `build_*.ps1`. → **dokumentiert, nicht
  registriert** (ctest-Eintrag wäre langsam + vcvars/pwsh-gebunden + nicht ZIH-portabel).
  - `test_pruefling_type_pilot` (#171) baut zwar KEINE DLL, zieht aber `sota_catalog.hpp` + `profile_runner.hpp`
    + `builder/experiment_tree/cache_engine_builder_iterator.hpp` MIT dem vollen rekursiven `generated/`-Tree
    + `Boost::mp11` + dem manuell mitkompilierten `xml_config_parser.cpp` (s. `build_pruefling_type_pilot.ps1`
    Z. 19/24) — identischer schwerer Include-Satz wie die SOTA-Pilot-Harness, daher ctest-untauglich (gehört
    zur ADHOC-Reproduktion, NICHT in die schlanke ctest-Suite). Reproduktion: `pwsh build_pruefling_type_pilot.ps1`.
- **Mess-Treiber** (`run_lazy_150`, `tier150_axis_grid`, `thesis_sa_*`, `thesis_nt_*`,
  `measure_adapter_tiere`, `gen_golden_fullpilot`): echter DLL-/Tier-Bau, Laufzeit Minuten–Stunden.
  → **bewusst kein Test**; gehören in den Mess-Workflow.
