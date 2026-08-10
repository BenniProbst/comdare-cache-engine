#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  SELBSTTEST der BAUWEG-WACHE (scripts/ci_test_bauweg_wache.sh)
#  D1b/D1e, 2026-08-09 -- K13/V-2: eine Wache gilt erst als gebaut, wenn ein
#  ZUFAELLIG erzeugter Koeder sie zum Beissen bringt UND der unmanipulierte Lauf
#  gruen bleibt.
# =============================================================================
#
# WARUM ES IHN GIBT. Die Bauweg-Wache ist heute gruen. Gruen heisst zweierlei, und die
# beiden sind von aussen nicht zu unterscheiden: "sie prueft und findet nichts" oder
# "sie prueft nicht mehr". Eine Wache, deren Biss nie nachgefahren wird, kann still
# konstant gruen sein -- genau die Klasse, die run_all_tests.sh mit 13x [NOT FOUND] und
# exit 0 vorgefuehrt hat. Dieser Selbsttest faehrt den Biss bei JEDEM Lauf nach.
#
# WIE ER KOEDERT -- ein SYNTHETISCHER Bau-Baum, nicht der echte. Die Wache braucht
# genau zwei Eingaben: eine CTestTestfile.cmake (was aufgerufen wird) und eine
# build.ninja (was gebaut wird). Beide lassen sich in wenigen Zeilen erzeugen. Das ist
# der ehrlichere Weg als eine Mutation am echten Baum: der echte Baum muesste dafuer
# veraendert und wieder hergestellt werden, und ein Abbruch mittendrin liesse ihn kaputt
# zurueck. Der synthetische Baum ist hermetisch und nach dem Lauf restlos weg.
#
# DIE KOEDER-NAMEN KOMMEN AUS /dev/urandom, bei jedem Lauf neu. Damit koennen sie weder
# versehentlich in der Allowlist stehen noch aus dieser Datei abgeschrieben sein -- ein
# fest verdrahteter Koeder waere ein Koeder, den die Wache auswendig kennen koennte.
#
# GEPRUEFT WERDEN BEIDE RICHTUNGEN, nicht nur der Biss:
#   1 GRUEN      alles haengt an comdare_tests            -> Exit 0
#   2 ROT        haengt nur an 'all'                      -> Exit 1 + Name + "nur an 'all'"
#   3 ROT        haengt nirgends                          -> Exit 1 + Name + "nirgends"
#   4 GRUEN      Datenpfad, den ninja nicht herstellt     -> Exit 0, NICHT als Befund
#   5 ABBRUCH    build.ninja fehlt                        -> Exit 2 (fail-closed, kein Gruen)
# Fall 1 und 4 sind der Gegenkoeder: eine Wache, die immer rot ist, ist so wertlos wie
# eine, die nie rot wird.
#
# AUFRUF:  sh scripts/ci_test_bauweg_wache.selbsttest.sh
# EXIT:    0 = alle Faelle wie erwartet
#          1 = mindestens ein Fall verhielt sich anders (die Wache ist nicht, was sie scheint)
#          2 = der Selbsttest konnte nicht pruefen (fail-closed)
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

WURZEL=$(git rev-parse --show-toplevel 2>/dev/null) || {
    echo "ABBRUCH: kein git-Arbeitsbaum." >&2; exit 2; }
cd "$WURZEL"

WACHE="scripts/ci_test_bauweg_wache.sh"
[ -f "$WACHE" ] || { echo "ABBRUCH: $WACHE fehlt." >&2; exit 2; }
command -v ninja >/dev/null 2>&1 || { echo "ABBRUCH: ninja nicht im PATH." >&2; exit 2; }

TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

# -- Koeder wuerfeln ---------------------------------------------------------
TOK=$(head -c 12 /dev/urandom | od -An -tx1 | tr -d ' \n') || TOK=""
[ -n "$TOK" ] || { echo "ABBRUCH: /dev/urandom lieferte nichts -- kein Koeder, kein Test." >&2; exit 2; }
A="koeder_in_tests_$TOK"       # haengt an comdare_tests
B="koeder_nur_all_$TOK"        # haengt nur an 'all'
C="koeder_nirgends_$TOK"       # haengt an keinem von beiden
D="koeder_datenpfad_$TOK"      # ninja kennt dafuer keine Regel

echo "============================================================================="
echo " SELBSTTEST der BAUWEG-WACHE   Koeder-Token (aus /dev/urandom): $TOK"
echo "============================================================================="

BAU="$TMP/bau"
mkdir -p "$BAU"

