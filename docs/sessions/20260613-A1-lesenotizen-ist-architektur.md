# A1-Lesenotizen + IST-Architektur-Destillat (Masterplan-Phase A1 → B-Entwurf)

> **Zweck:** Durable Destillation der Architektur-Vollektüre (Masterplan A1), kompaktierungs-fest. Jede Zeile =
> verifizierter Stand aus den gelesenen Docs/Code. Wird in Phase B zum konsolidierten Master-Doc poliert.
> **Quellen-Status:** Thesis-Basis `docs/architektur/` 00–14 = **SUPERSEDED-Banner (2026-05-31)** = Begriffs-/
> Konzept-Kontext, NICHT IST. **IST-Single-Source = `docs/sessions/architektur-ziele-offene-punkte-ledger.md`
> (Stand 2026-06-01, HEAD 292aef9) + `20260531-e2e-abnahme-audit-und-entscheidungen.md`.**

## 1. Konzept-Modell (aus Docs 09/10/11/14 — Begriffs-Fundament, gilt weiter)

**3-Ebenen-Modell (Doc 30 §8.0 / Doc 14 §0, AUTORITATIV):**
- **GATTUNG = ein INTERFACE / Prüf-Dock:** `SearchAlgorithm` / `Container` / `Graph` (die Außen-API, je Gattung ein Dock).
- **TIER-UNTERKLASSE = unter dem Gattungs-Interface, FESTER Achsen-Satz:** die 5 Klassen SearchAlgorithm/Set/
  Sequence/Adapter/View (Säugetier/Vogel/Reptil/Wirbelloses/Pflanze) sind **Tier-Unterklassen, KEINE Gattungen**.
- **ACHSEN = Organe der Tier-Unterklasse. KEINE Achse optional** — jede in JEDEM Tier-Binary uniform getrieben;
  „kein Puffer/Prefetch" = konkreter Durchreich-Algorithmus (NoBuffer/NonePrefetch/…), NICHT „Achse weggelassen".
- **queuing q1/q2 = Pflicht-Achsen** der SearchAlgorithm-Tier-Unterklasse (→ AdHocComposition 17→**19**).

**Organ-Metapher (Doc 14 §1/§9, Master-Statement):** Achse=Organ · Algorithmus=Permutations-Konfiguration ALLER
Achsen · **Permutation = genetisches Experiment am Tier (Organe testweise gegeneinander tauschen)** · Bottom-Up:
abstrahiere vom Algorithmus (Tier) zum Organ. Original-Algorithmen = Reference-Compositions (1 Punkt im Raum).
ArtComposition vs ArtPaperBindingComposition = unterscheiden sich in GENAU `search_algo`, 16 Achsen identisch.

**4 Konzept-Ebenen (Doc 11 axes_vs_strategies — Anti-Vermischung):** I Bausteine-Achsen (Permutations-Dim) ·
II CE-Sub-Engines C1-C12 (CE-interne Services) · III Cache-Strategien F1-F29 (Impl. der Sub-Engines) ·
IV Such-Engine-Familien S1-S30 (Impl. der Achsen). Achse ≠ C-Sub-Engine ≠ F-Strategie ≠ S-Familie.

**4-Subsystem-Modell (Doc 10):** messung_driver (OUTER LOOP/Auswertung, Diplomarbeit/Code) → **CacheEngineBuilder**
(autonomes Plattform-Ausmess-System, App) → **CacheEngine** (Werkzeug-Bibliothek) ↔ **Prüfling (prt-art)**
(bidirektional: CE bindet Prüfling als Permutations-Struktur + Prüfling nutzt CE-Services). CEB+CE im selben Repo,
2 unabhängige Subsysteme. Orthogonal zum 3-Repo-Layer (Diplomarbeit/prt-art/cache-engine).

## 2. B+-Experiment-Baum (Doc 26/27/29 + `builder/experiment_tree/experiment_tree.hpp` — KERN für Achsen-Austausch)

- Achsen = **Baum-Ebenen** (`AxisLevel{axis, values[]}`, static/dynamic gleichrangig als Knoten-Eigenschaft).
  Ein Pfad Wurzel→Blatt = ein `binary_id` = eine Tier-Binary (statische Rekombination). Blatt + dyn. Belegung
  = eine ExperimentSetting.
