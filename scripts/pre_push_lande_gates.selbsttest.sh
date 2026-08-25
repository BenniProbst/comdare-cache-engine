#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  SELBSTTEST des FLOOR-GATES [5/6] in scripts/pre_push_lande_gates.sh
#  H-1 (Raeumung Q3, 2026-08-25): der Gate-Baum wird GENANNT, nicht geraten --
#  und ein genannter Baum muss der Gegenstand DIESES Checkouts sein.
# =============================================================================
#
# WARUM ES IHN GIBT. Bis H-1 hatte das Floor-Gate einen Default-Baum (build/gcc-release,
# der Preset-Pfad). Am Objekt stand dort zuletzt ein ALT-Baum eines aelteren Commits neben
# dem echten Gate-Baum; ein Lauf ohne Variable haette den alten gezaehlt und bei
# Anker == Anker GRUEN gemeldet. Und ein GENANNTER Baum eines anderen Worktrees meldete
# am 25.08.2026 ebenfalls GRUEN (545 == 545) -- ohne dass eine Zeile des pruefenden
# Checkouts je gezaehlt worden waere. Beides ist ein richtiges Messgeraet am falschen
# Gegenstand (V6.6). Dieser Selbsttest faehrt die Abbruch-Pfade bei JEDEM Lauf nach.
#
# WIE ER KOEDERT -- SYNTHETISCHE Bau-Baeume (Muster ci_test_bauweg_wache.selbsttest.sh):
# das Gate liest aus einem Baum genau CMakeCache.txt (CMAKE_HOME_DIRECTORY, Host-Klasse),
# CTestTestfile.cmake (ctest -N) und Testing/Temporary/LastTest.log (Beweis-Schutz). Alle
# drei lassen sich in wenigen Zeilen erzeugen; der echte Baum wird NIE angefasst.
#
#   1 ABBRUCH   Variable ungesetzt                          -> Exit 2, "KEINEN Default-Baum"
#   2 ABBRUCH   Baum aus einem ANDEREN Checkout             -> Exit 2, "ANDEREN Checkout"
#   3 ABBRUCH   Registrierung aelter als CMake-Dateien      -> Exit 2, "AELTER als CMake-Dateien"
#   4 ROT       eigener Baum, 1 Test, Marker-LastTest.log   -> Exit 1, "UNTER Anker",
#               LastTest.log danach BYTEIDENTISCH (ctest -N haette es zum Stub gemacht)
#   5 ROT       eigener Baum OHNE LastTest.log              -> Exit 1, danach weiterhin KEINE Datei
# Fall 4/5 sind zugleich der Gegenkoeder gegen "immer Abbruch": der Baum besteht alle drei
# Proben und das Gate kommt bis zur Inventur. Der GRUENE Pfad (Inventur == Anker) ist NICHT
# synthetisierbar, ohne den Anker aus der Floor-Datei zu erben (Zufalls-Gruen, V6.6 (b)) --
# er bleibt dem echten Gate-Lauf der Landung vorbehalten.
#
# AUFRUF:  sh scripts/pre_push_lande_gates.selbsttest.sh
# EXIT:    0 = alle fuenf Faelle wie erwartet
#          1 = mindestens ein Fall verhielt sich anders (das Gate ist nicht, was es scheint)
#          2 = der Selbsttest konnte nicht pruefen (fail-closed)
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin). Exits werden NIE hinter einer Pipe gemessen.
# =============================================================================

set -eu

abbruch() { echo "SELBSTTEST: ABBRUCH -- $1" >&2; exit 2; }

command -v git >/dev/null 2>&1 || abbruch "git fehlt"
command -v ctest >/dev/null 2>&1 || abbruch "ctest fehlt"
ROOT=$(git rev-parse --show-toplevel) || abbruch "Repo-Wurzel nicht bestimmbar"
cd "$ROOT" || abbruch "cd in die Repo-Wurzel fehlgeschlagen"
GATE=scripts/pre_push_lande_gates.sh
[ -f "$GATE" ] || abbruch "$GATE fehlt"

TMPD=$(mktemp -d) || abbruch "mktemp -d fehlgeschlagen"
trap 'rm -rf "$TMPD"' EXIT INT TERM
ROOT_R=$(pwd -P)

# Zufalls-Marker aus /dev/urandom (nie als Literal hier drin, nie erratbar).
MARKER=$(od -An -N8 -tx1 /dev/urandom 2>/dev/null | tr -d ' \n')
[ -n "$MARKER" ] || abbruch "kein Zufalls-Marker (od/urandom)"

# Einen synthetischen Baum anlegen: $1 = Pfad, $2 = CMAKE_HOME_DIRECTORY.
baum_anlegen() {
    mkdir -p "$1/Testing/Temporary" || abbruch "mkdir $1"
    printf 'add_test(koeder_%s /bin/true)\n' "$MARKER" > "$1/CTestTestfile.cmake"
    {
        printf 'CMAKE_HOME_DIRECTORY:INTERNAL=%s\n' "$2"
        printf 'COMDARE_HOST_RUNS_AVX2:INTERNAL=1\nCOMDARE_HOST_RUNS_AVX512F:INTERNAL=1\n'
    } > "$1/CMakeCache.txt"
}

