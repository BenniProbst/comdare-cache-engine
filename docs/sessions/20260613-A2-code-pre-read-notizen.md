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

## A2-Lese-Fortschritt (Checklist)
- ✅ `anatomy/abi_adapter.hpp` (Mess-Kern, Befund-2, I1-POD, Q1-Sequenz, Pfad-A/B, CoW — am Code gegen Doc 24/30/31/33 verifiziert)
- ✅ (A1, frühere Session) `anatomy/composition_concept.hpp` · `builder/experiment_tree/experiment_tree.hpp` (B+-Baum-Substanz)
- ⬜ OFFEN: `composition_factory.hpp` (AdHocComposition<17> + CompositionFromPermTuple) · `search_algorithm_anatomy.hpp`
  (observe_all + Organ-Member, Composition-Driver-Stand Doc 29) · `observable_tier.hpp` (EIN POD + IObservableTier/
  IDriveableTier-Split) · `perm_runner.hpp` + `cache_engine_builder_iterator.hpp` (Host-Treiber + Resume/Stamp Doc 33 §5) ·
  `registry_to_axis_levels.hpp`/`profile_to_tree.hpp`/`composition_registry.hpp` (BR-1/BR-2) · `permutation_engine.hpp` ·
  `genus_binding_traits.hpp`. Dann **A3** (85-Audit-Soll-Abgleich) → **B** (Konsolidierung).
