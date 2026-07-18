# Doc 36 — Die EINE Architektur: `Lebewesen ≡ SearchAlgorithm` (Anatomie = sein Körper)

> **Status:** Verbindliche Klarstellung (User-Direktive 2026-06-20).
> **Anlass:** Im Thesis-Text wurde der Code-IST-Zustand (zwei getrennte Template-Bäume) zweimal
> fälschlich als gewolltes Design beschrieben — erst „orthogonal", dann „parallel". Beides ist FALSCH.
> **Primärquellen (Intention):** Doc 14 (`docs/architektur/14_achsen_komposition_organ_metapher.md`,
> §1, §3, §33–§40), Doc 34 (`docs/architecture/34_KONSOLIDIERTER_MASTER_IST_STAND.md`), Doc 30
> (§8.0/§8.1), `anatomy/anatomy_base.hpp`, `anatomy/search_algorithm_anatomy.hpp`, `anatomy/abi_adapter.hpp`.
> **Verifikation:** Multi-Agent-Workflows `we4xlbb23` (Code-Ist) + `w8aqb657k` (Doku-Intention, adversarial).

---

## 0. Kurzfassung (TL;DR)

Es gibt **genau eine** Architektur — eine einzige, in sich geschlossene Hierarchie, **keine**
Parallel-Bäume:

```
IExecutionEngine                       Wurzel: ALLES Ausmessbare
  ├─ IAnatomyBase : IExecutionEngine   Lebewesen (mit Achsen/Organen)        ┐ Geschwister
  │    └─ SearchAlgorithm-Unterklasse  ≡ „Lebewesen" (Säugetier, std::map)   │ unter EINER
  │         └─ SearchAlgorithmAnatomy<Composition>   = sein KÖRPER           │ Wurzel
  │              └─ 19 Achsen ≡ Organe                                       │
  │         └─ SearchAlgorithmAbiAdapter<A>  = ABI-Laufzeit-SICHT desselben  │
  │              Lebewesens über die .dll-Grenze („SearchEngine"-Rolle)      │
  └─ IVirusExecutionEngine : IExecutionEngine   Virus (achsenlos, z.B. Graph)┘
```

- **`Lebewesen` und `SearchAlgorithm` bezeichnen DASSELBE** — Metapher vs. technischer Identifier.
- Die **Anatomie ist der Körper** des Lebewesens (ein Ding MIT ihm), kein zweites Modell daneben.
- **„SearchEngine" ist keine zweite Hierarchie**, sondern nur die ABI-Laufzeit-**Sicht** desselben
  Lebewesens (`SearchAlgorithmAbiAdapter`).
- Dass der Code-IST heute zwei getrennte Bäume hat (`search_engine<>` vs. `SearchAlgorithmAnatomy`/
  `AbiAdapter`), ist ein **Defekt (I1-Verletzung)** — zu beheben (Doc 36 §4, Übergabe TODO-6), **nicht**
  in der Thesis als Design zu kanonisieren.

---

## 1. Zweck und Anlass

Diese Notiz hält fest, was die Architektur-Doku schon immer meinte, was im Thesis-Text aber verzerrt
wurde. Sie ist die kanonische Referenz für: (a) die Thesis-Formulierung (Kap. 1 + Kap. 4) und (b) den
Vereinheitlichungs-Auftrag an den Implementierungs-Agenten (Übergabe-Dokument
`docs/docs/sessions/20260620-UEBERGABE-impl-agent-EINE-ARCHITEKTUR-vereinheitlichung.md`).

**Leitsatz:** Maßgeblich für die *gewollte* Architektur ist die Doku/Metapher (Primärquelle der
Intention), **nicht** der evtl. defekte Code-IST.

---

## 2. Die EINE Architektur, Ebene für Ebene

