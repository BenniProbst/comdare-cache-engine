#!/bin/sh
# build_thesis.sh — Habich H3 Toolchain-Frontend (POSIX)
# Aufruf: ./build_thesis.sh [--source=DIR] [--output=DIR] [--engine=pdflatex|lualatex|xelatex]

set -e

SOURCE_DIR="../../docs/thesis"
OUTPUT_DIR="build/thesis-build"
LATEX_ENGINE="pdflatex"
MAIN_TEX="main.tex"

for arg in "$@"; do
    case "$arg" in
        --source=*) SOURCE_DIR="${arg#--source=}" ;;
        --output=*) OUTPUT_DIR="${arg#--output=}" ;;
        --engine=*) LATEX_ENGINE="${arg#--engine=}" ;;
        --main=*)   MAIN_TEX="${arg#--main=}" ;;
        --help|-h)
            echo "Usage: $0 [--source=DIR] [--output=DIR] [--engine=pdflatex|lualatex|xelatex] [--main=FILE]"
            exit 0 ;;
        *)
            echo "Unknown arg: $arg" >&2
            exit 4 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

cmake \
    -DCOMDARE_THESIS_SOURCE="$SOURCE_DIR" \
    -DCOMDARE_THESIS_OUTPUT="$OUTPUT_DIR" \
    -DCOMDARE_LATEX_ENGINE="$LATEX_ENGINE" \
    -DCOMDARE_THESIS_MAIN_TEX="$MAIN_TEX" \
    -P "$SCRIPT_DIR/latex_toolchain.cmake"
