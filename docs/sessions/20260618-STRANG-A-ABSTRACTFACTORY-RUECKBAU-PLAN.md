# Strang A — AbstractFactory-of-Anatomy-Rückbau: ausführbarer Plan (Workflow wuz2dbsnu)

> **Ergebnis der elaboraten Planungssession 2026-06-18** (5 Agenten, 10 Dateien code-verifiziert). Steuert die User-Direktive:
> die Mess-Selektion als Abstract Factory der Anatomie ausprägen, alle Session-Lösungen auf die offizielle Architektur anwenden.
> **HÖCHSTE PRIO der Folge-Session.** Deckt Tasks #168 + #162. Voller Workflow-Output: `tasks/wuz2dbsnu.output`.

## VERDIKT (entscheidend, entlastend)
Der Rückbau ist **KLEIN und chirurgisch** — er betrifft fast ausschließlich die Mess-SELEKTIONS-/ORCHESTRIERUNGS-Schicht in
`tests/unit/thesis_tiere/`, **NICHT die Anatomie**. Code-verifiziert: die offizielle Abstract-Factory-Ausprägung der Anatomie
**existiert bereits vollständig und sauber**. Es ist KEINE neue Struktur zu bauen. Entscheidend: die erzeugte DLL-Quelle IST schon
die offizielle Anatomie — der Pilot ruft `CompositionFromPermTuple<P>` + `render_adhoc_module_source` (pilot_source_map.hpp:41-42),
**nutzt also die Naht-Fabrik, dupliziert sie NICHT**. Alle inhaltlichen Session-Vertiefungen (6 Achsen, 5 Layouts, seg-Coverage, CLU,
PMAJOR-04, Working-Set-N, Resume) liegen schon auf den 19 offiziellen Organen/der Observer-Schicht. Netto: **~3 Dateien** anpassen.
`DeepPilotAxes` ist gar kein Code (0 Source-Treffer) — nichts auszubauen.

## DIE OFFIZIELLE FACTORY EXISTIERT BEREITS (5 Bausteine — nur konsequent statt parallel nutzen)
1. **Konkrete Factory** = `anatomy::SearchAlgorithmPermutationEngine<TopicConfigSets...>` (search_algorithm_permutation_engine.hpp:70-199):
   genus-Marker + `for_each_composition_type` (Visitor über den Raum) + `assert_pruefling_slot_genus` (compile-time Gattungs-Constraint)
   + Zugang zu pruefling_merge (3 Joins). **PilotAxes umgeht das mit der bare `perm::PermutationEngine`.**
2. **Metaprog-Naht (Produkt)** = `anatomy::CompositionFromPermTuple<PermT>` (composition_factory.hpp:82-111): PermTuple<V0..V18> →
   GENAU EINE `AdHocComposition<T0..T18>` (19 named Slots, static_assert exakt 19). **Das ist das „original sezierte Wiederzusammennähen".**
3. **Blatt→Komposition** = `CompositionRegistry::register_from_engine` (composition_registry.hpp:60-71, BR-2): slot_path==path==binary_id.
4. **Komposition→DLL** = `codegen::adhoc_emitter` via `ceb_generator::generate_all_real` (BR-4).
5. **Laufzeit-Factory/Prüfling** = `IPrueflingFactory`/`IPrueflingRegistry` (i_pruefling_factory.hpp:34-57) + AbstractFactory-Prüfling-Slot
   (pruefling_merge.hpp EmptyPrueflingSlot + MergeAxis Stufe1/2/3).

## DEVIATION-MAP
**SAUBER (NICHT anfassen):** AdHocComposition/CompositionFromPermTuple · genus-Engine · pruefling_merge (3 Joins) · composition_registry ·
known_compositions_list (11 SOTA real) · die 6 Achsen-Vertiefungen · LayoutAwareChunkedStore (5 Layouts) · seg-Coverage/CLU/PMAJOR-04 ·
Working-Set-N/Resume/CSV-Tags · `make_basis`/`make_axis_sweep` (flat_index = tree-true) · DeepPilotAxes (kein Code).
**PARALLEL (rückbauen):**
- `lazy_pilot_engine.hpp` PilotAxes::Engine = bare `perm::PermutationEngine` → durch genus-Engine ersetzen (Slot+Joins).
- `lazy_pilot_engine.hpp` PilotAxes L00..L18 inline `mp_take_c` + `static_levels()` → Reduktion in Registry-EnabledStrategies (Single-Source).
- `m3v2_select_profile.hpp` `sota_lebewesen_names` (String-Dup) + `sota_row_tags` (HELD-Tags) → reale `MergeAxis` Stufe1/2/3 + `KnownReferenceCompositions`.
- `run_lazy_150.cpp` `axis_to_level`-Map (192-199) + `sota:`-Branch + `build_pilot_levels<FullPilot>` → Factory-Level-Namen + registry-Engine.
- `build_pilot_source_map` (Pilot-Brücke) → auf die EINE Brücke `register_from_engine`/`generate_all_real` zusammenführen.

