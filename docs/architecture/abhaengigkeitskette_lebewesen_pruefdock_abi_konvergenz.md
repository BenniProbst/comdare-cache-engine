# Autoritative Referenz — Abhängigkeitskette LEBEWESEN → PRÜF-DOCK und der ABI-POD-Konvergenzpunkt (Vorlage für I1)

> **KORREKTUR-BANNER (korr. 2026-06-03, s. Doc 30 §8.0; verbatim Doc 24 §8.8 + Doku 14 §25):** Dieses Dokument verwendete
> „GATTUNG" durchgehend als Bezeichnung für die Ebene mit dem FESTEN 17-Achsen-Satz (`AnatomyGenus`-Enum, `SearchAlgorithmAnatomy<C>`).
> Das ist gemäß dem verbindlichen 3-Ebenen-Modell die **LEBEWESEN-UNTERKLASSEN-Ebene**, NICHT die Gattungs-Ebene. Klarstellung:
> (1) **GATTUNG = ein INTERFACE für die Außenwelt = ein Prüf-Dock**: SearchAlgorithm / Container / Graph (Doc 24 §8.8 „Prüf-Dock
> je Gattung", §8.6 „ABI-Interface der API der Gattung"). (2) **LEBEWESEN-UNTERKLASSE = liegt UNTER dem Gattungs-Interface und
> trägt den FESTEN Achsen-Satz** (Doku 14 §25/§26). Set/Sequence/Adapter/View sind Lebewesen-Unterklassen UNTER dem **Container**-Interface;
> die 17/19-Achsen-Komposition ist die **SearchAlgorithm-Lebewesen-Unterklasse**. (3) **ACHSEN = Organe der Lebewesen-Unterklasse, NIE optional** —
> die Interfaces ALLER Achsen werden in JEDEM Lebewesen-Binary uniform getrieben; ein nicht-pufferndes/nicht-prefetchendes Lebewesen wählt
> einen KONKRETEN Durchreich-Algorithmus (NoBuffer/NoFlush/NonePrefetch/NoMigration/None), es wird KEINE Achse weggelassen.
> Wo „Gattung" das Außen-Interface / Prüf-Dock / den `genus()`-Dock-Diskriminator meint, ist es KORREKT und bleibt. **WICHTIGE
> Nuance:** Aussagen wie „optionales Sub-Interface" / „nur 2 von 17 Achsen getrieben" / „dormant" beschreiben, WELCHE Achsen-Snapshots
> die .dll-ABI-Grenze tatsächlich queren — NICHT, dass eine Achse im Lebewesen fehlt. In-Process werden alle 17 Organe uniform getrieben.
> Die **Invariante** (feste Slot-Zahl pro Lebewesen-Unterklasse, `AdHocComposition<17>` als ABI-Identität) bleibt unverändert gültig.

