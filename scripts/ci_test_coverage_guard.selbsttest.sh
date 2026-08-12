# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- SELBSTTEST der DRITTEN ACHSE (Fremd-Inventur) der ABDECKUNGS-WACHE
#   Prueft: scripts/ci_test_coverage_guard.sh, Befund (e) "Fremd-Inventur"
#   (D1c, 2026-08-09; F1a/F1b-Erweiterung 2026-08-12)
# =============================================================================
# WAS DIESES SKRIPT ZUSICHERT (Fall-Nummern = Ausgabe unten):
#   (1) Eine fremde Inventur, die mit der eigenen uebereinstimmt, ist GRUEN.
#   (2) Weicht sie ab, ist die Wache ROT (4) und nennt die Differenz NAMENTLICH.
#   (3) Ist die Datei angekuendigt, aber FEHLT, ist die Wache ROT (fail-closed).
#   (4) Ist die Datei LEER, ist die Wache ROT (keine Bestaetigung aus Nichts).
#   (5) Ohne COMDARE_FREMD_INVENTUR meldet die Wache die Achse ausdruecklich
#       als NICHT GEFAHREN -- sie verschwindet nicht still aus dem Bericht.
#   (6) Ein DEKLARIERTES SOLL-DELTA (COMDARE_FREMD_SOLL_DELTA_BLOECKE, Block
#       AKTIV, Namen nur hier) wird abgezogen UND ausgewiesen -> GRUEN.
#   (7) Fehlen im Fremden ZUSAETZLICH gewuerfelte Namen, ist das ROT (4) MIT
#       diesen Namen -- das Delta deckt nur, was deklariert ist.
#   (8) Eine STALE Deklaration (Block-Namen stehen auch im Fremden) ist ROT (4)
#       und benannt -- ein Abzug, der nichts abzieht, wird nicht geschluckt.
#   (9) Eine KLASSEN-Differenz (synthetische Fremd-Probe einer anderen Sprosse)
#       ist GRUEN, wenn die Differenz exakt der Klassenleiter entspricht
#       (Leiter-Ausweis), und ROT (4) bei K+-1 (Richtung gewuerfelt).
#  (10) Eine angekuendigte, aber fehlende HOST-PROBE ist ROT (4) -- die Probe
#       ist PFLICHT, sobald die Achse faehrt.
#
# WAS ES NICHT ZUSICHERT: die uebrigen Befunde (a) Phantom-Gate, (b) toter
# Name, (c) Abdeckungsluecke. Die haben ihre eigene Herkunft und sind hier
# nicht Gegenstand.
#
# WARUM ES DEN KOEDER WUERFELT: ein fest eingetragener Beispielwert kann
# mitaltern -- verschwindet der Testname, waere der Fall stumm gruen. Deshalb
# werden Schrumpf-Menge, Extra-Namen, SOLL-DELTA-Block und die K+-1-Richtung
# bei JEDEM Lauf neu gezogen (K13: der Koeder muss beissen, beidseitig).
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

# -- Die EIGENE Host-Probe: dieselben Cache-Zeilen, die die Wache selbst liest --
# Seit F1b ist die Probe PFLICHT, sobald COMDARE_FREMD_INVENTUR gesetzt ist; die
# Bestandsfaelle (1)-(4) fahren deshalb mit der Probe des EIGENEN Baums (Klassen
# gleich) -- sonst mass jeder Fall die Probe-Pflicht statt seines Gegenstands.
_st_probe_eigen="${_st_tmp}/probe_eigen.txt"
sed -n '/^COMDARE_HOST_RUNS_AVX2:/p;/^COMDARE_HOST_RUNS_AVX2_COMPILED:/p;/^COMDARE_HOST_RUNS_AVX2_EXITCODE:/p;/^COMDARE_HOST_RUNS_AVX512F:/p;/^COMDARE_HOST_RUNS_AVX512F_COMPILED:/p;/^COMDARE_HOST_RUNS_AVX512F_EXITCODE:/p' \
    "${ST_BUILD}/CMakeCache.txt" > "$_st_probe_eigen"
