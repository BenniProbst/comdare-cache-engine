# STATUS_PROJEKT — comdare-cache-engine

**Stand:** 2026-05-08 (REV 1 nach Phase 4.B-detail (a)-(g))
**Phase:** 4.B Skelett + Originalcode-Integration

## Was ist hier?

Strukturelle Vorbereitung der Diplomarbeit-Implementation, jetzt mit
12 geklonten Original-Repos in `ext/`, 14 Re-Implementations-Skeletten in
`prt_art/legacy_reimpl/`, NOTICE-Datei und vollstaendiger Lizenz-Analyse.

**Keine** Source-Files mit Logik (kommt in Phase 5+).

## Was ist im Skelett enthalten?

### Phase 4.B Hauptlieferung (2026-05-04)

| Komponente | Status | Datei |
|------------|--------|-------|
| Top-Level CMakeLists.txt | ✅ | `CMakeLists.txt` |
| Apache 2.0 Lizenz | ✅ | `LICENSE` |
| **NOTICE-Datei (alle externen Lizenzen)** | ✅ NEU | `NOTICE` |
| README mit Phase-Status | ✅ | `README.md` |
| .gitignore (Build-Artefakte, Datasets, etc.) | ✅ | `.gitignore` |
| Submodule-Vorlage (Cluster-Migration) | ✅ | `.gitmodules.template` |
| 122 Verzeichnisse (Domaenen 1-6 + Tools + Tests + Docs) | ✅ | (siehe `find . -type d`) |
| 65 Sub-CMakeLists (54 INTERFACE + 11 Passthrough) | ✅ | (siehe `_generate_stubs.py`) |
| CMake-Helper (Submodule-Check, Permutations-Trigger) | ✅ | `cmake/` |
| Tools-Stubs (Codegen, ABI-Test, ZIH-Lieferung, Compiler) | ✅ | `tools/*/README.md` |
| Email-Kontakte fuer fehlende Code-Quellen | ✅ | `docs/email/20260508-1800-email_kontakte.md` |

### Phase 4.B-detail (2026-05-08) — heute abgeschlossen

| Detail-Phase | Status | Datei |
|--------------|--------|-------|
| **(a)** 12 Original-Repos in `ext/` kopiert (~120 MB, ohne `.git`) | ✅ | `ext/<paper>/<repo>/` (13 Verzeichnisse) |
| **(b)** Lizenz-Analyse aller geklonten Repos | ✅ | `docs/lizenzen/20260508-1500-lizenzen_uebersicht.md` |
| **(c)** Email-Recherche P26 + P27 Zhang Erstautoren | ✅ | `docs/email/20260508-1800-email_kontakte.md` (REV 1) |
| **(d)** 14 Re-Implementations-Skelette | ✅ | `prt_art/legacy_reimpl/P*/` (14 Verzeichnisse) |
| **(e)** NOTICE-Datei mit allen externen Lizenzen | ✅ | `NOTICE` (307 Zeilen) |
| **(f)** Bausteine_Matrix.txt mit Implementierungs-Pfaden | ✅ | `Termin 7/Bausteine_Matrix.txt` (REV 1) |
| **(g)** STATUS_PROJEKT.md aktualisiert | ✅ | `docs/status/20260508-1600-status_projekt.md` (dieses Dokument) |

## Originalcode-Integration (NEU 2026-05-08)

### 12 geklonte Original-Repos in `ext/`

