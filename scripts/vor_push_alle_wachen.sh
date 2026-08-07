#!/bin/sh
# =============================================================================
#  VOR-PUSH-WACHE: alle lokal fahrbaren Gates ueber ALLE beruehrten Dateien
#  (scripts/vor_push_alle_wachen.sh)
# =============================================================================
#
# WARUM ES DIESES SKRIPT GIBT (07.08.2026): an einem einzigen Tag ist derselbe
# Fehler ZWEIMAL passiert -- erst bei clang-format, dann beim ASCII-Selbstcheck:
# von ZWEI beruehrten Dateien wurde EINE geprueft und das Ergebnis fuer beide
# genommen. Die zweite fiel in der CI durch (ein einziges Em-Dash). Die Wachen
# existierten alle, nur lief sie niemand ZUSAMMEN und ueber die VOLLE Dateiliste.
#
# DAZU EINE ZWEITE FALLE, ebenfalls am selben Tag: die Diff-Hygiene-Wache lokal
# ohne Bereich aufgerufen meldete GRUEN -- mit "0 Zusatzzeilen geprueft", weil der
# Commit schon lag und der Working-Tree-Diff leer war. GRUEN MIT NENNER 0 IST ROT.
# Dieses Skript bestimmt den Bereich deshalb SELBST und gibt jeden Nenner aus.
#
# AUFRUF:
#   sh scripts/vor_push_alle_wachen.sh                 # gegen origin/<branch>
#   sh scripts/vor_push_alle_wachen.sh <basis-ref>     # gegen einen eigenen Anker
#
# EXIT: 0 = alle Wachen gruen. 1 = mindestens eine rot (Details oben im Log).
#       2 = ABBRUCH (Werkzeug fehlt / kein Repo / Bereich unbestimmbar) -- das ist
#       ausdruecklich KEIN Gruen: ein nicht gelaufener Test ist kein bestandener.
#
# POSIX-sh, kein bash-ismus, kein `grep -P` (Runner-Portabilitaet).
# =============================================================================

set -eu

abbruch() {
    echo ""
    echo "VOR-PUSH-WACHE: ABBRUCH -- $1" >&2
    exit 2
}

command -v git >/dev/null 2>&1 || abbruch "git fehlt"
git rev-parse --git-dir >/dev/null 2>&1 || abbruch "kein git-Repository"

REPO_ROOT=$(git rev-parse --show-toplevel) || abbruch "Repo-Wurzel nicht bestimmbar"
cd "$REPO_ROOT" || abbruch "cd in die Repo-Wurzel fehlgeschlagen"

echo "============================================================================="
echo " VOR-PUSH-WACHE: alle Gates ueber ALLE beruehrten Dateien"
echo "============================================================================="
echo ""

