# Strang A — KORRIGIERT: profil-getriebene Testlauf-Konfiguration (Workflow w3jvelwr3)

> **ERSETZT** den zu flachen `20260618-STRANG-A-ABSTRACTFACTORY-RUECKBAU-PLAN.md` (der die Code-Selektions-Schicht als „Factory-
> Konsument" bestehen ließ). User-Korrektur 2026-06-18, am Code BESTÄTIGT (5 Agenten, file:line). **HÖCHSTE PRIO.** Deckt #168/#162/#156.

## AUSFÜHRUNGS-STAND (je Increment round-trip-verifiziert + committet)
- ✅ **Inc 1 (S3-Kern, `bc1f7a3`):** Naht gezogen — profile_runner → offizielle `build_axis_levels`; base_pilot.profile.xml + test_profile_roundtrip → binary_ids byte-identisch zum Code-Pfad (Diff leer). Additiv, nichts entfernt.
- ✅ **Inc 2 (S1+S2, `57961f1`):** Schema +4 Felder (working_set_sweep/axis_sweep/sota_series/run_options, rückwärts-kompatibel) + dyn-Dim EINE Quelle + `m3v2_study.profile.xml` (volle Selektion). Basis-320 POSITIONS-IDENTISCH zu FullPilot + full[0] matcht reale `tier150_measurements.csv`.
- ⏭️ **Inc 3 (S3-complete):** Treiber konsumiert die 4 neuen Felder (working_set-Sweep-Loop / axis_sweep / sota_series / run_options + tier-Level für Basis-320 ignorieren) → profil-getriebener Lauf == Code-Lauf.
- ⏭️ **Inc 4 (S4+S5):** SourceGen aus Profil + **Code-Selektion ENTFERNEN** (PilotAxes/SelectMode/m3v2_select_profile).
- ⏭️ **Inc 5 (S6+S7):** SOTA/PRT-ART real (#162) + `CEB::run_profile`. ⏭️ **Inc 6 (S8+S9):** Voll-Verifikation + Doku + Bump.

## DIE KORREKTUR (warum der alte Plan falsch war)
Mein alter Plan reparierte die **Anatomie-Naht-Fabrik** (PilotAxes→genus-Engine, sota_*-Strings→MergeAxis) und ließ die Code-
Selektions-Schicht als „Factory-Adapter" stehen. **Das ist der vom User benannte Blödsinn.** Die **WHAT-Konfiguration** (welche
Lebewesen/Achsen/Sweeps/SOTA/Working-Set) gehört laut 4-Subsystem-Modell M **NICHT in Code**, sondern in ein **deklaratives
Diplomarbeit-gesteuertes Mess-Profil**, das der **CacheEngineBuilder** über das Prüf-Dock fährt.

## HARTER BELEG (claim bestätigt)
- **SOLL (Doc `architektur/10_schichten_modell_M.md`:64-119):** „Diplomarbeit … konfiguriert/triggert ▼ CacheEngineBuilder … ▼
  CacheEngine ◄▶ Prüfling"; §2.2 „messung_driver ruft `CacheEngineBuilder::build(config_xml)`"; §4 „Außen-API: CLI + XML-Config".
  → Mess-Config ist offiziell **XML-Profil der Diplomarbeit**, kein Code.
- **Mechanismus EXISTIERT (aber verwaist):** `ThesisProfile` (xml_config_parser.hpp:107-128: base_tiers/permute_axes/workloads/
  repetitions/modes[merge=Stufe1/2/3,pruefling,replaces_axes]) + `parse_thesis_profile` (:144) + **die Brücke** `build_axis_levels`
  (`profile_to_tree.hpp:25`: Profil→`std::vector<AxisLevel>`) + Vorlage `algorithm_profiles/thesis_profiles/cacheline_study.profile.xml`
  (deklariert base_tiers/permute_axes/modes A/B/C/compile_dims/repetitions). KF-1/2/7/8/9 alle completed.
- **IST umgeht das VOLLSTÄNDIG:** Grep `tests/unit/thesis_tiere/` nach `parse_thesis_profile|build_axis_levels|ThesisProfile|.profile.xml`
  = **0 Treffer.** Die Produktiv-Harness selektiert hartkodiert: `lazy_pilot_engine.hpp:65-90` PilotAxes-Template + `m3v2_select_profile.hpp:61`
  sota_*-Strings (liest trotz Namen KEIN Profil) + `run_lazy_150.cpp:192-220` axis_to_level-Map + SelectMode-Branches.

## DEVIATION-MAP
**BEHALTEN (offizielle Mechanik, kein Blödsinn):** CEB-Ausführung `run_lazy_static_then_dynamic` (cache_engine_builder_iterator.hpp:433) ·
Prüf-Dock `search_algorithm_dock.hpp` · Profil-Parser+Brücke `parse_thesis_profile`/`build_axis_levels` · LP-XML-Lastprofile (schon
sauber profil-getrieben) · Anatomie-Naht-Fabrik (CompositionFromPermTuple/generate_all_real) · AlgorithmResourceControl · Lazy-Compile
(1 DLL=1 TU).
**ENTFERNEN (Code-Selektions-Blödsinn):** `lazy_pilot_engine.hpp` PilotAxes-Template + build_pilot_levels + pilot_dynamic_dims ·
`run_lazy_150.cpp` SelectMode-Branches + axis_to_level-Map + select_search_algo_grid · m3v2-/sota_*-String-Konstanten.
**SALVAGEABLE → PROFIL:** Working-Set-Sweep · Per-Achsen-Sweep · SOTA-Reihen A/B/C · die 5 CSV-Tags · die 320er-Faktorisierung —
alle als **Profil-Inhalt**, nicht Code.

## KORRIGIERTE SCHRITTE (S0–S9)
- **S0** (Doku): Doc 10 §4 + Doc 34 Mess-Pfad fixieren „Selektion = Diplomarbeit-Profil, nicht Code". #168 umformulieren („als
  comdare_thesis_profile-Felder, die der CEB via build_axis_levels liest"); #162 als `<base_tiers>+<sota_series>+<modes>`-Track.
- **S1** (Schema +4 Felder, deklarativ): `comdare_thesis_profile` (SCHEMA.md + ThesisProfile-Struct + parser + build_axis_levels) um
  `<working_set_sweep>{N}` · `<axis_sweep axis=.. baseline=index0>` · `<sota_series id=A|B|C lebewesen=.. merge=Stufe1|2|3>` ·
  `<run_options cap/platform/build_version/resume>` + `<load_profiles ref=..>` ergänzen.
- **S2** (Profil-Autoring): `m3v2_study.profile.xml` schreiben = die heutige Code-Selektion deklarativ (base_tiers=7 SOTA+prt_art;
  permute_axes=4 variiert+4 vertieft mit value-Teilmengen; sota_series A/B/C=Stufe1/2/3; working_set_sweep; repetitions=3; workloads).
- **S3** (DIE EINE fehlende Naht): `build_axis_levels(profile)` als EINZIGE AxisLevels-Quelle des Laufs. run_lazy_150/neuer
  profile_runner liest `COMDARE_THESIS_PROFILE`→parse→build_axis_levels→tree.build. Ersetzt build_pilot_levels<FullPilot>. DynamicDims
  aus `runtime_dynamic` statt pilot_dynamic_dims.
- **S4** (SourceGen aus Profil): make_pilot_source_gen→profil-getriebene SourceGenFn über register_from_engine/generate_all_real/
  ceb_generator (KF-8). Lazy-Compile (1 DLL=1 TU) bleibt erhalten.
- **S5** (ENTFERNEN statt degradieren): m3v2_select_profile.hpp löschen/auf RowTags reduzieren; PilotAxes/FullPilot/SmallPilot raus
  oder Klein-Test-Fixture; axis_to_level + SelectMode-Branches raus.
- **S6** (SOTA/PRT-ART real #162): `<base_tiers profile_ref=sota/*.xml>` + `<mode pruefling=prtart replaces_axes=..>` → build_axis_levels
  Tier-Fanout + IPrueflingFactory/pruefling_merge Stufe1/2/3 (assert_pruefling_slot_genus bleibt).
- **S7** (EINE CEB-Eintritts-API): `CacheEngineBuilder::run_profile(profile_path, run_options)` schaffen (messung_driver/Diplomarbeit
  triggert); alten 4-XML-ExperimentDriver + Lazy-Weg darunter zusammenführen; build_and_measure_150_tiere.ps1 → reiner BUILD+LAUNCH-Wrapper.
- **S8** (Verifikation, literal): **(a) HART: Round-Trip** — m3v2-Profil erzeugt IDENTISCHE binary_ids wie der heutige Code-Pfad
  (Resume #139 darf NICHT brechen) — Gate vor jedem Voll-Lauf. (b) Grep-Beleg: thesis_tiere/ enthält parse_thesis_profile, KEIN
  PilotAxes/sota_lebewesen_names/COMDARE_SELECT_MODE mehr. (c) Klein-Profil-Lauf grün, CSV-Tags nachweislich aus XML. (d) test_kf1/kf9 grün.
- **S9** (Abschluss): Doc 27/34 nachziehen; 3-Repo-Bump (Diplomarbeit-Pointer mit); Ledger/#156/#162/#168.

## REIFEGRAD / WIE TIEF (ehrlich)
**Mittel-tief, NICHT von-null.** Der Mechanismus existiert (Parser+Brücke+Vorlage+KF-1/2/7/8/9 done), ist nur orphaned + hat 4 Schema-
Lücken. Rückbau = (1) die EINE fehlende Naht ziehen (Harness→build_axis_levels), (2) Schema +4 Felder, (3) `run_profile`-Eintritts-API,
(4) PilotAxes/SelectMode/m3v2-Strings **entfernen** (nicht degradieren).

## RISIKEN
- **binary_id-Drift** → Resume #139 bricht: build_axis_levels muss die 19 Slot-Namen + DynamicDim-block_ids EXAKT reproduzieren →
  Round-Trip-Gate (S8a) Pflicht vor Voll-Lauf.
- **Schema-Lücken real** (4 Konstrukte) → Parser+Struct+build_axis_levels + test_kf1/kf9 anpassen.
- **Zwei CEB-Wege** (ExperimentDriver vs Lazy) → zunächst nur Lazy profil-getrieben, ExperimentDriver später.
- **SOTA/PRT-ART-Bau** (#162 in_progress) → Profil-Umstellung verlagert nur die Steuerung, löst das Bau-Problem nicht; bis dahin A/B/C über Basis-Binaries.
- **Lazy-Compile darf nicht brechen** → SourceGen explizit über generate_all_real/ceb_generator (1 DLL=1 TU), kein monolithischer Engine-Typ.
- **Working-Set-Sweep** heute äußere PS-foreach → als äußere Lauf-Iteration belassen, nur N-Liste aus Profil.

> **Kernsatz:** Die Diplomarbeit steuert die Messung über ein deklaratives Profil; der CacheEngineBuilder führt aus. Die ganze
> Code-Selektions-Schicht (PilotAxes/SelectMode/m3v2_select_profile) wird ENTFERNT, nicht in einen Factory-Konsumenten umgebaut.
