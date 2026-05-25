# cache-engine modules/

**V41.E4 (2026-05-25):** Diese Verzeichnisstruktur ersetzt das frueher
geplante Git-Submodule-Konzept. Die 6 Module sind cache-engine INTERN
(eine Cache-Engine = 6 funktionale Saeulen) und werden nicht separat
versioniert. cache-engine wird als Gesamt-Framework getrackt.

## Module

| Modul | Rolle |
|-------|-------|
| comdare-search-engine | Suchalgorithmen (Trie, B+Tree, ART) |
| comdare-cache-engine-core | Kern-Cache-Verwaltung |
| comdare-measurement | PMU-Counter + Telemetrie |
| comdare-isa-dispatch | SIMD-Family-Erkennung + Dispatch |
| comdare-build-tools | Build-Helpers, Codegen |
| comdare-test-system | Testinfrastruktur |

## Status

Skelette (CMakeLists.txt + README.md + include/.gitkeep). Inhaltliche
Befuellung erfolgt in Phase 6+ (siehe MASTERPLAN_KONSOLIDIERUNG_TERMINE
der Diplomarbeit).

## Aenderung zur Vor-V41.E4 Variante

Frueher: `.gitmodules.template` mit URLs auf gitlab.comdare.de (Plan:
nach Cluster-Migration zu .gitmodules umbenennen). User-Direktive
2026-05-25: Module sind cache-engine intern, kein Submodule-Mechanismus.
Template umbenannt zu .gitmodules.template.obsolete (Historie).
