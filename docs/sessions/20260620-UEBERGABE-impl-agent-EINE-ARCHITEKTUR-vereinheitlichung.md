# ÜBERGABE an den Implementierungs-Agenten — Vereinheitlichung auf EINE Architektur

> **Von:** Text-Agent (Diplomarbeit). **An:** Implementierungs-Agent (cache-engine / prt-art Code).
> **Datum:** 2026-06-20. **Status:** verbindlich. **Vollreferenz:** `docs/architecture/36_eine_architektur_lebewesen_ist_searchalgorithm.md`.
> **Verifikation:** Workflows `we4xlbb23` (Code-Ist) + `w8aqb657k` (Doku-Intention, adversarial).
> **Ersetzt:** die „Option A/B parallel belassen"-Lesart aus Handout
> `20260619-…-tabellenbreite.md` TODO-5 (VERWORFEN). Dieses Dokument = der maßgebliche TODO-6.

---

## 1. Auftrag in einem Satz
Führe die beiden getrennten Code-Bäume, die heute dasselbe SearchAlgorithm-Lebewesen abbilden
(`search_engine<>` vs. `SearchAlgorithmAnatomy`/`SearchAlgorithmAbiAdapter`), zu **EINER** Hierarchie
zusammen — ohne Achsen-Duplikat (Observer-Konsolidierung **I1**, Doc 31).

## 2. Verbindlicher Grundsatz (NICHT verhandelbar)
- Es gibt **genau eine** Architektur. **Keine** Parallel-Hierarchien.
- **`Lebewesen ≡ SearchAlgorithm`** — Metapher und technischer Identifier bezeichnen *dasselbe*
  (`anatomy_base.hpp:62`: „Saeugetier | SearchAlgorithm"). Niemals als zwei Dinge behandeln.
- Die **Anatomie ist der Körper** des Lebewesens (ein Ding mit ihm), kein zweites Modell daneben.
- **„SearchEngine"** ist nur die **ABI-Laufzeit-Sicht** desselben Lebewesens
  (`SearchAlgorithmAbiAdapter`), keine eigene Hierarchie.
- Die Organe (Achsen) leben **genau einmal** — in der `Composition` / der Anatomie. Niemals zusätzlich
  in `search_engine<>`-Template-Parametern.

## 3. IST-Zustand (Defekt, code-belegt)
1. `search_engine<Collection, ConfigPermutation> : public execution_engine<typename ConfigPermutation::strategy_t>`
   — `include/cache_engine/abi/search_engine.hpp:19–21`. Trägt `lookup/insert/erase` (Z.36–38), aber
   **keine** 19 Achsen, **keine** Observer-Statistik. Selbstbeschreibung Z.2: „Stufe 3 der
   Drei-Schichten-Hierarchie".
2. `SearchAlgorithmAnatomy<Composition>` — `anatomy/search_algorithm_anatomy.hpp:32` (`genus()==SearchAlgorithm`
   Z.46; `observe_all()→ObserverAggregate` Z.62). **Der echte Körper, 19 Organe.**
3. `SearchAlgorithmAbiAdapter<A> final : public IAnatomyBase` — `anatomy/abi_adapter.hpp:119`
   (`static_assert(A::genus()==AnatomyGenus::SearchAlgorithm)`; „I1: GENAU EINE Observer-Schnittstelle"
   Z.124–125; Organ-Member Z.1609–1685). **Die ABI-Sicht desselben Lebewesens.**
4. Wurzel `IExecutionEngine` — `execution_engine/execution_engine_base.hpp:98`; `IAnatomyBase :
   IExecutionEngine` (Doc 14 §33–§40).

**Bruch:** (2)/(3) erben von `IAnatomyBase`→`IExecutionEngine`; (1) von `execution_engine<>`. Kein
gemeinsamer Code; duplizierte Achsen-/Slot-Verwaltung; `observe_all()` nur auf der Anatomie-Seite; drei
„Container"-Instanzen desselben Lebewesens (Anatomie-Organ / Builder-`container_` / lokales
`run_workload`-SearchAlgo — Doc 24 §-Tabelle). `grep search_engine` in `abi_adapter.hpp` findet nur den
Metapher-Kommentar (Z.9/109), keinen Code → die Bäume sind unverbunden.

## 4. SOLL (EINE Hierarchie)
```
IExecutionEngine                       (Wurzel; Virus = Geschwister)
  └─ IAnatomyBase : IExecutionEngine   (Lebewesen-Spezialisierung)
       └─ SearchAlgorithmAnatomy<Composition>   (Körper; 19 Organe + observe_all)
            └─ SearchAlgorithmAbiAdapter<A>      (ABI-Laufzeit-SICHT; .dll-Grenze → Prüf-Dock/Host)
```

