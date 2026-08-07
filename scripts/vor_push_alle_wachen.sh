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

# DREI PUNKTE, NICHT ZWEI -- und das ist der Unterschied zwischen "meine Arbeit" und
# "mein Rueckstand". Gefunden am 07.08.2026 an einem echten Fall:
#
#   git diff origin/development..HEAD    -> 12 Dateien   (ZWEI Punkte: Endpunkt gegen Endpunkt)
#   git diff origin/development...HEAD   ->  1 Datei     (DREI Punkte: ab der Abzweigung)
#
# Der Commit hatte GENAU EINE Datei angefasst. Die elf anderen kamen daher, dass
# origin/development inzwischen weitergezogen war. Der Zwei-Punkt-Diff zeigt fremde,
# auf development laengst GEHEILTE Zeilen als "hinzugefuegt" -- die Wache schlug in
# Code an, den der Autor nie beruehrt hat.
#
# ZWEI SCHAEDEN, der zweite ist der schlimmere:
#   (1) Der Autor sucht Stunden in fremdem Code nach einem Verstoss, der ihm nicht gehoert.
#   (2) Schlimmer: er "heilt" fremde Zeilen auf seinen ALTEN Stand zurueck -- und schleppt
#       damit einen Rueckwaerts-Merge ein. Eine Wache, die zu einer Regression verleitet,
#       ist schlechter als keine.
#
# Die Drei-Punkt-Form misst ab dem gemeinsamen Vorfahren (merge-base) und beantwortet
# damit die Frage, die diese Wache stellen will: WAS HABE ICH HINZUGEFUEGT?
BEREICH="$BASIS...HEAD"
_MERGE_BASE=$(git merge-base "$BASIS" HEAD 2>/dev/null) || abbruch "merge-base mit '$BASIS' nicht bestimmbar"
echo "BEREICH: $BEREICH   (Drei-Punkt: ab der Abzweigung, nicht ab dem Endpunkt)"
echo "  Basis: $(git rev-parse --short "$BASIS")   Abzweigung: $(git rev-parse --short "$_MERGE_BASE")   HEAD: $(git rev-parse --short HEAD)"
if [ "$_MERGE_BASE" != "$(git rev-parse "$BASIS")" ]; then
    echo "  HINWEIS: '$BASIS' ist seit der Abzweigung weitergezogen. Gemessen wird DEINE Arbeit"
    echo "           ab der Abzweigung -- nicht der Rueckstand. (Vor dem Landen trotzdem mergen.)"
fi
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
echo " [1/3] DIFF-HYGIENE (ASCII + Spaltenbreite)"
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
echo " [2/3] CLANG-FORMAT ueber alle beruehrten C++-Dateien"
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

# ── Wache 3: cppcheck (= CI-Job lint:static) ─────────────────────────────────
# WARUM DIESE WACHE 2026-08-07 DAZUKAM: sie stand hier als "Werkzeug lokal nicht
# vorhanden" -- und DAS WAR FALSCH. /home/comdare/tools/cppcheck-2.21.0 traegt exakt die
# CI-Version. Die Behauptung hat an EINEM Tag ZWEI rote Pipelines durchgelassen (15239,
# 15245: beide 19 von 20 Jobs gruen, nur lint:static rot, beide am selben Befund).
# Eine Wache, die ein vorhandenes Werkzeug fuer fehlend erklaert, ist schlimmer als gar
# keine: sie erzeugt die Gewissheit, geprueft zu haben.
#
# VOLLER SCOPE, nicht Diff-Scope: cppcheck faehrt MEHRERE Praeprozessor-Konfigurationen
# durch und meldet nur den ERSTEN preprocessorErrorDirective JE DATEI -- eine Aenderung in
# Datei A kann einen Befund in Datei B sichtbar machen, die selbst unberuehrt ist. Ein
# Diff-Scope wuerde genau das verpassen. Der Volllauf kostet ~1-2 min; eine rote Pipeline
# kostet mehr.
#
# DIE PFADE KOMMEN AUS DER .gitlab-ci.yml, NICHT AUS EINER KOPIE HIER: sonst driften Wache
# und Job auseinander, und die Wache wird wieder zu einer Aussage ueber sich selbst.
echo "============================================================================="
echo " [3/3] CPPCHECK (identisch zum CI-Job lint:static)"
echo "============================================================================="

CC="${COMDARE_CPPCHECK:-}"
CC_GESUCHT=""
if [ -z "$CC" ]; then
    for kandidat in \
        "${COMDARE_CITOOLS_DIR:-/nonexistent}/bin/cppcheck" \
        /home/comdare/tools/cppcheck-2.21.0/bin/cppcheck \
        cppcheck
    do
        CC_GESUCHT="$CC_GESUCHT $kandidat"
        if command -v "$kandidat" >/dev/null 2>&1; then CC="$kandidat"; break; fi
    done
fi

if [ -z "$CC" ]; then
    # EHRLICH bleiben: sagen, WO gesucht wurde -- sonst ist "nicht gefunden" wieder
    # eine Behauptung ohne Beleg, und genau daran ist diese Wache schon einmal gescheitert.
    echo "  NICHT GEFAHREN: kein cppcheck gefunden."
    echo "  Gesucht in:$CC_GESUCHT"
    echo "  Setze COMDARE_CPPCHECK=<pfad>. Die CI faehrt es trotzdem -- dieses Gate ist"
    echo "  damit OFFEN, das Gesamt-Verdikt sagt dazu nichts."
    CPPCHECK_GEFAHREN=0
