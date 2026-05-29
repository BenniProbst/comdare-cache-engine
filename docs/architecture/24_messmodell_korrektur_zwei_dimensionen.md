# Doku 24 — Mess-Modell-Korrektur: zwei Dimensionen (Tier-Wall-Clock vs. Achsen-Observer)

**Stand:** 2026-05-29 · **Auslöser:** User-Kritik an Säule 1 (ganze Algorithmen als Achse) + Säule 2
(nur 3 Achsen per Wall-Clock statt Per-Achsen-Observer). **Bezug:** Doku 14 (Organ-Metapher §1–§3,
§17, §20, §24), Doku 11 (Achsen vs. Strategien), Doku 22 (F15-Pipeline — wird hierdurch korrigiert).

> **Pflicht-Notiz:** Diese Doku hält eine vom User festgestellte ARCHITEKTUR-ABWEICHUNG meiner
> vorherigen autonomen Arbeit fest + den eindeutigen Korrektur-Pfad. Nicht löschen.

---

## §1 Die festgestellte Abweichung (ehrlich)

In der vorherigen Session wurden zwei dokumentierte Architektur-Prinzipien verletzt:

1. **Säule 1 — Achse-statt-Organ:** `axis_03a_search_algo` wurde um weitere *self-contained*
   Such-Strukturen erweitert (BST, B-Baum, SkipList, Hash, …), jede mit eigenem `std::vector<Node>`-
   Speicher + eigener Allokation + eigenem Layout. Das sind monolithische **„Tiere"**, keine
   komponierbaren **„Organe"** — sie konsumieren axis_04 (node_type), axis_05 (layout), axis_06
   (allocator) NICHT. Doku 14 §2.3/§7 markiert genau das als **REFACTORING-PFLICHT** („Tiere statt
   Organe"). Ein ganzer Algorithmus MUSS eine **Composition** über alle 17 Organe sein (Doku 14 §3).

2. **Säule 2 — Wall-Clock-3-Achsen statt Per-Achsen-Observer:** Der F15-Treiber maß Wall-Clock-Latenz
   von Kompositionen über nur **3 künstlich variierte Achsen** (search × allocator × layout, via
   angebolzte run_workload-Segmente) und **umging den `ObserverAggregate`** (`observe_all()`), den
   Doku 14 §17.2/§20 als zentrale Per-Achsen-Mess-Schnittstelle vorsieht. (Die Notwendigkeit der
   angebolzten Segmente war ein SYMPTOM von Abweichung 1: weil die Such-Wrapper nicht komponieren,
   hatte das Variieren anderer Achsen sonst keinen Effekt.)

---

## §2 Das KORREKTE Mess-Modell (User-Direktive 2026-05-29, verbatim-tragend)

Die Messung hat **zwei getrennte Dimensionen** plus eine separate Vergleichs-Dimension:

### §2.1 Tier-Ebene — der ganze Suchalgorithmus (CacheEngineBuilder, Wall-Clock + Ressourcen)

> „das 'Tier' also der Suchalgorithmus über die AKKUMULATION der Details der Latenz (Kurve über
> Element-Füllstand, read/write/delete), Speicherplatz/RAM, Speicherplatz disk und so weiter
> ausgemessen wird … die CacheEngineBuilder … wird weiterhin die Wallclock und die bisher
> festgelegten Metriken für den gesamten Algorithmus durchmessen."

- **Wer:** CacheEngineBuilder (kompiliert einen Suchalgorithmus aus Achsen + misst ihn als Ganzes).
- **Was:** Wall-Clock als **Akkumulation von Detail-Kurven** — Latenz **über den Element-Füllstand**,
  getrennt nach **read / write / delete**; **RAM**-Verbrauch; **Disk**-Platz; weitere festgelegte
  Gesamt-Metriken.
- **Status:** Wall-Clock bleibt hier (war als Tier-Metrik NICHT falsch — nur zu eng: nur 3 Achsen,
  Einzel-Batch statt Füllstands-Kurve, read/write/delete nicht getrennt). → anzureichern.

### §2.2 Achsen-Ebene — Per-Achsen-Statistics-Observer (Anatomie, observe_all)

> „jede Achse bezüglich ihrer Statistics observer, die dann über alle Achsen zu einem statistics
> tracing für den Gesamten Suchalgorithmus geloggt werden. … auf der Suchalgorithmus Ebene auf
> observe_all umstellen."

- **Wer:** `SearchAlgorithmAnatomy<Composition>::observe_all()` → `ObserverAggregate` (17 Snapshots).
- **Was:** Jede Achse exponiert ihren `statistics()`-Observer (z. B. search_algo: insert/lookup/hit/
  miss/peak_occupancy; allocator: allocation_count/bytes; …). Diese werden über ALLE 17 Achsen zu
  einem **Statistics-Tracing des Gesamt-Algorithmus** akkumuliert/geloggt.
- **Status:** `observe_all()` ist DERZEIT ein **Stub** (liefert leeres Aggregat, ruft `statistics()`
  je Achse NICHT auf — blockiert durch protected CRTP-Ctor der Wrapper). → echt zu verdrahten.

### §2.3 Achsen-VERGLEICH — Tests gegen vereinheitlichte Interfaces vs. bekannte Algorithmen

> „Der Vergleich der Algorithmen je Achse wird hingegen auf dieser Dimension durch Tests bestimmt,
> die gegen die vereinheitlichten Interfaces der Achse gegen bekannte Algorithmen laufen."

- **Wer:** Unit-/Property-Tests pro Achse.
- **Was:** Jede Achsen-Variante läuft gegen das **vereinheitlichte Achsen-Interface** und wird gegen
  **bekannte Algorithmen** (Referenz, z. B. `std::map`) geprüft. Welche Variante „je Achse besser" ist,
  bestimmt sich auf DIESER Dimension — **NICHT** über die Latenz-Benchmark.
- **Status:** Teilweise vorhanden — die Austauschbarkeits-Tests (`verify_matches_std_map<…>`) sind
  genau „Test gegen vereinheitlichtes Interface vs. bekannter Algorithmus (std::map)". → auszubauen.

### §2.4 Merksatz

| Dimension | Misst | Mechanismus | Ort |
|-----------|-------|-------------|-----|
| **Tier** (ganzer Algorithmus) | Latenz-Kurven (Füllstand, r/w/d) + RAM + Disk | Wall-Clock + Ressourcen | CacheEngineBuilder |
| **Achsen-Statistik** | je Achse `statistics()` → Gesamt-Trace | `observe_all()` → ObserverAggregate | Anatomie |
| **Achsen-Vergleich** | welche Variante je Achse besser | Tests vs. vereinheitlichtes Interface vs. bekannte Algos | Unit-Tests |

---

## §3 Korrektur-Plan (Reihenfolge per User-Direktive)

### Säule 2 ZUERST — Messung auf ObserverAggregate umstellen

1. `observe_all()` echt verdrahten: je Achse `statistics()` aufrufen (statt EmptyAxisSnapshot-Stub),
   sodass der Per-Achsen-Statistics-Trace real entsteht (mind. die getriebenen Achsen; Rest folgt mit
   Säule 1). Blocker (protected CRTP-Ctor) über die Builder-`AnatomyExecutionContext` lösen, die die
   Achsen real hält + treibt.
2. Mess-Pfad (`run_workload`/CLI) so erweitern, dass er pro Permutation ZUSÄTZLICH zur Tier-Wall-Clock
   das `observe_all()`-Aggregat erhebt + als Statistics-Trace ausgibt/loggt.
3. Tier-Wall-Clock (CacheEngineBuilder) bleibt + wird angereichert (Füllstands-Kurven, read/write/delete
   getrennt, RAM/Disk) — als eigene Dimension, NICHT als Achsen-Vergleich.

### Säule 1 DANACH — axis_03a entschärfen (Doku 14 §6)

4. Self-contained Such-Strukturen (Array256/BST/B-Baum/… inkl. der Session-Ergänzungen) als
   **Reference-Compositions / Stufe-2-Prüfling-Referenzen** umstufen (Doku 14 §5/§6).
5. `axis_03a` auf echte **Traversal-Organ-Varianten** reduzieren, die node_type/layout/allocator
   TATSÄCHLICH konsumieren → ein Algorithmus wird wieder eine Komposition über alle 17 Organe.
6. Achsen-Vergleichs-Tests (§2.3) gegen die vereinheitlichten Interfaces ausbauen.

---

## §4 Was korrekt bleibt (kein Rückbau)

- Die Such-Wrapper sind nicht „kaputt": sie erfüllen `std::map`-Semantik korrekt (Austauschbarkeit
  bewiesen) → als Reference-Compositions/Stufe-2-Referenzen weiter nutzbar (Doku 14 §14.2).
- Organ-Composition-Fundament existiert: `AdHocComposition<17>`, `SearchAlgorithmAnatomy`,
  `AnatomyPermutationDriver`, Reference-Compositions, `ObserverAggregate`/`observe_all`-Gerüst.
- Die robuste Statistik-Schicht (Median/MWU/Cliff's δ) + Coverage-Sampling bleiben gültige Werkzeuge
  der **Tier-Dimension** (Wall-Clock-Auswertung), sind aber NICHT der Achsen-Vergleich (§2.3).

---

**Ende Doku 24 — Mess-Modell-Korrektur (2026-05-29).**
