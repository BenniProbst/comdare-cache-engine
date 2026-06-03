# Doku 20 — cache-engine als Plugin-Controller (Anforderungs-Erweiterung)

**Stand:** 2026-05-29
**Typ:** Anforderungs-Erweiterung (User-Direktive 2026-05-29)
**Bezug:** E11 (#16 Facade + Pruefling-Factory), F.6 (#11 Migration), F.2 (#12 Namespaces), E6 (#22 nested-cleanup), F.5 (#15 Dreigliedrigkeit)

---

## §1 Anforderung (User-Wortlaut sinngemäß, 2026-05-29)

> Die cache-engine lädt den prt-art als **temporäres, cache-engine-konformes Repository
> zum Testen — parallel auf derselben Ebene ihres Wurzelordners**. prt-art soll als
> **Modul in die cache-engine importierbar** sein; das gilt später für **multiple
> Prüflings-Algorithmen rein per Konfiguration** aus der Anforderungs-Konfig der
> Diplomarbeit an die cache-engine. Die cache-engine ist damit **auch Plugin-Controller**
> zusätzlich zu ihren Framework-Eigenschaften. Die Diplomarbeit verwendet den
> cache-engine-Plugin-Controller, um Prüflinge wie den prt-art **als cache-engine-konformes
> Repo als Gesamtsystem zu laden und zu bauen**.

---

## §2 Korrigiertes Architektur-Modell (Lade-Richtung)

```
        Diplomarbeit (Mess-/Orchestrierungs-Projekt)
              │  Anforderungs-Konfig: welche Prüflinge?  (COMDARE_CE_PRUEFLINGE = prt-art; ...)
              ▼
   ┌─────────────────────────────────────────────────────────┐
   │  comdare-cache-engine                                     │
   │  ── Framework  (21 Achsen + PermutationEngine + Tools)    │
   │  ── PLUGIN-CONTROLLER                                      │
   │       lädt + baut konfigurierte Prüfling-Repos PARALLEL   │
   │       auf Wurzel-Ebene, registriert deren Achsen          │
   └─────────────────────────────────────────────────────────┘
              │  lädt parallel (temporär, zum Testen)
              ▼
   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
   │ comdare-     │   │ <pruefling-2>│   │ <pruefling-N>│
   │ prt-art      │   │              │   │              │
   │ (CE-konform) │   │ (CE-konform) │   │ (CE-konform) │
   └──────────────┘   └──────────────┘   └──────────────┘
```

**Richtung (verbindlich):** cache-engine → **lädt** → Prüfling(e). NICHT umgekehrt.

**FALSCH (Legacy, zu entfernen):** prt-art enthält ein **nested** `external/comdare-cache-engine`
(eigene cache-engine-Kopie). Das ist die redundante Alt-Struktur (Task #22 / E6). Ein
Prüfling besitzt KEINE eigene cache-engine — er bekommt die cache-engine-Header **vom
Controller bereitgestellt**.

---

## §3 Eigenschaften des Plugin-Controllers

1. **Konfigurations-gesteuert:** Die Diplomarbeit (Anforderungs-Konfig) gibt der cache-engine
   per CMake (`COMDARE_CE_PRUEFLINGE = prt-art[;weitere...]`) vor, welche Prüflinge geladen
   werden. Default: keiner (reine CE-Permutation, Stufe 1).
2. **Parallel-Loading:** Der Controller checkt/referenziert jedes Prüfling-Repo parallel zur
   cache-engine-Wurzel (z.B. `${CE_ROOT}/../comdare-prt-art` oder konfigurierter Pfad) und
   bindet es per `add_subdirectory`/FetchContent/Plugin-Mechanik als Gesamtsystem ein.
3. **CE-Konformität des Prüflings:** Jeder Prüfling stellt pro cache-engine-Achse eine Struktur
   bereit — und sei es nur ein Dummy — als `comdare::cache_engine::<axis>::optional_prt_art_impl`
   (siehe F.2). Der Prüfling KONSUMIERT cache-engine-BASIS (Richtung prt-art → cache-engine
   für Includes), liefert aber nur Achsen-Spezialisierungen.
4. **Multi-Prüfling:** Mehrere Prüflinge gleichzeitig ladbar (Liste). Jeder wird über die
   Abstract-Factory (`IPrueflingFactory`, E11) registriert.
5. **Merge + Test:** Die PermutationEngine merged die Prüfling-Achsen pro Achse gegen die
   CE-Achsen in den 3 Stufen (siehe §4) und baut self-contained Permutations-Binaries, die
   die Diplomarbeit als Messreihen A/B/C ausführt + auswertet (F15).

---

## §4 Verhältnis zur Prüfungs-Dreigliedrigkeit (F.5)

Der Controller erzeugt pro geladenem Prüfling die 3 kompositionalen Joins
([[3-kompositionale-joins-anatomie]] / [[3-stufen-pruefung]]):

| Stufe | Target | Inhalt |
|-------|--------|--------|
| 1 | `comdare_perms_ce` | nur cache-engine-Achsen (kein Prüfling) |
| 2 | `comdare_perms_pa_<pruefling>` | Prüfling ERSETZT pro Achse, Compile-Time-Fallback auf CE-Default |
| 3 | `comdare_perms_full_join` | non-redundantes kartesisches Produkt CE-Achsen × Prüfling-Achsen (mp_unique) |

Gattungs-Constraint: Merge nur innerhalb derselben Gattung (SearchAlgorithm);
Cross-Genus-Joins sind typsystem-mathematisch ausgeschlossen ([[gattungs-constraint-pruefling-merge]]).

> ⚠️ **(korr. 2026-06-03, s. Doc 30 §8.0):** Begriffs-Präzisierung der obigen Zeile nach dem
> verbindlichen 3-Ebenen-Modell. **Gattung = Außen-Interface** (SearchAlgorithm / Container /
> Graph; je Gattung ein Prüf-Dock, Doc 24 §8.8). Der hier beschriebene Achsen-Merge findet aber
> auf der Ebene des **FESTEN Achsen-Satzes** statt — und das ist die **Tier-Unterklassen**-Ebene,
> nicht die Gattungs-Ebene. Konkret: Der Merge erfolgt innerhalb **derselben SearchAlgorithm-Tier-
> Unterklasse** (die 17/19-Achsen-Komposition unter dem SearchAlgorithm-Interface; aktuell ist
> genau diese eine Tier-Unterklasse gebaut). Korrekt bleibt: Merge nur bei identischer Slot-Signatur
> (gleicher Achsen-Satz); Cross-Genus-Joins (z.B. SearchAlgorithm × Container) sind weiterhin
> typsystem-mathematisch ausgeschlossen, da unterschiedliche Außen-Interfaces unterschiedliche
> Prüf-Docks + Slot-Sätze haben. Lies „derselben Gattung (SearchAlgorithm)" oben daher als
> „derselben Tier-Unterklasse unter dem SearchAlgorithm-Interface".

---

## §5 Konsequenzen / Arbeitspakete

- **E11 (#16)** = die Plugin-Controller-Implementierung: `ICacheEngine`-Facade + `IPrueflingFactory`
  + CMake `COMDARE_CE_PRUEFLINGE` + Parallel-Load-Mechanik.
- **F.6 (#11)** verstärkt: BASIS-Achsen → cache-engine (Phase A läuft: status_code, serialization-
  primitives); prt-art wird dünnes konformes Plugin (nur `optional_prt_art_impl` je Achse).
- **F.2 (#12):** Namespace `comdare::cache_engine::<axis>::optional_prt_art_impl` ist der Slot,
  den der Controller pro Prüfling+Achse einbindet.
- **E6 (#22):** nested `comdare-prt-art/external/comdare-cache-engine` ENTFERNEN — Prüflinge
  haben keine eigene cache-engine.
- **F.5 (#15):** die 3 Stufen-Targets erzeugt der Controller pro geladenem Prüfling.

---

## §6 Cross-Refs
- Doku 19 (F.6 Migrations-Plan), Doku 18 (Achsen-Algorithmus-Paper-Map)
- Master-Architektur REV7.7 (Diplomarbeit/docs/architektur/02_*), Schichten-Modell M (10_*)
- Memory: [[3-kompositionale-joins-anatomie]], [[pruefling-replace-not-extend]], [[gattungs-constraint-pruefling-merge]]
