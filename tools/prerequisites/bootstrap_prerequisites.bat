@echo off
REM tools\prerequisites\bootstrap_prerequisites.bat
REM V41.F.6.1 (2026-05-29) - Prerequisite-Bootstrap (Windows).
REM
REM Stellt die Repo-Prerequisites bereit, die bei der Einrichtung sonst fehlen wuerden - derzeit
REM Boost.MP11 (header-only). Wird AUTONOM auch von boost_mp11_setup.cmake im configure-Schritt
REM erledigt; dieses Skript ist der explizite, eigenstaendige Einrichtungs-Pfad.
REM
REM KEIN Python (Memory-Direktive) - reine cmd + cmake-Aufrufe. CMake file(ARCHIVE_EXTRACT)
REM behandelt .7z / .zip / .tar.bz2 ueber libarchive.
REM
REM Verwendung:
REM   tools\prerequisites\bootstrap_prerequisites.bat [pfad\zu\boost_archiv]
REM Archiv-Reihenfolge: %1 > %COMDARE_BOOST_ARCHIVE% > prerequisites\boost_*.{tar.bz2,tar.gz,tar.xz,7z,zip}
setlocal
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..\..") do set "REPO_ROOT=%%~fI"

set "ARCHIVE=%~1"
if "%ARCHIVE%"=="" set "ARCHIVE=%COMDARE_BOOST_ARCHIVE%"

echo [prereq] Repo: %REPO_ROOT%
if not "%ARCHIVE%"=="" (
    echo [prereq] Boost-Archiv: %ARCHIVE%
    cmake -DCOMDARE_BOOST_ARCHIVE="%ARCHIVE%" -P "%REPO_ROOT%\cmake\provision_mp11.cmake" || exit /b 1
) else (
    echo [prereq] kein Archiv-Argument - nutze prerequisites\-Glob ^(falls vorhanden^)
    cmake -P "%REPO_ROOT%\cmake\provision_mp11.cmake" || exit /b 1
)

echo [prereq] fertig. Boost.MP11 ist nun vendored unter cmake\third_party\boost_mp11\include\
endlocal
