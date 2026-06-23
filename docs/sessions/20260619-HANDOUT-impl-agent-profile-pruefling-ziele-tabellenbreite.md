# Handout an den Implementierungsagenten (2026-06-19)

> Absender: **Text-Agent** (Diplomarbeit). Empfänger: **Implementierungsagent** (cache-engine / prt-art Code).
> Anlass: Cross-Check der geschärften Front-Matter gegen den Code (Code = Primärquelle). Vier Arbeitspakete,
> die **code-/generator-seitig** zu erledigen sind und die der Text-Agent nicht selbst ausführt
> (Code ist für den Text-Agenten read-only).

---

## TODO-1 — SOTA-Profile auf Paper-Vollabdeckung ausbauen

**Ist-Stand (verifiziert):** `algorithm_profiles/sota/*.profile.xml` = **30** funktionierende Profile;
`compositions/*_reference.hpp` = 13 Referenz-Kompositionen (8 Rang-1). Die Diplomarbeit nennt im Abstract/Intro
jetzt **konsistent „dreißig"** (vorher DE-Abstract fälschlich „mehr als vierzig" — war eine Ziel-Schätzung des
Autors, korrigiert).

**Ziel (Autor-Direktive 2026-06-19):** Jedes in Kap. 3 analysierte Paper (**P01–P33**) liefert
**mindestens einen neuen Algorithmus für mindestens eine Achse**. Zwei Paper-Typen:
1. **Abstrakter Achsen-Satz** — das Paper liefert nur einzelne neue Achsen-Algorithmen (wie PRT-ART),
   keinen vollständigen Algorithmus → fließt als Achsen-Variante(n) in den Baustein-Katalog ein.
2. **Voll-Algorithmus** — das Paper erfüllt die Mindest-Achsenzahl für ein vollständig eigenes Verfahren
   → eigenes vollständiges SOTA-Profil.

**Auftrag:** Lücke 30 → ≥ Paperzahl schließen; je Paper prüfen, welcher Typ vorliegt, und die fehlenden
Profile/Achsen-Konfigurationen anlegen. Mapping-Grundlage = Doc 18 (Algorithmus↔Paper↔Code↔Lizenz) +
`docs/sessions/.../kap3-instanz-mapping-survey` (Thesis-Seite).

---

## TODO-2 — Prüflinge: abstrakte **und** vollständige Varianten + „Originalkonfiguration"

**Definition Prüfling** (zur Selbstvergewisserung nachschlagen: Doc 14 §18–§19 Prüfling-Slot-Pattern,
3 kompositionale Joins): der **Prüfling** ist das zu testende Kandidaten-Lebewesen (z. B. PRT-ART), das
Achsen-Slots gegen den CacheEngine-Standard ersetzt/füllt (Stufe 1 ce-only / Stufe 2 Prüfling-Replace mit
Fallback / Stufe 3 Full-Join).

