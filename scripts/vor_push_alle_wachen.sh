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
#   sh scripts/vor_push_alle_wachen.sh --nur=format    # NUR Wache 2 (clang-format,
#                                                      # CI-Vollmenge) -- bereichsfrei,
#                                                      # laeuft auch auf schmutzigem Baum
#
# WAS DIESES SKRIPT NICHT BEANTWORTET (Posten #48, 10.08.2026 -- die Schwesterstelle
# ausdruecklich benannt statt stillschweigend mitgemeint): es misst den PUSH, also
# "$BASIS...HEAD" -- die Menge, die DIESER Push uebertraegt. Es beantwortet damit
# NICHT die Frage vor einem main-Fast-Forward: "ist der ganze STAND sauber, der nach
# main geht?" Beide Fragen sind legitim, und dieses Skript stellt bewusst die erste.
# Am 09.08.2026 kostete der Unterschied 58 Verstoesse auf einen Schlag (ce-Pipeline
# 15501, 86 Commits, gleicher Baum wie die gruene 15498 mit 1 Commit).
# Die zweite Frage stellt seit 10.08.2026 die CI selbst (test:coverage-guard, Schritt
# 0.6) auf jedem Zweig ausser dem Basis-Zweig. Wer sie lokal stellen will:
#     sh scripts/ci_diff_ascii_width_guard.sh --bereich origin/main origin/development
# Dieser Hinweis ist DOKUMENTATION, kein Gate -- was hier haelt, haelt der CI-Schritt.
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

# -- Aufruf-Schalter --nur=format (#83, 2026-08-12) --------------------------
# WAS ER TUT: er faehrt AUSSCHLIESSLICH Wache 2 (clang-format ueber die
# CI-Vollmenge). Die ist seit der CI-Formel-Paritaet BEREICHSFREI -- sie prueft
# den ARBEITSSTAND der Vollmenge, nicht einen Commit-Bereich, und braucht darum
# weder eine Basis noch einen sauberen Arbeitsstand. Der Schalter umgeht deshalb
# GENAU das Bereich/Clean-Gate; die bereichs- bzw. scopegebundenen Wachen 1 und 3
# werden ausdruecklich als NICHT GEFAHREN ausgewiesen, nicht still uebersprungen.
# WAS ER NICHT IST: kein Ersatz fuer den vollen Lauf vor einem Push.
NUR_FORMAT=0
BASIS_ARG=""
for _arg in "$@"; do
    case "$_arg" in
        --nur=format) NUR_FORMAT=1 ;;
        --nur=*) abbruch "unbekannter Schalter '${_arg}' (bekannt: --nur=format)" ;;
        --*) abbruch "unbekannter Schalter '${_arg}'" ;;
        *)
            [ -z "$BASIS_ARG" ] || abbruch "mehr als eine Basis-Ref angegeben ('${BASIS_ARG}' und '${_arg}')"
            BASIS_ARG="$_arg"
            ;;
    esac
done
if [ "$NUR_FORMAT" -eq 1 ] && [ -n "$BASIS_ARG" ]; then
    abbruch "--nur=format ist bereichsfrei -- eine Basis-Ref ('${BASIS_ARG}') hat dort keinen Gegenstand"
fi

echo "============================================================================="
if [ "$NUR_FORMAT" -eq 1 ]; then
    echo " VOR-PUSH-WACHE (--nur=format): NUR das Format-Gate, CI-Vollmenge"
else
    echo " VOR-PUSH-WACHE: alle Gates ueber ALLE beruehrten Dateien"
fi
echo "============================================================================="
echo ""

