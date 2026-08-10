#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  ACHSEN-ROUNDTRIP-WACHE -- die drei Registry-Roundtrip-Gates sind GENAU DREI.
#  (AS-Bewaffnung, 2026-08-09)
#  GOAL v8 Teil VIII: V-7 (fremder Nenner), V-1 (nie eine nackte Zahl)
# =============================================================================
#
# WAS SIE ZUSICHERT, als Aussage und nicht als Anwesenheit (T-2):
# Im gebauten Baum stehen als Registry-Roundtrip-Gates GENAU diese drei, nicht zwei
# und nicht vier:
#     test_axis_registry_roundtrip
#     test_system_axis_registry_roundtrip
#     test_measurement_axis_registry_roundtrip
# Sie sind die Gates, die belegen, dass eine Achse durch Schreiben und
# Wiedereinlesen unveraendert zurueckkommt -- fuer die Organ-Achsen, die
# System-Achsen und die Mess-Achsen je einmal. Faellt eines davon lautlos weg,
# faellt die Zusicherung fuer eine ganze Achsen-Familie weg.
#
# WARUM EINE EIGENE WACHE, obwohl es ci_test_sichtbarkeit_wache.sh schon gibt --
# die Luecke ist eine SELBST-INVENTUR und damit strukturell (V-7):
# Jene Wache vergleicht SOLL (ein Scan von tests/unit/CMakeLists.txt) gegen IST
# (`ctest -N` desselben Baums). Loescht jemand einen der drei add_test-Bloecke
# KOMPLETT aus der Quelle, sinken SOLL und IST GEMEINSAM -- die Differenz bleibt
# null, die Wache bleibt gruen, und das Gate ist trotzdem weg. Zwei Zahlen, die
# aus derselben Datei stammen, sind kein Vergleich.
# Diese Wache holt ihren Nenner deshalb NICHT aus dem CMake-Quelltext, sondern
# traegt ihn als LITERAL in sich: die Drei und die drei Namen stehen hier im
# Skript. Wer ein Gate entfernt, muss diese Datei mit anfassen -- und genau das
# ist der Zweck. Eine Loeschung wird damit ein bewusster Akt statt eines
# unbemerkten Nebeneffekts.
#
# WARUM KEINE ZEILEN-ANKER: der Wellenplan nannte die drei Bloecke bei
# tests/unit/CMakeLists.txt:5009/:5039/:5054; am 09.08.2026 standen sie bei
# :5379/:5409/:5424 -- um +370 Zeilen gewandert, in wenigen Tagen. Ein
# Zeilen-Anker waere hier nachweislich das falsche Werkzeug. Gemessen wird am
# gebauten Baum ueber `ctest -N`, also am Gegenstand (V-8), nicht am Quelltext.
#
# DIE SCHREIBWEISE, FUER DIE EINE FRUEHERE ZAEHLWEISE BLIND WAR: die drei Bloecke
# schreiben ihr NAME auf die FOLGEZEILE des add_test( -- eine Zaehlung per
# einzeiligem Muster uebersieht sie. `ctest -N` sieht sie, weil es den fertigen
# Baum liest und nicht den Quelltext. Der Koeder dieser Wache benutzt bewusst
# genau diese Schreibweise.
#
# AUFRUF:  sh scripts/ci_achsen_roundtrip_wache.sh <build-verzeichnis>
# EXIT:    0 = genau die drei erwarteten Gates, kein viertes, keines fehlt
#          1 = Anzahl oder Namensmenge weicht ab
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen)
#
# GRENZE, EHRLICH BENANNT: sie prueft die EXISTENZ und IDENTITAET der drei
# ctest-Eintraege, nicht ihr Ergebnis. Ob sie gruen laufen, sagt `ctest`, nicht
# diese Wache. Sie schliesst die Luecke "das Gate ist verschwunden", nicht die
# Luecke "das Gate ist da und schlaeft".
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

BUILD="${1:-}"
if [ -z "$BUILD" ]; then
    echo "AUFRUF: sh scripts/ci_achsen_roundtrip_wache.sh <build-verzeichnis>" >&2
    exit 2
fi
if [ ! -d "$BUILD" ]; then
    echo "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
    exit 2
fi
if [ ! -f "$BUILD/CTestTestfile.cmake" ]; then
    echo "ABBRUCH: '$BUILD/CTestTestfile.cmake' fehlt -- der Baum ist nicht konfiguriert." >&2
    echo "         Jede Zaehlung waere null und wuerde eine Loeschung vortaeuschen. Fail-closed." >&2
    exit 2
fi
command -v ctest >/dev/null 2>&1 || {
    echo "ABBRUCH: 'ctest' ist nicht im PATH -- die Wache konnte nicht pruefen." >&2
    exit 2
}

GREP=/usr/bin/grep
[ -x "$GREP" ] || GREP=grep

# ---------------------------------------------------------------------------
# DER NENNER, ALS LITERAL. Er stammt ausdruecklich NICHT aus der geprueften
# CMake-Datei -- das ist der ganze Punkt dieser Wache (V-7).
# ---------------------------------------------------------------------------
ERWARTET_N=3
ERWARTET_NAMEN="test_axis_registry_roundtrip
test_measurement_axis_registry_roundtrip
test_system_axis_registry_roundtrip"

