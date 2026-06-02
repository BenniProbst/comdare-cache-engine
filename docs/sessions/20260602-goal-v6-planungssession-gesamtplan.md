# Goal-V6 Planungssession — verabschiedeter Gesamt-Umsetzungsplan (2026-06-02)

> **Goal-V6 Phase C (elaborate Planungssession).** Dieses Session-Dokument konsolidiert die drei adversarial-
> präzisen Umsetzungspläne (Planungs-Agenten a0c96bb #74-Achsen · a94f574 Gattungen · a8de276 Skalierung) in EINEN
> verabschiedeten Gesamtplan: Arbeitsstränge, Reihenfolge, Abhängigkeitsgraph, Sprints (Dateien · Kriterium · Gate ·
> Aufwand), ZIH-Cluster-Integration (gate-frei vs GATE-MAXIMAL). Basis: SOLL `28_vollstaendigkeits-kartographie.md`,
> Lücken `goal-v6-luecken-ledger.md`. **Dies ist das Abschlusskriterium des gesetzten /goal** („bis alles durch eine
> elaborate Planungssession als erledigt gilt") — der Plan, den Phase D abarbeitet und Phase E adversarial auditiert.

## §0 Leitprinzipien (verbindlich für die gesamte Umsetzung)

1. **OOM-/Lazy-Disziplin:** NIE der ganze Baum — immer nur EIN Pfad Wurzel→Blatt (mixed-radix Odometer, O(Tiefe)); `binary_count` arithmetisch; `BuildSelection` ist der Schutzwall vor O(∏). Jeder Compile RAM-Watchdog (cl-Kill < 2.5 GB frei).
2. **Bauen parallel / Messen seriell:** K parallele DLL-Builds (keine Oversubscription); Messen = Unikat-Prozess je Binary (seriell, nur Binary-intern multithreaded).
3. **22 schrumpfen nie auf 17:** `AdHocComposition`/`ObserverAggregate` bleiben 17 (Gattungs-Invariante); die 3 Build-Achsen + 2 Container-Achsen werden GETRENNT getragen.
4. **Keine Erfolgsmarke ohne literale Tool-Ausgabe; keine „declare victory by reclassification".** Jeder Sprint compile-verifiziert gegen ECHTE Wrapper.
5. **3 Repos nach jedem Schritt:** commit + push + Submodul-Sync (DA-Pointer bumpen nach ce-Push). Destruktive Ops nur mit Tag+Commit+Push.
6. **Doku nie löschen** (nur git mv). Kein Python in der Buildchain. Compile-Time-Only / kein Runtime-Switch im Hot-Path.
7. **ZIH-Submission = GATE-MAXIMAL** (mit User absprechen); Skript-Emit + Container-Rezept + Ingest sind lokal gate-frei.

## §1 Drei Arbeitsstränge (aus den 3 Planungs-Outputs)

| Strang | Inhalt | Lücken | Charakter |
|--------|--------|--------|-----------|
| **A — Fundament/Skalierung/Tech-Debt** | BuildSelection, #73 O(K), #77 ceb_generator, runtime_measure_visitor, E2E-Pfad, ZIH | L-SEL, L-73, L-77, L-MEAS, L-CLUSTER | gate-frei (außer ZIH-Submission); kleinste Eingriffe, höchster Hebel |
| **B — 22 Achsen Observer/Build (#74)** | page_type/09b/12 Build-Varianten + Definition; R5.B-Operativität | L-74a/b/c | Gate-4-real |
| **C — 5 Gattungen (#75/#76)** | Container q2+DLL; Set; Sequence+axis_growth; View+3 Achsen; Viren | L-75, L-76a/b/c/d | Gattungs-Vollständigkeit; größter Umfang |

## §2 Abhängigkeitsgraph (kritischer Pfad)

```
A1 BuildSelection (L-SEL) ──► A2 #73 provision_all O(K) (L-73)
       │
       └──────────────► A5 E2E-Treiber (L-CLUSTER gate-frei) ◄── A4 runtime_measure_visitor (L-MEAS)
                                  │
C1 Container q2+DLL (L-75) ──► C2 Makro-Generalisierung COMDARE_DEFINE_GENUS_MODULE
       │                            │
       │   (etabliert gattungs-generischen DLL-Pfad)
       ▼                            ▼
   B1 page_type/09b/12 Build-Variant-Emit (L-74a) ──► B2 BuildVariantDefinition + ABI-Pull (L-74b)
                                                              (nutzt B1-Inspection-Symbole)
   B3 R5.B-Operativität (L-74c)  ── unabhängig, parallel zu B1/B2 startbar
       │
   C2 ──► C3 Set (L-76a) ──► C4 Sequence+axis_growth (L-76b) ──► C5 View+3 Achsen (L-76c)
   C6 Viren (L-76d) ── separater Strang, parallel/zuletzt
A3 #77 ceb_generator-Präzisierung (L-77) ── unabhängig, schnelle Ehrlichkeits-Korrektur, früh
       ═══════════════ GATE-MAXIMAL-LINIE (mit User) ═══════════════
   A6 comdare-ce.sif + sbatch-Submission + Webhook-Deploy (L-CLUSTER GATE-MAXIMAL)
```

## §3 Verabschiedete Sprint-Reihenfolge (Phase D)

> Prinzip: gate-freie Fundament-/Ehrlichkeits-Korrekturen zuerst (kleinster Eingriff, sofort literal grün),
> dann der gattungs-generische DLL-Pfad (Voraussetzung für #74 + Rest-Gattungen), dann #74, dann Rest-Gattungen,
> dann Verbindung (E2E/Messung), zuletzt GATE-MAXIMAL Cluster, dann Phase E.

| Sprint | Lücke | Inhalt | Verifikations-Kriterium (literal) | Gate | Aufwand |
|--------|-------|--------|-----------------------------------|------|---------|
| **D1** | L-SEL | `coverage_selection.hpp` + `BuildSelection` (full/one_wise/pinned/explicit) | `select_one_wise(48-Pilot).size()==max(level sizes)`; jede Variante ≥1× | frei | M |
| **D2** | L-73 | `provision_all(view, BuildSelection)` O(K); Rückwärts-Overload | Stub-View 1e12 + 5 Indizes → `results.size()==5`, kein OOM; `test_kf16` grün | frei | S |
| **D3** | L-77 | ceb_generator-Doku + Doc 26 §7 KF-8 + Task #70-Titel präzisieren (4.B); optional `generate_all_real<Engine>` (4.A) | Grep: ceb_generator-Doku nennt adhoc_emitter als realen Pfad; (4.A) gebaut→load→observe grün | frei | S(+M) |
| **D4** | L-75 | Container q2-Slot (`ContainerComposition<Q1,Q2>`) + `IContainerTier`+`ContainerObserverSnapshotV1` + `ContainerAbiAdapter` + `COMDARE_DEFINE_CONTAINER_MODULE` + `ContainerDllDock` | slot_count==2; organ_count()==2; **DLL-Round-Trip** genus()==Adapter, `dynamic_cast<IContainerTier*>≠null`, put/get über DLL, `tier_observe_container` put_count==N | frei | M |
| **D5** | (C2) | Makro auf `COMDARE_DEFINE_GENUS_MODULE(AnatomyTmpl,AdapterTmpl,Comp…)` generalisieren | beide Gattungen (SA+Container) bauen über dasselbe 4-Symbol-Gerüst; Loader unverändert | frei | S |
| **D6** | L-74a | page_type/09b/12 Build-Variant-Emit + 3 Inspection-Symbole + `hw_cross_constraint.hpp` | `comdare_build_simd_width_bits()==256` vs `512` (2 Varianten 1 Komposition); Cross-Constraint 96→~25 via mp_size | Gate-3+ | M |
| **D7** | L-74b | `BuildVariantDefinitionV1` + `NodeValue.build_def` + ABI-Pull-Reader | `build_variant_definition<DenseByte,Avx512,X86_64>().vector_width_bits==512`; gemessen real==true / ungemessen==false; 17+3+2==22 | Gate-4 | M |
| **D8** | L-74c | R5.B-Operativität: tier_observe + observe_all über 2-4 weitere Achsen (layout/serial/telemetry/node) + ehrliche Zählung | POD-Felder >0 real (Delta vor/nach Treiben); `observable_axis_count`==literal getrieben; R5.D PMC als „pending" markiert | Gate-4 | L |
| **D9** | L-76a | Set-Gattung (nach K-A-Klärung) — volle Kette + GenusBindingTraits<Set> | `IsSetComposition`; organ_count()==14; binary_count==∏(14); DLL-Round-Trip contains==true | frei | M–L |
| **D10** | L-76b | Sequence-Gattung + NEUE Achse `axis_growth` (Goldstandard) (nach K-B) | `GrowthPolicy<DoublingGrowth>`; genus()==Sequence; push_back N→tier_size==N, tier_at korrekt | frei | L |
| **D11** | L-76c | View-Gattung + 3 neue Achsen extent/layout/accessor (nach K-C) | genus()==View; Compile-Test `tier_insert` existiert NICHT; bind→read==extern_buf[i] | frei | L |
| **D12** | L-76d | Viren produktiv: `IVirusExecutionEngine` + GraphBfs + eigene Dock-/Loader-Familie | engine_kind()==Virus; BFS-Referenzgraph korrekt; (DLL) Virus-Loader-Round-Trip | frei | M–L |
| **D13** | L-MEAS | `runtime_measure_visitor.hpp` (RuntimeVariableLoop + Zwei-Phasen + Workload) | 6 Settings × 3 Repeats = 18 Snapshots, EINE DLL, kein Reload, <1s, observer_real==true | frei | M |
| **D14** | L-CLUSTER (frei) | E2E-Treiber `e2e_pipeline.hpp`+`apps/experiment_pipeline`; `perm_runner`; `result_ingest`; `.def`/`build_sif.sh`-Text | 48-Pilot E2E: measured_node_count==gebaute DLLs; perm_runner→JSON; Ingest→NodeValue | frei | L |
| **D15** | L-CLUSTER (GATE-MAXIMAL) | `apptainer build comdare-ce.sif` + ZIH-Upload + echte sbatch-Submission + Webhook-Receiver VLAN-60 | array_size==BuildSelection.size(); Cluster-Lauf → Ergebnis-Rückführung in Baum | **GATE-MAXIMAL** | (User/Termin) |

**Phase E (nach D1–D14, GATE-MAXIMAL-Teil D15 separat):** finaler adversarialer Audit-Workflow bestätigt 22 Achsen (Ebene+Observer/Definition+Build) · 5 Gattungen (Komposition/Anatomie/Dock/DLL) · 6 Gates OHNE Vorbehalt literal grün; kein reklassifizierter oder pilot-vorbehaltener Punkt. Erst dann ist Goal V6 erfüllt.

## §4 Generische Bausteine (EINMAL, begünstigen alle Gattungen)

| Baustein | Aktion | begünstigt |
|----------|--------|-----------|
| `COMDARE_DEFINE_GENUS_MODULE` (D5) | Makro gattungs-parametrisch | D4, D6, D9–D11 |
| `AnatomyModuleLoader` | UNVERÄNDERT (gibt `IAnatomyBase*`, Gattung runtime via `genus()`) | alle DLL-Pfade |
| `IPruefDock`/`PruefDockRegistry`/`pruef_dock_sequencer` | UNVERÄNDERT (gattungs-uniform via `dock_genus()`/`accepts()`) | D4, D9–D11 |
| `GenusBindingTraits<G>` | je Gattung 1 Spezialisierung ergänzen (IST die Generik; KEINE GenusComposition<Slots…>-Über-Abstraktion — named Aliase + disjunkte Concepts sind Pflicht, Doku 14 §32) | D4, D9–D11 |
| Emitter `render_adhoc_module_source` (Text-DRY) | wiederverwenden; je Gattung eigene `*_macro_args<C>()` | D4, D6, D9–D11 |
| **Pro neue Gattung Pflicht (Doc 24 §8.8):** eigenes Antriebs-Sub-Interface (`I*Tier`) + eigener V1-POD (`*ObserverSnapshotV1`) — `IAnatomyBase`/`ComdareTierObserverSnapshotV1` NIE mutieren | | D4, D9–D12 |

## §5 ZIH-Cluster-Integration (Skalierungs-Gleichung, User-verbindlich)

- **Aufteilung (Doc 28 §5):** (a) STATISCHE Binary-Rekombinatorik → ZIH (10000 Nodes × 32 Kerne, kein RAM-Limit, SLURM-Array + Singularity); (b) je Binary ALLE dynamischen Variablen + Testdaten <1s (Xeon Platinum/Epyc, RAM-resident). `binary_count==∏ mp_size` = Kardinalität des statischen Teilbaums; reale Build-Menge ≪ ∏ (BuildSelection/Coverage wählt). Die statisch/dynamisch-Trennung macht die Größenordnungen kompatibel.
- **Übergang 1.4e14 → real:** 3 orthogonale Stufen — S1 BUILD-PROFIL (Enabled-Listen, configure-time), S2 Coverage-Sampling 1-wise über die Lazy-View (D1), S3 explizite Selektion/Pinned-Signatur. Kombiniert → endliche `BuildSelection.indices`.
- **Gate-frei (D14):** Skript-Gen + Dry-Run, perm_runner gegen lokale Pilot-DLLs, result_ingest JSON→NodeValue, `.def`/`build_sif.sh`-Text. **GATE-MAXIMAL (D15):** echte sbatch-Submission, apptainer-Build+Upload, Webhook-Deploy, VPN/SSH zu Barnard/Capella, Grace-Hopper (separat, NUR Markus Velten).

## §6 Pre-Sprint-Klärungen (vor D9/D10/D11, durch genaues Doku-14-Lesen — NICHT User fragen)

K-A Set 14 vs 15 (§32.2 vs §28-Tabelle, filter strittig) · K-B Sequence 9 vs 10 · K-C View 7 = 4 geteilt + 3 eigen. Auflösung beim jeweiligen Sprint via §28-Tabelle als feinere Quelle; bei echtem Widerspruch Planrunde.

## §7 Aufwand-Synthese + Verdikt

- **Strang A (Fundament):** D1–D3 + D13–D14 ≈ 1.5–2 Wochen (gate-frei).
- **Strang B (#74):** D6–D8 ≈ 9–12 Tage.
- **Strang C (Gattungen):** D4–D5 + D9–D12 ≈ 4–6 Wochen (Set/Sequence/View je eigene Kette + 4 neue Achsen).
- **GATE-MAXIMAL (D15):** user-/termingebunden (nicht in der Eigen-Schätzung).
- **Gesamt:** ~2–3 Wochen Eigenarbeit bei fokussierter Parallelisierung der gate-freien Stränge + Gattungs-Pipeline — deckt sich mit der User-Erwartung („2 bis 3 Wochen non-stop").

**Verabschiedet.** Dieser Plan ist die Single-Source der Phase-D-Reihenfolge. Phase D arbeitet D1→D14 ab (D15 GATE-MAXIMAL nach User-Freigabe); jeder Sprint compile-verifiziert gegen echte Wrapper mit RAM-Watchdog, commit+push+Submodul-Sync; Phase E auditiert adversarial gegen literale Evidenz. **Damit ist das Planungs-Abschlusskriterium des /goal erreicht; die Umsetzung (Phase D) beginnt mit D1.**