| P-ID | Repo | Lizenz | Apache-2.0 | Quelle |
|------|------|--------|------------|--------|
| P01 | unodb (ART + OLC) | Apache-2.0 | ✅ | github.com/laurynas-biveinis/unodb |
| P02 | hot | ISC | ✅ | github.com/speedskater/hot |
| P03 | masstree-beta | MIT | ✅ | github.com/kohler/masstree-beta |
| P04 | CoCo-trie | GPL-3.0 | ⚡ Architekt-Direktive | github.com/aboffa/CoCo-trie |
| P05 | START | MIT | ✅ | github.com/jungmair/START |
| **P06** | **b2-tree-master + bart-master** (NEU) | LICENSE PENDING | ⚡ akademisch | direkt erhalten 2026-05-08 |
| P07 | wormhole | GPL-3.0 | ⚡ Architekt-Direktive | github.com/wuxb45/wormhole |
| P10 | SuRF | Apache-2.0 | ✅ | github.com/efficient/SuRF |
| P20 | leanstore | MIT | ✅ | github.com/leanstore/leanstore |
| P25 | hpides/prefetching | LICENSE PENDING | ⚡ akademisch | github.com/hpides/prefetching |
| P29 | userspace-rcu | LGPL-2.1 | ⚡ DYNAMISCH linken | github.com/urcu/userspace-rcu |
| P30 | haz_ptr | LICENSE PENDING | ⚡ NICHT genutzt (F12-K) | github.com/huangjiahua/haz_ptr |

**Architekt-Direktive 2026-05-08:** GPL/LGPL/keine-Lizenz kein Hinderungsgrund —
modularisierte Bruchstuecke + C++23-Metaprogrammierung = neues Werk im
Forschungskontext.

### 14 Re-Implementations-Skelette in `prt_art/legacy_reimpl/`

Pro Skelett: README.md + CMakeLists.txt + include/<name>.hpp (mit SPDX-Header
Apache-2.0) + src/.gitkeep + tests/.gitkeep.

| P-ID | Paper | Concept-Achse |
|------|-------|---------------|
| P11 | CSS-tree (Rao/Ross 1999) | Page-Type |
| P12 | CSB+-tree (Rao/Ross 2000) | Page-Type |
| P13 | Hankins (2003) | Page-Type / Cost-Model |
| P14 | Samuel CSB-Conscious (2005) | Page-Type / Configuration-Table |
| P16 | Bender Tree Layout (2002) | Memory-Layout |
| P17 | Bender Cache-Oblivious (2005) | Memory-Layout |
| P18 | Saikkonen Multi-Level (2008) | Memory-Layout / Allocator |
| P19 | Saikkonen Layout-Invariant (2016) | Memory-Layout |
| P21 | Chen Prefetching B+ (2001) | Page-Type / Prefetch |
| P22 | Chen Fractal (2002) | Page-Type / Prefetch |
| P23 | Khan Adaptive Prefetch (2010) | Prefetch |
| P24 | NaderanTahan (2016) | Telemetry/Studie |
| P26 | Q. Zhang FGCS Prefetch (2024) | Prefetch / Telemetry |
| P27 | T. Zhang ASPLOS Hierarchical (2025) | Prefetch |

## Wichtige Architektur-Entscheidungen (gem. Termin 7)

| Entscheidung | Quelle |
|--------------|--------|
| 6 Domaenen mit einseitigen Abhaengigkeiten | Domaenenmodell v3 §1+2 |
| CacheEngine = Heap-Singleton im Builder-Prozess | Domaenenmodell v4 D-1, F12-K |
| Hauptcompiler IMMER C++23 + Original-Compiler nur fuer Bausteine | F-EXTRA-1, Domaenenmodell v4 D-3+D-6 |
| Permutations-Identifier = Flag-System (CPUID-Vorbild) | F10-K, Flag_System.txt |
| In-Memory MeasurementBuffer (KEINE externe DB) | F5, Domaenenmodell v3 §1(I)+§5 |
| DecisionLambdaTrees PRO BAUSTEIN (kein globaler Default) | F-EXTRA-6, Domaenenmodell v4 D-2 |
| std::map-API-Vertrag (vollstaendig C++23) | F-EXTRA Glossar v4 BLOCK E |
| Bausteine-Quer-Permutation als zentrales Forschungsprinzip | F15, Domaenenmodell v3 §1(J) |
| Submodule-Ausnahme zur No-Submodules-Regel | F-EXTRA-3, Glossar v4 BLOCK K |
| **Architekt-Direktive: GPL/LGPL kein Hinderungsgrund** | **2026-05-08, Glossar v6 / NOTICE** |

## Naechste Schritte (Stand 2026-05-08)

