// #278/#24 (2026-07-06) — per-User-Temp-Verzeichnis fuer Tests (CI-/tmp-Kollisionsklasse).
// Feste Namen direkt unter temp_directory_path() kollidieren auf Shell-Runnern mit Resten
// FREMDER User (Owner-Mismatch + /tmp-Sticky-Bit): ofstream/remove scheitern dann STILL und
// Tests lesen leere/alte Inhalte (Job 214810: ap10, v5_disk_memento, d1_d2, r5f-Tool, r5g-Emitter).
// Alle Test-Temp-Pfade gehoeren deshalb unter dieses uid-suffixierte Unterverzeichnis.
//
// HAERTUNG Posten 69 (2026-08-04, Ledger-Nachtrag abend-10) -- ZWEITE Kollisionsklasse:
// die uid allein trennt FREMDE User, nicht die eigenen parallelen Worktrees. Mehrere Worktrees
// DESSELBEN Users, deren ctest-Laeufe gleichzeitig fahren, teilten sich bisher eine Wurzel
// (Ist-Erhebung: 15683 Eintraege) und ueberschrieben/loeschten einander die Fixture-Dateien --
// Flake-Klasse ~27% an test_s5_artifact_cache_bounded/test_cache_mc_timeout. Die Wurzel traegt
// deshalb zusaetzlich eine stabile BUILD-VERZEICHNIS-Kennung.
//
// WOHER DIE KENNUNG KOMMT: aus CMake, nicht aus dem Laufzeit-Prozess. tests/unit/CMakeLists.txt
// bildet sie als MD5-Praefix ueber CMAKE_BINARY_DIR und reicht sie als Compile-Definition
// COMDARE_TEST_TMP_BUILD_TAG herein. Damit ist sie DETERMINISTISCH je Build-Verzeichnis (kein
// Zeitstempel, keine Zufallszahl: zwei Laeufe aus demselben Build-Verzeichnis finden dieselbe
// Wurzel wieder, ein Aufraeumen bleibt moeglich) und zwangslaeufig VERSCHIEDEN fuer zwei
// Worktrees mit eigenen Build-Verzeichnissen. __FILE__ taugte dafuer nicht: es ist je Worktree
// gleich, sobald zwei Worktrees denselben Quellbaum-Pfad-Suffix haben, und es unterscheidet
// mehrere Build-Verzeichnisse desselben Worktrees gar nicht.
//
// FALLBACK: uebersetzt eine TU diesen Header ohne die Definition (z.B. ein Ziel ausserhalb
// tests/unit), bleibt exakt das alte, rein uid-suffixierte Verhalten stehen -- der Header bleibt
// eigenstaendig uebersetzbar und niemand faellt auf einen leeren Pfadteil herein.
#pragma once

#include <filesystem>
#include <string>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace comdare::test {

/// Die Build-Verzeichnis-Kennung der Test-Temp-Wurzel (Posten 69). Leer = ohne Compile-Definition
/// uebersetzt; dann bleibt die Wurzel exakt der uid-Stand von #278/#24.
[[nodiscard]] inline std::string tmp_build_tag() {
#if defined(COMDARE_TEST_TMP_BUILD_TAG)
    return std::string{"_"} + COMDARE_TEST_TMP_BUILD_TAG;
#else
    return std::string{};
#endif
}

[[nodiscard]] inline std::filesystem::path user_tmp_dir() {
#if defined(_WIN32)
    // Windows: temp_directory_path() ist bereits per-User (%LOCALAPPDATA%\Temp) -- keine User-Kollisionsklasse.
    // Die Worktree-Kollisionsklasse (Posten 69) besteht dort aber genauso -> Kennung wird auch hier angehaengt.
    auto const dir = std::filesystem::temp_directory_path() / ("comdare_test" + tmp_build_tag());
#else
    auto const dir =
        std::filesystem::temp_directory_path() / ("comdare_test_" + std::to_string(::getuid()) + tmp_build_tag());
#endif
    std::error_code ec;
    std::filesystem::create_directories(dir, ec); // best effort; Fehler zeigt sich beim ersten Schreibzugriff
    return dir;
}

} // namespace comdare::test
