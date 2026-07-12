@echo off
REM tools\permutation_codegen\codegen.bat — Windows-Pendant zu codegen.cmake (F-EXTRA-5 NO-PYTHON)
REM
REM DEPRECATED (#25 B, 2026-07-12): Dieses Skelett-Skript (nur Platzhalter-Output, KEINE echten
REM   Permutationen) ist durch das C++23-Tool abgeloest:
REM     apps\permutation_codegen_tool\ -> comdare_permutation_codegen_cli (Backend 'cpp').
REM   Das C++-Tool erzeugt eine BYTE-IDENTISCHE permutations.cmake zum Default-Backend 'cmake'
REM   (codegen.cmake). Neue Belange bitte dort implementieren, nicht hier.
REM   Wird bewusst NICHT geloescht (Doku-nie-loeschen); als bat-Backend weiter aufrufbar.
REM
REM Aufruf:  set COMDARE_TARGET_ISA=auto && set COMDARE_OUTPUT=<path> && codegen.bat
REM oder:    codegen.bat --target-isa=auto --output=<path>
REM
REM Synchronisations-Pflicht: identisch zu codegen.cmake und codegen.sh.

setlocal enabledelayedexpansion

if not defined COMDARE_TARGET_ISA set "COMDARE_TARGET_ISA=auto"
if not defined COMDARE_OUTPUT set "COMDARE_OUTPUT="

:parse
if "%~1"=="" goto check
set "_arg=%~1"
if "!_arg:~0,14!"=="--target-isa=" (
    set "COMDARE_TARGET_ISA=!_arg:~14!"
    shift & goto parse
)
if "!_arg:~0,9!"=="--output=" (
    set "COMDARE_OUTPUT=!_arg:~9!"
    shift & goto parse
)
if /i "%~1"=="--target-isa" (
    set "COMDARE_TARGET_ISA=%~2"
    shift & shift & goto parse
)
if /i "%~1"=="--output" (
    set "COMDARE_OUTPUT=%~2"
    shift & shift & goto parse
)
echo Unbekanntes Argument: %~1 1>&2
exit /b 2

:check
if "%COMDARE_OUTPUT%"=="" (
    echo FEHLER: COMDARE_OUTPUT erforderlich. 1>&2
    exit /b 2
)

for %%I in ("%COMDARE_OUTPUT%") do set "_out_dir=%%~dpI"
if not exist "%_out_dir%" mkdir "%_out_dir%" >nul 2>&1

> "%COMDARE_OUTPUT%" (
    echo # permutations.cmake — generiert von tools\permutation_codegen\codegen.bat
    echo # ^(Phase 4.B Skelett — keine Permutationen, kommt in Phase 5+^)
    echo # Target-ISA: %COMDARE_TARGET_ISA%
    echo.
    echo message^(STATUS "Permutations-Codegen-Skelett ^(keine Permutationen registriert; ISA=%COMDARE_TARGET_ISA%^)"^)
)

echo OK ^(Skelett^): %COMDARE_OUTPUT% geschrieben.
endlocal
