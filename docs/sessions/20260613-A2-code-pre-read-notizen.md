# A2-Code-Pre-Read-Notizen (Masterplan-Phase A2) — IST-Code ⇄ Doc-Abgleich

> **Zweck:** Durable Destillation des Pflicht-Code-Pre-Reads (Masterplan A2), kompaktierungs-fest. Jede Zeile =
> am ECHTEN Code verifiziert (Datei:Zeile), gegen die Architektur-Docs gegengeprüft. Baut auf dem A1-Notizen-Doc auf
> (`20260613-A1-lesenotizen-ist-architektur.md`). Wird in Phase B in das konsolidierte Master-Doc überführt.
> **A2-Liste (Masterplan):** experiment_tree · registry_to_axis_levels · profile_to_tree · composition_registry ·
> composition_concept ✓ · composition_factory · search_algorithm_anatomy · abi_adapter · observable_tier · perm_runner ·
> cache_engine_builder_iterator · permutation_engine · genus_binding_traits.

## A2.1 — `anatomy/abi_adapter.hpp` (DER Mess-Kern; Befund-2-Ort) — am Code verifiziert 2026-06-13

**Dreifach-Vererbung + B1-Split + compile-time-Removal (bestätigt Doc 31/messarchitektur_v5_design §7, abhaengigkeitskette):**
- `:116` `class SearchAlgorithmAbiAdapter final : public IAnatomyBase,` + `:121` `#if COMDARE_MEASUREMENT_ON` → die
  Observer/Memento/Workload-Sub-Interfaces werden NUR unter `COMDARE_MEASUREMENT_ON` zusätzlich vererbt (Release-DLL =
  nur Antrieb, kein vtable-Slot). **IST = Doc-31/v5_design-§7-Soll bestätigt am Code.**
- `:234` `#if COMDARE_MEASUREMENT_ON // V5-I2.2: Pfad-A run_workload NUR bei Messung-AN (V3-Designfehler; host-relokalisiert)`.

**🔴 BEFUND 2 (zwei getrennte Speicher) — IST WEITER REAL, ABER teilfix verdrahtet (verifiziert):**
- `search_organ_` (Monolith, `Composition::search_algo`) lebt weiter + liefert die SEARCH-Metriken: `:644/:677`
  `search_organ_.occupied_count()`-Delta, `:645` `.insert`, `:713` `.erase`, `:788` `.statistics()→axis_stats[search]`.
- `container_` (separater Store) liefert die STORAGE-Achsen: `:879–953` die Scan-Achsen (value_handle/isa/index_org/
  io_dispatch/migration/filter) routen jetzt über **`store_observe_* → NodeChunkedStore::organ_observe_*`** über das
  ECHTE `container_`-Slot-Backing (Q2-Schritt-1-3-Erweiterung — die Speicher-Achsen wirken real, node-abhängig).
- ⇒ **Q2-Schritt-4 (volle Such-Delegation: `search_organ_` entfällt, Such-Strategie ALS Traversal über DENSELBEN
  container_-Store) ist WEITER OFFEN** — die SEARCH-Zähler kommen weiter aus dem Monolith. **Das bleibt die SOLL-
  Korrektur für Mess-Echtheit (Audit-K5/K6) = E-Aufgaben-Kandidat.** (Bestätigt A1-§4d am Code; Doc-30-§6-Stand aktuell.)

**K5(a)-Fix VERIFIZIERT am Code:** `:644/:677` is_new via `occupied_count()`-Delta (before/after), NICHT verdeckter
Doppel-Lookup → der Audit-K5(a)-„is_new via occupied_count-Delta"-Fix ist im Code umgesetzt.

**queuing q1/q2 als mandatorische SA-Achsen (Doc 30 §8.1, 17→19) VERIFIZIERT:** `:664` `queuing_q2_organ_.should_flush(
occupied_count, 1024)` wird am `tier_insert` mit-getrieben → q1/q2 sind echte, uniform getriebene SA-Achsen-Organe.

**tier_clear-Reset-Fix (I-B.1) VERIFIZIERT:** `:724` `search_organ_.clear(); container_.clear();` + `:730`
`if constexpr (requires { search_organ_.reset(); }) search_organ_.reset();` + Kommentar „‚nicht resetbar' war VERALTET".

