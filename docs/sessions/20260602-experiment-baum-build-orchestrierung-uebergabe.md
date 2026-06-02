# Session-Übergabe — Experiment-B+-Baum (KF-8/KF-9) → Build-Orchestrierung (KF-16) (2026-06-02)

> **Zweck.** Elaborate Übergabe des Cache-Line-Konfigurator-Arbeitsstroms (aktives `/goal`) nach Abschluss von
> KF-8 (CebGenerator) + KF-9 (Experiment-B+-Baum) und vor Umsetzung der **neuen User-Direktive 2026-06-02**
> (statischer-Teilbaum-Iterator + multithreaded DLL-Build-Orchestrierung durch die CacheEngineBuilder).
> Provenienz: Fortsetzung nach Kontext-Kompaktierung; Architektur-Pre-Read aller 18 letzten Architektur-Dokumente
> in Präzision (10 vollständig + 8 präzise extrahiert) am 2026-06-02 durchgeführt.

---

## 0. Das aktive `/goal` (verbatim-tragend)

> „Vollende den Diplomarbeit-Cache-Line-Konfigurator über die verifizierte End-to-End-Kette hinaus (erledigt:
> KF-1/2/3/4 + KF-14). Maßgebliches Modell: docs/architecture/26 (Permutations-B+-Baum + inverse Signatur, NICHT
> Fingerprints). SCHICHT-TRENNUNG strikt: die Cache-Engine-Bibliothek besitzt das WIE (Profilverwaltung,
> Permutation, B+-Baum, Messen am Prüf-Dock via CacheEngineBuilder/Experiment-Manager); die Diplomarbeit liefert
> nur das WAS (comdare_thesis_profile) und liest Ergebnisse READ-ONLY über Baum-Traversal. Tiere/Organe bleiben
> UNVERÄNDERT (der Manager misst+organisiert nur). Für Suche IMMER Bäume (linear). Alles in C++23."

Danach: KF-9/KF-8/KF-5/KF-6/KF-7/KF-10/KF-11/KF-15 autonom; KF-12/KF-13 (SLURM/ZIH) nur VORBEREITEN (Cluster-gated).
Nach jedem Subtask: elaborate Planung. Nach jedem Meilenstein: elaborate Session-Doku. Commit+Push+Submodul-Sync
aller 3 Repos nach jedem Schritt.

---

## 1. KF-Task-Stand (verifiziert)

| KF | Inhalt | Status | Beleg |
|----|--------|--------|-------|
| KF-1 | Self-contained XML-DOM-Reader + `parse_thesis_profile` | ✅ | `xml_reader.hpp` + `xml_config_parser.{hpp,cpp}` |
| KF-2 | XML-Schema `comdare_thesis_profile` + Beispiel `cacheline_study` | ✅ | `algorithm_profiles/thesis_profiles/` |
| KF-3 | Cache-Line-Unterachse (Goldstandard-Vorlage) | ✅ | `axes/cacheline/cacheline_config.hpp` (45=3×3×5, `CacheLineAware<Cfg>`) |
| KF-4 | `Algorithm_Resource_Control` Laufzeit-Steuerschnittstelle | ✅ | `anatomy/resource_controllable_tier.hpp` + `builder/algorithm_resource_control.hpp` |
| KF-14 | Thesis-Anbindung (Profil → Mess-Anhang, 3-Wdh-Overlay) | ✅ | `anhang/{de,en}/A_measurements.tex`, verifizierte E2E-Kette (PDF 0 Fehler) |
| **KF-9** | **Experiment-B+-Baum (Experiment-Manager) + per-node Key-Value** | ✅ **12/12** | `builder/experiment_tree/experiment_tree.hpp` + `profile_to_tree.hpp` (CE cb21be4) |
| **KF-8** | **C++23/CEB-Generator (kein Python) — Baum → `perm_<id>.cpp`** | ✅ **11/11 + gen. Source kompiliert** | `builder/experiment_tree/ceb_generator.hpp` (CE 56a6931, DA c1b0a9d) |
| KF-5 | Cache-Line-Support in betroffene Tier-Algorithmen einweben (4 Achsen) | ⏳ offen | — |
| KF-6 | node-Achse echt verdrahten (Run-Body-Divergenz je Node-Format) | ⏳ offen | — |
| KF-7 | Dynamischer Laufzeit-Durchlauf am Prüf-Dock (`Algorithm_Resource_Control` + MSR) | ⏳ offen | — |
| KF-10 | Wiederholungen Default 3 — separat, nie interpoliert | ⏳ offen | — |
| KF-11 | Telemetrie Default-AN + silent-mode (Snapshot-Diff) | ⏳ offen | — |
| KF-15 | Inverse Auswertung via Signatur-`multimap` + read-only Tree-Interface | ⏳ offen | — |
| **KF-16** | **NEU 2026-06-02: statischer-Teilbaum-Iterator + multithreaded DLL-Build-Orchestrierung** | ⏳ in Arbeit | — |
| KF-12/13 | SLURM-Launcher + ZIH-Array/Singularity/Webhook | ⏸ NUR VORBEREITEN (Cluster-gated) | — |

