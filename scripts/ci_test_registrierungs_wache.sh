#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  TEST-REGISTRIERUNGS-WACHE -- keine Test-QUELLDATEI darf ausserhalb des
#  Bauwegs liegen. (MT-L4, 2026-08-09)
#  GOAL v8 Teil VIII, T-7: "Ein Test existiert erst, wenn er in `ctest -N`
#  erscheint UND sein Binary im Bauweg haengt."
# =============================================================================
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST (am Objekt gemessen, 09.08.2026, ce aebc4f2c):
# Vier Test-Quelldateien lagen im Repository, waren getrackt, waren uebersetzbar --
# und kamen in KEINER CMakeLists.txt vor. Sie wurden nie gebaut, nie ausgefuehrt,
# nie gezaehlt. Gemessen: je 0 Treffer in tests/unit/CMakeLists.txt.
#   tests/unit/test_a9b_active_deklaration_inert.cpp     seit 26.07.2026 (14 Tage)
#   tests/unit/test_c3b_kanal_merge_beleg.cpp            seit 26.07.2026 (14 Tage)
#   tests/unit/test_rf2_admission_marker_inert.cpp       seit 26.07.2026 (14 Tage)
#   tests/unit/test_d4b_container_dll.cpp                seit 02.06.2026 (68 Tage)
# Der Erstlauf von rf2 war ROT: eine Zusicherung stand seit 26.07. auf einer
# CSV-Zellzahl, die der Renderer inzwischen geaendert hat. 14 Tage lang hat das
# niemand bemerkt, weil die Datei nie gebaut wurde.
#
# WARUM DIE VORHANDENE SICHTBARKEITS-WACHE DAS NICHT FINDET -- die Luecke ist strukturell:
# scripts/ci_test_sichtbarkeit_wache.sh vergleicht SOLL (die Registrierungs-AUFRUFE im
# CMake-Quelltext) gegen IST (`ctest -N`). Eine Datei, die gar kein add_test aufruft,
# taucht in ihrem SOLL nie auf -- sie kann von ihr nicht vermisst werden. Beide Wachen
# pruefen deshalb verschiedene Stufen derselben Kette und ersetzen einander NICHT:
#   ci_test_sichtbarkeit_wache.sh  Registrierung   -> ctest-Eintrag   (Stufe 2)
#   ci_test_registrierungs_wache.sh  Quelldatei    -> Bauweg          (Stufe 1, hier)
#
# DER NENNER KOMMT VON AUSSEN (V-7 / T-3). Beide Zahlen dieser Wache stammen aus
# verschiedenen Quellen, und KEINE der beiden ist die CMake-Datei, die geprueft wird:
#   SOLL  `git ls-files` -- der Git-Index. Faellt eine Registrierung aus der
#         CMakeLists.txt, sinkt der SOLL NICHT mit. Genau das ist der Punkt: eine
#         Selbst-Inventur gegen einen Selbst-Selektor waere kein Vergleich.
#   IST   der GEBAUTE Baum (compile_commands.json, sonst build.ninja) -- also der
#         Gegenstand, nicht die Ankuendigung. Ein comdare_add_test(), das hinter einer
#         falschen Bedingung steht, steht im Quelltext und trotzdem nicht im Bauweg;
#         nur der gebaute Baum weiss das.
#
# WARUM NICHT PER TEXT-GREP GEGEN tests/unit/CMakeLists.txt: das beantwortet die
# falsche Frage. Am Objekt gemessen (09.08.2026) stehen vier Dateien
# (test_concepts_compile, test_value_handle, test_three_layer_audit,
# test_six_page_structures) MIT comdare_add_test() im Quelltext -- und sind trotzdem
# nicht im Bauweg, weil ihr if(COMDARE_PRT_ART_LEGACY_AVAILABLE) im Standalone-Bau
# falsch ist. Ein Quelltext-Grep haette sie als "registriert" durchgewinkt.
#
# WAS SIE VERLANGT -- Rechenschaft, nicht Vollstaendigkeit:
# Nicht jede Test-Quelldatei MUSS in jedem Baum gebaut werden; ein Test hinter einem
# optionalen Fremdbaum ist dort legitim abwesend. Verlangt wird, dass jede Abwesenheit
# BEGRUENDET in scripts/ci_test_registrierungs_allowlist.txt steht -- und dass die
# Begruendung noch gilt.
#
# DIE BEGRUENDUNG WIRD AM GEGENSTAND NACHGEPRUEFT (V-8), nicht geglaubt: jede
# Allowlist-Zeile nennt eine DATEI, deren ABWESENHEIT die Ausnahme traegt. Existiert
# diese Datei im Baum, ist die Ausnahme erloschen und die Zeile wird ROT -- auch wenn
# niemand die Allowlist angefasst hat. Eine Allowlist ohne diese Gegenprobe waere ein
# Freibrief mit unbegrenzter Laufzeit.
#
# AUFRUF:  sh scripts/ci_test_registrierungs_wache.sh <build-verzeichnis>
# EXIT:    0 = jede getrackte Test-Quelldatei ist im Bauweg oder begruendet abwesend
#          1 = mindestens eine Test-Quelldatei OHNE gueltige Begruendung ausserhalb
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen)
#
# GRENZE, EHRLICH BENANNT -- was diese Wache NICHT deckt:
# Sie prueft, ob die Quelldatei UEBERSETZT wird. Sie prueft NICHT, ob das entstehende
# Binary auch als ctest-Eintrag registriert ist -- das ist Stufe 2 und Sache von
# ci_test_sichtbarkeit_wache.sh. Eine Datei kann also hier gruen sein und trotzdem
# keinen ctest-Eintrag haben; erst BEIDE Wachen zusammen schliessen die Kette
# "Datei -> Uebersetzung -> ctest-Eintrag".
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