echo "HOST=$( (hostname 2>/dev/null || echo unbekannt) | sed -n '1p')" >> "$_st_probe_eigen"
_st_probe_zeilen=$(wc -l < "$_st_probe_eigen" | tr -d ' ')
[ "$_st_probe_zeilen" -eq 7 ] || st_abbruch "eigene Host-Probe hat ${_st_probe_zeilen} Zeilen, erwartet 7 (6 Cache-Zeilen + HOST) -- CMakeCache ohne ISA-Probe?"

# -- GRUNDLAUF: ohne die Achse (und ohne ihre F1-Zusatz-Auskuenfte). Nur wenn ---
# -- der gruen ist, laesst sich der RC-BEITRAG der Achse ueberhaupt messen. -----
( unset COMDARE_FREMD_INVENTUR COMDARE_FREMD_HOST_PROBE COMDARE_FREMD_SOLL_DELTA_BLOECKE
  sh "$_st_wache" "$ST_BUILD" ) > "${_st_tmp}/grund.log" 2>&1
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

# -- Die EIGENE Klasse: aus der Ausgabe des Grundlaufs, nicht neu hergeleitet --
# (eine zweite Ableitung hier waere eine Abschrift, die driften kann).
_st_eigen_klasse=$(sed -n 's/^HOST-KLASSE (gemessen, .*): //p' "${_st_tmp}/grund.log" | sed -n '1p')
[ -n "$_st_eigen_klasse" ] || st_abbruch "eigene Host-Klasse nicht aus dem Grundlauf lesbar."

# -- Der SOLL-DELTA-Block fuer (6)-(8): AKTIV, mit Tests, FRISCH GEWUERFELT ----
_st_prot="${ST_BUILD}/comdare_registrierungs_protokoll.txt"
[ -f "$_st_prot" ] || st_abbruch "Registrierungs-Protokoll fehlt: ${_st_prot} ('make inventar' fahren)."
sed -n 's/^BLOCK|//p' "$_st_prot" | awk -F'|' '$2=="AKTIV" && $3!="-" && $3!="" {print}' > "${_st_tmp}/aktive_bloecke.txt"
[ -s "${_st_tmp}/aktive_bloecke.txt" ] || st_abbruch "kein AKTIVer Block mit Tests im Protokoll -- die Faelle (6)-(9) haetten keinen Gegenstand."
_st_block_zeile=$(shuf -n 1 --random-source=/dev/urandom "${_st_tmp}/aktive_bloecke.txt")
_st_block=$(printf '%s\n' "$_st_block_zeile" | awk -F'|' '{print $1}')
printf '%s\n' "$_st_block_zeile" | awk -F'|' '{print $3}' | tr ',' '\n' \
    | sed '/^$/d;/^-$/d' | LC_ALL=C sort -u > "${_st_tmp}/blocktests.txt"
_st_block_n=$(wc -l < "${_st_tmp}/blocktests.txt" | tr -d ' ')
echo "SOLL-DELTA-Block (gewuerfelt): '${_st_block}' mit ${_st_block_n} Test(s)"

# fremde Basis-Inventur der Delta-Faelle: echt MINUS Block-Tests (simuliert den
# Baum, der den Block nicht laedt -- exakt die test:unit-Rolle, T-4).
LC_ALL=C comm -23 "${_st_tmp}/echt.txt" "${_st_tmp}/blocktests.txt" > "${_st_tmp}/fremd_ohne_block.txt"