# =============================================================================
# Wache 2 als Funktion: clang-format in CI-FORMEL-PARITAET (#83, 2026-08-12)
# =============================================================================
# WARUM VOLLMENGE STATT BERUEHRTER DATEIEN: der CI-Job lint:format prueft IMMER
# die Vollmenge -- git ls-files ueber COMDARE_LINT_PATHS (Formel identisch in
# Cluster-ci-templates/base-pipeline.yml:293). Diese Wache pruefte bis heute nur
# die im Bereich beruehrten Dateien: lokal gruen, CI rot, sobald irgendwo ein
# Altbestand abweicht -- genau der #83-Befund. Ab hier fahren beide DIESELBE
# Formel; die Wache ist damit bereichsfrei und von --nur=format einzeln fahrbar.
# Dazu drei Praezisierungen gegen stille Drift:
#   * ENDUNGS-REGEX exakt wie die CI: '\.(c|cc|cxx|cpp|h|hh|hxx|hpp)$'. Die alte
#     case-Liste hier kannte .c/.cxx/.hxx NICHT -- drei Endungen, die die CI
#     prueft, waren lokal unsichtbar.
#   * COMDARE_LINT_PATHS wird aus der .gitlab-ci.yml EXTRAHIERT, fail-closed bei
#     0 oder >1 Treffern -- kein Template-Default, kein 'erste gewinnt': eine
#     geratene Formel misst einen anderen Lauf als den, der zaehlt.
#   * VERSIONSPFLICHT ==22.1.8 statt blinder Kandidatenkette: verschiedene
#     clang-format-Majors formatieren verschieden; ein lokales Gruen mit fremder
#     Version ist keine Aussage ueber den CI-Lauf.
wache2_format() {
    _w2_label=$1
    echo "============================================================================="
    echo " ${_w2_label} CLANG-FORMAT -- CI-Formel-Paritaet (Vollmenge wie Job lint:format)"
    echo "============================================================================="

    # (1) VERSIONSPFLICHT. ABSCHRIFT MIT HERKUNFT: das SOLL stammt aus
    # Cluster-ci-templates/base-pipeline.yml:43 (COMDARE_LLVM_VER: "22.1.8") --
    # ein fremdes Repo; bumpt es DORT, gehoert diese Abschrift nachgezogen.
    _w2_soll="22.1.8"
    _w2_version() {
        "$1" --version 2>/dev/null | sed -n 's/.*version[[:space:]]*\([0-9][0-9.]*\).*/\1/p' | sed -n '1p'
    }
    if [ -n "${COMDARE_CLANG_FORMAT:-}" ]; then
        CF="$COMDARE_CLANG_FORMAT"
        command -v "$CF" >/dev/null 2>&1 || abbruch "COMDARE_CLANG_FORMAT='$CF' ist nicht ausfuehrbar"
        _w2_ver=$(_w2_version "$CF")
        if [ "$_w2_ver" != "$_w2_soll" ]; then
            abbruch "COMDARE_CLANG_FORMAT='$CF' hat Version '${_w2_ver:-unbekannt}', die CI faehrt ${_w2_soll} \
(base-pipeline.yml:43). Abbruch statt Pruefung mit fremder Version -- \
ein Gruen daraus waere keine Aussage ueber die CI."
        fi
    else
        CF=""
        _w2_gesehen=""
        for _w2_k in \
            /home/comdare/tools/cf22/usr/bin/clang-format-22 \
            clang-format-22 clang-format
        do
            command -v "$_w2_k" >/dev/null 2>&1 || continue
            _w2_ver=$(_w2_version "$_w2_k")
            if [ "$_w2_ver" = "$_w2_soll" ]; then CF="$_w2_k"; break; fi
            _w2_gesehen="${_w2_gesehen} ${_w2_k}(${_w2_ver:-unbekannt})"
        done
        if [ -z "$CF" ]; then
            abbruch "kein clang-format mit Version ==${_w2_soll} gefunden (Pflicht, Herkunft base-pipeline.yml:43). \
Gesehen und wegen Versions-Differenz VERWORFEN:${_w2_gesehen:- nichts}. COMDARE_CLANG_FORMAT=<pfad> setzen."
        fi
    fi
    echo "WERKZEUG: $CF -- Version ${_w2_soll} (Pflicht ==${_w2_soll}, Herkunft base-pipeline.yml:43 COMDARE_LLVM_VER)"

    # (2) DIE DATEI-MENGE: git ls-files ueber COMDARE_LINT_PATHS, sed-extrahiert
    # aus der .gitlab-ci.yml -- fail-closed bei 0 oder mehr als 1 Treffer.
    _w2_yml=".gitlab-ci.yml"
    [ -f "$_w2_yml" ] || abbruch "${_w2_yml} fehlt -- ohne sie ist die CI-Formel nicht bestimmbar"
    _w2_lp_alle=$(sed -n 's/^[[:space:]]*COMDARE_LINT_PATHS:[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' "$_w2_yml")
    _w2_lp_anz=$(printf '%s\n' "$_w2_lp_alle" | grep -c . || true)
    [ -z "$_w2_lp_anz" ] && _w2_lp_anz=0
    if [ "$_w2_lp_anz" -ne 1 ]; then
        abbruch "COMDARE_LINT_PATHS steht ${_w2_lp_anz}-mal in ${_w2_yml}, erwartet GENAU EINMAL. \
Fail-closed: kein Template-Default und kein 'erste gewinnt' -- die Formel waere geraten (#83)."
    fi
    _w2_lp=$_w2_lp_alle

    # (3) DER AUSSCHLUSS. ABSCHRIFT MIT HERKUNFT: Cluster-ci-templates/
    # base-pipeline.yml:59 --
    #   COMDARE_LINT_EXCLUDE_RE: "(^|/)(ext|build|_archive_code_pre_migration|cmake-build-[^/]*|modules)/"
    # In der CI setzt das Template die Variable in die Job-Umgebung; deshalb
    # gewinnt eine gesetzte COMDARE_LINT_EXCLUDE_RE auch hier. Aendert sich das
    # Template, gehoert diese Abschrift nachgezogen.
    _w2_ex="${COMDARE_LINT_EXCLUDE_RE:-(^|/)(ext|build|_archive_code_pre_migration|cmake-build-[^/]*|modules)/}"

    _w2_voll=$(git ls-files -- $_w2_lp | grep -c . || true)
    [ -z "$_w2_voll" ] && _w2_voll=0
    _w2_liste=$(mktemp)
    git ls-files -- $_w2_lp \
        | grep -E '\.(c|cc|cxx|cpp|h|hh|hxx|hpp)$' \
        | grep -vE "$_w2_ex" > "$_w2_liste" || true
    _w2_n=$(grep -c . "$_w2_liste" || true)
    [ -z "$_w2_n" ] && _w2_n=0
    if [ "$_w2_n" -eq 0 ]; then
        rm -f "$_w2_liste"
        abbruch "0 C++-Dateien in der CI-Vollmenge (Pfade: ${_w2_lp}) -- dann ist die Formel kaputt, \
nicht der Baum sauber. Ein leeres Gate ist KEIN gruenes Gate."
    fi

    CPP_GEPRUEFT=0
    CPP_ABWEICHEND=0
    echo ""
    echo "VERSTOESSE (falls vorhanden):"
    while IFS= read -r _w2_f; do
        # getrackt, aber im Arbeitsstand geloescht: nichts zu formatieren
        [ -f "$_w2_f" ] || continue
        CPP_GEPRUEFT=$((CPP_GEPRUEFT + 1))
        if "$CF" --style=file "$_w2_f" | diff -q - "$_w2_f" >/dev/null 2>&1; then
            :
        else
            echo "  FORMAT-ABWEICHUNG   $_w2_f"
            CPP_ABWEICHEND=$((CPP_ABWEICHEND + 1))
            ROT=1
        fi
    done < "$_w2_liste"
    rm -f "$_w2_liste"
    [ "$CPP_ABWEICHEND" -eq 0 ] && echo "  (keine)"
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "NENNER (nie eine nackte Null):"
    echo "  ${_w2_voll} Dateien Vollmenge / ${CPP_GEPRUEFT} geprueft, davon ${CPP_ABWEICHEND} abweichend."
    echo "  Formel (CI-paritaetisch, base-pipeline.yml:293): git ls-files -- ${_w2_lp}"
    echo "  | Endungs-Regex (c|cc|cxx|cpp|h|hh|hxx|hpp) | Exclude-RE (base-pipeline.yml:59)."
    echo "-----------------------------------------------------------------------------"
    echo ""
}

