# Architektur-Ziele- / Offene-Punkte-Ledger (Single-Source-of-Truth)

> **Zweck (/goal V2, User 2026-05-30):** Die EINE autoritative, IST-verifizierte Liste aller Architektur-Ziele
> + offenen Punkte. Garantiert Workflow-Agenten-bestätigt (Audit-Workflow wqzpyh5ks, 5 Agenten, 2026-05-30):
> kein Punkt "erledigt" ohne literale Code-/Test-Evidenz; extern-blockierte, user-manuelle und V42-Punkte
> GETRENNT (NICHT als erledigt gezählt). /goal erst erfüllt, wenn KEIN offener actionable nicht-blockierter
> Punkt mehr existiert UND ein finaler Audit das bestätigt.

## Nutzungs-Disziplin (jede Charge)
1. Nächsten Punkt aus (a) wählen (Prio: Korrektheit > Architektur-Pflicht > Refinement).
2. Bearbeiten + verifizieren (literale Build-/Test-Evidenz, [[feedback_no_success_marks_without_literal_output]]).
3. Eintrag → done-verified mit Commit-Ref + Test-Evidenz (nach (e) verschieben).
4. Neu entdeckte Punkte ergänzen.
5. Bei Strom-Abschluss: Audit-Workflow erneut fahren ([[feedback_verify_ist_state_before_gross_tasks]]).

**Autoritative Quellen:** TaskList · Open-TODOs-Master docs/sessions/20260524-V41-open-todos.md (Superprojekt)
· R7.x #717-736 · Doku 11/14/22/24 · jüngste Session-Handoffs.