ST_FEHLER=0
ST_FAELLE=0
ST_LAEUFE=0
ST_GEZAEHLT=""
st_fall() {   # $1=FallNr $2=Name $3=erwarteter RC $4=Fremd-Datei-oder-leer
              # $5=Zusatz-Env "VAR=WERT ..." oder leer  (Rest: Muster, die vorkommen MUESSEN)
    _f_nr=$1; _f_name=$2; _f_soll=$3; _f_datei=$4; _f_env=$5; shift 5
    if [ -z "$_f_datei" ]; then
        ( unset COMDARE_FREMD_INVENTUR COMDARE_FREMD_HOST_PROBE COMDARE_FREMD_SOLL_DELTA_BLOECKE
          sh "$_st_wache" "$ST_BUILD" ) > "${_st_tmp}/${_f_name}.log" 2>&1
    else
        # shellcheck disable=SC2086 -- die Env-Paare sind bewusst wortgesplittet
        # (Werte ohne Leerzeichen; das SOLL-DELTA dieses Selbsttests ist EIN Block).
        env $_f_env COMDARE_FREMD_INVENTUR="$_f_datei" \
            sh "$_st_wache" "$ST_BUILD" > "${_st_tmp}/${_f_name}.log" 2>&1
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
    ST_LAEUFE=$(( ST_LAEUFE + 1 ))
    case " ${ST_GEZAEHLT} " in
        *" ${_f_nr} "*) ;;
        *) ST_GEZAEHLT="${ST_GEZAEHLT} ${_f_nr}"; ST_FAELLE=$(( ST_FAELLE + 1 )) ;;
    esac
    if [ "$_f_ok" = ja ]; then
        printf '  [ok  ]  (%s) %-38s RC=%s (erwartet %s)\n' "$_f_nr" "$_f_name" "$_f_rc" "$_f_soll"
    else
        printf '  [FEHL]  (%s) %-38s RC=%s (erwartet %s)\n' "$_f_nr" "$_f_name" "$_f_rc" "$_f_soll"
        ST_FEHLER=$(( ST_FEHLER + 1 ))
    fi
}

echo ""
echo "FAELLE:"

# (5) Achse ungefahren -> ausdruecklich gemeldet, kein Einfluss auf den RC
st_fall 5 "ungesetzt_meldet_laut" 0 "" "" "DRITTE ACHSE (Fremd-Inventur): NICHT GEFAHREN"

# (1) Gleichstand (Probe = eigener Baum, Klassen gleich) -> gruen
st_fall 1 "gleichstand_gruen" 0 "${_st_tmp}/echt.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen}" \
    "deckungsgleich" "BILANZ: eigen="

# (2) SCHRUMPF-KOEDER, bei JEDEM Lauf frisch gewuerfelt -> rot MIT NAMEN
_st_k=$(( ($(od -An -N1 -tu1 /dev/urandom | tr -d ' ') % 4) + 2 ))
shuf -n "$_st_k" --random-source=/dev/urandom "${_st_tmp}/echt.txt" > "${_st_tmp}/entfernt.txt"
LC_ALL=C grep -vxF -f "${_st_tmp}/entfernt.txt" "${_st_tmp}/echt.txt" > "${_st_tmp}/geschrumpft.txt"
echo "  (Koeder Fall 2: ${_st_k} Tests frisch gezogen und aus der fremden Inventur entfernt)"
set --
while read -r _st_n; do set -- "$@" "$_st_n"; done < "${_st_tmp}/entfernt.txt"
st_fall 2 "abweichung_rot_mit_namen" 4 "${_st_tmp}/geschrumpft.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen}" \
    "ABWEICHUNG" "$@"

# (3) angekuendigt, aber fehlend -> rot (fail-closed)
st_fall 3 "fehlende_datei_rot" 4 "${_st_tmp}/gibt-es-nicht-$$.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen}" \
    "Datei fehlt"

# (4) leer -> rot (fail-closed)
: > "${_st_tmp}/leer.txt"
st_fall 4 "leere_datei_rot" 4 "${_st_tmp}/leer.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen}" \
    "ist leer"

# (6) SOLL-DELTA deklariert, Block AKTIV, Namen nur hier -> gruen + Ausweis
set --
while read -r _st_n; do set -- "$@" "$_st_n"; done < "${_st_tmp}/blocktests.txt"
st_fall 6 "soll_delta_gruen" 0 "${_st_tmp}/fremd_ohne_block.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen} COMDARE_FREMD_SOLL_DELTA_BLOECKE=${_st_block}" \
    "SOLL-DELTA abgezogen" "deckungsgleich nach SOLL-DELTA" "$@"

