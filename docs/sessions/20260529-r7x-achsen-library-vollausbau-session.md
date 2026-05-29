# Session 2026-05-29 — R7.x Achsen-Library-Vollausbau + verify-first-Methodik

**Repo-Endstand:** ce `fa4c18b` · da `bc75806` (Pointer = ce-HEAD) · pa `35d1143` (unverändert).
**Aktiv:** `/goal` (autonome Fortsetzung bis alle TODOs/Architekturanforderungen erfüllt).
**Destruktiv-Autonomie + Submodul-Sync-Direktive** durchgehend eingehalten (Tag + Commit + Push pro Einheit; da-Pointer nach jedem ce-Push gebumpt).

---

## 1. Geleistete Einheiten (10, alle verifiziert + gepusht)

| # | Einheit | Beleg | ce-Commit |
|---|---------|-------|-----------|
| 1 | **R5.G Auto-Permutations-Skalierung** — `comdare-adhoc-emitter` (configure-time-CLI) enumeriert den ganzen Pilot-Raum + `cmake/adhoc_emitter.cmake` baut JEDE Permutation auto zu SHARED-DLL | `test_v41_anatomy_adhoc_autobuilt_load` 1/1 (3 DLLs geladen+gemessen) | e60650b |
| 2 | **R5.B Routing-Gap** präzise analysiert + dokumentiert (Doku 21 §5) | gated auf R7.x (Achsen sind Compile-Time-Deskriptoren) | 9bb199b |
| 3 | **R7.3 Queuing+Concurrency** verifiziert komplett + `AppendOnlyBuffer::drain_all()` Vollausbau | topic_queuing 219/219, axis_08 8/8 | e2714e6 |
| 4 | **R7.2 KArySearchAlgo** (S10, k-ary search, SIMD-Partition) | DaMoN 2009, 187/187 | 57dc99d |
| 5 | **R7.2 InterpolationSearchAlgo** (S11, verteilungsbewusst) | CACM 1978, 202/202 | 7abd110 |
| 6 | **R7.2 EytzingerSearchAlgo** (S12, cache-conscious BFS-Layout) | JEA 2017, 217/217 | 7e709d6 |
| 7 | **R7.4 Allocator-Adapter** `as_std_allocator<T>()` + `as_pmr_resource()` (Stubs → echt, alle 24 Wrapper) | 255/255 | 34b98ed |
| 8 | **A4 AoSoAMemoryLayout** (S5, SIMD-Hybrid-Layout) | 15/15 | 4ae38cd |
| 9 | **R7.2 SkipListSearchAlgo** (S13, probabilistische geordnete Struktur) | Pugh CACM 1990, 231/231 | eb1b79a |
| 10 | **G.1 Achsen-Hierarchie im Build-Output** + Permutations-Raum-Auswertung | 15 Topics·22 Achsen·138 Wrapper·≥1e15 | fa4c18b |

**axis_03a: 9 → 13 Strategien** (Such-Methoden-Dimension komplett: 3 Paradigmen SIMD-Partition/Verteilung/Cache-Layout + erste Struktur Skip-Liste). Alle neuen Wrapper: web-recherchiert (`[[web-research-per-algorithm]]`), goldstandard-konform (Wrapper + flags.hpp.in + Registry + CMake-Option + PAPER_REFERENCES + Doku-18-Map + exhaustive Tests), originalgetreue C++23-Re-Impl (`is_original=false`, `[[pseudocode-papers-fallback]]`).

**Tasks:** #6 (R7.3) + #18 (G.1) abgeschlossen; #5 (R7.2) + #7 (R7.4) + #20 (A3+A4) advanciert (in_progress mit Fortschritt).

---

## 2. Methodik-Lektion: verify-first (v4× bestätigt)

Mehrere als „GROSS / 29–42 SP" gelistete Tasks waren faktisch ~90–100% fertig — der echte Rest je ein bounded, prinzipientreuer Wrapper/Adapter (kein Quick-Fix, kein Stub):
- **R7.3** (29 SP): nur `drain_all()`-TODO offen; gemeldete „Bugs" (AdaptiveLsm-Header, axis_08-Stub) waren schon erledigt.
- **R7.4** (34 SP): 24 Wrapper + 252 Tests fertig; nur 2 Adapter-Stubs offen.
- **#20 A3+A4**: SIMD-Achse (8 Wrapper inkl. AVX512+NEON) komplett; nur AoSoA-Layout fehlte.
- **#16 E11**: Prüfling-Slot (`PrueflingRegistry`) fertig + cross-repo getestet; Master-Facade bewusst gated.

