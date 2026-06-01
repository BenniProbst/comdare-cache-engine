# Session-Übergabe 2026-06-01 — V5-Substanz, TODO-Abarbeitung, Build-Zustand

> **Pflicht-Pre-Read nächste Session:** dieses Dokument + `architektur-ziele-offene-punkte-ledger.md` +
> `20260531-v5-reverifikation-substanz-luecken.md` + `20260531-todo-abarbeitung-autonom.md`.
> Single-Source-of-Truth bleibt das Ledger.

## Git-Stand (WICHTIG)
- **HEAD = `16781af`**, **55 Commits vor `origin/main`, UNGEPUSHT** (FortiGate-SSL-Inspection blockt git push;
  zudem User aktuell nur per VPN → Push erst inside-network, durch Cert-/Inside-Agent). NICHT konkurrierend pushen.
- Working tree SAUBER an HEAD (nur untracked `ext/A05-jemalloc/` = Residuum eines jemalloc-paper-init aus dem
  #19-Experiment; harmlos, kann entfernt werden).
- DA-Superprojekt-Submodul-Pointer noch NICHT auf den neuen CE-HEAD gebumpt (nach Push fällig).

## Was diese Session committet + verifiziert wurde (alles committed-grün)
- **V5 I1–I10** (frühere Commits): ABI-Split, Konformitäts-Gate, Memento-Concept, Zwei-Phasen-Treiber,
  Host-Workload-Orchestrator, 3-Profil-cmake, Dock-Wiring, E2E→PDF. 8 Test-Exes grün (Real-Build).
- **#50** autoritativer 16+6-Mess-POD `ComdareMeasurementSnapshotV1` + Workload→PDF-Bridge (`f70dad7`),
  **pipeline_v5_real.pdf** mit echten V5-Daten. test_v5_measurement_snapshot 5/5.
- **#44** echte per-Achsen `MementoAxis` (ComposedSearch + ObservableComposedSearch) + Adapter-Wiring (`d467f69`).
- **#26** PMC-Mess-Quellen-Abstraktion (`IPmcSource`/`NullPmcSource`, ehrliches `pmc_available`) (`e10c59a`).
- **#46** Disk-Memento-Referenz (`DiskCheckpointStore`, Kontrakt „inkl. Disk-Persistenz" bewiesen) (`cc901e9`).
- **#49 Zipfian** YCSB-Treue: Zipfian/Latest-Key-Verteilung + Cooper-et-al-SoCC-2010-Zitat (`262539f`).
- **#19** mimalloc: 2 verifizierte ext/-Build-Bugfixes (HAVE-Set + Pfad allocator/A04-mimalloc) (`16781af`).
- **#27, #42** waren bereits erledigt (als completed markiert).

## ⚠️ YCSB-E/F-Versuch — VERWORFEN (wichtige Lektion)
Versuch, das Op-Set um Scan (E) + ReadModifyWrite (F) zu erweitern (User-Freigabe). **`tier_scan` IN-PLACE
auf `IObservableTier` zu legen brach die vtable-Kompatibilität** mit bereits gebauten/geladenen DLLs →
**SEH 0xc0000005** in `test_v41_pruef_dock_search_algorithm` + `test_v41_anatomy_adhoc_dll_load`.
→ **VOLLSTÄNDIG REVERTED** (git checkout der 10 Dateien; Source zurück auf committed-grün; ABI-Minor zurück 1).
**LEKTION für nächste Session:** Scan/RMW NICHT durch In-Place-Änderung von IObservableTier, sondern als
**separates optionales Sub-Interface `IScannableTier`** (per `dynamic_cast` gezogen, alte DLLs → null →
graziöser Skip) — wie IRollbackableTier. RMW (F) braucht KEINE ABI-Änderung (lookup+insert im Runner). Die
Zipfian-Verteilung (das YCSB-Definitionsmerkmal, #49) ist bereits committed-grün.

## 🔧 BUILD-ZUSTAND `build/msvc-release` — REPARATUR NÖTIG (lokal, nicht committed)
Meine #19-mimalloc-Experimente (PERMUTATIONS=ON-Reconfigure-Toggles) + ein `--clean-first` haben den
**lokalen** Build-Dir korrumpiert. Source/Commits sind unberührt. Zwei Befunde:
1. **Generierte is_original-Header gelöscht** (`--clean-first`). Regeneration: `cmake --build build/msvc-release
   --target is_original_validator comdare_paper_a04_mimalloc_codegen comdare_paper_a05_jemalloc_codegen
   comdare_paper_a07_snmalloc_codegen comdare_paper_a10_rpmalloc_codegen comdare_paper_a11_lrmalloc_codegen
   comdare_paper_a20_dlmalloc_codegen comdare_paper_p01_art_codegen comdare_paper_p02_hot_codegen
   comdare_paper_p05_start_codegen comdare_paper_p07_wormhole_codegen comdare_paper_p10_surf_codegen
   comdare_paper_q01_concurrentqueue_codegen` (ALL-Targets, laufen NICHT bei gezieltem Test-Build).
2. **VORBESTEHENDER latenter Build-Config-Bug (von mir freigelegt, NICHT meine Regression):** die 23
   `libs/cache_engine/axes/alloc/vendor_includes/*.hpp` inkludieren `topics/allocator/axis_06_allocator/
   axis_06_allocator_flags.hpp`, aber die configure_file (CMakeLists.txt:734) generiert die Flags NUR nach
   `axes/alloc/`. Der topics/-Pfad existierte nur als **stale Altlast** in `build/.../generated/topics/`.
   - Mein Wipe (`rm -rf generated/`) entfernte die Altlast → frischer Build scheitert an den vendor_includes.
   - **NICHT lösbar durch Duplizieren** der Flags nach topics/ (verursacht C2374/C2086-Doppeldefinition von
     `alloc::flags::*`, da `flags.hpp.in` per `#pragma once` pfad-basiert dedupliziert).
   - **KORREKTE Fixe (eine wählen, nächste Session):** (a) die 23 vendor_includes auf
     `<axes/alloc/axis_06_allocator_flags.hpp>` umstellen (= Konvention der `axis_06_allocator_*.hpp`-Wrapper,
     die alle axes/alloc/ nutzen) — sauberste Lösung; ODER (b) `flags.hpp.in` mit Makro-Guard
     (`#ifndef COMDARE_AXIS_06_ALLOCATOR_FLAGS_H …`) statt `#pragma once`, dann beide Pfade dedupe-fähig.
   - **Schnellster Recovery zum grünen Build:** frisches Build-Dir + voller Bootstrap (2× configure + Codegen-
     ALL-Targets bauen) — ein truly-fresh-dir hat KEINE Altlast, scheitert aber an demselben latenten Bug, daher
     ZUERST Fix (a) anwenden, DANN fresh build.

## Verbleibende TODOs (Stand + Gating)
- **#49-Rest** (YCSB-E/F): via `IScannableTier`-Sub-Interface (s. Lektion) + RMW-Runner. Zipfian-Teil done.
- **#19-Rest** (Allokatoren): mimalloc-Lib BAUT (verifiziert), aber USE_MIMALLOC-Propagation in flags.hpp +
  Wrapper-Linking offen; jemalloc/tcmalloc/hoard/scalloc = keine Windows-Vendor-Build-Kette (extern).
- **#26-Rest** (reale PMC-Werte): Intel-PCM/MSR-Hardware (Beschaffung). Software-Abstraktion + Drop-in fertig.
- **#22** (6 Submodule-Repos): User entschied „befüllen+pushen" — aber **INSIDE-NETZ** (FortiGate; User auf VPN
  pausiert). Echtcode in libs/cache_engine/ extrahieren → 6 modules/-Repos → Push inside.
- **#9** (Naming): User entschied „jetzt anwenden" — Allocator-Goldstandard-Suffix auf axis_12/04/03a/q1+q2/08;
  axis_12-Kollision (general_hardware vs telemetry) auflösen. NICHT begonnen (riskanter Ripple-Refactor).
- **#4** (masstree is_original): GitHub-Source FortiGate-blockiert (inside).
- **#24** Cluster (extern), **#25** Diplomarbeit-Text (User schreibt).

## Sofort-Schritte nächste Session (empfohlen)
1. Build-Recovery: Fix (a) [vendor_includes → axes/alloc/] anwenden, dann fresh build + V5-Test-Suite grün bestätigen.
2. `ext/A05-jemalloc/` untracked entfernen (Experiment-Residuum) oder bewusst behalten.
3. #9 Naming (lokal, User-freigegeben) ODER #49-E/F via IScannableTier (lokal).
4. Inside-Netz: #22 Submodule-Push + #4 masstree-Source + die 55 ungepushten Commits.
