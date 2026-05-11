#!/usr/bin/env bash
# tools/permutation_codegen/codegen.sh — POSIX Pendant zu codegen.cmake (F-EXTRA-5 NO-PYTHON)
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
