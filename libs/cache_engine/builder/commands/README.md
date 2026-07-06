# cache_engine/builder/commands/ — V32.DD.1 Command-Pattern (AA.2 Korrektur)


> **CMD-1 (c) (#267, 2026-07-06):** `compare_engine_command.hpp` und `auto_permutate_axis_command.hpp` wurden mit 0-Konsumenten-Beweis ENTFERNT (Nachfolger der Command-Semantik: compile-time `AxisCommand`, topics/axis_command_base.hpp). `i_command.hpp` + `execute_engine_command.hpp` bleiben als dokumentierte V32-/ABI-Adapter-Ausnahme (GEPARKT-Notiz in den Headern). Die Tabellen unten beschreiben den historischen V32-Stand.
**Stand:** 2026-05-18 (V32.DD.1 Skelett)
**Trigger:** User-Direktive 2026-05-18 (AA.2 KRITISCHE KORREKTUR)

## Zweck

Command-Pattern-Hierarchie fuer den CacheEngineBuilder als **Test-Treiber** (statt nur "CE-Builder"). Loest das im M-Modell identifizierte Design-Problem:

> "Beide [CacheEngine und PRT-ART] werden in erster Linie durch die CacheEngineBuilder orchestriert."

## Klassen-Inventar

| Header | Klasse | Funktion |
|---|---|---|
| `i_command.hpp` | `ICommand` | Abstract base mit `execute()`, `command_name()`, `is_parallelizable()` |
| `execute_engine_command.hpp` | `ExecuteEngineCommand` | fuehrt eine EE auf einer Permutation aus |
| `compare_engine_command.hpp` | `CompareEngineCommand` | vergleicht 2 EE-Ergebnisse (F15-Forschungsmission) |
| `auto_permutate_axis_command.hpp` | `AutoPermutateAxisCommand` | lookup CE-Bibliothek + permutiert fehlende Achse (AA.3) |

## V32.DD.1 Status

Aktuell **SKELETT-Implementation**. Konkrete `execute()`-Bodies folgen in V32.1-V32.4 Sprint:

- `ExecuteEngineCommand::execute()` braucht: `engine_->configure(perm) + engine_->execute(workload) + collect_result()`
- `CompareEngineCommand::execute()` braucht: Result-Vergleichs-Metriken (Throughput / Latency / CacheMiss / MemoryFootprint)
- `AutoPermutateAxisCommand::execute()` braucht: `discover_axis_implementations + platform_filter + user_limit + run + compare`

## Build-Integration

Aktuell **header-only** (Skelett). Wenn Bodies hinzukommen: CMakeLists.txt-Eintrag in `cache_engine/builder/CMakeLists.txt`.

## Querverweise

- M-Modell-Korrektur: `../../../../docs/architektur/10_schichten_modell_M.md` §0 AA.2 (in Diplomarbeit-Repo)
- Default-Lookup-Korrektur: `../../../../docs/uml_planning/Z5_master_index_und_gap_analyse.md` §0 AA.3
- V32-Plan: `../../../../docs/adapters/V32_CODE_REFACTORING_PLAN.md`
- drawio M-CORRECT-V2 Tab 50 + Default-Lookup Tab 51 in `phase5_uml_detail_REV7.drawio`
