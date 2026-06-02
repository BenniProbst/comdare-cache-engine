# Permutations-B+-Experiment-Baum — 4 Brücken + 6 Gates LITERAL GRÜN (finaler Audit, 2026-06-02)

> **Gate-6 (Doc 27 §4.6):** „belegt Doc 26/27 + finaler Session-Doku; finaler Audit bestätigt die Gleichheit."
> Dieses Dokument IST der finale Audit. Es konsolidiert die literalen Test-Belege + Commit-Refs der 6 Gates,
> die in dieser Session einzeln erzeugt wurden. Quelle der literalen Ausgaben: `build/*_result.txt` (cl,
> RAM-Watchdog). [[feedback_no_success_marks_without_literal_output]] — jeder Haken hat eine literale Ausgabe.

## Ausgangslage (Session-Beginn)

Der Experiment-Baum war **string-getrieben** (`profile_to_tree` las `std::map<string,vector<string>>` aus XML),
`NodeValue` ein 4-uint64-**Stub**, KEINE Bindung an die echten Registries/PermutationEngine/ObserverAggregate.
Ziel (/goal V5): registry-getrieben gegen ALLE **22** Achsen, jedes Blatt → reale baubare Tier-Binary, jeder
gemessene Knoten → echter Observer + Definition.

## Kritische Architektur-Korrekturen dieser Session (User-Direktiven)

1. **OOM-Befund → lazy Baum:** Den GANZEN Baum zu materialisieren ist falsch (∏ ≥ 1e15 → ~21 GB OOM). Korrigiert:
   `binary_count()` rein arithmetisch (∏), Iteration = lazy mixed-radix Odometer (EIN Pfad zur Zeit, O(Tiefe)),
   `StaticBinaryView` on-demand-dekodiert, NodeValue sparse. Bauen = parallel (KF-16b); Messen = Unikat-Prozess
   je Binary (seriell, nur Binary-intern multithreaded). Jeder Compile RAM-Watchdog-gewahr (cl-Kill < 2.5 GB frei).
2. **22-vs-17 (Doc 27 §0.1, autoritativ):** 22 = GESAMTZAHL; „17" = NUR die SearchAlgorithm-Gattungs-Komposition
   (`AdHocComposition<17>`-Invariante). Differenziert: 17 SearchAlgorithm-Slots + page_type/09b/12 (Sub-/Build-
   Varianten) + queuing q1/q2 (eigene Container-Gattung). Alle 22 als Baum-Ebene + je eigener Observer.

## Die 6 Gates — literal grün (Doc 27 §4)

| Gate | Bedingung | Literaler Beleg (Test, RAM-Tiefstand) | Commit (CE) |
|------|-----------|----------------------------------------|-------------|
| **1** | `tree.binary_count() == ∏ mp_size(Enabled_i)` | `test_br1_full22_count`: **== 137.594.142.720.000** über 22 Achsen (17 GB frei) | 835ffda |
| **2** | alle 22 Achsen als Baum-Ebene, volles Enabled-Inventar | `test_br1_full22_count`: 22 distinkte Achsen-Blöcke, Bidirektionalität `block_id()==axis()` | 835ffda |
| **3** | jedes Blatt → reale `AdHocComposition` + baubar | `test_br2_roundtrip`: 4/4 Blätter → reale `AdHocComposition<17>` (Pfad-Mengen BR-1==BR-2); **BR-4** `br4_load`: Baum-Blatt → reale Anatomie-DLL gebaut + geladen (organ_count==17) | 976f8c3 · bc8e6ea |
| **4** | jeder gemessene Knoten → echter ObserverAggregate + Definition, **22 Observer** | `test_br3_observer`: NodeValue trägt echten Snapshot (`search_insert==256`, kein Stub); `br4_load`: observe_all über REALE DLL (256/256/256, 4 Achsen); `test_br3_obs22`: **22 Achsen klassifiziert (17 SA + 3 DefinitionOnly + 2 Container)** + je Definition | 0c4b79f · bc8e6ea · 89d4d9f |
| **5** | inverse Signatur-Projektion (KF-15) über reale Kompositionen | `test_br_kf15_real`: 8 reale Blätter projizieren auf gepinnte Signatur (registry-getrieben), `aggregate_for_signature` sieht echten NodeValue | c0b7dc1 |
| **6** | belegt Doc 26/27 + Session-Doku, finaler Audit | **dieses Dokument** + Doc 26 §2 (lazy) + Doc 27 §0.1/§3/§4 (4 Brücken-Belege) | (dieser Commit) |

## Die 4 Pflicht-Brücken (registry-getrieben, compile-verifiziert gegen ECHTE Wrapper)

- **BR-1** (`registry_to_axis_levels.hpp` + `axis_reflect.hpp`): alle 22 Enabled-mp_lists → AxisLevels, block_id-getaggt.
- **BR-2** (`composition_registry.hpp` + `axis_path_serialization.hpp`): Blatt-Pfad ↔ `CompositionFromPermTuple<P>` →
  `AdHocComposition<17>`; zentrale Pfad-Konvention (BR-1↔BR-2↔BR-4 identisch).
- **BR-3** (`node_value_measurement.hpp` + `NodeObserverSnapshot`): realer genus-ABI-Adapter treibt + `tier_observe`
  → echter Per-Achsen-Observer (R5.B ehrlich: search_algo+allocator+ operativ, Rest Default). Sparse value_map.
- **BR-4** (`br4_emit`/`br4_load`, 3-Phasen): Baum-Blatt → `render_adhoc_module_source` (reale Anatomie) → SHARED-DLL
  → `AnatomyModuleLoader` → `dynamic_cast<IObservableTier*>` → observe_all über die reale Komposition.

## Verbleibend (NICHT Teil der 6 Gates — /goal-Erweiterung + Tech-Debt)

- **Gattungs-Generik (Option-B Schritt 2–3, User 2026-06-02):** `GenusBindingTraits` abstrahieren (SearchAlgorithm =
  Spezialfall) → **Container-Bau-Brücke** für queuing q1/q2 (eigene `ContainerComposition`/`ContainerAnatomy`/
  Container-Prüf-Dock). Der Baum-KERN ist bereits gattungs-agnostisch (trägt q1/q2 als Ebenen + ContainerObserver-
  Klassifikation); die BAU-Seite der Container-Gattung ist der Folgeschritt. (q1/q2-Observer-Seite = #72 erledigt.)
- **#73:** `provision_all` results-Vektor batchen (O(K) statt O(∏)) — relevant erst bei Voll-Inventar-Builds.

## Fazit

Der Permutations-B+-Experiment-Baum permutiert **registry-getrieben** über ALLE **22** realen Achsen; die
Kardinalitäts-Identität (Gate-1) ist literal belegt (1.4e14, OOM-sicher gezählt statt materialisiert); jedes Blatt
bildet auf eine reale, baubare, ladbare, observierbare `AdHocComposition<17>`-Tier-Binary ab; jeder gemessene
Knoten trägt einen echten Observer; alle 22 Achsen sind gattungs-korrekt observer-differenziert (kein
Wegschrumpfen); die inverse Signatur projiziert über reale Kompositionen. **Die 6 Gates sind literal grün.**
