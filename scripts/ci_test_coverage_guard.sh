# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- ABDECKUNGS-WACHE fuer die CI-Test-Auswahl (R4, 2026-08-06)
# =============================================================================
# BEHAUPTUNG, DIE HIER GEPRUEFT WIRD (die Invariante):
#
#     Jeder in dieser Konfiguration registrierte ctest-Test wird von mindestens
#     einem CI-Job ausgefuehrt, der in diesem Pipeline-Lauf auch wirklich faehrt.
#
# Bis R4 war das eine Behauptung im Kommentar (.gitlab-ci.yml, #278 vom 2026-07-06:
# "der Ausschluss verweist auf die dedizierten Gates"). Niemand hat sie je wieder
# nachgerechnet; live gemessen liefen am 2026-08-06 exakt 9 Tests in keinem Job.
# Ab hier ist sie ein Gate.
#
# WIE (und warum das nicht raten kann):
#   * Die Gesamtmenge kommt aus der LIVE-Inventur des Bau-Baums (ctest -N), nicht
#     aus einer Liste. Ein neu registrierter Test ist damit ohne Zutun Teil der
#     Pruefung -- er kann nicht "vergessen" werden, denn niemand traegt ihn ein.
#   * Die gedeckte Menge entsteht, indem fuer JEDE im Manifest deklarierte
#     Job-Auswahl derselbe ctest-Matcher gefragt wird, den der Job selbst benutzt
#     (ctest -N mit demselben -R/-L/-LE). Es wird also keine Regex-Semantik
#     nachgebaut und keine YAML gelesen.
#   * Job und Wache beziehen die Auswahl aus DERSELBEN Datei
#     (scripts/ci_test_coverage_manifest.sh) -- ein Auseinanderlaufen von
#     "was der Job tut" und "was die Wache glaubt" ist bauartbedingt nicht moeglich.
#   * Ein Job mit rules-Gate deckt nur, wenn sein Gate in DIESER Umgebung wirklich
#     zieht (Gate-Klassen: siehe Manifest-Kopf). Ein per Default abgeschalteter Job
#     kann also nie die einzige Deckung eines Tests sein.
#
# WEITERE FEHLERKLASSEN, DIE HIER MITFALLEN:
#   (a) Selektor trifft NICHTS -- ein Job, dessen -R-Liste ins Leere zeigt
#       (Test umbenannt/entfernt), ist ein Phantom-Gate. Harter Fehler.
#   (b) TOTER NAME in einer Namensliste -- steht ein Test namentlich in einem
#       Job-Selektor, existiert aber nicht mehr, faellt das auf, obwohl die
#       uebrigen Namen der Liste den Selektor noch "gruen" aussehen lassen.
#
# AUFRUF:  sh scripts/ci_test_coverage_guard.sh <build-verzeichnis>
# EXIT:    0 = Invariante haelt | 1 = Abdeckungsluecke (Testnamen im Klartext)
#          2 = Bedienung/Umgebung | 3 = Manifest defekt
#
# WICHTIG (Ground Truth): Der Bau-Baum MUSS so konfiguriert sein wie der Job, der
# die Voll-Suite faehrt -- inklusive des 2-Pass-Codegen-Configures des .test-
# Templates. Ohne den zweiten Pass registriert CMake zwei codegen-abhaengige Tests
# NICHT (live gemessen 2026-08-06: 404 statt 406) und die Wache haette einen zu
# kleinen Wahrheitsbegriff. Der CI-Job test:coverage-guard macht genau das.
#
# ASCII-only (Leitplanke).
# =============================================================================

set -u

_ce_dir=$(dirname "$0")
_ce_manifest="${_ce_dir}/ci_test_coverage_manifest.sh"

ce_abbruch() {
    echo ""
    echo "ABDECKUNGS-WACHE: ABBRUCH -- $1" >&2
    exit "${2:-2}"
}

[ -f "$_ce_manifest" ] || ce_abbruch "Manifest nicht gefunden: ${_ce_manifest}" 2
# shellcheck source=scripts/ci_test_coverage_manifest.sh
. "$_ce_manifest"

