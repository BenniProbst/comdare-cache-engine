# Release-Vorbereitung 2026-05-13 (Habich H4, Aufgabe #103)

**Status:** VORBEREITUNG, NICHT AUSGEFUEHRT.
**User-Entscheidung erforderlich** vor `git commit` + `git push`.

---

## §1 Repo-Separation (Stand prüfen)

| Repo | Origin | Status |
|---|---|---|
| comdare-cache-engine | `https://github.com/BenniProbst/comdare-cache-engine.git` | 70 Files changed/untracked |
| comdare-prt-art     | `https://github.com/BenniProbst/comdare-prt-art.git`     | 3 Files changed/untracked |

Beide Repos sind bereits **getrennt** (kein Monorepo). Eine weitere
Separation ist NICHT erforderlich.

---

## §2 Aenderungen comdare-cache-engine (heute)

### §2.1 Komplett neue Module (untracked, 31 Dateien)

#### Subdirectories
- `succinct/` — eigene SDSL-Lite-Portierung (#105)
  - `succinct/CMakeLists.txt`
  - `succinct/include/comdare/succinct/bit_vector.hpp`
  - `succinct/include/comdare/succinct/louds.hpp`
- `tools/CMakeLists.txt` — Tools-Aggregator
- `tools/ycsb_cli/` — YCSB-Generator-CLI (#190)
- `tools/latex_anhang/` — Phase 8 LaTeX-Tabellen-Tool (#88)
- `tools/latex_toolchain/` — Habich H3 Diagram+LaTeX-Pipeline (#102)
- `cache_engine/include/cache_engine/hbm/` — HBM-Hybrid-Hierarchie (#106)
- `cache_engine/reclamation/rcu_reclaim/rcu.hpp` — eigene RCU (#104)
- `cache_engine/builder/module_loader/module_loader.{hpp,cpp}` — DLL-Loader (#195)
- `docs/quality_audit/HABICH_H2_CODE_QUALITY_2026_05_13.md` — H2-Audit (#101)

#### Tests (16 Dateien)
- `tests/unit/test_ycsb_cli.cpp`
- `tests/unit/test_builder_codegen.cpp`
- `tests/unit/test_latex_anhang.cpp`
- `tests/unit/test_rcu.cpp`
- `tests/unit/test_succinct.cpp`
- `tests/unit/test_hbm_hierarchy.cpp`
- `tests/unit/test_module_loader.cpp`
- `tests/unit/mock_permutation_module.cpp`
- `prt_art/legacy_reimpl/P{11,12,13,14,16,17,18,19,21,22,23,24,26,27}/tests/test_*.cpp`

### §2.2 Modifiziert (39 Dateien)

- `CMakeLists.txt` (top-level) — add_subdirectory(succinct, tools)
- `cache_engine/builder/main.cpp` — Phase 4-7 wire-up
- `cache_engine/builder/codegen/{codegen.hpp,codegen.cpp}` — Aggregator + dllexport
- `cache_engine/builder/CMakeLists.txt` — link module_loader + workload_generator + experiment
- `cache_engine/builder/module_loader/CMakeLists.txt` — STATIC lib mit dl
- `cache_engine/reclamation/CMakeLists.txt` — add_subdirectory(rcu_reclaim)
- `cache_engine/reclamation/rcu_reclaim/CMakeLists.txt` — INTERFACE-Lib
- `prt_art/CMakeLists.txt` — add_subdirectory(legacy_reimpl)
- `prt_art/legacy_reimpl/P{11..27}/include/*.hpp` — 14 LEGACY_REIMPL ausgefuellt (#86)
- `prt_art/legacy_reimpl/P{11..27}/CMakeLists.txt` — INTERFACE-Libs + Tests
- `tests/unit/CMakeLists.txt` — alle neuen Tests registriert

---

## §3 Aenderungen comdare-prt-art (heute)

| Datei | Status | Inhalt |
|---|---|---|
| `prt_art/include/prt_art/identity/status.hpp`             | UNTRACKED  | errno-style Status-Codes (REV 7.1) |
| `prt_art/include/prt_art/identity/prt_art_search_engine.hpp` | MODIFIED  | Hybride PrtArtSearchEngine (Vector-/Map-API) |
| `tests/unit/test_prt_art_identity.cpp`                    | MODIFIED  | 51 neue Tests (Map/Tuple/Vector/Status) |

---

## §4 Vorgeschlagene Commit-Gruppen

### comdare-cache-engine (8 vorgeschlagene Commits)

```
1. Phase 7.2.A-C: cmake-Pipeline (Aggregator + Subbuild + 7 Codegen-Tests)
   Files:
     - cache_engine/builder/codegen/{codegen.hpp,codegen.cpp}
     - cache_engine/builder/main.cpp (Phase 1-3)
     - prt_art/CMakeLists.txt
     - tests/unit/test_builder_codegen.cpp

2. Phase 7.2.D-F: ModuleLoader + Phase 4-7 + 8 Loader-Tests
   Files:
     - cache_engine/builder/module_loader/{module_loader.hpp,module_loader.cpp,CMakeLists.txt}
     - cache_engine/builder/main.cpp (Phase 4-7)
     - cache_engine/builder/CMakeLists.txt
     - cache_engine/builder/codegen/codegen.cpp (dllexport-fix)
     - tests/unit/{test_module_loader.cpp,mock_permutation_module.cpp}

3. Step B: 14 LEGACY_REIMPL Skelette ausgefuellt
   Files:
     - prt_art/legacy_reimpl/P{11..27}/include/*.hpp
     - prt_art/legacy_reimpl/P{11..27}/tests/test_*.cpp
     - prt_art/legacy_reimpl/P{11..27}/CMakeLists.txt

4. Step C: YCSB-Generator-CLI (#190 + #191)
   Files:
     - tools/CMakeLists.txt
     - tools/ycsb_cli/
     - tests/unit/test_ycsb_cli.cpp

5. Phase 8: LaTeX-Anhang-Tool (#88)
   Files:
     - tools/latex_anhang/
     - tests/unit/test_latex_anhang.cpp

6. Aufgabe #104: eigene RCU-Implementation
   Files:
     - cache_engine/reclamation/rcu_reclaim/rcu.hpp
     - cache_engine/reclamation/{rcu_reclaim/,*}/CMakeLists.txt
     - tests/unit/test_rcu.cpp

7. Aufgabe #105+#106: SDSL-Lite-Portierung + HBM-Hybrid-Hierarchie
   Files:
     - succinct/
     - cache_engine/include/cache_engine/hbm/
     - CMakeLists.txt (top-level)
     - tests/unit/{test_succinct.cpp,test_hbm_hierarchy.cpp}

8. Habich H2+H3: Code-Quality-Audit + LaTeX-Toolchain (#101+#102)
   Files:
     - docs/quality_audit/HABICH_H2_CODE_QUALITY_2026_05_13.md
     - tools/latex_toolchain/
     - RELEASE_PREP_2026_05_13.md
```

### comdare-prt-art (1 Commit)

```
1. Step A: Hybride PrtArtSearchEngine (Vector-/Map-/Tuple-API) + 51 Tests
   Files:
     - prt_art/include/prt_art/identity/status.hpp
     - prt_art/include/prt_art/identity/prt_art_search_engine.hpp
     - tests/unit/test_prt_art_identity.cpp
```

---

## §5 Push-Anleitung (NACH User-Bestaetigung)

### comdare-cache-engine

```bash
cd /path/to/comdare-cache-engine
# Pro Commit-Gruppe oben:
git add <list of files>
git commit -m "<message>"
# ...
git push origin main
```

### comdare-prt-art

```bash
cd /path/to/comdare-prt-art
git add prt_art/include/prt_art/identity/status.hpp \
        prt_art/include/prt_art/identity/prt_art_search_engine.hpp \
        tests/unit/test_prt_art_identity.cpp
git commit -m "Hybride PrtArtSearchEngine: Vector-/Map-/Tuple-API + 51 Tests (REV 7.1)"
git push origin main

# Submodule-Pin in comdare-cache-engine auf neues prt-art commit aktualisieren
```

---

## §6 Verifikations-Status (vor Push)

Alle 140 neue Tests heute pass:

| Test-Suite | Anzahl | Status |
|---|---|---|
| Hybride PrtArtSearchEngine (comdare-prt-art) | 51 | gruen |
| 14 LEGACY_REIMPL Skelette                    | 60 | gruen |
| YCSB-CLI                                      | 14 | gruen |
| LaTeX-Anhang                                  |  9 | gruen |
| RCU                                            |  8 | gruen |
| Succinct (BitVector + LOUDS)                   | 15 | gruen |
| HBM-Hierarchy                                  | 12 | gruen |
| Codegen-Aggregator                             |  7 | gruen |
| ModuleLoader                                   |  8 | gruen |
| **Summe**                                      | **184** | **alle gruen** |

E2E-Pipeline (Cache-Engine):
- 54 Permutations-DLLs gebaut
- 54/54 Module geladen
- 500 ops × 54 modules executed
- measurements.csv + JSON exportiert
- 666-byte YCSB-binary verifiziert
- 54-Zeilen-LaTeX-Tabelle generiert

---

## §7 Delta-Doku unter Diplomarbeit

Im Diplomarbeits-Verzeichnis wurden 5 Delta-Dokumente erstellt:

- `25_architektur_delta_hybrid_search_engine_2026_05_13.md`
- `26_architektur_delta_legacy_reimpl_2026_05_13.md`
- `27_architektur_delta_ycsb_cli_2026_05_13.md`
- `28_architektur_delta_cmake_pipeline_2026_05_13.md`
- `29_architektur_delta_phase4_7_loader_2026_05_13.md`

Diese sind im Diplomarbeits-Repo (anderer Speicherort), NICHT in einem der
beiden Code-Repos.

---

## §8 Empfohlene naechste Schritte (User-Decision)

1. **User-Review** der RELEASE_PREP_2026_05_13.md
2. **User-Bestaetigung** zu Push (oder Aenderungen an Commit-Gruppierung)
3. NACH OK: 8 Commits + Push fuer comdare-cache-engine
4. NACH OK: 1 Commit + Push fuer comdare-prt-art
5. Submodule-Pin in comdare-cache-engine auf neues prt-art-commit bumpen
6. Optional: Tag `v0.2.0-phase7-complete` auf beide Repos setzen