> **Provenienz:** Inventar-Workflow `w1fzdr47e` (5 Explore-Agenten je Schicht + Synthese, 2026-05-31). Alle Datei:Zeile-Angaben
> aus dem Schicht-Inventar; Snapshot-Layout zusätzlich an der Quelle (`observable_tier.hpp:40-59`) verifiziert. Annahmen
> als **[ANNAHME]** markiert. Geltungsbereich: `external/comdare-cache-engine/libs/cache_engine/`.
>
> **I1-Abgleich (User-Entscheidung „EIN einziger ABI-POD"):** Der eine autoritative Mess-POD = der NEUE
> `ComdareTierObserverSnapshotV2` (volle Spaltenmenge). V1 bleibt nur als deprecated Regressions-Typ erhalten; der
> LIVE-Pfad nutzt ausschließlich V2 + `ANATOMY_ABI_MAJOR 1→2`, sodass alt-gebaute V1-DLLs vom Loader-Major-Check sauber
> abgelehnt werden (= „alle Permutations-DLLs neu"). Der dormante Zwilling `comdare_hw_counters_v1`/`pull_live_counters`
> (`module_abi_v1.hpp`) geht in den einen POD auf.

---

## 1. Zielbild

Die **Observer+Tier-Schnittstelle** (`IObservableTier` + `ComdareTierObserverSnapshotV1`) ist keine eigenständige zweite
ABI, sondern eine **tier-unterklassen-spezialisierte Erweiterung der Anatomy-ABI** (korr. 2026-06-03, s. Doc 30 §8.0 — vorher
„gattungs-spezialisiert"; die Spezialisierung sitzt auf der **Lebewesen-Unterklassen**-Ebene, dem festen 17-Achsen-Satz, nicht auf der
Gattungs-/Interface-Ebene): Ein Lebewesen-Modul exportiert genau vier
extern-`C`-Symbole (Factory/Destroy/Version/Magic, `anatomy_module_abi_v1_decl.hpp:61-74`) und liefert über
`comdare_create_anatomy()` einen `IAnatomyBase*`. Erst *nach* dem Laden fragt der Host per `dynamic_cast<IObservableTier*>`
ab, ob dieses konkrete Modul über die ABI-Grenze zusätzlich den Pfad-B-Antrieb (`tier_insert/lookup/erase/observe`) **exportiert** —
das ist eine reine ABI-Exposure-Frage (welche Achsen-Treiber die .dll-Grenze queren), KEINE Aussage darüber, ob das Lebewesen diese
Achse besitzt; in-process trägt jedes Lebewesen alle 17 Organe (korr. 2026-06-03, s. Doc 30 §8.0). Beide Teile —
die unveränderliche Anatomy-Wurzel-vtable und das per `dynamic_cast` erreichbare Observer-Sub-Interface — **müssen zusammen über dieselbe
Brücke laufen** (Lebewesen-Binary → `AnatomyModuleLoader` → `IPruefDock`), weil der `CacheEngineBuilder` einen einzigen
polymorphen Mess-Loop pro Gattungs-Prüf-Dock fährt: das Dock zieht aus *einem* geladenen Handle sowohl die Gattungs-Interface-Identität
(`genus()`, ABI-fest — der Dock-Diskriminator; korrekt als Gattungs-Begriff) als auch die Mess-Observable (Snapshot-POD). Trennte man beide ABIs, verlöre der Loader
die Garantie, dass Heap-Allokation (`comdare_create_anatomy`) und Heap-Freigabe (`comdare_destroy_anatomy`) im selben
DLL-Kontext liegen, und der Snapshot-Transport würde von der Lebenszyklus-Verwaltung des Moduls entkoppelt.

---

## 2. Die Kette als ASCII-Diagramm

```
 COMPILE-TIME (Modul-Autor-Seite, in der DLL)            │ ABI-GRENZE │   RUNTIME (Host / CacheEngineBuilder-Seite)
═══════════════════════════════════════════════════════ │ ══════════ │ ═════════════════════════════════════════════
                                                         │            │
 LEBEWESEN  IExecutionEngine                             │            │
   execution_engine_base.hpp:98                          │            │
   (Schwester: IVirusExecutionEngine,                    │            │
    test_v41_execution_engine.cpp:148 — NICHT abgeleitet)│            │
        │  erbt                                          │            │
        ▼                                                │            │
 GATTUNG/   AnatomyGenus  (enum uint8, 5 Werte)          │            │
 LEBEWESEN-UKL   anatomy_base.hpp:44     SearchAlgorithm=0    │            │
 (gemischt) (korr. 2026-06-03, s. Doc 30 §8.0: dieser    │            │
   Enum MISCHT 2 Modell-Ebenen — SearchAlgorithm/Container│           │
   /Graph = GATTUNG=Interface=Prüf-Dock; Set/Sequence/   │            │
   Adapter/View = LEBEWESEN-UNTERKLASSEN unter Container;      │            │
   genus()-Wert dient als Dock-Diskriminator [Gattung,    │            │
   korrekt] UND nennt Lebewesen-Unterklassen)                  │            │
        │  diskriminiert (Dock-Wahl = Gattungs-Interface) │            │
        ▼                                                │            │
 LEBEWESEN-UKL   Composition  (17 using-Organ-Slots)          │            │
 (LEBEWESEN-ART)  (korr. 2026-06-03: „LEBEWESEN-ART"=LEBEWESEN-UNTERKLASSE,│           │
            trägt den FESTEN 17-Achsen-Satz, Doku 14 §25) │            │
   z.B. ArtComposition  art_reference.hpp:58             │            │
   Contract: IsComposition  composition_concept.hpp:18   │            │
        │  Template-Parameter <IsComposition C>          │            │
        ▼                                                │            │
 ANATOMIE   SearchAlgorithmAnatomy<C>                    │            │
   search_algorithm_anatomy.hpp:31                       │            │
   genus()==SearchAlgorithm, observe_all()→              │            │
   ObserverAggregate<C> (17 Slots, observer_aggregate.hpp:94)         │
        │  template<AnatomyConcept A>                    │            │
        ▼                                                │            │
 ADAPTER    SearchAlgorithmAbiAdapter<A>      ━━━━━━━━━━━━┿━━ vtable ━━┿━━► IAnatomyBase*  (anatomy_base.hpp:99)
   abi_adapter.hpp:75   DREIFACH-VERERBUNG:              │  (fix)     │       │ genus() → Dock-Wahl
   : IAnatomyBase, IMeasurableWorkload, IObservableTier  │            │       │
   static_assert(A::genus()==SearchAlgorithm)            │            │       │ dynamic_cast<IObservableTier*>
        │                                                │            │       │ dynamic_cast<IMeasurableWorkload*>
        │  COMDARE_DEFINE_ANATOMY_MODULE(C)              │            │       ▼
        ▼                                                │            │   LOADER  AnatomyModuleLoader::load()
 FACTORY    extern "C" 4 Symbole  ━━━━━━━━━━━━━━━━━━━━━━━━┿━ Symbole ━━┿━►   anatomy_module_loader.hpp:149
   anatomy_module_abi_v1_decl.hpp:61-74                  │            │     resolve: magic→major→minor
   create / destroy / abi_version / abi_magic            │            │     → AnatomyModuleHandle (RAII, :76)
                                                         │            │       │ destroy_fn DANN dlclose
 SNAPSHOT   ComdareTierObserverSnapshotV1  ━━━━━━━━━━━━━━━┿━ POD memcpy┿━►   PRÜF-DOCK  PruefDockRegistry
   observable_tier.hpp:40  (13×uint64)                   │            │     ::select_for(Handle)→genus()
                                                         │            │       │
                                                         │            │       ▼  SearchAlgorithmDock::measure()
                                                         │            │     search_algorithm_dock.hpp:22
                                                         │            │     → drive_tier_observe_trace_abi
                                                         │            │       (tier_observe_trace_abi.hpp:77)
```

---

## 3. Schicht-für-Schicht

### Schicht 1 — LEBEWESEN vs. VIREN (biologische Wurzel)

| Typ | Kind | Datei:Zeile | Rolle |
|---|---|---|---|
| `IExecutionEngine` | class | `execution_engine_base.hpp:98` | Abstrakte Wurzel aller ausmessbaren Engines |
| `ExecutionEngineKind` | enum | `execution_engine_base.hpp:39` | Anatomy=0 / Virus=1 / Hybrid=2 (uint8 POD) |
| `EngineLifecycleState` | enum | `execution_engine_base.hpp:60` | Uninitialized→Warming→Running→Idle→Shutdown |
| `ExecutionEngineConcept` | concept | `execution_engine_base.hpp:78` | erzwingt `measurement_snapshot_t`, `engine_name()`, `engine_kind()` |
| `IAnatomyBase` | class | `anatomy_base.hpp:99` | Lebewesen-Spezialisierung, `final engine_kind()=Anatomy` |
| `IVirusExecutionEngine` | class | `test_v41_execution_engine.cpp:148` | Virus-Spezialisierung (Skelett im Test; Ziel: `virus/virus_execution_engine.hpp` in R6/V42) |

**Kanten:**
- `IAnatomyBase : public IExecutionEngine`
- `IVirusExecutionEngine : public IExecutionEngine` — **Schwester, nicht Tochter**: `static_assert(!is_base_of<IAnatomyBase, IVirusExecutionEngine>)`
- `GraphBfsVirusStub : IVirusExecutionEngine` (Beispiel-Virus, kein AnatomyBase)

**Metapher (Doku 14 §27-§40):** Lebewesen = Anatomien mit Topics/Achsen; Viren = Graphen-Algos ohne Achsen. Beide ausmessbar (Lifecycle), aber parallele Spezialisierungen.

**ABI-fest (vtable-Reihenfolge unveränderlich seit prt_art_legacy Rev5):** `engine_name()` (String quert Grenze), `engine_kind()→uint8`, `lifecycle_state()→uint8`, `warm_up/run/reset/shutdown` (void).

---

### Schicht 2 — GATTUNGS-INTERFACE + LEBEWESEN-UNTERKLASSEN-DISKRIMINATOR (AnatomyGenus)

> **(korr. 2026-06-03, s. Doc 30 §8.0):** Der `AnatomyGenus`-Enum MISCHT zwei Modell-Ebenen und ist daher kein reiner
> „Gattungs"-Enum. **GATTUNG (= Interface für die Außenwelt = Prüf-Dock)** sind die drei Außen-Interfaces SearchAlgorithm /
> Container / Graph. **Set / Sequence / Adapter / View sind LEBEWESEN-UNTERKLASSEN UNTER dem Container-Interface** (Doku 14 §26 die
> std-Familien), KEINE eigenen Gattungen. `SearchAlgorithm` ist hier zugleich Gattungs-Interface UND die einzige bisher gebaute
> Lebewesen-Unterklasse (std::map-ähnlich, 17 Achsen). Der `genus()`-Rückgabewert dient als **Dock-Diskriminator** — in dieser Rolle
> ist „Gattung" korrekt (er wählt das Außen-Interface/Prüf-Dock). Die untenstehenden Code-Identifier (`AnatomyGenus`,
> `SearchAlgorithmAnatomy`) bleiben unverändert; nur ihre Ebenen-Einordnung wird hier richtiggestellt.

| Typ | Kind | Datei:Zeile | Rolle |
|---|---|---|---|
| `AnatomyGenus` | enum | `anatomy_base.hpp:44` | 5 Werte: SearchAlgorithm(0)/Set(1)/Sequence(2)/Adapter(3)/View(4). **(korr. 2026-06-03)** Mischt Gattungs-Interface (SearchAlgorithm; Container/Graph noch nicht im Enum) mit Lebewesen-Unterklassen unter Container (Set/Sequence/Adapter/View) |
| `AnatomyConcept` | concept | `anatomy_base.hpp:76` | fordert `composition_t`, `composition_name()`, `paper_id()`, `organ_count()`, `genus()` |
| `SearchAlgorithmAnatomy` | template | `search_algorithm_anatomy.hpp:31` | Mammal-**Lebewesen-Unterklassen**-Template (korr. 2026-06-03, vorher „Gattungs-Template"; trägt den festen 17-Achsen-Satz, Doku 14 §25), `genus()=SearchAlgorithm` |
| `AnatomyModuleLoader` | class | `anatomy_module_loader.hpp:149` | dlopen + 4-Symbol-Resolve + Version/Magic-Check |
| `AnatomyModuleHandle` | class | `anatomy_module_loader.hpp:76` | RAII über Handle + `IAnatomyBase*` + `destroy_fn` |

**Kanten:**
- `SearchAlgorithmAnatomy<C>` erfüllt `AnatomyConcept` + `static_assert(A::genus()==SearchAlgorithm)`
- `AnatomyModuleHandle` kapselt `(void* native_handle, IAnatomyBase* anatomy_ptr, void(*destroy_fn))`

**Metapher (Doku 14 §27.2):** Säugetier=SearchAlgorithm (K→V, 17 Achsen) / Vogel=Set (K-only) / Reptil=Sequence (V-indexed) / Wirbelloses=Adapter / Pflanze=View. Alle im `kingdom_name()='Animalia'`.
> **(korr. 2026-06-03, s. Doc 30 §8.0):** Diese fünf Lebewesen-Klassen sind **LEBEWESEN-UNTERKLASSEN**, nicht fünf Gattungen. Säugetier = die
> SearchAlgorithm-Lebewesen-Unterklasse (auch das einzige Außen-Interface, das bisher gebaut ist). Vogel/Reptil/Wirbelloses/Pflanze
> (Set/Sequence/Adapter/View) sind Lebewesen-Unterklassen UNTER dem **Container**-Außen-Interface. „5 Gattungen" wäre falsch — Gattungen
> (= Außen-Interfaces/Prüf-Docks) sind SearchAlgorithm / Container / Graph.

**Lebewesen-Unterklassen-/Gattungs-Constraint (Doku 14 §32):** Cross-Genus-Joins (Mischen verschiedener Lebewesen-Unterklassen bzw. verschiedener Gattungs-Interfaces) type-system-mathematisch unmöglich; `static_assert` im AbiAdapter (Zeile 78-81). `genus()` ist **doppelt** vorhanden: constexpr (Concept-Seite) + virtual `IAnatomyBase::genus()` (ABI-Seite — der Dock-Diskriminator; in dieser Rolle ist „Gattung" korrekt). *(korr. 2026-06-03: „Gattungs-Constraint" umfasst hier beide Ebenen-Schnitte; die Invariante selbst bleibt unverändert.)*

---

### Schicht 3 — LEBEWESEN-UNTERKLASSE / LEBEWESEN-ART (Composition)

> **(korr. 2026-06-03, s. Doc 30 §8.0):** „LEBEWESEN-ART" ist hier die **LEBEWESEN-UNTERKLASSEN-Ebene** des Modells (sie trägt den FESTEN
> 17-Achsen-Satz; Doku 14 §25). Diese Ebene liegt UNTER dem Gattungs-Interface (hier: SearchAlgorithm). Die einzelnen
> Compositions (`ArtComposition`, `HotComposition`, …) sind konkrete Permutationen DIESER einen Lebewesen-Unterklasse. Die
> 17-Slot-Invariante (`AdHocComposition<17>` als ABI-Identität) ist und bleibt korrekt.

| Typ | Kind | Datei:Zeile | Rolle |
|---|---|---|---|
| `IsComposition` | concept | `composition_concept.hpp:18` | ALLE 17 Achsen-using-Aliases müssen präsent sein |
| `AdHocComposition<T0..T16>` | template | `composition_factory.hpp:45` | Monolith-Permutation (17 Slots in fester Reihenfolge) |
| `CompositionFromPermTuple<PermT>` | alias | `composition_factory.hpp:101` | `PermTuple<V0..V16>` → `AdHocComposition` |
| `ArtComposition` | struct | `art_reference.hpp:58` | benannte Reference-Composition (HasCompositionLocation) |
| `HotComposition` / `WormholeComposition` | struct | `hot_reference.hpp` / `wormhole_reference.hpp` | weitere benannte Compositions |
| `KnownReferenceCompositions` | template | `known_compositions_list.hpp:64` | `mp_list<11 Entries>` (6 CE-Reimpl + 5 PaperBinding) |
| `TierOrganPair<Tier,Organ>` | struct | `tier_to_organ_mapping.hpp:102` | Doku-Paar (Umstufung-B-Beleg, **keine Vererbung**) |

**Slot-Reihenfolge (UNVERRÜCKBAR, T0..T16):** `search_algo, cache_traversal, mapping, path_compression, node_type, memory_layout, allocator, prefetch, concurrency, serialization, telemetry, value_handle, isa, index_organization, io_dispatch, migration_policy, filter` → 3 traversal + 2 nodes + 7 Einzel + 5 = **17**.

**Metapher (Doku 14 §3):** Composition = konkrete LEBEWESEN-ART innerhalb der **Lebewesen-Unterklasse** (Permutation aller 17 Organe; korr. 2026-06-03, s. Doc 30 §8.0). Achse = Organ (z.B. search_algo = Atemapparat, allocator = Verdauungsapparat) — **jedes der 17 Organe ist Pflicht**, keine Achse wird weggelassen; ein Lebewesen ohne z.B. Pufferung wählt einen konkreten Durchreich-Algorithmus (NoBuffer/NoFlush/NonePrefetch/NoMigration/None), nicht „Organ fehlt". `ArtComposition::search_algo = ObservableArtTrieOrgan = ObservableComposedContainer<ArtTrieOrgan>` (Observable-Hülle um Organ, #42 Umstufung-B). *(Hinweis: `ObservableComposedContainer` ist ein Code-Identifier für die Organ-Hülle — kein Bezug zur Container-Gattung.)*

---

### Schicht 4 — ANATOMIE (SearchAlgorithmAnatomy + Observer-Aggregat)

| Typ | Kind | Datei:Zeile | Rolle |
|---|---|---|---|
| `SearchAlgorithmAnatomy` | template | `search_algorithm_anatomy.hpp:31` | hält `axis_search_algo_`, `observe_all()→ObserverAggregate<C>` |
| `ObserverAggregate` | struct | `observer_aggregate.hpp:94` | 17-Member-POD, `observable_count()` (in-process, NICHT über ABI) |
| `ObservableAxis` | concept | `observer_aggregate.hpp:40` | fordert `snapshot_t` + `statistics()` |
| `snapshot_of_t` | alias | `observer_aggregate.hpp:64` | `ObservableAxis<A>?A::snapshot_t : EmptyAxisSnapshot` |
| `EmptyAxisSnapshot` | struct | `observer_aggregate.hpp:26` | Fallback-POD (standard_layout + trivially_copyable) |
| `AnatomyExecutionContext` | template | `anatomy_execution_context.hpp:33` | Builder-Side-Container, ergänzt `observe_all()` mit echtem Organ |

**Kanten:**
- `SearchAlgorithmAnatomy<Composition>` constrained durch `IsComposition`
- `AnatomyExecutionContext<C>` hält `anatomy_t` + `container_t = ObservableComposedSearch<SortedBinaryTraversal, ComposedStore<node_type, memory_layout, allocator>>` + `search_organ_`

**Observer-Aggregation:** `observe_all()` populiert real nur den `search_algo`-Slot via `axis_search_algo_.statistics()`; `AnatomyExecutionContext::observe_all()` ersetzt diesen mit dem echten `container_`-Organ; R6-allocator-Slot über doppeltes Gate (`Composition::allocator` observable AND `container_t` hat `allocator_statistics()`).

---

### Schicht 5 — OBSERVER+TIER + PRÜF-DOCK (Konvergenz-Schicht)

| Typ | Kind | Datei:Zeile | ABI? |
|---|---|---|---|
| `ComdareTierObserverSnapshotV1` | struct | `observable_tier.hpp:40` | **ja** — quert als POD |
| `IObservableTier` | interface | `observable_tier.hpp:80` | **ja** — optionales Sub-Interface |
| `IMeasurableWorkload` | interface | `measurable_workload.hpp:20` | **ja** — optionales Sub-Interface |
| `SearchAlgorithmAbiAdapter<A>` | class | `abi_adapter.hpp:75` | Dreifach-Vererbung, in DLL instantiiert |
| `AnatomyAbiVersion` | struct | `anatomy_module_abi_v1_decl.hpp:85` | Host-seitiger Versions-Unpacker |
| `comdare_anatomy_abi_version` | fn | `anatomy_module_abi_v1_decl.hpp:61` | extern-C |
| `comdare_anatomy_abi_magic` | fn | `anatomy_module_abi_v1_decl.hpp:64` | extern-C (`0x434F4D444141312E`) |
| `comdare_create_anatomy` | fn | `anatomy_module_abi_v1_decl.hpp:66` | extern-C Factory |
| `comdare_destroy_anatomy` | fn | `anatomy_module_abi_v1_decl.hpp:73` | extern-C Dtor |
| `IPruefDock` | interface | `pruef_dock.hpp:51` | **nein** — nur Builder-Binary |
| `SearchAlgorithmDock` | class | `search_algorithm_dock.hpp:22` | nein |
| `PruefDockRegistry` | class | `pruef_dock_registry.hpp:22` | nein |
| `measure_genus_sequential` | fn | `pruef_dock_sequencer.hpp:44` | nein |
| `drive_tier_observe_trace_abi` | fn | `tier_observe_trace_abi.hpp:77` | nein (treibt aber über ABI) |

> **Begriffs-Klärung „optional" (korr. 2026-06-03, s. Doc 30 §8.0):** „optionales Sub-Interface" oben meint AUSSCHLIESSLICH die
> C++-ABI-Mechanik — ob ein konkretes Modul `IObservableTier`/`IMeasurableWorkload` über `dynamic_cast` anbietet (Mess-Exposure
> über die .dll-Grenze). Das ist **KEINE** „optionale Achse": Achsen sind nie optional. Alle 17 Organe der Lebewesen-Unterklasse werden
> in jedem Lebewesen-Binary uniform getrieben; eine nicht-puffernde/nicht-prefetchende Lebewesen-Unterklasse wählt einen konkreten
> Durchreich-Algorithmus (NoBuffer/NoFlush/NonePrefetch/NoMigration/None), sie lässt die Achse nicht weg. `measure_genus_sequential`
> / `…→genus()` referenzieren das **Gattungs-Interface/Prüf-Dock** (Dock-Diskriminator) — korrekter Gattungs-Gebrauch, bleibt.

**Kern-Kante — Dreifach-Vererbung:**
`SearchAlgorithmAbiAdapter<A> : public IAnatomyBase, public IMeasurableWorkload, public IObservableTier` (`abi_adapter.hpp:75`). `IAnatomyBase : public IExecutionEngine`. `IMeasurableWorkload` und `IObservableTier` hängen **bewusst NICHT** an `IAnatomyBase` — sonst würde sich dessen vtable-Layout ändern und alte DLLs brechen.

**Pfad-A/B-Koexistenz im Adapter:** `search_organ_` (echtes seziertes Such-Organ, uint64-Key, persistent, Pfad B) **+** `container_` (`ObservableComposedSearch`-Wrapper, allocator-messend). Pfad A (`run_workload`) erzeugt einen lokalen Wegwerf-Algo; Pfad B (`tier_*`) treibt `search_organ_`. Keine Interferenz.

---

## 4. DER KONVERGENZPUNKT — alles, was die .dll-Grenze quert

Zwei getrennte ABI-Ebenen treffen sich hier:
- **module_abi_v1** = die 4 extern-`C`-Symbole + `IAnatomyBase`-vtable (Pflicht, „lebend" für *jedes* Modul).
- **anatomy-ABI-Erweiterung** = die per `dynamic_cast` erreichbaren Sub-Interfaces + der Snapshot-POD (teils „dormant", weil heute nur die Snapshots der zwei getriebenen Achsen über die .dll-Grenze exportiert werden).

> **(korr. 2026-06-03, s. Doc 30 §8.0):** „dormant" / „2 getriebene Achsen" beschreibt, WELCHE Achsen-Snapshots heute die ABI-Grenze
> queren — NICHT, dass die übrigen 15 Achsen im Lebewesen fehlen oder optional sind. In-Process trägt die Lebewesen-Unterklasse alle 17 Organe
> uniform; die nicht-exportierten Achsen sind nur (noch) nicht über die ABI sichtbar. Keine Achse ist optional.

| # | Name | Datei:Zeile | Heutiges Layout / Signatur | Ebene | Status |
|---|---|---|---|---|---|
| 1 | `comdare_create_anatomy` | `anatomy_module_abi_v1_decl.hpp:66` | `() -> IAnatomyBase*` (Ownership zum Caller) | module_abi_v1 | **lebend** |
| 2 | `comdare_destroy_anatomy` | `…:73` | `(IAnatomyBase*) -> void` (muss in gleicher DLL) | module_abi_v1 | **lebend** |
| 3 | `comdare_anatomy_abi_version` | `…:61` | `() -> uint64_t` (`major<<32 \| minor`) | module_abi_v1 | **lebend** |
| 4 | `comdare_anatomy_abi_magic` | `…:64` | `() -> uint64_t` = `0x434F4D444141312E` | module_abi_v1 | **lebend** |
| 5 | `IExecutionEngine`-vtable | `execution_engine_base.hpp:98` | 7 virt. (engine_name/kind/lifecycle_state/warm_up/run/reset/shutdown) + dtor | module_abi_v1 | **lebend, eingefroren** |
| 6 | `IAnatomyBase`-vtable (Erweiterung) | `anatomy_base.hpp:99` | +4 virt. (composition_name/paper_id/genus/organ_count) | module_abi_v1 | **lebend, eingefroren** |
| 7 | `IAnatomyBase::genus()` | `anatomy_base.hpp:99` | `() -> AnatomyGenus` (uint8) — **Dock-Diskriminator** | module_abi_v1 | **lebend** |
| 8 | `ComdareTierObserverSnapshotV1` | `observable_tier.hpp:40-59` | **13×uint64** (6 search + 5 allocator + 2 meta), standard_layout + trivially_copyable (static_assert :61-64) | anatomy-ABI | **lebend** (2/17 Achsen) |
| 9 | `IObservableTier`-vtable | `observable_tier.hpp:80-106` | 6 virt.: `tier_insert(u64,u64)→bool`, `tier_lookup(u64,u64*)const→bool`, `tier_erase(u64)→bool`, `tier_clear()`, `tier_size()const→u64`, `tier_observe(Snapshot*)const` | anatomy-ABI (Sub) | **lebend** (per dynamic_cast) |
| 10 | `IMeasurableWorkload`-vtable | `measurable_workload.hpp:20` | 1 virt.: `run_workload(u64,u64,u64,int64*,u64)→u64` | anatomy-ABI (Sub) | **lebend** (per dynamic_cast) |
| 11 | `kTierObserverSnapshotVersion` | `observable_tier.hpp:67` | `constexpr uint32_t = 1` | anatomy-ABI | **lebend** |
| 12 | `ObserverAggregate<C>` (17 Slots) | `observer_aggregate.hpp:94` | 17-Member-POD, kompositions-typisiert | **NICHT über ABI** | **dormant** (in-process only) |
| 13 | 15 nicht-getriebene Achsen-Snapshots | (in #12 enthalten) | latent in ObserverAggregate, nicht in Snapshot V1 | — | **dormant** |

> **Layout-Korrektur ggü. Zwischen-Inventar:** Der Snapshot hat **13 Felder** (6 search + 5 alloc + `observable_axis_count` + `tier_fill_level`), verifiziert an `observable_tier.hpp:42-56`. I1 muss mit 13 (V1) rechnen; V2 wächst darüber hinaus.

**Kritische Reihenfolge-Invariante:** `AnatomyModuleHandle::unload()` ruft `destroy_fn` **vor** `dlclose` (`anatomy_module_loader.hpp:76`). Umgekehrt = UAF + Heap-Corruption.

**Versions-Logik:** `AnatomyAbiVersion::host_compatible_with(module)` = `major==host.major && module.minor <= host.minor` (`anatomy_module_abi_v1_decl.hpp:85`). Modul darf **alt** sein, nicht zukünftig.

---

## 5. I1-Konsequenz — Vereinheitlichung zu EINEM Mess-POD + ANATOMY_ABI_MAJOR-Bump

**Stoßrichtung (User-Entscheidung „EIN einziger ABI-POD"):** den heute gattungs-/getrieben-spezifischen
`ComdareTierObserverSnapshotV1` (2 von 17 Achsen) durch *einen* einheitlichen, alle relevanten Achsen + die 6 HW-Counter
tragenden Mess-POD (`…SnapshotV2`) ersetzen und `ANATOMY_ABI_MAJOR` von 1 auf 2 erhöhen.

### 5a. Anzufassende Typen/Konstanten/Dateien (Definitions-Seite)

- [ ] **Neuer Snapshot-Typ** `ComdareTierObserverSnapshotV2` neben V1 in `observable_tier.hpp:40` — V1 **nicht** mutieren (Layout-Versionierung; V1 = deprecated Regressions-Typ). Beide `static_assert` (standard_layout + trivially_copyable, :61-64) für V2 duplizieren.
- [ ] **Version hochziehen:** `kTierObserverSnapshotVersion` `observable_tier.hpp:67` → 2.
- [ ] **`COMDARE_ANATOMY_ABI_MAJOR`-Bump** (`anatomy_module_abi_v1_decl.hpp:28`) 1→2.
- [ ] **Magic-Konstante** `0x434F4D444141312E` (`…:32/:64`) — bei Major-Bump auf neuen Wert (".A1." → ".A2.") setzen. **[ANNAHME: Magic kodiert Versionsstring]**
- [ ] **`IObservableTier::tier_observe`** Signatur `observable_tier.hpp:105` auf V2-POD umstellen (oder überladenes `tier_observe_v2` zusätzlich, um alte Loader-Pfade nicht zu brechen).
- [ ] **`AnatomyAbiVersion`/`kHostAnatomyAbiVersion`** `anatomy_module_abi_v1_decl.hpp:85,107` — `host.major` zieht auf 2 mit (keine Logik-Änderung).
- [ ] **dormanten Zwilling auflösen:** `comdare_hw_counters_v1`/`pull_live_counters` (`module_abi_v1.hpp:45-51,114`) → in den einen POD überführen / als deprecated markieren.

### 5b. Mitzuziehende Konsumenten

- [ ] **Adapter:** `SearchAlgorithmAbiAdapter::tier_observe` `abi_adapter.hpp:251` — befüllt heute 13 Felder; muss die neuen Felder aus `search_organ_` + `container_` (+ ggf. weiteren getriebenen Organen + HW-Counter host-seitig) spiegeln.
- [ ] **dynamic_cast-Stelle + Verbraucher:** `SearchAlgorithmDock::measure()` `search_algorithm_dock.hpp:22` — `dynamic_cast<IObservableTier*>` bleibt, Snapshot-Verbraucher liest V2.
- [ ] **Treiber:** `drive_tier_observe_trace_abi` `tier_observe_trace_abi.hpp:77` — Snapshot-Korrelation + CSV/JSON-Spalten erweitern; `max_insert_stagnation`-Robustheit unberührt.
- [ ] **Loader:** `AnatomyModuleLoader::load` `anatomy_module_loader.hpp:149` — Major-Mismatch greift für alte V1-Module (sauberer Reject); entscheiden ob V1-Fallback-Pfad oder harter Reject.
- [ ] **Registry/Sequencer:** `PruefDockRegistry::select_for` + `measure_genus_sequential` — Dispatch über `genus()` (ABI-fest) unverändert, nur Snapshot-Payload ändert sich.
- [ ] **Tests:** alle, die V1 per `operator==` (`observable_tier.hpp:58`) vergleichen — neue V2-Fixtures, V1-Tests als Regressionsschutz behalten.
- [ ] **Makro:** `COMDARE_DEFINE_ANATOMY_MODULE(C)` — falls Versions-/Magic-Werte eingebettet, mitziehen; emittiert die 4 Symbole als `new SearchAlgorithmAbiAdapter<…>`.
- [ ] **Alle Permutations-DLLs** via `comdare_perms_all` neu generieren+bauen (sonst Major-Reject).

### 5c. Was NICHT angefasst wird
- `IExecutionEngine`-vtable (`execution_engine_base.hpp:98`) — eingefroren seit Rev5.
- extern-`C`-Factory-Signatur `comdare_create_anatomy() -> IAnatomyBase*` (`…:66`) — bleibt; der V2-POD ist DLL-intern, kein neues Public-Symbol.
- `genus()`-Diskriminator und die 5 `AnatomyGenus`-Werte (`anatomy_base.hpp:44`).

---

## 6. Risiken / offene Fragen (ehrlich — vor I1 zu entscheiden)

1. **vtable-Layout-Stabilität von `IAnatomyBase`.** Jede *weitere* virtuelle Methode an `IAnatomyBase` ist ein Hard-Break. I1 darf neue Mess-Fähigkeiten **nur** über Sub-Interfaces (`IObservableTier`/`IMeasurableWorkload`) oder einen neuen POD einführen, nicht durch Anhängen an die Wurzel-vtable. **Offene Frage:** Major-Bump nutzen, um die zwei Sub-Interfaces in `IAnatomyBase` zu *konsolidieren* (sauberer, bricht jede alte DLL hart) — oder `dynamic_cast`-Degradation erhalten (kompatibler, aber dualistisch)?

2. **Zwei koexistierende ABIs.** Die Observer-Fähigkeit ist *nicht* im Versions-Handshake kodiert, sondern wird per `dynamic_cast` erprobt → ein Modul kann „ABI-kompatibel" sein und `tier_observe` trotzdem nicht können (`nullptr` → `dock_status_subinterface_missing`). **Offene Frage:** Observer-Fähigkeit Teil des Versions-Handshakes machen (Loader-Erweiterung) oder reines Runtime-Probing?

3. **Dreifach-Vererbungs-Reihenfolge.** `: IAnatomyBase, IMeasurableWorkload, IObservableTier` (`abi_adapter.hpp:75`) bestimmt Sub-Object-Layout + `dynamic_cast`-Offsets. Reihenfolge-Änderung → alle gebauten DLLs binär inkompatibel, selbst bei gleichem Major. **[ANNAHME]:** Reihenfolge einfrieren, oder Änderung zwingt zu Major-Bump *und* Voll-Rebuild (ohnehin I1-Plan).

4. **„Dormante" 15 Achsen-Snapshots** (korr. 2026-06-03, s. Doc 30 §8.0: „dormant" = noch nicht über die ABI-Grenze exportiert, NICHT „Achse fehlt/optional" — alle 17 Organe sind in-process Pflicht). Will I1 die Snapshots *aller* 17 Achsen über die Grenze tragen, wächst der POD; bei komposition-typisiertem Transport variierte das Layout je Composition → bräche `static_assert(standard_layout)`. **Empfehlung:** **fester `uint64`-Slot-Block je Achse** (V2, ABI-stabil, `trivially_copyable` erhalten) statt komposition-typisierter Transport. Pro-Achse-Slots können für (noch) nicht über die ABI exportierte Achsen 0 bleiben (wie heute Wall-Clock/Observer-Degradation) — was die Pflicht-Existenz des Organs im Lebewesen nicht berührt.

---

### Schlüssel-Pfade (Kurzreferenz für I1)
- Snapshot/Sub-Interfaces: `libs/cache_engine/anatomy/observable_tier.hpp:40,67,80,105`
- Workload-Sub-Interface: `libs/cache_engine/anatomy/measurable_workload.hpp:20`
- Adapter (Dreifach-Vererbung): `libs/cache_engine/anatomy/abi_adapter.hpp:75,251`
- Wurzel/Gattungs-Interface + Lebewesen-Unterklassen-Diskriminator (korr. 2026-06-03): `libs/cache_engine/execution_engine/execution_engine_base.hpp:98` · `libs/cache_engine/anatomy/anatomy_base.hpp:44,76,99`
- ABI-Decl (4 Symbole + Version/Magic): `libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp:28,32,61,64,66,73,85`
- Loader/Handle: `libs/cache_engine/builder/anatomy_module_loader/anatomy_module_loader.hpp:76,149`
- Prüf-Dock: `libs/cache_engine/builder/pruef_dock/pruef_dock.hpp:51` · `search_algorithm_dock.hpp:22` · `pruef_dock_registry.hpp:22` · `pruef_dock_sequencer.hpp:44`
- Treiber: `libs/cache_engine/builder/anatomy_commands/tier_observe_trace_abi.hpp:77`
- In-process-Aggregat (dormant): `libs/cache_engine/anatomy/observer_aggregate.hpp:26,40,64,94`
- Dormanter Zwilling (module_abi_v1): `libs/cache_engine/include/cache_engine/abi/module_abi_v1.hpp:45-51,114`