BUILD="${1:-}"
if [ -z "$BUILD" ]; then
    echo "AUFRUF: sh scripts/ci_test_registrierungs_wache.sh <build-verzeichnis>" >&2
    exit 2
fi
if [ ! -d "$BUILD" ]; then
    echo "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
    exit 2
fi

WURZEL=$(git rev-parse --show-toplevel 2>/dev/null) || {
    echo "ABBRUCH: kein git-Arbeitsbaum -- der SOLL-Nenner ist nicht erhebbar." >&2
    exit 2
}
cd "$WURZEL" || exit 2

# Absoluter Pfad des Bau-Baums: build.ninja fuehrt Quellen absolut.
BUILD_ABS=$(cd "$BUILD" && pwd) || exit 2

GREP=/usr/bin/grep
[ -x "$GREP" ] || GREP=grep

# ---------------------------------------------------------------------------
# IST-QUELLE: der GEBAUTE Baum. compile_commands.json ist generator-unabhaengig
# und wird bevorzugt; build.ninja ist der Fallback (der CI baut mit
# --with-generator=Ninja). Findet sich keine von beiden, ist das FAIL-CLOSED --
# eine Wache, die nicht pruefen kann, meldet nicht "in Ordnung".
# ---------------------------------------------------------------------------
IST_DATEI=""
IST_ART=""
if [ -f "$BUILD_ABS/compile_commands.json" ]; then
    IST_DATEI="$BUILD_ABS/compile_commands.json"
    IST_ART="compile_commands.json"
elif [ -f "$BUILD_ABS/build.ninja" ]; then
    IST_DATEI="$BUILD_ABS/build.ninja"
    IST_ART="build.ninja"
else
    echo "ABBRUCH: weder compile_commands.json noch build.ninja unter '$BUILD'." >&2
    echo "         Der IST-Nenner waere leer -- jede Datei erschiene als unregistriert." >&2
    echo "         Das ist fail-closed und ausdruecklich KEIN Gruen." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# MESSGERAET-GEGENPROBE (V4): bevor ein Nullbefund etwas bedeutet, muss belegt
# sein, dass das Werkzeug ueberhaupt sucht. Eine Datei, von der wir wissen, dass
# sie gebaut wird, MUSS treffen. Trifft sie nicht, ist nicht der Baum kaputt,
# sondern die Messung -- und dann darf diese Wache kein Urteil faellen.
# ---------------------------------------------------------------------------
GEGENPROBE="tests/unit/test_pressure_state.cpp"
if [ ! -f "$GEGENPROBE" ]; then
    echo "ABBRUCH: die Gegenprobe-Datei '$GEGENPROBE' existiert nicht mehr." >&2
    echo "         Ohne Gegenprobe ist ein Nullbefund nicht von einem kaputten Muster zu trennen." >&2
    exit 2