**I1-Observer-Konsolidierung (EIN POD + Q1-Sequenz, Doc 31) VERIFIZIERT am Code:**
- `:779` `fill_observer_v3(ComdareTierObserverSnapshot* out)` schreibt die Per-Achsen-Observer DIREKT in den
  konsolidierten POD `out->axis_stats[T][f]` (kein V1/V2/V3-Bridge mehr).
- `:1143` **EIN** `void tier_observe(ComdareTierObserverSnapshot* out) const noexcept override` → `:1147–1148` SCHRITT 1
  `fill_observer_v3` (axis_stats-READ) → `:1151` SCHRITT 2 `fill_segment_timing_v3(&seg)` (seg_ns-Timing). **Q1-Sequenz
  (Doc 31 §6: READ→Timing→Reset gegen Doppelzählung) am Code bestätigt.**

**Pfad-B Per-Achsen-Timing über REALE Komposition (V4/seg_ns, NICHT synthetisch) VERIFIZIERT:**
- `:990–995` `fill_segment_timing_v3(ComdareSegmentLatencyV2* out)` zeitet die 19 Achsen über die **„befüllte composite-
  Tier-Struktur (search_organ_ + container_.chunks_ + Instanz-Organe)"** — KEIN synthetischer Puffer; `:994` memento-
  neutral (Daten an search_organ_/container_ unberührt); `:1007` `search_organ_.save_state()` für n>1-seg_ns-Key-Ernte.
  ⇒ Der frühere „Pfad-B-Timing"-Plan ist im Code REAL umgesetzt (nicht nur geplant).
- KONTRAST: `:490–626` Pfad-A `run_workload`-Segment-Timer (`do_seg19`, 19 Achsen einzeln über **synthetischem `lbuf`**)
  = die isolierte Achsen-Messung (Doc 24 §8.1 Pfad A). Beide Pfade koexistieren wie spezifiziert.

**CoW-Memento Rev.2 (Doc 33) VERIFIZIERT am Code:** `:640/:710/:722` `if (cow_armed_) cow_materialize_copy_();` (Vollkopie
VOR der ersten mutierenden Op); `:1158–1203` Memento-Internals (`saved_search_stats_` O(1)-POD-Snapshot, `cow_armed_`,
`cow_materialized_`, In-Memory-Tiefkopie search_organ_+container_; T6-Allocator aus Datenzustand DERIVIERT; Fallback-Kaskade
über `restore_statistics`/Copy requires-detektiert). `tier_memento_is_copy_on_write()` Diagnose. **Doc-33-Rev.2-Soll = Code-IST.**

**A2.1-Fazit:** Der Mess-Kern entspricht den Docs (Doc 24/30/31/33 + v5_design) — die Mess-Architektur ist im Code real
verankert (I1-EIN-POD, Q1-Sequenz, Pfad-A/B-Koexistenz, CoW-Memento, q1/q2-SA-Achsen, K5(a)/tier_clear-Fixes). **Der EINE
verbleibende Echtheits-Defekt = Q2-Schritt-4 (Such-Organ beschattet node/layout via search_organ_-Monolith)** — bestätigt,
präzise lokalisiert, E-Aufgaben-Kandidat (K5/K6). Keine Doku-↔-Code-Drift im Mess-Kern festgestellt.

## A2.2 — `anatomy/observable_tier.hpp` (EIN konsolidierter POD + IDriveableTier-Split) — verifiziert 2026-06-13

- **EIN POD `ComdareTierObserverSnapshot` (`:111`):** `axis_stats[19][8]` + `seg_ns[19]` + 4 Meta (`observable_axis_count`,
  `tier_fill_level`, `filled_axis_count`, `batches_measured`); `static_assert standard_layout + trivially_copyable`;
  **sizeof==1400, alignof==8** (Kommentar). EXAKT Doc 31. V1/V2/V3-PODs + Sub-IFs ENTFERNT; Versionierung via ABI-Major.
  `kTierObserverSnapshotVersionUnified=4`.
