# V41.E10 — STATIC/SHARED-Bibliotheks-Achse (pro Projekt / pro Untermodul)

**Stand:** 2026-05-31 · **Status:** E10.1 (Audit) + E10.2 (Option + Helper) ✅ done-verified

## Ziel

Der Linkage-Typ der COMDARE-First-Party-Bibliotheken (STATIC vs. SHARED) soll **nicht** in jeder
einzelnen `CMakeLists.txt` hart verdrahtet sein, sondern **pro logischem Projekt** über eine
CMake-Option steuerbar — Default **STATIC** (keine Regression, ein selbst-enthaltenes Build-Artefakt
pro Konsument), SHARED optional für Cluster-/Plugin-Szenarien.

## E10.1 — `add_library`-Audit

Vollständige Klassifikation aller `add_library()`-Aufrufe in `libs/` + `apps/` (ohne `ext/` 3rd-party,
ohne `build/`):

| Typ | Anzahl | E10-relevant? | Begründung |
|-----|-------:|---------------|------------|
| `INTERFACE` | 62 | **nein** | header-only, kein Linkage-Typ (keine Objektdateien) |
| `STATIC`    | 11 | **JA** | die `.cpp`-tragenden First-Party-Libs = Toggle-Ziele |
| `ALIAS`     | 12 | nein | Namens-Alias, kein eigenes Target |
| per-Permutations-DLL | (programmatisch) | **nein** | `comdare_build_adhoc_modules` — MÜSSEN SHARED sein |

### Die 11 STATIC-Toggle-Ziele (je logischem Projekt-Schlüssel)

| Projekt-Schlüssel | Library-Target | Pfad |
|-------------------|----------------|------|
| `CACHE_ENGINE` | comdare_anatomy_codegen_tool | libs/cache_engine/builder/anatomy_codegen_tool |
| `CACHE_ENGINE` | comdare_anatomy_module_loader | libs/cache_engine/builder/anatomy_module_loader |
| `CACHE_ENGINE` | comdare_builder_codegen | libs/cache_engine/builder/codegen |
| `CACHE_ENGINE` | comdare_builder_experiment_driver | libs/cache_engine/builder/experiment_driver |
| `CACHE_ENGINE` | comdare_builder_experiment_runner | libs/cache_engine/builder/experiment_runner |
| `CACHE_ENGINE` | comdare_module_loader | libs/cache_engine/builder/module_loader |
| `CACHE_ENGINE` | comdare_builder_permutation_loop | libs/cache_engine/builder/permutation_loop |
| `CACHE_ENGINE` | comdare_workload_driver | libs/cache_engine/builder/workload_driver |
| `COMMON` | comdare_builder_xml_config_parser | libs/common/serialization/xml_config_parser |
| `EXECUTION_ENGINE` | comdare_experiment | libs/execution_engine |
| `TEST_INFRA` | comdare_workload_generator | libs/test_infra/workload_generator |

## E10.2 — Option + `comdare_add_library`-Helper

`cmake/comdare_add_library.cmake` führt einen einheitlichen Wrapper ein:

```cmake
comdare_add_library(<target> PROJECT <KEY> SOURCES <src...>)
```

**Zwei Steuer-Ebenen:**

| Ebene | Variable | Default |
|-------|----------|---------|
| global | `COMDARE_BUILD_SHARED_LIBS` | `OFF` (STATIC) |
| pro Projekt | `COMDARE_<KEY>_BUILD_SHARED_LIBS` | folgt dem globalen Wert |

Bei SHARED setzt der Helper auf MSVC `WINDOWS_EXPORT_ALL_SYMBOLS ON` (Toggle ohne `__declspec`-Quell-
Annotation) + `POSITION_INDEPENDENT_CODE ON` (ELF). INTERFACE-Libs + die per-Permutation-DLLs bleiben
bewusst ausserhalb des Helpers.

## Verifikation (2026-05-31)

| Schritt | Ergebnis |
|---------|----------|
| Configure Default (alle STATIC) | ✅ `Configuring/Generating done`, exit 0 |
| Build abhängiger Test (`test_workload_and_experiment`) | ✅ grün; Artefakt `comdare_workload_generator.lib` (STATIC, kein `.dll`) |
| `-DCOMDARE_TEST_INFRA_BUILD_SHARED_LIBS=ON` → Build `comdare_workload_generator` | ✅ erzeugt `comdare_workload_generator.dll` (SHARED) |
| Toggle zurück `=OFF` → Rebuild | ✅ wieder `.lib`, kein `.dll` (non-destruktiv) |

## Abgrenzung / offen (korrekt §(b), extern)

- **E10.6/E10.7** (Cluster-Build-Layout, Verteil-Topologie der SHARED-Libs über ZIH-Nodes) bleiben
  termin-/extern-abhängig — NICHT Teil dieser lokalen CMake-Achse.
