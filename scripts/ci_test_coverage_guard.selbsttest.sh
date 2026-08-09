# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- SELBSTTEST der DRITTEN ACHSE (Fremd-Inventur) der ABDECKUNGS-WACHE
#   Prueft: scripts/ci_test_coverage_guard.sh, Befund (e) "Fremd-Inventur"
#   (D1c, 2026-08-09)
# =============================================================================
# WAS DIESES SKRIPT ZUSICHERT:
#   (1) Eine fremde Inventur, die mit der eigenen uebereinstimmt, ist GRUEN.
#   (2) Weicht sie ab, ist die Wache ROT und nennt die Differenz NAMENTLICH.
#   (3) Ist die Datei angekuendigt, aber FEHLT, ist die Wache ROT -- nicht
#       "uebersprungen" (fail-closed).
#   (4) Ist die Datei LEER, ist die Wache ROT (eine leere Gegenquelle ist
#       keine Bestaetigung).
#   (5) Ohne COMDARE_FREMD_INVENTUR meldet die Wache die Achse ausdruecklich
#       als NICHT GEFAHREN -- sie verschwindet nicht still aus dem Bericht.
#
# WAS ES NICHT ZUSICHERT: die uebrigen Befunde (a) Phantom-Gate, (b) toter
# Name, (c) Abdeckungsluecke. Die haben ihre eigene Herkunft und sind hier
# nicht Gegenstand.
#
# WARUM ES DEN KOEDER WUERFELT: ein fest eingetragener Beispielwert kann
# mitaltern -- verschwindet der Testname, waere der Fall stumm gruen. Deshalb
# wird die Schrumpf-Menge bei JEDEM Lauf neu aus der echten Inventur gezogen.
#
# WARUM ES EINEN ECHTEN BAU-BAUM BRAUCHT: die Wache rechnet gegen 'ctest -N'.
# Ein Spielzeug-Baum haette eine andere Inventur als das Manifest erwartet und
# der Selbsttest wuerde Befunde messen, die es im Ernstfall nicht gibt.
#
# AUFRUF:  sh scripts/ci_test_coverage_guard.selbsttest.sh <build-verzeichnis>
# EXIT:    0 = alle Faelle wie erwartet
#          1 = mindestens ein Fall weicht ab
#          2 = ABBRUCH (Baum fehlt / Grundlauf nicht gruen) -- KEIN Gruen
#
# ASCII-only (Leitplanke).
# =============================================================================

set -u

_st_dir=$(dirname "$0")
_st_wache="${_st_dir}/ci_test_coverage_guard.sh"

st_abbruch() {
    echo ""
    echo "SELBSTTEST ABDECKUNGS-WACHE: ABBRUCH -- $1" >&2
    exit 2
}

ST_BUILD=${1:-}
[ -n "$ST_BUILD" ] || st_abbruch "kein Bau-Verzeichnis angegeben. Aufruf: sh $0 <build-verzeichnis>"
[ -f "$_st_wache" ] || st_abbruch "Wache nicht gefunden: ${_st_wache}"
[ -f "${ST_BUILD}/CTestTestfile.cmake" ] || st_abbruch "'${ST_BUILD}' ist kein konfigurierter Bau-Baum."
command -v ctest >/dev/null 2>&1 || st_abbruch "ctest ist nicht im PATH."
command -v shuf  >/dev/null 2>&1 || st_abbruch "shuf ist nicht im PATH."

_st_tmp=$(mktemp -d) || st_abbruch "mktemp -d fehlgeschlagen."
trap 'rm -rf "$_st_tmp"' EXIT INT TERM

echo "============================================================================="
echo " SELBSTTEST der DRITTEN ACHSE (Fremd-Inventur)  (scripts/ci_test_coverage_guard.selbsttest.sh)"
echo " Bau-Baum : ${ST_BUILD}"
echo "============================================================================="

# -- Die echte Inventur ist zugleich die "fremde" Quelle des Gleichstand-Falls --
ctest --test-dir "$ST_BUILD" -N 2>/dev/null \
    | sed -n 's/^ *Test *#[0-9][0-9]*: *//p' | LC_ALL=C sort -u > "${_st_tmp}/echt.txt"
ST_N=$(wc -l < "${_st_tmp}/echt.txt" | tr -d ' ')
[ "$ST_N" -gt 0 ] || st_abbruch "die Inventur meldet 0 Tests -- Baum ohne Tests konfiguriert."
echo ""
echo "INVENTUR des Baums: ${ST_N} Tests (zugleich die 'fremde' Quelle fuer den Gleichstand-Fall)"