CE_BUILD_DIR=${1:-}
[ -n "$CE_BUILD_DIR" ] || ce_abbruch "kein Bau-Verzeichnis angegeben. Aufruf: sh scripts/ci_test_coverage_guard.sh <build-verzeichnis>" 2
[ -f "${CE_BUILD_DIR}/CTestTestfile.cmake" ] || ce_abbruch "'${CE_BUILD_DIR}' ist kein konfigurierter CMake-Bau-Baum (CTestTestfile.cmake fehlt)." 2
command -v ctest >/dev/null 2>&1 || ce_abbruch "ctest ist nicht im PATH." 2

_ce_tmp=$(mktemp -d) || ce_abbruch "mktemp -d fehlgeschlagen." 2
trap 'rm -rf "$_ce_tmp"' EXIT INT TERM

# ce_namen <ctest-argumente...> -> sortierte Testnamen auf stdout
ce_namen() {
    ctest --test-dir "$CE_BUILD_DIR" -N "$@" 2>/dev/null \
        | sed -n 's/^ *Test *#[0-9][0-9]*: *//p' | LC_ALL=C sort -u
}

echo "============================================================================="
echo " ABDECKUNGS-WACHE der CI-Test-Auswahl  (scripts/ci_test_coverage_guard.sh)"
echo " Bau-Baum : ${CE_BUILD_DIR}"
echo " Manifest : ${_ce_manifest}"
echo "============================================================================="

ce_namen > "${_ce_tmp}/alle.txt"
CE_GESAMT=$(wc -l < "${_ce_tmp}/alle.txt" | tr -d ' ')
[ "$CE_GESAMT" -gt 0 ] || ce_abbruch "die Live-Inventur (ctest -N) meldet 0 Tests -- der Bau-Baum ist ohne -DCOMDARE_BUILD_TESTS=ON konfiguriert oder leer." 2
echo ""
echo "LIVE-INVENTUR (ctest -N): ${CE_GESAMT} registrierte Tests"
echo ""
echo "DEKLARIERTE JOB-AUSWAHLEN (Quelle: Manifest):"

: > "${_ce_tmp}/gedeckt.txt"
: > "${_ce_tmp}/tote_namen.txt"
CE_LEERE_SELEKTOREN=""