---

## 2. Das maßgebliche Modell (Doc 26) — knapp

- **EIN zusammenhängender Permutations-B+-Baum** = das Gesamt-Experiment. Jede Ebene = Achsen-Entscheidung.
  Gepinnte Achse → Fanout 1; freigegebene Achse → Fanout N.
- **STATISCHE Knoten** (compile-time, inkl. cacheline size/align/hint): je distinkter Static-Pfad **lädt EINE neue
  Tier-Binary** ins Prüf-Dock. **DYNAMISCHE Knoten** (thread_count, hw_prefetcher): **virtuelle for-Schleife** auf
  EINER geladenen Binary via `Algorithm_Resource_Control` (KF-4). Erzeugen KEINE neue Binary.
- **Materialisierung:** nur die statischen Ebenen werden materialisiert (→ Binary-Blätter); dyn. Variablen sind
  virtuell geschachtelte Schleifen → `binary_count` ≠ dyn. Kartesik. `static_filter()`/`dynamic_filter()`.
- **Blatt = EIN Mess-Lauf** (`ExperimentSetting` = Binary × eine dyn. Belegung). Pfad Wurzel→Blatt = serialisierte
  ID (ERSETZT FNV1a-Fingerprint).
- **Jede node hält key+value:** key = serialisierte Signatur der Kind-Permutationen; value = Observer-Statistics +
  Mess-Auswertung der Ebene → Diplomarbeit liest auf beliebiger Granularität read-only.
- **Paper-Wiedererkennung** (KF-15): statische Signatur (gepinnte Achsen-Werte) → `multimap<Signatur, Paper>`.
- **Zwei Knotenarten via Abstract Factory** (kein enum-Flag): `StaticAxisNode` / `DynamicVariableNode`.

---

## 3. NEUE User-Direktive 2026-06-02 → KF-16 (in Arbeit)

> „Der gefilterte Statische Teilbaum liefert aus der CacheEngineBuilder auch ein Interface, um die notwendigen
> statischen Eigenschaften zur Kompilation aller notwendigen Tier-Binaries **indexiert mit einem Iterator** zu
> durchlaufen, um die Tier-Binaries **vor Durchführung der Experimente bereitzustellen**. Laut Architektur ist es
> Aufgabe der CacheEngineBuilder die dll Tier-Binaries vor den Experimenten bereitzustellen, dabei soll
> **Multithreading für die Builds** unterstützt werden. Günstig ist ein **default von 2 Kernen je parallelem dll
> Build Prozess unter Verwendung aller CPU Kerne**."

**Architektur-Einordnung (Pre-Read-Beleg):**
- `messarchitektur_v5_design.md` §2: „Profil baut **zuerst den HOST** → Host permutiert eine Tier-Binary-Config →
  **baut zuerst alle DLLs** → **misst danach** (Build vor Mess-Phase, nicht verschränkt)." → KF-16 ist die
  **C++23-Realisierung dieses „baut zuerst alle DLLs"**, getrieben vom statischen Teilbaum.
