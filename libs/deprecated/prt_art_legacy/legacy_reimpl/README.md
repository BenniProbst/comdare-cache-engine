# prt_art/legacy_reimpl/ — MIGRATED (REV 7.6 V10.1)

**Status:** Die 14 Pruefling-Re-Implementations (P11-P27) wurden physisch
nach `comdare-prt-art/prt_art/legacy_reimpl/` verschoben.

**Aktion V10.1 (2026-05-14):** `git rm -rf` aller P*-Subordner aus diesem
Verzeichnis. Der Ordner bleibt nur als Migrations-Marker.

---

## Wo finde ich die Pruefling-Re-Implementations heute?

```
github.com/BenniProbst/comdare-prt-art:
  prt_art/legacy_reimpl/
  ├── P11-CSS-tree/                 (Rao/Ross 1999)
  ├── P12-CSB-tree/                 (Rao/Ross 2000)
  ├── P13-Hankins/                  (Hankins/Patel 2003)
  ├── P14-Samuel/                   (Samuel/Pedersen/Bonnet 2005)
  ├── P16-Bender-TreeLayout/        (Bender et al. 2002)
  ├── P17-Bender-CacheOblivious/    (Bender et al. 2005)
  ├── P18-Saikkonen-MultiLevel/     (Saikkonen 2008)
  ├── P19-Saikkonen-LayoutInvariant/(Saikkonen 2016)
  ├── P21-Chen-PrefetchBPlus/       (Chen 2001)
  ├── P22-Chen-Fractal/             (Chen 2002)
  ├── P23-Khan-AdaptivePrefetch/    (Khan 2010)
  ├── P24-NaderanTahan/             (Naderan-Tahan 2016)
  ├── P26-Zhang-FGCS/               (Zhang FGCS 2024)
  └── P27-Zhang-ASPLOS-Hierarchical/(Zhang ASPLOS 2025)
```

---

## Migrations-Geschichte

| Phase | Aktion | Commit |
|---|---|---|
| V8.2  | DEPRECATED-Marker + Build OFF (cache-engine) | `4048638` |
| V9.4  | Physische Verschiebung (cp -r) nach prt-art-Repo | prt-art `fbda49c` |
| V10.1 | `git rm -rf` aus cache-engine (heute) | (in diesem Push) |

---

## Konsumtions-Pfad

PRT-ART nutzt diese Pruefling-Re-Implementations als Beitritts-Verifikation
gegen die echten cache-engine SOTA-Adapter. Aktivierung:

```bash
cd comdare-prt-art
cmake -B build -DCOMDARE_PRT_ART_BUILD_LEGACY_REIMPL=ON
cmake --build build
```

---

## Querverweis

- Master-Doku: `Diplomarbeit/STRUCTURAL_CORRECTION_diplomarbeit.md` §2.2
- V8.2 Migrations-Plan: `docs/sessions/20260514-0900-v8-cache-engine-strukturkorrekturen.md`
- V9.4 Verschiebung: `Diplomarbeit/docs/sessions/20260514-1230-v9-final-stand-und-naechste-schritte.md`
- V10.1 Loeschung: `Diplomarbeit/docs/sessions/20260514-1300-v10-anker.md`