# (7) SOLL-DELTA deklariert, aber ZUSAETZLICH gewuerfelte Namen entfernt -> rot 4 MIT Namen
_st_k7=$(( ($(od -An -N1 -tu1 /dev/urandom | tr -d ' ') % 4) + 2 ))
shuf -n "$_st_k7" --random-source=/dev/urandom "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/entfernt7.txt"
LC_ALL=C grep -vxF -f "${_st_tmp}/entfernt7.txt" "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/geschrumpft7.txt"
echo "  (Koeder Fall 7: ${_st_k7} Extra-Namen frisch gezogen und ZUSAETZLICH entfernt)"
set --
while read -r _st_n; do set -- "$@" "$_st_n"; done < "${_st_tmp}/entfernt7.txt"
st_fall 7 "soll_delta_plus_extra_rot" 4 "${_st_tmp}/geschrumpft7.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen} COMDARE_FREMD_SOLL_DELTA_BLOECKE=${_st_block}" \
    "ABWEICHUNG" "$@"

# (8) STALE Deklaration: Block-Namen stehen AUCH im Fremden -> rot 4 benannt
_st_stale_name=$(sed -n '1p' "${_st_tmp}/blocktests.txt")
st_fall 8 "soll_delta_stale_rot" 4 "${_st_tmp}/echt.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_eigen} COMDARE_FREMD_SOLL_DELTA_BLOECKE=${_st_block}" \
    "STALE DEKLARATION" "$_st_stale_name"

# (9) KLASSEN-DIFFERENZ, synthetische Probe einer ANDEREN Sprosse.
# K = Leiter-Differenz der beiden Klassen (committete Untergrenze, NUR gelesen).
_st_floor_pfad=${COMDARE_D2_FLOOR_PFAD:-${_st_dir}/ci_test_inventory_floor.txt}
[ -f "$_st_floor_pfad" ] || st_abbruch "Klassenleiter fehlt: ${_st_floor_pfad}"
st_floor() {
    sed -e 's/#.*$//' "$_st_floor_pfad" | awk -v k="$1" '$1==k {print $2; exit}'
}
case "$_st_eigen_klasse" in
    avx512f) _st_syn_klasse=avx2;  _st_richtung=reicher ;;
    avx2)    _st_syn_klasse=basis; _st_richtung=reicher ;;
    basis)   _st_syn_klasse=avx2;  _st_richtung=aermer ;;
    *)       st_abbruch "unbekannte eigene Klasse '${_st_eigen_klasse}'." ;;
esac
if [ "$_st_richtung" = reicher ]; then
    _st_k9=$(( $(st_floor "$_st_eigen_klasse") - $(st_floor "$_st_syn_klasse") ))
else
    _st_k9=$(( $(st_floor "$_st_syn_klasse") - $(st_floor "$_st_eigen_klasse") ))
fi
[ "$_st_k9" -ge 0 ] || st_abbruch "Klassenleiter invertiert (K=${_st_k9}) -- Sprossen-Ordnung kaputt."

# Synthetische Probe der Klasse ${_st_syn_klasse} -- exakt die Zeilenform, die
# check_cxx_source_runs schreibt (Typ INTERNAL, _COMPILED TRUE, Wert<->_EXITCODE).
_st_probe_syn="${_st_tmp}/probe_syn.txt"
if [ "$_st_syn_klasse" = avx2 ]; then
    _st_syn_avx2_wert=1;  _st_syn_avx2_ec=0
else
    _st_syn_avx2_wert=""; _st_syn_avx2_ec=1
fi
{
    echo "COMDARE_HOST_RUNS_AVX2:INTERNAL=${_st_syn_avx2_wert}"
    echo "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE"
    echo "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=${_st_syn_avx2_ec}"
    echo "COMDARE_HOST_RUNS_AVX512F:INTERNAL="
    echo "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE"
    echo "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1"
    echo "HOST=synthetische-probe"
} > "$_st_probe_syn"

# (9a) exakt K Namen Differenz -> gruen mit Leiter-Ausweis
if [ "$_st_richtung" = reicher ]; then
    shuf -n "$_st_k9" --random-source=/dev/urandom "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/klasse_diff.txt"
    LC_ALL=C grep -vxF -f "${_st_tmp}/klasse_diff.txt" "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/fremd9.txt"