# -- --nur=format: NUR Wache 2, dann Verdikt -- kein Bereich, kein Clean-Gate -
if [ "$NUR_FORMAT" -eq 1 ]; then
    ROT=0
    wache2_format "[nur=format]"
    echo "============================================================================="
    echo " NICHT VON DIESEM LAUF GEPRUEFT (--nur=format):"
    echo "   * Diff-Hygiene (Wache 1) -- bereichsgebunden."
    echo "   * cppcheck (Wache 3)     -- eigener Voll-Scope-Lauf."
    echo "   Der volle Lauf vor dem Push bleibt Pflicht; dieses Verdikt gilt NUR dem Format."
    echo "============================================================================="
    echo ""
    echo "============================================================================="
    if [ "$ROT" -eq 0 ]; then
        echo "VOR-PUSH-WACHE (--nur=format): GRUEN -- das Format-Gate ueber die CI-Vollmenge."
        echo "KEINE Aussage ueber die oben genannten ungeprueften Gates."
        echo "============================================================================="
        exit 0
    fi
    echo "VOR-PUSH-WACHE (--nur=format): ROT -- das Format-Gate hat angeschlagen (siehe oben)."
    echo "============================================================================="
    exit 1
fi

# ── Bereich bestimmen ────────────────────────────────────────────────────────
# Ohne Argument: gegen den Upstream desselben Branches -- das ist genau die Menge,
# die ein Push uebertragen wuerde. Faellt der Upstream weg, ist das ein ABBRUCH und
# kein stilles "dann halt gegen HEAD" (das waere die Nenner-0-Falle von oben).
if [ -n "$BASIS_ARG" ]; then
    BASIS="$BASIS_ARG"
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