# -- GRUNDLAUF: ohne die Achse. Nur wenn der hier gruen ist, laesst sich der ---
# -- RC-BEITRAG der Achse ueberhaupt messen; sonst maskieren andere Befunde. ---
( unset COMDARE_FREMD_INVENTUR; sh "$_st_wache" "$ST_BUILD" ) > "${_st_tmp}/grund.log" 2>&1
ST_GRUND_RC=$?
echo "GRUNDLAUF (Achse ungefahren): RC=${ST_GRUND_RC}"
if [ "$ST_GRUND_RC" -ne 0 ]; then
    echo ""
    echo "Der Grundlauf ist bereits ROT. Dann laesst sich NICHT unterscheiden, ob ein"
    echo "spaeteres ROT von der dritten Achse kommt oder von einem der uebrigen Befunde."
    echo "Letzte Zeilen des Grundlaufs:"
    tail -12 "${_st_tmp}/grund.log"
    st_abbruch "Grundlauf nicht gruen -- der RC-Beitrag der Achse ist nicht messbar."
fi

ST_FEHLER=0
st_fall() {   # $1=Name $2=erwarteter RC $3=Datei-oder-leer  (Rest: Grep-Muster, die vorkommen MUESSEN)
    _f_name=$1; _f_soll=$2; _f_datei=$3; shift 3
    if [ -z "$_f_datei" ]; then
        ( unset COMDARE_FREMD_INVENTUR; sh "$_st_wache" "$ST_BUILD" ) > "${_st_tmp}/${_f_name}.log" 2>&1
    else
        COMDARE_FREMD_INVENTUR="$_f_datei" sh "$_st_wache" "$ST_BUILD" > "${_st_tmp}/${_f_name}.log" 2>&1
    fi
    _f_rc=$?
    _f_ok=ja
    [ "$_f_rc" = "$_f_soll" ] || _f_ok=nein
    for _f_muster in "$@"; do
        grep -q -- "$_f_muster" "${_st_tmp}/${_f_name}.log" || {
            _f_ok=nein
            echo "    fehlendes Muster: ${_f_muster}"
        }
    done
    if [ "$_f_ok" = ja ]; then
        printf '  [ok  ]  %-34s RC=%s (erwartet %s)\n' "$_f_name" "$_f_rc" "$_f_soll"
    else
        printf '  [FEHL]  %-34s RC=%s (erwartet %s)\n' "$_f_name" "$_f_rc" "$_f_soll"
        ST_FEHLER=$(( ST_FEHLER + 1 ))
    fi
}

echo ""
echo "FAELLE:"

# (5) Achse ungefahren -> ausdruecklich gemeldet, kein Einfluss auf den RC
st_fall "ungesetzt_meldet_laut" 0 "" "DRITTE ACHSE (Fremd-Inventur): NICHT GEFAHREN"

# (1) Gleichstand -> gruen
st_fall "gleichstand_gruen" 0 "${_st_tmp}/echt.txt" "deckungsgleich"

# (2) SCHRUMPF-KOEDER, bei JEDEM Lauf frisch gewuerfelt -> rot MIT NAMEN
_st_k=$(( ($(od -An -N1 -tu1 /dev/urandom | tr -d ' ') % 4) + 2 ))
shuf -n "$_st_k" --random-source=/dev/urandom "${_st_tmp}/echt.txt" > "${_st_tmp}/entfernt.txt"
LC_ALL=C grep -vxF -f "${_st_tmp}/entfernt.txt" "${_st_tmp}/echt.txt" > "${_st_tmp}/geschrumpft.txt"
echo "  (Koeder: ${_st_k} Tests frisch gezogen und aus der fremden Inventur entfernt)"
set --
while read -r _st_n; do set -- "$@" "$_st_n"; done < "${_st_tmp}/entfernt.txt"
st_fall "abweichung_rot_mit_namen" 4 "${_st_tmp}/geschrumpft.txt" "ABWEICHUNG" "$@"

# (3) angekuendigt, aber fehlend -> rot (fail-closed)
st_fall "fehlende_datei_rot" 4 "${_st_tmp}/gibt-es-nicht-$$.txt" "Datei fehlt"

# (4) leer -> rot (fail-closed)
: > "${_st_tmp}/leer.txt"
st_fall "leere_datei_rot" 4 "${_st_tmp}/leer.txt" "ist leer"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER: 5 Faelle gefahren, davon ${ST_FEHLER} abweichend."
echo "-----------------------------------------------------------------------------"
if [ "$ST_FEHLER" -eq 0 ]; then
    echo "SELBSTTEST ABDECKUNGS-WACHE (dritte Achse): GRUEN."
    exit 0
fi
echo "SELBSTTEST ABDECKUNGS-WACHE (dritte Achse): ROT."
exit 1