- **🔴 SCHEMA VOLLSTÄNDIG (über Doc-30-Snapshot HINAUS):** `kV3AxisSchema[19]` (`:66–86`, single-source Schreiber↔CSV-Spalte)
  ist für **ALLE 19 Achsen befüllt** — Phase A (T0/T1/T2/T4/T5/T6/T9/T10/T17/T18) + **Phase B abgeschlossen (2026-06-04):**
  T3 path_compression/T7 prefetch/T8 concurrency/T11 value_handle/T12 isa/T13 index_org/T14 io_dispatch/T15 migration/T16 filter.
  `v3_count_filled_axes()` zählt single-source aus dem Schema (kein Hardcode). ⇒ die „R5.B operativ nur 2 Achsen"-Grenze (Doc 30 §b)
  ist im POD-Schema überholt — alle 19 Achsen tragen benannte Observer-Felder.
- **B1-Split (`:144`):** `class IObservableTier : public IDriveableTier` — `IDriveableTier` = funktionaler Antrieb (IMMER
  einkompiliert), `IObservableTier` ergänzt NUR `tier_observe` (nur unter `COMDARE_MEASUREMENT_ON` vererbt). EINE `tier_observe(
  ComdareTierObserverSnapshot*)`. Hängt NICHT an IAnatomyBase (vtable-Stabilität), `dynamic_cast` 1× kalt. Doc 31/v5_design §7 = IST.

## A2.3 — `anatomy/search_algorithm_anatomy.hpp` (Composition-Driver-Stand, Doc 29) — verifiziert 2026-06-13