## SELEKTION ALS FACTORY-PARAMETER (statt Parallel-Konfig)
- **Basis-320:** Engine aus benannten, IM TopicConfigSet/Registry deklarierten Enabled-Teilmengen → `build_all_axis_levels` (BR-1) →
  StaticBinaryView; `make_basis` bleibt dünner `select_explicit(0..N-1)`-Pass.
- **Sweep:** `make_axis_sweep` unverändert (flat_index), aber auf Factory-View + Factory-Level-Namen.
- **SOTA-Reihen A/B/C** = Factory-Parameter `MergeStrategy{Stufe1_CeOnly, Stufe2_PrueflingReplace, Stufe3_FullJoin}` (NICHT RowTag-Strings);
  6 SOTA aus `KnownReferenceCompositions`, je 1 Punkt in den Raum gehoben → `for_each_composition_type` → DLL; PRT-ART über IPrueflingFactory.
- **Working-Set-N/Repetition/Workload** bleiben DynamicDims (nicht teil der binary_id) — unverändert.

## SCHRITTE (S0–S9, geordnet)
- **S0** (kein Code): Spec + Doc 34 §3 als Soll fixieren; #168 umformulieren („… in die Factory-EnabledStrategies über die offizielle Engine"); #162 = realer-Joins-Track.
- **S1** Registry-Deklaration: zu variierende Teilmengen als benannte Enabled-Teilmengen IM TopicConfigSet (z.B. `M3v2EnabledSearchAlgo`); Reduktion raus aus lazy_pilot_engine.
- **S2** Factory-Fassade `AnatomyAbstractFactory` (header-only) — bündelt `build_all_axis_levels` (BR-1) + `CompositionRegistry::lookup` (BR-2) + `generate_all_real` (BR-4); KEINE neue Logik.
- **S3** PilotAxes::Engine → offizielle genus-Engine; AxisLevels aus `build_all_axis_levels`; binary_ids identisch (Round-Trip-Test).
- **S4** run_lazy_150 entschlacken: axis_to_level-Map + sota-Branch raus; Factory liefert Level-Namen + Joins-Selektion.
- **S5** SOTA real: `sota_lebewesen_names`→KnownReferenceCompositions; `sota_row_tags`→`make_sota_series` via MergeAxis + IPrueflingFactory (PRT-ART, assert_pruefling_slot_genus).
- **S6** Brücke vereinheitlichen: `build_pilot_source_map`→`register_from_engine`/`generate_all_real`.
- **S7** Parallel-Welt degradieren: PilotAxes/FullPilot/SmallPilot auf Factory-Adapter reduzieren; m3v2_select_profile behält nur make_basis/make_axis_sweep + reine Metadaten-Tags.
- **S8** Verifikation: (a) Round-Trip `serialize_composition_path<P> == binary_id` als **harte Gate-Bedingung** · (b) Klein-Pilot grün · (c) N-Sweep zeigt binary_id = reine 19-Organ-Rekombination · (d) A/B/C aus realen MergeAxis-Joins. Doc 34 + Spec §3c/§8 nachziehen.
- **S9** Abschluss: 3-Repo-Submodul-Bump (Tag+Commit+Push); Ledger/#156/#162/#168 aktualisieren.

## RISIKEN (+ Mitigation)
- **Round-Trip-Drift** (binary_id bricht → Resume #139 bricht) → S8(a) harte Gate-Bedingung vor jedem Voll-Lauf.
- **Compile-Explosion (C1060)** → Reduktion klein in Registry-Teilmengen; Materialisierung je separat emittiertem perm_<id>.cpp (1 DLL = 1 TU), nie gesammelt.
- **PRT-ART-Prüfling-Gattung** (Cross-Genus-Join unmöglich) → Slot::genus explizit SearchAlgorithm; #162 ist genau dieser Track.
- **SOTA-DLL-Baubarkeit** → je Reference-Composition Round-Trip-Bau im Klein-Pilot vor Voll-Lauf.
- **Scope-Creep ins Anatomie-Level** → Rückbau betrifft AUSSCHLIESSLICH die Selektions-Schicht; libs/cache_engine/axes|anatomy unberührt.
- **Mess-Vergleichbarkeit alt/neu** → build_version-Tag trennen (m3v2→m3v2-factory); Round-Trip garantiert identische Tupel→identische binary_ids.

> **Kernsatz für die Folge-Session:** Die Architektur ist schon da. Umgebaut wird NUR die dünne Selektions-Schicht (~3 Dateien), die
> zum Konsumenten der EINEN Factory wird. Gate-frei vor dem (separat HELD) Voll-Lauf abschließbar.