for _ce_job in $CE_COV_JOBS; do
    _ce_mode=$(ce_cov_lookup MODE "$_ce_job") || exit 3
    _ce_patt=$(ce_cov_lookup PATT "$_ce_job") || exit 3
    _ce_gate=$(ce_cov_lookup GATE "$_ce_job") || exit 3

    # -- Gate gegen die ECHTE Umgebung auswerten (keine zweite Abschrift der rules) --
    _ce_aktiv=nein
    _ce_grund=""
    case "$_ce_gate" in
        always)
            _ce_aktiv=ja; _ce_grund="ohne rules-Gate"
            ;;
        declared:*)
            _ce_var=${_ce_gate#declared:}
            eval "_ce_wert=\${${_ce_var}-__CE_UNGESETZT__}"
            case "$_ce_wert" in
                __CE_UNGESETZT__)
                    _ce_aktiv=ja
                    _ce_grund="${_ce_var} ungesetzt -> ANNAHME deklariert (Lauf ausserhalb der CI)"
                    ;;
                "")
                    _ce_aktiv=nein
                    _ce_grund="${_ce_var} ist LEER -> Job faehrt in diesem Lauf NICHT"
                    ;;
                *)
                    _ce_aktiv=ja
                    _ce_grund="${_ce_var}='${_ce_wert}'"
                    ;;
            esac
            ;;
        optin:*)
            _ce_rest=${_ce_gate#optin:}
            _ce_var=${_ce_rest%%=*}
            _ce_soll=${_ce_rest#*=}
            eval "_ce_wert=\${${_ce_var}-}"
            if [ "$_ce_wert" = "$_ce_soll" ]; then
                _ce_aktiv=ja; _ce_grund="${_ce_var}='${_ce_wert}'"
            else
                _ce_aktiv=nein; _ce_grund="opt-in ${_ce_var}='${_ce_wert}' != '${_ce_soll}' -> keine Deckungsgutschrift"
            fi
            ;;
        *)
            ce_abbruch "unbekannte Gate-Klasse '${_ce_gate}' bei Kennung '${_ce_job}' (erlaubt: always, declared:VAR, optin:VAR=WERT)." 3
            ;;
    esac

    ce_namen "$_ce_mode" "$_ce_patt" > "${_ce_tmp}/job.txt"
    _ce_treffer=$(wc -l < "${_ce_tmp}/job.txt" | tr -d ' ')

    if [ "$_ce_treffer" -eq 0 ]; then
        CE_LEERE_SELEKTOREN="${CE_LEERE_SELEKTOREN} ${_ce_job}"
    fi

    if [ "$_ce_aktiv" = ja ]; then
        cat "${_ce_tmp}/job.txt" >> "${_ce_tmp}/gedeckt.txt"
        printf '  [deckt ]  %-28s %-4s %-72s %4s Tests   (%s)\n' "$_ce_job" "$_ce_mode" "$_ce_patt" "$_ce_treffer" "$_ce_grund"
    else
        printf '  [inaktiv] %-28s %-4s %-72s %4s Tests   (%s)\n' "$_ce_job" "$_ce_mode" "$_ce_patt" "$_ce_treffer" "$_ce_grund"
    fi

    # -- (b) tote Namen in Namenslisten: nur rein literale Alternativen pruefen --
    _ce_inner=""
    case "$_ce_patt" in
        '^('*')$') _ce_inner=${_ce_patt#'^('}; _ce_inner=${_ce_inner%')$'} ;;
        '^'*'$')   _ce_inner=${_ce_patt#'^'};  _ce_inner=${_ce_inner%'$'} ;;
        *)         _ce_inner="" ;;
    esac
    if [ -n "$_ce_inner" ]; then
        _ce_alt_ifs=$IFS
        IFS='|'
        for _ce_alt in $_ce_inner; do
            IFS=$_ce_alt_ifs
            case "$_ce_alt" in
                *[\^\$.*+?\(\)\[\]\{\}\\]* ) : ;;   # nicht literal -> nicht pruefbar, uebersprungen
                "" ) : ;;
                * )
                    if [ "$(ce_namen -R "^${_ce_alt}\$" | wc -l | tr -d ' ')" -eq 0 ]; then
                        echo "${_ce_job}|${_ce_alt}" >> "${_ce_tmp}/tote_namen.txt"
                    fi
                    ;;
            esac
            IFS='|'
        done
        IFS=$_ce_alt_ifs
    fi
done

LC_ALL=C sort -u "${_ce_tmp}/gedeckt.txt" -o "${_ce_tmp}/gedeckt.txt"
CE_GEDECKT=$(wc -l < "${_ce_tmp}/gedeckt.txt" | tr -d ' ')
LC_ALL=C comm -23 "${_ce_tmp}/alle.txt" "${_ce_tmp}/gedeckt.txt" > "${_ce_tmp}/luecke.txt"
CE_LUECKE=$(wc -l < "${_ce_tmp}/luecke.txt" | tr -d ' ')

echo ""
echo "BILANZ: ${CE_GEDECKT} von ${CE_GESAMT} registrierten Tests werden von einem fahrenden Job ausgefuehrt."

CE_RC=0

