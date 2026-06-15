# Mess-Architektur V5 — bindende User-Entscheidungen (2026-05-31)

> **Status:** Entscheidungs-Log (crash-sicher festgehalten). Die *ausgearbeitete* Doku (auf IST-Code abgebildet, mit
> allen cmake-Flags + Memento/Konformitäts-Inventar) erstellt Workflow `wlw1w69eg` → wird als
> `messarchitektur_v5_design.md` nachgereicht. Ergänzt/REVIDIERT teils: `messarchitektur_klarstellungen_und_entscheidungen.md`
> + `messarchitektur_design_observer_handle_no_dynamic_cast.md` (Capability-Bit/dynamic_cast-Ansatz ist hinfällig — s.u.).

## 1. Zwei Seiten — was wo lebt

**LEBEWESEN-BINARY (.dll):** exportiert **nur `IDriveableTier`** (funktionaler Gattungs-Antrieb, IMMER, ABI-stabil) +
**`observer_all`** + **`memento_all`**. Die letzten beiden werden **NUR bei Messung-AN einkompiliert** — reine
**Compile-Time-Metaprogrammierung, KEIN `dynamic_cast`**. Bei Messung-OFF restlos entfernt → **reine
funktional-getriebene Lebewesen-Binary OHNE Overhead, an die Forschung auslieferbar**. **KEINE Workloads in der DLL**
(das in-DLL `run_workload` war ein schwerer V3-Designfehler).

**HOST (CacheEngineBuilder):** **Prüfdock je GATTUNG** (abstract-factory-ähnlicher Handle → fragt die Observer
**einheitlich** ab, egal welche Implementierung die Binary hat). **`IMeasurableWorkload` = rein generisch
host-seitig** (Pfad A war IMMER hier geplant, nicht in der DLL). Mehrere Lastprofile je Binary; Lebenszyklus
**import → messen → abstoßen**. Host hält nur **Latenz-Mess-State** + traced das **Operations-Protokoll + Ergebnisse**.
Der Lebewesen-Binary-STATE liegt **IN der Binary**, nicht host-seitig.

## 2. Memento_all (NEUES System, parallel zu observe_all)

- Lebewesen-Binary-**einkompiliertes** Memento-System (nur bei Messung-AN, zusammen mit der IO/memory-Achse).
- Rollt den **GESAMTEN Zustand** einer Lebewesen-Binary nach Warmup über **ALLE stateful Achsen** via einkompiliertes
  **Memento-Pattern** zurück und wiederholt die Op.
- **Gigantische Memento-Integration**: einheitliche Memento-Hilfsfunktionen für **ALLE stateful Achsen-Interfaces
  UND alle deren Algorithmen** (Rollback-Funktionen über die Achsen einheitlich, wie die Achsen gegenüber ihren
  Algorithmen selbst).
- **IO/Disk-Persistenz** der Such-Algorithmen ist möglich → **ein einfacher Snapshot reicht NICHT**.
- Ausgeführt **nach dem Observer-Pattern = hybrider Visitor**: die Lebewesen-Binary-**Rollback-Klassen BESUCHEN den Host**
  zur Steuerung.

## 3. Zwei-Phasen-Messung pro Op (default, über alle YCSB-Lastprofile)

Jede Operation **genau zweimal**, mit echten Last-Ops, echten Daten, echten Lebewesen-Binary-Zuständen:
```
memento-save-all  →  op (warmup, kalt)  →  rollback-all  →  op (measure, persistiert + gemessen)
```
**Mess-OFF:** Warmup+Messung laufen trotzdem, aber nur host-seitig über **billige Wall-Clock**, **OHNE** einkompilierte
Statistics/Memento in den DLLs.

## 4. Konformitäts-Gate (NEUER Baustein, schon für V3 fällig)

Jede C++23-Modul-Lebewesen-Binary muss bei Verwendung **zuerst** durch **dieselben `std::map`-Hüllen-Tests** auf
Konformität (alle Randfälle valide, egal wie die Hülle für die Key-Value-Map gebaut ist). Experiment misst **nur
Performance-Eigenschaften** — aber jede Binary muss **nach ihrer Gattung** allgemeine Testdaten **konform
speichern + wiedergeben** können. Gate **vor** der Messung.

## 5. Drei streng getrennte Profile

1. **BUILD-PROFIL (statisch):** Parameter der zu erzeugenden Lebewesen-Binaries + Permutations-Gesetzmäßigkeiten
   **interner + externer abstrakter Lebewesen**.
2. **LASTENPROFIL:** alle Testdaten **UND** Operationsabläufe mit **Pausen + Testzeiten**, je Gattung über das
   algorithmus-ABI-stabile Interface; host-seitig **generisch** je temporär zur Laufzeit per import angebundener
   (+ später abgestoßener) Lebewesen-Binary durchlaufen.
