# Mess-Architektur V5 — bindende User-Entscheidungen (2026-05-31)

> **Status:** Entscheidungs-Log (crash-sicher festgehalten). Die *ausgearbeitete* Doku (auf IST-Code abgebildet, mit
> allen cmake-Flags + Memento/Konformitäts-Inventar) erstellt Workflow `wlw1w69eg` → wird als
> `messarchitektur_v5_design.md` nachgereicht. Ergänzt/REVIDIERT teils: `messarchitektur_klarstellungen_und_entscheidungen.md`
> + `messarchitektur_design_observer_handle_no_dynamic_cast.md` (Capability-Bit/dynamic_cast-Ansatz ist hinfällig — s.u.).

## 1. Zwei Seiten — was wo lebt

**TIER-BINARY (.dll):** exportiert **nur `IDriveableTier`** (funktionaler Gattungs-Antrieb, IMMER, ABI-stabil) +
**`observer_all`** + **`memento_all`**. Die letzten beiden werden **NUR bei Messung-AN einkompiliert** — reine
**Compile-Time-Metaprogrammierung, KEIN `dynamic_cast`**. Bei Messung-OFF restlos entfernt → **reine
funktional-getriebene Tier-Binary OHNE Overhead, an die Forschung auslieferbar**. **KEINE Workloads in der DLL**
(das in-DLL `run_workload` war ein schwerer V3-Designfehler).

**HOST (CacheEngineBuilder):** **Prüfdock je GATTUNG** (abstract-factory-ähnlicher Handle → fragt die Observer
**einheitlich** ab, egal welche Implementierung die Binary hat). **`IMeasurableWorkload` = rein generisch
host-seitig** (Pfad A war IMMER hier geplant, nicht in der DLL). Mehrere Lastprofile je Binary; Lebenszyklus
**import → messen → abstoßen**. Host hält nur **Latenz-Mess-State** + traced das **Operations-Protokoll + Ergebnisse**.
Der Tier-Binary-STATE liegt **IN der Binary**, nicht host-seitig.

## 2. Memento_all (NEUES System, parallel zu observe_all)

- Tier-Binary-**einkompiliertes** Memento-System (nur bei Messung-AN, zusammen mit der IO/memory-Achse).
- Rollt den **GESAMTEN Zustand** einer Tier-Binary nach Warmup über **ALLE stateful Achsen** via einkompiliertes
  **Memento-Pattern** zurück und wiederholt die Op.
- **Gigantische Memento-Integration**: einheitliche Memento-Hilfsfunktionen für **ALLE stateful Achsen-Interfaces
  UND alle deren Algorithmen** (Rollback-Funktionen über die Achsen einheitlich, wie die Achsen gegenüber ihren
  Algorithmen selbst).
- **IO/Disk-Persistenz** der Such-Algorithmen ist möglich → **ein einfacher Snapshot reicht NICHT**.
- Ausgeführt **nach dem Observer-Pattern = hybrider Visitor**: die Tier-Binary-**Rollback-Klassen BESUCHEN den Host**
  zur Steuerung.

## 3. Zwei-Phasen-Messung pro Op (default, über alle YCSB-Lastprofile)

Jede Operation **genau zweimal**, mit echten Last-Ops, echten Daten, echten Tier-Binary-Zuständen:
```
memento-save-all  →  op (warmup, kalt)  →  rollback-all  →  op (measure, persistiert + gemessen)
```
**Mess-OFF:** Warmup+Messung laufen trotzdem, aber nur host-seitig über **billige Wall-Clock**, **OHNE** einkompilierte
Statistics/Memento in den DLLs.

## 4. Konformitäts-Gate (NEUER Baustein, schon für V3 fällig)

Jede C++23-Modul-Tier-Binary muss bei Verwendung **zuerst** durch **dieselben `std::map`-Hüllen-Tests** auf
Konformität (alle Randfälle valide, egal wie die Hülle für die Key-Value-Map gebaut ist). Experiment misst **nur
Performance-Eigenschaften** — aber jede Binary muss **nach ihrer Gattung** allgemeine Testdaten **konform
speichern + wiedergeben** können. Gate **vor** der Messung.

## 5. Drei streng getrennte Profile

1. **BUILD-PROFIL (statisch):** Parameter der zu erzeugenden Tier-Binaries + Permutations-Gesetzmäßigkeiten
   **interner + externer abstrakter Tiere**.
2. **LASTENPROFIL:** alle Testdaten **UND** Operationsabläufe mit **Pausen + Testzeiten**, je Gattung über das
   algorithmus-ABI-stabile Interface; host-seitig **generisch** je temporär zur Laufzeit per import angebundener
   (+ später abgestoßener) Tier-Binary durchlaufen.
3. **COMPILE-RELEASE-PROFIL:** allgemeine Build-Flags (Messung bauen vs Release-DLL ohne Messung …), als
   **cmake-Config IN der Cache Engine** gesetzt, **DEFAULT = Messung eingebaut**.

**Build-Profil ⊥ Lastenprofil = die ZWEI Haupt-Experiment-Achsen** (von Diplomarbeit/User an die Cache Engine
geliefert, um Resultate zurückzubekommen). **Ablauf:** Profil baut zuerst den **HOST** (CacheEngineBuilder als
Ganzes) → Host **permutiert** eine Tier-Binary-Config → baut zuerst alle konfigurierten DLLs → misst sie danach.

## 6. Observer/Memento-Entfernung = ausschließlich Compile-Time

Nur Metaprogrammierung/Compile-Time entfernt **alle** Observer **und** Memento aus dem **Prüfdock-Handle-Übergang
UND den Tier-Binaries**. → kein `dynamic_cast`, kein Runtime-Capability-Bit. Das **einheitliche Build-Profil** (Host
+ DLLs co-gebaut) macht Runtime-Probing überflüssig.

## 7. Konsequenz für I1 (revidiert)

Der frühere „Capability-Bit / kalter dynamic_cast genügt"-Ansatz ist **hinfällig**. I1 wird:
- `IObservableTier` → **`IDriveableTier`** (Ops, Pflicht, immer) **+ `observer_all` + `memento_all`** (compile-time-Sub-Schnittstellen, nur Messung-AN).
- EIN autoritativer Mess-POD (volle Spalten + HW-Counter) + `ANATOMY_ABI_MAJOR 1→2` → **alle DLLs neu**, Rollback-Tag, volle Regression grün.
- `run_workload` aus der DLL-ABI **entfernen** (host-seitig relokalisieren).
- Detail-Anfass-Liste + Datei:Zeile: `messarchitektur_v5_design.md` (Workflow `wlw1w69eg`).
