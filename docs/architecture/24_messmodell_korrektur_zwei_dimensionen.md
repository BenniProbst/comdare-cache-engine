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

## §5 Verifikation: Trennung CacheEngineBuilder ↔ Suchalgorithmus-Composition (Ist-Zustand 2026-05-29)

**Frage (User):** Besteht die Trennung zwischen CacheEngineBuilder und den erzeugten
Suchalgorithmus-Compositions noch?

**Antwort:** **Formal JA — aber der Datenfluss ist über DREI Pfade FRAGMENTIERT** (Befund-Verifikation
im Code, nicht behauptet).

### §5.1 Formal korrekt (Doku 14 §17.2/§24 R5.B umgesetzt)

| Einheit | Inhalt | Trennungs-Konformität |
|---------|--------|------------------------|
| `Composition` (AdHocComposition<17>) | reines 17-Achsen-**Typ**-Tupel (using-Aliases) | ✅ nur Typ-Bündel |
| `SearchAlgorithmAnatomy<C>` | **Organe + observe_all()** — KEIN insert/lookup/erase/Container | ✅ Container-API entfernt (R5.B) |
| `AnatomyExecutionContext<C>` (Builder) | hält `anatomy_` + `container_` + insert/lookup/erase/clear/size-**Commands** | ✅ Container+Commands im Builder |

→ Die R5.B-Korrektur (Container raus aus der Anatomie, rein in den Builder) IST vollzogen; die
Anatomie ist sauber „nur Organe + Observer".

### §5.2 ABER: drei DISKONNEKTIERTE Container/Antriebs-Pfade (Fragmentierung)

| Pfad | Wo lebt der „Container"? | Wer treibt ihn? | Observer-Anschluss |
|------|--------------------------|------------------|--------------------|
| **(1) Anatomie-Organ** | `anatomy_.axis_search_algo_` (Säule-2-Fix) | **niemand** (Produktionspfad) | observe_all() liest IHN |
| **(2) Builder-Context** | `container_` = `std::map<uint64,uint64>` (R5.B-Pilot-Platzhalter) | insert/lookup/erase | observe_all() liest NICHT ihn → **liefert 0** |
| **(3) ABI-Mess-Adapter** | lokales `SearchAlgo algo;` in `run_workload` | run_workload (Wall-Clock) | gar keiner (lokal, verworfen) |

**Konkrete Gaps (verifiziert):**
- **`AnatomyExecutionContext::observe_all()` liefert NULL-Statistik:** der Context treibt `container_`
  (std::map), aber observe_all() delegiert an `anatomy_.observe_all()` — und das Anatomie-Organ ist
  ungetrieben. Container und Observer sind ENTKOPPELT.
- **`abi_adapter::run_workload` umgeht beides:** misst Wall-Clock eines lokalen Organs, weder
  Builder-Container noch Anatomie-Organ/Observer.

### §5.3 Ziel-Zustand (Vereinheitlichung — Säule-2-Fortsetzung)

Der Builder (AnatomyExecutionContext) MUSS die **Anatomie-Organe** treiben (statt eines losgelösten
std::map), sodass:
- die „Container"-Aufgabe in den Organen lebt (von der Anatomie gehalten, Doku 14 §17.2),
- der Builder sie über Commands treibt,
- `observe_all()` die ECHTEN getriebenen Organ-Statistiken liefert (Säule-2-Trace),
- die Mess-Last (abi_adapter) über Builder/Anatomie statt eines lokalen Organs läuft.

**Blocker für die Vereinheitlichung:** Key-Type-Mismatch — `SearchAlgorithmAnatomy::key_type = uint64`,
aber die Such-Organe (Array256 = uint8, BST/B-Baum = uint16) haben eigene, schmalere Key-Typen. Der
std::map<uint64,uint64> im Builder umgeht das; das Treiben des Organs erfordert eine bewusste
Reconciliation (Anatomie-Key-Typ aus der Composition ableiten ODER Organ über Anatomie-Key
parametrisieren). Dieser Mismatch ist selbst ein Symptom der Säule-1-Modellierung (Organe mit eigenen
Key-Typen statt komponierbarer Traversal-Organe über einen gemeinsamen Key).

### §5.4 Konsequenz für die Reihenfolge

Die Säule-2-Vereinheitlichung (Builder treibt Organ → observe_all real im Mess-Pfad) ist mit der
Key-Type-Reconciliation **verschränkt mit Säule 1** (Organ-Key-Typen). Saubere Reihenfolge:
1. observe_all-Mechanismus real (✅ erledigt, §2.2).
2. Builder-Context auf Anatomie-Organ-Antrieb umstellen + Key-Type-Reconciliation (Säule-2 ↔ Säule-1-Brücke).
3. abi_adapter-Mess-Last über den Builder/Anatomie statt lokalem Organ.
4. axis_03a entschärfen (Säule 1).

---