### Warten auf User / extern (Tasks #74-#77)
- ⏳ Email-Antworten auf 5 verschickte Anfragen (P06, P28, P31, P32, P33)
- ⏳ Habich-Sprechstunde zu P31/P32/P33
- ⏳ Optionale Email-Vorlagen verschicken (P26 Q. Zhang, P27 T. Zhang)
- ⏳ Cluster-Migration: Fortigate-31G + COMDARE-Modules-GitLab-Push

### Zurueckgestellt — autonom bearbeitbar (Tasks #78-#84)
- F-EXTRA-Detail-Implementations (Compiler-Versions-Tabelle, Codegen-Skript,
  DecisionLambdaTree-Stubs, Mess-Default-Hooks, ABI-Stabilitaets-Tools)
- Adapter-Skelette in `adapters/<paper>/`
- STRUKTUR_NOTIZ.md pro geklontem Repo
- Datasets Phase 4.A Implementation (Tooling-Stubs, S/M/L/XL-Generierung)
- Datasets Detail-Spezifikationen OP-1 bis OP-6
- Bausteine_Matrix Detail-Punkte P4B-1 bis P4B-6
- bart-master Cross-Algorithm-Vergleichsbasis verankern

### Phase 5+ (Tasks #85-#88, gesperrt bis UML-GO)
- 🔒 drawio UML Klassendiagramm (Phase 5)
- ⏳ PRT-ART Implementation in `prt_art/`
- ⏳ Re-Implementations starten (14 Skelette ausfuellen)
- ⏳ Permutations-Builds + Experiment-Loop
- ⏳ LaTeX-Anhang fuer Diplomarbeit

## Verzeichnis-Karte

```
comdare-cache-engine/
├── CMakeLists.txt              (Build-Driver)
├── README.md / LICENSE / NOTICE / .gitignore / .gitmodules.template
├── _generate_stubs.py          (Reproduzierbare Stub-Generierung)
├── _copy_ext_repos.py          (Repo-Kopier-Skript)
├── _analyze_licenses.py        (Lizenz-Analyse)
├── _generate_legacy_reimpl.py  (Re-Impl-Skelette-Generator)
├── modules/                    (Submodule-Slot, Phase 4.B Migration)
├── ext/                        ✅ 12 Original-Repos (120 MB)
│   ├── P01-ART/unodb/                  (Apache-2.0)
│   ├── P02-HOT/hot/                    (ISC)
│   ├── P03-Masstree/masstree-beta/     (MIT)
│   ├── P04-CoCo-trie/CoCo-trie/        (GPL-3.0 — Architekt-Direktive)
│   ├── P05-START/START/                (MIT)
│   ├── P06-B2tree/                     (LICENSE PENDING — direkt erhalten)
│   │   ├── b2-tree-master/             (Haupt-B²-tree-Implementation)
│   │   └── bart-master/                (Cross-Algorithm-Vergleichsbasis)
│   ├── P07-Wormhole/wormhole/          (GPL-3.0 — Architekt-Direktive)
│   ├── P10-SuRF/SuRF/                  (Apache-2.0)
│   ├── P20-BTreesAreBack/leanstore/    (MIT)
│   ├── P25-Mahling/prefetching/        (LICENSE PENDING)
│   ├── P29-RCU/userspace-rcu/          (LGPL-2.1 — dynamisch linken)
│   └── P30-HazardPointers/haz_ptr/     (LICENSE PENDING — nicht genutzt)
├── adapters/                   (TBD: 12 Adapter-Skelette)
├── prt_art/                    (PRT-ART Test-Algorithmus, eigen)
│   └── legacy_reimpl/          ✅ 14 Re-Impl-Skelette
│       ├── P11-CSS-tree/
│       ├── P12-CSB-tree/
│       ├── P13-Hankins/
│       ├── P14-Samuel/
│       ├── P16-Bender-TreeLayout/
│       ├── P17-Bender-CacheOblivious/
│       ├── P18-Saikkonen-MultiLevel/
│       ├── P19-Saikkonen-LayoutInvariant/
│       ├── P21-Chen-PrefetchBPlus/
│       ├── P22-Chen-Fractal/
│       ├── P23-Khan-AdaptivePrefetch/
│       ├── P24-NaderanTahan/
│       ├── P26-Zhang-FGCS/
│       └── P27-Zhang-ASPLOS-Hierarchical/
├── search_engine/              (Domaene 1)
├── cache_engine/               (Domaene 2 — KERNBEITRAG)
├── engine_choice/              (Domaene 5)
├── measurement/                (Domaene 3)
├── hardware_isa/               (Domaene 4)
├── tools/                      (5 Tooling-Stubs)
├── benchmarks/                 (YCSB + Mikrobenchmarks + Plattform-Kalibrierung)
├── tests/                      (Generic + Module-Specific Google-Tests)
├── docs/                       ✅ Architektur-Doku, Email-Kontakte, Lizenzen
│   ├── architecture/README.md
│   ├── EMAIL_KONTAKTE.md       (REV 1 mit Status der Anfragen)
│   ├── LIZENZEN_UEBERSICHT.md  (NEU 2026-05-08, mit Architekt-Direktive)
│   └── STATUS_PROJEKT.md       (dieses Dokument)
├── cmake/                      (Helper-Module)
├── build/                      (gitignored)
└── datasets/                   (gitignored, sehr gross)
```