schreibe_ninja() {
    cat > "$BAU/build.ninja" <<NINJA
rule noop
  command = true
build $A: noop
build $B: noop
build $C: noop
build comdare_tests: phony $A
build all: phony $A $B
NINJA
}

# $1 = Liste der Pfade, die der synthetische Test aufruft
schreibe_testfile() {
    : > "$BAU/CTestTestfile.cmake"
    for P in $1; do
        printf 'add_test([=[selbsttest_%s]=] "%s/%s")\n' "$P" "$BAU" "$P" \
            >> "$BAU/CTestTestfile.cmake"
    done
}

FEHLER=0
GEFAHREN=0

# $1 Fallname  $2 erwarteter Exit  $3 Muster das VORKOMMEN muss (leer = egal)
pruefe() {
    _name="$1"; _soll="$2"; _muster="$3"
    GEFAHREN=$((GEFAHREN + 1))
    set +e
    sh "$WACHE" "$BAU" > "$TMP/lauf.log" 2>&1
    _ist=$?
    set -e
    if [ "$_ist" != "$_soll" ]; then
        echo "FEHLGESCHLAGEN [$_name]: Exit $_ist, erwartet $_soll"
        sed 's/^/    | /' "$TMP/lauf.log"
        FEHLER=$((FEHLER + 1))
        return
    fi
    if [ -n "$_muster" ] && ! grep -q -- "$_muster" "$TMP/lauf.log"; then
        echo "FEHLGESCHLAGEN [$_name]: Exit $_ist stimmt, aber '$_muster' fehlt in der Ausgabe."
        sed 's/^/    | /' "$TMP/lauf.log"
        FEHLER=$((FEHLER + 1))
        return
    fi
    echo "  OK  [$_name] Exit $_ist${_muster:+ , Ausgabe nennt '$_muster'}"
}

# -- 1 GEGENKOEDER: unmanipuliert muss GRUEN sein ----------------------------
schreibe_ninja
schreibe_testfile "$A"
pruefe "1 gruen: alles an comdare_tests" 0 "BAUWEG-WACHE: OK"

# -- 2 KOEDER: haengt nur an 'all' -------------------------------------------
schreibe_testfile "$A $B"
pruefe "2 rot: nur an 'all'" 1 "$B"
grep -q "nur an 'all'" "$TMP/lauf.log" || {
    echo "FEHLGESCHLAGEN [2]: der Befund unterscheidet 'nur all' nicht von 'nirgends'."
    FEHLER=$((FEHLER + 1)); }

# -- 3 KOEDER: haengt nirgends -----------------------------------------------
schreibe_testfile "$A $C"
pruefe "3 rot: weder comdare_tests noch 'all'" 1 "$C"
grep -q "wird nirgends gebaut" "$TMP/lauf.log" || {
    echo "FEHLGESCHLAGEN [3]: 'nirgends' wird nicht als eigene Klasse gemeldet."
    FEHLER=$((FEHLER + 1)); }

# -- 4 GEGENKOEDER: ein Datenpfad darf KEIN Befund sein ----------------------
# Sonst waere die Wache eine Dauerklage ueber Arbeitsverzeichnisse und CSV-Ausgaben.
schreibe_testfile "$A $D"
pruefe "4 gruen: Datenpfad ist kein Befund" 0 "$D"

# -- 5 FAIL-CLOSED: ohne build.ninja darf sie NICHT gruen melden -------------
rm -f "$BAU/build.ninja"
schreibe_testfile "$A"
pruefe "5 abbruch: build.ninja fehlt" 2 "fail-closed"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER: $GEFAHREN Fall/Faelle gefahren, $FEHLER davon abweichend."
echo "  Koeder-Token dieses Laufs: $TOK (bei jedem Lauf neu aus /dev/urandom)."
echo "-----------------------------------------------------------------------------"

if [ "$GEFAHREN" -ne 5 ]; then
    echo "ABBRUCH: $GEFAHREN statt 5 Faelle gefahren -- der Selbsttest ist selbst unvollstaendig." >&2
    exit 2
fi
if [ "$FEHLER" -gt 0 ]; then
    echo "FEHLER: $FEHLER von $GEFAHREN Faellen abweichend -- die Bauweg-Wache misst nicht," >&2
    echo "        was sie zu messen behauptet." >&2
    exit 1
fi

echo "SELBSTTEST BAUWEG-WACHE: OK ($GEFAHREN von $GEFAHREN Faellen wie erwartet)."
exit 0