### §5.5 Befund: Die Vereinheitlichung ist durch die Organ-Key-Typen BLOCKIERT (Säule-1-Verschränkung)

Beim Versuch, Schritt §5.4.2 (Builder treibt das Anatomie-Organ AS Container) umzusetzen, zeigt sich
ein harter Blocker (im Code verifiziert):

- Die Such-Organe haben **schmale, organ-eigene Key-Typen**: `Array256` = **uint8** (Key 0–255),
  `BST`/`BTree` = **uint16** (0–65535). Treibt man z. B. `ArtComposition`s Organ (Array256) als
  Container, wird Key `999` still zu `999 % 256 = 231` → **verlustbehaftet** (zwei verschiedene Keys
  kollidieren, size/lookup werden falsch). Die `builder_anatomy_commands`-Tests passieren nur
  ZUFÄLLIG (ihre Keys 1/2/5/7/999 kollidieren in uint8 nicht), aber als allgemeiner Container ist das
  Organ wegen des schmalen Key-Raums ungeeignet.
- Genau deshalb hält der Builder-Context bisher einen `std::map<uint64,uint64>` (breiter, exakter
  Key-Raum) — als Platzhalter, der den Mismatch umgeht.

**Konsequenz für die Reihenfolge:** Die volle Säule-2-VEREINHEITLICHUNG (Builder treibt Organ →
observe_all real im Mess-Pfad) ist NICHT vor der Säule-1-Key-Type-Reconciliation sauber machbar.
Optionen:
- **(A) Säule-1-Reconciliation zuerst:** Organe über einen gemeinsamen, breiten Key (z. B. uint64)
  parametrisieren bzw. Composition-Key ableiten — dann kann der Builder das Organ verlustfrei als
  Container treiben. Sauber, aber berührt alle Such-Wrapper (Key-Typ templatisieren) → GROSS.
- **(B) Transitions-Mirror:** Builder treibt zusätzlich zum std::map das Organ (für den Observer),
  Keys gecastet — schließt die observe_all-Lücke, ist aber redundant + bei breiten Keys lossy für den
  Observer-Teil. Nur als dokumentierte Übergangslösung vertretbar.

**Status:** observe_all-MECHANISMUS ist real (§2.2 erledigt, verifiziert). Die produktive Verdrahtung
in den Mess-Pfad ist durch (A)/(B) eine bewusste Entscheidung — vorgelegt zur Steuerung, KEINE fragile
Hau-Ruck-Änderung erzwungen.

---

## §6 Säule-1 Re-Modellierung — Increment 1: komponierbares Traversal-Organ-Modell (erledigt)

User-Entscheidung (2026-05-29): „Säule 1 jetzt: axis_03a re-modellieren" (die saubere Lösung statt
Transitions-Mirror). Erster build-grüner Increment (parallele neue Struktur, bestehende Wrapper
unangetastet):

`libs/cache_engine/topics/traversal/axis_03a_search_algo/composable/composable_search.hpp` etabliert
das Modell aus Doku 14 §11.3:
- **Storage-Organ** `RawSlotStore` (Pilot) — indizierte Slots ueber GEMEINSAMEM **uint64**-Key
  (vertritt das node_type/layout/allocator-getriebene Substrat).
- **Traversal-Organ-Concept** `TraversalOrgan<T,Store>` — statische `insert_into/lookup_in/erase_from`
  auf einem Storage-Organ, KEIN eigener Speicher (Organ, nicht Tier).
- **2 Pilot-Traversal-Organe**: `LinearScanTraversal` (ART-Node4) + `SortedBinaryTraversal` (Binärsuche).
- **`ComposedSearch<Traversal, Store>`** — Such-Algorithmus = Traversal ⊕ Storage, std::map-Interface.

**Verifiziert** (`Saeule1_ComposableOrgan.BothTraversalOrgansMatchStdMapOverWideKeys`, traversal 297/297):
beide Traversal-Organe ueber demselben Store sind std::map-aequivalent (Organ-Swappability, Doku 14 §1.2)
UND funktionieren mit **weiten Keys >65535** → der Key-Type-Blocker (§5.5) ist strukturell geloest.

**Folge-Increments (Säule 1, Mehr-Session):**
1. node_type/layout/allocator als ECHTE Storage-Organe (statt trait-only) → `RawSlotStore` durch
   organ-getriebenen Speicher ersetzen.
2. Bestehende monolithische Tier-Wrapper (Array256/BST/B-Baum/Original*…) als **Reference-Compositions /
   Stufe-2-Prüfling-Referenzen** umstufen (Doku 14 §6) statt als axis_03a-Organe.
3. SearchAlgorithmAnatomy + AnatomyExecutionContext auf `ComposedSearch` (gemeinsamer uint64-Key)
   umstellen → schliesst die §5.2-Lücke (observe_all real im Mess-Pfad) + vereinheitlicht die 3 Pfade.

---

**Ende Doku 24 — Mess-Modell-Korrektur (2026-05-29).**
