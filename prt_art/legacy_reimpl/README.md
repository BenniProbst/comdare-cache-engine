# prt_art/legacy_reimpl/ — DEPRECATED (REV 7.6 V8.2)

**Migrations-Status:** Diese Re-Implementations-Skelette werden in einer
Folgesession nach `comdare-prt-art/prt_art/legacy_reimpl/` verschoben.

**Anlass:** User-Klarstellung 2026-05-13/14
(`Diplomarbeit/STRUCTURAL_CORRECTION_diplomarbeit.md` §2.2):

> *"Derzeit gibt es faelschlicherweise einen PRT-ART Ordner direkt in
> der CacheEngine, aber der PRT-ART sollte mit neuen Layered
> Algorithmus-Baustein-Bestandteilen und einer separaten eigenen
> Konfiguration im PRT-ART repo als Pruefling dargestellt werden, um
> den Fall einer Beitrittspruefung eines Algorithmus zum Stand der
> Technik zu zeigen."*

PRT-ART ist als **Pruefling** in seinem eigenen Repo (`comdare-prt-art`)
und gehoert NICHT in die cache-engine. Die in diesem Ordner liegenden
Re-Implementations-Skelette der TIER-2/3-Paper (P11-P27) sind PRT-ART-
Konkretisierungen der LEGACY-Algorithmen, die heute fuer den Vergleich
der hybride PrtArtSearchEngine gegen Stand-der-Technik verwendet werden.

---

## Migrations-Plan (gestaffelt)

| Phase | Aktion | Status |
|---|---|---|
| 1 (heute V8.2) | `COMDARE_BUILD_LEGACY_REIMPL=OFF` (Default) — verhindert versehentliches Bauen | DONE |
| 2 (Folgesession) | Skelette nach `comdare-prt-art/prt_art/legacy_reimpl/` verschieben | TBD |
| 3 (Folge-Folge) | `prt_art/`-Ordner aus cache-engine entfernen | TBD |

Die Staffelung vermeidet Build-Brueche fuer heutige Konsumenten waehrend
der Uebergangsphase.

---

## Aktuell (vor Migration)

```
prt_art/legacy_reimpl/
├── P11-CSS-tree/                    Rao/Ross 1999 — CSS-Tree
├── P12-CSB-tree/                    Rao/Ross 2000 — CSB+ Tree
├── P13-Hankins/                     Hankins/Patel 2003 — Node Size
├── P14-Samuel/                      Samuel/Pedersen/Bonnet 2005 — Processor Conscious CSB+
├── P16-Bender-TreeLayout/           Bender/Demaine/Farach-Colton 2002 — Tree Layout
├── P17-Bender-CacheOblivious/       Bender et al 2005 — Cache-Oblivious B-Trees
├── P18-Saikkonen-MultiLevel/        Saikkonen/Soisalon-Soininen 2008 — Multi-Level
├── P19-Saikkonen-LayoutInvariant/   Saikkonen/Soisalon-Soininen 2016 — Layout-Invariant
├── P21-Chen-PrefetchBPlus/          Chen/Gibbons/Mowry 2001 — Prefetching B+ Trees
├── P22-Chen-Fractal/                Chen et al 2002 — Fractal Prefetching
├── P23-Khan-AdaptivePrefetch/       Khan 2010 — Adaptive Prefetch
├── P24-NaderanTahan/                Naderan-Tahan/Sarbazi-Azad 2016
├── P26-Zhang-FGCS/                  Zhang et al FGCS 2024
└── P27-Zhang-ASPLOS-Hierarchical/   Zhang et al ASPLOS 2025 — Hierarchical
```

---

## Wie aktivieren (nur fuer Folge-Migration / Verifikation)?

```bash
cmake -B build -DCOMDARE_BUILD_LEGACY_REIMPL=ON
```

**Hinweis:** Default ist OFF. Aktivierung nur sinnvoll, solange die
Migration zu prt-art noch nicht abgeschlossen ist.

---

## Querverweis

- `Diplomarbeit/STRUCTURAL_CORRECTION_diplomarbeit.md` §2.2
- `Diplomarbeit/docs/sessions/20260514-0900-v8-implementations-anker-rev7-6.md` V8.2
- `docs/sessions/20260514-0900-v8-cache-engine-strukturkorrekturen.md` V8.2
- Ziel-Repo: `https://github.com/BenniProbst/comdare-prt-art` `prt_art/legacy_reimpl/`
