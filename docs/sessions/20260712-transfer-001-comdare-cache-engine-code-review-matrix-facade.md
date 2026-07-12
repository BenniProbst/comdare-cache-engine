# CR-MF-T-001 — comdare-cache-engine: Code-Review Matrix/Fassade

- Review-ID: `CR-MF-T-001`; finaler Analyse-Stand `origin/development@7d50bba1e41e`; acht vorbestehende Dirty-Einträge unverändert erhalten.
- Re-Review statisch/read-only; keine Builds, Tests oder Codefixes ausgeführt.

## Quellen/Dependencies

- 20 neueste relevante Session-Dokumente aus `docs/sessions` gelesen; Leitquellen darunter `docs/sessions/goal-v6-luecken-ledger.md`, `docs/sessions/GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md`, `docs/sessions/GOAL-AUTONOM-ABARBEITUNG-20260613.md`.
- BuildSystem: Im Repo-Root existiert kein `buildsystem.xml`; damit fehlt die erste Stufe des geforderten XML→CMake→C++-Vertrags.
- CMake: reales Projekt bei `CMakeLists.txt:8`; Domänen werden bei `CMakeLists.txt:587` bis `CMakeLists.txt:603` eingebunden, Tools/Apps bei `CMakeLists.txt:623` und `CMakeLists.txt:624`.
- Weiche Abhängigkeitspfade: PAPI/PMC-Warnings bei `CMakeLists.txt:67` bis `CMakeLists.txt:88`, übersprungener Plugin-Prüfling bei `CMakeLists.txt:616`, konditionale Codegen-Targets ab `CMakeLists.txt:1487`.
- C++/Tests: generierter Permutationskonsum bei `tests/unit/auto_emitted_perm_module.cpp:11`; Matrix-Achsen bei `tests/unit/br4_emit.cpp:12` bis `tests/unit/br4_emit.cpp:20`.
- Ungeprüft: `build*`, `cmake-build*`, `ext`, Vendor-/Cache-/Third-Party- und generierte Bäume, Dirty-IDE-Dateien sowie Runtime/Hardwaremessung.

## Findings/TODOs

- P0: kein sicher belegter P0.
- P1: Ohne `buildsystem.xml` ist die generische Modulidentität samt Abhängigkeiten, Versionen, Phasen und Resolver-Provenienz nicht maschinenlesbar; der CMake-/C++-Graph kann nicht gegen einen BuildSystem-Vertrag geprüft werden.
- P1: Require-vs.-optional ist im CMake-Graph weich: fehlende Hardware-Counter und Plugin-Prüflinge führen zu Warning/Skip (`CMakeLists.txt:67`, `CMakeLists.txt:616`) statt zu einem explizit typisierten Capability-Vertrag.
- P1: Konditionale Codegen-Verkettung (`CMakeLists.txt:1487`) kann Matrix-Zellen abhängig von bereits vorhandenen Targets unterschiedlich instanziieren; ein reproduzierbarer Target-/SHA-Lock fehlt.
- P2: Skillgraph-Aggregat 202 Knoten/479 Kanten, 15 unresolved, 20 Duplicate-Klassen, 33 Missing Sessions; CacheEngine-Achsen/Builder als typisierte Template-Knoten ergänzen.

## Matrix/Fassade / Owner-Child

- Das Repo ist generische Module-Erbmasse: Template-/Permutationslogik darf intern bleiben. Ein Product konsumiert sie ausschließlich über eine versionsspezifische Product-Fassade und kompiliert diese zu Library/Binary; Product n→n−1, kein Template im externen ABI.
- Fehlende spezialisierte CacheEngine-Product-Baselines sind Ecosystem-/Consumer-Mapping-Gaps, nicht automatisch ein Defekt dieses Moduls. Erforderlich sind Zielpfad, Quell-Repo+SHA+Datum, Achsenvertrag und Instanziierungsbefehl.
- Physische `ext`-/generierte Children sind aus dem Review ausgeschlossen und erhalten keine erfundenen Sessions.

## Re-Review e5946cdb6eac..7d50bba1e41e

- Gezielter Scope: acht geänderte Pfade, davon drei CMake-/C++-Signale. Produktives Root-CMake, BuildSystem-Manifest, öffentliche Header und Product-/Research-Fassaden blieben im Diff unverändert.
- C++: `tests/unit/genus_buildvariant_real_avx2.cpp:19` und `tests/unit/genus_buildvariant_real_avx512.cpp:20` ergänzen ausschließlich eine `cppcheck-suppress unknownMacro`-Direktive vor dem bestehenden CHECKED-Makro. Kein Include, Target oder Matrixvertrag änderte sich.
- CMake/Testkante: `tests/unit/perm_codegen_byte_identity.cmake:1` führt einen neuen Testtreiber ein; `tests/unit/perm_codegen_byte_identity.cmake:38` startet das CMake-Backend, `tests/unit/perm_codegen_byte_identity.cmake:67` das opt-in C++-CLI-Backend und `tests/unit/perm_codegen_byte_identity.cmake:80` vergleicht deren Artefakte bytegenau.
- Finding/Impact: Der neue Treiber stärkt den Nachweis, dass CMake- und C++-Codegen dieselben Permutationsartefakte liefern. Er schließt nicht die bestehenden P1-Gaps für fehlendes `buildsystem.xml`, Warning-/Skip-Capabilities oder einen gepinnten Product-Fassaden-Lock.
- TODO: Testtreiber in der tatsächlichen CI-/CTest-Kante hart registrieren und die generische Codegen-Library weiterhin ausschließlich über eine versionsfixierte Product/Research-Fassade binär instanziieren.
- Tests nicht ausgeführt (read-only; statischer Re-Review).

## Tests/Schritt 2

- Tests nicht ausgeführt (read-only).
- Akzeptanz: vollständiges BuildSystem-Manifest; XML-, CMake- und C++-Kanten deckungsgleich; Capabilities explizit required/optional mit hartem Verhalten; Codegen aus fixiertem Lock; generische Library standalone und über eine echte Product-Fassade/Aggregat grün; Dirty-Bestand unangetastet.
