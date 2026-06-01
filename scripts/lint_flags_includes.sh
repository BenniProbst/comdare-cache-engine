#!/usr/bin/env bash
# lint_flags_includes.sh — Wartbarkeits-Lint (2026-06-01).
#
# Invariante: jeder `#include` eines GENERIERTEN `*_flags.hpp` muss den tatsächlichen
# configure_file-Generierungs-Ort treffen. Hintergrund: die Flags-Header werden je Achse
# entweder nach `generated/axes/<x>/` ODER nach `generated/topics/<x>/` generiert (NICHT beides).
# Ein Include über den falschen Pfad (z.B. `topics/...` für einen nach `axes/...` generierten
# Flags-Header) kompiliert nur, solange ein stale Build-Artefakt am falschen Ort liegt — ein
# frischer Build-Dir scheitert (genau der 2026-06-01 gefundene Bug: vendor_includes + 2 Tests).
#
# Verwendung:  bash scripts/lint_flags_includes.sh   (aus der Repo-Wurzel; Exit 1 bei Mismatch)

set -euo pipefail
cd "$(dirname "$0")/.."

# 1) Map: <flags-basename> -> erwarteter Include-Pfad (= generierter Pfad ohne `generated/`-Prefix,
#    da der Build `generated/` als Include-Root hinzufügt).
declare -A EXPECTED
while IFS= read -r gen; do
  EXPECTED["$(basename "$gen")"]="${gen#generated/}"
done < <(grep -ohE 'generated/[A-Za-z0-9_/]*_flags\.hpp' CMakeLists.txt | sort -u)

if [ "${#EXPECTED[@]}" -eq 0 ]; then
  echo "lint_flags_includes: WARN — keine generierten *_flags.hpp in CMakeLists gefunden (Pfad/Pattern?)."
  exit 0
fi

# 2) Alle #include eines *_flags.hpp in libs/tests/apps prüfen.
fail=0
while IFS= read -r hit; do
  file="${hit%%:*}"
  inc="$(printf '%s' "$hit" | grep -oE '[<"][^">]*_flags\.hpp[>"]' | head -1 | tr -d '<>"')"
  [ -z "$inc" ] && continue
  exp="${EXPECTED[$(basename "$inc")]:-}"
  [ -z "$exp" ] && continue          # kein generierter Flags-Header (z.B. .in-Template-intern) → ignorieren
  case "$inc" in
    */*)                             # nur Includes MIT Verzeichnis-Prefix prüfen (Same-Dir-Includes sind ok)
      if [ "$inc" != "$exp" ]; then
        echo "FLAGS-INCLUDE-MISMATCH: $file"
        echo "    include : $inc"
        echo "    generiert: $exp"
        fail=1
      fi ;;
  esac
done < <(grep -rnE '#include[[:space:]]*[<"][^">]*_flags\.hpp[>"]' libs tests apps 2>/dev/null || true)

if [ "$fail" -ne 0 ]; then
  echo "lint_flags_includes: FAILED — mindestens ein Flags-Include trifft nicht den Generierungs-Ort."
  exit 1
fi
echo "lint_flags_includes: OK (${#EXPECTED[@]} generierte Flags-Header, alle Includes konsistent)"
