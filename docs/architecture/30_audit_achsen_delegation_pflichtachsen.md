# 30 — Audit: Achsen-Delegation + Pflicht-Achsen pro Tier (2026-06-03)

**Anlass (User-Einwand 2026-06-03):** „Ich bin davon ausgegangen, dass jedes Tier durchgängig alle und immer dieselben
Achsen-Interfaces verwendet wie alle anderen Tiere, also Pflicht immer alle 17 plus die externen 5. Jeder Tier-Binary-
Algorithmus besteht IMMER aus den klassischen Bestandteilen eines Suchalgorithmus. Ich sehe hier einen Verstoß gegen
sorgfältig geplante Architektur und Experiment-Ablauf."

**Methode:** drei tiefenlesende Agenten (Pflicht-Achsen-Vollständigkeit · Sezierungs-/Delegations-Audit · Web-Recherche
kanonischer Suchalgorithmus-Bestandteile) + eigene Code-Verifikation der Kernstelle. **Ergebnis: der User hat in der
Substanz recht — es liegt ein echter Architektur-Verstoß vor, aber an anderer Stelle als die reine Achsen-Zählung.**

---

## Befund 1 — Achsen-ZAHL pro SearchAlgorithm-Tier: 17 Organe + 3 Build = 20 (nicht 22)

Belegt (Agent 1, file:line): das Makro `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT` (`abi/anatomy_module_abi_v1.hpp:87-89`)
expandiert zu `AdHocComposition<__VA_ARGS__>` — **hart auf exakt 17 Slots fixiert** (`composition_factory.hpp:41-85`,
`static_assert sizeof...(Vs)==17`; `composition_concept.hpp:18-61`) — PLUS einem separaten Inspektions-Symbol für die
3 Build-Achsen (page_type/simd_extension/general_hardware; `build_variant_inspection.hpp:17-22`).

Die „22" (`axis_observer_classification.hpp:42-66`, literal getestet `test_br3_obs22.cpp`) = **17 SearchAlgorithm-Slots
+ 3 Definition (Build) + 2 Container (queuing q1/q2)**. Die 2 Container-Achsen gehören per `genus_binding_traits.hpp:53-68`
zur **eigenständigen Container/Adapter-Gattung** (eigene Tier-Binary, 2 Slots), NICHT zu SearchAlgorithm — Cross-Genus-
Joins sind „type-system-mathematisch unmöglich" (Doc 28 §0). Per-Gattung-Slots: SearchAlgorithm 17, Container 2, Set 15,
Sequence 11, View 7.

**Teil-Ergebnis:** Auf der reinen ZÄHLUNG ist das aktuelle Makro (17+3) für ein SearchAlgorithm-Tier vollständig; „22
pro Tier" widerspricht der code-erzwungenen Gattungs-Invariante. **ABER das ist nur die strukturelle Hälfte der Frage —
und nicht die, die der eigentliche Verstoß betrifft.** Ob queuing/Pufferung (q1/q2) konzeptionell in ein Suchalgorithmus-
Tier gehört (User-Sicht: ja — vgl. LSM-/B-Tree-Insert-Buffer) oder in eine getrennte Gattung (aktueller Stand) ist eine
**offene Design-Entscheidung** (s. §4 Q1).

---

## Befund 2 — ECHTER VERSTOSS: die Such-Organe umgehen die Speicher-Achsen (Monolithen)

Belegt (Agent 2 + eigene Verifikation an `abi_adapter.hpp:504-517`):

Der `SearchAlgorithmAbiAdapter` hält **zwei getrennte, unverbundene Speicher**:
- `SearchAlgo search_organ_{};` (`:516`) — das **monolithische Such-Organ** (z. B. `Array256SearchAlgo` mit eigenem
  `std::array<…,256> data_`, `axis_03a_search_algo_array256.hpp:134`, eigener `uint8`-Key `:46`). Es nimmt KEINEN
  node_type/layout/allocator-Slot, delegiert an NICHTS.
