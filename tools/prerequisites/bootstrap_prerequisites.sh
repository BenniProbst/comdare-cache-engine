#!/usr/bin/env sh
# tools/prerequisites/bootstrap_prerequisites.sh
# V41.F.6.1 (2026-05-29) — Prerequisite-Bootstrap (Linux/macOS/Git-Bash).
#
# Stellt die Repo-Prerequisites bereit, die bei der Einrichtung sonst fehlen wuerden — derzeit
# Boost.MP11 (header-only). Wird AUTONOM auch von boost_mp11_setup.cmake im configure-Schritt
# erledigt; dieses Skript ist der explizite, eigenstaendige Einrichtungs-Pfad (z.B. fuer CI).
#
# KEIN Python (Memory-Direktive) — reine sh + cmake-Aufrufe.
#
# Verwendung:
#   tools/prerequisites/bootstrap_prerequisites.sh [pfad/zu/boost_archiv]
# Archiv-Reihenfolge: $1 > $COMDARE_BOOST_ARCHIVE > prerequisites/boost_*.{tar.bz2,tar.gz,tar.xz,7z,zip}
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)

ARCHIVE="${1:-${COMDARE_BOOST_ARCHIVE:-}}"

echo "[prereq] Repo: $REPO_ROOT"
if [ -n "$ARCHIVE" ]; then
    echo "[prereq] Boost-Archiv: $ARCHIVE"
    cmake -DCOMDARE_BOOST_ARCHIVE="$ARCHIVE" -P "$REPO_ROOT/cmake/provision_mp11.cmake"
else
    echo "[prereq] kein Archiv-Argument — nutze prerequisites/-Glob (falls vorhanden)"
    cmake -P "$REPO_ROOT/cmake/provision_mp11.cmake"
fi

echo "[prereq] fertig. Boost.MP11 ist nun vendored unter cmake/third_party/boost_mp11/include/"