else
    _st_wuerfel=$(od -An -N4 -tx4 /dev/urandom | tr -d ' ')
    : > "${_st_tmp}/klasse_diff.txt"
    _st_i=0
    while [ "$_st_i" -lt "$_st_k9" ]; do
        echo "st9_synthetischer_fremd_test_${_st_wuerfel}_${_st_i}" >> "${_st_tmp}/klasse_diff.txt"
        _st_i=$(( _st_i + 1 ))
    done
    LC_ALL=C sort -u "${_st_tmp}/fremd_ohne_block.txt" "${_st_tmp}/klasse_diff.txt" > "${_st_tmp}/fremd9.txt"
fi
echo "  (Koeder Fall 9: eigen=${_st_eigen_klasse}, fremd synthetisch=${_st_syn_klasse}, K=${_st_k9} gewuerfelte Namen, Richtung ${_st_richtung})"
st_fall 9 "klassen_differenz_k_gruen" 0 "${_st_tmp}/fremd9.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_syn} COMDARE_FREMD_SOLL_DELTA_BLOECKE=${_st_block}" \
    "Leiter-Ausweis" "KLASSEN-DIFFERENZ ERKLAERT" "Klasse ${_st_syn_klasse}, Host synthetische-probe"

# (9b) K+-1 (Richtung gewuerfelt) -> rot 4. Bei K=0 gibt es kein K-1 unter Null:
# dann zwingt der Wuerfel auf +1 (ehrlich vermerkt, kein stiller Skip).
_st_kdir=$(( $(od -An -N1 -tu1 /dev/urandom | tr -d ' ') % 2 ))
if [ "$_st_k9" -eq 0 ] || [ "$_st_kdir" -eq 0 ]; then
    _st_k9b=$(( _st_k9 + 1 )); _st_kdir_txt="+1"
else
    _st_k9b=$(( _st_k9 - 1 )); _st_kdir_txt="-1"
fi
if [ "$_st_richtung" = reicher ]; then
    shuf -n "$_st_k9b" --random-source=/dev/urandom "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/klasse_diff_b.txt"
    LC_ALL=C grep -vxF -f "${_st_tmp}/klasse_diff_b.txt" "${_st_tmp}/fremd_ohne_block.txt" > "${_st_tmp}/fremd9b.txt"
else
    head -n "$_st_k9b" "${_st_tmp}/klasse_diff.txt" > "${_st_tmp}/klasse_diff_b.txt"
    if [ "$_st_k9b" -gt "$_st_k9" ]; then
        echo "st9_synthetischer_fremd_test_extra_$$" >> "${_st_tmp}/klasse_diff_b.txt"
    fi
    LC_ALL=C sort -u "${_st_tmp}/fremd_ohne_block.txt" "${_st_tmp}/klasse_diff_b.txt" > "${_st_tmp}/fremd9b.txt"
fi
echo "  (Koeder Fall 9 Gegenprobe: K${_st_kdir_txt} = ${_st_k9b} Namen -- MUSS rot beissen)"
st_fall 9 "klassen_differenz_koeder_rot" 4 "${_st_tmp}/fremd9b.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_probe_syn} COMDARE_FREMD_SOLL_DELTA_BLOECKE=${_st_block}" \
    "ABWEICHUNG"

# (10) Probe angekuendigt, aber fehlend -> rot 4 (Pflicht-Auskunft fehlt)
st_fall 10 "probe_fehlt_rot" 4 "${_st_tmp}/echt.txt" \
    "COMDARE_FREMD_HOST_PROBE=${_st_tmp}/probe-gibt-es-nicht-$$.txt" \
    "HOST-PROBE FEHLT"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER: ${ST_FAELLE} Faelle gefahren (${ST_LAEUFE} Wache-Laeufe; Fall 9 traegt den"
echo "Gegenkoeder als zweiten Lauf), davon ${ST_FEHLER} Lauf/Laeufe abweichend."
echo "-----------------------------------------------------------------------------"
if [ "$ST_FEHLER" -eq 0 ]; then
    echo "SELBSTTEST ABDECKUNGS-WACHE (dritte Achse): GRUEN."
    exit 0
fi
echo "SELBSTTEST ABDECKUNGS-WACHE (dritte Achse): ROT."
exit 1