- Doc 26 §0: Build+Messen = Aufgabe der Bibliothek (`CacheEngineBuilder`, das WIE). → KF-16 sitzt korrekt im Builder.
- Bestehender Mechanismus (Doc 21 §5, Doc 24 §8.3): `adhoc_emitter` → `comdare_build_adhoc_modules` (CMake-Glob-Loop)
  → `AnatomyModuleLoader::load_all`. KF-16 **ersetzt die CMake-Glob-Loop-Rolle für die Experiment-Baum-Binaries**
  durch einen C++23-Orchestrator (kein Python — `[[no_python_in_buildchain]]`; der Orchestrator ruft den Compiler
  als Subprozess, parallelisiert).

**KF-16-Bausteine (Plan):**
1. **`StaticBinaryView`** (in `experiment_tree.hpp`, erweitert KF-9 `static_filter`/`for_each_binary`): **indizierter,
   iterierbarer** Blick auf den statischen Teilbaum — `size()`, `operator[](i)`, `begin()/end()`, je Element
   `BinarySpec{index, binary_id, pinned_signature, axes[(achse,wert)…]}`. Das ist exakt das geforderte „Interface,
   um die statischen Eigenschaften zur Kompilation aller Tier-Binaries indexiert mit einem Iterator zu durchlaufen".
2. **`BuildOrchestrator`** (`builder/build_orchestrator/build_orchestrator.hpp`, C++23, header-only): nimmt die
   `StaticBinaryView` + KF-8-`generate_perm_source` → erzeugt/kompiliert je Binary ein `perm_<id>.dll`.
   **Multithreading:** `parallel_jobs = max(1, total_cores / cores_per_build)`, Default `cores_per_build = 2`,
   `total_cores = hardware_concurrency()`. Thread-Pool (`std::jthread` + atomarer Index), je Worker EIN Compile.
   `cores_per_build` ist zugleich Parallelitäts-Divisor UND `{cores}`-Token im Compile-Kommando (z.B. MSVC `/MP2`).
   **Injizierbare `CompileFn`** (Default = realer `cl /LD`-Subprozess; Test = Stub) → Orchestrierung deterministisch
   testbar (jeder Index genau 1×, beobachtete Parallelität ≤ `parallel_jobs`, indizierte Ergebnisse) + EIN realer
   E2E-Compile als ehrlicher Beleg.
3. Per-Binary `BuildResult{index, binary_id, status, dll_path, message}`.

---

## 4. Schicht-Trennung (strikt, mehrfach bestätigt)

- **Cache-Engine-Bibliothek = WIE:** Profilverwaltung + Permutation (Baum-Aufbau/-Traversal) + Organe/Achsen + Messen
  am Prüf-Dock + **Build-Orchestrierung (KF-16)** = immer Builder-Aufgabe.
- **Diplomarbeit = WAS:** liefert `comdare_thesis_profile`, liest Ergebnisse **read-only über Baum-Traversal**.
- **Tiere/Organe UNVERÄNDERT** durch den Experiment-Manager. AUSNAHME (User-mandatiert): die per-Organ-`cacheline`-
  Unterachse (KF-3/KF-5) ist eine **separate Bibliotheks-Achsen-Erweiterung** (erweitert Organ-Algorithmen, damit
  sie die untersuchte Cache-Line-Eigenschaft tragen) — NICHT Teil des Managers.

---

## 5. Compile-/Runtime-Faustregel (Architektur-Ausnahmen)

- **Compile-time (StaticAxisNode → Binary):** alle Architektur-Entscheidungen, inkl. cacheline `line_size`/`alignment`/
  `sw_hint`. KF-8 backt sie als `#define COMDARE_PERM_<AXIS>_IS_<VALUE>` in den Perm-Source.
- **Runtime (DynamicVariableNode → for-Schleife):** OS-seitig einstellbar ∧ nicht-architektonisch → `thread_count`
  (Algorithm_Resource_Control, KF-4), `hw_prefetcher_state` (MSR 0x1A4 Intel / AMD — KF-7/Launcher).

---

## 6. Verifizierte End-to-End-Kette (KF-14, bleibt grün)

DLLs → Treiber-`.bin` → `binary-to-csv` (16-Spalten, magic 0xC0FFEE02 v2) → `generate_measurement_appendix.ps1`
→ `A_measurements.tex` → `build.ps1 -Lang {de,en}` → PDF (Tabelle/Abbildung A.2, 0 LaTeX-Fehler).

---

