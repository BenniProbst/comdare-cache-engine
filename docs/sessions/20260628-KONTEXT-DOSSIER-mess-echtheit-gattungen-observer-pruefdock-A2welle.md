# KONTEXT-DOSSIER — Mess-Echtheit · Gattungen · Observer-Aufbau durch das Prüf-Dock (A2/Audit-Welle #211–#226 + #188)

> **Zweck.** Abgesicherter, durable Kontext-Anker, der VOR jeder Bearbeitung der Audit-Welle (#211–#226) und
> des Befund-2-Kerns (#188) gelesen wird. Er erdet das konzeptionelle Fundament (Gattungen/Anatomie),
> den Observer-Aufbau durch das Prüf-Dock, die Apparat-Reinheit + Meta-Lehre #3, das **code-verifizierte
> Audit-Verdikt** (haben wir uns verrannt?) und die **eng angelegte Aufgabe #212** in elaborierter Form.
> Erstellt 2026-06-28 (Impl-Agent), nachdem die Session mit genau diesem Problem begann (s. §0).
> **Single-Source-Vorrang:** bei Widerspruch schlägt die Diplomarbeit (`thesis/diplomarbeit`) jeden Code-/Doc-Stand.

---

## §0 Warum dieses Dossier (das Problem, mit dem die Session begann) — die „fünfte Aufgabe" (#227)

Die Session sollte die Audit-Welle **#211–#226** abarbeiten, beginnend mit **#212 (NullNotify)**. Beim Einstieg
zeigte sich eine **Verständnis-Lücke**, die größer war als der Einzel-Fix: die Verwechslung von

- der **Gattung „Suchalgorithmus"** (Ebene-1-Außen-Interface) ⇄
- dem **internen `container_`-Member** der SearchAlgorithm-Anatomie (der `LayoutAwareChunkedStore`, der die Records hält) ⇄
- der **Container-Gattung** (eine eigene Ebene-1-Gattung neben SearchAlgorithm/Graph).

Diese Namens-/Konzept-Kollision (`container_` ≠ Container-Gattung) hätte einen falsch gerahmten Eingriff
ermöglicht. Daher wurde — auf User-Direktive — erst das **gesamte Observer-/Gattungs-Konzept im Kontext des
Gesamt-Compiles** durchdrungen, die Diplomarbeit + alle relevanten Sessions gelesen, und ein **Audit** gefahren,
ob wir uns verrannt haben. Dieses Dossier hält das Ergebnis fest (= Task **#227**).

---

## §1 Das Drei-Ebenen-Modell der ANATOMIE (A1 Gattung / A2 Unterklasse / A3 Organe) — Soll = Diplomarbeit

> **⚠️ ZWEI orthogonale „Ebenen"-Systeme — Konvention ab 2026-06-28 (NICHT verwechseln; die alte Nummern-Kollision ist hiermit aufgelöst):**
> - **A-Ebenen = ANATOMIE-Struktur EINES Lebewesens (dieser §1):** **A1** Gattung (außen, Prüf-Dock-Interface) · **A2** Gattungs-Unterklasse (fixer Achsen-Satz) · **A3** Organe/Achsen (innen). Außen→innen.
> - **E-Ebenen = EXPERIMENT-Maschinerie / Mess-Pipeline (§13):** **E4** XML-Experiment-Definition + Messwert-Extraktion/Auswertung (Superprojekt) · **E3** Permutations-B+-Baum pro Gattung · **E2** Tier-Binaries (StaticAxisNode) · **E1** RC-Laufzeit (DynamicVariableNode). Oben→unten.
> - **Beziehung:** die E-Pipeline permutiert + misst die A-Strukturen — **E2** materialisiert je EINE vollständige Anatomie (A1+A2+A3) als statische Tier-Binary; **E3** (B+-Baum *pro Gattung* = pro A1) permutiert die Organe/Achsen (A3) der Unterklasse (A2). **Die TODO-Ebenen-Notizen (§15) referenzieren stets die E-Ebenen.** Die folgenden §1-Bullets „Ebene 1/2/3" SIND A1/A2/A3.

Quelle (autoritativ): `thesis/diplomarbeit/anhang/de/C_glossary.tex` (Definitionen) + `…/kapitel/de/04_concept_architecture.tex`
(`ssec:three-levels`).

- **Ebene 1 — Gattung** = „das **nach außen sichtbare Algorithmus-Interface (das Prüf-Dock)**". Drei anatomie-tragende
  Gattungen als **Geschwister** unter der Wurzel `IExecutionEngine`/`IAnatomyBase` (+ achsenlose **Viren**):
  - **SearchAlgorithm** — Schlüssel-Wert-Interface (`std::map`-artig). Fokus der Arbeit; „das fokale Lebewesen (Säugetier)".
  - **Container** — **schlüsselloser** Speicher.
  - **Graph** — eigener, **andersartiger Organsatz**.
- **Ebene 2 — Gattungs-Unterklasse** (Code: `AnatomyGenus`) = „legt einen **festen Achsen-Satz** fest":
  - unter **SearchAlgorithm**: die **volle 19-Achsen-Anatomie** (das „Säugetier").
  - unter **Container**: **Set · Sequence · Adapter · View** („Vogel/Reptil/Wirbelloses/Pflanze").
  - **Graph**: eigener Organsatz.
- **Ebene 3 — Organe/Achsen** = die einzelnen Achsen (SearchAlgorithm: T0–T18), die *innerhalb* der Unterklasse permutiert werden.

**Was die Gattungen unterscheidet — der Basis-Achsen-Satz** (`04_concept…` Z.27–37 / `03_state_of_the_art.tex` §3.1):
> „Nicht-baumartige Suchverfahren — etwa eine Hash-Tabelle — … belegen [baum-spezifische Achsen] als **Durchreich-Varianten**
> und tragen nur das gemeinsame Achsen-Subset; sie bleiben **dieselbe Gattung mit eingeschränktem Profil**. Eine **eigene
> Gattung entstünde erst, wenn sich der Basis-Achsen-Satz grundlegend unterschiede.**"

⟹ Eine Gattung = **Interface-Kategorie + fixer Basis-/Minimal-Achsen-Satz (Organsatz)**. Pass-through-Varianten bleiben dieselbe
Gattung; ein **fundamental anderer Basis-Achsen-Satz ⟹ neue Gattung**.

| Gattung | Außen-Interface | Basis-Organsatz (Wesen des Unterschieds) |
|---|---|---|
| **SearchAlgorithm** | Schlüssel→Wert (`std::map`) | die 19 Such-Achsen; der Schlüssel ist Ordnungsprinzip (search_algo T0 lokalisiert Keys; mapping/path-compression/node-type … = keyed Baum-Anatomie) |
| **Container** | schlüssellos | fundamental anderer Satz: Mitgliedschaft/Ordnung/Position — Set (kein Wert), Sequence (positionsindexiert + `axis_growth`), Adapter (umhüllt `inner_container`), View (non-owning: extent/layout/accessor) |
| **Graph** | Graph | eigener Organsatz (Kanten/Knoten/Adjazenz/Graph-Traversal) |

### §1.1 Das Gattungs-Außen-Interface ist eine KONKRETE STL-Schnittstellen-Hülle (User-Präzisierung 2026-06-28)

Das „Außen-Interface" einer Gattung ist nicht bloß „artig", sondern eine **konkrete STL-Schnittstellen-Hülle, die JEDES Lebewesen der Gattung ableitet/erfüllt** (das Prüf-Dock behandelt es ausschließlich über diese Hülle):

- **SearchAlgorithm-Gattung ⟹ `std::map`-Interface-Hülle.** ALLE Suchalgorithmen **leiten die `std::map`-Hülle ab**; das Gattungsinterface bietet **genau das `std::map`-Interface** an (schlüssel-geordnete Map: `insert`/`lookup`/`erase` + geordnete Iteration via `lower_bound`/`begin`/`end`). Das Konformitäts-Gate prüft jedes Lebewesen gegen das `std::map`-Orakel (`conformance_gate.hpp`).
- **Container-Gattung ⟹ `std::vector`-Interface-Hülle** (positions-/index-basiert, schlüssellos).
- **Graph-Gattung ⟹ eigene Graph-Schnittstelle.**

**Konsequenz für #214 (tier_scan, 2026-06-28 umgesetzt):** Der YCSB-E-Range-Scan IST eine native `std::map`-Interface-Operation (`lower_bound(start) → geordnete Iteration`). Der GoF-Iterator (`scan_range` → `Traversal::scan_into`) realisiert damit nichts Fremdes/Aufgesetztes, sondern **genau die geordnete-Iterations-Fähigkeit, die das `std::map`-Gattungsinterface ohnehin trägt** — der Scan ist eine Methode der SearchAlgorithm-Gattung, kein Mess-Apparat. Das `std::map`-Orakel im YCSB-E-Test (`test_v5_ycsb_op_set.cpp`) nutzt exakt `data_.lower_bound(start)` — dieselbe Semantik, an der der neue `scan_range` (`test_v41_scan_range_organ.cpp`) gemessen wird.

---

## §2 Anatomie = Verdrahtung = Feature-Interaktion (der wissenschaftliche Kern)

`04_concept_architecture.tex` Z.239–248:
> „Die … Anatomie ist mehr als die bloße Aufzählung dieser Organe: sie ist ihre **Verdrahtung**. Jedes Organ exportiert ein
> generalisiertes Interface, über das die übrigen Organe seine Dienste nutzen, **ohne dessen konkrete Belegung zu kennen**."

`03_state_of_the_art.tex` §3.6.3 (`ssec:sota-design-contribution`, Z.579–603) erdet das in der **Feature-orientierten
Software-Komposition** (Aßmann „invasive Komposition" TU Dresden; Czarnecki „generative Programmierung"; FOSD-Produktlinien):
> „eine **Achse** ≙ **Feature** · ein **Lebewesen** ≙ **Feature-Komposition** · ein **Lebewesen-Typ** ≙ **Feature-Set-statische-Definition**
> · eine **Gattung** ≙ **(Such-)Algorithmus-Interface-Kategorie** · die **Anatomie** ≙ **Verdrahtung zwischen den Organen = Feature-Interaktion**."

⟹ **Die Achsen-Bibliothek ist eine Feature-Bibliothek; sie wird erst durch die Verdrahtung (Feature-Interaktion) — also
die Metaprogrammierung der jeweiligen Gattung — zur Anatomie.** „Fixer Minimalsatz, erweiterbar" = die Feature-Set-Definition
der Gattungs-Unterklasse, erweiterbar über N-Phasen/Sub-Achsen/Pass-through. **Kein Monolith** — gerade weil die Anatomie die
Verschaltung generischer Organ-Interfaces ist.

**Provenienz** (§3.6.3 Z.598–603): das Achsen-Konzept stammt aus einer Vorarbeit des Autors (Comdare/BEP Venture, UltiHash —
Deduplikation); die Arbeit überträgt es auf cache-bewusste Suchstrukturen.

---

## §3 Observer-Aufbau durch das Prüf-Dock (= Pfad B; die in dieser Session re-gegroundete Aufgabe)

**Prüf-Dock** (`libs/cache_engine/builder/pruef_dock/search_algorithm_dock.hpp`, je Gattung ein Dock): host-seitige Andock-Station,
die EIN Lebewesen (Permutations-DLL) lädt und **nur über die ABI-stabile Gattungs-Schnittstelle** behandelt: **import →
Konformität (`conformance_gate.hpp`, std::map-Orakel) → messen → abstoßen**.

**Hybrid-Messmodell — ZWEI Pfade** (Doc 24 §8; Session `20260530-hybrid-messmodell-erkenntnisse-vollstaendig.md`):
- **Pfad A** — isolierte Achsen-Algorithmen gegeneinander, IN der DLL (`IMeasurableWorkload::run_workload`, synthetischer lbuf). Bleibt.
- **Pfad B** — das GANZE Lebewesen zentral über das Dock: **BEIDE Dimensionen korreliert** — Wall-Clock + Per-Achsen-Observer.

**I1-Konsolidierung (DONE; Doc 31; Sessions `20260604`/`20260605`):**
- **EIN POD** `ComdareTierObserverSnapshot` (`anatomy/observable_tier.hpp:112-129`): `axis_stats[19][8]` + `seg_ns[19]` (Pfad B) +
  Meta; `sizeof==1416`, `static_assert standard_layout + trivially_copyable` (memcpy über DLL-Grenze).
- **EINE Schnittstelle** `IObservableTier::tier_observe(ComdareTierObserverSnapshot*)` (`observable_tier.hpp:153-162`), erbt
  `IDriveableTier`; nur unter `COMDARE_MEASUREMENT_ON` vererbt; Host-Abfrage `dynamic_cast` 1× kalt.
- **Versionierung über ABI-Major** (Loader-Reject). **Offen geblieben (I-C.2/I-D):** Alt-PODs V1/V2/V3 + IObservableTierV2/V3/V4
  löschen + Major 2→3 + DLL-Neubau (atomarer ~15–20-Datei-Block; Plan in `20260605-…`).
- **Q1-Sequenz (bindende Invariante)** in `tier_observe`: **axis_stats-READ → seg_ns-Timing → per-op-Reset** (gegen Doppelzählung).
- **PULL, nicht PUSH** (Soll der Arbeit): das Dock zieht Snapshots; KEIN per-Op-Notify-Callback im Hot-Pfad (= #212-Gegenstand).
- **Aggregation entkoppelt vom Push:** `ObserverAggregate<Composition>` (`anatomy/observer_aggregate.hpp:95-146`) sammelt
  `snapshot_of_t<Axis>` via `a.statistics()` — hält **keine** Observer-Instanzen; `ObservableAxis`-Concept (:41-45) verlangt nur
  `snapshot_t`+`statistics()`.

**Zwei Modi** (`CMakeLists.txt:103-138`): Experiment = `COMDARE_MEASUREMENT_MODE=ON`→`COMDARE_MEASUREMENT_ON=1`+`COMDARE_CE_ENABLE_STATISTICS=1`;
Produktiv = `COMDARE_RELEASE_MODE=ON`→MEASUREMENT_MODE OFF→**STATISTICS OFF** (Observer restlos raus, null Overhead).

---

## §4 Apparat-Reinheit + Meta-Lehre #3 (das „Warum" der A2-Welle)

Diplomarbeit (Impl 5.6): „Per-Node-Zähler verursachen Cache-Line-Ping-Pong, **was genau jene Messung verfälscht, die sie
erheben**" → die Arbeit mitigiert Apparat-Verfälschung bewusst (Leaf-Only-Zähler etc.). Aufgaben-Fairnessregel: „fehlende
Fähigkeiten **als N/A, nicht verdeckt emuliert**" (`aufgabenstellung/de.tex`).

**🎯 Meta-Lehre #3 (mission-kritisch):** Ein **Achsen-Austauschbarkeits-Beleg ist nur gültig, wenn verschiedene Achsen-Wahlen
nachweislich verschiedene Organ-Pfade durchlaufen.** Sonst ist der gemessene Unterschied ein **Apparat-Artefakt**. Der gesamte
Observer-Aufbau durch das Prüf-Dock existiert, um **wissenschaftlich gültige** Per-Achsen-Messung herzustellen (Datengrundlage für
FF1–FF4). Die A2-Apparat-Reinheits-Welle (#211–#217) ist das Fundament dieser Gültigkeit (kein Doppel-Lookup, kein
`std::function`-notify, kein O(n)-Rebuild-Container, kein Phantom-Allocator, echte Such-über-Store-Traversierung).

---

## §5 AUDIT (code-verifiziert 2026-06-28): „Haben wir uns verrannt?"

**Ursprüngliche Aufgabe** = Diplomarbeit-Aufgabenstellung (`thesis/diplomarbeit/aufgabenstellung/de.tex`): Kern = **Trennbarkeit**
der Algorithmus-Bestandteile via **Achsen-Framework (Kap.4) + 3-granulare Mess-Methodik (Kap.6)**; Messung **pro Stelle UND gesamt**,
austauschbar/systematisch/fair; SOTA als explizite Konfigurationen (Teilaufgabe 2: ART/HOT/Masstree/CoCo/START/B²/Wormhole/SuRF);
3 Messreihen A/B/C × 3 Granularitäten (Micro/Macro/Overall).

**Code-Befund (`anatomy/abi_adapter.hpp` `fill_observer_v3`:929; Quellen-Kommentar :922-927; Daten-Pfad :781-838):**
Woraus die Achsen-Werte eines **Baum-Lebewesens (Weg-B)** stammen:
- **T0 search** (:937-947): Weg-A → `container_.statistics()`; **Weg-B (Bäume/Tries/Hash) → `search_organ_.statistics()`** (echtes Lebewesen). ✓ real
- **T4 node_type · T5 memory_layout · T6 allocator · T9 serialization · T11–T16** → **aus dem `container_`-Store** (`store_observe_*`).
- **T1/T2/T3/T7/T8/T17/T18** → **auto-gekoppelte Member-Organe** (`ct_organ_`/`map_organ_`/`pc_organ_`/`pf_organ_`/`cc_organ_`/q1/q2), parallel getrieben (:793-821).
- Daten-Pfad Weg-B (:786, :834-837): jeder Record liegt **doppelt** (echter `search_organ_` + Spiegel-`container_`).

⟹ Für die **SOTA-Bäume (genau Teilaufgabe 2)** beschreiben **T1–T18 (außer T0) einen PARALLELEN Apparat**, nicht die echten
Baum-Organe. Die A2.4/A2.5-Vereinheitlichung (`StoreTraversalAdapter`) wirkt **nur für die Array-Familie** (LinearScan/Interpolation;
`axes/lookup/composable/store_traversable_search_algo.hpp` + `traversal_for_search_algo.hpp`).

**Verdikt: NICHT verrannt — aber der Haupthebel liegt woanders.**
- Richtung korrekt: Apparat-Reinheit + I1 sind das Fundament der fairen Per-Stelle-Messung; die Weg-B-Grenze ist via
  `tier_search_routes_through_store()==false` **ehrlich geflaggt** (Fairnessregel „N/A statt verdeckt emuliert"), nicht versteckt.
- ABER: Mess-Echtheit (Meta-Lehre #3) ist **nur für die triviale Array-Familie** erreicht — **nicht für die SOTA-Bäume**, die das
  Forschungsobjekt sind. Dort messen die Achsen einen Neben-Apparat.
- **Bereits getrackt als Architektur-Fix #188** („T0-Such-Delegation über Speicher-Achsen, Experiment-B+-Baum zentral") = die
  *eigentliche* Befund-2-Vollendung für ALLE Lebewesen. Die Apparat-Reinheits-Welle (#211–217) ist **valide, aber nachgelagert**;
  der große Hebel ist **#188** + der `cowfix-v1`-320-DLL-Neubau (**#215 / A2.8**, der die A2-Fixes erst in die Abgabe-Daten bringt).

---

## §6 Die eng angelegte Aufgabe **#212 — NullNotify** (elaborierte Genau-Beschreibung)

**Audit-Tag:** A2 · K5b/P5/P8 · Disposition **[FIX-E] „compile-time NullNotify (zero-cost)"** · Arch-Anker Apparat-Reinheit Meta #6.

### 6.1 Der exakte Defekt
`MeasurableObserver<Snapshot>` (`libs/cache_engine/src/measurement/measurable_concept.hpp:52-70`) hält
`std::function<void(snapshot_t const&)> callback_` als Member in **jeder** Achse (`mutable observer_t observer_{}`); `notify()` macht
je Op `if (callback_) callback_(snap)`. Im Mess-Build wird **nie ein Subscriber gesetzt** (nur 8 Test-Assertions, s. 6.4) → jedes
`observer_.notify(stats_)` = `std::function`-Indirektion + Branch + 32-B-Member **ohne Wirkung** = Apparat-Overhead auf der
Wall-Clock. Die Stats-Zähler (`++stats_.…`) sind NICHT betroffen — sie speisen den **PULL**-Pfad (`statistics()`); nur der tote
**PUSH** ist Müll. Alles unter `#ifdef COMDARE_CE_ENABLE_STATISTICS`.

Call-Sites (Beispiele, je `observer_.notify(stats_)` in Organ-Ops): `axes/cache_traversal/axis_03b_cache_traversal_*.hpp`
(z.B. `…_linear_fanout.hpp:74/85/98/111`), `axes/mapping/axis_03m_mapping_*.hpp`, `axes/alloc/axis_06_allocator_*.hpp`. Erreicht
werden sie im Mess-Build über die **Auto-Kopplung** des Adapters (tier_insert/lookup treibt die Organe, :793-821).

### 6.2 ⚠️ Reconciliation mit A2.1 (NICHT doppelt fixen!)
**A2.1 (ce `38b1374`, 2026-06-13) hat K5b BEREITS für die zwei mess-kritischen Hüllen** `axes/lookup/composable/observable_composed_search.hpp`
+ `observable_composed_container.hpp` erledigt (Notify **gelöscht**, Pull via `statistics()` erhalten). **#212 = der verbleibende
per-Achsen-Rest** in den Einzel-Organen (cache_traversal/mapping/alloc), den A2.1 NICHT berührt hat.

### 6.3 Gewählter Ansatz (User-Entscheid 2026-06-28): Opt-in-Compile-Flag — Single-Source
Neues `COMDARE_CE_ENABLE_OBSERVER_PUSH`, **Default AUS**, genistet UNTER `COMDARE_CE_ENABLE_STATISTICS`. In `measurable_concept.hpp`:
`std::function callback_` + `on_event` + `has_callback` hinter `#ifdef COMDARE_CE_ENABLE_OBSERVER_PUSH`; im Default-Zweig
`void notify(snapshot_t const&) const noexcept {}` (No-Op, **kein Member**). **Typ `MeasurableObserver` bleibt** → das Concept
`MeasurableComponent` (`measurable_concept.hpp:99-106`, `same_as<observer_t, MeasurableObserver<snapshot_t>>`) **bleibt erfüllt,
KEIN Concept-Edit**, **0 Call-Site-Änderungen** (alle `observer_.notify(stats_)` bleiben, kompilieren zu nichts).
- **Begründung Flag statt Policy-Template:** ein Policy-Template mit nicht-Default-Tag würde den Typ ändern → Concept-`same_as`
  bricht (verifiziert). Der Flag behält Typidentität → saubere, concept-erhaltende Realisierung der „NullNotify-Policy"-Disposition.

### 6.4 Tests (bleiben grün, opt-in)
3 Push-Tests rufen `axis.observer().on_event(...)` + `has_callback()`: `test_v41_topic_allocator_axis_06.cpp` (:253/256/668/689/710/726/728),
`test_v41_topic_traversal.cpp:174`, `test_v41_topic_queuing.cpp:454/469`. Diese Targets erhalten `target_compile_definitions(… PRIVATE
COMDARE_CE_ENABLE_OBSERVER_PUSH)` → unverändert grün. **Getrennte Familie, NICHT anfassen:** das `on_event(event)` der
**concurrency disciplines** (`include/cache_engine/concepts/i_observer.hpp:36`, `…/concurrency_manager.hpp:25`,
`test_concurrency_disciplines.cpp:50`) ist ein ANDERES Interface (nimmt ein Event, keinen Callback).

### 6.5 Blast-Radius: **NULL ABI** (Agent-1 + eigene Lektüre verifiziert)
`MeasurableObserver` ist von `ObserverAggregate` (pull, liest `statistics()`) und der ABI-POD `ComdareTierObserverSnapshot`
(enthält nur uint64/int64, KEINE Achsen-Objekte) **entkoppelt** → **kein Major-Bump, kein erzwungener 320-DLL-Rebuild für
Kompatibilität**, kein Concept-Edit. Nur die interne Achsen-Objektgröße schrumpft (Default: leere Klasse). DLLs profitieren beim
nächsten Neubau (Teil von #215/A2.8).

### 6.6 Verifikations-Plan (Pipeline, NICHT Laptop)
Doppel-Build: (a) Mess-Build/Push-AUS → zero-cost No-Op; (b) Test-Target/Push-AN → 8 Assertions grün. Dann Codex-Review (nur Code-Repo,
Token maskiert) → commit cache-engine **MIT** `Co-Authored-By: Claude Opus 4.8 (1M context)` + Submodul-Bump super → push beide Remotes
(GitLab + GitHub) → grün auf prod-Runner via Pipeline-ID-Poller (Voll-SHA). **Keine Erfolgsmarke ohne literale Tool-Ausgabe.**

### 6.7 Thesis-Konformität + Verhältnis zu #188
Stützt das Pull-Modell (§3.6.3 / Glossar), die Apparat-Reinheit (Impl 5.6), Produktiv-zero-overhead (Conclusion FF2). **Unabhängig
von und kompatibel mit #188** — #212 ist ein kleines, sauberes Apparat-Reinheits-Stück, NICHT der Haupthebel (das ist #188).

---

## §7 Welche Aufgaben brauchen diesen Kontext (VOR Bearbeitung dieses Dossier lesen)

| Task | Inhalt | Bezug zu diesem Dossier |
|---|---|---|
| **#211** | container_→LinearScan/Append (K5c/P4) | Befund-2 §5; gehört in den Kern-Block A2.2/A2.4/A2.5 (Traversal-Vereinheitlichung) |
| **#212** | NullNotify-Rest (K5b) | §6 (elaboriert) |
| **#213** | echter Policy-Allocator (K6/P6) | **A2.3 ✅ bereits exekutiert** (ce `6f719be`) — prüfen, ggf. nur Verifikation/Doku |
| **#214** | tier_scan GoF-Iterator-Organ (K4/P2) | Apparat-Reinheit; Scan-Achsen §3/§5 |
| **#215** | CoW real für 320 (K3) = **cowfix-v1-DLL-Neubau** | A2.8 — macht ALLE A2-Fixes erst in den Abgabe-Daten wirksam |
| **#216** | seg_ns n>1 + stat_*-Reset NACH Load (K9) | Q1-Sequenz §3; Store-Key-Ernte |
| **#217** | uint16→uint64-Entscheid axis_03a-Pilot-Organe (K9) | Key-Raum-Treue |
| **#218–#220** | A1-Rest (Resume-Härte K8 · Pipeline-Integrität · Load/Insert-Key-Räume K7b) | Mess-Validität, nachgelagert |
| **#221–#223** | A3 (RC Null-Object K1 USER · Key-Scrambling K7c · Konformitäts-Gate K9) | Meta-Lehre #3 / Gate §5 |
| **#224** | A4 GoF-Etiketten-Hygiene (K10) | überlappt #179-Sweep |
| **#225** | A5 Second-Execution vs Zwei-Phasen (NUR Diskussion, USER) | §3 Q1/Zwei-Phasen |
| **#226** | Appendix-Daten-Limitierungen ehrlich führen | §5 Weg-B-Limitierung |
| **#188** | **T0-Such-Delegation über Speicher-Achsen (Experiment-B+-Baum zentral)** | **§5 — der eigentliche Befund-2-Kern für die SOTA-Bäume; größter Hebel** |
| #156/#162 | M3-Neumessung + PRT-ART/SOTA-Reihen A/B/C | hängt an #215/A2.8 + #188 |

---

## §8 ALLE Referenz-Doc-Pfade (absolut, Repo-Wurzel `C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken`)

### Diplomarbeit (Soll, Vorrang) — `thesis\diplomarbeit\`
- `aufgabenstellung\de.tex` — die ursprüngliche Aufgabe (Zielsetzung i–iv; 7 Teilaufgaben; Trennbarkeit via Achsen-Framework+3-granular; two-mode).
- `kapitel\de\01_introduction.tex` … `08_conclusion.tex` — FF0–FF4, Grundlagen, SOTA, Konzept, Implementierung, Methodik, Ergebnisse, FF-Antworten.
- `kapitel\de\03_state_of_the_art.tex` — §3.1 Z.27–37 (Gattungs-Kriterium); §3.6.3 `ssec:sota-design-contribution` Z.579–603 (FOSD-Mapping); §3.7 `sec:gap` Z.605–629.
- `kapitel\de\04_concept_architecture.tex` — `ssec:three-levels` Z.228–237; eine-Architektur-Abb. Z.172–203; Anatomie=Verdrahtung Z.239–248; `sec:axes` Z.276–285; `sec:abi` Z.250–272.
- `anhang\de\C_glossary.tex` — Anatomie :18-22 · Drei-Ebenen-Modell :35-38 · Gattung :40-41 · Gattungs-Unterklasse :42-46 · Organ :59-60 · Prüf-Dock :62-63 · Lebewesen :51-52.
- `anhang\de\D_building_block_matrix.tex` — Bausteine-/Achsen-Matrix je Gattung.

### Architektur-Docs — `Code\external\comdare-cache-engine\docs\architecture\`
- `24_messmodell_korrektur_zwei_dimensionen.md` — Hybrid Pfad A/B (§8.1/§8.6/§8.7). **Autoritativ Mess-Modell.**
- `31_observer_interface_konsolidierung_i1.md` — I1 (EIN POD + EINE Schnittstelle + ABI-Major).
- `30_audit_achsen_delegation_pflichtachsen.md` — Befund 2 + 3-Ebenen §8.0/§8.1.
- `34_KONSOLIDIERTER_MASTER_IST_STAND.md` — Doc 34 (Master-IST; §9 Befund-2, §6 Observer, §3 Baum).
- `33_undolog_memento_und_mess_resume.md` — CoW Rev.2 + Resume-Stamp (#215/K3, K8).
- `messarchitektur_design_observer_handle_no_dynamic_cast.md` — „kein per-Op-dynamic_cast".
- `abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz.md` — §5 EIN Mess-POD + ABI-Major + 4 extern-C-Symbole.
- `messarchitektur_v5_design.md` (§2.3 Flags / §7 compile-time-Removal) · `messarchitektur_v5_drei_profile.md` · `messarchitektur_v5_entscheidungen.md`.
- `28_vollstaendigkeits-kartographie.md` (22 Achsen + kV3AxisSchema) · `36_eine_architektur_lebewesen_ist_searchalgorithm.md` (eine Architektur).
- `26_permutations_bplus_baum_und_inverse_signatur.md` · `27_…4_bruecken.md` · `29_…composition_driver.md` · `32_lastprofil_katalog_und_paper_bias.md` · `INDEX.md`.

### Session-/Ledger-Docs — `Code\external\comdare-cache-engine\docs\sessions\`
- `20260529-saeule2-observable-anatomy-context-design-agenten-session.md` — observe_all real für search_algo (Säule 2).
- `20260530-hybrid-messmodell-erkenntnisse-vollstaendig.md` — Pfad A/B formalisiert.
- `20260531-v5-messarchitektur-i1-i10-umsetzung.md` — Dock/ABI-Split/Zwei-Phasen/Konformitäts-Gate/ABI-Major 2.
- `20260604-observer-konsolidierung-und-mess-echtheit.md` + `20260605-UEBERGABE-START-HIER-observer-konsolidierung.md` — I1 (EIN POD/EINE tier_observe/Q1) + I-C.2/I-D-Restplan.
- `20260613-A2-code-pre-read-notizen.md` — As-built ⇄ Doc verifiziert; Befund-2 lokalisiert.
- `20260613-E-wellea2-befund2-q2schritt4-komplexplan.md` — **A2.1–A2.8 Komplexplan + §9–§11 Exekutions-Stand (A2.1/A2.3/A2.4/A2.5 done)**.
- `20260613-D-audit-85-befunde-durcharbeitung.md` — Disposition K1–K10 + #211–#226-Mapping.
- `20260613-A3-audit-soll-abgleich.md` · `20260618-PHASE-E-VERTIEFUNG-AUDIT-NEUMESSUNG-MASTERPLAN.md` (A1–A5-Priorisierung).
- **dieses Dossier:** `20260628-KONTEXT-DOSSIER-mess-echtheit-gattungen-observer-pruefdock-A2welle.md`.

### Schlüssel-Code — `Code\external\comdare-cache-engine\libs\cache_engine\`
- `src\measurement\measurable_concept.hpp` — `MeasurableObserver` :52-70 (**#212**); Concept `MeasurableComponent` :99-106; NULL-Variante :110-125.
- `anatomy\observable_tier.hpp` — `kV3AxisSchema` :66-86; `ComdareTierObserverSnapshot` :112-129 (sizeof 1416); `IObservableTier` :153-162; `IMigratableTier` :178-186.
- `anatomy\observer_aggregate.hpp` — `ObservableAxis` :41-45; `snapshot_of_t` :48-65; `ObserverAggregate` :95-146.
- `anatomy\abi_adapter.hpp` — Vererbung :116-121; Pfad-A run_workload :234; tier_insert :696; Weg-A/B lookup :781-789; Auto-Kopplung :793-821; tier_erase :826-838; tier_clear :841-906; `fill_observer_v3` :929 (Quellen :922-927; T0 :937-947); `fill_segment_timing_v3` :990; `tier_observe`/Q1 ~:1143-1151; `search_organ_` :1296/:1311; `container_` :1302-1305; CoW :1158-1203.
- `anatomy\search_algorithm_anatomy.hpp` — `observe_all` :62-112 (9 reale Achsen-Organe); `observer_aggregate_t` :37.
- `anatomy\composition_factory.hpp` — `AdHocComposition<T0..T18>` 19 Slots.
- `axes\lookup\composable\store_traversable_search_algo.hpp` + `traversal_for_search_algo.hpp` — A2.4 (Weg-A-Klassifikation/Mapping).
- `axes\lookup\composable\observable_composed_search.hpp` + `observable_composed_container.hpp` — die A2.1-Hüllen (Notify bereits entfernt).
  ⚠️ **Duplikat-Befund (2026-06-28):** dieselben zwei Hüllen liegen AUCH unter `topics\traversal\axis_03a_search_algo\composable\` —
  #212 muss zuerst klären, welche Kopie der reale Build (`abi_adapter.hpp`-Includes / `container_t`/`search_organ_`) zieht
  (axis-zentrische Restrukturierung V41.F.2 → `axes\lookup\` ist vermutlich kanonisch; `topics\…` evtl. Alt-Stand). Beide prüfen.
- `builder\pruef_dock\search_algorithm_dock.hpp` (+ `adapter_dock`/`set_dock`/`sequence_dock`/`view_dock` je Gattung) · `conformance_gate.hpp` (std::map-Orakel).
- `builder\experiment_tree\perm_runner.hpp` (tier_observe→POD→CSV) · `cache_engine_builder_iterator.hpp` (Resume/Stamp).
- `CMakeLists.txt` :112 MEASUREMENT_MODE · :118 MEASUREMENT_ON · :113 RELEASE_MODE · :133-138 STATISTICS-option (**#212-Flag hier nisten**).
- Per-Achsen-Notify-Call-Sites: `axes\cache_traversal\axis_03b_cache_traversal_*.hpp` · `axes\mapping\axis_03m_mapping_*.hpp` · `axes\alloc\axis_06_allocator_*.hpp`.
- **`modules\*` = TOTE Spiegel — NIE anfassen** (realer Build inkludiert NUR `libs\cache_engine`).

### Memory-Direktiven (Host `…\.claude\projects\C--WINDOWS-system32\memory\`)
- `feedback_one_consistent_observer_interface_pruefdock` · `feedback_zwei_dimensionen_messmodell` · `project_observer_konsolidierung_resume_ic2`
- `feedback_always_use_trees_for_search` · `feedback_infra_cleanest_not_easiest` · `feedback_no_success_marks_without_literal_output` · `feedback_codex_mcp_review_before_code_complete`.

### Plan-Datei (Host, nicht im Repo)
- `C:\Users\benja\.claude\plans\dynamic-frolicking-truffle.md` — Pfad-B-Timing + layout-honorierender Store + Observer-Konsolidierung (S2.5).

---

## §9 Invarianten (must_not_break)

1. **modules/* = tote Spiegel** — nie anfassen; realer Build = nur `libs/cache_engine`.
2. **Q1-Sequenz** in `tier_observe`: axis_stats-READ → seg_ns-Timing → per-op-Reset (sonst Doppelzählung T0/T1/T2/T3/T7/T8/T10/T17/T18).
3. **Pfad A bleibt** (`IMeasurableWorkload`/`run_workload` + `ComdareSegmentLatencyV2`) — isolierter Achsen-Bench; `seg_ns` im POD = **Pfad B**.
4. **kV3AxisSchema** = Single-Source Schreiber↔CSV-Spaltenname (Namen kV3* bewusst behalten); **Honest-0** (Baseline echte 0, kein „Fehler").
5. **ABI-POD** `is_standard_layout && is_trivially_copyable` (memcpy über DLL-Grenze).
6. **Wire-Format-Symmetrie** `format_perm_result`↔`ingest_result_line` (175 Felder) synchron (test_d14b/d14c).
7. **Capability nie still degradieren** (Meta #1/#2): `static_assert` über die ZIEL-Population (die 320), nicht nur Referenz-Komp.
8. **Mess-Echtheit ehrlich** (Meta #3 + Fairnessregel): nicht-store-geroutete Lebewesen als solche ausweisen (`tier_search_routes_through_store()`), nie verdeckt.
9. **Keine Erfolgsmarke ohne literale Tool-Ausgabe**; Verifikation NUR auf der Pipeline (Laptop zu langsam).

---

## §10 Die Array-Achse über ZWEI Gattungen — #217 ist kein Key-Width-Fix (User-Hinweis 2026-06-28)

**User-Hinweis:** „ein array ist möglicherweise ein Container, kann aber auch für den Fanout in einem Baum
konfiguriert werden — die Trennung oder gemeinsame Nutzung zweier Gattungen derselben Achse ist hier
notwendig." Untersuchung bestätigt:

- **`Array65535SearchAlgo`** (`axes/lookup/axis_03a_search_algo_array65535.hpp`) = „**ART-artige direkte
  Adressierung**", `key_type = std::uint16_t` = **Fanout-Diskriminator** (2-Byte-Radix-Chunk), `kCapacity=65536`,
  `max_fanout()=65536`. Analog `Array256SearchAlgo` (1-Byte). Das ist **kein Container-Key**, sondern der
  Radix-Index eines (Ein-Knoten-)ART-Fanouts.
- Dieselbe Array-Fanout-Struktur existiert in **zwei Achsen/Rollen**: (1) **search_algo (T0)** als standalone
  „Ein-Knoten-ART" (Array256/Array65535) **und** (2) **node_type** als echte ART-Fanout-Knoten
  (`axis_04_node_type_node256/48/16/4.hpp`).

**Konsequenz für #217:** „uint16→uint64" ist KEIN mechanischer Key-Width-Fix (ein 2^64-Fanout ist absurd).
Die Trunkierung >65535 tritt NUR auf, wenn das Array **als standalone search_algo mit dem vollen Key** getrieben
wird; **als Baum-Fanout-Knoten** sieht es nur einen Radix-Chunk (voller uint64-Key über den Baum-Pfad → keine
Trunkierung). Die echte Frage = **Trennung vs. gemeinsame Nutzung der Array-(Fanout-)Achse über die
Container-Gattung (array-als-Speicher) und die SearchAlgorithm-Gattung (array-als-Baum-Fanout)** — entangelt mit
der **node_type-Achse** + **#188** (Such-über-Store). ⟹ **#217 = USER-Entscheid, nicht autonom.** Bis dahin:
Array256/Array65535 ehrlich als Narrow-Diskriminator-Pilots mit Range-Limit führen ([LIMIT], Goal §2.5-b).

**RESOLUTION (User-Entscheid 2026-06-28) — #217 GEKLÄRT, KEIN uint16→uint64:** Ein **Array ist grundsätzlich ein
Container** (Container-Gattung). Jede Achse hat ein **einheitliches, zur Compile-Zeit per Metaprogrammierung
gebautes Interface** = gekapseltes Modul ihrer Ebene, das seine Dienste der Implementierung **anderer** Achsen
anbietet. Es kann eine **Container-Achse** geben, die als „Algorithmus" verschiedene Container-Algorithmen (das
Array über die Container-Gattung) unter der Haube aufruft; **diese Achse wird dann in den Fanout der Bäume
(node_type) integriert**, sofern der Originalalgorithmus es bestimmt = **Metaprogrammierung-Rekursion** (eine
Gattung rekursiv als Sub-Komponente einer anderen über das uniforme Achsen-Interface). ⟹ Das Array steht damit
**Fanout + standalone-Container + search_algo** zur Verfügung — je Konfiguration, **OHNE Key-Width-Änderung**
(`uint16` = Fanout-Diskriminator/Container-Index, korrekt). Die Trunkierung existiert NUR im falschen Framing
(array-als-standalone-search_algo mit vollem Key); im korrekten Modell (Container-Achse → ggf. Baum-Fanout)
nicht. **Entscheid steht** (array-is-Container + Metaprog-Rekursion); die volle **Container-Achsen→Fanout-
Rekursions-Implementierung** ist Architektur-Arbeit mit der node_type-Achse (verwandt mit #188). Bis dahin
standalone-getriebene Array256/65535 als Narrow-Diskriminator-Pilots ([LIMIT]).

## §11 Reconciliation-Befund: der #211–#226-NACHTRAG ist teils STALE (2026-06-28)

Beim Abarbeiten code-verifiziert: **#220 (K7b Load/Insert-Key-Räume) + #222 (K7c Zipfian-Scrambling) sind
BEREITS ERLEDIGT** (`workload_generator.cpp` splitmix64_scramble_ :126/:140 + neue-Key-Inserts :172-177) — via
einer Audit-Restwelle vor dieser Session, OBWOHL die durcharbeitung-Disposition sie „offen" listet. ⟹ Die
durcharbeitung (und damit der #211–#226-NACHTRAG vom 27.06.) ist **unzuverlässig/stale**; mehrere „offene"
Befunde sind im Code schon gefixt. **Direktive für künftige A2/A3-Arbeit:** JEDEN #21x-Task ZUERST gegen den
realen Code verifizieren (nicht der Disposition vertrauen), sonst „verrennt" man sich an bereits Erledigtem.
**Verbleibende ECHTE Arbeit konzentriert sich auf:** #188 (Befund-2-Kern für SOTA-Bäume) + #215/A2.8
(cowfix-v1-320-DLL-Neubau, macht alles wirksam) + USER-Entscheide (#217 Array-Gattung · #221 RC · #225
Second-Execution) + Doku (#226 Appendix-LIMIT). Die übrigen sind großteils #188-entangelt oder bereits erledigt.

## §12 Der Permutations-B+-Baum (Experiment-Manager) + die zwei Knotenarten — RC korrekt eingeordnet (User-Korrektur 2026-06-28)

Quelle (autoritativ): `docs/architecture/26_permutations_bplus_baum_und_inverse_signatur.md` + `builder/experiment_tree/` (KF-9, `experiment_tree.hpp`) + `runtime_variable_loop.hpp` (KF-7) + `cache_engine_builder_iterator.hpp` (3 Iteratoren).

**EIN B+-Baum = das GESAMTE Experiment** (alle Paper + alle Permutationen). Jede Baumebene = Entscheidung über den Algorithmus EINER Achse; Fanout je Knoten = explorierte Optionen (Achse gepinnt→Fanout 1, freigegeben→Fanout N). Jede node hält key (serialisierte Pfad-Signatur Wurzel→Knoten) + value (Observer-Statistics, aggregiert über den Teilbaum) → auf JEDER Ebene lesbarer Ergebnis-Speicher (die Diplomarbeit liest read-only per Traversal, beliebige Granularität). Paper-Wiedererkennung = **statische Signatur** (Array der gepinnten Achsen-Werte), NICHT Hash; `multimap<Signatur,Paper>` für Doppelerkennung. `binary_count` = ∏ der statischen Ebenen-Größen REIN ARITHMETISCH (nie eager materialisiert → OOM-Schutz; lazy mixed-radix-Odometer, EIN Pfad Wurzel→Blatt zur Zeit; Build = nur K Pfade parallel).

**ZWEI Knotenarten (Factory-Method, Doc 26 §4) — die Kern-Trennung, die ich falsch hatte:**
- **`StaticAxisNode` = compile-time → je distinkter Static-Pfad eine EIGENE, statisch gebaute Tier-Binary (DLL).** Die compile-time-Achsen-Variationen (search_algo/node_type/memory_layout/cacheline-size/alignment/sw_hint/…) werden als **ZUSÄTZLICHE Tier-Binary-Permutationen STATISCH gebaut und EINZELN über das Prüf-Dock gezogen** — der CacheEngineBuilder **wechselt die GANZE Binary zur Laufzeit der Messung** (DLL A laden → messen → entladen → DLL B …). Das ist die Trennung CacheEngineBuilder ⇄ Tier-Binary: ein Binary-Wechsel, KEINE Laufzeit-Umschaltung im Inneren.
- **`DynamicVariableNode` = Laufzeit → eine FOR-SCHLEIFE auf der BEREITS GELADENEN Binary** (`RuntimeVariableLoop`, KF-7), die echt-laufzeit-einstellbare Variablen über `Algorithm_Resource_Control` (= RC) durchprobiert — **erzeugt KEINE neue Binary**. Doc 26 §4: nur echt laufzeit-einstellbare Größen (thread_count, hw_prefetcher) sind DynamicVariableNodes; compile-time-Größen (auch cacheline-size/alignment) sind StaticAxisNodes (= Binaries).
- **Blatt = EIN Mess-Lauf = (Binary × eine Laufzeit-Einstellung).** Der serialisierte Pfad Wurzel→Blatt ersetzt den FNV1a-Fingerprint als eindeutige ID.

**⟹ RC ist KEIN Widerspruch zum compile-time-Achsen-Modell — RC IST die DynamicVariableNode-Ebene des B+-Baums** (die komplementäre 2. Dimension): compile-time-Strukturen = StaticAxisNode = separate Binaries; laufzeit-Variablen = DynamicVariableNode = RC-Schleife auf der geladenen Binary. (Korrigiert meine frühere Fehl-Rahmung „RC ⊥ compile-time, daher raus" — das war FALSCH.)

**Was ist RC?** RC = **Resource Control** = `Algorithm_Resource_Control` (KF-4) = die ABI-stabile **Laufzeit-Steuer-Schnittstelle** `IResourceControllableTier` (`tier_query_resource_caps` + `tier_apply_resource_control`) + flacher POD `ComdareResourceControlV1` (5 Felder, je 1 Achse: thread_count→concurrency, prefetch_distance→prefetch, pool_budget_bytes→allocator, batch_size→traversal, inline_threshold_bytes→value_handle). IMMER einkompiliert (auch Messung-aus), weil sie algorithmus-INTERNE Properties steuert (nicht die Messung). Der RuntimeVariableLoop (Builder-Seite, `runtime_variable_loop.hpp`) ist KORREKT: er expandiert die virtuelle Kartesik der `DynamicDim`, baut je Kombination den `req`-POD (`set_field` mappt var→Feld), klammert via `clamp(req, caps, env)` und ruft `tier_apply_resource_control` auf der geladenen Binary, dann den Mess-Visitor.

**Der #221-Defekt im KORREKTEN Rahmen:** Die DynamicVariableNode-Ebene ist **Builder-seitig vollständig**, aber **Tier-seitig ein Null-Object**: `abi_adapter::tier_apply_resource_control` (:212-225) klammert die Werte und schreibt sie nur in `applied_rc_` (:1729) — und **kein Organ liest `applied_rc_` je** (Grep eindeutig). Folge: alle DynamicVariableNode-Settings messen DASSELBE → die gesamte dynamische Dimension des B+-Baums = **Phantom-Mess-Zeilen** (Meta-Lehre #3). `tier_query_resource_caps` behauptet dennoch 5 steuerbare Achsen (caps 64/64/1GiB/4096/256). Der CacheLinePolicySelector (#174/Versprechen #172.2) ist dadurch ebenfalls wirkungslos. **#221 = USER-Entscheid:** die DynamicVariableNode-Dimension Tier-seitig ECHT verdrahten (vollenden) ODER vorerst stilllegen+[LIMIT] — pro RC-Feld korrekt klassifiziert als DynamicVariableNode (echt laufzeit) vs StaticAxisNode (compile-time-Binary) vs Workload-Dim.

## §13 Die VIER Ebenen des Experiments — XML-Definition (oben) ⟶ B+-Baum/Gattung ⟶ Tier-Binaries ⟶ RC-Laufzeit (User-Ergänzung 2026-06-28)

Zwei Präzisierungen zu §12: (1) Der Permutations-B+-Baum ist die **Gesamtmaschinerie ALLER permutierbaren compile-time UND runtime Achsen EINER Gattung** und **pro Gattung** zu implementieren (**Abstract Factory je Gattung**: SearchAlgorithm/Container/Graph permutieren je ihren eigenen inneren Achsen-Algorithmus-Satz zu je eigenen statisch kompilierten Tier-Binaries). (2) Darüber sitzt eine **vierte, definitorische Ebene**: die XML-Experiment-Konfiguration — das **eigentliche Experiment** + dessen programmatische Definition leben in der **XML, NICHT im Baum** (der Baum ist nur die Maschinerie, die die XML-Definition in Binaries+RC-Läufe übersetzt).

| # | Ebene | Was | Artefakt / Code |
|---|---|---|---|
| **4** | **XML-Experiment-Definition** (oberste, Definitionsebene jedes Laufes) | DAS EXPERIMENT — programmatisch deklariert: `base_tiers` (Paper-Tupel), `permute_axes` (compile-time frei), `runtime_dynamic` (Laufzeit), `compile_dims`/workloads, `modes` (3 Joins), `repetitions`, sweeps. **Alle Bestandteile dynamisch einstellbar.** | `comdare_thesis_profile` (`algorithm_profiles/thesis_profiles/*.profile.xml` + `SCHEMA.md`). Im **Superprojekt (Diplomarbeit)** angesteuert/definiert = das WAS; cache-engine = das WIE. *(Ist-Delta: Profile liegen aktuell in der Bibliothek `algorithm_profiles/`, konzeptionell gehören sie ins Superprojekt.)* |
| **3** | **Permutations-B+-Baum (PRO GATTUNG, Abstract Factory)** | interpretiert die XML → AxisLevels → EIN B+-Baum, der compile-time- (StaticAxisNode) UND runtime-Achsen (DynamicVariableNode) **verwebt**. | `profile_to_tree.hpp::build_axis_levels` (tier→base_tiers, permute_axes/cacheline→statisch, runtime_dynamic→dynamisch) + `experiment_tree.hpp` (KF-9). |
| **2** | **StaticAxisNode → Tier-Binaries** (statische compile-Ebene, primär) | je distinkter Static-Pfad → eine eigene statisch kompilierte DLL; CacheEngineBuilder baut sie (CEB-Gen KF-8 → `perm_<id>.cpp`→SHARED-DLL) + zieht sie EINZELN über das Prüf-Dock (Binary-Wechsel zur Mess-Laufzeit). | `cache_engine_builder_iterator.hpp` (statischer + dyn-filter-Iterator). |
| **1** | **DynamicVariableNode → RC** (dynamische Config je Binary, mit Ebene 2 verwoben) | FOR-Schleife auf der GELADENEN Binary; `runtime_dynamic`-Settings via `Algorithm_Resource_Control` (= RC) OHNE Neu-Bauen. | `runtime_variable_loop.hpp` (KF-7) + `IResourceControllableTier`. |

**Gesamt-Ablauf (CacheEngineBuilder = Orchestrator von Build UND Messung), User-verbatim 2026-06-28:**
`compile` → `XML-Experiment-Definition-Interpretation` (4→3) → `Tier-Binaries` (3→2) → `Tier-Binary-dynamische-Konfiguration` (2→1, RC) → `CacheEngineBuilder-Messauswertung-und-Analyse` (Observer-Pull + inverse Signatur-Projektion KF-15 → Thesis-Anbindung KF-14). **Jede (Binary × Laufzeit-Einstellung × Wiederholung) wird EINZELN durchgemessen, ausgewertet und laut Kontext der Diplomarbeit behandelt** (read-only Baum-Traversal, Paper-Signatur-Filter).

**Schärfung für RC/#221 (aus `SCHEMA.md` §Faustregel + `runtime_variable_loop.hpp`):** Die `runtime_dynamic`-Ebene (= RC, Ebene 1) ist im XML-Schema explizit für **OS-seitige, NICHT-architektonische** Größen vorgesehen — `thread_count` (OS-Thread-Pool) + `hw_prefetcher` (MSR 0x1A4, SLURM/MSR-Launcher KF-12, **gar kein POD-Feld** → Cluster-gated). Die algorithmus-internen POD-Felder (prefetch_distance/pool_budget/batch_size/inline_threshold) sind der **Graubereich**: KF-4 deklariert sie als RC-steuerbar, aber der Mess-Pfad liest `applied_rc_` nie (§12-Null-Object), und das Schema klassifiziert sie eher als compile-time (StaticAxisNode) denn als OS-Laufzeit. ⟹ #221 = pro RC-Feld entscheiden: echte OS-Laufzeit-Dynamik (Ebene 1, via OS/MSR, teils Cluster-gated) · algorithmus-intern→Organ-Hook (Ebene-1-Verdrahtung) · oder compile-time→StaticAxisNode (Ebene 2, gehört NICHT in RC).

## §14 AUDIT-SEKTION: Top-Down-Durchgang Ebene 4→1 — „Kann ein Diplomarbeit-Experiment definiert, gebaut, gemessen, ausgewertet werden?" (laufend, ab 2026-06-28)

**Auftrag (User 2026-06-28):** **VOLLENDUNG** der DynamicVariableNode/RC-Dimension (nicht raus/stilllegen). **Strategie:** top-down Ebene 4→1 durch den Code; je Ebene die Funktionalität gegen die Diplomarbeit-Anforderungen prüfen; offene Aufgaben je Ebene identifizieren + Fixes **direkt** mit durchführen. **ZIEL-Invariante:** *die Diplomarbeit ändert NUR die XML (Ebene 4) — die Mess-Pipeline (CacheEngineBuilder) übernimmt den Rest vollautomatisch* (compile → XML-Interpret → Tier-Binaries → RC-Config → Messauswertung). Tracking: Task **#229** (Strecke) + **#221** (RC-Kern). Pipeline-Mess-Verifikation #210-gated; Code+Test pipeline-unabhängig.

**Prüf-Frage je Ebene:** „Lässt sich ein laut Diplomarbeit (Aufgabenstellung + Kap.6-Methodik: 3 Messreihen A/B/C × 3 Granularitäten Micro/Macro/Overall · SOTA als Konfigurationen · austauschbar/systematisch/fair · pro-Stelle UND gesamt) gefordertes Experiment auf dieser Ebene vollständig + korrekt abbilden — und reicht die Ebene ihre Definition VERLUSTFREI an die nächste weiter?"

**User-Präzisierungen, die den Soll-Zustand binden:**
- Die 4 algorithmus-internen RC-POD-Felder (prefetch_distance/pool_budget/batch_size/inline_threshold) **dürfen NICHT tot sein** — sie müssen laut ihren zugehörigen Achsen für das Experiment **in Ebene 2 verwendet** werden (die Achse konsumiert den Wert), RC (Ebene 1) variiert ihn zur Laufzeit.
- **thread_count** erstmal optional → später als dynamische Subachse unter concurrency in Ebene 2 variiert.
- Ebene-1-Aktivierung = **KEIN großer Umbau** (Vorbereitungen fertig verbaut, liefen letzte Woche) → erst prüfen, was verbaut ist, dann verdrahten.
- Der Permutations-B+-Baum ist **pro Gattung als Abstract Factory** zu führen.

### Ebenen-Status (top-down gefüllt)
| Ebene | Prüfgegenstand | Status | Kern-Befunde / Fixes |
|---|---|---|---|
| **4 XML-Definition + Auswertung** | Schema/Parser (Eingang) + Auswertungs-Kette CSV→LaTeX (Ausgang) vs. Diplomarbeit-Methodik | 🟢 Tools fertig / 🟡 A/B/C-Daten fehlen | B4-1: 4 POD-Felder nicht XML-definierbar (E2-abh.) · B4-2: Auswertungs-Kette komplett+reproduzierbar, aber 3 Messreihen A/B/C parametrisch übersprungen bis #156/#215 |
| **3 B+-Baum / Gattung** | `profile_to_tree` + `experiment_tree`; Verwebung; Abstract Factory pro Gattung | 🟡 SA verifiziert / Gate-Lücke | B3-1: pro-Gattung via `GenusBindingTraits<G>` (SA grün, Container-Unterkl. da, Graph offen) · B3-2: Konformitäts-Gate #223 NICHT im Voll-Lauf verdrahtet |
| **2 StaticAxisNode → Tier-Binaries** | Achsen-Organe konsumieren Parameter; #188-Wurzel | 🟡 compile-time läuft / Laufzeit-Konsum-Lücke | B2-1: #188 container_/search_organ_-Spaltung (Weg-A gelöst, Weg-B = Spiegel-Apparat) · B2-2: Cache-Line compile-time eingewoben, aber KEINE Achsen-RC-Setter (E1-Konsum fehlt) |
| **1 DynamicVariableNode → RC** | `RuntimeVariableLoop` + `tier_apply` → Organ-Konsum; Mess-Durchführung | 🟡 Infra fertig / Achsen-Konsum fehlt | B1-1: RC-Infra (Builder+ABI+clamp+caps+Cache-Line-compile-time) fertig; verbleibend = Achsen-Laufzeit-Setter + apply1-Verdrahtung (#221, überschaubar) |

### Befund-/Fix-Log (chronologisch, je Ebene — wird beim Durchgehen fortgeschrieben)

**B4-1 (Ebene 4, 2026-06-28) — runtime_dynamic-Abdeckung.** Verlustfreiheit für die GENUTZTEN Dims bestätigt: `build_axis_levels` (`profile_to_tree.hpp:74-85`) emittiert `concurrency.thread_count`, `prefetch.hw_prefetcher`, `repetition.repetition_index` als dynamische Ebenen (`is_static=false`; `set_field` mappt thread_count auf das POD-Feld, hw_prefetcher/repetition_index sind architektonische Ausnahmen ohne POD-Feld). ABER: `ThesisProfile` trägt für runtime_dynamic NUR `thread_counts` + `hw_prefetcher`; die 4 algorithmus-internen RC-POD-Felder (prefetch_distance/pool_budget/batch_size/inline_threshold) haben **keinen XML-Eingang, kein ThesisProfile-Feld, keinen SCHEMA-Eintrag, kein build_axis_levels-Emit** → aktuell **gar nicht XML-definierbar** (verletzt die ZIEL-Invariante „nur XML ändern"). Der `RuntimeVariableLoop::set_field` (runtime_variable_loop.hpp:53-60) KÖNNTE sie zwar setzen (mappt alle 5) — es fehlt also nur die Ebene-4-Eingangsseite. **FIX-Richtung (entscheidet sich an Ebene 2):** die 4 Felder als Achsen-Parameter (Ebene 2, analog cacheline-Sub-Achsen in `permute_axes`) XML-definierbar machen — der User verortet sie in Ebene 2 (die Achse konsumiert den Wert), RC variiert ihn zur Laufzeit. Konkreter Fix nach Ebene-2-Prüfung (Frage: tragen prefetch/allocator/traversal/value_handle überhaupt diese Parameter-Slots?). **Status: identifiziert, Fix Ebene-2-abhängig — NICHT isoliert in Ebene 4 fixbar (top-down korrekt).**

**B4-2 (Ebene 4, 2026-06-28) — Messwert-Extraktions-/Auswertungs-Kette im Superprojekt (der „blinde Fleck", jetzt auditiert).** Die XML ist nur der EINGANG von E4; der AUSGANG ist eine vollständige, eigenständige Kette: Mess-Lauf → **WIDE-CSV** (`;`-getrennt, ~120.960 Z., Default `Messdaten-Backup/tier150_measurements_INDEX320_cowfix-v1_2026-06-18.csv`) → **zwei C++-CLI-Tools im Code-Repo** `csv-to-latex.exe` (`Code/.../04_csv_to_latex`) + `diagram-generator.exe` (`05_diagram_generator`) → **Superprojekt-Orchestrator** `thesis/diplomarbeit/generate_wide_appendix.ps1` → `anhang/{de,en}/tabellen/*.tex` (**11 Tabellen**: `bias_matrix_table` + 6× `lc_surface_<z>` [ns_per_op, op_{insert,lookup,erase,scan,rmw}_p50_ns Heatmaps] + 4× `ld_exchange_<achse>` + `le_limitierung`) → `A_measurements.tex` → `build.ps1 -Lang {de,en}` → bilinguale Abgabe-PDF. **Eigenschaften (gut):** byte-identisch reproduzierbar (`git diff --stat anhang/` = 0), Stale-Binary-Schutz (le_limitierung wird NICHT verkürzt, wenn die Exe älter ist), m3v2-Sektionen werden **parametrisch übersprungen statt erfunden** (Ehrlichkeit).
**KERN-LÜCKE:** die **3 Messreihen A/B/C** (= Diplomarbeit-Kern FF3 + Kap.6-Methodik) + Working-Set-Sweep + seg_coverage sind in der aktuellen cowfix-v1-Auswertung **parametrisch übersprungen** — cowfix-v1 trägt die Spalten `series`/`working_set_n`/`sweep_axis`/`seg_coverage` NICHT. **Die Tool-Seite ist FERTIG** (CLI-Modi `--sota-series`/`--sweep-axis`/`--seg-coverage`/`--sweep-curve` in `main_cli.cpp` vorhanden; `-M3v2`-Flag im Orchestrator). Es fehlt NUR die **m3v2-Matrix** (#156), die die A/B/C-Spalten trägt — und die hängt an #162 (SOTA-Reihen-Def) + #215 (A2-Fix-DLL-Neubau, sonst basieren die Daten auf dem cowfix-v1-Stand vom 18.06. OHNE die A2-Fixes #212/#214/…) + #210 (Runner).
**B3-1 (Ebene 3, 2026-06-28) — Abstract Factory pro Gattung.** Der Baum-KERN (`experiment_tree.hpp`) ist gattungs-AGNOSTISCH (AxisLevel/StaticAxisNode tragen beliebige Achsen); die `AbstractNodeFactory`/`ExperimentNodeFactory` (GoF-Factory-Method `make_static`/`make_dynamic`) ist für die KNOTEN-ARTEN, NICHT pro-Gattung. Die **pro-Gattung-Implementierung** ist als **`GenusBindingTraits<G>`-Template-Spezialisierung** (`genus_binding_traits.hpp`) realisiert: jede Gattung deklariert Slot-Zahl + Achsen-Namen + Composition→Anatomie-Bindung (= was eine Permutation in eine baubare Tier-Binary verwandelt). Stand: **SearchAlgorithm = verifizierter Spezialfall** (19 Slots, alle 4 Brücken grün — die Diplomarbeit-Fokus-Gattung); Container-Tier-Unterklassen Adapter/Set/Sequence/View spezialisiert (#76); **Graph offen** (sobald Komposition/Anatomie existiert). „Cross-Genus type-unmöglich" → bewusst GETRENNTE Traits. ⟹ E3-pro-Gattung **funktional korrekt für SearchAlgorithm**; deckt den User-Soll „jede Gattung eigene Bau-Implementierung" ab — eleganter als 3 Baum-Kopien (1 Kern + pro-Gattung-Traits). Verlustfreiheit XML→AxisLevel→`static_binary_view` (Binaries) + `dynamic_filter` (RC-Schleife) verdrahtet.

**B3-2 (Ebene 3, 2026-06-28) — Konformitäts-Gate NICHT im Voll-Lauf-Pfad (#223).** `conformance_gate.hpp` (std::map-Orakel, A1-Gattungs-Interface) EXISTIERT im Prüf-Dock. ABER die EINE Voll-Lauf-Treiber-Funktion `run_lazy_static_then_dynamic` (`cache_engine_builder_iterator.hpp`) ruft es NICHT — sie macht: statische Kompilierung → DLL-load → IObservableTier/IResourceControllableTier → messen (kein GATE dazwischen; die „gate"-Treffer dort sind „gate-frei"/RAM-gated/quality-flag, NICHT das Konformitäts-Gate). Der Soll-Pfad **import→GATE→messen** (jedes Lebewesen vor der Messung gegen das std::map-Orakel verifizieren) fehlt → ein nicht-konformes Lebewesen würde gemessen, ohne Korrektheits-Prüfung = Mess-Validitäts-Loch. **#223-Befund bestätigt** (E3/E2): Gate in den Voll-Lauf-Pfad verdrahten + SelectMode=search_algo_grid. **Pipeline-unabhängig fixbar** (Host-Logik + Test). Querbezug: passt zur Fairnessregel/Meta-Lehre #3 (nur konform-verifizierte Lebewesen liefern gültige Mess-Daten).

**Strategische E4-Erkenntnis:** **E4 ist NICHT der Engpass** — Eingang (XML) + Ausgang (Auswertungs-Tools) sind bereit; die Lücke ist die DATEN-Erzeugung über die tieferen Ebenen. Der reale Arbeits-Schwerpunkt liegt in **E2** (#188/#211/#213/#214/#216/#217-Achsen-Fixes) + **E1** (#221-RC) + **#215** (Neubau-Schleuse) + **#210** (Runner-Infra). Das bestätigt die Top-Down-Fix-Reihenfolge E4→E3→E2→E1, aber mit dem Befund, dass E4 selbst (Tooling) weitgehend grün ist; die A/B/C-Reihen-Aktivierung (`-M3v2`) ist ein kleiner E4-Schritt, sobald die m3v2-Matrix existiert.

**B2-1 (Ebene 2, 2026-06-28) — #188 = die E2-Wurzel (der Mess-Echtheits-Hebel).** Doc 30 Q2: die `container_`/`search_organ_`-Doppel-Speicher-Spaltung ist „ein Bug, kein Geschmack". **Weg-A** (Array-Familie, StoreTraversable): `container_` trägt das echte Traversal-Organ → **gelöst** (A2.4/A2.5). **Weg-B** (SOTA-Bäume/Tries/Hash): `search_organ_` = echtes Baum-Organ (T0), `container_` = `SortedBinary`-Spiegel; die Speicher-Achsen (node/layout/alloc/T1–T18) messen über den `container_`-Spiegel = **paralleler Apparat** → Meta-Lehre #3 verletzt für genau die SOTA-Bäume (das Forschungsobjekt). **#188** = `search_organ_`-Entfall, `container_` trägt `Composition::search_algo` für ALLE Familien → der eigentliche Mess-Echtheits-Hebel. Verwoben: #211 (container_-O(n)-Rebuild raus), #214 (scan, code-complete), #216 (seg_ns), #213 (Allocator-Realität), #217 (Array-Gattung, geklärt). Diese sind Teil-Aspekte DERSELBEN E2-Achsen-Verdrahtung → mit #188 gemeinsam, dann #215 (320-DLL-Neubau = Wirksamkeits-Schleuse).

**B2-2 (Ebene 2, 2026-06-28) — Achsen-Parameter-Konsum: compile-time läuft, Laufzeit-Konsum fehlt.** Die Cache-Line-Parameter (line_size/alignment/sw_hint) sind COMPILE-TIME in die Tier-Binary eingewoben (KF-5 #55 done, StaticAxisNode) → laufen. ABER: Grep über `axes/` fand **KEINE Laufzeit-RC-Setter** (kein `set_prefetch_distance`/`set_pool_budget`/`set_inline_threshold`/…; nur compile-time-Strategien wie `prefetch_distance_estimator` + das read-only Observer-Feld `last_prefetch_distance`). ⟹ die **E1→E2-Laufzeit-Konsum-Verdrahtung fehlt** (konsistent mit dem `applied_rc_`-Null-Object #221): die 4 RC-POD-Felder (B4-1) können erst XML-wirken, wenn die betroffenen Achsen-Organe einen Laufzeit-Konsum-Pfad (Setter, der den compile-time-Default zur Laufzeit overridet) bekommen. **⚠️ ZU KLÄREN mit User (vor #221-Verdrahtung):** die genannten „Vorbereitungen, die letzte Woche liefen" — mein Grep zeigt Builder-Seite (`RuntimeVariableLoop`) + Cache-Line-compile-time als fertig, aber **Achsen-seitige Laufzeit-Setter scheinen zu fehlen**; evtl. anderer Konsum-Mechanismus übersehen → erst verifizieren, dann verdrahten (nicht raten).

**B1-1 (Ebene 1, 2026-06-28) — RC-Laufzeit-Konsum-Verdrahtung fehlt (DEFINITIV, B2-2-Frage aufgelöst).** Grep über ALLE 19 RC-Feld-berührenden Dateien (`libs/cache_engine`) verifiziert: **kein Achsen-Organ/Composition konsumiert ein `ComdareResourceControlV1`-Feld zur Laufzeit.** Die Felder leben ausschließlich in: (a) RC-Infrastruktur (POD-Def, `abi_adapter` apply1→`applied_rc_` [tot/nie gelesen], `AlgorithmResourceControl::clamp`, `RuntimeVariableLoop::set_field`, caps), (b) XML/Builder-Definition+Durchreichung (Profile, `profile_to_tree`, iterator, `cacheline_policy`), (c) prefetch-Achse: `prefetch_distance` als COMPILE-TIME-Strategie (`distance_estimator`/limits) + read-only Observer-Statistik `last_prefetch_distance`. Die zwei ABI-Header-Treffer (`module_abi_v1.hpp` `prefetch_distance` im `action_kind`-Aktions-Descriptor; `processing_strategy.hpp` `prefetch_distance_max`-limit) sind eigene algorithmische Strukturen, NICHT RC-Konsum.
⟹ **Auflösung der B2-2-Frage („lief letzte Woche"):** Die fertigen Vorbereitungen = die RC-**Infrastruktur** (Builder-`RuntimeVariableLoop` + ABI `tier_apply_resource_control`/`clamp`/caps + Cache-Line-**compile-time**-Einwebung KF-5). Der **verbleibende #221-Schritt** = pro betroffener Achse einen **Laufzeit-Konsum-Setter** + Verdrahtung in `tier_apply_resource_control` (apply1 ruft den Organ-Setter, statt nur `applied_rc_` zu schreiben). Überschaubar (kein großer Umbau, wie vom User gesagt) — aber NICHT null: die End-Verdrahtung Achse↔RC fehlt. Konsistent mit B2-2.
**Mess-Durchführung (E1):** #225 = Grundsatz Second-Execution vs Zwei-Phasen (NUR Diskussion, USER, nicht-code). #216 = seg_ns n>1 (per-Achsen-Timing, hängt an #215).

### ✅ TOP-DOWN-AUDIT E4→E1 ABGESCHLOSSEN (2026-06-28)
Alle vier Experiment-Ebenen auditiert (B4-1/B4-2 · B3-1/B3-2 · B2-1/B2-2 · B1-1). **Gesamt-Verdikt:** Die Pipeline ist **strukturell gesund + XML-only-fähig im Gerüst** — XML-Definition (E4-Eingang), B+-Baum/Gattung (E3), Tier-Binary-Bau (E2-compile-time), Auswertungs-Tooling (E4-Ausgang) existieren + funktionieren für SearchAlgorithm. **Zwei Wurzel-Lücken + zwei Verdrahtungs-Lücken bleiben:**
1. **#188 (E2-Wurzel):** Weg-B-Mess-Echtheit (`search_organ_`-Entfall → `container_` trägt echtes search_algo für alle) — der große Mess-Echtheits-Hebel.
2. **#221 (E1-Verdrahtung):** Achsen-Laufzeit-RC-Konsum-Setter + apply1-Verdrahtung (Infra fertig).
3. **#223 (E3-Verdrahtung):** Konformitäts-Gate import→GATE→messen in den Voll-Lauf.
4. **B4-1 (E4↔E2):** die 4 RC-POD-Felder XML-definierbar (folgt automatisch, sobald #221-Achsen-Konsum steht).
**Wirksamkeits-Schleuse #215** (320-DLL-Neubau) bringt alle E2/E1-Fixes in die Daten; **#210** (Runner) ist das Infra-Gate für die finale Mess-Verifikation; dann aktiviert ein kleiner E4-Schritt (`-M3v2` + m3v2-Matrix #156/#162) die A/B/C-Reihen in der Auswertung. **Empfohlene Abarbeitungs-Reihenfolge (E2→E1, da E4/E3-Lücken davon abhängen):** #188 (+#211/#214/#216/#213/#217) → #221 (RC-Konsum, löst B4-1) → #223 (Gate) → #215 (Neubau) → [#210 Infra] → #156/#162 (A/B/C-Aktivierung).

## §15 EBENEN-EINTEILUNG aller offenen TODOs (teile-und-herrsche je E-Ebene, 2026-06-28)

**Zweck (User):** jedes offene TODO einer **E-Ebene** zuordnen → später je Ebene filtern + gezielt abarbeiten, nichts vergessen. Primär-Ebene + (Sekundär). **E0 = Querschnitt** (Infra/CI/Meta/Usability — NICHT Teil der Experiment-Pipeline). Reihenfolge: erst einteilen (diese Tabelle), dann Zusammenhänge/Widersprüche abwägen (s.u.) → verkleinert den Entwurfsraum durch sauber getrennte APIs je Ebene.

### E4 — XML-Experiment-Definition + Messwert-Extraktion/Auswertung (Superprojekt-Kette)
#25 Diplomarbeit-Text/Bausteine-Matrix (User) · #156 M3-Neumessung (Gesamt-Lauf-Def) · #162 PRT-ART+SOTA-Reihen A/B/C in den Lauf · #184 Dataset-Loader Nicht-YCSB · #219 Pipeline-Integrität XML-records/CSV-Auswertung · #226 Appendix-Daten-LIMITs · #152 Cache-Misses-Metrik in M3-Auswertung (E4/E1) · #187 PMC-Auto-Adaption Mess+Tabellen (E4/E1) · #165 Quality-Flag/Perzentil-Ausgabe (E4/E1) · #178 sota_catalog Stufe→Reihe (E4/E3).
### E3 — Permutations-B+-Baum PRO GATTUNG (Abstract Factory)
#223 Konformitäts-Gate import→GATE→messen in den Voll-Lauf (E3/E2) · #188 Experiment-B+-Baum zentral (T0-Such-Delegation) (E3/E2, Kern).
### E2 — Tier-Binaries / Achsen-Organe / Bau (StaticAxisNode)
#188 T0-Such-Delegation über Speicher-Achsen (Kern-Anatomie A3) · #19 Allokatoren echt linken · #211 container_→LinearScan/Append · #213 echter Policy-Allocator (T6) · #214 tier_scan GoF-Iterator (Scan-Achse) · #215 CoW real 320-DLL-Bau (Wirksamkeits-Schleuse) · #216 seg_ns n>1 + stat-Reset (E2/E1) · #217 Array-Gattung-Achse (Container-Metaprog-Rekursion) · #163 SIMD/ISA+Allokator als variierte Achsen · #185 TPIE/EM-BFS io_dispatch-Achse · #125 lazy DLL-Bibliothek (Content-Hash) · #224 GoF-Etiketten-Hygiene (Achsen-Namen, E2/E0).
### E1 — RC-Laufzeit / DynamicVariableNode / Mess-Durchführung
#221 RC Null-Object → DynamicVariableNode VOLLENDEN (Kern) · #225 Second-Execution vs Zwei-Phasen-Pflicht · #216 seg_ns (s.E2).
### E0 — Querschnitt (Infra/CI/Meta/Usability — NICHT Experiment-Pipeline)
#10 V42-Infra · #24 Cluster-Tasks · #149 MP-E Meta-Mission · #179 Wartbarkeits-Sweep · #186 CI/CD-Pipelines · #189 Infra-Handoff Runner · #193 manuell-bedienbar (Usability) · #199–#205 Pipeline-Stufen · #206–#210 Infra/Runner-EOF (#210 = Mess-Blocker) · #228 sslverify-Security · #229 Top-Down-Audit-Strecke (Meta).

### Querbezüge / Widersprüche (Anfang — wird beim Top-Down vertieft)
1. **#188 ist die E2/E3-Wurzel; #214/#211/#216 sind Teil-Aspekte derselben Achsen-Verdrahtung** → gemeinsam planen (isoliert = Doppelarbeit/Konflikt; #214 ist schon code-complete, muss zu #188 konsistent bleiben).
2. **E1 (#221-RC) setzt E2 voraus:** die RC-POD-Felder verdrahten an Achsen, die in E2 erst real KONSUMIEREN müssen → E2-Achsen-Konsum zuerst, dann E1-RC-Override (top-down: E4→E3→E2→E1 ist auch die korrekte FIX-Reihenfolge).
3. **#215 (E2, 320-DLL-Neubau) = Wirksamkeits-Schleuse:** alle E2-Achsen-Fixes (#188/#211/#213/#214/#216/#217) werden erst durch #215 in den Messdaten wirksam → #215 NACH den E2-Fixes, EINMAL gemeinsam.
4. **E4 #156/#162 (Gesamt-Lauf) ist letztes Glied** — gated durch E1/E2/E3 + #210 (Runner-Infra). 
5. **E4-Auswertungs-Kette im Superprojekt = offener blinder Fleck** (User-Hinweis): die XML ist nur ein kleiner Teil von E4; die Messwert-Extraktion/Auswertung/Thesis-Anbindung im Superprojekt ist noch NICHT vollständig auditiert → erster inhaltlicher E4-Schritt.