**Auftrag:** Für jeden Prüfling sicherstellen, dass **beide** Ausprägungen existieren und gemessen werden:
- **Abstrakter Prüfling** — füllt nur eine Teilmenge der Achsen, restliche Achsen via ce-Fallback.
- **Vollständiger Prüfling** — die **„Originalkonfiguration"**: mindestens **einmal** wird das Verfahren
  **NUR** mit den **eigenen** Achsen-Algorithmen des Prüflings *self-contained* gemessen und bewertet
  (vgl. `docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md` §0/§1.2: „bekannte
  Paper-Suchalgorithmen als Basis-Lebewesen in Originalkonfiguration").

**Kennzeichnung (neu):** In der **Profil-Erzeugung** und in den **Messungen** muss je Messreihe der Typ
**`abstract` / `full`** explizit gesetzt und ausgegeben werden (eigenes Feld/Spalte), damit die Auswertung
Original-vs.-rekombinierte Konfiguration trennen kann.

---

## TODO-3 — Noch nicht umgesetzte Ziel-Versprechen implementieren

Diese drei Punkte sind in der Diplomarbeit jetzt **ehrlich als Zielsetzung/Ausblick** formuliert (Code gewinnt),
ihre **Implementierung** ist Aufgabe des Impl-Agenten:
1. **Automatische Auswahl + Versand der besten Binary** — aus den feingliedrigen Messergebnissen die beste
   Permutation post-hoc auswählen und als eigenständige, ABI-stabile Binary ausliefern. (Die Mess-Pipeline
   liefert die Rangbildung bereits; Auswahl + Versand fehlen.)
2. **Laufzeit-dynamische Cache-Line-Anpassung** — anhand eines ausgewerteten Profils zur Laufzeit die
   korrekten cache-line-aware Einstellungen je Last/Operation wählen (statische heuristische System-Richtlinie).
3. **XML-Heuristik-/Lastprofil-Export** — die „Extraktion" als wiederverwendbares XML-Lastprofil-Ergebnis je
   Architekturfokus ablegen, um Heuristiken automatisch zu erzeugen.

---

## TODO-4 — Tabellen-/Diagramm-Generator: Breite auf `\textwidth` zwingen (Layout-Bug)

**Befund (Thesis-Build 2026-06-19):** **184 Overfull-Boxen** (davon **134 > 10 pt**), DE und EN identisch,
**ausschließlich** aus `anhang/{de,en}/tabellen/` (auto-generiert), z. B.:
- `lc_surface_*` (TikZ-Heatmap, `width=0.95\textwidth` + `scale only axis` + `colorbar` + Y-Label → Gesamtbreite
  > `\textwidth`, ~77 pt).
- `cartesian_smoke43_diagram_body` (TikZ ybar, `width=0.85\textwidth`, 43 symbolische x-Koordinaten, `bar width=10pt`).
- `ld_exchange_*` (Alignment ~35 pt zu breit), `bias_matrix_table` (21-Spalten-WIDE-Tabelle).

**Ursache:** Bei `scale only axis` gilt `width=` nur für die **Achsenfläche**; Titel, Achsenbeschriftung,
Tick-Labels und **Colorbar** kommen **zusätzlich** dazu → die Gesamtbox überschreitet `\textwidth`.

**Auftrag (Generator `diagram_generator` REV 7.6 / `csv_to_latex`):**
- Plots mit Colorbar/Y-Label: `width` auf ~`0.72–0.78\textwidth` reduzieren **oder** das gesamte
  `tikzpicture` in `\resizebox{\textwidth}{!}{…}` setzen (Nicht-Float-Variante) bzw. die `figure` mit
  `\centering\makebox[\textwidth]{\resizebox{\textwidth}{!}{…}}` kapseln.
- Sehr breite WIDE-Tabellen (≥ ~15 Spalten): `\scriptsize` + `\setlength{\tabcolsep}{2pt}` + bei Bedarf
  `sidewaystable`/landscape **oder** Spaltensplit.
- Ziel: **0** echte (> 10 pt) Overfull-Boxen aus `tabellen/`.

**Hinweis:** Der Text-Agent patcht diese **auto-generierten** Dateien bewusst **nicht** (würden bei der
nächsten Generierung überschrieben). Es sind Demo-/Platzhalter (smoke43) — bei den echten Kap-7-Messläufen
ohnehin neu zu erzeugen, dann bitte mit der korrigierten Breiten-Logik.

---

## TODO-5 — Architektur-Klärung: `search_engine` (Klasse) ↔ Anatomie — KEIN blinder Umbau

> ⚠️ **REVIDIERT durch TODO-6 (User 2026-06-20):** Die „Option A/B"-Lesart unten (parallel belassen
> als zulässiges Design) ist VERWORFEN. Es gibt nur EINE Architektur (`Lebewesen ≡ SearchAlgorithm`;
> die Anatomie ist sein Körper). Der Code-Ist mit zwei Bäumen ist ein **Defekt**, kein Design.
> Maßgeblich ist **TODO-6** (Vereinheitlichung). TODO-5 bleibt nur als Befund-Dokumentation stehen.