fi
if ! "$GREP" -q -F -- "/$GEGENPROBE" "$IST_DATEI"; then
    echo "ABBRUCH: die Gegenprobe '$GEGENPROBE' steht NICHT in $IST_ART." >&2
    echo "         Entweder ist der Baum nicht konfiguriert, oder das Suchmuster trifft nicht." >&2
    echo "         In beiden Faellen waere jeder Nichtfund dieser Wache wertlos." >&2
    exit 2
fi

ALLOWLIST="scripts/ci_test_registrierungs_allowlist.txt"

# ---------------------------------------------------------------------------
# SOLL: alle getrackten Test-Quelldateien im GANZEN Baum, ohne den vendorierten
# ext/-Baum (fremder Code, fremde Bauwege).
#
# WARUM REPO-WEIT UND NICHT NUR tests/: die zwei Dateien, an denen dieser ganze
# Befund haengt, liegen gar nicht unter tests/ --
# libs/cache_engine/builder/commands/tests/test_commands.cpp und
# test_engine_adapters.cpp. Ihre 27 gtest-Faelle waren 79 Tage lang in jedem Baum
# unsichtbar (W-1: enable_testing() stand nach dem add_subdirectory). Eine Wache
# gegen unsichtbare Tests, die ausgerechnet diese beiden nicht im Nenner hat,
# waere eine Wache mit einem Loch an genau der Stelle des Vorfalls.
# Am Objekt gemessen (09.08.2026): 459 Dateien insgesamt, 456 unter tests/ und
# 3 darunter -- die zwei oben plus
# libs/cache_engine/builder/workload_driver/test_load_profile_writer.cpp.
#
# Das Muster ist am DATEINAMEN verankert, nicht am Pfad, und zwar bewusst nach
# `git ls-files` statt als Pathspec: in einem git-Pathspec matcht '*' auch '/',
# weshalb '*/test_*.cpp' auch
# libs/test_infra/workload_generator/src/workload_generator.cpp trifft -- eine
# Datei, die kein Test ist. Am Objekt geprueft: Pathspec 460 Treffer,
# Basename-Filter 459.
# ---------------------------------------------------------------------------
TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

git ls-files 2>/dev/null | "$GREP" -v '^ext/' | "$GREP" -v '/ext/' |
    "$GREP" -E '(^|/)test_[^/]*\.cpp$' | sort > "$TMP/soll.txt" || true

SOLL_N=$(wc -l < "$TMP/soll.txt" | tr -d ' ')
if [ "$SOLL_N" -eq 0 ]; then
    echo "ABBRUCH: der SOLL ist leer -- 'git ls-files' fand keine Test-Quelldatei." >&2
    echo "         Ein leerer Nenner macht jede Aussage wahr. Fail-closed." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# Abgleich: liegt der repo-relative Pfad, mit '/' davor verankert, im IST?
# Die Verankerung ist noetig und wurde am Objekt geprueft: ohne sie wuerde
# '/test_value_handle.cpp' faelschlich in '/test_value_handle_real.cpp'
# treffen (gemessen 09.08.2026: 0 vs. 10 Treffer -- die Verankerung trennt).
# ---------------------------------------------------------------------------
: > "$TMP/fehlend.txt"
while IFS= read -r f; do
    [ -n "$f" ] || continue
    if ! "$GREP" -q -F -- "/$f" "$IST_DATEI"; then
        printf '%s\n' "$f" >> "$TMP/fehlend.txt"
    fi
done < "$TMP/soll.txt"

FEHLEND_N=$(wc -l < "$TMP/fehlend.txt" | tr -d ' ')

# ---------------------------------------------------------------------------
# Allowlist auswerten. Format je Zeile, drei Felder, '|'-getrennt:
#   <test-quelldatei> | <gegenstand-der-fehlen-muss> | <begruendung>
# Feld 2 ist die Gegenprobe der Begruendung: existiert diese Datei, traegt die
# Ausnahme nicht mehr und die Zeile wird ROT.
# ---------------------------------------------------------------------------
: > "$TMP/begruendet.txt"
: > "$TMP/unbegruendet.txt"
: > "$TMP/erloschen.txt"