- **Container-API ENTFERNT (`:156–168`):** insert/lookup/erase/clear/size → `builder::anatomy_commands::AnatomyExecutionContext`
  (Doc 14 §17.2/§24: „Anatomie = nur Achsen + Observer"). `observe_all()→ObserverAggregate<C>`; `genus()==SearchAlgorithm`.
- **🔴 observe_all hält jetzt 9 reale Achsen-Organe (über Doc 29 §3e „2./3." HINAUS):** `:62–112` sammelt via ObservableAxis-Guard
  search_algo + telemetry + memory_layout + serialization + node_type (5 OperativeCapable via `ObservableXxx`-Hüllen, Doc 29 §3c/e)
  **+ Phase A (2026-06-04): cache_traversal/mapping/queuing_q1/queuing_q2** (T1/T2/T17/T18). 9 Member-Organe (`:173–196`,
  default-init OHNE `{}` wg. Aggregat-Brace-ill-formed-Befund Doc 29 §3c). Der protected-CRTP-ctor-Block ist via Hüllen gelöst.
- **🔴 ZWEI getrennte Observe-Mechanismen (KLÄREN in B):** (1) `SearchAlgorithmAnatomy::observe_all()` = in-process-Pfad, hält die
  9 Achsen-Organe (AnatomyExecutionContext-Pfad). (2) `SearchAlgorithmAbiAdapter::fill_observer_v3` (A2.1) füllt den ABI-POD
  `axis_stats[19][8]` aus seinem EIGENEN `search_organ_` + `container_`(NodeChunkedStore::organ_observe_*) — NICHT aus der Anatomie-
  observe_all. ⇒ Der gemessene ABI-Pfad (abi_adapter) und der Anatomie-observe_all sind zwei verschiedene Observe-Wege; ihr Verhältnis
  + die Befund-2-Q2-Schritt-4-Korrektur (search_organ_ entfällt) sind in B/E konsolidiert zu adressieren.

## A2.4 — `anatomy/composition_factory.hpp` (AdHocComposition = 19 Slots) — verifiziert 2026-06-13

- **`AdHocComposition<T0..T18>` = 19 Slots** (`:49–76`): T0 search_algo … T16 filter (17 Such-Achsen) + **T17 queuing_q1 + T18
  queuing_q2** als reguläre, EXPLIZITE SA-Slots (KEINE Template-Defaults). Die 17→19-Migration (Doc 30 §8.1) ist im Code REAL.
  Slot-Reihenfolge exakt wie Doc 27/abhaengigkeitskette (UNVERRÜCKBAR). `paper_id="P00 AdHoc Permutation R4"`, `name="AdHocComposition"`.
- **`CompositionFromPermTuple<PermT>`** (`:99–111`) + `static_assert sizeof...(Vs)==19` (`:91`); `IsPermTuple19`-Concept (+ rückwärts-
  kompatibles Alias `IsPermTuple17 = IsPermTuple19`). ⇒ die ABI-Identität ist jetzt 19-Slot (Tier-Unterklassen-Invariante, Doc 30 §8.0).

## A2.5 — `builder/experiment_tree/genus_binding_traits.hpp` (3-Ebenen real + ALLE 5 Tier-Unterklassen gebunden) — verifiziert 2026-06-13

- **3-Ebenen-Modell im Code real (Doc 30 §8.0):** `AdapterGenusBindingTraits` trägt BEIDE Enums — `genus = AnatomyGenus::Adapter`
  (Ebene 2 = Tier-Unterklasse) **+ `gattung = AnatomyGattung::Container`** (Ebene 1 = Außen-Interface). Die Gattung/Tier-Unterklassen-
  Trennung ist also nicht nur Doku, sondern code-verankert (AnatomyGattung-Enum existiert).
- **ALLE 5 Tier-Unterklassen GenusBindingTraits-spezialisiert (GenusBound 5/5 — über Doc-28-„FUTURE"-Snapshot HINAUS):**
  **SearchAlgorithm** (19 Slots, verifizierter Spezialfall BR-2/3/4, `CompositionFromPermTuple→SearchAlgorithmAnatomy`, `kCompositionAxisNames`19) ·
  **Adapter** (13 Achsen unter Container-Gattung: 12 delegiert/geteilt §28 + `inner_container`, KEINE „ordering"-Achse, `Inner=DequeInner`-Default) ·
  **Set** (15, Bird K-only) · **Sequence** (11 = 10 + `axis_growth`, `DoublingGrowth`-Default) · **View** (7 = 4 + extent/layout/accessor,
  non-owning). Cross-Genus type-unmöglich → getrennte Komposition/Anatomie/Observer je Gattung (Doku 14 §32). ⇒ Doc 28 §2 „Set/Sequence/
  View = FUTURE" ist code-seitig zumindest als Binding-Traits vorhanden (SearchAlgorithm voll; übrige als Binding-Instanzen, `test_genus_binding` 5/5 Doc 29 §1).

## A2.6 — Host-Treiber/Baum/Engine-Layer (grep-verifizierte Präsenz, deckt Doc 27/29/33) — 2026-06-13

Die „WIE"-Mechanik-Dateien existieren mit ihren dokumentierten Kern-Symbolen (grep-bestätigt; Substanz = Doc 27/29/33, A1-§2/§2a):
- **`builder/experiment_tree/experiment_tree.hpp`:** `StaticBinaryView` (Mixed-Radix-Bijektion) + `binary_count()` (∏ arithmetisch) +
  `for_each_binary` (lazy Odometer) — B+-Baum-KERN (Doc 26/27/29).
- **`registry_to_axis_levels.hpp`** (`build_all_axis_levels`, BR-1) + **`composition_registry.hpp`** (`register_from_engine`, BR-2) +
  **`axis_path_serialization.hpp`** (`serialize_composition_path` = die EINE Pfad-Konvention BR-1↔BR-2↔BR-4) — die 4 Brücken (Doc 27).
- **`src/permutations/permutation_engine.hpp`** (`class PermutationEngine`, `mp_product`/`for_each_permutation`) + **5 per-Gattung-
  Engines** (`anatomy/{set,sequence,view}_permutation_engine.hpp` + SearchAlgorithm + `anatomy_permutation_driver.hpp`) — die
  Gattungs-spezialisierten Engines (Doc 29 §29, alle 5 Tier-Unterklassen).
- **`perm_runner.hpp`** (`run_workload_perm`, Wall-Clock+seg_ns je Perm) + **`cache_engine_builder_iterator.hpp`** (`lazy_try_resume`,
  `result.csv.stamp`, `resume_completed_binaries` = der Resume/Stamp-Mechanismus Doc 33 §5: Config-Stamp mit BuildVersion → copymem-v1
  wird in cowmem/cowfix-Lauf NIE als fertig gewertet) + **`tier_observe_trace_abi.hpp`** (`drive_two_phase`, Zwei-Phasen-Treiber Doc 33).
- **`build_orchestrator.hpp`** (`provision_all`, KF-16b, RAM-Admission) + **`coverage_selection.hpp`** + **`ceb_generator.hpp`** +
  **`inverse_signature_eval.hpp`** (KF-15 Signatur-Projektion) + **`pruef_dock/search_algorithm_dock.hpp`** (Prüf-Dock).
⇒ Die Host-/Baum-/Engine-Mechanik ist code-präsent + dokumentations-konform (keine festgestellte Doku-↔-Code-Drift); Volltext nur bei
konkretem E-Aufgaben-Bedarf nötig (z.B. Resume-Stamp-Härtung K8, perm_runner→V2-POD bei Q2-Schritt-4).

## A2-FAZIT (Code-Pre-Read, 2026-06-13)

**Der IST-Code entspricht den Architektur-Docs — an mehreren Stellen ist er VORAUS dem Doc-30/28-Snapshot:** (1) AdHocComposition ist
**19 Slots** (q1/q2 real integriert); (2) der konsolidierte Observer-POD-Schema ist für **ALLE 19 Achsen befüllt** (nicht nur 2); (3)
`observe_all` hält **9 reale Achsen-Organe**; (4) **alle 5 Tier-Unterklassen** haben GenusBindingTraits (3-Ebenen-Enum `AnatomyGattung`
real). **Der EINE verbleibende echte Mess-Defekt = Befund-2/Q2-Schritt-4** (`search_organ_`-Monolith beschattet node/layout; volle Such-
Delegation offen) = Audit-K5/K6 = klarer E-Aufgaben-Kandidat. **Keine Doku-↔-Code-Drift im Mess-Kern.** A2 substantiell abgeschlossen.

## A2-Lese-Fortschritt (Checklist)
- ✅ `anatomy/abi_adapter.hpp` (Mess-Kern, Befund-2, I1-POD, Q1-Sequenz, Pfad-A/B, CoW — am Code gegen Doc 24/30/31/33 verifiziert)
- ✅ (A1, frühere Session) `anatomy/composition_concept.hpp` · `builder/experiment_tree/experiment_tree.hpp` (B+-Baum-Substanz)
- ✅ `anatomy/observable_tier.hpp` (EIN konsolidierter POD axis_stats[19][8]+seg_ns[19]+Meta, sizeof 1400; Schema ALLE 19 befüllt;
  IObservableTier:public IDriveableTier B1-Split — gegen Doc 31 verifiziert; → A2.2)
- ✅ `anatomy/search_algorithm_anatomy.hpp` (Container-API entfernt; observe_all hält 9 reale Achsen-Organe via ObservableXxx-Hüllen;
  ZWEI Observe-Mechanismen [Anatomie-observe_all vs abi_adapter-fill_observer_v3] — gegen Doc 14/29 verifiziert; → A2.3)
- ✅ `anatomy/composition_factory.hpp` (AdHocComposition = 19 Slots; → A2.4) · `builder/experiment_tree/genus_binding_traits.hpp`
  (3-Ebenen-Enum real + 5 Tier-Unterklassen gebunden; → A2.5)
- ✅ Host-/Baum-/Engine-Layer (experiment_tree/registry_to_axis_levels/composition_registry/axis_path_serialization/permutation_engine +
  5 per-Gattung-Engines/perm_runner/cache_engine_builder_iterator-Resume-Stamp/tier_observe_trace_abi-Zwei-Phasen/build_orchestrator) —
  grep-verifizierte Präsenz + dokumentations-konform (Doc 27/29/33); → A2.6. Volltext nur bei E-Aufgaben-Bedarf.
- ✅ **A2 SUBSTANTIELL ABGESCHLOSSEN** (Mess-Kern + Anatomie + Composition + Genus-Bindung + Host-Layer am Code gegen Doc 14/24/27/28/29/30/31/33
  verifiziert; keine Mess-Kern-Drift; IST teils voraus). **NÄCHSTER SCHRITT: A3** (85-Audit-Befunde als Architektur-Soll-Abgleich) → **B** (Konsolidierung).
