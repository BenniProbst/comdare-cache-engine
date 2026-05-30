# Doku 23a — F.2 Pilot-Migrations-Spec: `io/axis_io` → `io_dispatch` (ausführbar, build-sicher)

**Stand:** 2026-05-31 · **Quelle:** Planrunde (Design-Agent, code-verifiziert) · **Bezug:** Doku 23 §2 Stufe 2.
**Zweck:** exakter, mechanisch ausführbarer Pilot der physischen Achsen-Migration; Muster für die übrigen 16 Achsen.

## Strategie (risikoärmste Variante)
Move + **Forwarding-Header am alten Pfad** + **Rückwärts-Namespace-Alias** = 0 externe Pflicht-Edits, jederzeit grüner Build. Header ziehen nach `libs/cache_engine/axes/io_dispatch/`, Definition-Namespace `io::axis_io` → `io_dispatch`. Die 18 externen `#include <topics/io/axis_io/...>` + alle `io::axis_io::`-Nutzungen (App + 11 Compositions + 5 Tests) bleiben über Forwarder + Alias gültig. Referenz-Migration + Forwarder-Entfernung = Stufe 3 (separat).

## Verifizierte Faktenlage
- **Include-Root:** kein separates `-I .../axes`; jedes Ziel hat `libs/cache_engine` als Root → `<axes/io_dispatch/...>` sofort auflösbar (Beleg: `test_v41_search_algorithm_permutation_engine.cpp:31` inkludiert `<axes/axis_centric_namespaces.hpp>`).
- **Generierte Flags:** `configure_file` in Root-`CMakeLists.txt` **Z. 879–882**: INPUT `…/topics/io/axis_io/axis_io_flags.hpp.in` → OUTPUT `…/generated/topics/io/axis_io/axis_io_flags.hpp`. Consumer `axis_io_registry.hpp:4` inkludiert `<topics/io/axis_io/axis_io_flags.hpp>`.
- **Umbrella-Glob-Bruch:** `COMDARE_ALL_AXIS_GENERATED_DIRS` (tests/unit/CMakeLists.txt:11–21) globt `topics/*/axis_*` aus dem SOURCE-Tree. Move aus `topics/` → io fällt aus der Liste → `test_v41_codegen_all_axes_umbrella`/compositions brechen, WENN nicht 5c repariert.
- **Topic-Concept:** `topics/io/concepts/topic_io_concept.hpp` bleibt am Ort (topic-, nicht achsen-spezifisch); relativer Pfad `../concepts/` (4×) + `../../concepts/` (1×) → stabile Include-Dir-Form `<topics/io/concepts/topic_io_concept.hpp>`.

## Schritte
1. **Rollback-Tag:** `git tag v41-f2-io_dispatch-pre-migration`.
2. **git mv** der 9 Header + `axis_io_flags.hpp.in` + `PAPER_REFERENCES.md` nach `axes/io_dispatch/` (concepts-Unterordner mit). `topic_io_concept.hpp` + `topic_io_config_set.hpp` bleiben in `topics/io/`.
3. **Edits in verschobenen Headern:** (3a) Definition-Namespace `comdare::cache_engine::io::axis_io` → `comdare::cache_engine::io_dispatch` (alle 9 + Template; `using topic_tag = …io::concepts::IoTopicTag;` BLEIBT). (3b) 5× Topic-Concept-Include auf `<topics/io/concepts/topic_io_concept.hpp>`. (3c) Flags-Include `axis_io_registry.hpp:4` + die 4 Strategie-Header auf `<axes/io_dispatch/axis_io_flags.hpp>`.
4. **Forwarding-Header** am alten Pfad (9 Dateien): `#pragma once` + `#include <axes/io_dispatch/<same>.hpp>`.
5. **CMake:** (5a) configure_file Z. 879–882 INPUT+OUTPUT auf `axes/io_dispatch`. (5b) io-Test generated-Dir (tests/unit/CMakeLists.txt:795) auf `generated/axes/io_dispatch`. (5c) **nach Z. 21** `list(APPEND COMDARE_ALL_AXIS_GENERATED_DIRS "${CMAKE_BINARY_DIR}/generated/axes/io_dispatch")`.
6. **Rückwärts-Alias** in `axis_centric_namespaces.hpp`: Z. 84 (`namespace io_dispatch = …io::axis_io;`) ENTFERNEN; auf Datei-Top-Level (außerhalb `namespace comdare::cache_engine{}`) einfügen:
   ```cpp
   namespace comdare::cache_engine::io::axis_io { using namespace comdare::cache_engine::io_dispatch; }
   ```
7. **Verifikation:** `cmake -S . -B build` (reconfigure!) → build+ctest Targets `test_v41_axis_io`, `test_v41_search_algorithm_permutation_engine`, `test_v41_codegen_all_axes_umbrella`, `test_v41_compositions` + `comdare_adhoc_emitter_cli`.

## Top-Risiken
1. **Glob-Bruch (5c vergessen)** → C1083 `axis_io_flags.hpp` in umbrella/compositions. 2. **Flags-Include-Pfad ↔ configure_file-OUTPUT** müssen exakt matchen. 3. **Alias NICHT** in `namespace comdare::cache_engine{}` verschachteln (sonst `comdare::cache_engine::comdare::cache_engine`). 4. `axis_centric_namespaces.hpp` ist auch die F.3-Concept-Facade — Tippfehler bricht ALLE Achsen-Concepts.

## Rollback
`git reset --hard v41-f2-io_dispatch-pre-migration && cmake -S . -B build`.

## Muster für die übrigen 16 Achsen (Doku 23 §3 Map)
Identisches Schema je Achse: git mv → axes/<axis>/, Namespace-Rename, Topic-Concept-Pfadfix, Forwarder, configure_file+Glob-APPEND, Rückwärts-Alias, Verifikation. Reihenfolge nach Größe (klein zuerst: io/migration/filter, dann mittlere, zuletzt traversal/axis_03a).