else
    echo "WERKZEUG: $CC ($("$CC" --version 2>/dev/null || echo 'Version unbekannt'))"
    CI_YML=".gitlab-ci.yml"
    LINT_PATHS=""
    [ -f "$CI_YML" ] && LINT_PATHS=$(sed -n 's/^[[:space:]]*COMDARE_LINT_PATHS:[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' "$CI_YML" | head -1)
    # Faellt die Variable im Repo weg, gilt der Template-Default -- nicht raten, benennen.
    if [ -z "$LINT_PATHS" ]; then
        LINT_PATHS="libs apps tests"
        echo "  HINWEIS: COMDARE_LINT_PATHS steht nicht in $CI_YML -> Template-Default '$LINT_PATHS'."
    fi
    IGN_DIRS="${COMDARE_CPPCHECK_IGNORE_DIRS:-ext build _archive_code_pre_migration modules}"
    IGN="-i ./.citools"
    IGN_ANZ=1
    # POSIX: KEINE Process Substitution (`< <(...)`) -- dieses Skript laeuft unter /bin/sh.
    # Ein `while read` hinter einer Pipe liefe zudem in einer Subshell und verloere IGN.
    # Der for-Loop ueber $(find ...) ist wortsplitting-abhaengig; das ist hier unkritisch,
    # weil die Namen aus IGN_DIRS stammen (ext/build/modules) und keine Leerzeichen tragen.
    for d in $IGN_DIRS; do
        case "$d" in
            */*) IGN="$IGN -i ./$d"; IGN_ANZ=$((IGN_ANZ + 1)) ;;
            *)   for p in $(find . -path ./.citools -prune -o -type d -name "$d" -print 2>/dev/null); do
                     IGN="$IGN -i $p"; IGN_ANZ=$((IGN_ANZ + 1))
                 done ;;
        esac
    done
    CC_LOG=$(mktemp)
    # EXAKT der Aufruf aus ci-templates/base-pipeline.yml (.lint-static) -- jede Abweichung
    # macht das Gate zu einer Aussage ueber einen anderen Lauf als den, der zaehlt.
    set +e
    "$CC" --enable=warning,portability --inline-suppr --library=googletest \
          --error-exitcode=2 --std=c++23 --language=c++ -q $LINT_PATHS $IGN >"$CC_LOG" 2>&1
    CC_RC=$?
    set -e
    CC_FEHLER=$(grep -c ': error:' "$CC_LOG" 2>/dev/null || true)
    [ -z "$CC_FEHLER" ] && CC_FEHLER=0
    echo ""
    echo "VERSTOESSE (falls vorhanden):"
    if [ "$CC_RC" -ne 0 ]; then
        grep ': error:' "$CC_LOG" | sed 's/^/  /' | head -30
        ROT=1
    else
        echo "  (keine)"
    fi
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "NENNER (nie eine nackte Null):"
    echo "  Pfade: $LINT_PATHS | $IGN_ANZ Ignore-Eintraege | Exit $CC_RC | $CC_FEHLER Fehlerzeile(n)."
    echo "  VOLLER Scope wie die CI -- NICHT nur die $ANZ_DATEIEN beruehrten Dateien."
    echo "-----------------------------------------------------------------------------"
    rm -f "$CC_LOG"
    CPPCHECK_GEFAHREN=1
fi
echo ""

# ── WAS DIESE WACHE NICHT PRUEFT ─────────────────────────────────────────────
# Eine Wache, die nur ihr eigenes Gruen meldet und ueber ihre Luecken schweigt, ist
# eine Einladung zum Fehlschluss "gruen == landefaehig". Genau so ist Pipeline 15199
# rot geworden: diese Wache war gruen, aber cppcheck (Job lint:static) faengt Klassen,
# die sie gar nicht kennt (uninitMemberVarNoCtor an 7 Membern). Deshalb steht die
# Luecke ab jetzt IM Verdikt, nicht in einem Kommentar, den niemand liest.
echo "============================================================================="
echo " NICHT VON DIESER WACHE GEPRUEFT (die CI faehrt sie trotzdem):"
if [ "${CPPCHECK_GEFAHREN:-0}" -eq 0 ]; then
echo "   * cppcheck / lint:static  -- OFFEN, Werkzeug nicht gefunden (s. [3/3] oben)."
fi
echo "   * Bau + ctest             -- absichtlich nicht: sie gehoeren in den Bau-Schritt,"
echo "     nicht in eine Datei-Wache. VOR dem Push selbst fahren."
echo "   * chktex / LaTeX-Gates    -- nur im thesis-Repo relevant."
echo "   * gitleaks                -- braucht einen echten Klon (Submodul-Falle), separat."
echo "============================================================================="
echo ""

# ── Verdikt ──────────────────────────────────────────────────────────────────
echo "============================================================================="
if [ "$ROT" -eq 0 ]; then
    echo "VOR-PUSH-WACHE: GRUEN -- die HIER gefahrenen Gates ueber alle $ANZ_DATEIEN beruehrten"
    echo "Dateien. KEINE Aussage ueber die oben genannten ungeprueften Gates."
    echo "============================================================================="
    exit 0
fi
echo "VOR-PUSH-WACHE: ROT -- mindestens ein Gate hat angeschlagen (siehe oben)."
echo "============================================================================="
exit 1
