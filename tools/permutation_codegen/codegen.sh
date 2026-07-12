#!/usr/bin/env bash
# tools/permutation_codegen/codegen.sh — POSIX Pendant zu codegen.cmake (F-EXTRA-5 NO-PYTHON)
#
# DEPRECATED (#25 B, 2026-07-12): Dieses Skelett-Skript (nur ein Platzhalter-Output, KEINE echten
#   Permutationen) ist durch das C++23-Tool abgeloest:
#     apps/permutation_codegen_tool/ -> comdare_permutation_codegen_cli (Backend 'cpp').
#   Das C++-Tool erzeugt eine BYTE-IDENTISCHE permutations.cmake zum Default-Backend 'cmake'
#   (codegen.cmake). Neue Belange bitte dort implementieren, nicht hier.
#   Wird bewusst NICHT geloescht (Doku-nie-loeschen); als sh-Backend weiter aufrufbar.
#
# Aufruf:  COMDARE_TARGET_ISA=auto COMDARE_OUTPUT=<path> ./codegen.sh
# oder:    ./codegen.sh --target-isa=auto --output=<path>
#
# Aufgabe (Phase 4.B Skelett — Vollausbau in Phase 5+):
#   identisch zu codegen.cmake — generiert build/generated/permutations.cmake.
#
# Synchronisations-Pflicht: identisch zu codegen.cmake und codegen.bat.

set -euo pipefail

COMDARE_TARGET_ISA="${COMDARE_TARGET_ISA:-auto}"
COMDARE_OUTPUT="${COMDARE_OUTPUT:-}"

while [ $# -gt 0 ]; do
    case "$1" in
        --target-isa=*) COMDARE_TARGET_ISA="${1#*=}" ;;
        --output=*)     COMDARE_OUTPUT="${1#*=}" ;;
        --target-isa)   COMDARE_TARGET_ISA="$2"; shift ;;
        --output)       COMDARE_OUTPUT="$2"; shift ;;
        *) echo "Unbekanntes Argument: $1" >&2; exit 2 ;;
    esac
    shift
done

if [ -z "${COMDARE_OUTPUT}" ]; then
    echo "FEHLER: COMDARE_OUTPUT erforderlich (--output=<path> oder Env-Variable)" >&2
    exit 2
fi

mkdir -p "$(dirname "${COMDARE_OUTPUT}")"

cat > "${COMDARE_OUTPUT}" <<EOF
# permutations.cmake — generiert von tools/permutation_codegen/codegen.sh
# (Phase 4.B Skelett — keine Permutationen, kommt in Phase 5+)
# Target-ISA: ${COMDARE_TARGET_ISA}

message(STATUS "Permutations-Codegen-Skelett (keine Permutationen registriert; ISA=${COMDARE_TARGET_ISA})")
EOF

echo "OK (Skelett): ${COMDARE_OUTPUT} geschrieben."
