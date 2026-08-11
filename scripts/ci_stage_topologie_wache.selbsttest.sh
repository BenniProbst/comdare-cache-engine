# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- SELBSTTEST der STUFEN-TOPOLOGIE-WACHE
#   Prueft: scripts/ci_stage_topologie_wache.sh                  (2026-08-11)
# =============================================================================
# WAS DIESES SKRIPT ZUSICHERT:
#   (1) Die ECHTE .gitlab-ci.yml ist gruen -- und zwar ueber einem NICHT leeren
#       Nenner (die Wache nennt die Kantenzahl, der Fall prueft sie mit).
#   (2) KOEDER: eine Kopie mit ZURUECKGETAUSCHTEN Stufen (contract wieder vor
#       test) macht die Wache ROT und benennt die Kante MIT Namen und Stufen.
#       Das ist der eigentliche Grund dieser Wache: der Stufen-Tausch dieses
#       Commits ist nur zulaessig, WEIL keine Kante vorwaerts zeigt -- und
#       dieser Fall zeigt, dass die Wache den Unterschied ueberhaupt sieht.
#   (3) GEGENKOEDER gegen Dauer-Rot: dieselbe Kopie mit wieder RICHTIGER
#       Reihenfolge ist wieder gruen. Ohne ihn koennte (2) auch von einem
#       kaputten Parser kommen.
#   (4) FAIL-CLOSED: eine YAML ohne needs-Kanten ist rc=2, nicht gruen.
#       0 Kanten waeren zwangslaeufig 0 vorwaerts -- eine Null ohne Nenner.
#   (5) FAIL-CLOSED: ein Job, dessen Stufe nicht aufloesbar ist, ist rc=2.
#       Ein uebersprungener Job koennte die Vorwaerts-Kante gerade verbergen.
#   (6) FAIL-CLOSED: eine Allowlist-Zeile ohne Begruendung ist rc=2.
#
# WARUM DER KOEDER GEWUERFELT WIRD: der Fall (5) haengt an einem Job-Namen. Ein
# fest eingetragener Name kann mitaltern -- verschwindet der Job, waere der Fall
# stumm gruen. Deshalb wird der Phantom-Job bei JEDEM Lauf neu gewuerfelt.
#
# WAS ES NICHT ZUSICHERT: dass die Stufenreihenfolge fachlich sinnvoll ist, und
# dass ein needs-ZIEL ueberhaupt existiert (andere Fehlerklasse, s. Kopf der
# Wache). Es prueft auch nicht die dritte Achse der Abdeckungs-Wache -- dafuer
# steht scripts/ci_test_coverage_guard.selbsttest.sh, der einen Bau-Baum braucht.
#
# AUFRUF:  sh scripts/ci_stage_topologie_wache.selbsttest.sh [<yaml>]
# EXIT:    0 = alle Faelle wie erwartet
#          1 = mindestens ein Fall weicht ab
#          2 = ABBRUCH (Wache fehlt, Koeder griff nicht) -- KEIN Gruen
#
# ASCII-only (Leitplanke). POSIX sh.
# =============================================================================

set -u

_st_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 2
_st_repo=$(CDPATH= cd -- "${_st_dir}/.." && pwd) || exit 2
_st_wache="${_st_dir}/ci_stage_topologie_wache.sh"
ST_YAML=${1:-${_st_repo}/.gitlab-ci.yml}

st_abbruch() {
    echo ""
    echo "SELBSTTEST STUFEN-TOPOLOGIE: ABBRUCH -- $1" >&2
    exit 2
}

[ -f "$_st_wache" ] || st_abbruch "Wache nicht gefunden: ${_st_wache}"
[ -f "$ST_YAML" ]   || st_abbruch "YAML nicht gefunden: ${ST_YAML}"
command -v od >/dev/null 2>&1 || st_abbruch "od ist nicht im PATH."

_st_tmp=$(mktemp -d) || st_abbruch "mktemp -d fehlgeschlagen."
trap 'rm -rf "$_st_tmp"' EXIT INT TERM

echo "============================================================================="
echo " SELBSTTEST der STUFEN-TOPOLOGIE-WACHE"
echo "  Wache : ${_st_wache}"
echo "  YAML  : ${ST_YAML}"
echo "============================================================================="

ST_FEHLER=0
ST_FAELLE=0

# st_fall <name> <erwarteter-rc> <yaml> <allowlist-oder-leer> <muster...>
st_fall() {
    _f_name=$1; _f_soll=$2; _f_yaml=$3; _f_allow=$4; shift 4
    ST_FAELLE=$(( ST_FAELLE + 1 ))
    _f_log="${_st_tmp}/${_f_name}.log"
    if [ -z "$_f_allow" ]; then
        sh "$_st_wache" "$_f_yaml" > "$_f_log" 2>&1
    else
        COMDARE_STAGE_ALLOWLIST="$_f_allow" sh "$_st_wache" "$_f_yaml" > "$_f_log" 2>&1
    fi
    _f_rc=$?
    _f_ok=ja
    [ "$_f_rc" = "$_f_soll" ] || _f_ok=nein
    for _f_muster in "$@"; do
        grep -qF -- "$_f_muster" "$_f_log" || {
            _f_ok=nein
            echo "    fehlendes Muster: ${_f_muster}"
        }
    done
    if [ "$_f_ok" = ja ]; then
        printf '  [ok  ]  %-32s RC=%s (erwartet %s)\n' "$_f_name" "$_f_rc" "$_f_soll"
    else
        printf '  [FEHL]  %-32s RC=%s (erwartet %s)\n' "$_f_name" "$_f_rc" "$_f_soll"
        sed 's/^/          | /' "$_f_log"
        ST_FEHLER=$(( ST_FEHLER + 1 ))
    fi
}