## 7. Stehende Constraints (Pflicht)

Deutsch (Diakritika); KEINE Erfolgsmarken ohne literale Tool-Ausgabe; keine Quick-Fixes (Root-Cause);
Doku nie löschen (nur `git mv`); destruktive Ops in den 3 Thesis-Repos OHNE Rückfrage NUR mit Tag+Commit+Push;
Submodul-Sync nach jedem ce/pa-Push (DA-Pointer bumpen); Keys nie committen; Algorithmus-Korrektheit bei
Namensanspruch; KEIN Python in der Buildchain; Compile-Time-Only / kein Runtime-Switch im Hot-Path; FÜR SUCHE
IMMER BÄUME (linear); Tiere/Organe bleiben durch den Manager UNVERÄNDERT; Bibliothek = WIE / Diplomarbeit = WAS.

---

## 8. Nächste Schritte (Reihenfolge)

1. **KF-16** umsetzen + verifizieren (StaticBinaryView + BuildOrchestrator, Stub-Test + realer E2E-Compile).
2. **KF-5** Cache-Line-Support in die 4 Achsen (page/node/traversal/allocator) einweben.
3. **KF-6** node-Achse Run-Body-Divergenz; **KF-7** Prüf-Dock-Laufzeit-Schleife.
4. **KF-10** 3 Wiederholungen separat; **KF-11** Telemetrie silent-mode; **KF-15** inverse Signatur-Auswertung.
5. **KF-12/13** vorbereiten (nicht ausführen). Finaler Audit gegen das Ledger.

---

## 9. Abschluss-Stand dieses Turns (2026-06-02) — 6 KF-Meilensteine verifiziert + gepusht

| KF | Inhalt | Beleg | Commit (CE) |
|----|--------|-------|-------------|
| KF-8  | C++23 CebGenerator (Baum→`perm_<id>.cpp`, kein Python; bricht 5-Achsen-Deckel) | Test 11/11 + generierter Source kompiliert | 56a6931 |
| KF-16 | StaticBinaryView (indizierter Iterator) + multithreaded DLL-Build-Orchestrierung (2 Kerne/Build, alle Kerne) | 26/26 + realer E2E-Build 2/2 DLLs (peak=2) | 08a07e7 |
| KF-5  | Cache-Line-Unterachse in **5 Achsen** eingewebt (4 + axis_05_memory_layout, User-Entscheidung) | gegen echte Wrapper: COMPILE 0 + 10/10 + alle static_assert | 43d8008 |
| KF-6  | node-Achse runtime-operativ — `node_find_scan` divergent je ART-Format | COMPILE 0 + 8/8: 4 distinkte Prüfsummen 160/270/115/30 | 397d123 |
| KF-7  | dynamischer Laufzeit-Durchlauf am Prüf-Dock (`RuntimeVariableLoop`, eine Binary/N Einstellungen) | COMPILE 0 + 18/18 (Clamp cap∩env, Kartesik, hw_prefetcher launcher-gated) | 5680063 |
| KF-15 | inverse Auswertung via Signatur-`multimap` + read-only Tree-Interface | COMPILE 0 + 16/16 (Doppelerkennung + Projektion) | fe2ff30 |

**Damit ist die Experiment-Baum-Kette geschlossen:** Profil → B+-Baum (KF-9) → statischer Teilbaum-Iterator (KF-16)
→ CebGenerator (KF-8) → multithreaded DLL-Build (KF-16) → [Prüf-Dock: statische Organe cacheline-fähig (KF-5) +
divergente node-Run-Bodies (KF-6)] → dynamischer Laufzeit-Durchlauf (KF-7) → inverse Paper-Projektion read-only (KF-15).

**VERBLEIBEND (autonom):** KF-10 (Wiederholungen Default 3, separat/nie interpoliert — CSV repetition_index +
Diagramm-Overlay) · KF-11 (Telemetrie Default-AN + silent-mode via Snapshot-Diff). **VORBEREITEN (Cluster-gated,
NICHT ausführen):** KF-12 (C++23 SLURM-Launcher: affinity/MSR/governor/SMT/ASLR/NUMA) · KF-13 (ZIH SLURM-Array +
Singularity + Webhook). Danach finaler Audit gegen das Ledger.
