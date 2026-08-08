#!/bin/sh
# lint_layer_includes.sh -- SF-1-Schichtschnitt-Wache (2026-08-08).
#
# WORUM ES GEHT: die Schichten sind strikt einsortiert -- unter Gattung kommt Genus, und
# anatomy/ ist die UNTERE Schicht (wo Gattung und Genus wohnen). builder/ ist die OBERE
# Schicht (die realen Bau-Strategien der Gattungs-Interface-Factory). Eine anatomy/-Datei,
# die per #include einen Namen aus builder/ zieht, kehrt die Abhaengigkeitsrichtung um.
#
# SF-1 (2026-08-08) hat genau EINE solche Kante im ganzen anatomy/-Baum gefunden
# (container_framework.hpp:37 zog builder/experiment_tree/genus_binding_traits.hpp) und sie
# per Abhaengigkeitsumkehr geheilt: die Bau-Bindung ist seither ein Concept-bewachter
# Template-Parameter (GenusBuildBinding<G, Binding>), kein interner Include mehr. Vier
# andere anatomy/-Dateien dokumentierten den Verzicht auf denselben Include schon vorher
# (adapter_anatomy.hpp, genus_observer_aggregate.hpp, organ_concept.hpp,
# sequence_anatomy.hpp -- Stichwort "hier waere sie ein Include-Zyklus"). Diese Wache macht
# die Disziplin, die die Schicht bereits hatte, MASCHINELL PRUEFBAR statt nur kommentiert.
#
# DIE HAUSFALLE UMGEKEHRT (FALLEN-REGISTER: `grep -v '/build'` FRISST `/builder/`): dort
# war das Problem ein zu GROBER Ausschluss, der ein Teilwort verschluckt hat. Hier ist das
# Gegenteil gewollt -- diese Wache MATCHT ausdruecklich das Teilwort `builder/` in
# #include-Zeilen (kein -v, kein Ausschluss). Sie ist die Umkehrung des Musters: wo die
# Falle eine Kante VERSTECKT hat, soll diese Wache eine Kante ausdruecklich FINDEN.
#
# WERKZEUG: /usr/bin/grep explizit (kein PATH-Alias, keine ugrep-Falle -- FALLEN-REGISTER:
# ugrep liefert auf manchen Mustern eine STILLE Null auf beiden Engines).
#
# AUFRUF:
#   sh scripts/lint_layer_includes.sh   (aus der Repo-Wurzel; Exit 1 bei Fund, 2 bei Abbruch)
#
# POSIX-sh, kein bash-ismus, kein Python (Hausdoktrin: kein Python im Buildsystem).

set -eu

cd "$(dirname "$0")/.."

ZIEL="libs/cache_engine/anatomy"
GREP="/usr/bin/grep"

[ -d "$ZIEL" ] || { echo "lint_layer_includes: ABBRUCH -- $ZIEL fehlt (Pfad verschoben?)"; exit 2; }
[ -x "$GREP" ] || { echo "lint_layer_includes: ABBRUCH -- $GREP fehlt (POSIX-Grep vorausgesetzt)"; exit 2; }

DATEIEN=$(find "$ZIEL" -type f \( -name '*.hpp' -o -name '*.h' -o -name '*.hh' \) | sort)
ANZ_DATEIEN=$(printf '%s\n' "$DATEIEN" | "$GREP" -c . || true)
[ -z "$ANZ_DATEIEN" ] && ANZ_DATEIEN=0

# Ein leerer Scope ist KEIN gruenes Gate (Lehre aus vor_push_alle_wachen.sh: GRUEN MIT
# NENNER 0 IST ROT) -- wenn der Baum verschoben oder umbenannt wurde, soll das AUFFALLEN,
# nicht als stiller Erfolg durchgehen.
if [ "$ANZ_DATEIEN" -eq 0 ]; then
    echo "lint_layer_includes: ABBRUCH -- 0 Header unter $ZIEL gefunden. Ein leerer Scope ist KEIN gruenes Gate."
    exit 2
fi

TREFFER=0
echo "VERSTOESSE (falls vorhanden):"
for f in $DATEIEN; do
    HIT=$("$GREP" -n '#include.*builder/' "$f" 2>/dev/null || true)
    if [ -n "$HIT" ]; then
        printf '%s\n' "$HIT" | sed "s#^#  $f:#"
        TREFFER=$((TREFFER + 1))
    fi
done
[ "$TREFFER" -eq 0 ] && echo "  (keine)"
echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $ANZ_DATEIEN Header unter $ZIEL geprueft, davon $TREFFER mit einer builder/-Include-Zeile."
echo "-----------------------------------------------------------------------------"

if [ "$TREFFER" -gt 0 ]; then
    echo "lint_layer_includes: FAILED -- anatomy/ (untere Schicht) kennt einen Namen aus builder/ (obere Schicht)."
    exit 1
fi
echo "lint_layer_includes: OK ($ANZ_DATEIEN Header unter $ZIEL geprueft, keine builder/-Kante)."
exit 0