**Anlass:** Cross-Check (Workflow `we4xlbb23`, 5 Trace + 3 adversariale Linsen, Konsens). Die Frage
„besitzt die Such-API-Klasse `search_engine` eine Anatomie (Kette Gattung→Lebewesen→Achsen)?" lautet
**`not_realized`**. Frühere Formulierung „orthogonal" war irreführend → korrekt: **getrennte
Parallel-Hierarchien mit gemeinsamer Wurzel**.

**Ist-Stand (code-belegt):**
- `search_engine<Collection, ConfigPermutation>` (`abi/search_engine.hpp:19-21`) erbt NUR von
  `execution_engine<…>`, trägt KEINE Anatomie/Composition/Achsen; einzige Collection-Links
  `key_t/value_t/binary_key_t` (Z.26-28); reine virtuelle Such-API (lookup/insert/erase/size/empty).
- Die 19 Achsen + die Kette leben im PARALLELEN Baum: `IExecutionEngine → IAnatomyBase →
  SearchAlgorithmAbiAdapter<A>` (`abi_adapter.hpp:118-138`), der die 19 Organe als private Member hält
  (`abi_adapter.hpp:1609-1685`); `A = SearchAlgorithmAnatomy<Composition>`
  (`search_algorithm_anatomy.hpp:31`), `Composition` = 19 `using`-Achsen (`composition_concept.hpp:20-58`).
- KEINE Verbindung zwischen beiden Bäumen außer der Wurzel `execution_engine`/`IExecutionEngine`;
  `search_engine` erscheint in `abi_adapter.hpp` nur im Metapher-Kommentar (Z.9/109). Zusammenführung
  erst zur Laufzeit über Builder/`AnatomyExecutionContext` + `AnatomyModuleLoader`.
- **Namens-Kollision (wichtig):** `search_engine` ist ZWEIERLEI — (a) die *dormant* ABI-Klasse
  `search_engine<>`, (b) ein *lebendiger Namespace* `comdare::cache_engine::search_engine::axis_01_…`
  (Achsen-Topics). Nicht verwechseln.

**Vorbedingung (zuerst klären, NICHT raten):** Ist die KLASSE `search_engine<>` lebend/geplant oder
überholt? Evidenz spricht für **überholt**: (1) keine Instanziierung/Ableitung der Klasse gefunden
(`grep -rn "search_engine<" libs/ tests/` liefert nur Namespace-Treffer); (2)
`identity/prt_art_search_engine_adapter.hpp` ist `deprecated`, „aktueller Adapter =
prt_art_execution_engine_adapter.hpp" (`docs/architecture/19_f6_prtart_migration_plan.md:191`); (3)
Thesis-Intro: „in früheren Entwürfen … ExecutionEngine→SearchEngine→SearchEngineType". → Definitiv via
Git-History/Sessions bestätigen.

**Option A (wahrscheinlich) — Klasse überholt → KEIN Umbau, nur bereinigen:** Falls keine produktiven
Instanzen: `abi/search_engine.hpp` deprecaten bzw. den „Stufe-3/hält-Anatomie"-Kommentar (Z.2-7) tilgen;
in Doku/Thesis die Beziehung als „getrennte Parallel-Hierarchien" führen. Sauberster Weg, kein ABI-Risiko.

**Option B — Klasse SOLL Such-Fassade der Anatomie werden → Kopplung OHNE ABI-Bruch:** NICHT
`search_engine` mit Achsen-Membern aufblähen (verletzt Mess-Pfad-A/B + Observer-Konsolidierung I1).
Stattdessen `SearchAlgorithmAbiAdapter<A>` ZUSÄTZLICH von `search_engine<Collection,ConfigPermutation>`
ableiten (neben `IAnatomyBase`), `Collection`/`ConfigPermutation` aus `A::composition_t` ableiten, die
virtuelle Such-API auf die bestehenden `tier_*`-Ops über `search_organ_`/`container_` mappen. Anatomie
dann von der Such-API via `dynamic_cast<IAnatomyBase*>` auf denselben Adapter erreichbar — kein neuer
Member, kein ABI-Major-Bump (`search_engine` header-only, überquert die `extern "C"`-Factory NICHT).
`static_assert`, dass abgeleitete `key_t/value_t` == `Collection::key_t/value_t`.

