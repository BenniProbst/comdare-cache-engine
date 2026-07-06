// #278/#24 (2026-07-06) — per-User-Temp-Verzeichnis fuer Tests (CI-/tmp-Kollisionsklasse).
// Feste Namen direkt unter temp_directory_path() kollidieren auf Shell-Runnern mit Resten
// FREMDER User (Owner-Mismatch + /tmp-Sticky-Bit): ofstream/remove scheitern dann STILL und
// Tests lesen leere/alte Inhalte (Job 214810: ap10, v5_disk_memento, d1_d2, r5f-Tool, r5g-Emitter).
// Alle Test-Temp-Pfade gehoeren deshalb unter dieses uid-suffixierte Unterverzeichnis.
#pragma once

#include <filesystem>
#include <string>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace comdare::test {

[[nodiscard]] inline std::filesystem::path user_tmp_dir() {
#if defined(_WIN32)
    // Windows: temp_directory_path() ist bereits per-User (%LOCALAPPDATA%\Temp) — keine Kollisionsklasse.
    auto const dir = std::filesystem::temp_directory_path() / "comdare_test";
#else
    auto const dir = std::filesystem::temp_directory_path() / ("comdare_test_" + std::to_string(::getuid()));
#endif
    std::error_code ec;
    std::filesystem::create_directories(dir, ec); // best effort; Fehler zeigt sich beim ersten Schreibzugriff
    return dir;
}

} // namespace comdare::test