**Stand:** 2026-05-30 · **Repo:** comdare-cache-engine HEAD `59d6e87` · **Synthese aus 4 Audit-Streams (dedupliziert)**
**Verifikation diese Session:** observable_tier.hpp / pruef_dock/* / axis_centric_namespaces.hpp existieren; allocator-Wrapper=28; axis_03a=77 Header; 22 axis_*-Dirs; 0 TODO/FIXME im Topic-Hauptcode; R6-Commit-Kette `5b72eae..db4de2e` real; F.2 = reine Alias-Fassade (keine phys. axes/-Subdirs).

---

## (a) OFFEN · ACTIONABLE · NICHT-BLOCKIERT — nach Priorität (Korrektheit > Architektur-Pflicht > Refinement)

| ID | Titel | Kat. | Status (verif.) | Evidenz | Referenzen | Nächster Schritt |
|----|-------|------|-----------------|---------|------------|------------------|
| **R8** (=prt-art-Prüfling / Task #8,#31) | prt-art-Prüfling einbinden: **CMake + Metaprogrammierung-Join** (KEINE Header-Migration!) — `optional_prt_art_impl`-Slot-Befüllung (Compile-Time-Join) + 3-Join-Muster-Binaries via Prüf-Dock messen | large-multi-session | **open** (Phase A ✅ erledigt; Phase B Slot-Join offen) | **REFRAMING (User 2026-05-30, Doku 24 §8.9):** PRT-ART = abstrakte Tier-Permutation, liefert neue Achsen-Algos, die per C++23-Metaprog. zur Compile-Zeit mit jeder CE-Achse joinen; gemessen auf 3 Join-Mustern (Stufe1 ce-only + Stufe2 pruefling-replace = 2 getrennt; Stufe3 full-join) über das Gattungs-Prüf-Dock. **Planrunde `wrvvcf8ed` IST-verifiziert (HEAD `60b1369`): Phase-A-Migration = 0 offen** (alle additiven basis_missing bereits im CE: status_code, array65535, olc_reserved_blocks, chain_ref, bplus/redirect, telemetry-tracker etc. — `7405524`/`fc038b3`/2026-05-29). 17 `optional_prt_art_impl`-Slots deklariert (alle leerer Body, axis_centric_namespaces.hpp Z.152-168); `pruefling_merge.hpp` 3-Stufen-Pattern + 2 Tests vorhanden. **R8-Rest:** ~28 Phase-B-Slot-Joins + 14 Phase-C-Löschungen (gekoppelt an F.2/#12, erst NACH Phase B) + 1 opt. Utility (cache_line_aligned_layout, kein CE-Ziel) | Doku 24 §8.9; Doku 19/21; pruefling_merge.hpp; axis_centric_namespaces.hpp Z.152-168; Code/external/comdare-prt-art | **CE-Foundation ✅ VERIFIZIERT (94f9a17):** test_v41_anatomy_pruefling_merge 13/13 + test_v41_pruefling_registry 3/3 gr; 3-Stufen-Pattern + Registry + Stufe-3-`comdare_perms_full_join`-DLL-Codegen (tests/CMakeLists Z.946) aktiv. Die 17 `optional_prt_art_impl`-Slots sind CE-seitige LEERE Namespaces (Merge-Deklarationspunkte). **Phase-B-Slot-Füllung ist prt-art-SEITIG** (Konsument re-öffnet CE-Namespace + injiziert Variants → `StufeTwoAxis` joint compile-time) — Cross-Repo, nächste R8-Charge im prt-art-Repo. CE-Seite R8 = Foundation-komplett. NICHT Phase-C zuerst (Build-Bruch) |
| **R7.1.b–d** (Task #27) | Goldstandard-Restachsen: ~~axis_08-Concurrency-Test~~ ✅ + fehlende is_original-Header (axis_03a ART/HOT/START) | large-multi-session | **in-progress** (axis_08-Test ✅; is_original-Header offen) | **axis_08-Test ✅ VERIFIZIERT VOLLSTÄNDIG (`02cf85d`):** 9 Wrapper (nicht 8), olc existiert, statische Policy-Typen ohne Laufzeit-API → Concept/Pattern/FlagSuffix/Subaxes/Registry/FamilyId/TopicConfigSet alle 9 + CacheEnginePermutationStrategy 5→9 vervollständigt, 8/8 gr (Audit-Claim „keine Coverage" widerlegt). **OFFEN:** axis_03a-Original-Codes (ART/HOT/START) ohne is_original-Header (6 Allocator-Header existieren) | `02cf85d`; tests/unit/test_v41_axis_08_concurrency.cpp; build/.../legacy_code/paper_a*_is_original.hpp | is_original-Header-Codegen für axis_03a-Original-Codes + Introspection-Tests |
| **R7.4-Restcharge** (Task #7) | Allocator: verbleibende Vendor-Wrapper-Integrationstests | actionable | **in-progress** | 27/28 Wrapper real implementiert (kein Stub, korrekte portable_aligned_alloc-Fallbacks), 23 Vendor-Includes real. Task #7 in_progress (Body fertig, Integrationstests offen) | topics/allocator/axis_06_allocator/; PAPER_REFERENCES.md | Integrationstests verbleibender Vendor-Wrapper (R7.4-Phase-2) |
| **R7.2-Restcharge** (Task #5) | Traversal-Vollausbau Abschluss (Verflechtung mit Naming/F.6-B prüfen) | large-multi-session | **in-progress** | 77 axis_03a-Header real; Body weitgehend fertig (s. „done"-Sektion). Task #5 in_progress — Restscope unklar | Task #5; Memory R7.2/R7.3/R7.4-Verband | Verifizieren ob für F.6-Phase-B erforderlich (Dichte-Dispatch/OLC); ggf. Design-Phase |
| **OpenDone.0 / R6-Folge** | Echter .dll-Round-Trip: Permutations-DLLs mit neuem SearchAlgorithmAbiAdapter (IObservableTier) NEU bauen + Host-Test über AnatomyModuleLoader | large-multi-session | **open** | drive_tier_observe_trace_abi auf IObservableTier generalisiert; in-process getestet. adhoc_emitter-DLLs noch NICHT neu gebaut | search_algorithm_dock.hpp:42 (dynamic_cast); apps/adhoc_emitter; Doku 24 §8.8 | DLL-Rebuild mit neuer abi_adapter.hpp, dann Host-Loader-Test |
| **OpenDone.2** | ExperimentDriver Phase5 auf anatomy_module_loader + Prüf-Dock verdrahten (zwei Loader-Welten zusammenführen) | large-multi-session | **open** (bewusst entkoppelt) | experiment_driver.hpp:100 ruft alten module_loader (Pfad A). measure_genus_sequential existiert, aber NICHT in phase5 verdrahtet (Doku 24 §8.8:536 „bewusst, Blueprint-Risiko") | experiment_driver.hpp; module_loader vs anatomy_module_loader | (A) ExperimentDriver migrieren ODER (B) Standalone-Builder-CLI mit measure_genus_sequential |
| **F.2** (Task #12) | Axen-zentrische Namespace-Restrukturierung: physischer Rename pro Achse | large-multi-session | **in-progress** | Alias-Fassade ERLEDIGT (verifiziert: axis_centric_namespaces.hpp = reine Aliase, 55 namespace-Zeilen, KEINE phys. axes/-Subdirs). F2F3_AxisCentricFacade-Test grün. Phys. Rename pending (~10 Edits+30 Refs/Achse) | Doku 23 §1-4; test_v41_f2f3.cpp | Inkrementeller Rename je Achse (rückwärts-Alias-stabil), Start mit lookup/axis_03a |
| **R5.D** (Task #26) | PMC-Hardware-Counter + Mess-Treiber (Wall-Clock-Korrelation Perfmon-ABI) | actionable | **open** | Doku 22 §3.3 nennt PMC-Limit. R6-Pfad-B geschlossen, aber PMC NICHT explizit implementiert. Möglich Teil von R6 oder separate Charge | Doku 22 §3.3; Session 20260530-hybrid; commit 185f038 | Verifizieren ob Teil R6 oder separat; falls separat: Perfmon-ABI-Adapter Design+Impl |
| **B2** (Task #21) | YCSB-Workloads A/B/C/D/E echt (Profile-Implementierung) | actionable | **in-progress** | Infra ok, 5 Tests; Profile-Implementierung TODO | libs/test_infra/workload_generator | Profile A–E implementieren |
| **B5** (Task #21) | Plugin-Discovery via COMDARE_PERM_ROOT (env-var-Check) | actionable | **in-progress** | Kein env-var-Check vorhanden | Task #21 | env-var-Check hinzufügen |
| **A3+A4** (Task #20) | SIMD-Achse (AVX512+NEON Host-Filter) + Layout-Achse (AoSoA, packed structs) | actionable | **in-progress** | axis_09b_simd_extension + axis_05_memory_layout existieren; Vollausbau in_progress | Task #20 | Implementierung vervollständigen |
| **E1** | gtest-Config-Quick-Win | actionable | **open** | Quick-win | TODO-Master | Config |
| **E2** | Cache-Quick-Win | actionable | **open** | Quick-win | TODO-Master | Add |
| **E3** | README-Quick-Win | actionable | **open** | Quick-win | TODO-Master | Doc |
| **E7** | MEMORY-Konsolidierung | actionable | **open** | Quick-win | TODO-Master | Consolidate |

---

## (b) EXTERN-BLOCKIERT / TERMIN-ABHÄNGIG

| ID | Titel | Kat. | Status | Evidenz | Nächster Schritt |
|----|-------|------|--------|---------|------------------|
| **A1** | jemalloc echt linken | blocked-external | open | Fallback std; User-Entscheidung nötig | User decision |
| **A2.1** | Allocator-Voll-Linking (jemalloc/tcmalloc/hoard/scalloc, Voll-Präzision) | blocked-external | open | 2 real / 5 fallback; hängt an A1 | A1, dann Vendor |
| **B4.1** | MinGW-Build | blocked-external | open | ESET blockiert Binaries | User: ESET-Ausnahme |
| **C1** | Cluster-Tasks | blocked-external | open | Extern, Phase 7+ | Termin |
| **C2** | Grace Hopper | blocked-external | open | Extern, kritisch | User-Absprache |
| **F.4** (Task #14) | Tools-Plugin-Concept ICacheEngineTools Facade | blocked-external (Design/User-Decision) | open | Memory erwähnt; kein Impl-Pointer; Scope unklar (Refinement F.2/F.3 vs. Standalone?) | Scope-Klärung mit User (R8-Kontext) |
| **E10** (Task #17) | STATIC/SHARED pro-Projekt/-Untermodul-Achse | blocked-external (Design/User-Decision) | open | Task pending; kein Code-Pointer; CMake-Achse vs. Konzept unklar | User-Klärung (Abhängigkeit E11?) |
| **E11** (Task #16) | Master-Framework Facade + AbstractFactory-Prüfling-Slot | blocked-external (Design/User-Decision) | open | Task pending; kein Design-Doc | Design-Phase (analog R5.A ExecutionEngine) |
| **Doku-11-Verif.** | TopicConfigSet/PermutationEngine-Doku lokalisieren | blocked-external | open | Doku 11 nicht im Repo (nur 17–24); Konzept im Code verankert | Termin-7-Verzeichnis konsultieren |
| **Doku-14-Verif.** | Anatomie/Gattungen-Doku lokalisieren | blocked-external | open | Doku 14 nicht im Repo; Code unter anatomy/ (26 Header) implementiert | Termin-7-Quellen / Glossar ergänzen |

---

## (c) USER-MANUELL

| ID | Titel | Kat. | Status | Evidenz | Nächster Schritt |
|----|-------|------|--------|---------|------------------|
| **D1** (Task #25) | Diplomarbeit-Kapitel-Text | user-manual | open | User schreibt Volltext | User |
| **D2** (Task #25) | Bausteine-Matrix-Doku / Matrix-Update | user-manual | open | User aktualisiert | User |

---

## (d) V42-FUTURE / OPTIONALE ROADMAP

| ID | Titel | Kat. | Status | Evidenz | Nächster Schritt |
|----|-------|------|--------|---------|------------------|
| **OpenDone.1** | V42 Gattungs-Docks: Set/Sequence/Adapter/View | v42-future | open | SearchAlgorithmDock = Template; Gattungs-Anatomien existieren noch nicht (Doku 24 §8.8:539) | Pro Gattung: IPruefDock-Subklasse + Sub-Interface (Blueprint: SearchAlgorithmDock) |
| **R7.6.c** (Task #4) | Echtes is_original-Linking lizenzierter Klasse-A-Codes | v42-future (optional) | open | Als optional klassifiziert (Doku 17 §1) | Nach Termin-Deadline |
| **Naming-Refactor-Backlog** | axis_12/04/03a/q1q2/08 Naming-Harmonisierung | v42-future | open | Memory-Direktive; Inventar teils geklärt (axis_12=telemetry, q1/q2 bewusst ungefiltert) | User-Entscheidung Reihenfolge |
| **E9** | raw-string-Workaround ablösen | v42-future | open | Stabiler Workaround vorhanden | Optional |
| **Task #10** | V42 + Infrastruktur (#648-653, #613, #619, #621, #622) | v42-future | open | Niedrige Prio | Nach Pflicht-Pfad |
| **Task #22 (E4.1+E6)** | 6 cache-engine-Submodule-Repos befüllen + nested cleanup | v42-future | open | Pending | Nach Pflicht-Pfad |

---

## (e) VERIFIZIERT ERLEDIGT (Code-/Test-/Commit-Evidenz vorhanden)

| ID | Titel | Evidenz (verifiziert) |
|----|-------|------------------------|
| **R6.1** | IObservableTier + ComdareTierObserverSnapshotV1 POD | observable_tier.hpp (Datei verifiziert vorhanden); static_assert standard_layout+trivially_copyable; commit 5b72eae; Tests test_v41_pruef_dock + test_v41_anatomy_module_abi |
| **R6.2** | drive_tier_observe_trace_abi (Wall-Clock+Observer korreliert) | tier_observe_trace_abi.hpp; commits 4b68b13+146e6b2; test_v41_tier_observe_trace |
| **R6.3** | Loader-Entkopplung anatomy_module_abi_v1_decl.hpp | commit 6140705 (verifiziert in git log); kein C1083 mehr |
| **R6.4 / R6-Pfad-B-2D** | Allocator-Achse im Cross-ABI-POD (2-dimensional) | abi_adapter.hpp; observable_composed_search.hpp:84-93; commit db4de2e (verifiziert); JSON alloc_bytes_allocated>0 |
| **R6.5 / Pfad-B-Schleife** | Prüf-Dock-System (IPruefDock+SearchAlgorithmDock+Registry+Sequenzierer) | pruef_dock/*.hpp (5 Dateien verifiziert vorhanden); commits 5a6820e+59d6e87 (HEAD); test_v41_pruef_dock_search_algorithm |
| **ErledDone.0** | observe_all() real in SearchAlgorithmAbiAdapter | abi_adapter.hpp; commit 7c6d670/c8f26e8; test_v41_anatomy_observer |
| **Säule1.0** | Komponierbare Traversal-Organe + ComposedSearch (Organ-Swappability) | composable_search.hpp; test_v41_saeule1_composable_organ (297/297, Keys >65535) |
| **R7.2** | Traversal-Vollausbau (axis_03a, Body) | 77 Header verifiziert; ART/HOT/START/Wormhole/SuRF/Eytzinger; 0 TODO/FIXME verifiziert |
| **R7.3** | Queuing+Concurrency-Vollausbau | axis_q1 (12-14 Wrapper) + axis_q2 (5); Task #6 completed; 0 TODO/FIXME |
| **R7.4-Body** | Allocator 28 Wrapper real + resource_ownership() | 28 Wrapper verifiziert; commit 185f038 (ResourceOwnership-Enum); 473/473 Regression |
| **22.Achsen** | 22 axis_*-Dirs vollständig | 22 axis_*-Dirs verifiziert per ls |
| **Zero.TODO** | 0 TODO/FIXME im Topic-Hauptcode | grep verifiziert: 0 Treffer (excl legacy/build/paper) |
| **F.3** (Task #13) | 17 Achsen-Concepts abstrahiert | axis_centric_namespaces.hpp 55 namespace-Zeilen verifiziert; F2F3-Test grün; perm-engine 21/21 |
| **U1** | Umstufung (Monolith-frei) | Tier→Organ; verifiziert |
| **Umstufung-A** (Task #41) | Alle Tiere seziert (Hash/SkipList/B-Baum/Original) | commits 176c60b/ccdbbde/f9de770; Masstree 0 Bugs |
| **Umstufung-B** (Task #42) | Tiere aus axis_03a raus → Gattungs-Konfiguratoren | commits e31541a+8bcbd7e; EnabledStrategies 17→4; 264 Tests grün |
| **s4** (Task #43) | OriginalXxx Trie-Anatomie seziert (ART/HOT/START/SuRF/Wormhole) | ByteWiseKeyPrefix-Organ 18/18; SuRF=Filter-Gattung, Wormhole=GPL-3 markiert |
| **Cross-Constraints** (Task #9, R5.C.3 #704) | ISA×SIMD + ISA×Plattform | commits f9b6303+d29fdef; 12 Paare→6 möglich (Beweis) |
| **G.1** (Task #18) | Hierarchische Achsen-Iteration im Build-Output | completed |
| **Pflicht-vs-Roadmap** | Architektur-Klassifikation Pflicht (R7.1-6/F.1-6) vs. Optional (V42/R7.6.c) | Doku 17 §1; Task #4/#10 optional, #5-9 Pflicht |

**Dedup-Notizen:** F.2/F.3 in Stream-2 (kompakt) + Stream-4 (detailliert) → zusammengeführt (F.3 done, F.2 in-progress). R7.2/R7.3/R7.4 in Stream-3+4 → Body-done in (e), Restchargen in (a). R6-Punkte (R6.1-6.5/ErledDone.0/Säule1.0) aus Stream-1 mit Stream-4 (R6-Pfad-B/R7.4-resource_ownership) abgeglichen — identische Commits, eine Zeile je Befund. F.6: Stream-3 (R8.prt-art) = Stream-4 (F.6-Phase-B+C) = Tasks #8/#31 → EIN Punkt R8. „done"-Items s1-s8/ErledDone aus Stream-2 in Umstufung-A/B/s4 (Stream-4) aufgegangen.
