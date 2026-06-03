# Session 2026-06-03 — queuing-Migration 17→19 (#88/#89 fertig) + #87 Container-Tier-Unterklasse (in Arbeit)

> **Resume-Dokument** (Kontext lief aus). Autoritativ für den Architektur-Stand: `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md` §8 + §8.0.
> Diese Doku ist der Wiederaufsatzpunkt für **#87**.

---

## 0. Rolle des Agenten (verbatim, stehend)

> „du bist der Implementierungsagent und kannst dem laufenden Infrastruktur Agenten höchstens Wünsche
> in goal K78 übergeben bzw. ausschließlich erweitern, aber führst diese selbst nicht aus."

→ Ich (Implementierungsagent) baue **cache-engine / Diplomarbeit**. Cluster-/Infra-Arbeit wird NICHT von mir
ausgeführt, nur als Wunsch in `cluster_development/docs/sessions/K78-GOAL-CUTOVER-READY.md` §8 (CE-D1…D5)
hinterlegt (bereits committet+gepusht+synchron, Commit `808a1f9`). Memory: `feedback_implementation_agent_delegates_infra_via_k78`.

---

## 1. DAS 3-EBENEN-MODELL (autoritativ — Doc 30 §8.0)

Verbatim begründet in **Doku 24 §8.8** („Prüf-Dock je Gattung") + **Doku 14 §25**. Vom User mehrfach geschärft:

```
Ebene 1 — GATTUNG          = Außen-Interface zur Welt:  SearchAlgorithm | Container | Graph
                              (std::map-artig | std::queue/stack-artig | Graph-artig)
Ebene 2 — TIER-UNTERKLASSE = fester Achsen-Satz UNTER dem Interface (eine konkrete „Tierart")
Ebene 3 — ACHSEN           = Organe (Sub-Aufgaben). ALLE PFLICHT + UNIFORM getrieben.
                              KEINE Achse ist optional. Eine nicht-puffernde Tier-Unterklasse wählt
                              einen konkreten DURCHREICHENDEN Algorithmus (NoBuffer/NoFlush/NonePrefetch/
                              NoMigration …) — Durchreichen ist ein ECHTER Algorithmus, NICHT Abwesenheit.
```

**Aktueller Bau-Stand:** Es ist **nur EINE Tier-Unterklasse** real gebaut — die der **SearchAlgorithm**-Gattung.
Für sie sind alle Achsen Pflicht. Sie hat jetzt **19 Komposition-Achsen** (T0…T18).

**Kritische Konsequenzen (vom User erzwungen, nicht aufweichen):**
- Jeder Tier-Binary nutzt **durchgängig dieselben** Achsen-Interfaces wie alle anderen (uniform).
- **Keine Achse thematisch + kein Topic konzeptionell doppelt.**
- queuing ist eine **ACHSE** (Modulbaustein), **KEINE Gattung** — das war der korrigierte Kategorienfehler.

---

## 2. ERLEDIGT — #88 + #89: queuing q1/q2 → mandatorische SA-Achsen (AdHocComposition<17> → <19>)

**Status: fertig, verifiziert, committet, gepusht, submodul-synchron über 3 Ebenen.**

| Repo/Ebene | Commit | Inhalt |
|---|---|---|
| Modul-Mirror `comdare-cache-engine-core` | `e452b79` (gepusht) | observer/memento_aggregate total_slots 17→19 |
| cache-engine | `c9f051b` (gepusht, `main`) | #88+#89, 60 Dateien |
| Superprojekt (Diplomarbeit) | `6254283` (gepusht) | Submodul-Pointer-Bump, synchron |

**Was sich geändert hat:**
- `AdHocComposition<17>` → `<19>`: neue Slots **T17 = queuing_q1 (buffer_strategy)**, **T18 = queuing_q2 (flush_policy)**.
- queuing-Organe (real, durchreichend als Default): `queuing::axis_q1_queuing::NoBuffer` (default), `FIFOQueueBuffer`,
  `LIFO`, `BoundedRing`, … / `queuing::axis_q2_queuing::LazyFlush` (default), `EagerFlush`, `AdaptiveLsm`, `WatermarkFlush`.
- **Explizit je Tier** (kein Auto-Anhängen): jede Tier-Quelle endet jetzt mit `…, NoBuffer, LazyFlush)`.
- Kern: `composition_factory` (T0..T18, `static_assert ==19`, `IsPermTuple17`→`IsPermTuple19`),
  `composition_concept` (`IsComposition` + queuing_q1/q2, organ_count 19), `observer_aggregate`/`memento_aggregate` (19),
  `abi_adapter` (`queuing_q1_organ_`/`q2_organ_` Member), `search_algorithm_anatomy`.
- Umbrella/Namen/Registry/Codegen: `all_axes_umbrella`, `axis_path_serialization` (`kCompositionAxisNames` 19),
  `genus_binding_traits` (SearchAlgorithm `slot_count` 19), `composition_registry`, `adhoc_emitter`, `anatomy_module_abi_v1`.
- 10 Tier-Quellen + 11 benannte Compositions (art/hot/masstree/start/surf/wormhole +_paper_binding) je +queuing_q1/q2.
- ~25 Tests auf 19 gezogen.

**Klassifikation modell-konsistent** (`libs/cache_engine/builder/experiment_tree/axis_observer_classification.hpp`):
- queuing_q1/q2 von `ContainerObserver` → `SearchAlgorithmObserver` umklassifiziert.
- **19 SearchAlgorithmObserver + 3 DefinitionOnly (page_type/simd_extension/general_hardware) + 0 ContainerObserver = 22.**
- `ContainerObserver` enum ist jetzt **RESERVIERT für die echte Container-Gattung (#87)** — NICHT für queuing.

**Verifikation (literal):**
- Mess-Pfad: 8/8 Tiere, 19 Achsen, exit 0 (`obs_axes` 4→6).
- `test_d7b_definition_per_node`: frisch kompiliert, **SearchAlgorithmObserver==19 / DefinitionOnly==3 / ContainerObserver==0**, `static_assert(19+3+0==22)`, exit 0.
- `test_genus_binding` (slot==19), `test_v41_anatomy`, `test_adhoc_buildvariant_dll` (organ_count==19): PASSED.
- cmake-Reconfigure: configure+generate sauber.

Tag **`pre-delegation-sweep-20260603`** = Rücksetzpunkt vor dem ganzen Delegations-/queuing-Sweep.

---

## 3. KATEGORIENFEHLER-KORREKTUR (erledigt, dokumentiert)

`AnatomyGenus::Adapter` („Container"-Gattung) war eine **Hülle um die queuing-Achse**
(`ContainerComposition<Q1Buffer, Q2Flush>` — q1 `FIFOQueueBuffer` hatte bereits die volle Container-API).
queuing = Achse, NICHT Gattung. Korrigiert in **Doc 30 §8** (KORREKTUR) + **§8.0** (3-Ebenen-Modell, verbatim begründet)
+ über alle Architektur-Dokumente ab Dok 10 (26-Agenten-Konsistenz-Sweep: queuing≠Gattung + Tier-Unterklasse-Terminologie).
Doku 14 §25 verbatim erhalten + 7 Schärfungs-Notizen. Memory: `project_axis03a_monolith_violation_delegation_fix`.

---

## 4. #87+#90 — Container-GATTUNG + Tier-Unterklassen (WIEDERAUFSATZPUNKT, korr. 2026-06-03)

> ⚠️ **ZWEI frühere Designs in dieser Doku waren FALSCH (User-Korrekturen):**
> - „inner+ordering als Container-Gattung" — Ebenen-Fehler: Container ist die **Gattung**, Adapter eine **Tier-Unterklasse** darunter.
> - „ordering"-Achse — **erfunden** (Raten), steht NICHT in §28. (Memory: `feedback_never_guess_always_lookup_state_of_art_and_docs`.)

**AUTORITATIV (teuer nachgeschlagen, NICHT geraten):**
- **Doku 14 §27.2/§28** (`…/Diplomarbeit - Datenbanken/docs/architektur/14_…md`) + **Doc 30 §8.0** + C++-Standard.
- **3-Ebenen:** Gattung = Außen-Interface = **SearchAlgorithm | Container | Graph** (nur 3, neu `AnatomyGattung`-Enum). Darunter Tier-Unterklassen (`AnatomyGenus`-Enum, historischer Name): SA→MapLike(19 Achsen); **Container→{Set, Sequence, Adapter, View}**. Darunter Achsen (permutieren, keine optional).
- **Adapter-Tier-Unterklasse = §28-Invertebrate-Spalte = 13 Achsen:** delegiert (9) `search_algo, cache_traversal, memory_layout, allocator, prefetch, concurrency, isa, io_dispatch, migration_policy` + aktiv (3) `serialization, telemetry, value_handle` + spezifisch (1) **`inner_container`** (NEU axis_inner). **KEINE „ordering"-Achse.** §26.4: stack/queue→deque, priority_queue→vector+Compare; API `push/pop/top/front/back` (Disziplin in der API-Nutzung, NICHT als Achse).
- **Bau-Muster = exakt analog `SequenceComposition` (10 geteilt + growth)** → `AdapterComposition<T0..T11, Inner>` (12 geteilt + inner_container). Anatomie treibt `inner_container` real, delegiert/trägt den Rest.

**Bereits erledigt (verifiziert):** `anatomy_base.hpp` — `AnatomyGattung{SearchAlgorithm,Container,Graph}` + `gattung_of()` + `gattung_name()` eingeführt (nicht-brechend, kompiliert via test_v41_anatomy_base). AnatomyGenus als Tier-Unterklasse re-dokumentiert.

**Noch zu tun:** (a) `ContainerComposition<Inner,Ordering>`→`AdapterComposition<T0..T11,Inner>` (13 Achsen, ordering raus); ContainerAnatomy→AdapterAnatomy (API push/pop/top/front/back); inner_container-Organe (Deque/Vector/List) behalten, Fifo/Lifo/Priority-Ordering-Organe VERWERFEN. (b) genus_binding_traits<Adapter> auf 13 Achsen (§28-Namen). (c) container_tier/abi_adapter/dock/module_abi nachziehen (Observer-Felder: inner-Belegung + delegierte; flush/ordering raus). (d) test_container_genus + 5 Tests neu. (e) #90: `AnatomyGenus`→`AnatomyTierSubclass` Rename-Sweep + Datei-Rename container_*→adapter_* + Doku-Sweep ab Doc 10. (f) Dateinamen container_*.hpp → adapter_*.hpp (wie set_/sequence_/view_).
**STATUS 2026-06-03: #87 + #90-Kern + #90-Rename ERLEDIGT, verifiziert, committet + gepusht.**
- `18adc08` cache-engine: #87+#90-Kern (AnatomyGattung-Ebene; AdapterComposition §28 13 Achsen; ABI/Dock/Tests). Super `1b0f626`.
- `7d8130d` cache-engine: #90-Polish (container_*/Container* → adapter_*/Adapter*; 5 Header git-mv). Super `a03d2cf`.
- Verifiziert (literal): `test_container_genus` exit 0 ALLE OK (Ebenen + 13 Achsen + §26.4-API + Deque/Vector + GenusBindingTraits); `test_container_dock`/`test_genus_docks`/`test_v41_anatomy_base` grün; d4b_*/genus_binding compile-only grün.
- NICHT umbenannt (Doku 14 §27.2 Z.1100): `AnatomyGenus`-Enum + `::Adapter`, `AnatomyGattung::Container`, `gattung_name`.
- VERBLEIBEND: (1) Doku-Konsistenz-Sweep — Architektur-Docs 27–30 + Doc 30 §8.1 auf §28-Adapter-Modell + adapter_*-Typnamen (Session-Logs = historisch, belassen). (2) priority_queue (vector+Compare/Heap) = §28-Folgeschritt (nicht als Achse modelliert).

**ALTES (falsches) Design unten (§4.1–§4.5) ist HINFÄLLIG — nur als Fehler-Historie behalten.**

### 4.1 Exakte Umbaufläche (Dateien)

| Datei | Heute (Kategorienfehler) | Soll (#87) |
|---|---|---|
| `libs/cache_engine/anatomy/container_anatomy.hpp` | `ContainerComposition<Q1Buffer, Q2Flush>` + `ContainerAnatomy` treibt q1-Buffer-Organ (put/get über FIFOQueueBuffer); `ContainerFlushDecision` gespiegelt von q2 | `ContainerComposition<Inner, Ordering>`; `ContainerAnatomy` = echter Decorator: hält Inner-Container + wendet Ordering-Disziplin auf put/get/top an. queuing-Bezug RAUS. |
| `libs/cache_engine/builder/experiment_tree/genus_binding_traits.hpp` (Adapter-Spec) | `slot_count == 2` mit Achsen {queuing_q1, queuing_q2}; `name=="Container"` | `slot_count == 2` (oder mehr bei delegiert) mit Achsen {`inner_container`, `ordering`}; `name=="Container"` bleibt |
| `anatomy/container_tier.hpp` + `anatomy/container_abi_adapter.hpp` | ABI-Seite treibt q1/q2 | ABI-Seite treibt Inner+Ordering |
| `tests/unit/test_container_genus.cpp` | nutzt `ContainerComposition<q1::FIFOQueueBuffer, q2::WatermarkFlush>`; `organ_count==2 (buffer_strategy+flush_policy)`; FlushDecision-Konvention-Guard gegen q2 | NEU schreiben gegen `ContainerComposition<DequeInner, FifoOrdering>`; `organ_count==2 (inner_container+ordering)`; put/get-FIFO über echtes Inner-Organ; Q2-Block (WatermarkFlush) RAUS |
| `tests/unit/test_d4b*.cpp` (falls vorhanden) | Container-Gattung q1/q2 | inner/ordering |
| `axis_observer_classification.hpp` | ContainerObserver=0 (reserviert) | ContainerObserver bekommt seine ECHTEN Einträge: {`inner_container`, `ordering`} — ABER: das ist die **separate Container-Gattungs-Inventur**, NICHT in den 22 SA-Achsen. Klären: ob die 22-Klassifikation um die Container-Achsen ERWEITERT wird (dann >22) oder die Container-Gattung eine EIGENE Klassifikations-Liste bekommt. **Empfehlung: eigene Liste** (`kContainerAxisObserverClasses`), die 22er-SA-Liste bleibt 19/3/0. |

### 4.2 Test-Vertrag heute (`test_container_genus.cpp`, vollständig gelesen)

Der Test KODIERT SELBST noch den Kategorienfehler — er nutzt `ContainerComposition<FIFOQueueBuffer>` als „die Container-Gattung". Schlüsselzeilen, die #87 ändern muss:
- `using Q1 = q1::FIFOQueueBuffer; using Comp = cea::ContainerComposition<Q1>;` → `using Comp = cea::ContainerComposition<DequeInner, FifoOrdering>;`
- `organ_count == 2 (buffer_strategy + flush_policy)` → `(inner_container + ordering)`
- D4-Block „Q2 flush_policy (WatermarkFlush)" + der `static_assert` FlushDecision-Konvention-Guard gegen `q2c::FlushDecision` → **entfernen** (queuing gehört nicht mehr zur Container-Gattung).
- `genus()==Adapter`, `composition_name=="ContainerComposition"`, `ContainerObserverSnapshot` (put/get/peak/current_occupancy), `GenusBound<Adapter>==true`, `name=="Container"` → **bleiben** (das ist gattungs-korrekt).

### 4.3 Design der neuen Achsen (minimal-aber-echt, leichtgewichtige Organe — NICHT volles SA-Goldstandard nötig, da separate Gattung)

- **`ordering`-Organe:** `FifoOrdering` (queue), `LifoOrdering` (stack), `PriorityOrdering` (priority_queue). Bestimmen get()-Reihenfolge.
- **`axis_inner`-Organe:** `DequeInner` (default, wie std::queue/stack über std::deque), `VectorInner` (priority_queue), ggf. `ListInner`. Liefern Storage + Basis-Ops.
- `ContainerAnatomy<Comp>`: hält Inner-Container, put() → inner.push, get() → per Ordering (FIFO=front, LIFO=back, Priority=max), size()/observe_all() liefern `ContainerObserverSnapshot`.
- **Wichtig (kein Doppel-Topic):** „FIFO" als Container-Disziplin ≠ „FIFO" als SA-Write-Buffer (queuing). Verschiedene Topics/Rollen → keine Verletzung von „kein Topic konzeptionell doppelt". Im Commit/Doku klar benennen.

### 4.4 Verifikation für #87
- `test_container_genus` neu grün (inner+ordering, FIFO put/get echt).
- Container-DLL baubar.
- Die 22-SA-Klassifikation bleibt 19/3/0 (Container-Achsen in eigener Liste).
- `genus_binding_traits<Adapter>` sauber.

### 4.5 Vorgehen-Empfehlung
Design steht (oben). Implementierung kann an einen fokussierten Agenten delegiert werden (wie #88/#89) MIT dieser präzisen Spec + Build-Verifikation (`test_container_genus` + Container-DLL), **ohne Commit** — ich (bzw. der nächste Lauf) reviewt + committet. ACHTUNG Modell-Sensitivität: der User hat mehrfach Modellfehler korrigiert — die Spec oben strikt einhalten, NICHT „kreativ" erweitern.

---

## 5. OFFEN / VORBESTEHENDE BLOCKER

- **#90 (User-Freigabe nötig):** codebase-weite `AnatomyGenus`→3-Ebenen-Benennung (Gattung/Tier-Unterklasse). Groß, separat.
- **queuing-Treiben:** die queuing-Organe sind als Member in `abi_adapter` präsent, werden aber im Mess-Pfad noch nicht aktiv getrieben (observed=0 für q1/q2) — Vervollständigung offen (Such-Organ-Delegation, Audit-30 „Q2 Schritt 4").
- **Vorbestehend (NICHT meine Regression):**
  - `test_br3_obs22` ist **kein CMake-Target** (MSB1009 „test_br3_obs22.vcxproj nicht vorhanden", auch nach Reconfigure). Quelle ist korrekt auf 19/3/0 editiert; verifiziert-äquivalent via `test_d7b` (grün, gleiche `count_observer_kind`-Logik). Eigener Fix: Target in `tests/unit/CMakeLists.txt` ergänzen.
  - `test_v41_anatomy_adhoc_autobuilt_load` + `f15_measurement`: .vcxproj nicht in Solution.
  - `br4_emit`: vorbestehender `type_name<T>()`-Bug (class-keyword im Template-Arg), BR-4 Phase 2.

---

## 6. BUILD-/VERIFIKATIONS-KOMMANDOS

```powershell
# Repo
cd "C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine"
# vcvars + cmake-Preset
cmake --preset msvc-release           # configure+generate (Solution build/msvc-release)
# Einzeltest bauen+laufen (Beispiel)
cmake --build build/msvc-release --target test_d7b_definition_per_node --config Release
./build/msvc-release/tests/unit/Release/test_d7b_definition_per_node.exe
# Mess-Pfad (Tiere): tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1
# WICHTIG: Messung braucht /DCOMDARE_MEASUREMENT_ON=1 (gated IObservableTier, abi_adapter.hpp), sonst exit 1
```

---

## 7. NÄCHSTE KONKRETE SCHRITTE (Resume-Checkliste)

1. **#87 starten** (Wiederaufsatz oben §4): neue Achsen `axis_inner` + `ordering` anlegen → `ContainerComposition<Inner,Ordering>` + `ContainerAnatomy`-Decorator → `genus_binding_traits<Adapter>` auf {inner_container, ordering} → `container_tier`/`container_abi_adapter` → `test_container_genus` neu (queuing-/Q2-Bezug raus) → Container-DLL bauen → grün.
2. ContainerObserver-Einträge in EIGENER Liste (`kContainerAxisObserverClasses`); 22er-SA-Liste bleibt 19/3/0.
3. Audit-30 §6 auf „#88/#89 umgesetzt+verifiziert; #87 umgesetzt" nachziehen, sobald #87 grün.
4. Commit #87 (cache-engine) + Submodul-Bump Superprojekt + Modul-Mirror falls berührt.
5. Danach #90 (User-Freigabe) bzw. queuing-Treiben vervollständigen.

---

## 8. Submodul-Sync-Disziplin (Pflicht)
Nach jedem cache-engine-Push: Superprojekt-Pointer bumpen (`git add Code/external/comdare-cache-engine` + commit + push).
Bei berührtem Modul-Mirror: zuerst `modules/comdare-cache-engine-core` committen+pushen, dann cache-engine-Pointer.
Memory: `feedback_submodule_sync_3repos`, `feedback_destructive_autonomy_3repos_with_tag`.