FEHLER=0
fall() {   # $1 = Nr, $2 = Erwartung-Exit, $3 = Pflicht-Text, $4 = Ist-Exit, $5 = Log
    if [ "$4" -eq "$2" ] && grep -q "$3" "$5"; then
        echo "FALL $1  OK       Exit $4, Text '$3' gefunden"
    else
        echo "FALL $1  ANDERS   Exit $4 (erwartet $2), Text '$3': $(grep -c "$3" "$5" || true) Treffer"
        sed 's/^/    | /' "$5" | tail -4
        FEHLER=1
    fi
}

echo "PRE-PUSH-LANDE-GATES-SELBSTTEST (Floor-Gate [5/6], synthetische Baeume unter $TMPD)"

# 1 -- ungesetzt
set +e
env -u COMDARE_PRE_PUSH_BUILD_DIR sh "$GATE" --nur=floor > "$TMPD/f1.log" 2>&1
RC=$?
set -e
fall 1 2 "KEINEN Default-Baum" "$RC" "$TMPD/f1.log"

# 2 -- fremder Checkout
mkdir -p "$TMPD/fremd_checkout"
baum_anlegen "$TMPD/baum_fremd" "$TMPD/fremd_checkout"
set +e
COMDARE_PRE_PUSH_BUILD_DIR="$TMPD/baum_fremd" sh "$GATE" --nur=floor > "$TMPD/f2.log" 2>&1
RC=$?
set -e
fall 2 2 "ANDEREN Checkout" "$RC" "$TMPD/f2.log"

# 3 -- Registrierung aelter als die CMake-Dateien des Checkouts
baum_anlegen "$TMPD/baum_alt" "$ROOT_R"
touch -t 200001010000 "$TMPD/baum_alt/CTestTestfile.cmake" || abbruch "touch -t"
set +e
COMDARE_PRE_PUSH_BUILD_DIR="$TMPD/baum_alt" sh "$GATE" --nur=floor > "$TMPD/f3.log" 2>&1
RC=$?
set -e
fall 3 2 "AELTER als CMake-Dateien" "$RC" "$TMPD/f3.log"

# 4 -- eigener Baum, Marker-LastTest.log: ROT (1 Test unter dem Anker), Beweis bleibt byteidentisch
baum_anlegen "$TMPD/baum_eigen" "$ROOT_R"
printf 'VOLLAUF-BEWEIS %s\n' "$MARKER" > "$TMPD/baum_eigen/Testing/Temporary/LastTest.log"
cp -p "$TMPD/baum_eigen/Testing/Temporary/LastTest.log" "$TMPD/f4.vorher"
set +e
COMDARE_PRE_PUSH_BUILD_DIR="$TMPD/baum_eigen" sh "$GATE" --nur=floor > "$TMPD/f4.log" 2>&1
RC=$?
set -e
fall 4 1 "UNTER Anker" "$RC" "$TMPD/f4.log"
if cmp -s "$TMPD/f4.vorher" "$TMPD/baum_eigen/Testing/Temporary/LastTest.log"; then
    echo "FALL 4b OK       LastTest.log byteidentisch ($(wc -c < "$TMPD/f4.vorher") Bytes, Marker erhalten)"
else
    echo "FALL 4b ANDERS   LastTest.log VERAENDERT: $(wc -c < "$TMPD/baum_eigen/Testing/Temporary/LastTest.log") Bytes"
    FEHLER=1
fi

# 5 -- eigener Baum OHNE LastTest.log: ROT, und danach liegt weiterhin keine Datei dort
rm -f "$TMPD/baum_eigen/Testing/Temporary/LastTest.log"
set +e
COMDARE_PRE_PUSH_BUILD_DIR="$TMPD/baum_eigen" sh "$GATE" --nur=floor > "$TMPD/f5.log" 2>&1
RC=$?
set -e
fall 5 1 "UNTER Anker" "$RC" "$TMPD/f5.log"
if [ -e "$TMPD/baum_eigen/Testing/Temporary/LastTest.log" ]; then
    echo "FALL 5b ANDERS   ctest -N hat einen LastTest.log-Stub hinterlassen"
    FEHLER=1
else
    echo "FALL 5b OK       kein LastTest.log-Stub hinterlassen"
fi

if [ "$FEHLER" -eq 0 ]; then
    echo "SELBSTTEST: GRUEN -- 5 Faelle (+2 Beweis-Proben) wie erwartet. NICHT geprueft: der gruene Pfad (echter Baum)."
    exit 0
fi
echo "SELBSTTEST: ROT -- mindestens ein Fall verhielt sich anders (oben)."
exit 1