- **Nie voll materialisiert** (∏ astronomisch): lazy Mixed-Radix-Odometer, O(Tiefe) Speicher. `binary_count()` =
  ∏ arithmetisch. `StaticBinaryView`: `operator[](i)` ⇄ `flat_index(tuple)` = **inverse Bijektionen**.
- **ACHSEN-AUSTAUSCH = tree-nativ:** `tuple`, NUR `tuple[d]` (Ebene d=Achse a) k→k' ändern, `flat_index` →
  Geschwister-Tier (diff in genau a). O(Geschwister), NICHT flach-quadratisch. ⇒ **Achsen-Struktur/-Austausch
  gehört in den Baum (cache-engine), NIE flach im Eval-Tool** ([[feedback_axis_exchange_belongs_in_bplus_tree]]).
- Mess-Werte sparse in `value_map_` (key=binary_id → NodeValue mit axis_stats[19][8]+seg_ns[19]). KF-15 inverse
  Auswertung = multimap pinned_signature→binary_id. Knoten via Abstract Factory (Static/Dynamic).

## 3. IST-Stand der Mess-Architektur (Ledger 2026-06-01 + V5-Design — AUTORITATIV)

- **3-Säulen (Doc 02, alt aber Struktur gilt):** IExecutingEngine → ISearchEngine/IFutureEngine → Säule A
  (Suchalgo-Datenstruktur) → Säule B (CacheEngine+Plattform-Modell).
- **V5-Mess-Architektur (Ledger §a.V5, I1–I10 done-verified):** 2 Phasen (save→op-warmup→rollback→op-measure) ·
  3 Profile · `memento_all`/`observe_all` parallel · **Konformitäts-Gate gegen std::map** (import→GATE→messen,
  in ALLEN 3 Mess-Pfaden) · **Observer/Memento NUR compile-time entfernbar (kein dynamic_cast im Hot-Pfad)** ·
  `IDriveableTier` (Antrieb, immer) + `IObservableTier` (nur MEAS_ON) + `IRollbackableTier` + `IScannableTier`.
- **ABI:** ANATOMY_ABI_MAJOR (Loader-Reject per Magic-Mismatch); I1-Observer-Konsolidierung = EINE IObservableTier
  + EIN POD (axis_stats[19][8]+seg_ns[19]) + Major 2→3 (Doc 31). status_t errno-style (Schreib-Ops int, Lese-Ops
  natürlicher Typ).
- **17/17 Achsen physisch unter `libs/cache_engine/axes/<axis>/`** (F.2, `topics/`=Forwarder). Allocator=28 Wrapper.
- **3-Stufen-Prüfung (R8 done):** Stufe1 CE-only · Stufe2 Prüfling-einzeln (ERSETZT-mit-Fallback; leere Prüfling-
  Achse reust ALLE CE-Algorithmen) · Stufe3 full-join (Union non-redundant). **Prüfling ≠ Prüflings-Binary**
  (Prüfling = komplettes Achsen-Kompendium/Projekt; Binary = EINE Rekombination; CEB orchestriert Zehntausende).
- **Verantwortlichkeit (Doc 14 §17):** PermutationEngine=Anatomie-Generator (1 Anatomie=1 Permutation) ·
  SearchAlgorithmAnatomy=Organ-Container + ABI-Observer-Aggregat (KEINE insert/lookup — die sind CEB-Commands) ·
  CacheEngineBuilder=Mess-Orchestrierung + ABI-Loader (CMake je Permutation → .dll, dlopen, measure).
- **CEB-Pipeline (Doc 02 §6):** 1 enumerate · 2 codegen · 3 compile (Stage-1/2) · 4 load · 5 run · 6 measure · 7 export.
- **F15-Mission:** schnellste Rekombination im Permutations-Raum finden + alle Achsen-Permutationen studieren;
  V1–V4 Engine-Choice-Dimension (NO-CE/Static/Informed/Adaptive).