- `container_t container_{};` (`:517`) — ein **separater** `ObservableComposedSearch<SortedBinaryTraversal, ComposedStore<…>>`
  mit **hart verdrahtetem** `SortedBinaryTraversal` (`:508`, NICHT dem `Composition::search_algo`!), nur da, „um die
  ALLOCATOR-Achse über die ABI-Grenze zu messen".

`tier_insert` treibt BEIDE getrennt (`:279-280`, Doppel-Buchführung). Die Such-Metriken (`peak_occupancy`,
`tier_fill_level`) kommen aus dem **Monolith** (`search_organ_.statistics()`, `:313-334`); nur `alloc_*` kommt aus dem
ComposedStore. Zusätzlich verwendet `ComposedStore` `node_type` nur als constexpr-Deko (`N::max_capacity()`,
`axis_04_node_type_composed_store.hpp:67`), der reale Speicher ist ein unbounded `std::vector` (`:143`) → node-Kapazität
ohne Laufzeitwirkung. Und der V1-Mess-POD (`observable_tier.hpp:42-61`, von `perm_runner` genutzt) trägt **kein einziges
node_*-Feld** (die existieren nur im nie aufgerufenen V2-POD).

**Konsequenz:** Die Tiere durchlaufen **NICHT uniform alle 17 Organ-Interfaces**. Das Such-Organ beschattet node_type/
memory_layout. Das Sezierungs-Prinzip (Doku 14 §3.1: „Achse = Organ; ein Such-/Traversal-Organ besitzt KEINEN eigenen
Speicher; Algorithmus = Komposition") ist im aktiven Mess-Pfad **verletzt**. Der Task-#42-Anspruch („Umstufung-B: ALLE
axis_03a-Tiere seziert → Gattungs-Konfiguratoren") ist als Gerüst vorhanden (`composable_search.hpp`), aber im gemessenen
Pfad **nicht verdrahtet** — die Monolithen sind unverändert in Gebrauch.

**Korrektur der Mess-Doku:** Mein „Befund B = ehrliche Null-Kontrolle / reale Achsen-Orthogonalität"
(`docs/sessions/20260603-l-meas-thesis-searchalgo-tiere.md`) war eine **Fehldeutung** — die node-Invarianz ist der
Abdruck dieses Verstoßes (Tautologie), kein empirischer Befund. Auch Befund A misst die *interne Eigenspeicher-Struktur
der Monolithen*, nicht ein in node/layout/allocator zerlegtes Organ-System.

---

## Befund 3 — Web-Validierung: die 17-Achsen-Zerlegung ist literatur-treu (mit 2 Lücken)

Belegt (Agent 3, Quellen: Knuth TAOCP Vol. 3; CLRS; Hellerstein et al. „GiST" VLDB 1995; Leis et al. „Adaptive Radix
Tree" ICDE 2013; Graefe „Modern B-Tree Techniques" 2011 + „B-Tree Locking Survey" TODS 2010; Kim et al. „FAST" SIGMOD
2010; Kraska et al. „Learned Index Structures" SIGMOD 2018; Petrov „Database Internals" 2019; Ross/Rao „CSB+-Trees"
SIGMOD 2000; Broneske et al. „SIMD for Index Structures" 2018; Garcia-Molina/Ullman/Widom; Ramakrishnan/Gehrke):

- **14 von 16 kanonischen Bausteinen** sind 1:1 als Achse abgebildet. Das GiST-Theorem („common search-tree skeleton +
  trennbare typ-spezifische Methoden") liefert das **theoretische Fundament** für die Achsen-Zerlegbarkeit überhaupt.
- **Minimale Pflicht-Organe (nicht-degenerierbar):** (C1) Such-/Navigations-Strategie · (C3) Schlüssel→Position-Mapping ·
  (C4) Knoten-/Speicher-Organisation · (C5) Wert-/Payload-Handling. Alle übrigen (Layout, Allokation, Prefetch,
  Concurrency, Serialisierung/IO, SIMD/ISA, Telemetrie, Filter, Migration) sind **Pflicht-Dimensionen mit erlaubtem
  Trivial-Default** — aber sie dürfen NICHT durch einen Monolith *umgangen* werden.
- **2 echte Lücken:** (G1) **Key-Normalisierung/-Transformation** (order-preserving key transform; bei ART/Graefe ein
  erstklassiger Baustein) hat keine saubere eigene Achse. (G2) **Struktur-Erhaltung/Split-Merge-Policy** — `axis_02`
  deckt nur Pfad-Kompression, nicht das kanonische Reorganisations-Organ (B-Tree-Split/Merge, GiST-PickSplit/Penalty).
- **Über-Zerlegungs-Hinweis:** `axis_03b`/Prefetch/Memory-Layout sind in FAST drei Facetten EINER Latenz-Optimierung
  (eng gekoppelt); „Hardware-Profil" + „Telemetrie" sind Kontext/Querschnitt, keine Such-Organe.

**Teil-Ergebnis:** Die User-Intuition „jeder Suchalgorithmus besteht aus klassischen Bestandteilen" ist literatur-
gestützt. Die 17-Zerlegung ist treu und sogar reicher als die Lehrbuch-Sicht; minimal müssen C1/C3/C4/C5 **echt** (nicht
umgangen) sein — genau das verletzt Befund 2.

---

## §4 — Synthese: zwei Lesarten von „Pflicht-Achsen" + zwei offene Entscheidungen

Der User meint die **operative** Lesart, die Architektur erfüllt nur die **strukturelle**:

| Lesart | Aussage | Status |
|---|---|---|
| **strukturell** | jedes SA-Tier *instanziiert* die 17 Slots (+3 Build) | ✅ erfüllt (Makro) |
| **operativ** | jedes Tier *routet durch* alle 17 Organ-Interfaces (kein Eigenspeicher-Bypass) | ❌ **verletzt** (Befund 2) |

**Offene Entscheidung Q1 (Design, Achsen-Zahl):** Sollen queuing/Puffer-Organe (q1/q2) Teil JEDES SearchAlgorithm-Tiers
sein (User-Sicht „17+5=22 uniform", literatur-plausibel via Write-Buffer/LSM) — oder bleibt die aktuelle Gattungs-
Trennung (q1/q2 = eigene Container-Gattung)? Ersteres bräuchte einen Bruch der `AdHocComposition<17>`-Invariante
(→ `<22>` o. ä.) ODER eine Neudefinition, was „SearchAlgorithm-Anatomie" umfasst. **User-Entscheidung.**

**Offene Entscheidung Q2 (Bug-Fix, der eigentliche Verstoß):** echte Delegation der Such-Organe an die Speicher-Achsen
herstellen. Fix-Plan (Agent 2, 4 Schritte):
1. **Such-Organe als Traversal-Organe rekonstruieren** (statische `insert_into`/`lookup_in`/`erase_from` auf einem
   übergebenen `Store`), Vorbild `composable_search.hpp:65-112` (`LinearScanTraversal`/`SortedBinaryTraversal`). Array256
   wird ein Direkt-Adress-Traversal über `ComposedStore<Node256,…>`; die „256" stammt dann aus dem node-Organ.
2. **Adapter auf EINEN Speicher** (`search_organ_` entfällt; `container_` trägt das ECHTE `Composition::search_algo`-
   Traversal statt des hart verdrahteten `SortedBinaryTraversal`). Such- UND Speicher-Metriken aus DEMSELBEN Objekt.
3. **ComposedStore bounded** (`N::max_capacity()` als hartes Limit / node-getragene Slot-Anordnung), damit node_type
   Laufzeitwirkung erhält.
4. **perm_runner auf V2-POD** (`tier_observe_v2` mit node_*-Feldern), sonst bleibt der node-Effekt unsichtbar.

**Habich-Konflikt (zu beachten):** Die Direktive „Original-Paper-Code immer wenn vorhanden" (`feedback_paper_original_code_pattern`)
könnte verlangen, dass manche Organe ihren Original-Algorithmus monolithisch behalten. Dann braucht es eine Brücke
(Original-Organ AS-A Traversal-Adapter über den Store) statt Neuimplementierung — Klärung mit User.

---

## §5 — Empfehlung

1. **Q2 ist ein Bug, kein Geschmack** — die echte Delegation sollte hergestellt werden, sonst ist das Experiment
   (Achsen einzeln messbar) für axis_04/05 nicht aussagekräftig. Empfohlener Einstieg: **vertikaler Beweis-Schnitt** —
   EIN echtes delegierendes Tier (ComposedSearch über node_type) bauen + messen, das zeigt „node_type wirkt jetzt",
   bevor alle 5 Organe + der Adapter umgebaut werden.
2. **Q1 ist eine echte Design-Frage** für den User (Gattungs-Trennung beibehalten vs. queuing in jedes SA-Tier).
3. **Mess-Doku** ist bereits ehrlich korrigiert (Befund B = Verstoß-Abdruck, nicht Orthogonalität).
4. **Lücken G1/G2** (Key-Normalisierung, Split-Merge-Policy) als Achsen-Kandidaten vormerken (separat von Q2).

---

## §6 — Umsetzung (2026-06-03, User-Auftrag „beide Punkte durchführen")

**Q2 Schritt 1–3 UMGESETZT + im echten Mess-Pfad VERIFIZIERT (literal):** Der `container_` im `SearchAlgorithmAbiAdapter`
ist von der unbounded `ComposedStore` auf die node-wirksame **`NodeChunkedStore<N,L,A>`** umgestellt (`abi_adapter.hpp`
container_t + Include `axes/node/axis_04_node_type_chunked_store.hpp`). `NodeChunkedStore` speichert in node-großen
Chunks (cap=`N::max_capacity()`) und meldet `allocator_statistics().allocation_count = ceil(size/cap)` → node-abhängig.
Belegt (perm_runner, alle 8 thesis-Tiere neu gebaut + gemessen, `build/thesis_tiere/thesis_measurements.csv`):

| node_type (axis_04, Such-Organ fix Array256) | alloc_cnt @ n=1000 | @ n=2000 | @ n=4000 |
|---|---|---|---|
| Node4   | 250 | 500 | 1000 |
| Node16  | 63  | 125 | 250  |
| Node48  | 21  | 42  | 84   |
| Node256 | 4   | 8   | 16   |

**Vorher (Befund 2):** alloc_cnt über alle Node-Varianten identisch (18) — node_type inert. **Jetzt:** `ceil(n/node_cap)`,
node-abhängig → die Speicher-Achsen node_type/layout/allocator sind im gemessenen Pfad real wirksam. Zusätzlich
standalone-Beleg `test_node_delegation_proof` (ComposedSearch über NodeChunkedStore, PROOF_OK).

**Q2 Schritt 4 (perm_runner→V2-POD) + volle Such-Organ-Delegation (search_organ_ entfällt):** OFFEN — die SEARCH-Zähler
(peak/fill/lookup) kommen weiter aus dem Monolith `search_organ_`; die STORAGE-Achsen delegieren jetzt korrekt. Die
restliche Vereinheitlichung (Such-Strategie ebenfalls als Traversal über DENSELBEN Store) ist der verbleibende Teil.

**Q1 (22 uniform pro Tier) — SAUBERE Migration (User-Direktiven 2026-06-03: „kein doppeltes Topic" + „keine Achse
thematisch / kein Topic konzeptionell doppelt"):** `AdHocComposition<17>→<19>` mit queuing q1/q2 als ECHTE, explizite
Slots 17/18 (dasselbe queuing-Topic axis_q1/axis_q2 — NICHT dupliziert, KEIN Auto-Anhängen). Jedes Tier wählt q1/q2
explizit (wie die übrigen 17). **Die Adapter/Container-Gattung wird ENTFERNT** (ihr einziger Zweck = queuing-als-eigene-
Gattung → würde das queuing-Konzept doppeln). So existiert queuing genau EINMAL (als SA-Slots).

### §7 — Migrationsplan (atomar, ~30 Dateien)

> ⚠️ **DIESER §7-PLAN IST REVIDIERT/VERWORFEN — s. §8 (Gattungs-Kategorienfehler-Audit 2026-06-03).** Er ruht auf einem
> Kategorienfehler (queuing-Achse als Gattung bzw. als Pflicht-Slot). NICHT ausführen. Korrektes Modell in §8.

**A. Kern-Komposition 17→19:**
- `anatomy/composition_factory.hpp` — AdHocComposition T0..T18 + `using queuing_q1=T17; queuing_q2=T18;`; Helper-static_assert ==19.
- `anatomy/composition_concept.hpp` — IsComposition + `typename C::queuing_q1/queuing_q2`; `composition_organ_count = 19`.
- `anatomy/search_algorithm_anatomy.hpp` — organ_count()-Bezug 17→19.
- `anatomy/observer_aggregate.hpp` — 19 Achsen iterieren (falls 17 hartkodiert).
- `builder/codegen/all_axes_umbrella.hpp` — Achsen-Typ-Liste/Makro 17→19 (+ queuing-Aliase).
- `builder/experiment_tree/axis_path_serialization.hpp` — `kCompositionAxisNames` +`"queuing_q1","queuing_q2"` (→19).
**B. Achsen-Klassifikation (22 bleibt: jetzt 19 SA-Composition + 3 Build, KEINE 2 Container):**
- `builder/experiment_tree/axis_observer_classification.hpp` — 17→19 SearchAlgorithmObserver, 2 ContainerObserver entfallen.
- `builder/experiment_tree/registry_to_axis_levels.hpp` — Baum-Ebenen 22 (19+3) statt 17+3+2.
**C. Tier-Quellen (16, je +2 explizite queuing-Organe T17/T18):** `tests/unit/{genus_adhoc_buildvariant, genus_buildvariant_*,
genus_module_*, thesis_tiere/thesis_*}.cpp` + `auto_emitted_perm_module.cpp`.
**D. Registry/Codegen:** `builder/experiment_tree/composition_registry.hpp` (PermTuple<19>), `builder/codegen/adhoc_emitter.hpp` (emittiert 19).
**E. Gattung-Generik:** `builder/experiment_tree/genus_binding_traits.hpp` — SA slot_count 19 + names; **Adapter-Spezialisierung
ENTFERNT**; `test_genus_binding` 5→4 Gattungen.
**F. Adapter/Container-Gattung ENTFERNEN (Absorption):** `anatomy/container_{anatomy,tier,abi_adapter,composition}.hpp`
löschen/entkernen; `anatomy/anatomy_base.hpp` AnatomyGenus::Adapter (Enum-Wert) bereinigen; CMake-Targets
`perm_container_*`/`test_d4b`/Container-Teil von `test_dgenus_dll` entfernen.
**G. Tests anpassen:** `test_br3_obs22` (19+3, keine 2 Container), `test_br1_full22_count` (22 = 19+3), Container-Tests entfernen.
**Verifikation:** Mess-Pfad `build_and_measure_thesis_tiere.ps1` (Tiere jetzt 19 Achsen) + `cmake --build` grün.

---

## §8 — KORREKTUR: Gattungs-Kategorienfehler (Agenten-Audit 2026-06-03, User-Einwand)

**Anlass (User 2026-06-03):** „Wie kann ein Modulbaustein wie queuing eine Gattung sein? Gattungen waren die Ebene von
Suchalgorithmen oder Container, jeweils eine statische Konfiguration an Achsen-Compounds mit austauschbaren Algorithmen
je Achse." → tiefenlesender Agent (Doku 14 §7/§25/§27/§28/§32 + Code + Doc 27-30). **Verdikt: Kategorienfehler bestätigt;
§6-Q1 + §7 (oben) sind die falsche Auflösung.**

**Befund (mit Belegen):**
- **Gattung = Datenstruktur-ART mit festem Achsen-Satz** (Doku 14 §32.1 `:1249`: „Gattungen verwenden die exakt selben
  permutativen Achsen"). 5 Gattungen (Tier-Metapher): SearchAlgorithm(17)/Set(15)/Sequence(11)/**Adapter**/View(7).
- **`AnatomyGenus::Adapter` ist im IST ein Kategorienfehler:** `ContainerComposition` hat nur 2 Slots = q1 buffer + q2
  flush (`container_anatomy.hpp:54-79`); `put/get/size/clear` sind 1:1-Pass-Through auf das q1-Organ `FIFOQueueBuffer`,
  das bereits die volle Container-API + eigenen `std::deque` hat (`axis_q1_queuing_fifo.hpp:56-108`). Die „Gattung" ist
  also nur eine Hülle um EINE Achse — **queuing-Achse zur Gattung erhoben.** Das WAHRE Adapter-Wesen (Doku 14 §28 `:1122`,
  §32.2 `:1264`) = Decorator über **`axis_inner`** (Inner-Container) + delegierte Standard-Achsen + ordering — fehlt komplett.
- **Ursprung des Fehlers:** Doc 27 §0.1 (2026-06-02) re-interpretierte eine User-Aussage fälschlich zu „queuing = eigene
  Gattung", unter Zweckentfremdung von §32 (Cross-Genus verbietet das KREUZEN von Gattungen — macht aber keine Achse zur
  Gattung). **Doku 14 §7 (`:270-271`) sagte korrekt „q1/q2 = Organe" — das ist REHABILITIERT.**
- **Mein §7-Plan (queuing→Pflicht-SA-Slots + Adapter löschen) ist ebenfalls falsch:** queuing ist KEIN Pflicht-Organ
  (Doc-30-§3-Web-Befund: Pflicht-Kern = Such-Strategie/Mapping/Node/Value; ART/HOT/SuRF haben keinen Write-Buffer) und die
  legitime Adapter-Gattung darf nicht gelöscht werden.

**KORREKTES Modell (ersetzt §6-Q1 + §7; inkl. User-Korrekturen #3/#4 2026-06-03):**

**0. DREI EBENEN (User-Präzisierung #4 — „damit der Punkt mit den Gattungen nicht falsch wird"; Doku 14 §25 verbatim + §26):**
   - **(a) Gattung = Außen-Interface** — was die Außenwelt sieht: **SearchAlgorithm / Container / Graph** (Doc 24 „ABI-
     stabiles Interface für die API der Gattung"; Doku 14 §25 „Suchalgorithmen und Container gehören zu unterschiedlichen
     Gattungen"). Coarse, schnittstellen-definierend.
   - **(b) Tier-Unterklasse** = liegt UNTER dem Interface und verwendet einen **FESTEN Achsen-Satz** (Doku 14 §25 „die
     Permutation Engine … durch Unterklassen … spezifiziert"; §26 die std-Familien A–F). Hier lebt die feste Achsen-
     Konfiguration — NICHT auf der Gattungs-Ebene.
   - **(c) Achsen** = Organe der Tier-Unterklasse; **alle Pflicht + in jedem Tier-Binary uniform getrieben** (User-Korr. #3);
     je Achse ein austauschbarer Algorithmus (inkl. Durchreich-Algorithmen wie NoBuffer/NonePrefetch).
   - **AKTUELLER STAND:** wir definieren erstmal **NUR EINE Tier-Unterklasse** (unter dem SearchAlgorithm-Interface,
     std::map-ähnlich), für die ALLE Achsen Pflicht sind. (Die code-seitigen „5 AnatomyGenus" SearchAlgorithm/Set/Sequence/
     Adapter/View bzw. Doku-14-§26 „Gattung A–F" benennen Container-Tier-Unterklassen lose als „Gattung" — per dieser
     Präzisierung sind sie Tier-Unterklassen der Container-Gattung; nur die SearchAlgorithm-Tier-Unterklasse ist heute gebaut.)
   - **Folge für queuing:** queuing ist eine **Pflicht-Achse der (aktuellen SearchAlgorithm-)Tier-Unterklasse** — kein
     Interface, keine Gattung. Der „Adapter = queuing-Gattung"-Fehler war DOPPELT falsch: (i) queuing ist keine Gattung;
     (ii) selbst die Container-Adapter (std::queue/stack/priority_queue, Doku 14 §26.4) wären eine **Tier-Unterklasse der
     Container-GATTUNG** (axis_inner + ordering), nicht eine „queuing-Gattung".

1. **Achsen einer Tier-Unterklasse sind NIE optional** (User-Korrektur #3 — verwirft das „optional"-Framing des Agenten): JEDE Achse
   ist in JEDEM Tier-Binary präsent, ihr Interface wird UNIFORM getrieben. queuing ist also eine **reguläre, mandatorische
   SA-Achse** — kein „optionaler Slot". Ein Tier, das nicht puffert, wählt den KONKRETEN Algorithmus `NoBuffer`/`NoFlush`
   („ein Algorithmus, der eigentlich nicht queued", durchreicht) — die Achse + ihr Interface bleiben present + uniform
   getrieben. Exakt analog prefetch=`NonePrefetch`, filter=`None`, migration=`NoMigration`: das sind konkrete Algorithmen,
   KEINE abwesenden/optionalen Achsen.
2. **queuing q1/q2 als reguläre SA-Slots 17/18** — jedes Tier wählt EXPLIZIT einen Algorithmus (meist NoBuffer/NoFlush,
   ein LSM/Bw-Tree-Tier einen echten Buffer). Das IST „22 uniform pro Tier", korrekt verstanden: alle Achsen-Interfaces in
   jedem Tier-Binary, je Achse ein austauschbarer Algorithmus. `AdHocComposition<17>→<19>` (kein Auto-Anhängen — explizit).
3. **Adapter-Gattung NICHT löschen, sondern RICHTIG definieren:** echte Container-Adapter (`std::queue/stack/priority_queue`)
   = `axis_inner` (Inner-Container) + delegierte Standard-Achsen + ordering/discipline (FIFO/LIFO/Priority). Die jetzige
   2-queuing-Slot-Hülle (`container_anatomy.hpp`) verwerfen/umbauen.
4. **Keine Doppelung:** mit (2)+(3) existiert queuing genau EINMAL (optionale SA-Achse); die Adapter-Gattung nutzt
   axis_inner+ordering, NICHT queuing.

**Zu revidierende Falschaussagen (Agenten-Liste, `[[never-delete-documentation]]` → Korrektur-Vermerke, nicht löschen):**
Code: `container_anatomy.hpp:2-8,78`, `genus_binding_traits.hpp:7-8,50-68`, `axis_observer_classification.hpp:11-12,64-65`,
`container_tier.hpp`, `container_abi_adapter.hpp`. Doku: `27_*.md:44,46-74,60`, `28_*.md:49-50,61,67` (insb. die „Doku-14-§7
ist überholt"-Zeile UMKEHREN), `29_*.md`, dieses Doc §6-Q1+§7, `19_*` (prüfen). Sessions (historisch markieren):
`20260602-experiment-baum-4-bruecken-final-audit.md`, ggf. weitere mit „Container-Gattung q1/q2".

**Hinweis:** Der Storage-Delegations-Fix (§6 Q2 Schritt 1-3, NodeChunkedStore → node_type wirksam) ist von diesem
Kategorienfehler UNABHÄNGIG + bleibt gültig (betrifft search↔node/layout/allocator, nicht queuing/Gattung).