# -- NENNER ZUERST: wie viele Kanten hat die echte Datei? ---------------------
# Ohne diese Zahl waere jedes "0 vorwaerts" unten nicht einzuordnen.
sh "$_st_wache" "$ST_YAML" > "${_st_tmp}/grund.log" 2>&1
ST_GRUND_RC=$?
ST_KANTEN=$(sed -n 's/^KANTEN: \([0-9][0-9]*\) needs.*/\1/p' "${_st_tmp}/grund.log")
[ -n "$ST_KANTEN" ] || st_abbruch "die Wache meldet keine Kantenzahl -- Ausgabeformat geaendert?"
[ "$ST_KANTEN" -ge 1 ] || st_abbruch "die echte Datei hat 0 Kanten -- jeder Fall unten waere gegenstandslos."
echo ""
echo "NENNER der echten Datei: ${ST_KANTEN} needs-Kante(n), Grundlauf RC=${ST_GRUND_RC}"
if [ "$ST_GRUND_RC" -ne 0 ]; then
    st_abbruch "der Grundlauf ist schon rot -- der Beitrag der Koeder waere nicht messbar."
fi

echo ""
echo "FAELLE:"

# (1) die echte Datei -> gruen, ueber einem nicht leeren Nenner
st_fall "echte_datei_gruen" 0 "$ST_YAML" "" \
    "NENNER: 0 von ${ST_KANTEN} Kanten zeigen VORWAERTS." "STUFEN-TOPOLOGIE-WACHE: GRUEN."

# (2) KOEDER: Stufen zurueckgetauscht -> rot, Kante namentlich
# Der Tausch wird am INHALT vorgenommen, nicht an Zeilennummern, und danach
# GEGENGEPRUEFT: griff er nicht, ist der Fall gegenstandslos (Abbruch).
awk '
  /^[ \t]+- test([ \t]|$)/ && !t { print "  - contract"; t = 1; next }
  /^[ \t]+- contract[ \t]*$/ && t && !c { print "  - test"; c = 1; next }
  { print }
' "$ST_YAML" > "${_st_tmp}/getauscht.yml"
_st_stufen='/^stages:/{f=1;next} f&&/^[ \t]+- /{printf "%s ", $2} f&&/^[^ \t#]/{exit}'
_st_vorher=$(awk "$_st_stufen" "$ST_YAML")
_st_nachher=$(awk "$_st_stufen" "${_st_tmp}/getauscht.yml")
echo "  (Stufen vorher : ${_st_vorher})"
echo "  (Stufen nachher: ${_st_nachher})"
[ "$_st_vorher" != "$_st_nachher" ] \
    || st_abbruch "der Stufen-Tausch griff NICHT -- ein nicht greifender Koeder belegt nichts."
st_fall "stufen_zurueckgetauscht_rot" 1 "${_st_tmp}/getauscht.yml" "" \
    "[VORWAERTS]" "test:coverage-guard" "test:unit" \
    "NENNER: 1 von ${ST_KANTEN} Kanten zeigen VORWAERTS."

# (3) GEGENKOEDER: dieselbe Kopie wieder richtig herum -> wieder gruen
awk '
  /^[ \t]+- contract[ \t]*$/ && !c { print "  - test"; c = 1; next }
  /^[ \t]+- test[ \t]*$/ && c && !t { print "  - contract"; t = 1; next }
  { print }
' "${_st_tmp}/getauscht.yml" > "${_st_tmp}/zurueck.yml"
st_fall "gegenkoeder_wieder_gruen" 0 "${_st_tmp}/zurueck.yml" "" \
    "NENNER: 0 von ${ST_KANTEN} Kanten zeigen VORWAERTS."

# (4) FAIL-CLOSED: keine Kanten -> rc=2, nicht gruen
grep -v 'needs:' "$ST_YAML" > "${_st_tmp}/ohne_needs.yml"
st_fall "ohne_kanten_abbruch" 2 "${_st_tmp}/ohne_needs.yml" "" \
    "0 needs-Kanten gefunden"

# (5) FAIL-CLOSED: unaufloesbare Stufe -> rc=2. Der Phantom-Job wird gewuerfelt,
# damit der Fall nicht an einem Namen haengt, der wegaltern kann.
_st_tok=$(od -An -tx1 -N4 /dev/urandom | tr -d ' \n')
[ -n "$_st_tok" ] || st_abbruch "/dev/urandom lieferte nichts -- Koeder unmoeglich."
{
    cat "$ST_YAML"
    echo ""
    echo "phantom_${_st_tok}:"
    echo "  script:"
    echo "    - echo phantom"
    echo "  needs: [\"lint:secrets\"]"
} > "${_st_tmp}/phantom.yml"
echo "  (gewuerfelter Phantom-Job ohne stage: phantom_${_st_tok})"
st_fall "unaufloesbare_stufe_abbruch" 2 "${_st_tmp}/phantom.yml" "" \
    "UNAUFLOESBAR" "phantom_${_st_tok}"

# (6) FAIL-CLOSED: Allowlist-Zeile ohne Begruendung -> rc=2
printf '.test test\n' > "${_st_tmp}/allow_ohne_grund.txt"
st_fall "allowlist_ohne_grund_abbruch" 2 "$ST_YAML" "${_st_tmp}/allow_ohne_grund.txt" \
    "ALLOWLIST-FEHLER"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER: ${ST_FAELLE} Faelle gefahren, davon ${ST_FEHLER} abweichend."
echo "-----------------------------------------------------------------------------"
if [ "$ST_FEHLER" -eq 0 ]; then
    echo "SELBSTTEST STUFEN-TOPOLOGIE: GRUEN."
    exit 0
fi
echo "SELBSTTEST STUFEN-TOPOLOGIE: ROT." >&2
exit 1