# ── Bereich bestimmen ────────────────────────────────────────────────────────
# Ohne Argument: gegen den Upstream desselben Branches -- das ist genau die Menge,
# die ein Push uebertragen wuerde. Faellt der Upstream weg, ist das ein ABBRUCH und
# kein stilles "dann halt gegen HEAD" (das waere die Nenner-0-Falle von oben).
if [ $# -ge 1 ]; then
    BASIS="$1"
    git rev-parse --verify --quiet "$BASIS" >/dev/null || abbruch "Basis-Ref '$BASIS' existiert nicht"
else
    BRANCH=$(git rev-parse --abbrev-ref HEAD) || abbruch "Branch nicht bestimmbar"
    [ "$BRANCH" = "HEAD" ] && abbruch "detached HEAD -- bitte Basis-Ref als Argument angeben"
    BASIS="origin/$BRANCH"
    git rev-parse --verify --quiet "$BASIS" >/dev/null \
        || abbruch "'$BASIS' existiert nicht (erst 'git fetch origin', oder Basis-Ref als Argument angeben)"
fi

BEREICH="$BASIS..HEAD"
echo "BEREICH: $BEREICH"
echo "  Basis: $(git rev-parse --short "$BASIS")   HEAD: $(git rev-parse --short HEAD)"
echo ""

# ── SCHMUTZIGER ARBEITSSTAND IST EIN ABBRUCH, KEINE WARNUNG ──────────────────
# Der Bereich <basis>..HEAD sieht ausschliesslich COMMITTETE Aenderungen. Das ist
# fuer einen Push genau richtig -- aber es heisst auch: ein Fehler, der noch
# uncommittet im Arbeitsverzeichnis liegt, ist fuer diese Wache UNSICHTBAR.
# Genau daran ist die erste Bissprobe dieses Skripts gescheitert: Em-Dash
# uncommittet wieder eingebaut -> Wache meldete GRUEN. Wer dann committet und
# pusht, hat ein gruenes Gate im Ruecken, das den Fehler nie gesehen hat.
# Deshalb: schmutziger Arbeitsstand = ABBRUCH (Exit 2), nicht GRUEN.
SCHMUTZ=$(git status --porcelain -- . | grep -c . || true)
[ -z "$SCHMUTZ" ] && SCHMUTZ=0
if [ "$SCHMUTZ" -gt 0 ]; then
    echo "ARBEITSSTAND: $SCHMUTZ nicht committete Aenderung(en):"
    git status --porcelain -- . | sed 's/^/    /'
    echo ""
    abbruch "der Arbeitsstand ist schmutzig. Diese Wache prueft NUR Committetes ($BEREICH) -- \
uncommittete Aenderungen waeren fuer sie unsichtbar und das Gruen damit wertlos. \
Erst committen (oder stashen), dann erneut fahren."
fi
echo "ARBEITSSTAND: sauber (0 uncommittete Aenderungen) -- der Bereich ist vollstaendig."
echo ""

# ── Die Dateiliste: EINMAL bestimmt, von allen Wachen benutzt ────────────────
DATEIEN=$(git diff --name-only "$BEREICH" || true)
ANZ_DATEIEN=$(printf '%s\n' "$DATEIEN" | grep -c . || true)
[ -z "$ANZ_DATEIEN" ] && ANZ_DATEIEN=0

echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $ANZ_DATEIEN Datei(en) im Bereich beruehrt."
echo "-----------------------------------------------------------------------------"
echo ""

if [ "$ANZ_DATEIEN" -eq 0 ]; then
    abbruch "0 beruehrte Dateien -- pruefe den Bereich. Ein leerer Diff ist KEIN gruenes Gate."
fi

printf '%s\n' "$DATEIEN" | sed 's/^/  /'
echo ""

ROT=0

# ── Wache 1: Diff-Hygiene (ASCII + Spaltenbreite) ────────────────────────────
echo "============================================================================="
echo " [1/2] DIFF-HYGIENE (ASCII + Spaltenbreite)"
echo "============================================================================="
if [ -f scripts/ci_diff_ascii_width_guard.sh ]; then
    if sh scripts/ci_diff_ascii_width_guard.sh "$BEREICH"; then
        :
    else
        ROT=1
    fi
else
    abbruch "scripts/ci_diff_ascii_width_guard.sh fehlt -- die CI faehrt sie, also muss sie hier laufen"
fi
echo ""

# ── Wache 2: clang-format ueber ALLE beruehrten C++-Dateien ──────────────────
echo "============================================================================="
echo " [2/2] CLANG-FORMAT ueber alle beruehrten C++-Dateien"
echo "============================================================================="

CF="${COMDARE_CLANG_FORMAT:-}"
if [ -z "$CF" ]; then
    for kandidat in \
        /home/comdare/tools/cf22/usr/bin/clang-format-22 \
        clang-format-22 clang-format
    do
        if command -v "$kandidat" >/dev/null 2>&1; then CF="$kandidat"; break; fi
    done
fi
[ -n "$CF" ] || abbruch "kein clang-format gefunden (COMDARE_CLANG_FORMAT setzen)"
echo "WERKZEUG: $CF ($("$CF" --version 2>/dev/null || echo 'Version unbekannt'))"
echo ""

CPP_GEPRUEFT=0
CPP_ABWEICHEND=0
echo "VERSTOESSE (falls vorhanden):"
for f in $DATEIEN; do
    case "$f" in
        *.hpp|*.cpp|*.h|*.cc|*.hh) ;;
        *) continue ;;
    esac
    # geloeschte Dateien haben nichts zu formatieren
    [ -f "$f" ] || continue
    CPP_GEPRUEFT=$((CPP_GEPRUEFT + 1))
    if "$CF" --style=file "$f" | diff -q - "$f" >/dev/null 2>&1; then
        :
    else
        echo "  FORMAT-ABWEICHUNG   $f"
        CPP_ABWEICHEND=$((CPP_ABWEICHEND + 1))
        ROT=1
    fi
done
[ "$CPP_ABWEICHEND" -eq 0 ] && echo "  (keine)"
echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $CPP_GEPRUEFT C++-Datei(en) formatgeprueft, davon $CPP_ABWEICHEND abweichend."
echo "  (von $ANZ_DATEIEN beruehrten Dateien insgesamt; der Rest ist kein C++.)"
echo "-----------------------------------------------------------------------------"
echo ""

# ── Verdikt ──────────────────────────────────────────────────────────────────
echo "============================================================================="
if [ "$ROT" -eq 0 ]; then
    echo "VOR-PUSH-WACHE: GRUEN -- alle Gates ueber alle $ANZ_DATEIEN beruehrten Dateien."
    echo "============================================================================="
    exit 0
fi
echo "VOR-PUSH-WACHE: ROT -- mindestens ein Gate hat angeschlagen (siehe oben)."
echo "============================================================================="
exit 1
