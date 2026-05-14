# libs/deprecated/ — Folly-Stil obsoleter Code

**Eingefuehrt:** V23.E (2026-05-14)
**Konvention:** Folly-`experimental/`-Idiom invertiert. Code hier wird
spaeter geloescht, ist aber temporaer noch zugreifbar.

## Mitglieder

| Pfad | Status | Begruendung | Removal-Date |
|---|---|---|---|
| `prt_art_legacy/` | DEPRECATED | Memory-Direktive: PRT_ART gehoert in comdare-prt-art Repo. Echter Code dort, hier nur Vor-Migrations-Skelett (V23.E). | nach Habich-Termin~9 final loeschen |

## CMake-Aktivierung

Per Default NICHT gebaut. Aktivieren via:
```bash
cmake -DCOMDARE_BUILD_DEPRECATED=ON ...
```

## Querverweis

- Memory-Direktive: `feedback_prt_art_consumes_cache_engine.md`
- V23-Layout-Vorschlag: `docs/sessions/20260514-3000-v23-layout-vorschlag.md`
- comdare-prt-art Repo (Haupt-Pruefling): https://github.com/BenniProbst/comdare-prt-art