MUSTER="axis_registry_roundtrip"

TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

# ---------------------------------------------------------------------------
# MESSGERAET-GEGENPROBE (V4): eine Null aus einem leeren Baum ist keine
# Loeschung, sondern eine kaputte Messung. Erst wenn der Baum ueberhaupt Tests
# kennt, darf ein Nichtfund als Befund gelten.
# ---------------------------------------------------------------------------
ctest --test-dir "$BUILD" -N > "$TMP/alle.txt" 2>&1 || {
    echo "ABBRUCH: 'ctest -N' im Baum '$BUILD' schlug fehl." >&2
    sed 's/^/  /' "$TMP/alle.txt" >&2
    exit 2
}
GESAMT=$("$GREP" -cE '^[[:space:]]*Test[[:space:]]+#[0-9]+:' "$TMP/alle.txt" || true)
if [ "${GESAMT:-0}" -eq 0 ]; then
    echo "ABBRUCH: 'ctest -N' meldet 0 Tests im Baum '$BUILD'." >&2
    echo "         Der IST-Nenner ist leer -- ein Nichtfund waere nicht von einer" >&2
    echo "         Loeschung zu unterscheiden. Fail-closed." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# Die eigentliche Messung. Namen aus der ctest-Zeile "Test #N: <name>".
# ---------------------------------------------------------------------------
ctest --test-dir "$BUILD" -N -R "$MUSTER" > "$TMP/treffer.txt" 2>&1 || {
    echo "ABBRUCH: 'ctest -N -R $MUSTER' im Baum '$BUILD' schlug fehl." >&2
    sed 's/^/  /' "$TMP/treffer.txt" >&2
    exit 2
}
"$GREP" -E '^[[:space:]]*Test[[:space:]]+#[0-9]+:' "$TMP/treffer.txt" |
    sed 's/^.*#[0-9]*:[[:space:]]*//' | sed 's/[[:space:]]*$//' | sort > "$TMP/ist.txt" || true

IST_N=$(wc -l < "$TMP/ist.txt" | tr -d ' ')
printf '%s\n' "$ERWARTET_NAMEN" | sort > "$TMP/soll.txt"

echo "-----------------------------------------------------------------------------"
echo "ACHSEN-ROUNDTRIP-WACHE (AS-Bewaffnung) -- Muster '$MUSTER'"
echo "  Baum      : $BUILD"
echo "  SOLL      : $ERWARTET_N Gate(s), als Literal IN DIESEM SKRIPT (nicht aus CMake)"
echo "  IST       : $IST_N Gate(s) laut 'ctest -N -R'"
echo "-----------------------------------------------------------------------------"
echo "GEFUNDEN:"
if [ "$IST_N" -eq 0 ]; then
    echo "  (keine)"
else
    sed 's/^/  /' "$TMP/ist.txt"
fi

FEHLT=$(comm -23 "$TMP/soll.txt" "$TMP/ist.txt" || true)
ZUVIEL=$(comm -13 "$TMP/soll.txt" "$TMP/ist.txt" || true)

if [ -n "$FEHLT" ]; then
    echo ""
    echo "FEHLT (im SOLL, nicht im Baum) -- ein Gate ist verschwunden:"
    printf '%s\n' "$FEHLT" | sed 's/^/  /'
fi
if [ -n "$ZUVIEL" ]; then
    echo ""
    echo "UNERWARTET (im Baum, nicht im SOLL) -- ein Gate ist dazugekommen:"
    printf '%s\n' "$ZUVIEL" | sed 's/^/  /'
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Zahl):"
echo "  $IST_N von $ERWARTET_N erwarteten Roundtrip-Gates im Baum '$BUILD'."
echo "  Grundgesamtheit der Messung: $GESAMT ctest-Eintrag/Eintraege insgesamt."
echo "  Differenz: $(printf '%s' "$FEHLT" | "$GREP" -c . || true) fehlend, \
$(printf '%s' "$ZUVIEL" | "$GREP" -c . || true) unerwartet."
echo "-----------------------------------------------------------------------------"

if [ "$IST_N" -ne "$ERWARTET_N" ] || [ -n "$FEHLT" ] || [ -n "$ZUVIEL" ]; then
    echo "ACHSEN-ROUNDTRIP-WACHE: ROT -- $IST_N Gate(s) statt $ERWARTET_N."
    echo "Wer ein Gate absichtlich entfernt oder hinzufuegt, aendert die Literale"
    echo "ERWARTET_N/ERWARTET_NAMEN in diesem Skript MIT -- das ist der Zweck, nicht"
    echo "die Huerde: eine Aenderung an dieser Zusicherung soll sichtbar sein."
    exit 1
fi

echo "ACHSEN-ROUNDTRIP-WACHE: OK ($IST_N von $ERWARTET_N Gates, aus $GESAMT ctest-Eintraegen)."
exit 0