### 2.1 Ebene 0 — Wurzel: `IExecutionEngine` (alles Ausmessbare)
`IExecutionEngine` ist die abstrakte Wurzel über allem, was gemessen werden kann
(`execution_engine/execution_engine_base.hpp:98`: „IExecutionEngine — abstract base fuer alle
ausmessbaren Engines"). Unter ihr stehen **zwei Geschwister**: Lebewesen (`IAnatomyBase`) und
Nicht-Lebewesen/Viren (`IVirusExecutionEngine`). Die Mess-Schnittstelle liegt auf dieser Ebene — sowohl
Lebewesen als auch Viren sind messbar (Doc 14 §33–§40).

### 2.2 Ebene 1 — Gattung = Außen-Interface = Prüf-Dock (nur 3)
`AnatomyGattung` (`anatomy/anatomy_base.hpp:40–44`): **SearchAlgorithm | Container | Graph**. Das ist
das Interface, das die Außenwelt/der Host sieht und gegen das gemessen wird (Prüf-Dock). „Gattung"
(präzis) = Außen-Interface, NICHT die Tier-Unterklasse.

### 2.3 Ebene 2 — Lebewesen ≡ SearchAlgorithm + sein Körper (Anatomie)
`AnatomyGenus` (Tier-Unterklasse, fester Achsen-Satz; `anatomy_base.hpp:62/68`): **SearchAlgorithm |
Set | Sequence | Adapter | View**. Das „Lebewesen" (Metapher, „Säugetier") und „SearchAlgorithm"
(technischer Identifier) sind **zwei Namen für eine Sache** (anatomy_base.hpp:62 „Saeugetier |
SearchAlgorithm | … map, multimap, unordered_map").

Der **Körper** dieses Lebewesens ist seine **Anatomie** — `SearchAlgorithmAnatomy<Composition>`
(`anatomy/search_algorithm_anatomy.hpp:32`). Sie ist KEIN separates Modell neben dem Suchalgorithmus,
sondern seine Compile-Time-Manifestation: sie trägt die Organe als Member (`axis_search_algo_`,
`axis_telemetry_`, … alle 19 Slots), liefert `genus()==AnatomyGenus::SearchAlgorithm` (Z.46) und
`observe_all()→ObserverAggregate` (Z.62). Slot-Zahlen je Tier-Unterklasse (`genus_binding_traits.hpp`):
**SearchAlgorithm 19**, Set 15, Sequence 11, Adapter 13, View 7.

### 2.4 Ebene 3 — Achsen ≡ Organe (19 für SearchAlgorithm, keine optional)
Jede Achse ist ein **Organ** (eine Teilaufgabe, die jeder Suchalgorithmus auf irgendeine Weise löst;
Doc 14 §1: „Achse = Organ. Algorithmus = Permutations-Konfiguration aller Achsen"). Ein konkreter
Algorithmus (z.B. ART) ist **keine eigene Klasse**, sondern genau eine Punkt-Konfiguration aller 19
Achsen (Doc 14 §3.1–§3.3); das Permutieren der Achsen ist „das genetische Experiment am Lebewesen".
Die 19 Slots sind hart geprüft (`composition_concept.hpp:20–58` `IsComposition`;
`composition_factory.hpp` `static_assert sizeof...(Vs)==19`). Keine Achse ist optional: ein
„nicht-pufferndes" Lebewesen wählt den Algorithmus `NoBuffer`, NICHT „keine Puffer-Achse".

### 2.5 „SearchEngine" — ABI-Sicht, keine zweite Hierarchie
Über die Modulgrenze wird **dasselbe Lebewesen** als ABI-Sicht sichtbar: `SearchAlgorithmAbiAdapter<A>`
(`anatomy/abi_adapter.hpp:119`, `final : public IAnatomyBase`), mit
`static_assert(A::genus()==AnatomyGenus::SearchAlgorithm)` und „I1: GENAU EINE Observer-Schnittstelle"
(Z.124–125). Dieser Adapter brückt die Compile-Time-Anatomie über die `extern "C"`-Factory
(`comdare_create_anatomy → IAnatomyBase*`) zum Host (Modul-Loader). Der frühere Klassenname
`search_engine<Collection,ConfigPermutation>` (search_engine.hpp:2: „Dach … Stufe 3 der
Drei-Schichten-Hierarchie") ist nur der **historische Identifier desselben Dings** — keine eigene,
zweite Hierarchie.

Die ABI-Schichtung ist also rein technisch EINER Hierarchie:
**Prüf-Dock (Gattungs-Interface) → SearchAlgorithmAnatomy (Compile-Time-Körper) →
SearchAlgorithmAbiAdapter (Runtime-Sicht) → 19 Achsen/Organe.**

### 2.6 Viren — die einzige legitime Geschwister-Kategorie
`IVirusExecutionEngine : IExecutionEngine` (Nicht-Lebewesen wie reine Graphen-Algorithmen): **keine
Achsen, keine Composition** (intern undurchsichtig, aber messbar). Das ist die einzige zulässige
„Mehrfachheit" — und selbst sie sind **Geschwister unter EINER Wurzel** `IExecutionEngine`, nicht
parallel-unabhängig.

---

## 3. Term-Mapping (EIN Konzept, zwei Vokabulare)

| Metapher (Kommentar/Doku) | Technischer Identifier (Code) | Bedeutung |
|---|---|---|
| (Wurzel, alles Ausmessbare) | `IExecutionEngine` | Gemeinsame Mess-Schnittstelle für Lebewesen UND Viren |
| Lebewesen / Säugetier | **`SearchAlgorithm`** (`AnatomyGenus::SearchAlgorithm`) | **DASSELBE Ding**; std::map-artiges K→V-Lebewesen, 19 Achsen |
| Gattung / Prüf-Dock | `AnatomyGattung` (SearchAlgorithm/Container/Graph) | Außen-Interface, was der Host sieht (nur 3) |
| Körper | `SearchAlgorithmAnatomy<Composition>` | Compile-Time-Manifestation des Lebewesens; trägt die Organe |
| Organ | Achse (1 Composition-Slot) | Teilaufgabe; 19 Slots für SearchAlgorithm; keine optional |
| Genom / Bauplan | `Composition` (`composition_t`) | die 19 `using`-Aliase, die die Organe festlegen |
| „SearchEngine" (ABI-Sicht) | `SearchAlgorithmAbiAdapter<A>` | dasselbe Lebewesen über die .dll-Grenze; KEINE zweite Hierarchie |
| genetisches Experiment | Permutation der Achsen | ein konkreter Algorithmus = eine Punkt-Konfiguration |
| Virus | `IVirusExecutionEngine` | Nicht-Lebewesen (achsenlos); Geschwister unter derselben Wurzel |

**Konsequenz:** Weil `Lebewesen ≡ SearchAlgorithm`, kann es nur **ein** Lebewesen-Modell geben. Eine
„Anatomie parallel zum SearchEngine" wäre eine Scheindopplung desselben Begriffs.

---

## 4. Der Code-IST-Defekt (I1-Verletzung: zwei Bäume)

Heute bilden im Code **zwei getrennte Template-Bäume** dasselbe konzeptionelle Ding ab, ohne durch
Vererbung verbunden zu sein:

1. **`search_engine<Collection, ConfigPermutation> : public execution_engine<…>`**
   (`include/cache_engine/abi/search_engine.hpp:19–21`) — trägt Container-API (`lookup/insert/erase`,
   Z.36–38), aber **keine** 19 Achsen, **keine** Observer-Statistik.
2. **`SearchAlgorithmAnatomy<Composition>`** (`anatomy/search_algorithm_anatomy.hpp:32`) +
   **`SearchAlgorithmAbiAdapter<A> : IAnatomyBase`** (`anatomy/abi_adapter.hpp:119`) — die ECHTE
   Lebewesen-Repräsentation: alle 19 Organe + `observe_all()`.

**Bruchstellen:** (a) kein gemeinsamer Code — (2) erbt von `IAnatomyBase`→`IExecutionEngine` (virtuelle
Runtime-Wurzel), (1) von `execution_engine<>` (C++-Template-Ebene); (b) duplizierte Achsen-/Slot-
Verwaltung; (c) Observability (`observe_all`, 19 Achsen) nur auf der Anatomie-Seite; (d) drei
„Container"-Instanzen desselben Lebewesens nebeneinander (Anatomie-Organ / Builder-`container_` /
lokales `run_workload`-SearchAlgo — Doc 24 §-Tabelle). Das verletzt die Observer-Konsolidierung **I1**
(Doc 31).

→ **Zu beheben:** Vereinheitlichung auf EINE Hierarchie — siehe Übergabe-Dokument (TODO-6).

---

## 5. Warum „parallel / orthogonal" falsch war (Lehre)

1. **Code-Defekt ≠ gewollte Architektur.** Der beobachtete Ist-Zustand (zwei Bäume) wurde zur Intention
   erklärt; die Doku verlangt aber ausdrücklich EINE unitäre Hierarchie (Doc 34 §1; Doc 30 §8.0).
2. **`Lebewesen` und `SearchAlgorithm` sind dasselbe** — sie als zwei Dinge zu behandeln, erzeugt die
   Scheindopplung „Anatomie neben SearchEngine".
3. **Ein Körper kann nicht „parallel" zu seinem eigenen Lebewesen liegen** — die Organe SIND das
   Lebewesen, seziert (Doc 14: „Permutation = genetisches Experiment am Lebewesen").
4. **„SearchEngine" ist eine SICHT, kein Geschwister.** Die einzige legitime Mehrfachheit ist Lebewesen
   vs. Virus — und selbst die sind Geschwister unter EINER Wurzel.

---

## 6. Konsequenzen

- **Thesis** (Abstract + Kap. 1, DE+EN; Commit `73554e1`): „parallel" entfernt → EINE Architektur.
  Kap. 4 breitet die eine Architektur formal aus (diese Doc 36 als Grundlage).
- **Code** (Impl-Agent): Vereinheitlichung gemäß Übergabe-Dokument / Handout TODO-6 (cache-engine
  `f60799e`). Die frühere „Option A/B parallel belassen"-Lesart (Handout TODO-5) ist **verworfen**.
- **Code-Vereinheitlichung DURCHGEFÜHRT (Impl-Agent, 2026-06-25):** Der tote Parallel-Baum
  `comdare::search_engine<>` + `comdare::execution_engine<>`
  (`include/cache_engine/abi/{search_engine,execution_engine}.hpp`) + der von keinem `#include`
  konsumierte Waisen-Adapter `prt-art …/identity/prt_art_search_engine_adapter.hpp` wurden **entfernt**.
  Es verbleibt EINE Hierarchie `IExecutionEngine→IAnatomyBase→SearchAlgorithmAnatomy<C>→SearchAlgorithmAbiAdapter<A>`
  (= ABI-Sicht „SearchEngine"; Invariante als Code-Kommentar in `anatomy/abi_adapter.hpp`). Die reale
  prt-art-Kopplung (`optional_prt_art_impl`-Slots + CMake-`COMDARE_CE_PRUEFLINGE`,
  `axes/axis_centric_namespaces.hpp:145-172`) bleibt **unberührt**. ABI-Major unverändert (Header-only,
  überquert die `extern "C"`-Factory nicht). Build grün (`test_abi_interface`, `test_v41_anatomy_module_abi`).

---

## 7. Quellen
- Doc 14 `docs/architektur/14_achsen_komposition_organ_metapher.md` (§1, §3.1–§3.3, §33–§40).
- Doc 34 `docs/architecture/34_KONSOLIDIERTER_MASTER_IST_STAND.md` (3-Ebenen-Modell, „EINE Hierarchie").
- Doc 30 §8.0/§8.1 (Gattung/Genus, queuing als reguläre SA-Achse).
- Doc 31 (Observer-Konsolidierung I1).
- Code: `anatomy/anatomy_base.hpp`, `anatomy/search_algorithm_anatomy.hpp`, `anatomy/abi_adapter.hpp`,
  `anatomy/composition_concept.hpp`, `anatomy/genus_binding_traits.hpp`,
  `execution_engine/execution_engine_base.hpp`, `include/cache_engine/abi/search_engine.hpp`.