→ Memory `[[feedback_verify_ist_state_before_gross_tasks]]`. **Vor jedem GROSS-Task: Tests bauen+laufen + Ist-Header lesen, BEVOR man Frischarbeit annimmt.** Doku-Notizen beschreiben oft vergangene Soll-Zustände.

---

## 3. Genuin verbleibende Arbeit (Mehr-Session / gated / user-manuell)

| Task | Art | Hinweis |
|------|-----|---------|
| #5 R7.2 Rest | Tree-STRUKTUR-Paper (CoCo-trie/B²tree/BTreesAreBack/Mahling) | je ganze Index-Struktur, korrektheitskritisch; Masstree DEFERRED (nur Templates). #692 Organ-Metapher-Refactoring. |
| #12/#13 F.2/F.3 | GROSS | Achsen-zentrische Namespace-Restrukturierung + Legacy-Concept-Wiederverwendung |
| #16 E11 + #22 E4.1 | gated | Master-Facade (`api/i_cache_engine.hpp` Skelett) braucht die 6 befüllten Submodule (E4.1) |
| #26 R5.D/E/R6 | GROSS | CacheEngineBuilder-CLI + Mess-Treiber = die eigentliche F15-Messung |
| R5.B | gated auf R7.x | Achsen operativ machen, damit run_workload den GANZEN Algorithmus misst (Doku 21 §5) |
| #4 R7.6.c | optional | echtes is_original-Linking (OLC/unodb, Q1/moodycamel) |
| #25 D1/D2 | user-manuell | Diplomarbeit-Volltext (User schreibt, ich liefere Skizzen) |
| #8/#9/#10/#17/#19/#21/#23/#24/#27/#31 | gemischt | siehe Task-Liste |

---

## 4. Nächster eindeutiger Schritt