# -- Befund (a): Phantom-Gates --
if [ -n "$CE_LEERE_SELEKTOREN" ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: PHANTOM-GATE -- ein Job-Selektor trifft KEINEN einzigen Test."
    echo "Der Job laeuft dann gruen, ohne irgendetwas zu pruefen."
    for _ce_j in $CE_LEERE_SELEKTOREN; do
        echo "  * Kennung '${_ce_j}': $(ce_cov_lookup MODE "$_ce_j") $(ce_cov_lookup PATT "$_ce_j")"
    done
    echo "-----------------------------------------------------------------------------"
    CE_RC=1
fi

# -- Befund (b): tote Namen --
if [ -s "${_ce_tmp}/tote_namen.txt" ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: TOTER NAME in einer Job-Auswahl -- der Name existiert in der Inventur nicht"
    echo "(umbenannt oder entfernt). Die uebrigen Namen derselben Liste verdecken das sonst."
    while IFS='|' read -r _ce_j _ce_n; do
        echo "  * Job-Kennung '${_ce_j}' nennt '${_ce_n}' -- kein solcher Test registriert."
    done < "${_ce_tmp}/tote_namen.txt"
    echo "-----------------------------------------------------------------------------"
    CE_RC=1
fi

# -- Befund (c): die eigentliche Invariante --
if [ "$CE_LUECKE" -gt 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: ABDECKUNGSLUECKE -- ${CE_LUECKE} Test(s) werden GEBAUT, aber von KEINEM"
    echo "fahrenden CI-Job ausgefuehrt. Ihre Zusicherungen sind wirkungslos:"
    echo ""
    while read -r _ce_t; do
        _ce_marke="ohne contract/pmc-Label"
        if ce_namen -L pmc | grep -qx "$_ce_t"; then
            _ce_marke="Label 'pmc' (Hardware-Klasse)"
        elif ce_namen -L contract | grep -qx "$_ce_t"; then
            _ce_marke="Label 'contract'"
        fi
        echo "    UNGEDECKT: ${_ce_t}   [${_ce_marke}]"
    done < "${_ce_tmp}/luecke.txt"
    echo ""
    echo "SO WIRD DAS BEHOBEN (nicht durch Aufweichen der Wache):"
    echo "  * Der Regelfall braucht GAR NICHTS: test:unit faehrt alles ausser der"
    echo "    Hardware-Klasse. Taucht hier trotzdem etwas auf, ist eine Job-Auswahl"
    echo "    in scripts/ci_test_coverage_manifest.sh enger geworden als die Menge"
    echo "    der Tests -- dort ist es zu heilen."
    echo "  * Traegt der Test 'pmc', gehoert er in die Vendor-Lanes (pmc:amd/pmc:intel)"
    echo "    UND in deren --target-Liste. Steht oben ein Gate auf [inaktiv], laeuft der"
    echo "    zustaendige Job in diesem Lauf gar nicht -- dann ist die Deklaration"
    echo "    (z.B. COMDARE_PMC_LANES) die Ursache, nicht der Test."
    echo "  * Ein Label allein deckt NICHTS ab. Deckung entsteht nur durch einen Job,"
    echo "    der faehrt."
    echo "-----------------------------------------------------------------------------"
    CE_RC=1
fi

if [ "$CE_RC" -eq 0 ]; then
    echo ""
    echo "PARTITIONS-BELEG (dieselbe Rechnung, nur andersherum gelesen):"
    _ce_ohne_pmc=$(ce_namen -LE pmc | wc -l | tr -d ' ')
    _ce_mit_pmc=$(ce_namen -L pmc | wc -l | tr -d ' ')
    echo "  ohne Label 'pmc' (test:unit)        : ${_ce_ohne_pmc}"
    echo "  mit  Label 'pmc' (pmc:amd/intel)    : ${_ce_mit_pmc}"
    echo "  Summe                               : $(( _ce_ohne_pmc + _ce_mit_pmc ))  ==  Inventur ${CE_GESAMT}"
    echo "  -> -L und -LE mit demselben Muster sind komplementaer; ein dritter Fall"
    echo "     existiert nicht. Die Deckung ist damit strukturell, nicht durch Audit."
    echo ""
    echo "ABDECKUNGS-WACHE: GRUEN -- kein Test ohne fahrenden Job."
else
    echo ""
    echo "ABDECKUNGS-WACHE: ROT."
fi

exit "$CE_RC"
