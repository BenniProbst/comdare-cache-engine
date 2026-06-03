#!/usr/bin/env bash
# L-CLUSTER (gate-frei-TEXT) — baut den comdare-ce.sif Apptainer-Container aus comdare-ce.def. Bindet den Repo-
# Quellbaum als /src ein; der %post-Block baut perm_runner (gcc-13 C++23, Boost.MP11 vendored offline).
#
# ⚠️ AUSFÜHRUNG IST GATE-MAXIMAL: `apptainer build` läuft auf einer ZIH-Login-Node mit User-Freigabe
# (ZIH-Nutzungsbedingungen; Strafen/Account-Sperre möglich — CLAUDE.md „Kritische Manöver"). NICHT lokal/autonom
# ausführen. Dieses Skript ist der reproduzierbare Bau-BEFEHL (Text); die reale Ausführung erfolgt erst nach Freigabe.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${1:-$(cd "${HERE}/.." && pwd)}"   # Default: das cache-engine-Repo (deploy/ liegt eine Ebene darunter)
OUT="${2:-comdare-ce.sif}"

echo "[build_sif] REPO=${REPO}"
echo "[build_sif] OUT =${OUT}"
echo "[build_sif] def =${HERE}/comdare-ce.def"

if ! command -v apptainer >/dev/null 2>&1; then
    echo "[build_sif] FEHLER: apptainer nicht gefunden — auf der ZIH-Login-Node ausfuehren (module load apptainer)." >&2
    exit 3
fi

# --bind bringt den Quellbaum als /src in den %post-Build-Kontext (der %post-cmake liest -S /src).
apptainer build --bind "${REPO}:/src" "${OUT}" "${HERE}/comdare-ce.def"
echo "[build_sif] fertig: ${OUT}  (Smoke-Test: apptainer test ${OUT})"
