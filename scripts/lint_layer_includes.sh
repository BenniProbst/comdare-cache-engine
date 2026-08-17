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

# A2.5-Fix F-3: ZWEI Ziele, nicht eins. Die Wache startete ihren find IN
# libs/cache_engine/anatomy und war damit blind fuer das GESCHWISTER-Verzeichnis anatomy_drive/
# (K2, Owner 09.08.) -- eine ganze stufen-neutrale Schicht lag ausserhalb ihres Scopes.
# Wer hier ein drittes neutrales Verzeichnis anlegt, traegt es in DIESE Liste ein.
ZIELE="libs/cache_engine/anatomy libs/cache_engine/anatomy_drive"
GREP="/usr/bin/grep"

# ERLAUBTE builder-Kanten -- benannt, begruendet, einzeln. KEINE Muster, keine Wildcards:
# jede Zeile ist eine Einzelfall-Entscheidung mit Beleg, damit eine zweite Kante NICHT
# stillschweigend unter dieselbe Ausnahme rutscht.
#
#   anatomy_drive/search_algorithm_drive.hpp -> builder/anatomy_module_loader/...
#     BELEG: Wellenplan-Korb-B-Posten B-10 "Loader stufen-neutral" + Owner 09.08. ("der Loader
#     wandert in eine stufen-neutrale Bibliothek"). Der Loader IST bereits builder-unabhaengig --
#     eigene CMake-Lib, nur Boost+libdl -- er liegt lediglich noch physisch unter builder/.
#     Die Include-ZEILE ist damit ein PFAD-Artefakt, keine Schicht-Verletzung.
#     WANN DIESE ZEILE VERSCHWINDET: sobald das Loader-Verzeichnis physisch aus builder/
#     herauswandert. Das ist ein eigener Posten (Verzeichnis-Umzug beruehrt 34 CMake-Stellen)
#     und bewusst NICHT Teil der K2-Extraktion.
ALLOWLIST="libs/cache_engine/anatomy_drive/search_algorithm_drive.hpp"

for Z in $ZIELE; do
    [ -d "$Z" ] || { echo "lint_layer_includes: ABBRUCH -- $Z fehlt (Pfad verschoben?)"; exit 2; }
done
[ -x "$GREP" ] || { echo "lint_layer_includes: ABBRUCH -- $GREP fehlt (POSIX-Grep vorausgesetzt)"; exit 2; }

DATEIEN=$(find $ZIELE -type f \( -name '*.hpp' -o -name '*.h' -o -name '*.hh' \) | sort)
ANZ_DATEIEN=$(printf '%s\n' "$DATEIEN" | "$GREP" -c . || true)
[ -z "$ANZ_DATEIEN" ] && ANZ_DATEIEN=0

# Ein leerer Scope ist KEIN gruenes Gate (Lehre aus vor_push_alle_wachen.sh: GRUEN MIT
# NENNER 0 IST ROT) -- wenn der Baum verschoben oder umbenannt wurde, soll das AUFFALLEN,
# nicht als stiller Erfolg durchgehen.
if [ "$ANZ_DATEIEN" -eq 0 ]; then
    echo "lint_layer_includes: ABBRUCH -- 0 Header unter [$ZIELE] gefunden. Ein leerer Scope ist KEIN gruenes Gate."
    exit 2
fi

TREFFER=0
echo "VERSTOESSE (falls vorhanden):"
ERLAUBT_GENUTZT=0
for f in $DATEIEN; do
    HIT=$("$GREP" -n '#include.*builder/' "$f" 2>/dev/null || true)
    # Allowlist: benannte Einzelfaelle. Sie werden GEZAEHLT und unten ausgewiesen -- eine
    # Ausnahme, die niemand mehr sieht, ist der Anfang der naechsten.
    for a in $ALLOWLIST; do
        if [ "$f" = "$a" ] && [ -n "$HIT" ]; then
            ERLAUBT_GENUTZT=$((ERLAUBT_GENUTZT + 1))
            HIT=""
        fi
    done
    if [ -n "$HIT" ]; then
        printf '%s\n' "$HIT" | sed "s#^#  $f:#"
        TREFFER=$((TREFFER + 1))
    fi
done
[ "$TREFFER" -eq 0 ] && echo "  (keine)"
echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $ANZ_DATEIEN Header unter [$ZIELE] geprueft, davon $TREFFER mit einer builder/-Include-Zeile."
echo "  ERLAUBTE Kanten (benannte Allowlist, s. Kopf): $ERLAUBT_GENUTZT genutzt."
echo "-----------------------------------------------------------------------------"

if [ "$TREFFER" -gt 0 ]; then
    echo "lint_layer_includes: FAILED -- eine untere Schicht ([$ZIELE]) kennt einen Namen aus builder/ (obere Schicht)."
    exit 1
fi
echo "lint_layer_includes: OK ($ANZ_DATEIEN Header unter [$ZIELE] geprueft, keine unerlaubte builder/-Kante; $ERLAUBT_GENUTZT erlaubte)."
exit 0
