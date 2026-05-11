# 2026-05-12 13:00 — Phase 6 Migration aus Diplomarbeit/code

**Session-Typ:** Code-Migration (Schritt a aus Plan in `20260512-1200-phase6-vorbereitung-rev6-stand-und-plan.md` §6.1-§6.7)
**Quelle:** `C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\code\` (vor Migration angelegt 2026-05-11)
**Ziel:** `C:\Users\benja\OneDrive\Desktop\Projekte\Research\comdare-cache-engine\`

---

## 1. Was migriert wurde

### 1.1 Cache-Engine Concept-Header (8 Dateien)

| Quelle (Diplomarbeit/code) | Ziel (comdare-cache-engine) | Namespace-Refactor |
|----------------------------|------------------------------|---------------------|
| `cache_engine/include/cache_engine/cache_engine.hpp` | `cache_engine/include/cache_engine/cache_engine.hpp` | (Aggregator-Header, keine Namespaces) |
| `.../concepts/cache_recommendation.hpp` | `.../concepts/cache_recommendation.hpp` | `prt_art::cache_engine` → `comdare::cache_engine` |
| `.../concepts/i_cache_engine.hpp` | `.../concepts/i_cache_engine.hpp` | wie oben |
| `.../concepts/i_sub_engine.hpp` | `.../concepts/i_sub_engine.hpp` | wie oben |
| `.../concepts/platform_snapshot.hpp` | `.../concepts/platform_snapshot.hpp` | wie oben |
| `.../concepts/pressure_state.hpp` | `.../concepts/pressure_state.hpp` | `prt_art::cache_engine::state` → `comdare::cache_engine::state` |
| `.../concepts/request_context.hpp` | `.../concepts/request_context.hpp` | wie oben |
| `.../platform_probe/cpuid_probe.hpp` | `.../platform_probe/cpuid_probe.hpp` | `prt_art::cache_engine::platform_probe` → `comdare::cache_engine::platform_probe` |

### 1.2 PRT-ART Concept-Header (11 Dateien)

| Quelle | Ziel | Namespace-Refactor |
|--------|------|---------------------|
| `prt_art/include/prt_art/prt_art.hpp` | `prt_art/include/prt_art/prt_art.hpp` | (Aggregator) |
| `.../concepts/value_handle.hpp` | `.../concepts/value_handle.hpp` | `prt_art` → `comdare::prt_art` |
| `.../concepts/i_node.hpp` | `.../concepts/i_node.hpp` | wie oben |
| `.../concepts/i_root_node.hpp` | `.../concepts/i_root_node.hpp` | wie oben |
| `.../concepts/i_fanout.hpp` | `.../concepts/i_fanout.hpp` | wie oben |
| `.../concepts/i_search_page.hpp` | `.../concepts/i_search_page.hpp` | wie oben |
| `.../concepts/i_search_page_structure.hpp` | `.../concepts/i_search_page_structure.hpp` | wie oben |
| `.../concepts/i_search_page_structure_interpreter.hpp` | `.../concepts/i_search_page_structure_interpreter.hpp` | wie oben |
| `.../concepts/i_cache_page.hpp` | `.../concepts/i_cache_page.hpp` | wie oben |
| `.../concepts/i_executing_engine.hpp` | `.../concepts/i_executing_engine.hpp` | wie oben + `cache_engine::ICacheEngine` → `comdare::cache_engine::ICacheEngine` |
| `.../concepts/i_search_engine.hpp` | `.../concepts/i_search_engine.hpp` | wie oben |

### 1.3 Multi-OS-CMake-Module (4 Dateien)

| Quelle | Ziel | Refactor |
|--------|------|----------|
| `cmake/platform_detection.cmake` | `cmake/platform_detection.cmake` | `PRT_ART_*` → `COMDARE_*` |
| `cmake/compiler_flags.cmake` | `cmake/compiler_flags.cmake` | wie oben |
| `cmake/isa_features.cmake` | `cmake/isa_features.cmake` | wie oben |
| `cmake/gtest_setup.cmake` | `cmake/gtest_setup.cmake` | wie oben |

### 1.4 Tests (6 Dateien)

| Quelle | Ziel |
|--------|------|
| `cache_engine/tests/test_pressure_state.cpp` | `tests/unit/test_pressure_state.cpp` |
| `cache_engine/tests/test_cache_recommendation.cpp` | `tests/unit/test_cache_recommendation.cpp` |
| `cache_engine/tests/test_cpuid_probe.cpp` | `tests/unit/test_cpuid_probe.cpp` |
| `prt_art/tests/test_concepts_compile.cpp` | `tests/unit/test_concepts_compile.cpp` |
| `prt_art/tests/test_value_handle.cpp` | `tests/unit/test_value_handle.cpp` |
| `prt_art/tests/test_three_layer_audit.cpp` | `tests/unit/test_three_layer_audit.cpp` |

**Gesamt:** 8 Cache-Engine-Header + 11 PRT-ART-Header + 4 CMake-Module + 6 Tests = **29 Dateien migriert**, 25 davon Namespace-refaktoriert.

---

## 2. Was offen bleibt (in dieser Migration ausgelassen)

### 2.1 Master-CMakeLists.txt Integration

Die existierende `comdare-cache-engine/CMakeLists.txt` enthaelt aktuell:
```cmake
add_subdirectory(measurement)
add_subdirectory(hardware_isa)
add_subdirectory(search_engine)
add_subdirectory(cache_engine)
add_subdirectory(engine_choice)
add_subdirectory(prt_art)
add_subdirectory(adapters)
add_subdirectory(ext)
```

**FEHLENDE Aenderung (nicht in dieser Migration eingebaut):**

1. Vor allen `add_subdirectory()`-Aufrufen die 4 neuen Module einbinden:
   ```cmake
   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
   include(platform_detection)
   include(compiler_flags)
   include(isa_features)
   if(COMDARE_BUILD_TESTS)
       include(gtest_setup)
   endif()
   ```

2. Sub-Verzeichnis `tests/unit/` integrieren:
   ```cmake
   if(COMDARE_BUILD_TESTS)
       enable_testing()
       add_subdirectory(tests/unit)  # NEU
   endif()
   ```

3. `cache_engine/include/` als INTERFACE-Library exportieren (Sub-CMakeLists noetig).
4. `prt_art/include/` analog.

### 2.2 F-EXTRA-5 Fix

`cmake/permutations.cmake` ruft `find_package(Python3 REQUIRED)` + `tools/permutation_codegen/codegen.py` auf. Muss umgebaut werden zu CMake + sh/bat.

### 2.3 5 fehlende Sub-Engine-Slots

`cache_engine/subsystems/` deckt nur 7/12 Sub-Engines ab. Es fehlen:
- C02_pinning_engine
- C04_coherence_engine
- C08_encoding_engine
- C09_heuristik_engine
- C10_topologie_engine
- C12_filter_engine

Plus 5 bestehende sollten umbenannt werden mit C-Praefix (z.B. `prefetch_controller` -> `c03_prefetch_engine`).

### 2.4 Echte Implementierung

Saemtliche Implementation der 12 Sub-Engines, 6 Seitentypen + PRT-ART-Erweiterung steht aus (Phase 6 Inkrement).

---

## 3. Status der migrierten Dateien

**KOMPILIERBARKEIT:** noch NICHT verifiziert (Master-CMakeLists muss erst ergaenzt werden, siehe §2.1)

**VOLLSTAENDIGKEIT der Migration laut Plan:**
- Schritt 1 (Multi-OS-CMake-Migration): DATEIEN KOPIERT + REFACTORED, Master-Integration offen
- Schritt 2 (Concept-Header-Migration): DATEIEN KOPIERT + REFACTORED, Sub-CMakeLists offen
- Schritt 3 (12 Sub-Engine-Slots): OFFEN
- Schritt 4 (F-EXTRA-5 Fix): OFFEN
- Schritt 5 (Tests-Migration): DATEIEN KOPIERT, CMakeLists-Integration offen
- Schritt 6 (Diplomarbeit/code archivieren): SIEHE §4

---

## 4. Archivierung Diplomarbeit/code

Nach erfolgreicher Migration soll `Diplomarbeit - Datenbanken/code/` archiviert werden als:
`Diplomarbeit - Datenbanken/_archive_code_pre_migration/`

Das wird mit dem naechsten Inkrement umgesetzt (nach Verifikation, dass die Migration kompilierbar ist).

---

## 5. Naechste Aktion

1. Master `comdare-cache-engine/CMakeLists.txt` ergaenzen (siehe §2.1)
2. `cache_engine/include/CMakeLists.txt` + `prt_art/include/CMakeLists.txt` anlegen (INTERFACE-Libraries)
3. `tests/unit/CMakeLists.txt` anlegen
4. F-EXTRA-5 Fix
5. 5 fehlende Sub-Engine-Slots
6. Lokal i7-1270P MSVC Configure-Test
7. Diplomarbeit/code/ archivieren