allow_zeile() {
    # $1 = gesuchte Datei; gibt die Allowlist-Zeile aus oder nichts
    [ -f "$ALLOWLIST" ] || return 0
    while IFS= read -r z; do
        case "$z" in ''|'#'*) continue ;; esac
        _d=$(printf '%s' "$z" | cut -d'|' -f1 | sed 's/[[:space:]]*$//;s/^[[:space:]]*//')
        [ "$_d" = "$1" ] || continue
        printf '%s\n' "$z"
        return 0
    done < "$ALLOWLIST"
}

while IFS= read -r f; do
    [ -n "$f" ] || continue
    z=$(allow_zeile "$f")
    if [ -z "$z" ]; then
        printf '%s\n' "$f" >> "$TMP/unbegruendet.txt"
        continue
    fi
    geg=$(printf '%s' "$z" | cut -d'|' -f2 | sed 's/[[:space:]]*$//;s/^[[:space:]]*//')
    txt=$(printf '%s' "$z" | cut -d'|' -f3- | sed 's/^[[:space:]]*//')
    if [ -n "$geg" ] && [ -e "$geg" ]; then
        printf '%s -- ERLOSCHEN: "%s" existiert wieder, die Ausnahme traegt nicht mehr\n' \
            "$f" "$geg" >> "$TMP/erloschen.txt"
    else
        printf '%s -- %s (Gegenstand abwesend: %s)\n' "$f" "$txt" "$geg" >> "$TMP/begruendet.txt"
    fi
done < "$TMP/fehlend.txt"

BEGR_N=$(wc -l < "$TMP/begruendet.txt" | tr -d ' ')
UNBEGR_N=$(wc -l < "$TMP/unbegruendet.txt" | tr -d ' ')
ERL_N=$(wc -l < "$TMP/erloschen.txt" | tr -d ' ')

echo "-----------------------------------------------------------------------------"
echo "TEST-REGISTRIERUNGS-WACHE (MT-L4) -- Quelldatei gegen Bauweg"
echo "  SOLL-Quelle: git ls-files (Git-Index)"
echo "  IST-Quelle : $IST_ART im Baum '$BUILD'"
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ]; then
    echo ""
    echo "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS -- diese Dateien werden nie uebersetzt:"
    sed 's/^/  /' "$TMP/unbegruendet.txt"
fi
if [ "$ERL_N" -gt 0 ]; then
    echo ""
    echo "AUSNAHME ERLOSCHEN -- die Begruendung gilt am Gegenstand nicht mehr:"
    sed 's/^/  /' "$TMP/erloschen.txt"
fi
if [ "$BEGR_N" -gt 0 ]; then
    echo ""
    echo "ABWESEND, ABER BEGRUENDET (Allowlist $ALLOWLIST):"
    sed 's/^/  /' "$TMP/begruendet.txt"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Zahl):"
echo "  $SOLL_N getrackte Test-Quelldatei(en) im Baum (ohne ext/) -- SOLL."
echo "  $FEHLEND_N davon NICHT im Bauweg des Baums '$BUILD'."
echo "  davon $BEGR_N begruendet, $ERL_N mit ERLOSCHENER Begruendung, $UNBEGR_N ohne Begruendung."
echo "  Gegenprobe des Messgeraets: '$GEGENPROBE' trifft in $IST_ART (das Muster sucht)."
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ] || [ "$ERL_N" -gt 0 ]; then
    echo "TEST-REGISTRIERUNGS-WACHE: ROT ($UNBEGR_N von $SOLL_N ohne Begruendung, $ERL_N erloschen)."
    echo "Abhilfe: die Datei per comdare_add_test()/add_test() in den Bauweg nehmen -- ODER"
    echo "         sie mit einem nachpruefbaren Gegenstand in $ALLOWLIST begruenden."
    exit 1
fi

echo "TEST-REGISTRIERUNGS-WACHE: OK ($SOLL_N Quelldateien, $UNBEGR_N ohne Begruendung ausserhalb)."
exit 0