3. **COMPILE-RELEASE-PROFIL:** allgemeine Build-Flags (Messung bauen vs Release-DLL ohne Messung …), als
   **cmake-Config IN der Cache Engine** gesetzt, **DEFAULT = Messung eingebaut**.

**Build-Profil ⊥ Lastenprofil = die ZWEI Haupt-Experiment-Achsen** (von Diplomarbeit/User an die Cache Engine
geliefert, um Resultate zurückzubekommen). **Ablauf:** Profil baut zuerst den **HOST** (CacheEngineBuilder als
Ganzes) → Host **permutiert** eine Lebewesen-Binary-Config → baut zuerst alle konfigurierten DLLs → misst sie danach.

## 6. Observer/Memento-Entfernung = ausschließlich Compile-Time

Nur Metaprogrammierung/Compile-Time entfernt **alle** Observer **und** Memento aus dem **Prüfdock-Handle-Übergang
UND den Lebewesen-Binaries**. → kein `dynamic_cast`, kein Runtime-Capability-Bit. Das **einheitliche Build-Profil** (Host
+ DLLs co-gebaut) macht Runtime-Probing überflüssig.

## 7. Konsequenz für I1 (revidiert)

Der frühere „Capability-Bit / kalter dynamic_cast genügt"-Ansatz ist **hinfällig**. I1 wird:
- `IObservableTier` → **`IDriveableTier`** (Ops, Pflicht, immer) **+ `observer_all` + `memento_all`** (compile-time-Sub-Schnittstellen, nur Messung-AN).
- EIN autoritativer Mess-POD (volle Spalten + HW-Counter) + `ANATOMY_ABI_MAJOR 1→2` → **alle DLLs neu**, Rollback-Tag, volle Regression grün.
- `run_workload` aus der DLL-ABI **entfernen** (host-seitig relokalisieren).
- Detail-Anfass-Liste + Datei:Zeile: `messarchitektur_v5_design.md` (Workflow `wlw1w69eg`).

## 8. IDriveableTier-Vollständigkeit je Gattung = vollständige Standard-Container-Hülle (User 2026-05-31)

**Bindende Erweiterung von [[std_map_unified_interface]]:** Die Antriebs-Schnittstelle einer Lebewesen-Binary
(`IDriveableTier` für die SearchAlgorithm-Gattung) muss **die VOLLSTÄNDIGE Schnittstelle des repräsentativen
Standard-Containers** anbieten — weil das Lebewesen **genau das IST** (eine `std::map`-Hülle).

- **SearchAlgorithm-Gattung ↔ `std::map`-Hülle:** Die aktuell vorhandenen **5 Ops** (`tier_insert/lookup/erase/
  clear/size`) sind **korrekt + super als Mock/Startpunkt**, müssen aber **zukünftig erweitert** werden, bis die
  Hülle der **Vollständigkeit eines echten `std::map`** nahekommt: `operator[]`, `at`, `find`, `count`, `contains`,
  `begin/end`/Iteratoren, `lower_bound/upper_bound/equal_range`, `emplace/insert(hint)`, `erase(range/iterator)`,
  `empty`, `size`, `clear`, `swap`, `merge`, … (über den ABI-uint64-Key/Value-Raum, ABI-stabil projiziert).
- **Sequence-Gattung ↔ `std::vector`-Hülle:** muss **vollständig von `std::vector`** als Hülle ableiten —
  `push_back/pop_back/operator[]/at/front/back/data/begin/end/size/capacity/reserve/resize/insert/erase/clear/empty`, …
- **Verallgemeinerung:** Jede Gattung exponiert über ihren Drive-Handle die **volle Standard-Container-API** ihres
  Repräsentanten (Set ↔ `std::set`, Adapter ↔ Container-Adapter, View ↔ `std::span`/Range-View — je nach Gattungs-Map).

**Bezug:** Genau dafür existiert das **Konformitäts-Gate gegen `std::map`** (§4 / `messarchitektur_v5_design.md` §6):
es testet die Hülle gegen den Standard-Container-Oracle über **alle** Interface-Methoden + Randfälle. Die Erweiterung
der Drive-Interfaces ist damit die Voraussetzung, dass das Gate die **volle** Container-Äquivalenz prüfen kann.

**Status:** Notiert + persistiert. Die 5-Op-`IDriveableTier`-Hülle bleibt der verifizierte Startpunkt (I2.1);
die Voll-API-Erweiterung je Gattung ist ein eigener Inkrement-Strang (V5-I-Drive-Vollausbau, nach dem ABI-Split).