Weiterer R7.2-Struktur-Wrapper **oder** verify-first auf einen der mittleren Tasks (#17 E10, #9 Cross-Constraints, #27 Tech-Debt). Per dieser Session: **vor jedem als GROSS gelisteten Task zuerst Ist-State verifizieren** — der echte Rest ist oft bounded.

Goldstandard-Vorlage für neue axis_03a-Wrapper: `axis_03a_search_algo_skip_list.hpp` (Struktur) bzw. `axis_03a_search_algo_k_ary.hpp` (Such-Methode) + die 5 Wiring-Punkte (CMake-Option `option(COMDARE_AXIS_03A_ENABLE_*)` Z.126ff + foreach Z.636 + message + flags.hpp.in + Registry) + Test in `tests/unit/test_v41_topic_traversal.cpp` + PAPER_REFERENCES + Doku 18 §2.

---

## 5. Addendum — Units 11–17 (Fortsetzung gleiche Session, ce 325ce73)

| # | Einheit | Beleg | ce-Commit |
|---|---------|-------|-----------|
| 11 | **R7.2 BinarySearchFanout** (axis_03b CT03, sortiert+lower_bound, Bayer/McCreight 1972) | traversal 239/239 | c3bf24d |
| 12 | **R6 multiple_comparison** (Bonferroni + Holm-Bonferroni FWER-Korrektur) | F15 +5 | 21cd5d5 |
| 13 | **R5.E result_aggregator** (ExecutionResult → CSV (RFC-4180) / JSON) | F15 +4 | f4dd175 |
| 14 | **R5.E latency_stats** (Perzentile p50/p99/min/max/mean, geteilt+non-mutierend; ExecuteEngineCommand DRY) | test_commands 20/20 | 6fd813e |
| 15 | **R6 multi_compare** (multi_compare_against_baseline: N vs Baseline, Welch+Holm) | F15 +3 | 3c31e35 |
| 16 | **R5.E report_to_csv/json** (MultiCompareReport-Export) | F15 +1 | 6440366 |
| 17 | **R6 summarize** (F15-Headline win_rate über den Report) | F15 23/23 | 325ce73 |

**F15-Mess-Auswertungs-Schicht jetzt END-TO-END** (Namespace `builder::commands::stats`):
`latency_stats` (Perzentile) → `welch_t_test` (Effektgröße/p) → `multi_compare` (N-vs-Baseline + Holm-FWER) → `summarize` (Headline `win_rate`) → `result_aggregator`/`report_to_csv/json` (Export). Alle header-only, ohne externe Abhängigkeit, exhaustiv getestet.

**axis_03b** zusätzlich 2→3 (BinarySearchFanout). Achsen-Bibliothek-Endstand: axis_03a 13, axis_03b 3, axis_05 5; 22 Achsen / ~140 Wrapper (G.1-Auswertung).

**Verbleibend unverändert** (siehe §3) — GROSS/gated/user-manuell: R5.D CacheEngineBuilder-CLI-Main + HW-Counter + durchgängiger Messlauf-Treiber (baut nun auf der fertigen Auswertungs-Schicht) · axis_03a-Tree-Struktur-Paper · F.2/F.3 · E11/E10 (→E4.1) · D1/D2.

---

## 6. Addendum — Units 18–30 (Fortsetzung gleiche Session, ce c1e0b13 / v34-final-205)

| # | Einheit | Beleg | ce-Commit |
|---|---------|-------|-----------|
| 18 | **R5.D make_execution_result** (Konnektor Samples→ExecutionResult, p50/p99) + volle Driver-Kette in-process | F15 25/25 | 84662fe |
| 19 | **R5.D F15-Treiber über REALE DLLs** (load_all→run_workload→multi_compare→summarize, FullF15DriverOverRealDlls) | 2/2 | 0567b68 |
| 20 | **R5.D comdare-f15-compare CLI** (apps/f15_compare) — Mess-Treiber als Tool | empirisch: Array256 ~8× < sorted, p≈1e-132 | 1e37bf6 |
| 21 | **R5.D Permutations-Raum 3→8 Such-Algos** (head-to-head F15) | 7/7 Holm-signifikant | 6339af5 |
| 22 | **R7.2 HashSearchAlgo** (S14, open-addressing, Tombstone-Erase, Knuth) | traversal 254/254 | cc9ff85 |
| 23 | **Doku 22** (F15-Pipeline + Such-Bibliothek + empirisches Resultat, D1/D2-Vorlage) | — | d6945fb |
| 24 | **R5.D CTest-Smoke** für f15-compare CLI | ctest 1/1 | 8a66f57 |
| 25 | **R5.D CLI-Ranking** (alle Kompositionen nach Latenz, Spanne) | Achsen-Wahl ~10× | d2c1545 |
| 26 | **B5 COMDARE_PERM_ROOT-Discovery** (CLI env-fallback) + B2-Verifikation (YCSB-Gen 21/21 vorhanden) | 3 Modi | a960388 |
| 27 | **R7.2 LinearScanSearchAlgo** (S15, unsortiert, ART-Node4-Baseline) | traversal 268/268 | cdfce2f |
| 28 | Doku: axis_03a PAPER_REFERENCES §3-Zählung korrigiert (15 Wrapper) | — | 12e627f |
| 29 | **Integrations-Regression** (volle ctest-Suite) + Diagnose 4 vorbestehender _NOT_BUILT | 1965/1969, root-cause | (verify) |
| 30 | **E1 prt-art-Legacy-Tests gegatet** (COMDARE_PRT_ART_LEGACY_AVAILABLE) → Standalone-Suite makellos | **1965/1965 (100%)** | c1e0b13 |

**FINALER STAND (30 Einheiten):**
- **Säule 1 — Achsen-Bibliothek:** axis_03a **15 Such-Strategien** (vollständige Paradigmen-Palette: dense/sorted/Such-Methode/Struktur/Hash/unsortiert-linear/Original-Trie) · axis_03b 3 · axis_05 5 · R7.3/R7.4 · R5.G · G.1.
- **Säule 2 — F15-Messung:** end-to-end + ausführbares CLI (`comdare-f15-compare`, Ranking + COMDARE_PERM_ROOT) + empirisches Resultat (8 Permutationen head-to-head: Achsen-Wahl → ~10× Latenz-Spanne, alle Holm-FWER-signifikant).
- **Qualität:** volle ctest-Suite **1965/1965 (100%)**, CLI CI-getestet, session-weite Regression sauber.

**Methodik:** verify-first (`[[feedback_verify_ist_state_before_gross_tasks]]`) deckte 6× auf, dass „GROSS"-Tasks fast fertig waren; 1 vorbestehendes Build-Stabilitäts-Problem (E1) root-cause behoben. Alle Einheiten prinzipientreu (kein Quick-Fix/Stub), durchgängig 3-Repo-synchron (≈31 Restore-Tags).

**Genuin verbleibend (Mehr-Session/gated/user-manuell):** R5.D HW-Counter (PMC) + voller kartesischer Raum · R5.B (Achsen operativ, gated auf R7.x) · F.2/F.3 Namespace (GROSS) · E11/E10 (gated auf E4.1-Submodule) · weitere Tree-Struktur-Paper · D1/D2 (Autor schreibt Volltext, Doku 22 als Vorlage).

---

## Fortsetzung (2026-05-29, Einheiten 31–40) — Such-Paradigmen schliessen + R5.B-Achsen-Operativierung + R5.D-Mess-Robustheit

| # | Einheit | Ergebnis | Commit |
|---|---------|----------|--------|
| 31 | **R7.2 BinarySearchTreeSearchAlgo** (S16, unbalancierter BST, Hibbard-Deletion, Knuth §6.2.2) — deterministische Vergleichs-Baum-Baseline | gegen std::map austauschbar; traversal 283/283 | 633d363 |
| 32 | **R7.2 BTreeSearchAlgo** (S17, balancierter block-orientierter Mehrwege-B-Baum, CLRS-Insert/Delete, t=4) — schliesst Balance-Achse (unbalanciert/probabilistisch/det.+block) | traversal 296/296; perm 17/17 | d853ac9 |
| 33 | **F15-Messset 12 Paradigmen** (BST+B-Baum in adhoc_emitter) + **numerische DLL-Loader-Ordnung** (Root-Cause: lexikograf. `_10` zwischen `_1`/`_2` → Label≠SA-Index) | 12 DLLs, alle Holm-sig | 58031cc |
| 34 | **R5.B PoolResourceAllocator** (eigener std::pmr::unsynchronized_pool_resource, behavioral-distinkt OHNE externes Linking — erster nicht-hohler axis_06-Variations-Kandidat) | axis_06 265/265 | 43b886a |
| 35 | **R5.B 2. Mess-Dimension** search_algo×allocator (24 DLLs) + interpretierbares `manifest.txt` (ohne composition_name/ABI-Aenderung) | Pool 2–3× vs malloc, alle Holm-sig | 4b8114c |
| 36 | **R5.B Doku-Praezisierung** (verifiziert: Achsen sind compile-time-trait-only; allocator war Sonderfall mit Laufzeit-API + std-Impl) | — | d91140a |
| 37 | **R5.B memory_layout (3. Achse) runtime-operativ** (scan_field_sum AoS/SoA/AoSoA/packed in 5 Wrappern) + **ehrliches Wall-Clock-Limit** (Layout-Effekt unter Rausch-Schwelle → motiviert PMC; KEIN Effekt behauptet) | axis_05 16/16; 48 DLLs | 5e1fc8d |
| 38 | **R5.D F15-Ranking robust nach Median (p50)** statt Mittelwert (gegen ~10×-Ausreisser) | p50<mean stabil; Cluster reproduzierbar | 8a50530 |
| 39 | **R5.D Mann-Whitney-U** robuster Rang-Signifikanztest + Diskrepanz-Erkennung (recoveriert 2 reale Differenzen, die Welch via varianz-inflation verschluckte) | f15_measurement 26/26 | 48910e6 |
| 40 | **R5.D Cliff's delta** robustes Effektmass (rang-biserial aus U) — Stats-Triade vollstaendig (Median+MWU+delta) | delta=0,84 (large) f. allocator-Effekt | 90e9f21 |

**FINALER STAND (40 Einheiten, ce HEAD 90e9f21 = v34-final-219):**
- **Säule 1 — Achsen-Bibliothek:** axis_03a **17 Such-Strategien** (Paradigmen-Palette + geordnete Struktur in ALLEN drei Balance-Auspraegungen: BST unbalanciert / SkipList probabilistisch / B-Baum det.+block) · axis_06 +PoolResource (25 Vendor, 1 behavioral-distinkt) · axis_05 5 Layout-Wrapper jetzt **runtime-operativ** (scan_field_sum).
- **Säule 2 — F15-Messung:** **3-Achsen-Mess-Maschinerie** end-to-end (search×allocator×memory_layout, 48 DLLs, Manifest) · **zweidimensionale Messung empirisch belegt** (search ~119× **UND** allocator ~2–3×, beides am std::map-Interface) · **robuste Statistik-Triade** (Median-Ranking + Mann-Whitney-U + Cliff's delta).
- **Ehrliche Limit-Befunde (dokumentiert, NICHT als Erfolg ausgewiesen):** memory_layout-Cache-Effekt liegt unter der Wall-Clock-Rausch-Schwelle → saubere Quantifizierung fein-granularer Achsen braucht HW-Performance-Counter (R5.D-PMC). Trait-Achsen (serialization/…) brauchen erst eine Laufzeit-API, bevor ihre Variation nicht-hohl messbar ist.

**Methodik (Fortsetzung):** durchgaengig verify-first + nicht-hohl; R5.B von „gated" zu „erfuellt (allocator-Dimension)" konvertiert, indem die VERIFIZIERTE Ursache (Achsen trait-only) behoben wurde (PoolResource + scan_field_sum). Negativ-/Limit-Ergebnisse ehrlich ausgewiesen statt Effekte zu erfinden. 3-Repo-synchron, +10 Restore-Tags (pre-r7.2-bst … pre-r5d-cliffdelta).

**Genuin verbleibend (verifiziert NICHT in-Kontext-bounded):** R5.D-**PMC** (HW-Cache-Counter, OS-/Infra-Scope — nötig für fein-granulare Achsen) · R5.B-Erweiterung auf weitere Trait-Achsen (je Achse Laufzeit-API-Neugestaltung) · voller kartesischer Raum (1e15 → braucht Sampling-Strategie) · F.2/F.3 Namespace (codebase-weit GROSS) · E11/E10 (gated E4.1-Submodule) · D1/D2 (Autor-Volltext, Doku 22 als Vorlage).

---

## PAUSE-STAND (2026-05-29) — User-Kurs-Korrektur + Säule-1-Start; manuelle Wiederaufnahme

**Kontext:** Nach den Einheiten 1–40 (Such-Bibliothek + F15 + robuste Statistik + Coverage + F.2/F.3-Fassade) hat der User ZWEI fundamentale Architektur-Abweichungen festgestellt und eine Kurs-Korrektur gesteuert. Dieser Block hält den Stand bei der Pause fest. **Repo:** ce `3d3a41a` (v34-final-231) · da `27e3b70` · pa `35d1143` — 3-Repo-synchron.

### A. Festgestellte Abweichungen (User 2026-05-29) — beide bestätigt + dokumentiert
1. **Säule 1 — „Tiere statt Organe":** axis_03a wurde um monolithische self-contained Such-Strukturen (BST/B-Baum/SkipList/Hash, eigener Speicher, schmaler uint8/uint16-Key) erweitert. Widerspricht Doku 14 §1–§3 (Achse=Organ, Algorithmus=Komposition über 17 Organe) + §7 (axis_03a REFACTORING-PFLICHT).
2. **Säule 2 — Wall-Clock-3-Achsen statt Per-Achsen-Observer:** F15 maß Wall-Clock über 3 künstlich variierte Achsen + umging `observe_all()` (den Per-Achsen-`ObserverAggregate`, Doku 14 §17.2/§20).

### B. Korrigiertes Mess-Modell (Doku 24, autoritativ) — drei getrennte Aspekte
1. **Tier-Ebene (ganzer Algorithmus):** CacheEngineBuilder misst Wall-Clock als Akkumulation von Detail-Kurven (Latenz über Füllstand, read/write/delete getrennt) + RAM + Disk. (Wall-Clock bleibt hier.)
2. **Achsen-Ebene:** `observe_all()` → `ObserverAggregate` sammelt je Achse `statistics()` → Gesamt-Statistics-Trace.
3. **Achsen-VERGLEICH:** Tests gegen die vereinheitlichten Achsen-Interfaces vs. bekannte Algorithmen (z. B. `verify_matches_std_map`), NICHT die Latenz-Benchmark.

### C. In dieser Korrektur-Phase erledigt (verifiziert, committed)
- **Doku 24** (Mess-Modell-Korrektur, §1–§6) + Doku-22-Korrektur-Banner + Memory `[[feedback_zwei_dimensionen_messmodell]]`.
- **F.2/F.3** (Doku 23): axen-zentrische Alias-Fassade (17) + abstrakte Achsen-Concepts (alle 17, `cache_engine::concepts::*Axis`) + `optional_prt_art_impl`-Slots (17). perm-engine 21/21.
- **Säule 2, Schritt 2a:** `SearchAlgorithmAnatomy::observe_all()` un-gestubbt — hält `search_algo`-Organ real + sammelt dessen ECHTE `statistics()` (Test `Saeule2_ObserveAllReal`). Regress 0.
- **Trennungs-Verifikation (Doku 24 §5):** CEB↔Composition besteht formal, ist aber 3-Pfad-fragmentiert; konkret liefert `AnatomyExecutionContext::observe_all()` NULL (Container=std::map ↔ Observer=ungetriebenes Organ entkoppelt). **Blocker §5.5:** schmale Organ-Key-Typen (uint8/uint16) → Organ untauglich als allgemeiner Container.
- **Säule 1, Increment 1 (Doku 24 §6):** `composable/composable_search.hpp` — komponierbares Modell (RawSlotStore Storage-Organ + `TraversalOrgan`-Concept + LinearScan/SortedBinary-Organe + `ComposedSearch`) über GEMEINSAMEM uint64-Key → Key-Type-Blocker strukturell gelöst, Organ-Swappability bewiesen (Test `Saeule1_ComposableOrgan`, traversal 297/297).

### D. RESUME-PLAN (Reihenfolge, wenn der User fortsetzt) — Doku 24 §6 Folge-Increments
1. **node_type/layout/allocator als ECHTE Storage-Organe** (statt trait-only) → `RawSlotStore` durch organ-getriebenen Speicher ersetzen (axis_04/05/06 bekommen ein echtes Slot/Storage-API).
2. **Tier-Wrapper umstufen:** Array256/BST/B-Baum/Original*… als **Reference-Compositions / Stufe-2-Prüfling-Referenzen** (Doku 14 §6), NICHT als axis_03a-Organe.
3. **Anatomie + AnatomyExecutionContext auf `ComposedSearch`** (gemeinsamer uint64-Key) umstellen → schliesst die §5.2-NULL-Lücke (observe_all real im Mess-Pfad) + vereinheitlicht die 3 Pfade.
4. **Säule 2 vollenden:** Mess-Pfad (abi_adapter/CLI) erhebt pro Permutation den Per-Achsen-`observe_all`-Trace ZUSÄTZLICH zur Tier-Wall-Clock; Tier-Metriken anreichern (Füllstands-Kurven, r/w/d, RAM/Disk).
5. **Achsen-Vergleich** (Doku 24 §2.3) als Interface-Tests-vs-bekannte-Algos ausbauen.

### E. Restore-Tags dieser Phase (alle gepusht)
`pre-f2f3-axiscentric-aliases-…` · `pre-f3-all17-concepts-…` · `pre-saeule2-observer-korrektur-…` · `pre-saeule1-axis03a-remodel-20260529` (= Stand vor Increment 1).

**Status: PAUSE auf User-Wunsch.** Bestehende F15/Wrapper-Arbeit ist nicht „falsch" (Wrapper erfüllen std::map korrekt, als Reference-Compositions nutzbar); sie ist im Mess-Modell neu eingeordnet (Doku 24). Wiederaufnahme manuell durch den User.
