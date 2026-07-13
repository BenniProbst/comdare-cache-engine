@echo off
REM build_thesis.bat — Habich H3 Toolchain-Frontend (Windows)
REM Aufruf: build_thesis.bat [--source=DIR] [--output=DIR] [--engine=pdflatex|lualatex|xelatex]

setlocal EnableDelayedExpansion

set SOURCE_DIR=..\..\docs\thesis
set OUTPUT_DIR=build\thesis-build
set LATEX_ENGINE=pdflatex
set MAIN_TEX=main.tex

:parse_args
if "%~1"=="" goto run
set "ARG=%~1"

REM --- Attached form: cmd kept the whole "--key=value" as ONE token.
REM     Occurs when the caller QUOTES the argument, e.g. "--source=C:\a b".
if "!ARG:~0,9!"=="--source=" ( set "SOURCE_DIR=!ARG:~9!"   & shift & goto parse_args )
if "!ARG:~0,9!"=="--output=" ( set "OUTPUT_DIR=!ARG:~9!"   & shift & goto parse_args )
if "!ARG:~0,9!"=="--engine=" ( set "LATEX_ENGINE=!ARG:~9!" & shift & goto parse_args )
if "!ARG:~0,7!"=="--main="   ( set "MAIN_TEX=!ARG:~7!"     & shift & goto parse_args )

REM --- Split form (the common UNQUOTED case): cmd.exe treats '=' as an
REM     argument delimiter, so an unquoted "--key=value" arrives as TWO
REM     tokens (%1="--key", %2="value"). Read the value from %2 and consume
REM     BOTH tokens via a double shift. Previously this fell through the
REM     matcher, so the flag was silently ignored and the build reverted to
REM     the default paths (finding M-CE-20).
if "!ARG!"=="--source" ( set "SOURCE_DIR=%~2"   & shift & shift & goto parse_args )
if "!ARG!"=="--output" ( set "OUTPUT_DIR=%~2"   & shift & shift & goto parse_args )
if "!ARG!"=="--engine" ( set "LATEX_ENGINE=%~2" & shift & shift & goto parse_args )
if "!ARG!"=="--main"   ( set "MAIN_TEX=%~2"     & shift & shift & goto parse_args )

if "!ARG!"=="--help" goto help
if "!ARG!"=="-h"     goto help

REM Unknown argument: fail loudly instead of silently building with defaults
REM (mirrors build_thesis.sh, which exits 4 on an unrecognised argument).
echo Unknown arg: %~1 1>&2
exit /b 4

:help
echo Usage: build_thesis.bat [--source=DIR] [--output=DIR] [--engine=pdflatex^|lualatex^|xelatex] [--main=FILE]
exit /b 0

:run
set SCRIPT_DIR=%~dp0
cmake ^
    -DCOMDARE_THESIS_SOURCE=%SOURCE_DIR% ^
    -DCOMDARE_THESIS_OUTPUT=%OUTPUT_DIR% ^
    -DCOMDARE_LATEX_ENGINE=%LATEX_ENGINE% ^
    -DCOMDARE_THESIS_MAIN_TEX=%MAIN_TEX% ^
    -P "%SCRIPT_DIR%latex_toolchain.cmake"
exit /b %ERRORLEVEL%