**Verbot:** Keine Achsen-Member in `search_engine` selbst (Organe leben genau EINMAL im Adapter — I1).
Kein Quick-Bypass.

**Rückmeldung erbeten:** Welche Option gilt (A/B), bestimmt auch die Thesis-Formulierung in Kap. 4 +
ob das Abstract-Framing „CacheEngine→ExecutionEngine→SearchEngine" angepasst wird.

---

## TODO-6 — Es darf nur EINE Architektur geben (vereinheitlichen: `search_engine<>` ⇆ `SearchAlgorithmAnatomy`/`SearchAlgorithmAbiAdapter`)

> **Revidiert TODO-5:** Die frühere Lesart „Option A/B parallel belassen" ist VERWORFEN. Es gibt keine
> zwei zulässigen Hierarchien. `Lebewesen ≡ SearchAlgorithm` (Metapher vs. technischer Begriff =
> dasselbe Ding; Doc 14 / anatomy_base.hpp:62 „Saeugetier | SearchAlgorithm"). Die Anatomie ist sein
> Körper, kein zweites Modell. Ziel: EINE Hierarchie, KEIN Achsen-Duplikat (I1).

### Befund (Code-Ist = Defekt, zwei getrennte Bäume desselben Dings)
1. `search_engine<Collection, ConfigPermutation> : public execution_engine<typename ConfigPermutation::strategy_t>`
   (`libs/cache_engine/include/cache_engine/abi/search_engine.hpp:19-21`) — trägt Container-API
   (`lookup/insert/erase`, Z.36-38), aber KEINE 19 Achsen, KEINE Observer-Statistik.
2. `SearchAlgorithmAnatomy<Composition>` (`anatomy/search_algorithm_anatomy.hpp:32`) — der echte Körper:
   19 Organe + `observe_all()` (Z.46/62).
3. `SearchAlgorithmAbiAdapter<A> final : public IAnatomyBase` (`anatomy/abi_adapter.hpp:119`,
   `static_assert(A::genus()==AnatomyGenus::SearchAlgorithm)`, „I1: GENAU EINE Observer-Schnittstelle"
   Z.124-125) — die Runtime-ABI-SICHT desselben Lebewesens.
4. Wurzel `IExecutionEngine` (`execution_engine/execution_engine_base.hpp:98`); `IAnatomyBase :
   IExecutionEngine` (Doc 14 §33-§40).

**Problem:** (2)/(3) erben von `IAnatomyBase`→`IExecutionEngine` (virtuelle Runtime-Wurzel); (1) erbt von
`execution_engine<>` (C++-Template-Ebene). Kein gemeinsamer Code, duplizierte Achsen-/Slot-Verwaltung,
Observability nur auf der Anatomie-Seite, drei „Container"-Instanzen desselben Lebewesens (Anatomie-Organ
/ Builder-`container_` / lokales `run_workload`-SearchAlgo — Doc 24 §-Tabelle). Das ist eine I1-Verletzung.

### Soll (EINE Hierarchie)
```
IExecutionEngine                       (Wurzel, alles Ausmessbare; Virus = Geschwister)
  └─ IAnatomyBase : IExecutionEngine   (Lebewesen-Spezialisierung)
       └─ SearchAlgorithmAnatomy<Composition>   (Körper; 19 Organe + observe_all)
            └─ SearchAlgorithmAbiAdapter<A>      (ABI-Laufzeit-SICHT desselben Lebewesens)
                 ↓ extern "C"  →  .dll-Grenze  →  Prüf-Dock (Gattungs-Interface, Host)
```
Achsen existieren GENAU EINMAL: in `Composition` / den Organ-Membern der Anatomie. NICHT zusätzlich in
`ConfigPermutation::strategy_t` / `search_engine<>`-Template-Parametern.