## 4. Offene Punkte / Vorbehalte aus dem IST-Ledger (für D/E relevant)
- Vendor-Allokatoren (#19, jemalloc/tcmalloc/hoard/scalloc) + reale PMC (#26) = **extern/toolchain-gated**
  (lokal nicht baubar; Beschaffungs-Specs geliefert; erst ZIH/Cluster). Mechanik an mimalloc/snmalloc/dlmalloc bewiesen.
- #22 Submodule-Repos: Kern-Befüllung done (Option A); Option-B-Konsumptions-Migration gated auf GitLab/DependencyManager.
- V5-I-Drive-Vollausbau (IDriveableTier auf volle std::map/std::vector-API) = offener Strang (Voraussetzung volle Gate-Äquivalenz).
- YCSB A–F treu+zitiert done; ehrliche Lücke „update=Upsert" dokumentiert.

## 4b. IST-Ledger §b/c/d/e (vollständig gelesen, Z.148–226) — Status-Landkarte für C/D/E

- **§e VERIFIZIERT-ERLEDIGT (der Großteil!):** V5 I1–I10 · F.2 17/17 Achsen · F.3 17 Concepts · F.4 Tools-Facade ·
  R6.1–6.5 (IObservableTier+POD, Wall-Clock+Observer-Trace, Loader, Pfad-B-2D, Prüf-Dock) · R7.2/7.3/7.4-Body ·
  R8 (Prüfling 3-Stufen) · Umstufung-A/B · s4 · Cross-Constraints · G.1 (messung_driver axis_tree) · E10.x ·
  **#42-Phase-2: `EnabledStrategies = mp_filter<is_enabled, AllStrategies>` = 4 Such-ORGANE (K_ARY/INTERPOLATION/
  EYTZINGER/LINEAR_SCAN, USE=1), 13 Monolith-Tiere USE=0 deregistriert** (konfig. Flags-Header; Direktive
  no_whole_tier_axes auf EnabledStrategies-Ebene erfüllt). ⇒ Architektur-Substanz steht; Mess-Pfad real.
- **§b EXTERN/TOOLCHAIN-GATED:** A1/A2.1 Vendor-Allokatoren (jemalloc/tcmalloc/hoard/scalloc — lokal nicht baubar) ·
  R5.D/#26 PMC-HW-Counter (Intel PCM/MSR) · C1/C2 Cluster/Grace-Hopper · E10.6/7 ZIH-Verteilung ·
  E11-Facade-Impl (gated auf #22/V42) · F.6-Phase-C (23 Legacy-Header erst NACH Habich-Termin löschen) ·
  **Doku-11/14-Verif „nicht im Repo" — HEUTE AUFGELÖST: per Junction `docs/architektur` (Thesis-Basis 00–14) zugänglich.**
- **§c USER-MANUELL:** D1 (Diplomarbeit-Kapitel-Text) · D2 (Bausteine-Matrix-Update) — User schreibt.
- **§d V42-FUTURE:** Gattungs-Docks Set/Sequence/Adapter/View (Blueprint=SearchAlgorithmDock) · R7.6.c is_original-
  Linking · Naming-Refactor-Backlog (axis_12/04/03a/q1q2/08) · E9 raw-string · #22 Submodule-Option-B.
- **Implikation für E:** Die Original-Mission (Bias-Matrix-Messung) baut AUF dieser fertigen Substanz auf; die
  Audit-Befunde (D) sind Korrekturen AM bestehenden Mess-Pfad, keine Neubauten. Achsen-Austausch-Auswertung
  gehört in Baum/CEB (nicht Eval-Tool).

## 5. A1-Lese-Fortschritt (Checklist)
- ✅ Thesis-Basis: 00_INDEX · 02_master_REV7_7 · 09_taxonomien · 10_schichten_modell_M · 11_axes_vs_strategies
- ◐ 11_konzept_extension_visitor (§1–§11 von §… ; 4 Patterns + 3-Stufen + CRTP+Concept + Prüfling-Namespace gelesen)
- ◐ 14_organ_metapher (§0–§20 von §53; Organ-Metapher + 3-Schichten + Verantwortlichkeit + ObserverAggregate)
- ✅ IST-Ledger (vollständig, 226 Z.; §a/§a.V5/§a.P/§b/§c/§d/§e)
- ⬜ OFFEN: `20260531-e2e-abnahme-audit-und-entscheidungen.md` (2. IST-Doc) · Thesis 01,03,04,05,06,07,08,12,13
  + Rest 11/14 · cache-engine 15–33 + benannte (abhaengigkeitskette, messarchitektur_design_observer,
  messarchitektur_v5_*) · A2 Code-Pre-Read · A3 Audits.
