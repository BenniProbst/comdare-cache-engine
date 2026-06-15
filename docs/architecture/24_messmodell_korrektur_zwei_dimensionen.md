# Doku 24 — Mess-Modell-Korrektur: zwei Dimensionen (Lebewesen-Wall-Clock vs. Achsen-Observer)

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
   Speicher + eigener Allokation + eigenem Layout. Das sind monolithische **„Lebewesen"**, keine
   komponierbaren **„Organe"** — sie konsumieren axis_04 (node_type), axis_05 (layout), axis_06
   (allocator) NICHT. Doku 14 §2.3/§7 markiert genau das als **REFACTORING-PFLICHT** („Lebewesen statt
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

### §2.1 Lebewesen-Ebene — der ganze Suchalgorithmus (CacheEngineBuilder, Wall-Clock + Ressourcen)

> „das 'Tier' also der Suchalgorithmus über die AKKUMULATION der Details der Latenz (Kurve über
> Element-Füllstand, read/write/delete), Speicherplatz/RAM, Speicherplatz disk und so weiter
> ausgemessen wird … die CacheEngineBuilder … wird weiterhin die Wallclock und die bisher
> festgelegten Metriken für den gesamten Algorithmus durchmessen."

- **Wer:** CacheEngineBuilder (kompiliert einen Suchalgorithmus aus Achsen + misst ihn als Ganzes).
- **Was:** Wall-Clock als **Akkumulation von Detail-Kurven** — Latenz **über den Element-Füllstand**,
  getrennt nach **read / write / delete**; **RAM**-Verbrauch; **Disk**-Platz; weitere festgelegte
  Gesamt-Metriken.
- **Status:** Wall-Clock bleibt hier (war als Lebewesen-Metrik NICHT falsch — nur zu eng: nur 3 Achsen,
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
| **Lebewesen** (ganzer Algorithmus) | Latenz-Kurven (Füllstand, r/w/d) + RAM + Disk | Wall-Clock + Ressourcen | CacheEngineBuilder |
| **Achsen-Statistik** | je Achse `statistics()` → Gesamt-Trace | `observe_all()` → ObserverAggregate | Anatomie |
| **Achsen-Vergleich** | welche Variante je Achse besser | Tests vs. vereinheitlichtes Interface vs. bekannte Algos | Unit-Tests |

---

## §3 Korrektur-Plan (Reihenfolge per User-Direktive)

### Säule 2 ZUERST — Messung auf ObserverAggregate umstellen

1. `observe_all()` echt verdrahten: je Achse `statistics()` aufrufen (statt EmptyAxisSnapshot-Stub),
   sodass der Per-Achsen-Statistics-Trace real entsteht (mind. die getriebenen Achsen; Rest folgt mit
   Säule 1). Blocker (protected CRTP-Ctor) über die Builder-`AnatomyExecutionContext` lösen, die die
   Achsen real hält + treibt.
2. Mess-Pfad (`run_workload`/CLI) so erweitern, dass er pro Permutation ZUSÄTZLICH zur Lebewesen-Wall-Clock
   das `observe_all()`-Aggregat erhebt + als Statistics-Trace ausgibt/loggt.
3. Lebewesen-Wall-Clock (CacheEngineBuilder) bleibt + wird angereichert (Füllstands-Kurven, read/write/delete
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
  der **Lebewesen-Dimension** (Wall-Clock-Auswertung), sind aber NICHT der Achsen-Vergleich (§2.3).

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
  auf einem Storage-Organ, KEIN eigener Speicher (Organ, nicht Lebewesen).
- **2 Pilot-Traversal-Organe**: `LinearScanTraversal` (ART-Node4) + `SortedBinaryTraversal` (Binärsuche).
- **`ComposedSearch<Traversal, Store>`** — Such-Algorithmus = Traversal ⊕ Storage, std::map-Interface.

**Verifiziert** (`Saeule1_ComposableOrgan.BothTraversalOrgansMatchStdMapOverWideKeys`, traversal 297/297):
beide Traversal-Organe ueber demselben Store sind std::map-aequivalent (Organ-Swappability, Doku 14 §1.2)
UND funktionieren mit **weiten Keys >65535** → der Key-Type-Blocker (§5.5) ist strukturell geloest.

**Folge-Increments (Säule 1, Mehr-Session):**
1. node_type/layout/allocator als ECHTE Storage-Organe (statt trait-only) → `RawSlotStore` durch
   organ-getriebenen Speicher ersetzen.
2. Bestehende monolithische Lebewesen-Wrapper (Array256/BST/B-Baum/Original*…) als **Reference-Compositions /
   Stufe-2-Prüfling-Referenzen** umstufen (Doku 14 §6) statt als axis_03a-Organe.
3. SearchAlgorithmAnatomy + AnatomyExecutionContext auf `ComposedSearch` (gemeinsamer uint64-Key)
   umstellen → schliesst die §5.2-Lücke (observe_all real im Mess-Pfad) + vereinheitlicht die 3 Pfade.

---

## §7 Lebewesen→Organ-Rekonstruktions-Beleg + die ERKLÄRTE Ordnung (Korrektur 2026-05-29)

**Erklärte Ordnung (User-Direktive, autoritativ — korrigiert eine frühere Fehlrichtung):**
> Achsen enthalten **ausschließlich Organe**, **niemals** ganze Tiere. Es sind **NIE** monolithische
> Tiere als Achsen-Werte erlaubt — auch nicht „übergangsweise". Ein noch **nicht seziertes** Tier steht
> **außerhalb des Systems** (Doku 14 §3.1: „ein Gesamtalgorithmus steht außerhalb des Systems; erst zerlegt
> bringen wir seine Organe ein") — es wird **NICHT** als Achsen-Wert abgelegt. Erst nach Sezierung in
> Organe tritt es als **Gattungs-Konfigurator** (Composition über alle Organ-Achsen — ~~teils optional
> genutzten~~ **[korr. 2026-06-03, s. Doc 30 §8: Achsen einer Gattung sind NIE optional; die Interfaces
> ALLER Achsen werden in JEDEM Tier-Binary tatsächlich + uniform getrieben. Ein Tier, das ein Organ nicht
> aktiv „nutzt", wählt dort den konkreten durchreichenden Algorithmus (z. B. NoBuffer/NoFlush analog
> NonePrefetch/NoMigration) — „None" ist ein Algorithmus, keine Abwesenheit der Achse]**),
> den der CacheEngineBuilder metaprogrammiert zur exakten Wiederherstellung
> des Tieres aus seinen Organen.

> **[Begriffs-Korrektur 2026-06-03, s. Doc 30 §8.0 (gilt für „Gattungs-Konfigurator" im GANZEN Dokument —
> hier, §7-`Korrektur`, §7-`Umstufungs-Programm`, §8.8):** Der „Composition-über-alle-Organ-Achsen"-Konfigurator
> (`SearchAlgorithmAnatomy<Composition>`, der feste 17/19-Achsen-Satz) lebt auf der **TIER-UNTERKLASSEN-Ebene**,
> NICHT auf der Gattungs-Ebene. Im 3-Ebenen-Modell ist die **GATTUNG = das Außen-INTERFACE / Prüf-Dock**
> (Search / Container / Graph); die **TIER-UNTERKLASSE** liegt UNTER dem Gattungs-Interface und hält den festen
> Achsen-Satz; die **ACHSEN** sind ihre Organe. „Gattungs-Konfigurator" meint also präzise einen
> **Tier-Unterklassen-Konfigurator** unter dem SearchAlgorithm-Interface. Der etablierte Term bleibt als
> Code-/Doku-Name stehen, ist aber so zu LESEN. Die Gattungs-Invariante (fester Achsen-/Slot-Satz pro
> Tier-Unterklasse, `AdHocComposition<N>` als ABI-Identität) bleibt unverändert gültig — sie ist eine
> Tier-Unterklassen-Invariante. Ebenso: queuing-Achsen (q1/q2) wären — wo sie vorkommen — **Pflicht-Achsen
> dieser Tier-Unterklasse** (kein Interface, keine Gattung); KEINE Achse ist optional (s. korr.-Notiz oben).]**

> **Konsequenz für den Ist-Stand:** Die heutigen monolithischen `axis_03a`-Wrapper (Array256/BST/Hash/…)
> sind genau das zu behebende Anti-Pattern (Tiere als Achsen-Werte). Sie werden ausnahmslos seziert + aus
> `EnabledStrategies` entfernt; bis ein Tier seziert ist, gehört es nicht als Achsen-Wert ins System.

**Korrektur:** Die zuvor erwogene Lesart „Lebewesen-Wrapper als Achsen-Werte BEHALTEN + nur dokumentieren"
(gestützt auf Doku 14 §14.2 „PROMOTION") ist damit **falsch**. §14.2 bedeutet „nicht ersatzlos löschen",
**nicht** „als Achse behalten". Korrekt: die sezierten Lebewesen-Wrapper werden aus `axis_03a::EnabledStrategies`
**entfernt** und als Gattungs-Konfigurator (`SearchAlgorithmAnatomy<Composition>` über Organ-Achsen,
Doku 14 §3.3/§11.3/§27-§29) rekonstruiert. axis_03a hält dann nur noch **Traversal-Organe**.

**Dieser Increment (Rekonstruktions-Beleg, Brücke):** `composable/tier_to_organ_mapping.hpp` ordnet jedem
bereits sezierten Lebewesen-Wrapper seine äquivalente Organ-Komposition zu; `test_v41_axis_03a_tier_organ_equivalence.cpp`
**belegt** Lebewesen ≡ Organ-Komposition ≡ std::map (key-type-sicher). Damit ist bewiesen, dass die Lebewesen exakt aus
Organen wiederherstellbar sind — die Voraussetzung für die Umstufung. **Bewusst KEINE „mp_size==17 muss bleiben"-
Invariante** (würde die Umstufung blockieren).

**Grundprinzip (User 2026-05-29):** Ein Lebewesen darf **nur seziert** vorliegen — d. h. ausschließlich als
Organ-Komposition (Gattungs-Konfigurator). Es gibt **keine** monolithische Form im System, weder dauerhaft
noch übergangsweise. Folglich müssen **ALLE** Lebewesen seziert werden (auch OriginalXxx-Paper-Baselines);
keines bleibt monolithisch.

**Umstufungs-Programm (Reihenfolge per User 2026-05-29 — „erst restliche Lebewesen sezieren, dann ALLE umstufen"):**
1. **ALLE** noch nicht sezierten Lebewesen in Organe sezieren: Hash → Bucket-Pool+Probe-Organ, SkipList →
   SkipList-Pool+Walk-Organ, B-Baum → BTree-Pool+Walk-Organ (analog TreeNodePool/BST aus Roadmap-2 INC-2b);
   OriginalXxx (S04-S08) als paper-gebundene Organ-Kompositionen (Habich-Original-Code pro Organ).
2. DANN ALLE Lebewesen gemeinsam umstufen: aus `EnabledStrategies` entfernen → ausschließlich als Gattungs-
   Konfiguratoren (Compositions über Organe) rekonstruieren → Anatomie/Compositions/Tests umverdrahten
   (Doku 24 §6 Folge-Increment 1+3). End-Zustand: `axis_03a::EnabledStrategies` enthält **nur Traversal-Organe**,
   die Lebewesen existieren nur noch als Compositions.

**Bezug Memory:** `[[feedback_no_whole_tier_axes_genus_configurator]]` (kritische Ordnung + Pflicht-Pre-Read Doku 14).

---

## §8 Das HYBRID-Mess-Modell (User-Klarstellung 2026-05-30, vollständig dokumentiert)

**Bestätigung (User 2026-05-30):** Das Messmodell der Diplomarbeit IST akkurat dokumentiert — in dieser
Doku 24 (§2 die drei Dimensionen), Doku 22 (F15-Mess-Pipeline) und den Säulen-Session-Docs
(`20260529-roadmap3-saeule2-messpfad`, `…-roadmap4-saeule3-achsen-vergleich`,
`…-saeule1-inc2-composed-store`, `…-saeule2-observable-anatomy-context`). Diese Doku bleibt autoritativ.

### §8.1 Die zentrale Erkenntnis: die Messung ist ein HYBRID (zwei Pfade, je nach Mess-Konfiguration)

> **User 2026-05-30 (verbatim-tragend):**
> „Die CacheEngineBuilder und Submodule sollten den Tier-Modul-Binary-Build übernehmen und durchmessen."
> „Die Messung erfolgt durch ein ABI-stabiles Interface auf der Seite der CacheEngineBuilder durch Zugriff
> auf die **Observer** eines **composite Tiers**, welches aus Achsen permutiert **als Modul-Binary** aufgebaut wird."
> „Die Messung läuft bei Konfiguration der Messung von **isolierten Achsen-Algorithmen gegeneinander** auf
> der **DLL selbst**, aber im **Composite zentral über die CacheEngineBuilder**. Es ist ein **Hybrid-Modell**."

Gemeinsame Grundlage BEIDER Pfade: Das **composite Lebewesen** (ein ganzer Suchalgorithmus = Komposition über
die 17 Achsen) wird vom **CacheEngineBuilder + Submodulen** als **Modul-Binary** (.so/.dll) gebaut
(`apps/adhoc_emitter` enumeriert den Permutationsraum → CMake `comdare_build_adhoc_modules` baut JEDE
Permutation als SHARED-DLL → `AnatomyModuleLoader` lädt sie host-seitig). Die **Mess-KONFIGURATION**
entscheidet dann, WELCHER der beiden Mess-Pfade läuft:

| | **Pfad A — Isolierte Achsen-Algorithmen** | **Pfad B — Composite Lebewesen** |
|---|---|---|
| **Mess-Konfiguration** | Achsen-Algorithmus-Varianten **gegeneinander** isoliert vergleichen (z. B. welche `search_algo`-Variante / welcher Allocator je Achse schneller) | Den GANZEN Suchalgorithmus (alle 17 Achsen komponiert) als Lebewesen durchmessen |
| **WO läuft die Messung** | **auf der DLL selbst** (in-Modul) | **zentral host-seitig über die CacheEngineBuilder** |
| **Mechanismus** | `IMeasurableWorkload::run_workload` — die DLL fährt ihren eigenen Mess-Workload und liefert Batch-Latenzen | ABI-stabiler **Observer-Zugriff**: der Host treibt das Lebewesen-Modul + liest dessen **Observer** (`observe_all` → `ObserverAggregate`) über die ABI-Grenze |
| **Was wird gemessen** | Wall-Clock-Vergleich der isolierten Achsen-Variante (Doku 22 §3.1–§3.3: 18,5×/85× Spannen, Holm/MWU/Cliff's δ) | Lebewesen-Wall-Clock (Füllstand-Kurven, r/w/d getrennt, RAM/Disk, §2.1) **+** Per-Achsen-Statistik-Trace (`observe_all`, §2.2) des composite Lebewesens |
| **Dimension (§2)** | §2.3 Achsen-Vergleich (+ Lebewesen-Wall-Clock je isolierter Achse) | §2.1 Lebewesen + §2.2 Achsen-Observer, zentral |

**KORREKTUR einer früheren Fehlformulierung (dieser Doku, 2026-05-30):** `IMeasurableWorkload::run_workload`
ist **NICHT verworfen**. Es ist der **Pfad A** (isolierte Achsen-Messung auf der DLL selbst) des Hybrids —
korrekt + produktiv (Doku 22 §3 belegt damit die Achsen-Vergleichs-Resultate). Verworfen ist nur die
Vorstellung, run_workload sei der EINZIGE/zentrale Mess-Mechanismus: für das **Composite** misst der Host
zentral über die **Observer** (Pfad B).

### §8.2 Warum hybrid (Begründung)

- **Isolierte Achse → DLL-selbst:** Will man Achsen-Algorithmen GEGENEINANDER vergleichen, ist die je
  Permutation kompilierte DLL die natürliche Mess-Einheit — die Variante lebt + läuft IN ihrer DLL, und der
  Vergleich ist „eine DLL vs. die andere" (host-seitige Aggregation der DLL-gelieferten Samples + Statistik).
  Der Mess-Code läuft IN der DLL, weil nur dort die konkrete Achsen-Variante einkompiliert ist.
- **Composite → zentral CacheEngineBuilder:** Das GANZE Lebewesen durchzumessen heißt, seine 17 Achsen-Observer
  als EINEN Statistics-Trace zu erheben (`observe_all`). Das gehört zentral in den Builder (er hält/treibt
  das Lebewesen + liest dessen Observer über das ABI-stabile Interface), damit ALLE Achsen eines Lebewesens in EINEM
  Lauf konsistent getract werden — unabhängig davon, welche Achse gerade „interessiert".

### §8.3 Abbildung auf die existierende Implementierung

| Baustein | Rolle im Hybrid | Datei | Status |
|----------|-----------------|-------|--------|
| `adhoc_emitter` + `comdare_build_adhoc_modules` | baut composite Lebewesen als Modul-Binary (Grundlage beider Pfade) | `apps/adhoc_emitter`, CMake | ✅ |
| `AnatomyModuleLoader` | lädt die Lebewesen-Modul-Binaries host-seitig | `abi/module_loader.hpp` | ✅ |
| `IMeasurableWorkload::run_workload` | **Pfad A** — isolierte Achsen-Messung IN der DLL | `anatomy/measurable_workload.hpp`, `anatomy/abi_adapter.hpp` | ✅ |
| `f15_compare` / Welch+MWU+Cliff's δ | **Pfad A** Auswertung (host-seitige Aggregation der DLL-Samples) | `apps/f15_compare`, `builder/commands/stats/*` | ✅ |
| `AnatomyExecutionContext::observe_all()` | **Pfad B** — treibt echtes Composition-Organ + liest Observer (search_algo+allocator real, uint64-Key) | `builder/anatomy_commands/anatomy_execution_context.hpp` | ✅ in-process |
| `drive_tier_observe_trace` | **Pfad B** — Füllstand-Treiber: Lebewesen-Wall-Clock (r/w/d) + observe_all + RAM | `builder/anatomy_commands/tier_observe_trace.hpp` | ✅ in-process |
| **`IObservableTier` (ABI)** | **Pfad B über die Modul-Binary-Grenze** — Host treibt geladenes Lebewesen + liest dessen Observer als POD | `abi/observable_tier.hpp` (R6) | **✅ erledigt** (R6.1–R6.4 done-verified; reale DLL via `R8RestA_DockMeasuresRealDll` 2/2 grün) |

### §8.4 §5.5-Blocker AUFGELÖST

Der in §5.5 als „BLOCKIERT (Säule-1-Verschränkung, Organ-Key-Typen)" markierte Zustand ist durch
**Umstufung-A/B (erledigt 2026-05-30)** behoben: ALLE Such-Organe laufen über den gemeinsamen **uint64-Key**
(die schmal-key Lebewesen Array256=uint8 etc. sind deregistriert). Damit ist **Option (A)** aus §5.5 de facto
umgesetzt — der Builder treibt das echte Composition-Organ verlustfrei (Pfad B). Die zuvor
„USER-zu-bestätigende Entscheidung" ist mit „nach Plan umsetzen" (User 2026-05-30) **freigegeben**.

### §8.5 Ist-Stand der drei Dimensionen × der zwei Pfade

| Dimension | Pfad A (DLL-selbst, isolierte Achse) | Pfad B in-process (Composite) | Pfad B über Modul-Binary-ABI |
|-----------|--------------------------------------|-------------------------------|------------------------------|
| §2.1 Lebewesen-Wall-Clock | ✅ run_workload + f15_compare | ✅ tier_observe_trace (Füllstand, r/w/d, RAM) | ✅ **erledigt** (R6, reale DLL `R8RestA_DockMeasuresRealDll` 2/2) |
| §2.2 Achsen-`observe_all` | (n/a — Pfad A misst isoliert) | ✅ AnatomyExecutionContext::observe_all (search_algo+allocator real) | ✅ **erledigt** (R6, `IObservableTier` über ABI-Grenze) |
| §2.3 Achsen-Vergleich | ✅ Welch+MWU+Cliff's δ (Doku 22 §3) | ✅ verify_matches_std_map (Compile-Time-Korrektheit) | n/a |

### §8.6 R6 — der präzise verbleibende Schritt (Pfad B über die Modul-Binary-Grenze)

> **✅ R6 ERLEDIGT (Stand 2026-05-31, Audit `wdfezww0t` bestätigt):** der hier beschriebene Schritt ist
> umgesetzt — `IObservableTier` (`abi/observable_tier.hpp`) trägt über die echte `.dll`-Grenze; Host lädt
> reale Permutations-DLL via `AnatomyModuleLoader` → `dynamic_cast<IObservableTier*>` → `SearchAlgorithmDock.measure()`
> → CSV/JSON. Test-belegt: `R8RestA_DockMeasuresRealDll` 2/2 grün. Die folgende Beschreibung ist der
> Umsetzungs-Plan (historisch); Ledger §(e) R6.1–R6.4 = done-verified.

> **User 2026-05-30 (verbatim-tragend, „bitte nicht vergessen"):** „alle Tier-Binaries [sind] C++23
> dynamisch ladbare Module und daher separat von der CacheEngineBuilder, die CacheEngineBuilder baut die
> Tiere dynamisch und hat ein ABI-stabiles Interface für die API der **Gattung** eines Algorithmus (etwa
> Suchalgorithmen) und **testet die API durch**, dann **misst es die in dem Tier eingebauten Observer**
> durch und **zieht diese durch die Schnittstelle** zwischen CacheEngineBuilder und Tier-Binary hindurch,
> um als CacheEngineBuilder die **Messergebnisse der Observer zu persistieren**."

**Grundprinzip:** Lebewesen-Binaries sind **separate, dynamisch ladbare C++23-Module** (.so/.dll) — NICHT in den
Builder einkompiliert. Die CacheEngineBuilder baut sie dynamisch und kommuniziert mit ihnen **ausschließlich
über das ABI-stabile Interface der GATTUNG** (Gattungs-typisiert: für die SearchAlgorithm-Gattung die
insert/lookup/erase-API; vgl. `SearchAlgorithmAbiAdapter`/`IAnatomyBase::genus()`). Der vollständige
host-seitige Mess-Ablauf je Lebewesen-Modul:

1. **Bauen (dynamisch):** Builder/`adhoc_emitter`+CMake materialisiert die Permutation als ladbares Modul.
2. **Laden:** `AnatomyModuleLoader` (dlopen/LoadLibrary) → `comdare_create_anatomy()` → `IAnatomyBase*`;
   ABI-Version/Magic geprüft.
3. **Gattungs-API DURCHTESTEN:** Builder treibt die Gattungs-API (insert/lookup/erase) über das ABI gegen
   das Lebewesen-Modul und verifiziert sie (Korrektheit der API durch die Grenze — Gattungs-Vertrag).
4. **Eingebaute Observer MESSEN:** Builder triggert Observer-Updates (Zeitschritt/Zustands-Manipulation,
   §8.7) — die Observer leben IM Lebewesen-Modul.
5. **Durch die Schnittstelle ZIEHEN:** der Observer-Snapshot wird als **flacher POD** über die ABI-Grenze
   gezogen (`ComdareTierObserverSnapshotV1` bzw. der `ObserverAggregate` ist bereits `standard_layout` +
   `trivially_copyable` → memcpy-fähig; nur uint64-Felder, keine STL/vtable über die Grenze).
6. **PERSISTIEREN (durch den Builder):** der Builder schreibt die korrelierten Observer-Messergebnisse
   `(wall_clock_t ↔ ObserverAggregate)` heraus (binary records / CSV / JSON, vgl. `result_aggregator`).

**Umsetzung:** ABI-stabiles Sub-Interface `IObservableTier` (Muster `IMeasurableWorkload`: der genus-typisierte
ABI-Adapter erbt es ZUSÄTZLICH, Host-Abfrage via `dynamic_cast`, KEIN vtable-Bruch von `IAnatomyBase`):
`tier_insert/tier_lookup/tier_erase/tier_clear/tier_size` (Gattungs-Drive, uint64) +
`tier_observe(snapshot*)` (Observer-POD ziehen). Der `drive_tier_observe_trace`-Treiber (in-process) wird
über dieses Interface generalisiert (host treibt + stempelt Wall-Clock + zieht Observer-POD + persistiert).
Pfad A (run_workload) bleibt unverändert (kein Rückbau).

**✅ R6 Inkrement 1 (erledigt 2026-05-30):** `anatomy/observable_tier.hpp` — `IObservableTier`-Sub-Interface
+ flacher POD `ComdareTierObserverSnapshotV1` (nur uint64, `static_assert` standard_layout + trivially_copyable).
`SearchAlgorithmAbiAdapter` erbt `IObservableTier` zusätzlich + treibt das ECHTE Composition-Such-Organ
(uint64) + flacht `observe_all` (search_algo real) in den POD. Test `R6_HostSideObserverPullViaAbiInterface`
belegt host-seitig (via `dynamic_cast<IObservableTier*>`): Gattungs-API-Durchtesten + Observer-POD-Ziehen
(insert_count==2000, korrelierter Füllstand) + memcpy-Roundtrip.

**✅ R6 Inkrement 2a (erledigt 2026-05-30):** `builder/anatomy_commands/tier_observe_trace_abi.hpp` —
`drive_tier_observe_trace_abi(IObservableTier&, cfg)` generalisiert den in-process-Füllstands-Treiber auf
das ABI-Interface: der Builder treibt das Lebewesen-Modul über Füllstand-Checkpoints (Trigger-Modus
Zustands-Manipulation §8.7b), erhebt pro Checkpoint r/w/d-Wall-Clock-Roh-Samples (§2.1) UND einen
`tier_observe`-POD (§2.2), Wall-Clock-korreliert — OHNE den Composition-Typ zu kennen (nur das Gattungs-ABI).
Test `R6_AbiTierObserveTraceCorrelatesWallClockAndObservers` (3 Checkpoints 10/100/1000): pro Stufe
r/w/d-Samples + korrelierter Observer (tier_fill_level == Checkpoint, insert_count monoton wachsend).
f15 29/29, alle Adapter-Tests grün, Pfad A unberührt.

**Inkrement 2b — Befund 2026-05-30 (Teil-erledigt):** Drei Teilziele, je mit präzisem Aufwand:
- **✅ Wall-Clock-Stempel je Snapshot (erledigt):** `AbiFillLevelSnapshot::observe_wall_ns` — der ABI-Treiber
  stempelt jeden Observer-Snapshot mit einem Wall-Clock-Zeitstempel (relativ zum Trace-Start) im Moment des
  `tier_observe` → explizite (t ↔ Observer)-Korrelation (§8.7). Test: Stempel monoton wachsend über die Checkpoints.
- **✅ Persistierung als CSV (erledigt):** `serialize_abi_tier_trace_csv(AbiTierObserveTrace)` — §8.6 Schritt 6:
  der Builder serialisiert die korrelierten (Wall-Clock ↔ Observer)-Ergebnisse, eine Zeile je Checkpoint
  (observe_wall_ns + fill_level + r/w/d-Sample-Zahlen + alle Observer-POD-Felder). Test: Header + 3 Datenzeilen.
  Damit ist die Pfad-B-Schleife geschlossen: **bauen → laden → Gattungs-API durchtesten → Observer ziehen →
  Wall-Clock-korrelieren → persistieren**.
- **✅ JSON-Export + Perzentile (erledigt):** `serialize_abi_tier_trace_json` — JSON-Array (Objekt je Checkpoint)
  mit p50/p99 der r/w/d-Roh-ns-Kurven (Lebewesen-Wall-Clock-Detail-Auswertung §2.1, ausreisser-robust via p50
  vgl. Doku 22 §3.3) korreliert mit den Observer-Zählern + `observe_wall_ns`. `detail::nearest_rank_p`
  Nearest-Rank-Perzentil. Test: JSON-Form + p50/p99-Felder + 3 Checkpoint-Objekte. f15 29/29.
- **✅ Allocator-Achse in den Cross-ABI-POD (erledigt 2026-05-30):** Zwei Schritte:
  1. **Loader-Entkopplung (`6140705`):** `cache_engine/abi/anatomy_module_abi_v1.hpp` zog `abi_adapter.hpp`
     (schwere Template) → koppelte den leichten dlopen-Loader an die GANZE Achsen-Library + generierte-Flags-
     Maschinerie (C1083 in zwei Schichten: `measurement/` unter `src/` + generierte `axis_04_*_flags.hpp` unter
     `build/generated/`). **NEU `anatomy_module_abi_v1_decl.hpp`** = leichte ABI-Schnittstelle (Version/Magic,
     Factory-Decls, AnatomyAbiVersion, Interface-Includes) — der Loader inkludiert NUR das Decl; das volle
     Header (Decl + abi_adapter + COMDARE_DEFINE-Makros) bleibt für DLLs/Tests unverändert. KEIN Schmierfix,
     KEINE Emitter-Änderung.
  2. **ComposedStore im Adapter (`abi_adapter.hpp`):** der genus-Adapter hält jetzt den
     `ObservableComposedSearch<SortedBinaryTraversal, ComposedStore<N,L,A>>`-Container (spiegelt
     `AnatomyExecutionContext`); `tier_insert/erase/clear` treiben ihn, `tier_observe` flacht
     `store_allocator_statistics()` in die `alloc_*`-POD-Felder. Da der Loader entkoppelt ist, belasten die
     topics/-Includes nur die Voll-Header-Konsumenten (DLLs/Tests, die die Pfade ohnehin haben). Test
     `R6_HostSide…`: `alloc_bytes_allocated>0` + `alloc_allocation_count>0` → der Cross-ABI-POD ist jetzt
     **2-dimensional** (search_algo + allocator, beide aus den echten getriebenen Organen). f15 29/29,
     loader 11/11, execution_engine 11/11.
- **Echter .dll-Round-Trip:** Treiber über ein per `AnatomyModuleLoader` GELADENES Modul statt in-process-
  Adapter — erfordert Rebuild der `adhoc`-Permutations-DLLs mit dem `IObservableTier`-Adapter.

### §8.7 Pfad B im Detail: CacheEngineBuilder erhebt BEIDE Dimensionen, zeit-/zustands-KORRELIERT

> **User 2026-05-30 (verbatim-tragend):** „die CacheEngineBuilder [erhebt] SOWOHL die allgemeinen Metriken
> wie wall clock, als auch die Achsen-observer-statistics vollständig. Dabei kann jeder wall clock time mit
> sync nach Zeitschritten oder nach Manipulation des Suchalgorithmus-Zustandes ein update der Observer
> getriggert werden, die dann einer wall clock time zugeordnet werden können."

Im **Composite-Pfad (B)** sind Lebewesen-Wall-Clock (§2.1) und Achsen-Observer (§2.2) **NICHT zwei getrennte
Läufe**, sondern werden vom CacheEngineBuilder in EINEM Lauf **gemeinsam + korreliert** erhoben:

1. **Vollständigkeit:** Der Builder erhebt **beide** — die allgemeinen Lebewesen-Metriken (Wall-Clock, später
   RAM/Disk) UND die **vollständigen** Achsen-Observer-Statistics (`observe_all()` → `ObserverAggregate`
   über ALLE 17 Achsen, nicht nur die getriebenen — die nicht-getriebenen liefern ihre Default-Snapshots).
2. **Zwei Trigger-Modi für Observer-Updates** (Mess-Konfiguration wählt):
   - **(a) Zeitschritt-Sync (`sync nach Zeitschritten`):** periodisches Sampling — alle Δt wird ein
     `observe_all()`-Snapshot genommen + mit der aktuellen Wall-Clock-Zeit gestempelt. Liefert die
     **Observer-Trajektorie über die Zeit**.
   - **(b) Zustands-Manipulation (`nach Manipulation des Suchalgorithmus-Zustandes`):** ereignis-getrieben —
     nach einer zustandsändernden Operation (insert/erase; ggf. lookup) wird ein Snapshot genommen + gestempelt.
     Liefert die **Observer-Entwicklung über den Algorithmus-Zustand** (z. B. Füllstand — das ist exakt der
     Checkpoint-Modus von `tier_observe_trace.hpp`, dort an Füllstands-Stützpunkte gebunden).
3. **Korrelation:** JEDER Observer-Snapshot trägt einen **Wall-Clock-Zeitstempel** → die Messung ist eine
   **korrelierte Zeitreihe** `[(t₀, ObserverAggregate₀), (t₁, ObserverAggregate₁), …]`. Damit lässt sich
   jede Per-Achsen-Statistik-Änderung (z. B. `allocator.total_bytes_in_use`-Sprung, `search_algo.peak_occupancy`)
   einer Wall-Clock-Zeit (und damit einer Latenz-Phase) ZUORDNEN — die zwei Composite-Dimensionen sind
   über die Zeit/den Zustand **verschränkt auswertbar**, nicht nur nebeneinander.

**Konsequenz für die Datenstruktur (Pfad B):** Ein Lebewesen-Mess-Resultat ist eine Sequenz korrelierter
Samples `{ wall_clock_ns, op_type∈{read,write,delete}, fill_level, ObserverAggregate }` — `tier_observe_trace.hpp`
realisiert die in-process-Variante (Trigger-Modus (b), Füllstands-Checkpoints; pro Checkpoint r/w/d-Wall-Clock
+ ein observe_all). R6 generalisiert genau diese korrelierte Erhebung über die **Modul-Binary-ABI-Grenze**
(Host triggert Operation/Zeitschritt → stempelt Wall-Clock → liest Observer-POD `ComdareTierObserverSnapshotV1`).

---

### §8.8 Das PRÜF-DOCK — der per-Gattung ABI-stabile Mess-Übergang der CacheEngineBuilder (User 2026-05-30)

> **User 2026-05-30 (verbatim-tragend):** „eine geladene Anatomie ist eine bestimmte durch Metaprogrammierung
> unterstützte Gattung eines zu prüfenden per Konfiguration permutierten Tier-Binary-Algorithmus. Bevor eine
> Gattung an Algorithmen als Tier-Module durch die CacheEngineBuilder durchgemessen werden kann, muss die
> Gattung als Variante eines je Gattung ABI-stabilen Messübergangs von der CacheEngineBuilder unterstützt
> werden, wir nennen jede CacheEngineBuilder Seite für eine Gattung ein **Prüf-Dock** (etwa für Search
> Algorithm oder für Container oder für Graphen … die wir vorher auch mit der Metapher Lebewesen→Tiere vs
> Viren beschrieben hatten). In der Konfiguration der CacheEngineBuilder … können bei Bedarf auch multiple
> Gattungen quer gemessen werden, je nach der nächsten durchzumessenden Binary wird ein passendes Prüf-Dock
> verwendet … Default … Binaries derselben Gattung sequentiell, gefolgt von der Sequenz der nachfolgenden
> Gattung …"
> „Die Konfiguration der Anatomie und der compile, sowie das Prüf-Dock ist also Teil der CacheEngineBuilder
> und die Anatomie-Permutation einer Algorithmus-Gattung ist als Binary-Tier-Modul kompiliert und wird durch
> die CacheEngineBuilder durchgemessen."

> ⚠️ **KEIN NEUBAU — nur Benennung (User 2026-05-30):** „Prüf-Docks sind nur ein neuer Begriff der
> CacheEngineBuilder-seitigen ABI-Schnittstelle der VORHANDENEN Architektur für zu prüfende ABI-stabile
> Algorithmen-Binaries, es ist nicht komplett neu, nur anders benannt." → Das Prüf-Dock-Konzept benennt +
> organisiert die BEREITS EXISTIERENDE host-seitige ABI-Mess-Schnittstelle (`AnatomyModuleLoader` +
> `IObservableTier`/`IMeasurableWorkload` + `drive_tier_observe_trace_abi`). Die Umsetzung ist daher eine
> LEICHTE Benennungs-/Organisations-Schicht (per-Gattung-Aliase + Config-getriebene Dock-Auswahl), KEINE
> schwere neue Abstraktion und KEIN Rückbau des Bestehenden.

**Definition.** Ein **Prüf-Dock** ist die **CacheEngineBuilder-SEITE für genau EINE Gattung** (Anatomie-Genus):
ein **per-Gattung ABI-stabiler Mess-Übergang**, der ein als **Binary-Lebewesen-Modul** kompiliertes Lebewesen DIESER
Gattung (a) laedt, (b) ueber die **Gattungs-API** treibt und (c) dessen **Observer** durchmisst + persistiert.
Es gibt ein Prüf-Dock je Gattung: `SearchAlgorithm`-Dock, `Container`-Dock, `Graph`-Dock, … — entsprechend
der Lebewesen-Metapher (Lebewesen-Gattungen vs Viren = Graphen-Algos ohne Anatomie, Doku 14 §33-§40,
`[[execution-engine-als-wurzel]]` `[[anatomie-gattungen]]`). **Dies ist die heutige Architektur unter neuem
Namen** — `IObservableTier` IST der SearchAlgorithm-Gattungs-Antrieb, der Loader IST der Lade-Mechanismus.

**Die CacheEngineBuilder besitzt den vollen Lebenszyklus** (alles INNERHALB der cache-engine):
1. **Anatomie-Konfiguration** — welche Achsen-Permutation (Gattungs-Konfigurator) gemessen wird.
2. **Compile** — die Anatomie-Permutation einer Gattung wird als **Binary-Lebewesen-Modul** (.so/.dll) kompiliert
   (`adhoc_emitter` + `comdare_build_adhoc_modules`).
3. **Prüf-Dock** — das per-Gattung passende Mess-Dock laedt + treibt + misst das Binary-Lebewesen-Modul.

**Gattungs-Constraint.** Ein Prüf-Dock ist an seine Gattung gebunden (cross-genus type-system-mathematisch
unmoeglich, Doku 14 §32, `[[gattungs-constraint-pruefling-merge]]`). Der Loader liest `IAnatomyBase::genus()`
des Moduls; das **passende** Prüf-Dock wird je Binary-Gattung gewaehlt.

**Konfiguration + Sequenzierung.** Die Builder-Config waehlt je naechster zu messender Binary das passende
Prüf-Dock. **Default:** Binaries DERSELBEN Gattung werden SEQUENTIELL durchgemessen, dann die naechste
Gattung, usw. — was nacheinander bestimmte Prüf-Dock-Typen bedarf. **Optional:** multiple Gattungen QUER
gemessen (Config-gesteuert).

**Bezug zum Ist-Stand (R6).** Das **SearchAlgorithm-Prüf-Dock existiert faktisch bereits in Teilen** und ist
zu generalisieren, NICHT neu zu erfinden: `IObservableTier` (ABI-stabiler Gattungs-Antrieb + Observer-Zugriff,
Pfad B) + `AnatomyModuleLoader` (Laden via `genus()`-typisiertes `IAnatomyBase`) + `drive_tier_observe_trace_abi`
(host-seitiger Mess-/Persistier-Treiber). Die Prüf-Dock-Abstraktion hebt diese SearchAlgorithm-spezifische
Verdrahtung auf eine **per-Gattung-Variante eines gemeinsamen Mess-Übergangs** + ergaenzt die **Config-getriebene
Dock-Auswahl + Gattungs-sequentielle Mess-Orchestrierung**.

**✅ Umsetzung (2026-05-30, Planrunde `wt5q7de5t`):** `libs/cache_engine/builder/pruef_dock/`:
`pruef_dock.hpp` (`IPruefDock` + `PruefDockMeasureOptions`=Alias `AbiTierTraceConfig` + `dock_status_*`;
KEINE ABI-Grenze) · `search_algorithm_dock.hpp` (`SearchAlgorithmDock` verkabelt
`dynamic_cast<IObservableTier*>` → `drive_tier_observe_trace_abi` → `serialize_csv/json`, dünner Wrapper) ·
`pruef_dock_registry.hpp` (`PruefDockRegistry` + `select_for` via `genus()`) · `pruef_dock_sequencer.hpp`
(`measure_genus_sequential`: stabile Gattungs-Gruppierung, gleiche Gattung sequentiell, Cartesian-Ordnung
innerhalb erhalten). Test `test_v41_pruef_dock_search_algorithm` (3 gr). **Bewusst NICHT in den
Legacy-ExperimentDriver-phase5 verdrahtet** (zwei Loader-Welten module_loader vs anatomy_module_loader —
Folge-Charge); Set/Sequence/Adapter/View-Docks ab V42 (Gattungs-Implementierungen existieren noch nicht).
> **[korr. 2026-06-03, s. Doc 30 §8.0:** Es gibt KEIN eigenes Set/Sequence/Adapter/View-Dock — die GATTUNG
> ist das Außen-INTERFACE (= ein Prüf-Dock je Gattung: nur **SearchAlgorithm / Container / Graph**). Set,
> Sequence, Adapter, View sind **Lebewesen-Unterklassen UNTER dem Container-Interface**, NICHT je eine eigene
> Gattung; sie teilen sich das EINE **Container-Dock**. „ab V42" gilt also für das **Container-Dock** (+ dessen
> Lebewesen-Unterklassen Set/Sequence/Adapter/View als feste Achsen-Sätze unter dem Container-Interface), nicht für
> vier separate Docks. Das `SearchAlgorithm`-Dock (§8.8) bleibt korrekt = das eine Interface der heute einzigen
> gebauten Lebewesen-Unterklasse (std::map-ähnliche 17/19-Achsen-Komposition).]**

---

### §8.9 PRT-ART-Prüfling-Integration: CMake + Metaprogrammierungs-Join, gemessen via Prüf-Dock (User 2026-05-30)

> **User 2026-05-30 (verbatim-tragend):** „Der PRT-ART ist ein abstract Tier Permutation (angelehnt an das
> Konzept einer abstrakten Klasse) und liefert für einige Achsen **unbekannte Algorithmen**, die dem jeweiligen
> Wissensstand jeder cache-engine-Achse **joinen** können, indem bei der Kompilation **C++23-Metaprogrammierung**
> für das Einbinden der Prüflings-Tier-Stack-Achsen-Algorithmen verwendet wird, wobei die cache-engine die neuen
> Algorithmen auf **3 Join-Mustern** (dokumentiert) auf **2 Arten getrennt** misst und auf **eine Art als
> full-join**, um über das **Dock der Prüflings-Algorithmus-Tier-Gattung** die dabei entstandenen Binaries aus
> der Metaprogrammierung durchzumessen. Einen Prüfling (oder MEHRERE gleichzeitig) einzubinden ist also eine
> **CMake + Metaprogrammierung**-Aufgabe."

**Was PRT-ART ist.** Ein **Prüfling** ist eine **abstrakte Lebewesen-Permutation** (Konzept analog einer abstrakten
Klasse): er liefert für EINIGE Achsen **neue/unbekannte Algorithmen**, die zur **Compile-Zeit per C++23-
Metaprogrammierung** mit dem Wissensstand der jeweiligen cache-engine-Achse **gejoint** werden (Prüflings-Slot
`optional_prt_art_impl` je Achse — ERSETZT-mit-Fallback, `[[feedback_pruefling_replace_not_extend]]`). Achsen
ohne Prüflings-Beitrag fallen auf die CE-Defaults zurück.

**Die 3 Join-Muster (`[[reference_3_kompositionale_joins_anatomie]]`) + ihre Messung:**
| Stufe | Join-Muster | Messung |
|-------|-------------|---------|
| **Stufe 1** `comdare_perms_ce` | NUR cache-engine-Achsen (kein Prüfling) | **getrennt** (Baseline) |
| **Stufe 2** `comdare_perms_<pruefling>` | Prüfling ERSETZT je Achse, sonst CE-Fallback | **getrennt** (Prüfling-vs-Baseline) |
| **Stufe 3** `comdare_perms_full_join` | voller, NICHT-redundanter Join CE × Prüfling | **full-join** (Kombinations-Raum) |

→ **2 Arten getrennt** (Stufe 1 + Stufe 2) **+ 1 Art full-join** (Stufe 3). Die je Stufe per Metaprogrammierung
(`AdHocComposition` + `pruefling_merge.hpp`) entstehenden **Permutations-Binaries** werden über das **Prüf-Dock
der Prüflings-Gattung** (§8.8 — für Suchalgorithmen das `SearchAlgorithmDock`) durchgemessen: laden → Gattungs-API
durchtesten → Observer ziehen → korrelieren → persistieren.

**Konsequenz für R8 (KORREKTUR der Audit-Schätzung).** „prt-art als Prüfling einbinden" ist **KEINE Header-
Kopier-/Migrations-Aufgabe** (Phase-A-Migration ist IST-verifiziert bereits erledigt — alle additiven
basis_missing-Header sind im CE, Planrunde `wrvvcf8ed`/Commit `60b1369`). R8 ist eine **CMake + Metaprogrammierung**-
Aufgabe: (1) die `optional_prt_art_impl`-Slots je Achse befüllen (Compile-Time-Join der Prüflings-Achsen-Algos,
Stufe-2-Pattern), (2) die 3 Join-Muster-Targets (`comdare_perms_ce/_pruefling/_full_join`) als Permutations-
Binaries materialisieren, (3) sie über das Prüf-Dock durchmessen. EINEN oder MEHRERE Prüflinge gleichzeitig =
derselbe Mechanismus (mehrere Slots / mehrere full-join-Faktoren). Erster bounded Schritt: 1 Achse end-to-end
(axis_07_prefetch-Slot → Stufe-2-Verdrahtung → Test).

---

### §8.9.1 Zwei Anatomie-Generatoren EINER Gattung, EIN Prüf-Dock, EINE Konfigurator-Struktur — und die Regel der abstrakt-leeren Achse (User 2026-05-30)

> **User 2026-05-30 (verbatim-tragend):** „[…] beide cache engine und prt-art müssen aus derselben
> Konfigurator-Struktur heraus als zwei Anatomie-Generatoren von Tieren derselben Gattung agieren. Das heißt
> ihre Binaries werden alle durch exakt dasselbe Dock gemessen, aber per Konfiguration sind sie in derselben
> Gattung zwei unterschiedliche Tier-Klassifizierungen (wie es unter den Tieren Säugetiere und Reptilien gibt,
> deren Organe aber im Schnitt gleich und austauschbar sind und es Mischformen wie das Schnabeltier gibt, das
> weder Vogel, noch Reptil, noch Säugetier ist, aber dennoch alle Organe vereint → full-join-Hybrid zweier
> Tierklassen zu einer Tierklasse, beschreibbar durch die mathematische Mengenlehre A, B und A-JOIN-B). Wir
> hatten definiert, dass wenn die Achse eines Prüflings abstrakt leer ist, dass sie dann alle Algorithmen der
> cache engine automatisch als Default wiederverwendet und versucht ALLE Permutationen dieser bereitzustellen,
> um die schnellste Rekombination per Ausmessen für den Prüfling-Algorithmus-Tierklassen-Prototypen zu finden."

**(1) EINE Konfigurator-Struktur, zwei Generatoren.** cache-engine und Prüfling (prt-art) sind KEINE getrennten
Build-Welten: sie sind **zwei Anatomie-Generatoren EINER Gattung** (z.B. SearchAlgorithm), gespeist aus
**derselben Konfigurator-Struktur** — `PermutationEngine<TopicConfigSets...>`
(`src/permutations/permutation_engine.hpp`, `mp_product`-Kartesisch) über die je Achse per
`MergeAxis<Stufe, DefaultList, Slot...>` (`anatomy/pruefling_merge.hpp`) aufgelösten Varianten-Listen. Der
Generator ist identisch; nur die je-Achse-Liste unterscheidet die Stufe/Klassifizierung. Die cache-engine
funktioniert dabei IMMER unter ihrer eigenen Konfiguration (Stufe 1) — der Prüfling ist additiv eingeklinkt.

**(2) EIN Prüf-Dock misst alle.** Weil beide Generatoren Lebewesen DERSELBEN Gattung erzeugen, gehen ALLE ihre
Binaries — Stufe 1 (CE), Stufe 2 (Prüfling), Stufe 3 (Hybrid) — durch **exakt dasselbe Prüf-Dock** (§8.8;
für Suchalgorithmen `SearchAlgorithmDock`). `PruefDockRegistry::select_for` wählt nach `genus()`, **NICHT**
nach CE-vs-Prüfling — die Lebewesen-Klassifizierung ist mess-transparent; identische ABI-Schnittstelle, identischer
Mess-Treiber, identische Observer-Persistierung.

**(3) Mengenlehre der Lebewesen-Klassifizierungen.** Innerhalb der Gattung sind CE und Prüfling **zwei
Lebewesen-Klassifizierungen** (Metapher: Säugetier `A` vs. Reptil `B`) mit im Schnitt gleichen, austauschbaren
Organen (Achsen). Die 3 Join-Muster (§8.9) sind die mengentheoretischen Operationen:

| Stufe | Mengenlehre | Lebewesen-Metapher |
|-------|-------------|---------------|
| Stufe 1 `comdare_perms_ce` | `A` | reine CE-Klasse (Säugetier) |
| Stufe 2 `comdare_perms_<pruefling>` | `B` | reine Prüfling-Klasse (Reptil) |
| Stufe 3 `comdare_perms_full_join` | `A ⋈ B` (full join, dedupliziert) | **Schnabeltier** — Hybrid, der ALLE Organe beider Klassen vereint |

**(4) Die Regel der abstrakt-leeren Achse — der „Lebewesen-Klassen-Prototyp".** Ein Prüfling liefert NUR für EINIGE
Achsen eigene Algorithmen. Für jede Achse, die er NICHT belegt (Slot **abstrakt leer**, `has_pruefling == false`),
gilt: er **reust ALLE CE-Algorithmen dieser Achse als Default** und der Generator **permutiert sie voll aus**.
Type-Mechanik (`pruefling_merge.hpp`): `StufeTwoAxis<DefaultList, EmptyPrueflingSlot> == DefaultList`
(volle CE-Liste). Damit ist der Stufe-2-Raum des Prüfling-Prototyps:

```
B  =  ∏ (belegte Achsen i: {prüfling_i})  ×  ∏ (leere Achsen j: A_j)
   =  1^|belegt|  ×  ∏_j |A_j|
```

— das **kartesische Produkt ÜBER DIE LEEREN ACHSEN**, NICHT eine einzige Komposition. Genau diese
Rekombinationen materialisiert die Metaprogrammierung, und das Prüf-Dock **misst sie alle aus, um die schnellste
Rekombination des Prototyps zu finden**. Die frühere Teilsicht „Stufe 2 = 1 Komposition" gilt NUR, wenn man
ausschließlich die belegten Achsen betrachtet — über die volle (z.B. 17-Achsen-)Anatomie expandieren die leeren
Achsen den Raum.

**Beleg (Compile-Time, IST-verifiziert).** `prt_art/tests/unit/test_prt_art_pruefling_registration.cpp`,
`F5_DreigliedrigkeitPermutationSpace`:
- `.StufeTwoEmptyAxesReuseAllCeAlgorithms` — `static_assert(StufeTwoAxis<…,EmptyPrueflingSlot> == DefaultList)`
  (Type-Beleg) **+** `PermutationEngine::count() == ∏_leer |A_j|  > 1` (Raum-Beleg über 2 leere + 2 belegte Achsen).
- `.ThreeStufenProduceDistinctBinarySetSizes` — Stufe 1 = `∏ A_i`, Stufe 2 (belegt-Teilsicht) = 1,
  Stufe 3 = `∏ (|A_i|+1)` (full-join `A ⋈ B`).

---

**Ende Doku 24 — Mess-Modell-Korrektur (2026-05-29; §8 HYBRID + korrelierte Erhebung + R6 + §8.8 Prüf-Dock + §8.9 Prüfling-Integration + §8.9.1 Zwei-Generatoren/leere-Achse-Regel 2026-05-30).**