### Sauberster Weg (minimal, keine Achsen-Duplikate — I1)
1. **`search_engine<>` ist KEINE eigene Lebewesen-/Achsen-Hierarchie mehr.** Entweder (a) **entfernen**,
   oder (b) **umbenennen** zu `search_composition_template<>` und ausschließlich als reinen
   Compile-Time-Kompositions-Helfer nutzen (KEINE virtuelle Klasse, KEINE eigenen Achsen).
   (`search_engine.hpp:19-71`).
2. **Achsen-Single-Source:** Alle 19 Slots leben nur in `Composition` / `SearchAlgorithmAnatomy`.
3. **Observability konsolidieren (I1 + Doc 24/34 §6):** `observe_all()` real über alle 19 Achsen,
   ausschließlich aus der Anatomie; EIN POD über die Grenze (`ComdareTierObserverSnapshot`, V3, 19 Achsen);
   genau EINE `IObservableTier` (bereits abi_adapter.hpp:124-125).
4. **Eine Container-Instanz:** Die drei „Container"-Instanzen auf das Anatomie-Organ reduzieren.
5. **Invariante (Code-Kommentar + Doc):** „Lebewesen ≡ SearchAlgorithm; Anatomie = sein Körper;
   ‚SearchEngine' = nur ABI-Sicht desselben Lebewesens. KEINE Parallel-Hierarchie." `static_assert(genus()==SearchAlgorithm)` bleibt.

### Nicht tun
- KEINE separate Vererbungs-Schicht zwischen `ExecutionEngine` und der Anatomie wieder einführen.
- `search_engine<>` NICHT als zweite virtuelle Lebewesen-Klasse mit eigenen Achsen behalten (= der TODO-5-Fehler).

### Akzeptanzkriterium
Grep findet keine zweite, achsentragende „Engine"-Hierarchie außerhalb von
`IAnatomyBase`/`SearchAlgorithmAnatomy`. `observe_all()` liefert 19 reale Achsen-Snapshots aus genau EINER
Composition-Instanz. Build grün; ABI-Major unverändert oder bewusst gebumpt + dokumentiert.

---

## TODO-7 — Cross-Achsen-Delegation vollständig: Thesis-Claim §3.3 „verteilte Interfaces" überall einlösen

**Anlass (Autor-Schärfung 2026-06-23):** Kap. 3 §3.3 („Achsen-Sezierung der Suchverfahren",
`\label{sec:sota-axes}`) sagt jetzt: „viele Achsen sind in Sub-Achsen verfeinert **und liefern anderen
Achsen verteilt interfaces für die Durchführung und Optimierung ihrer Detail-Operationen**." Text-Agent
hat das gegen den Code auditiert (read-only, alle Funde selbst per grep/Read verifiziert).

**Ist-Stand (verifiziert) — der Claim trifft im KERN zu und ist NICHT zu entschärfen:** Der
distributed-interface-Mechanismus ist real. Der Storage-Organ exportiert **9** `organ_observe_<achse>`-
Methoden (`libs/cache_engine/axes/node/axis_04_node_type_chunked_store.hpp:110-201`: node_type, layout,
serialization, value_handle, isa, index_org, io_dispatch, migration, filter), je Achse über die ECHTEN
gespeicherten Slots getrieben; `observable_composed_search.hpp:120-171` reicht sie als `store_observe_*`
durch; `anatomy/abi_adapter.hpp` (`fill_observer_v3`) orchestriert alle 19 Slots. T7 prefetch konsumiert
die Store-Adressen direkt (`axis_07_prefetch_observable.hpp:131-132`, `drive(store,i)`). Bilanz: **≈9
Konsumenten + ≈4 Provider** (T4/T5/T6 + StorageOrgan-Contract); Sub-Achsen-Verfeinerung **17/19** Achsen
(`_subaxes_`-Tag-Dateien). „viele Achsen" ist damit belegt.