## 5. Konkrete Schritte (sauberster Weg, minimal, kein Achsen-Duplikat)
1. **`search_engine<>` ist KEINE eigene Lebewesen-/Achsen-Hierarchie mehr.** Entweder (a) **entfernen**,
   oder (b) **umbenennen** zu z.B. `search_composition_template<>` und ausschließlich als reinen
   **Compile-Time-Kompositions-Helfer** nutzen (KEINE virtuelle Klasse, KEINE eigenen Achsen).
   Datei: `include/cache_engine/abi/search_engine.hpp:19–71`. Vorab via Git-History/Grep bestätigen,
   dass die KLASSE `search_engine<…>` nirgends produktiv instanziiert/abgeleitet wird (Namens-Kollision
   beachten: `search_engine` ist auch ein lebendiger **Namespace** der Achsen-Topics —
   `…::search_engine::axis_01_…` — der bleibt unangetastet).
2. **Achsen-Single-Source:** Alle 19 Slots leben nur in `Composition` / `SearchAlgorithmAnatomy`. Keine
   zweite Achsen-/Strategie-Liste in `search_engine<>`-Parametern.
3. **Observability konsolidieren (I1, Doc 31 + Doc 34 §6):** `observe_all()` real über alle 19 Achsen,
   ausschließlich aus der Anatomie; EIN POD über die Grenze (`ComdareTierObserverSnapshot`, V3-Schema,
   19 Achsen); genau EINE `IObservableTier` (bereits `abi_adapter.hpp:124–125` angelegt). Prüfen, ob
   `SearchAlgorithmAbiAdapter` direkt von `SearchAlgorithmAnatomy<A::composition_t>` ableiten/halten
   kann, um die Composition nicht zu duplizieren.
4. **Eine Container-Instanz:** Die drei „Container"-Instanzen auf das Anatomie-Organ reduzieren
   (Builder-`container_` und das lokale `run_workload`-SearchAlgo an dasselbe Organ binden; Doc 24).
5. **Invariante festhalten** (Code-Kommentar + Doc): „`Lebewesen ≡ SearchAlgorithm`; Anatomie = sein
   Körper; ‚SearchEngine' = nur ABI-Sicht desselben Lebewesens. KEINE Parallel-Hierarchie."
   `static_assert(genus()==SearchAlgorithm)` bleibt.

## 6. Ausdrücklich NICHT tun
- KEINE Achsen-Member in `search_engine` selbst (würde die Organe verdoppeln — I1-Verletzung).
- KEINE separate Vererbungs-Schicht zwischen `ExecutionEngine` und der Anatomie wieder einführen.
- `search_engine<>` NICHT als zweite virtuelle Lebewesen-Klasse mit eigenen Achsen behalten (das war
  der TODO-5-Fehler).
- Den **Namespace** `comdare::cache_engine::search_engine::…` (Achsen-Topics) NICHT anfassen.

## 7. Akzeptanzkriterien
- `grep` findet keine zweite, **achsentragende** „Engine"-Hierarchie außerhalb von
  `IAnatomyBase`/`SearchAlgorithmAnatomy`.
- `observe_all()` liefert **19 reale Achsen-Snapshots** aus **genau EINER** Composition-Instanz.
- Build grün; bestehende Tests grün. ABI-Major unverändert ODER bewusst gebumpt **und dokumentiert**
  (Mess-Daten bleiben unangetastet — separate DLLs, keine `.csv` berührt; vgl. Direktive „Messdaten nie
  löschen").
- Ein kurzer Eintrag im Architektur-Ledger / Doc 36 §6 dokumentiert die durchgeführte Vereinheitlichung.

## 8. Verweise
- **Vollreferenz:** Doc 36 `docs/architecture/36_eine_architektur_lebewesen_ist_searchalgorithm.md`.
- Doc 14 `docs/architektur/14_achsen_komposition_organ_metapher.md` (§1, §3, §33–§40);
  Doc 34 `34_KONSOLIDIERTER_MASTER_IST_STAND.md`; Doc 30 §8.0/§8.1; Doc 31 (I1); Doc 24 (Container-Instanzen).
- Vorheriges Handout `20260619-HANDOUT-impl-agent-profile-pruefling-ziele-tabellenbreite.md`
  (TODO-1 Profile P01–P33, TODO-2 abstract/full-Prüflinge + Originalkonfiguration, TODO-3 Ziel-Versprechen,
  TODO-4 Tabellen-/Diagramm-Breite [erledigt], TODO-5 [durch dieses Dokument ersetzt], TODO-6 = dies).
