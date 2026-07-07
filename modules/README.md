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

## SUPERSEDIERT (2026-07-07 — #274 Schritt 1, User-GO)

Die V41.E4-Direktive oben ist durch die Ledger-Entscheide **A1/A6/A7 (§13.9)** und die
**User-Taxonomie vom 07.07.** ueberholt (Doku bleibt als Historie stehen, nichts wird geloescht):

- Die cache-engine ist ein **Framework-MODUL der comdare-Matrix**; ihr domaenenspezifischer Kern wird
  eine **eigene Modules-Familie** (Familien-Name wird beim Anlage-Schritt bestaetigt); die Diplomarbeit
  ist deren **Product und Aussen-Interface**. prt-art wird Achsen-erweiterndes Metaprogrammierungs-Modul
  IN dieser Familie.
- Die Matrix ist **Familien x Baseline-Stufen x Module** (Module = eigenstaendige Zellen-Repos,
  source-only, metaprogrammierungs-offen); Products/Research instanziieren ueber EIGENE Matrizen zu
  Binary-Interfaces. Die 6 hier gelisteten Skelett-Kopien unter `Modules/comdare-cacheengine-all/`
  werden NICHT befuellt: **Archiv/Tombstone**; der Schnitt erfolgt frisch aus einer Scratch-Kopie
  (Migrationsplan Schritt 4, F13).
- Bereits ausgefuehrt: **Schritt 0** — neue Familie `comdare-measurement-all`
  (gitlab comdare/modules/comdare-measurement, Projekt 300).
- Normative Quellen: `docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md` (super) Paragraph 13.12
  GOAL-V3-ERGAENZUNG (Voll-Pfad-Referenzen: GOALV2, Migrationsplan+ADDENDA, Standardprozess
  Research->Product).
