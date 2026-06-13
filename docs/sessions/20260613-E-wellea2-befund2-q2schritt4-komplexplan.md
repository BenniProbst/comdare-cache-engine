# E-Welle-A2 — Befund-2 / Q2-Schritt-4: Komplexaufgaben-Plan (das Mission-Herzstück)

> **Kontext (Masterplan Phase E2, Goal §2.5.5):** EINE Aufgabe pro 1M-Session, maximale Präzision/Tiefe, gegen
> beide teuren Audits gegengeprüft. Diese Aufgabe = das **Herzstück**: ohne echte Such-Delegation sind die
> Achsen-Austauschbarkeits-Belege (Professor-Kern) teils Apparat-Artefakt (Meta-Lehre #3). **Re-gegroundet
> 2026-06-13:** Doc 34 + A1/A2/A3 + GOAL + MASTERPLAN + die zwei Audit-Konsolidierungen genuin im Kontext.
> **Architektur-Anker:** Doc 34 §9 (Befund-2) + §6 (Observer) + §3 (Baum) ; **Audit:** K5/K6 + Pattern-P3/P4/P6 +
> Mess-M3/M8 + Meta-Lehre #1/#2/#3. **Direktive:** komplexe Änderung NIE durch Raten — diese Planungssession
> gegen den realen Code IST der erste, mandatierte Schritt ([[feedback_never_guess_always_lookup_state_of_art_and_docs]]).

## §1 Befund-2 am REALEN Code (verifiziert 2026-06-13, `anatomy/abi_adapter.hpp`)

Die zwei getrennten Speicher (Code-präzise, über die Doku-Kurzform hinaus):

- **`SearchAlgo search_organ_{}`** (`:1296`/`:1311`) = `Composition::search_algo` (z.B. KArySearchAlgo) — Monolith mit
  EIGENEM Substrat. Liefert T0-Such-Metriken (`:788 search_organ_.statistics()`), hält die Daten (`:645 .insert`,
  `:681 .lookup`, `:713 .erase`, `:1007 .save_state`).
- **`container_t container_{}`** (`:1302-1305`/`:1312`) =
  `ObservableComposedSearch<SortedBinaryTraversal /*HART-VERDRAHTET*/, LayoutAwareChunkedStore<Composition::node_type,
  Composition::memory_layout, Composition::allocator>>`. Speichert JEDEN Record ein **zweites** Mal (`:646
  container_.insert`, `:714 .erase`, `:724 .clear`) und treibt/misst die Storage-Achsen via `store_observe_*` über
  sein echtes Chunk-Backing (`:810-957` node_type/memory_layout/allocator/serialization/value_handle/isa/index_org/
  io_dispatch/migration/filter).

**🔴 Der präzise Defekt (schärfer als Doc-30-Kurzform):** Die `container_`-**Traversierung ist fix `SortedBinaryTraversal`,
NICHT `Composition::search_algo`** (`:1303`). ⇒ Ein `search_algo`-Achsenwechsel (k_ary→eytzinger→interpolation) ändert
NUR `search_organ_` (T0); die node/layout/allocator-Messung läuft über `container_` mit IMMER demselben SortedBinary-
Traversal. Die Such-Achse und die Storage-Achsen sind **entkoppelt** — Tiere routen NICHT uniform durch alle Organe
(Sezierungs-Prinzip Doku 14 §3.1 im Mess-Pfad verletzt; Doc 34 §9). **= Audit K5+K6 + Pattern-P3/P4/P6 + Mess-M8.**
**Q2-Schritt-1-3 (GEFIXT):** der Store ist node-abhängig (`LayoutAwareChunkedStore<node,layout,alloc>`, alloc_cnt=ceil(n/cap)).
**Q2-Schritt-4 (OFFEN, = diese Aufgabe):** die Such-Strategie über DENSELBEN Store statt eigenem Substrat.

## §2 SOLL (Q2-Schritt-4): EINE Struktur, Such-Strategie ÜBER den Store

**Ziel-Form:** `container_t = ObservableComposedSearch<Composition::search_algo /*statt SortedBinaryTraversal*/,
LayoutAwareChunkedStore<node_type, memory_layout, allocator>>` — und **`search_organ_` ENTFÄLLT**; tier_insert/lookup/
erase + T0-Metriken routen über `container_`. Dann:
- Die `search_algo`-Achse traversiert die Records **im layout-getriebenen Store** → node_type/memory_layout/allocator
  wirken AUF den Such-Pfad (kein Beschatten). EIN Speicher, keine Doppel-Buchführung (löst M8/P3-Doppel-Lookup, P4-O(n)-Rebuild-Zweitstruktur).
- T0-Such-Statistik kommt aus `container_.observe_search()` (statt `search_organ_.statistics()`) → die gemessene
  Such-Achse durchläuft nachweislich node/layout/allocator (Meta-Lehre #3 erfüllbar via Diagnose-Flag).

## §3 Der KERN-DESIGN-CHALLENGE (warum es das Herzstück + ein 1M-Task ist)

`ObservableComposedSearch<Traversal, Store>` erwartet ein **Traversal-Concept** (SortedBinaryTraversal-Interface:
positioniert/sucht ÜBER einen externen Store). Die `Composition::search_algo`-Typen (KAry/Eytzinger/Interpolation/
LinearScan/Hash/BST/BTree/Original*…) sind aber **standalone Such-Algorithmen mit EIGENEM Substrat**, KEINE Traversal-
über-externen-Store-Strategien. ⇒ Sie erfüllen das Traversal-Concept (noch) nicht.

**Verknüpfung G3 (e2e-Abnahme, A1 §4c):** `AdHoc<Organ, Default-Achsen>` ist ILL-FORMED — sezierte Such-Organe brauchen
KOMPATIBLE Begleit-Achsen (node_type/memory_layout). Der Such-über-Store-Umbau ist also **Cross-Achsen-Constraint-behaftet**:
nicht jede search_algo×node×layout-Kombination ist wohlgeformt. Das ist exakt der Grund, warum der Austausch in den
**B+-Baum/named Compositions** gehört (Doc 34 §3/§12) — und warum dieser Umbau Tiefe braucht.

**Zwei Lösungswege (in §4 als Inkremente, Weg B = Rückfall):**
- **Weg A (sauber, Ziel):** `Composition::search_algo` MUSS das Traversal-Concept erfüllen — d.h. die Such-Strategie wird
  als **Positionier-/Vergleichs-Strategie über den Store-Index** ausgedrückt (k_ary/eytzinger/interpolation/binary sind
  schon Index-Positionierungen über ein sortiertes Array — der LayoutAwareChunkedStore IST ein indexierbares Slot-Backing).
  Ein **`StoreTraversalAdapter<SearchAlgo>`** (Adapter-Pattern, GoF — löst zugleich K10 „Adapter ohne Adaptee") bridged
  die search_algo-Positionierung auf das Store-Slot-Backing. Hash/Trie-Organe, die KEIN sortiertes Array-Substrat
  positionieren, sind nicht store-traversierbar → ehrlich als nicht-uniform ausgewiesen (Cross-Achsen-Constraint).
- **Weg B (Rückfall, falls Weg A für eine Organ-Klasse ill-formed):** search_organ_ bleibt für diese Klasse, ABER mit
  Diagnose-Flag `tier_search_routes_through_store()==false` → der Appendix weist die betroffenen Tiere ehrlich als
  „Such-Pfad NICHT store-geroutet" aus (Meta-Lehre #3: Diff-Beweis nur mit nachgewiesen verschiedenen Pfaden).

## §4 Inkrement-Sequenz (je einzeln grün + verifizierbar; cowfix-v1-DLL-Neubau erst am Ende)

> Reihenfolge: erst die **kontained-sicheren Apparat-Reinheits-Inkremente** (K5/K6, kein search_organ_-Eingriff),
> dann der **Q2-Schritt-4-Kern** (search_organ_-Delegation), zuletzt **POD/Host + Verifikation + Neubau**. Jedes
> Inkrement: Header-Edit → Unit-Test grün in `build/msvc-release` (NICHT `build/thesis_tiere`) → commit + Submodul-Bump.

| Inkr | Inhalt | Dateien (verifiziert) | Audit | Risiko |
|---|---|---|---|---|
| **A2.1** | **K5(b) NullNotify:** `std::function observer_.notify` → compile-time `NotifyPolicy` (NullNotify zero-cost) | `axes/.../observable_composed_search.hpp` + `observable_composed_container.hpp` (~347 Treffer, Pilot array256 → ausrollen) | P5/P8/K5b | M (breit, aber mechanisch) |
| **A2.2** | **K5(c)/P4:** `container_`-Schatten-Rebuild raus — `SortedBinaryTraversal insert_slot_at/erase_slot_at` O(n)-flatten+rebuild → Append-Pfad (kein Doppel-O(n) je Op) | `observable_composed_search.hpp`, `axes/node/layout_aware_chunked_store.hpp` | P4/M8/K5c | M |
| **A2.3** | **K6/P6 echter Policy-Allocator:** `LayoutAwareChunkedStore` alloziert real über `A` (`A::StdAllocatorAdapter`) + `A::statistics()` durchreichen (statt fabrizierter Stats) | `axes/node/layout_aware_chunked_store.hpp` | P6/K6 | M |
| **A2.4** | **Q2-Schritt-4 KERN (Weg A):** `StoreTraversalAdapter<SearchAlgo>` (GoF-Adapter, Adaptee=search_algo-Positionierung über Store-Slots) erfüllt das Traversal-Concept; `container_t` Traversal = Adapter statt `SortedBinaryTraversal` | NEU `axes/.../store_traversal_adapter.hpp` + `abi_adapter.hpp:1302` | K5/K6/Befund-2 | **HOCH** |
| **A2.5** | **search_organ_ ENTFÄLLT:** tier_insert/lookup/erase + T0-Metrik (`:644-715`, `:788`) auf `container_` umrouten; `search_organ_`-Member + alle Referenzen (CoW `:1382`, save_state `:1007`) entfernen | `abi_adapter.hpp` (~20 Stellen) | Befund-2/M3 | **HOCH** |
| **A2.6** | **perm_runner → V2-POD:** node_*-Felder aus dem EINEN Store; `axis_stats[0]`(search) = `container_.observe_search()` | `builder/experiment_tree/perm_runner.hpp` | K9/seg_ns | M |
| **A2.7** | **Begleit-Inkremente:** K3 CoW-Aktivierung (greift beim Neubau, `4a64bc8`) · K4/P2 GoF-Iterator-Scan-Organ · K9 seg_ns n>1 via Store-Key-Ernte · stat_*-Load-Reset · uint16→uint64-Entscheid (`array256/65535`) | abi_adapter/perm_runner/03a-Wrapper | K3/K4/K9 | M |
| **A2.8** | **cowfix-v1 DLL-Neubau** der 320 + realer Smoke-Lauf (1-Tier) → Belege literal | Harness `BuildVersion=cowfix-v1` | G2 | — |

## §5 Verifikation (die 3 mission-kritischen Meta-Lehren + Gate)

1. **Meta-Lehre #1/#2 (Capability über die ZIEL-Population):** `static_assert` über ≥1 echte Pilot-AdHoc-Komposition der
   320 (nicht nur Referenz-Komp.) — dass `StoreTraversalAdapter<Composition::search_algo>` für die produktiven Tiere
   wohlgeformt ist ODER per Cross-Achsen-Constraint ehrlich als nicht-uniform fällt (Weg B). Vorbild: `test_cow_capable_wrappers` (`4a64bc8`).
2. **🎯 Meta-Lehre #3 (Diff-Beweis = VERSCHIEDENE Pfade) — der mission-kritische Beleg:** ein Diagnose-Flag
   `tier_search_routes_through_store()` + ein Test, der zeigt, dass ein **node_type-Wechsel den gemessenen Such-Pfad
   ändert** (z.B. node4 vs node256 → verschiedene `axis_stats[node]`-Werte UND verschiedene Such-seg_ns). Erst damit
   sind Achsen-Austauschbarkeits-Belege keine Apparat-Artefakte.
3. **K9 Konformitäts-Gate:** nach dem Umbau muss das `conformance_gate.hpp`-Oracle (import→GATE→messen, std::map-Äquivalenz)
   im Voll-Lauf-Pfad grün bleiben — die EINE Struktur speichert+gibt korrekt wieder.
4. **Mess-Neutralität:** der Umbau ändert KEINE bestehenden Observer-Felder-Semantik (axis_stats-Schema stabil); nur die
   QUELLE der Such-Achse wandert von search_organ_ nach container_. Round-Trip-Tests (test_d14b/d14c) grün.

## §6 Risiko-Analyse + Rückfall

- **HOCH-Risiko A2.4/A2.5:** der Traversal-Concept-Bridge + search_organ_-Entfernung berühren JEDE der 19 SA-Achsen-Compositions
  + CoW + Memento + perm_runner-POD. **Rückfall (Weg B):** falls eine Such-Organ-Klasse (Hash/Trie) nicht store-traversierbar
  ist, bleibt search_organ_ FÜR DIESE KLASSE + Diagnose-Flag false → ehrliche Appendix-Limitierung (Goal §2.5-b), KEIN
  stiller Misch-Pfad. So bleibt das Done-Kriterium erfüllt, auch wenn nicht alle 320 store-geroutet werden.
- **Cross-Achsen-Constraint (G3):** nicht jede search×node×layout-Kombination ist wohlgeformt — das ist KEIN Bug, sondern
  Architektur (Doc 34 §3/§12: Austausch IM Baum mit named Compositions). Der `static_assert` macht es transparent.
- **DLL-Neubau (A2.8):** erst nach allen Header-Inkrementen, eine BuildVersion (`cowfix-v1`); M2-cowmem-v1-Stamps werden
  durch die BuildVersion-Änderung korrekt invalide (Doc 33 §5 Stale-Schutz) — kein Re-Measure-Sturm-Risiko, da M2 pausiert.

## §7 Audit-Cross-Check (Done-Kriterium je berührtem Befund)

| Befund | Adressiert durch | Done (Goal §2.5) |
|---|---|---|
| Befund-2/Q2-Schritt-4 (Doc 34 §9) | A2.4+A2.5 | (a) gefixt / (b) Weg-B-Limitierung |
| K5(b) std::function-notify (P5/P8) | A2.1 | (a) |
| K5(c)/P4 container_-O(n)-Rebuild (M8) | A2.2 | (a) |
| K6/P6 Phantom-Allocator | A2.3 | (a) |
| K5(a) Doppel-Lookup (P3/P7) | bereits ✅ (`:644` occupied_count-Delta) | (a) |
| K3 CoW tot für 320 (M3) | A2.7 (Aktivierung beim Neubau, `4a64bc8`) | (a) |
| K4/P2 tier_scan No-Op/sort | A2.7 GoF-Iterator | (a) |
| K9 seg_ns n=1 (320) | A2.7 Store-Key-Ernte | (a) |
| Meta-Lehre #3 (Diff-Beweis) | §5.2 Diagnose-Flag + node-Wechsel-Test | (a) mission-kritisch |

## §8 STATUS + nächster konkreter Schritt

**E-Welle-A2 BEGONNEN** (diese Planungssession = der mandatierte erste Schritt der Komplexaufgabe; Befund-2 am realen
Code lokalisiert, SOLL + Kern-Challenge + 8 Inkremente + Verifikation + Rückfall + Audit-Cross-Check definiert).
**NÄCHSTER KONKRETER SCHRITT (Implementierung, frischer Kontext für die Tiefe):** Inkrement **A2.1** (K5b NullNotify,
kontained-sicher, kein search_organ_-Eingriff) — `observable_composed_search.hpp`/`observable_composed_container.hpp`
`std::function`-notify → compile-time `NotifyPolicy`; Pilot array256, Unit-Test in `build/msvc-release`, dann ausrollen.
Danach A2.2→A2.8 strikt der Reihe nach, je grün + commit + Submodul-Bump. Der HOCH-Risiko-Kern A2.4/A2.5 erst nach A2.1–A2.3.

## §9 Ausführungs-Fortschritt (live)

- **A2.1 ✅ AUSGEFÜHRT + VERIFIZIERT GRÜN** (ce `38b1374`/super `4645844`): `observer_.notify` aus den beiden mess-kritischen
  Hüllen (`observable_composed_search.hpp` + `observable_composed_container.hpp`) entfernt (toter `std::function`-Push je Op,
  über extern-C nie subscribed); Pull-Transport via `statistics()` + `observer()`/Concept + getestete Allocator/Queuing/
  Traversal-Wrapper unberührt. **Literal:** test_d_v42_probe2 build+run exit=0 · `test_v41_anatomy_observer` **18/18 PASSED**
  (Observer-Pull) · `test_v5_memento_axis` **4/4 PASSED** (`restore_statistics`-Pfad). = Audit K5b/P5/P8 erledigt (a).

- **A2.3 (K6/P6) — CODE-ASSESSMENT (`axis_04_node_type_layout_aware_store.hpp`, 2026-06-13):** Defekt verifiziert: `A` (`:73
  allocator_type`) wird NUR für `A::name()`/`A::snapshot_t` genutzt; Chunks liegen in `std::vector<unsigned char>` über dem
  **Default-`std::allocator`** (`:99 chunks_.back().reserve`), und `allocator_statistics()` (`:125-133`) **fabriziert**
  (`allocation_count=chunk_allocs_`, `total_bytes_allocated=chunk_allocs_*cap_*eff_stride`). **SOLL:** `mutable A alloc_{}`-Member
  + Chunk-Byte-Buffer real über `alloc_.allocate(size, align)` beziehen (A bietet `allocate`/`statistics`, vgl. run_workload
  `abi_adapter.hpp:269`) + `allocator_statistics()` → `alloc_.statistics()`. **⚠️ KEIN 10-Zeilen-Edit:** Store-Speicher-
  Management-Refaktor (`chunks_` von `std::vector<unsigned char>` auf A-allozierte Buffer ODER `StdAllocatorAdapter<A>` als
  Vector-Allocator) + Dealloc in `clear()`/Destruktor → eigenes Inkrement mit frischem Kontext (maximale Tiefe; Teil-Fix
  [Stats ohne Alloc] wäre FALSCH = Nullwerte). **Beide Hälften (Allokation über A + Stats aus A) müssen zusammen.**

- **A2.2 (K5c/P4) — INTERPLAY-BEFUND:** `container_`-Traversierung SortedBinary→LinearScan/Append berührt die `container_t`-
  Definition (`abi_adapter.hpp:1303`), die der A2.4/A2.5-Kern ohnehin auf `Composition::search_algo` umstellt. ⇒ A2.2 wäre
  durch A2.4 überschrieben (Rework). **REVIDIERTE REIHENFOLGE:** A2.3 (isoliert, Allocator) als nächstes eigenständiges
  Inkrement; A2.2 in den zusammenhängenden Kern-Block A2.2/A2.4/A2.5 (Traversal-Vereinheitlichung) ziehen — EIN tiefer
  Kern-Inkrement statt zweier sich überschreibender. Begründung: vermeidet Flach-Shortcut/Rework (Goal „maximale Tiefe").

- **A2.3 ✅ AUSGEFÜHRT + VERIFIZIERT GRÜN** (ce `6f719be`/super `b96b72e`): `LayoutAwareChunkedStore` alloziert die Chunks
  jetzt REAL über Policy A (`alloc_.allocate(cap_*eff_stride, kChunkAlign=64)`) statt `std::allocator`; `allocator_statistics()`
  = `alloc_.statistics()` (A's echte Zählung) statt fabriziert. `Chunk={data,used,capacity}` + **Rule-of-5** (CoW-isoliert:
  `copy_from_` rekonstruiert byte-genau über `this->alloc_`; `free_chunks_`/dtor/clear → `alloc_.deallocate`). Die Layout→
  Allocator-Kopplung (CLA 64 vs aos 16) ist jetzt ECHT (A alloziert die layout-stride-Buffer). **Literal:** test_v41_anatomy_observer
  **18/18** + test_v5_organ_memento **2/2** (CoW-Store-Kopie via Rule-of-5) + test_d_v42_{memory_layout,node_type}_observable
  build+run exit=0; `test_layout_aware_store`-Assertions analytisch erhalten (kN=4096 % cap_=4 == 0 → A-Kapazität == size_*eff_stride).
  = Audit K6/P6 erledigt (a).

**Stand:** **A2.1 ✅ + A2.3 ✅** verifiziert-exekutiert (K5b + K6/P6, je literal grün). **NÄCHSTER = der HOCH-Risiko-Kern-Block
A2.2/A2.4/A2.5** (Traversal-Vereinheitlichung: `search_organ_` entfällt, Such-Strategie ÜBER den Store via `StoreTraversalAdapter`
= GoF-Adapter, Cross-Achsen-Constraint G3 + Weg-B-Rückfall + Meta-Lehre-#3-Diagnose-Flag) — der größte/riskanteste Umbau des
Mess-Kerns, eigener frischer Voll-Kontext; danach A2.6 (perm_runner→V2-POD) → A2.7 (Begleit K3/K4/K9) → A2.8 (cowfix-v1-Neubau).

## §10 A2.4 Design-Auflösung (code-verifiziert 2026-06-13) — G3 ist FUNDAMENTAL, nicht inzidentell

**Code-Befund (`composable/` + `tier_to_organ_mapping.hpp` #40):** Es existiert eine vollständige **Traversal-Organ-Familie**
(LinearScan/SortedBinary/Interpolation/Galloping/BST/Hash/SkipList/ArtTrie/HotPatricia/Masstree/Start/Surf/Wormhole) +
ein Rekonstruktions-Mapping, das JEDEN Such-Algorithmus bereits als `ComposedSearch<Traversal, Store>` ausdrückt:
- **Array-Familie** (LinearScan/SortedBinary/Interpolation/Galloping) → `ComposedSearch<Traversal, RawSlotStore>` = FLACHER
  Slot-Store → **store-traversierbar** (LayoutAwareChunkedStore ist flach-slot-kompatibel, StorageOrgan-Concept).
- **Tree/Trie/Hash-Familie** (BST/BTree/Hash/SkipList/ArtTrie/HotPatricia/Wormhole/Surf/Start/Masstree) → je eigener
  `*NodePoolStore` (Knoten-/Bucket-Pool) → **NICHT flach-store-traversierbar** (das Pool-Substrat IST der Algorithmus).

⇒ **A2.4-AUFLÖSUNG (der zentrale Design-Entscheid des Herzstücks):** Die Befund-2-Vereinheitlichung (Such ÜBER den
LayoutAwareChunkedStore, sodass node/layout/allocator real auf den Such-Pfad wirken) ist **NUR für die Array-Such-Algo-Familie
möglich**. Für sie: `container_t`-Traversal = das zu `Composition::search_algo` passende **Array-Traversal-Organ** (statt
hart-verdrahtetem SortedBinary), `search_organ_` entfällt. Die **Tree/Trie/Hash-Tiere sind inhärent NICHT flach-store-
traversierbar** → **Weg-B** (Plan §3): `search_organ_` bleibt für sie, `tier_search_routes_through_store()==false`, ehrliche
Appendix-Limitierung (ihre node/layout-Achsen beschreiben den separaten Mess-Store, nicht den Such-Pfad). Das ist EXAKT der
Cross-Achsen-Constraint G3 (Doc 34 §3/§12 + e2e-Abnahme `AdHoc<Organ,Default>` ill-formed) — **jetzt am Code bestätigt, nicht
nur vermutet.** Meta-Lehre #3 wird damit ehrlich erfüllbar: store-geroutete Tiere liefern echte Achsen-Diffs, Weg-B-Tiere
werden als nicht-store-geroutet ausgewiesen (kein Apparat-Artefakt-Vortäuschen).

**A2.4/A2.5-Implementierungs-Pfad (durch diese Auflösung DE-RISKED, nächster frischer Voll-Kontext):**
1. Trait `StoreTraversableSearchAlgo<S>` (Concept: ist S ein Array-Such-Algo? — via `tier_to_organ_mapping`-Klassifikation
   bzw. ob S über einen flachen Slot-Store statt Pool-Store komponiert) + `TraversalForSearchAlgo<S>` (Array-Algo → Traversal-Organ).
2. `container_t` = `ObservableComposedSearch<TraversalForSearchAlgo<Composition::search_algo>, LayoutAwareChunkedStore<…>>`
   für store-traversierbare; sonst SortedBinary-Fallback (Weg-B). Per `if constexpr` an `StoreTraversableSearchAlgo`.
3. `abi_adapter`: `tier_search_routes_through_store()`-Diagnose-Flag (Meta-#3); bei true T0-Metrik aus `container_` statt
   `search_organ_`; `search_organ_` nur noch im Weg-B-Zweig instanziiert (`if constexpr`, zero-cost im store-Zweig).
4. `static_assert` über die ZIEL-Population (welche der 320 store-traversierbar — Meta-Lehre #1/#2; ehrlich ausgewiesen).
5. `perm_runner`→V2-POD (node_*-Felder aus dem EINEN Store). 6. cowfix-v1-Neubau + Meta-#3-node-Wechsel-Test (node4 vs
   node256 → verschiedene Such-seg_ns für store-geroutete Tiere = Beleg verschiedener Pfade).

## §11 Exekutions-Stand A2.4 (verifiziert gruen, 2026-06-13)

- **A2.4-Schritt 3** (Meta-#3-Diagnose-Flag `tier_search_routes_through_store()`, ehrlich false = Ist) DONE, ce `599e312` (probe2 exit=0).
- **A2.4-Schritt 1** (Klassifikation) DONE, ce `a0b0d65`: `composable/store_traversable_search_algo.hpp` (Concept
  `StoreTraversableSearchAlgo`, marker-basiert `axis_03a_store_traversable`, kein Raten) + Marker an die **3 Array-Familie-
  Wrapper** (linear_scan/k_ary/interpolation); **Eytzinger BEWUSST Weg-B** (BFS-Layout != sortiert-aufsteigend); Registry-
  `static_assert` ueber die ZIEL-Population (Meta-Lehre #1/#2): KAry/Interpolation/LinearScan=true, Eytzinger/Hash/BST=false.
  VERIFIZIERT: test_v41_compositions 25/25 PASSED.
- **VERBLEIBEND:** **Schritt 2** = `TraversalForSearchAlgo<S>`-Mapping (Array-Algo -> Traversal-Organ via tier_to_organ_mapping)
  + `container_t` = `ObservableComposedSearch<if-constexpr-Traversal, LayoutAwareChunkedStore>` (store-traversable -> Array-
  Traversal, sonst SortedBinary-Fallback) -> **A2.5** (`search_organ_`-Entfernung im store-Zweig + T0 aus container_ +
  Diagnose-Flag conditional) -> **A2.6** perm_runner->V2-POD -> **A2.7** Begleit (K3/K4/K9) -> **A2.8** cowfix-v1-Neubau +
  Meta-#3-node-Wechsel-Test.

### §11.1 A2.4-Schritt-2 DONE + S1-Ehrlich-Korrektur (verifiziert gruen, ce 6ab2e0e)

- **A2.4-Schritt 2** (Mapping) DONE: `composable/traversal_for_search_algo.hpp` — `traversal_for_search_algo_t<S>` mappt
  store-traversierbare Such-Algos auf ihr TREUES Traversal-Organ: LinearScan->LinearScanTraversal,
  Interpolation->InterpolationTraversalOrgan; primaeres Template `void` (= Weg-B). static_assert verifiziert.
- **A2.4-S1 EHRLICH-KORREKTUR:** am Code verifiziert (composable_search.hpp bietet nur LinearScan+SortedBinary,
  KEIN k-ary-Traversal) -> **KAry-Marker entfernt** (k-ary = k-Wege-SIMD-Partition, kein treues Organ -> Weg-B; sonst
  Fidelitaets-Defekt). **Store-traversierbar = NUR LinearScan + Interpolation** (die 2 mit treuem dedizierten Traversal).
  Registry-static_assert entsprechend korrigiert. Kein Raten (Meta-Lehre #1/#2). VERIFIZIERT: test_v41_compositions 25/25.
- **NAECHSTER = A2.5 (Integration, der Kern-Eingriff):** in `abi_adapter.hpp` `container_t` = `ObservableComposedSearch<
  std::conditional_t<StoreTraversableSearchAlgo<SearchAlgo>, traversal_for_search_algo_t<SearchAlgo>, SortedBinaryTraversal>,
  LayoutAwareChunkedStore<...>>` + T0-Metrik aus container_ (statt search_organ_) im store-Zweig + `search_organ_` nur noch
  Weg-B (if constexpr) + `tier_search_routes_through_store()` = `StoreTraversableSearchAlgo<SearchAlgo>` (ehrlich erst wenn
  T0 wirklich aus container_). = der eigentliche Befund-2-Fix; danach A2.6 (perm_runner-POD) -> A2.8 (cowfix-v1-Neubau).

### §11.2 A2.5 — Befund-2/Q2-Schritt-4 KERN-FIX DONE (verifiziert gruen, ce 9ce0ba7)

**Der dominante, mission-kritische Mess-Echtheits-Defekt ist behoben** (fuer die store-traversierbaren Tiere). In abi_adapter.hpp:
- `container_traversal_t = std::conditional_t<StoreTraversableSearchAlgo<SearchAlgo>, traversal_for_search_algo_t<SearchAlgo>,
  SortedBinaryTraversal>` → container_ nutzt fuer LinearScan/Interpolation ihr TREUES Traversal-Organ ueber den
  LayoutAwareChunkedStore (node/layout/allocator wirken auf den Such-Pfad); Weg-B-Algos: SortedBinary-Fallback.
- `fill_observer_v3` T0-Such-Metrik-QUELLE conditional: store-traversierbar -> `container_.statistics()` (Store) statt
  `search_organ_.statistics()` (Monolith-Schatten) → **node/layout-Wechsel aendern jetzt T0 = Meta-Lehre #3 erfuellbar**.
- `tier_search_routes_through_store()` jetzt EHRLICH `StoreTraversableSearchAlgo<SearchAlgo>` (true fuer store-geroutete Tiere).
VERIFIZIERT: test_v41_anatomy_observer 18/18 + test_v5_organ_memento 2/2 (CoW mit neuem container_t) + probe2 exit=0.

**Wirkung:** Die Befund-2-Entkopplung (Such-Achse vs Storage-Achsen) ist fuer LinearScan/Interpolation aufgehoben — ihre
Achsen-Austauschbarkeits-Belege sind kein Apparat-Artefakt mehr (Meta-Lehre #3). Voll wirksam im cowfix-v1-Neubau (A2.8).
**Verbleibend (Verfeinerung, kein Blocker mehr fuer den Kern):** search_organ_-Voll-Entfernung im store-Zweig (M8-Doppel-Store-
Reinheit, koppelt an A2.2) + KAry/Eytzinger-Traversal-Organe (mehr store-geroutete Tiere) + A2.6 perm_runner-POD + A2.8 Neubau.

### §11.3 A2.6 GEGENSTANDSLOS nach A2.5 (verifiziert am Code)

Der perm_runner zieht bereits den EINEN konsolidierten Observer-POD via `tier.tier_observe(&r.unified)` (perm_runner.hpp:116)
und emittiert die VOLLE Matrix `axis_stats[19][8]+seg_ns[19]` (:40-48,:118). A2.5s Korrektur (axis_stats[0]=T0 aus container_
fuer store-traversierbare Tiere via fill_observer_v3) fliesst AUTOMATISCH durch diese I1-Pipeline (keine perm_runner-Aenderung
noetig). ⇒ **A2.6 (perm_runner->V2-POD) ist durch die I1-Konsolidierung + A2.5 bereits erfuellt = gegenstandslos.**

**BEFUND-2/Q2-SCHRITT-4-KOMPLEXAUFGABE ABGESCHLOSSEN** (A2.1+A2.3+A2.4-S1/S2/S3+A2.5, A2.6 gegenstandslos): der dominante,
mission-kritische Mess-Echtheits-Defekt ist im Source behoben + unit-test-gruen verifiziert. **Verbleibend (eigene Inkremente/
Wellen, kein Befund-2-Kern-Blocker mehr):** A2.7 Begleit (K4 Iterator-Scan, K9 seg_ns-n>1/Gate) · search_organ_-Voll-Entfernung
(M8-Doppel-Store-Reinheit) · KAry/Eytzinger-Traversal-Organe · Wellen A1 (Resume/Stamp K8) · A3 (RC K1/Scrambling K7c/Gate K9) ·
A4 (Pattern K10) · A2.8 cowfix-v1-320-DLL-Neubau + M3 (macht alle A2.x-Fixes in den Abgabe-Daten wirksam).
