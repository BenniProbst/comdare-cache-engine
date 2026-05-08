# docs/ — INDEX (Top-Level)

**Aktualisiert:** 2026-05-08
**Datei-Konvention:** Jedes Dokument beginnt mit `YYYYMMDD-HHMM-` (Erstellungs-/letzte-Aktualisierungs-Zeitstempel; Doppelpunkt entfaellt wegen Windows-Dateisystem-Restriktion).

## Kategorien

| Verzeichnis | Inhalt | Index |
|-------------|--------|-------|
| [`architecture/`](architecture/INDEX.md) | Architektur-Doku, Verweise auf Termin-7-Quellen | [INDEX](architecture/INDEX.md) |
| [`architekturentscheidungen/`](architekturentscheidungen/) | (leer, Skelett-Slot fuer ADR-Dokumente) | — |
| [`bausteine/`](bausteine/) | (leer, Skelett-Slot fuer Bausteine-Detail) | — |
| [`domaenenmodell/`](domaenenmodell/) | (leer, Skelett-Slot fuer Domaenenmodell-Iterationen) | — |
| [`email/`](email/INDEX.md) | Email-Vorlagen + Kontakt-Liste fuer Code-Anfragen | [INDEX](email/INDEX.md) |
| [`glossare/`](glossare/) | (leer, Skelett-Slot fuer Glossar-Versionen) | — |
| [`lizenzen/`](lizenzen/INDEX.md) | Lizenz-Analysen pro geklontem Repo | [INDEX](lizenzen/INDEX.md) |
| [`status/`](status/INDEX.md) | Projekt-Status-Reports | [INDEX](status/INDEX.md) |

## Wichtige Dokumente (Quick-Access)

| Dokument | Pfad |
|----------|------|
| Status-Bericht (Phase 4.B-detail abgeschlossen) | [status/20260508-1600-status_projekt.md](status/20260508-1600-status_projekt.md) |
| Email-Kontakt-Liste (REV 2 mit Vollangaben-Regel) | [email/20260508-1800-email_kontakte.md](email/20260508-1800-email_kontakte.md) |
| Habich-Folge-Mail (Klarstellung mit Werk-Bloecken) | [email/20260508-1700-email_habich_folge_klarstellung.md](email/20260508-1700-email_habich_folge_klarstellung.md) |
| Lizenz-Analyse aller geklonten Repos | [lizenzen/20260508-1500-lizenzen_uebersicht.md](lizenzen/20260508-1500-lizenzen_uebersicht.md) |
| Architektur-Verweise auf Termin 7 | [architecture/20260504-2300-architektur_referenzen.md](architecture/20260504-2300-architektur_referenzen.md) |

## Verwandte Dokumente ausserhalb von docs/

| Dokument | Pfad (relativ zu Repo-Root) |
|----------|------------------------------|
| Apache-2.0-konforme Attribution | `NOTICE` |
| Hauptlizenz | `LICENSE` |
| Repo-README | `README.md` |
| Original-Repo-Inventar | `Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md` (ausserhalb dieses Repos) |
| Termin-7-Architektur-Quellen | `Diplomarbeit - Datenbanken/20260508 Termin 7/` (ausserhalb) |

## Datei-Namens-Konvention

```
YYYYMMDD-HHMM-<lowercase_kebab_or_snake_case>.md
```

Beispiele:
- `20260508-1800-email_kontakte.md`  ← Kategorie "email", erstellt am 2026-05-08 um 18:00
- `20260504-2300-architektur_referenzen.md`  ← Kategorie "architecture", erstellt 2026-05-04 23:00

**Konvention:**
- 8 Ziffern Datum (YYYYMMDD)
- Bindestrich
- 4 Ziffern Zeit (HHMM, 24h-Format)
- Bindestrich
- Dateiname (lowercase, snake_case oder kebab-case)
- `.md`-Endung

**Bei REV-Updates** (substantielle Aenderung) wird das Datum im Praefix
aktualisiert + Versions-Hinweis im Datei-Header. Datei kann auch unter
neuem Namen kopiert werden, wenn Vorgaengerversion erhalten bleiben soll.