# -- Wache 2: clang-format in CI-Formel-Paritaet (Definition oben, #83) ------
# Sie prueft die CI-VOLLMENGE, nicht die beruehrte Teilmenge -- die Dateiliste
# oben dient den Wachen 1 und 3; fuer das Format zaehlt, was der CI-Job sieht.
wache2_format "[2/3]"

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
    # ── 08.08.2026: DIESE AUSWERTUNG WAR SELBST EINE FALSCH-GRUENE ANZEIGE ────────
    # Sie filterte NUR auf ': error:'. cppcheck laeuft hier aber mit
    # --enable=warning,portability, und --error-exitcode=2 schlaegt AUCH bei einer
    # "warning:"-Meldung an. Folge bei einem reinen Warning: Verdikt ROT (der
    # Exit-Code wurde korrekt gelesen), aber Verstoss-Liste LEER und Nenner 0 --
    # also genau das "GRUEN MIT NENNER 0 IST ROT", vor dem der Kopf dieser Datei
    # warnt. Wer die Diagnose liest statt des Verdikts, sieht nichts.
    # Mit einem Koeder nachgestellt (uninitMemberVar, severity warning): cppcheck
    # rc=2, "grep -c ': error:'" -> 0. Geheilt durch das volle Severity-Muster plus
    # einen Rueckfall, der bei rotem Verdikt NIE stumm bleibt.
    CC_MUSTER=': (error|warning|style|performance|portability|information):'
    CC_FEHLER=$(grep -Ec "$CC_MUSTER" "$CC_LOG" 2>/dev/null || true)
    [ -z "$CC_FEHLER" ] && CC_FEHLER=0
    CC_HARTE=$(grep -c ': error:' "$CC_LOG" 2>/dev/null || true)
    [ -z "$CC_HARTE" ] && CC_HARTE=0
    echo ""
    echo "VERSTOESSE (falls vorhanden):"
    if [ "$CC_RC" -ne 0 ]; then
        if [ "$CC_FEHLER" -gt 0 ]; then
            grep -E "$CC_MUSTER" "$CC_LOG" | sed 's/^/  /' | head -30
        else
            # Rotes Verdikt ohne getroffenes Muster: eine unbekannte Severity oder ein
            # Werkzeug-Abbruch. Stumm zu bleiben waere hier der schlimmere Fehler.
            echo "  (kein bekanntes Severity-Muster getroffen -- rohes Log-Ende:)"
            tail -20 "$CC_LOG" | sed 's/^/  /'
        fi
        ROT=1
    else
        echo "  (keine)"
    fi
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "NENNER (nie eine nackte Null):"
    echo "  Pfade: $LINT_PATHS | $IGN_ANZ Ignore-Eintraege | Exit $CC_RC | $CC_FEHLER Befundzeile(n), davon $CC_HARTE mit Severity error."
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