## Statistik (Stand 2026-05-08)

| Metrik | Wert |
|--------|------|
| Verzeichnisse | 122 + 56 (ext + legacy_reimpl Sub-Tree) ~178 |
| CMakeLists.txt | 76 (11 echt + 65 Stubs) |
| Original-Repos in `ext/` | 13 Repo-Klone (12 Paper, P06 hat 2 Sub-Repos) |
| Re-Impl-Skelette in `prt_art/legacy_reimpl/` | 14 |
| `ext/`-Speicher | ~120 MB (ohne .git) |
| Externe Lizenzen | 7 verschiedene (Apache, MIT, ISC, BSD, GPL-3, LGPL-2.1, kein) |
| NOTICE-Datei | 307 Zeilen |
| Email-Status | 5 verschickt (P31/P32/P33/P06/P28) + 5 als Vorlage (NICHT mehr Pflicht) |
| Generator-Skripte | 4 Python (`_generate_stubs.py`, `_copy_ext_repos.py`, `_analyze_licenses.py`, `_generate_legacy_reimpl.py`) |

## Was fehlt zur Build-Faehigkeit

Das Skelett **buildet noch nicht** (keine Source-Files in den eigenen
Komponenten — nur INTERFACE-Stubs). Damit der Skelett-Build erfolgreich
durchlaeuft, muss in Phase 5+ pro INTERFACE-Library mindestens ein Header
existieren. Die `add_executable(cache_engine_builder)`-Definition in
`cache_engine/builder/CMakeLists.txt` ist auskommentiert, bis `main.cpp`
existiert.

Die Original-Repos in `ext/` koennten theoretisch separat gebaut werden
(jedes hat seine eigene CMakeLists), sind aber noch nicht in den Top-Level-
Build eingebunden — kommt in Phase 5+.

## Cross-Reference

| Aspekt | Dokument |
|--------|----------|
| Forschungs-These (Bausteine-Quer-Permutation) | F15 / Domaenenmodell v3 §1(J) |
| Bausteine-Liste mit Implementierungs-Pfaden | `Termin 7/Bausteine_Matrix.txt` (REV 1) |
| Permutations-Identifier-Schema | `Termin 7/Flag_System.txt` |
| Architektur-Beschluesse | `Termin 7/Architekturentscheidungen_F1_F15.txt` |
| Domaenenmodell | `Termin 7/Domaenenmodell_PRT_ART_v3.txt` + `_v4_DELTA.txt` |
| Begriffsglossar | `Termin 7/Begriffsglossar_v3..v6_FINAL.txt` |
| Datasets-Spezifikation | `Termin 7/Datasets_Spezifikation.txt` |
| Originalcode-Repo-Inventar | `Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md` |
| Email-Kommunikations-Log | `docs/email/20260508-1800-email_kontakte.md` |
| Lizenz-Bewertungen | `docs/lizenzen/20260508-1500-lizenzen_uebersicht.md` |
| Externe Lizenz-Attribution | `NOTICE` |
