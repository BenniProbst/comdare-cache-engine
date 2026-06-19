# Elaborate Übergabe 2026-06-19 — Gesamtplanung + komplexe Architektur-Tiefe

> **ZUERST LESEN** in der Folge-Session (nach Doc 34). Rolle: Implementierungs-Agent (cache-engine/Thesis). /goal aktiv (autonom,
> nicht fragen, fortfahren). Diese Datei = aktueller Gesamtzustand + die Architektur in der Tiefe. Single-Source-Kette unten (§7).

---
## §0 KRITISCH ZUERST — offener Strang + Schutz-Grenze
1. **Planrunde LÄUFT** (`wjpcw0foi` / runId `wf_1ab6ed43-c28`): ermittelt den eindeutig nächsten **freigegebenen kritischen** Punkt.
   **→ Result aus `tasks/wjpcw0foi.output` (JSON `result.plan.naechster_punkt`) abholen, dem User vorlegen, umsetzen.**
2. **User-Freigabe 2026-06-19:** „Ich gebe dir kritische nicht gate freie Arbeit autonom frei und schaue zu." = die HELD/gated
   Punkte (m3v2-Messung, Win-PMC, NAS) sind lokal autonom bearbeitbar. **ABER Schutz-Grenze:** die **ZIH/Cluster-Manöver bleiben
   absprache-pflichtig** (CLAUDE.md „Kritische Manöver — Strafen/Exmatrikulation möglich") — die generelle Freigabe deckt das
   penalty-riskante ZIH NICHT eindeutig; im Zweifel ZIH explizit rückfragen. LOKAL freigegeben: m3v2-Lauf auf dieser Maschine, Win-PMC.
3. **Alle 4 Repos committet+gepusht+synced** (User-Wunsch): super `dd95fcc` (User-Inhalte + **160-MB-Mess-CSV via git-LFS**;
   `Forschungsarbeiten/code/` 121 MB cloned bleibt .gitignored, Lizenz-pending) · cache-engine `786ff3b` · prt-art synced ·
   thesis/diplomarbeit `faf4303` (Overleaf gemerged + Diplomarbeit-PDF). **`.gitattributes` im Super → git-LFS Pflicht beim Klonen.**

---
## §1 DIE ARCHITEKTUR IN DER TIEFE (die „komplexe Tiefe")

### 1.1 Das 4-Subsystem-Modell M (wer steuert was) — `docs/architektur/10_schichten_modell_M.md`
**Diplomarbeit (messung_driver) → CacheEngineBuilder (CEB) → cache-engine (CE) ↔ Prüfling.** Die Diplomarbeit **konfiguriert +
triggert** (deklaratives XML-Profil); der CEB **führt aus**; CE/Prüfling werden **gemessen**. Seit Strang A (2026-06-18/19) ist das
REAL: die Mess-SELEKTION ist KEIN Code mehr, sondern ein `comdare_thesis_profile`-XML.

### 1.2 Die profil-getriebene Mess-Kette (Strang A — die Selektion)
`comdare_thesis_profile`-XML (z.B. `m3v2_study.profile.xml`)
→ `parse_thesis_profile` (xml_config_parser.cpp) → `ThesisProfile`-Struct
→ `build_axis_levels` (profile_to_tree.hpp) → `std::vector<AxisLevel>` (registry-getrieben, BR-1)
→ `ExperimentTree::build` → `StaticBinaryView` (B+-Baum, KF-9; Achsen-Austausch IM Präfixbaum, NIE Flach-Tupel)
→ `CEB::run_profile` (`tests/unit/thesis_tiere/profile_run_entry.hpp` `tlz::run_profile`) → `run_lazy_static_then_dynamic`
→ Prüf-Dock (`search_algorithm_dock.hpp`) → CSV. **Eintritt nutzerfreundlich:** `build_and_measure_150_tiere.ps1 -Profile <pfad>`
ODER `run_lazy_150 --validate <pfad>` (rein-lesende Profil-Prüfung VOR dem teuren Bau, #169).
**ENTFERNT (Strang A Inc 4):** `lazy_pilot_engine.hpp` (PilotAxes) + `m3v2_select_profile.hpp` (Code-Selektion) — `git rm`.

### 1.3 Das Build-Modell — `1 DLL = 1 TU` (KRITISCH, User-Korrektur 2026-06-19) — `docs/architecture/BUILD-MODELL-1DLL-1TU-KLARSTELLUNG.md`
Je `binary_id` (= serialisierter 19-Achsen-Pfad) wird GENAU EINE DLL aus GENAU EINER TU gebaut: ein `perm_<id>.cpp`, dessen Quelltext
nur `#include <builder/codegen/all_axes_umbrella.hpp>` (zieht alle 19 Achsen-Organ-Registry-Header) + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<19 FQ-Typen>)`
ist (adhoc_emitter.hpp:46-103); `cl /LD` → die DLL. **JEDE Lebewesen-TU kompiliert alle 19 Achsen-Organe neu** (keine geteilte
Achsen-Objektbibliothek). **KEINE inkrementelle „nur-die-neue-Achse"-Bau-Semantik** (`inkrementell_moeglich=false`). Einzige
Ersparnis = **Resume**: DLL-Skip bei identischer `build_version` (`.version`-Sidecar, build_orchestrator.hpp:150-159) + Mess-Skip
bei identischem `result.csv`-Stamp (#139). **„Ohne alles andere neu zu bauen" gilt NUR für die QUELLE** (man schreibt die 18 anderen
Achsen-Header nicht um), NICHT für den Bau. → der Framework-Nutzen ist **kompositionelle QUELL-Wiederverwendung + transparente
Per-Achse-/Ganz-Algorithmus-Messung**, NICHT reduzierter Build-Umfang.

### 1.4 Die Anatomie (Achsen = Organe) — der Kern-Beitrag
Ein **Lebewesen** (Such-Algorithmus) = `AdHocComposition<T0..T18>` = 19 Achsen-**Organe** (search_algo, cache_traversal, mapping,
path_compression, node_type, memory_layout, allocator, prefetch, concurrency, serialization, telemetry, value_handle, isa,
index_organization, io_dispatch, migration_policy, filter, queuing_q1, queuing_q2). Jede Achse = ein `TopicConfigSet` mit
`StaticAxisVariants` = `EnabledStrategies` (die wählbaren Algorithmen je Achse, registry-getrieben). Die **Permutations-Engine**
(`SearchAlgorithmPermutationEngine`) komponiert typ-getrieben; `CompositionFromPermTuple<P>` = die **Metaprog-Naht** (1 Permutation →
1 realer Algorithmus-Typ). Ein Original-Paper-Algorithmus = eine eindeutige Achsen-Komposition (Habich-Compliance: exakt
rekonstruierbar). Mess-Hybrid: Per-Achse (`seg_ns[19]`) UND ganzer Algorithmus (`total_ns`), korreliert; Vergleichsbasis = einheitliches
`std::map`/`std::vector`-Interface am Prüf-Dock.

### 1.5 Der Katalog + die 3 Mess-Subsets (ein Profil → alles) — `tests/unit/thesis_tiere/source_catalog.hpp` + `sota_catalog.hpp`
Der Katalog ist die **Materialisierungs-Domäne** (`binary_id → reale Modul-Quelle`), KEINE Selektion. `make_union_source_gen`
vereint 3 disjunkte Namensräume zu EINER `SourceGenFn`:
- **Basis-320** = `FullSourceCatalog = CatalogAxes<4,4,5,4>` (search_algo×node_type×memory_layout×prefetch; 15 Slots gepinnt).
  Harte Gate: binary_ids POSITIONS-IDENTISCH zu `golden_fullpilot_320_binary_ids.txt` (Resume #139-Schutz).
- **Achsen-Sweeps (FF #168)** = je vertiefte Achse ein kleiner `AxisSweepCatalog` (18 Slots Index-0, NUR die eine Achse voll) →
  `axis_sweep:<migration/filter/value_handle/path_compression>` baut REALE distinkte DLLs (z.B. migration=hot_cold ≠ none). KEIN
  Voll-Kartesisch (16 Sweep-Einträge gesamt → kein C1060).
- **SOTA-Reihen A/B/C (#162)** = `sota_catalog` über `pruefling_merge.hpp` (die **3 Kompositionalen Joins**: A=Stufe1_CeOnly,
  B=Stufe2_PrueflingReplace, C=Stufe3_FullJoin). 7 Lebewesen je Reihe: PRT-ART (`PrtArtComposition` = ART-Trie + Patricia-**R**edirect-
  Organ) + 6 SOTA (`KnownReferenceCompositions`: art/hot/masstree/surf/start/wormhole). PRT-ART-Slot deklariert `genus=SearchAlgorithm`
  (`assert_pruefling_slot_genus`; Cross-Genus-Join type-mathematisch unmöglich).
Der Treiber fährt im `run_profile` Multi-Pass: Basis-Pass + je Sweep-Achse ein eigener Sweep-Baum + je sota_series ein einwertiger
Baum — alle Zeilen in EINE CSV, getaggt `series;sweep_axis;working_set_n;platform;build_version`.

### 1.6 Mess-Validität (diese Session gehärtet, vor der teuren Messung)
Two-Phasen-Cache-Warmup PFLICHT · seg_ns attributiv (kommensurabler Nenner `seg_run_total_ns`, Coverage 98-99%, #161) · CLU aus
ECHTEM Footprint (5 reale Layout-Repräsentationen SoA/AoSoA/packed, #167) · prefetch reale `_mm_prefetch`-Adressen (#159) ·
PMAJOR-04 Release-zero-overhead über die ganze std::map-Schnittstelle (#166) · Working-Set-Sweep >LLC (#164). PMC/Cache-Misses
real = Linux+PMC bzw. Win-PCM (#152/#153) — noch offen.

---
## §2 ERLEDIGT diese Session (committet, je adversarial/G5-verifiziert)
**Strang A (profil-getrieben) Inc 1–6** `bc1f7a3..1e381fd` + **G5-Audit** `wl3i01xn4` (`strang_a_delivered=True`, keine
Reklassifikation). **Build-Modell-Korrektur** `dd16eb3`. **FF/#168** (4 vertiefte Achsen sweep-fähig) `e823a2b`. **#169** Erweiterungs-
Leitfaden `f45da60` + **`--validate`-Flag** `905d947`. Doku-Residuen `2bebc8b`. Abschluss-Doc `786ff3b`. (Plus die 4-Repo-Sicherung,
§0.3.) Tasks #166/#167/#168/#169 = completed.

---
## §3 OFFENE PLANUNG (Goal-Rest, priorisiert; Planrunde wjpcw0foi verfeinert die Reihenfolge)
- **#155 (gate-frei, Build-System):** vorbestehender CMake-Bug — `comdare_add_test` nutzt `gtest_discover_tests(DISCOVERY_MODE
  PRE_TEST)` (cmake/gtest_setup.cmake:60); das erzeugt `test_pressure_state[1]_include.cmake` mit `[1]`-Klammer, die
  `CTestTestfile.cmake`-`include()` als Glob-Char-Class fehlinterpretiert → **`ctest -N` bricht (Exit 8, 0 Tests, BESTÄTIGT)**.
  Fix-Kandidaten (brauchen Reconfigure-Test): DISCOVERY_MODE-Wechsel / Namens-Sanitisierung / CMake-Policy. + die ~15 neuen
  thesis_tiere-Standalone-PS-Tests in ctest registrieren.
- **#156 (freigegeben, GROSS):** der m3v2-Voll-Mess-Lauf via `CEB::run_profile` (jetzt baubar — die ganze Architektur ist fertig):
  320 + 21 SOTA × Two-Phasen × Working-Set. Mehrtägig. Cache-Misses=0 ohne PMC.
- **#152/#153 (freigegeben, lokal):** Win-PMC (`WindowsPcmPmcSource`, msr.sys+Admin) für reale Cache-Misses auf 1 Plattform.
- **G2-NAS (freigegeben):** M3-Roh-Matrix auf NAS (UNC + bash; lokal + git-LFS existiert).
- **G1+G3:** Phase-L-Auswertungs-Pipeline → bilinguale Abgabe-PDF (header-getrieben; verträgt die neuen Spalten seg_coverage/CLU/
  series/working_set_n + SOTA-/CLU-/Working-Set-/9-Achsen-Tabellen).
- **#163/#165 (ZIH = absprache-pflichtig!):** 2. Plattform (Barnard/Capella) + quiesziertes OS. **NICHT ohne Einzel-Freigabe.**
- **needs_user (nominal):** K1 (RC-Dimension), A5 (Second-Execution).

---
## §4 ERWEITERN DES SYSTEMS (für die Folge-Session + Forscher) — `docs/ERWEITERUNGS-LEITFADEN.md`
1. **Neuer Algorithmus in eine Achse:** Wrapper-Header nach Goldstandard → Achsen-Registry `AllStrategies`/`EnabledStrategies` →
   flags.hpp.in + CMakeLists → Profil-`<value>`. KEINE Umbrella/Katalog-Änderung (StaticAxisVariants=EnabledStrategies fließt automatisch).
   Bau-Folge: neue volle 19-Achsen-DLLs für alle Konfigs, die ihn nutzen.
2. **Neue Achse (20.):** 10 Goldstandard-Komponenten + `AdHocComposition` 19→20 + `adhoc_macro_args` + Umbrella + `CatalogAxes` +
   Observer-POD + **ABI-Major-Bump** (= ABI-brechend, ALLE DLLs neu).
3. **Neues Profil-Feld:** `ThesisProfile`-Struct + `parse_thesis_profile` + `build_axis_levels` + `run_profile`-Konsum + SCHEMA.md
   (Präzedenz: Strang-A-Inc2 = working_set_sweep/axis_sweep/sota_series/run_options). Selektions-Felder ändern die binary_id NICHT.

---
## §5 NÄCHSTE SCHRITTE (autonom, geordnet)
1. **Planrunde-Result abholen** (`wjpcw0foi`) → eindeutigen nächsten Punkt umsetzen (+ ZIH-Schutz-Grenze beachten).
2. Wahrscheinliche Folge (Planrunde bestätigt): #155-CMake-Fix (gate-frei, G5-relevant) → Win-PMC #153 → m3v2-Voll-Lauf #156 →
   G2-NAS → G1/G3-PDF auf m3v2-Daten → finaler G5-Gesamt-Audit. ZIH (#163) erst nach Einzel-Absprache.

---
## §6 BINDENDE DISZIPLINEN
Two-Phasen-Warmup PFLICHT · Achsen-Austausch im B+-Baum (nie Flach-Tupel) · für Suche immer Bäume · benannte Lehrbuch-Patterns +
zero-cost-Metaprog · **keine Erfolgsmarke ohne literale Tool-Ausgabe** · je Einheit Commit+Push+3-Repo-Submodul-Sync · destruktive
Ops nur in den 3 Thesis-Repos mit Tag · bei Unklarheit Planrunde (Multi-Agent) VOR Umsetzung · **adversarial verifizieren + eigenes
Commit-Gate** (der Verifier irrt manchmal — CLU-Phantom; selbst grep/build/literal). ZIH/PMC/Cluster = Absprache.

## §7 SINGLE-SOURCE-DOCS (Lese-Reihenfolge)
Doc 34 (IST) → diese Übergabe → `20260619-ABSCHLUSS-STRANG-A-FF-LEITFADEN.md` → `20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md`
→ `BUILD-MODELL-1DLL-1TU-KLARSTELLUNG.md` → `ERWEITERUNGS-LEITFADEN.md` → `20260618-M3v2-NEUMESSUNG-DESIGN-SPEC.md` →
`20260618-OFFENE-TODOS-LEDGER.md` → `20260618-WORKFLOW-LEDGER.md` → Goal `GOAL-AUTONOM-ABARBEITUNG-20260613.md`.