**Lücken, wo der Claim NICHT überall gilt (schließen, damit „überall" ehrlich wird):**

1. **HART (bereits als OFFEN dokumentiert) — T0 search_algo, Nicht-Store-Pfad.** Store-traversierbare
   Algos (LinearScan/SortedBinary über den Chunked-Store) delegieren korrekt; **Tree/Trie/Hash/k-ary/
   Eytzinger melden ihre Such-Metriken weiter aus einem monolithischen `search_organ_`** statt über die
   Speicher-Achsen T4/T5/T6 (`anatomy/abi_adapter.hpp:910-916`, Guard
   `tier_search_routes_through_store()==false` bei `:1461`). Identisch zu
   `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md:154` („Q2 Schritt 4 … volle
   Such-Organ-Delegation, `search_organ_` entfällt: **OFFEN**"). **Auftrag:** restliche Such-Organe wie
   den composed-Pfad über die Storage-Achsen führen; `search_organ_`-Sonderpfad entfernen → ALLE
   Such-Algos delegieren ihre Storage-Detail-Ops an node/layout/allocator.

2. **REVIEW — T1 cache_traversal & T2 mapping: redundanter Eigen-Zustand.** Beide führen eine eigene
   interne Lookup-Tabelle (`ct_organ_.register_entry`, `map_organ_.register_slot`) PARALLEL zum echten
   Store, statt ihn zu konsumieren (`anatomy/abi_adapter.hpp:704-710, 769-773`). **Entscheiden +
   umsetzen ODER bewusst als self-contained-by-design dokumentieren:** soll T2 (key→Position) die
   Store-Resolution treiben (dann Doppel-Zustand entfernen), oder ist der Eigen-Index gewollt? Falls
   gewollt → in Thesis/Doc als Ausnahme benennen, damit „verteilt interfaces" nicht als ausnahmslos
   gelesen wird.

3. **PARTIELL — T15 migration_policy.** Decide-Scan läuft über den Store; der echte 2-Tier-Blockmove
   (`organ_migrate_step`, `axis_04_node_type_layout_aware_store.hpp:273`) ist nur über `container_tier1_`
   verdrahtet und liefert für `NoMigration` 0 (`anatomy/abi_adapter.hpp:1528-1574`). Zweite Tier-
   Delegation des reicheren LayoutAware-Pfads für die Default-Tiere nachziehen.

**Self-contained-by-design (KEINE Lücke, zur Klarstellung):** T8 concurrency (treibt echtes
Sync-Primitive), T10 telemetry (querschnittlich), T17/T18 queuing (eigener Puffer) delegieren
korrekterweise NICHT an den Store; der Claim „viele" (nicht „alle") deckt das ab.

**Akzeptanzkriterium:** Nach (1) bezieht jeder Such-Algo (auch Tree/Trie/Hash) seine Storage-Detail-Ops
über die Speicher-Achsen (kein `search_organ_`-Sonderpfad); Doc 30 Q2 Schritt 4 OFFEN → erledigt.
(2)/(3) umgesetzt oder als bewusste Ausnahme dokumentiert. Damit gilt die §3.3-Schärfung nachweisbar
„überall", wo eine Achse eine delegierbare Detail-Operation hat. **Thesis bleibt unverändert** („viele"
ist bereits ehrlich); dieses TODO bringt nur den Code auf „überall".

**Thesis-Anker:** Kap. 3 §3.3 erster Absatz (`thesis/diplomarbeit/kapitel/{de,en}/03_state_of_the_art.tex`, `sec:sota-axes`).

---

### Quer-Referenzen
- Thesis-Tasks: AP-X1 (Text erledigt), AP-X2 (= dieses Handout), AP-X4 (Layout → TODO-4), AP-X5 (Architektur-Verifikation → TODO-5), AP-X6 (EINE Architektur → TODO-6). **Neu: §3.3-Delegations-Audit → TODO-7** (Cross-Achsen-Interfaces; Bezug Doc 30 Q2 Schritt 4).
- Frühere Handouts: `20260617-HANDOUT-impl-agent-io-achse-tpie-mehlhorn.md`,
  `20260617-HANDOUT-impl-agent-CE1-funchops-CE2-dataloader.md`.
