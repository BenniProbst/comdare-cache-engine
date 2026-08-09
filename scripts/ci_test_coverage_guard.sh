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
#   (c) UEBERSPRUNGENER REGISTRIERUNGS-BLOCK -- der Nenner selbst ist zu klein.
#       Siehe den folgenden Absatz; das war der Blindfleck bis 2026-08-09.
#   (d) BEHAUPTET-AKTIVER BLOCK OHNE TEST -- ein Block meldet sich als gelaufen,
#       sein Test steht aber nicht in der Inventur (umbenannt, Registrierung
#       verschluckt). Die Gegenrichtung von (c).
#   (e) NENNER GEGEN FREMDE QUELLE (D1c, 2026-08-09) -- die AUSSENSICHT. (c)/(d)
#       belegen den Nenner gegen den Quelltext DIESES Baums; sie koennen nicht
#       sehen, ob dieser Baum ueberhaupt so gebaut wurde wie der Baum, in dem die
#       Voll-Suite faehrt (beide koennten denselben Block ueberspringen, und das
#       Protokoll saehe in beiden gleich aus). (e) vergleicht deshalb gegen die
#       Inventur eines ANDEREN Jobs, hereingereicht ueber COMDARE_FREMD_INVENTUR.
#       Fail-closed: angekuendigt-aber-fehlend und leer sind ROT, nicht
#       "uebersprungen". Ungesetzt = ausdruecklich als NICHT GEFAHREN gemeldet.
#
# AUFRUF:  sh scripts/ci_test_coverage_guard.sh <build-verzeichnis>
#   Umgebung: COMDARE_FREMD_INVENTUR=<datei>  aktiviert Befund (e). Die Datei ist
#   eine Zeile-je-Testname-Inventur, wie test:unit sie nach
#   build/Testing/ctest_unit_inventar.txt schreibt.
# EXIT:    0 = Invariante haelt
#          1 = Abdeckungsluecke: ein registrierter Test faehrt in keinem Job
#          2 = Bedienung/Umgebung. Dazu zaehlt ein fehlendes, leeres oder
#              abgerissenes Registrierungs-Protokoll: wer seinen Nenner nicht
#              belegen kann, meldet nicht gruen (fail-closed).
#          3 = Manifest defekt
#          4 = NENNER-BEFUND: die Inventur ist nachweislich kleiner als das, was
#              dieser Baum registrieren muesste (uebersprungener Block), oder sie
#              widerspricht dem Protokoll (Block meldet AKTIV, Test fehlt), oder
#              sie weicht von der Inventur eines anderen Jobs ab (Befund (e)).
#          WARUM 4 EIN EIGENER CODE IST -- und nicht einfach 1: der Selbsttest
#          muss "rot aus dem richtigen Grund" von "rot aus irgendeinem Grund"
#          unterscheiden koennen. Ein Koeder, der nur 'nicht 0' prueft, beisst
#          auch bei einem Phantom-Gate und beweist damit nichts ueber den Nenner.
#
# WICHTIG (Ground Truth): Der Bau-Baum MUSS so konfiguriert sein wie der Job, der
# die Voll-Suite faehrt. Mehrere Tests dieses Projekts werden erst registriert,
# wenn ein 2-Pass-Werkzeug GEBAUT ist und CMake danach ein zweites Mal laeuft.
# Das betrifft ZWEI Werkzeuge, nicht eines -- die zweite Stelle fehlte hier bis
# 2026-08-09 und ist der Grund fuer den Rest dieses Absatzes:
#   * comdare_anatomy_codegen_cli (Codegen-Pass) -> test_v41_anatomy_r5i_configure_codegen
#     und test_v41_anatomy_f15_measurement.
#   * comdare_adhoc_emitter_cli (Adhoc-Emitter-Pass) -> test_v41_anatomy_adhoc_autobuilt_load
#     und f15_compare_cli_smoke.
#
# AM OBJEKT GEMESSEN (2026-08-09, build-covguard dieses Worktrees; Verfahren: die
# fertig gebaute Werkzeug-Binary beiseitelegen, 'cmake -S . -B build-covguard'
# erneut laufen lassen, 'ctest -N' zaehlen -- kein Neubau, nur Configure):
#   beide Werkzeuge vorhanden ............................ 434 Tests
#   Adhoc-Emitter beiseite, Codegen vorhanden ............ 432 Tests
#   beide beiseite (= Zustand nach Pass 1) ............... 430 Tests
# Jeder der beiden Passes traegt also genau zwei Tests; zusammen sind es vier.
#
# Das Emitter-Werkzeug steht in KEINEM CI-Job namentlich: '/usr/bin/grep -c
# adhoc_emitter .gitlab-ci.yml' -> 0 (Gegenprobe mit demselben grep in derselben
# Datei: 'anatomy_codegen' -> 2, 'make inventar' -> 3; die Null ist eine Null und
# kein kaputtes Werkzeug). Es kommt seit dem GNU-Bauweg ueber 'make inventar'
# (= 'all' + Reconfigure) mit, ohne dass es irgendwo namentlich gepflegt werden
# muss. Der offizielle Weg, der diesen Zustand herstellt, ist 'make inventar';
# der CI-Job test:coverage-guard macht genau das.
#
# UND DER PUNKT, AN DEM DIESE WACHE SELBST BLIND WAR: sie konnte den Unterschied
# nicht SEHEN. Ein uebersprungener Registrierungs-Block macht keinen Laerm -- er
# macht die Inventur kleiner. Ueber der kleineren Menge rechnete die Wache
# korrekt und meldete GRUEN; sie hatte sogar recht ueber die Menge, die sie sah.
# Ein falsches Messgeraet faellt irgendwann auf, ein richtiges Messgeraet am
# falschen Gegenstand nie. Deshalb liest sie jetzt zusaetzlich das
# REGISTRIERUNGS-PROTOKOLL (cmake/registrierungs_protokoll.cmake): dort vermerkt
# JEDER bedingte Block SELBST, ob er gelaufen ist und welche Tests er registriert
# haette. Ein uebersprungener Block ist ROT; ein fehlendes oder abgerissenes
# Protokoll ist ABBRUCH (fail-closed). Der Nenner steht ab hier in der AUSGABE,
# nicht im Kopf des Autors.
#
# SELBSTTEST (T-6): tests/unit/test_d2_abdeckungs_wache_nenner.cpp -- ein GOOGLE
# TEST, keine weitere Shell-Probe (Owner-Entscheid 2026-08-09: "Es waere sauberer
# im cmake-Debug Modus standard google Tests zu fahren und diese in Release zu
# wiederholen aufgrund von compile regressionen. Skripte sagen gar nichts.").
# Er baut praeparierte Bau-Baeume mit FRISCH GEWUERFELTEN Kennungen (/dev/urandom,
# nie abgeschrieben), faehrt DIESES Skript darauf und prueft Exit-Code UND die
# woertliche Ausgabe -- inklusive der Gegenprobe, dass derselbe Baum mit
# gelaufenem Block NICHT mit 4 endet.
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
#
# K11-HEILUNG (D2-G3.3, 2026-08-09). Hier stand bis heute:
#     ctest --test-dir "$CE_BUILD_DIR" -N "$@" 2>/dev/null \
#         | sed -n 's/^ *Test *#[0-9][0-9]*: *//p' | LC_ALL=C sort -u
# Zwei Fehler in einer einzigen Zeile:
#   * '2>/dev/null' verschluckt den Fehlerkanal von ctest. Ein ctest, das gar
#     nicht durchlief, sah danach exakt aus wie ein ctest, der nichts fand.
#   * Ein 'rc=$?' hinter dieser Pipe haette 'sort' gemessen, nie ctest. Deshalb
#     laeuft ctest jetzt ALLEIN in eine Datei, und der Code wird als EIGENE
#     Anweisung unmittelbar danach gelesen.
# AM OBJEKT GEMESSEN (2026-08-09, ctest 4.3.4, /tmp/d2probe):
#     CTestTestfile.cmake mit Syntaxfehler .. Exit 8, stderr "Parse error."
#     Selektor ohne Treffer (-R zzz) ....... Exit 0, leere Liste
#     Verzeichnis ohne CTestTestfile ....... Exit 0, "Total Tests: 0"
# Die ersten beiden Faelle sind also unterscheidbar -- vorher kamen beide als
# "0 Treffer" an. Ein kaputtes ctest wurde damit als PHANTOM-GATE (Exit 1)
# oder als leerer Bau-Baum (Exit 2) gemeldet: eine falsche Anschuldigung gegen
# den Job-Selektor bzw. gegen die Konfiguration.
#
# WARUM EIN VERMERK IN EINER DATEI UND KEIN DIREKTES 'exit': ce_namen wird an
# mehreren Stellen in einer Kommando-Substitution $( ) gerufen. Ein 'exit' darin
# beendet nur die Subshell -- das Skript liefe mit einer leeren Zahl weiter und
# waere damit genau die Sorte Wache, gegen die dieses Paket gebaut ist. Der
# Vermerk in "${_ce_tmp}/werkzeug_fehler.txt" ueberlebt die Subshell;
# ce_werkzeug_pruefen() macht daraus im Hauptprozess einen Abbruch.
ce_namen() {
    ctest --test-dir "$CE_BUILD_DIR" -N "$@" > "${_ce_tmp}/ctest_roh.txt" 2> "${_ce_tmp}/ctest_err.txt"
    _ce_ctest_rc=$?
    if [ "$_ce_ctest_rc" -ne 0 ]; then
        {
            echo "ctest --test-dir '${CE_BUILD_DIR}' -N $* -> Exit ${_ce_ctest_rc}"
            sed 's/^/      /' "${_ce_tmp}/ctest_err.txt"
        } >> "${_ce_tmp}/werkzeug_fehler.txt"
        return 1
    fi
    sed -n 's/^ *Test *#[0-9][0-9]*: *//p' "${_ce_tmp}/ctest_roh.txt" | LC_ALL=C sort -u
}

# Ein WERKZEUG-Fehler ist keine Messung. Diese Pruefung steht an jeder Stelle,
# an der die Wache aus ce_namen-Ergebnissen ein Urteil formt -- fail-closed.
ce_werkzeug_pruefen() {
    [ -s "${_ce_tmp}/werkzeug_fehler.txt" ] || return 0
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: DAS WERKZEUG SELBST HAT VERSAGT -- das ist NICHT '0 Treffer'."
    echo "ctest ist nicht durchgelaufen. Was es auf dem Fehlerkanal sagte (frueher"
    echo "durch '2>/dev/null' verworfen):"
    echo ""
    cat "${_ce_tmp}/werkzeug_fehler.txt"
    echo ""
    echo "UNTERSCHEIDUNG, DIE HIER HAENGT: eine leere Trefferliste aus einem GESUNDEN"
    echo "ctest ist ein Befund ueber den Selektor (Phantom-Gate, Exit 1). Eine leere"
    echo "Trefferliste aus einem ABGESTUERZTEN ctest ist ein Befund ueber die Umgebung"
    echo "(Exit 2). Wer beide gleich behandelt, beschuldigt den Selektor fuer einen"
    echo "Werkzeug-Defekt -- und haelt eine Wache fuer scharf, die gar nicht gemessen hat."
    echo "-----------------------------------------------------------------------------"
    _ce_m="ctest ist nicht durchgelaufen (Ausgabe oben). Eine Inventur, die nicht"
    _ce_m="${_ce_m} erhoben werden konnte, ist weder eine leere noch eine gedeckte Menge."
    ce_abbruch "$_ce_m" 2
}

echo "============================================================================="
echo " ABDECKUNGS-WACHE der CI-Test-Auswahl  (scripts/ci_test_coverage_guard.sh)"
echo " Bau-Baum : ${CE_BUILD_DIR}"
echo " Manifest : ${_ce_manifest}"
echo "============================================================================="

# DIE ROT-MARKE DES NENNERS WIRD HIER GESETZT, NICHT ERST BEI DER URTEILSBILDUNG.
# Sie stand bis 2026-08-09 unmittelbar vor den Befunden (c)/(d)/(e). Jede Pruefung
# davor -- und die Untergrenze unten IST eine solche -- haette ihr Ergebnis bei der
# spaeteren Zuweisung 'CE_NENNER_ROT=0' wieder verloren: eine Pruefung, die rechnet,
# druckt und dann ueberschrieben wird. Genau die Bauform, gegen die D2 gebaut ist.
# Wer hier eine weitere Initialisierung einfuegt, macht alles darueber wirkungslos.
CE_NENNER_ROT=0

ce_namen > "${_ce_tmp}/alle.txt"
ce_werkzeug_pruefen
CE_GESAMT=$(wc -l < "${_ce_tmp}/alle.txt" | tr -d ' ')
[ "$CE_GESAMT" -gt 0 ] || ce_abbruch "die Live-Inventur (ctest -N) meldet 0 Tests -- der Bau-Baum ist ohne -DCOMDARE_BUILD_TESTS=ON konfiguriert oder leer." 2
echo ""
echo "LIVE-INVENTUR (ctest -N): ${CE_GESAMT} registrierte Tests"

# =============================================================================
# DIE UNTERGRENZE DES NENNERS (D2, 2026-08-09) -- eine Zahl, die NICHT aus diesem
# Lauf stammt
# =============================================================================
# WAS OHNE SIE FEHLT: bis hierher hat die Wache ihren Nenner ausschliesslich aus
# sich selbst bezogen. 'ctest -N' liefert die Menge, 'ctest -N -R ...' die
# Teilmengen -- dasselbe Werkzeug, derselbe Baum, derselbe Augenblick. Schrumpft
# der Baum, schrumpfen BEIDE Zahlen im Gleichschritt und ihr Verhaeltnis bleibt
# 'vollstaendig'. Die einzige Zahl, die vorher gegen Schrumpfen half, war
# '> 0' -- ein Baum mit einem einzigen Test erfuellte sie.
#
# V-7 (FREMDER NENNER): diese Untergrenze steht in einer COMMITTETEN Datei. Sie
# ist damit die einzige Zahl im ganzen Lauf, die niemand aus dem gemessenen Baum
# herleiten kann -- sie stammt aus einem frueheren, bewusst abgenommenen Zustand
# und aendert sich nur durch einen Menschen in einem eigenen Commit.
#
# FAIL-CLOSED: fehlt die Datei oder ist ihr Inhalt keine einzelne Ganzzahl, ist
# das ABBRUCH (Exit 2), nicht 'dann eben ohne Untergrenze'. Eine Wache, die ihre
# Vergleichsgroesse verliert und trotzdem gruen meldet, hat die Pruefung nur
# aufgehuebscht.
#
# UEBERSCHREITEN IST KEIN FEHLER: waechst die Testzahl, ist das der Normalfall.
# Die Wache sagt dann, dass die Datei nachgezogen gehoert -- in einem eigenen,
# im Diff sichtbaren Commit. Sie schreibt sie NIEMALS selbst: eine Untergrenze,
# die sich selbst nachfuehrt, ist keine.
# =============================================================================
_ce_floor_pfad=${COMDARE_D2_FLOOR_PFAD:-${_ce_dir}/ci_test_inventory_floor.txt}
if [ ! -f "$_ce_floor_pfad" ]; then
    _ce_m="die committete Untergrenze fehlt: '${_ce_floor_pfad}'."
    _ce_m="${_ce_m} Ohne sie hat diese Wache keine einzige Zahl, die nicht aus dem"
    _ce_m="${_ce_m} geprueften Baum selbst stammt (V-7). Anlegen: eine Zeile, eine"
    _ce_m="${_ce_m} Ganzzahl; '#'-Kommentare sind erlaubt."
    ce_abbruch "$_ce_m" 2
fi

# Kommentare weg, Leerraum weg, Leerzeilen weg. Das abschliessende 'echo' erzwingt
# einen Zeilenabschluss, damit eine Datei ohne schliessenden Zeilenumbruch nicht
# als 0 Zeilen durchgeht.
{ sed -e 's/#.*$//' -e 's/[[:space:]]//g' "$_ce_floor_pfad"; echo; } | sed '/^$/d' > "${_ce_tmp}/floor.txt"
_ce_floor_zeilen=$(wc -l < "${_ce_tmp}/floor.txt" | tr -d ' ')
if [ "$_ce_floor_zeilen" -ne 1 ]; then
    _ce_m="'${_ce_floor_pfad}' enthaelt ${_ce_floor_zeilen} Wertzeilen, erwartet ist"
    _ce_m="${_ce_m} GENAU EINE. Zwei Zahlen in einer Untergrenze sind keine"
    _ce_m="${_ce_m} Untergrenze, sondern eine offene Frage."
    ce_abbruch "$_ce_m" 2
fi
CE_FLOOR=$(cat "${_ce_tmp}/floor.txt")
case "$CE_FLOOR" in
    '' | *[!0-9]*)
        _ce_m="die Untergrenze in '${_ce_floor_pfad}' ist keine reine Ganzzahl,"
        _ce_m="${_ce_m} sondern '${CE_FLOOR}'. Fail-closed: eine unlesbare"
        _ce_m="${_ce_m} Untergrenze wird nicht als 'keine' behandelt."
        ce_abbruch "$_ce_m" 2
        ;;
esac

echo "UNTERGRENZE (committet, ${_ce_floor_pfad}): ${CE_FLOOR}"
if [ "$CE_GESAMT" -lt "$CE_FLOOR" ]; then
    echo "  -> UNTERSCHRITTEN um $(( CE_FLOOR - CE_GESAMT )) Test(e)."
    CE_NENNER_ROT=1
    CE_FLOOR_ROT=1
elif [ "$CE_GESAMT" -gt "$CE_FLOOR" ]; then
    echo "  -> ueberschritten um $(( CE_GESAMT - CE_FLOOR )) Test(e). Kein Fehler; die Datei"
    echo "     gehoert bei Gelegenheit in einem EIGENEN Commit auf ${CE_GESAMT} nachgezogen."
    CE_FLOOR_ROT=0
else
    echo "  -> genau erreicht (${CE_GESAMT} == ${CE_FLOOR})."
    CE_FLOOR_ROT=0
fi

# =============================================================================
# DER PARTITIONS-BELEG WIRD GERECHNET, NICHT GESETZT (D2-G3.2, 2026-08-09)
# =============================================================================
# Hier stand bis heute, ganz am Ende und nur im gruenen Zweig, die Zeile
#     echo "  Summe : $(( _ce_ohne_pmc + _ce_mit_pmc ))  ==  Inventur ${CE_GESAMT}"
# Das '==' darin war eine ZEICHENKETTE. Es sah aus wie das Ergebnis eines
# Vergleichs und war der Text zwischen zwei Zahlen: die Zeile haette
# '431 == 430' gedruckt und die Wache waere gruen geblieben.
#
# ZWEI FEHLER, EINER DAVON UNSICHTBAR:
#   * Der Vergleich fand nicht statt (das '==').
#   * Der Block lief nur bei CE_RC == 0 und stand HINTER der Zuweisung
#     'CE_RC=4'. Ein dort gesetztes CE_NENNER_ROT haette das Urteil nicht mehr
#     erreicht -- die Pruefung waere auch nach dem Einbau eines echten
#     Vergleichs wirkungslos geblieben. Deshalb wird hier gerechnet, oben, wo
#     das Ergebnis noch zaehlt; gedruckt wird weiter unten.
#
# WAS DIE GLEICHUNG BEHAUPTET: '-L pmc' und '-LE pmc' sind komplementaer, ihre
# Summe MUSS die Inventur sein. Weicht sie ab, ist eine der drei Zahlen nicht
# das, wofuer die Wache sie haelt -- und jede Deckungsaussage darueber ebenso.
# Das ist ein NENNER-Befund (Exit 4), kein Abdeckungs-Befund (Exit 1).
# =============================================================================
CE_OHNE_PMC=$(ce_namen -LE pmc | wc -l | tr -d ' ')
CE_MIT_PMC=$(ce_namen -L pmc | wc -l | tr -d ' ')
ce_werkzeug_pruefen
CE_PARTITION_SUMME=$(( CE_OHNE_PMC + CE_MIT_PMC ))
if [ "$CE_PARTITION_SUMME" -ne "$CE_GESAMT" ]; then
    CE_NENNER_ROT=1
    CE_PARTITION_ROT=1
else
    CE_PARTITION_ROT=0
fi

# =============================================================================
# DER NENNER ERKLAERT SICH SELBST -- Registrierungs-Protokoll auswerten (D2)
# =============================================================================
# Die Inventur oben ist nicht die Menge aller Tests dieses Repos, sondern die
# Menge, die DIESER Bau-Baum registriert hat. Bedingte Registrierungs-Bloecke
# (2-Pass-Werkzeuge) koennen sie verkleinern, ohne dass irgendetwas rot wird.
# Das Protokoll ist die einzige Stelle, an der ein solcher Block sich meldet --
# es wird zur Configure-Zeit von den Bloecken selbst geschrieben, nicht hier
# nachgebaut (dieselbe Doktrin wie beim Manifest: eine Quelle, zwei Leser).
_ce_protokoll="${CE_BUILD_DIR}/comdare_registrierungs_protokoll.txt"

if [ ! -f "$_ce_protokoll" ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "Erwartet wurde: ${_ce_protokoll}"
    echo "Geschrieben wird es zur Configure-Zeit von cmake/registrierungs_protokoll.cmake"
    echo "(Abschluss-Aufruf am Ende der Wurzel-CMakeLists.txt)."
    echo "Ohne dieses Protokoll kann die Wache nicht wissen, ob ihre Inventur"
    echo "vollstaendig ist -- und eine Wache, die ihren Nenner nicht belegen kann,"
    echo "meldet nicht gruen. Der Bau-Baum wurde vermutlich mit einem aelteren Stand"
    echo "konfiguriert: 'make inventar' erneut fahren."
    echo "-----------------------------------------------------------------------------"
    ce_abbruch "Registrierungs-Protokoll fehlt -- der Nenner ist unbelegt (fail-closed)." 2
fi

sed -n 's/^BLOCK|//p' "$_ce_protokoll" > "${_ce_tmp}/bloecke.txt"
sed -n 's/^ENDE|//p'  "$_ce_protokoll" > "${_ce_tmp}/ende.txt"
CE_BLOCKS=$(wc -l < "${_ce_tmp}/bloecke.txt" | tr -d ' ')
_ce_ende_zeilen=$(wc -l < "${_ce_tmp}/ende.txt" | tr -d ' ')

# Endmarke: genau eine, und ihre Zahl muss zu den gelesenen Bloecken passen.
# Ein abgebrochener Configure-Lauf hinterlaesst sonst ein halbes Protokoll, und
# "eben weniger Bloecke" saehe genauso aus wie "alles in Ordnung".
if [ "$_ce_ende_zeilen" -ne 1 ]; then
    echo ""
    echo "Erwartet: GENAU EINE Zeile 'ENDE|<n>' in ${_ce_protokoll}."
    echo "Gefunden: ${_ce_ende_zeilen}."
    ce_abbruch "Registrierungs-Protokoll ohne genau eine Endmarke -- abgerissen oder doppelt." 2
fi
CE_BLOCKS_ERWARTET=$(cat "${_ce_tmp}/ende.txt")
if [ "$CE_BLOCKS_ERWARTET" != "$CE_BLOCKS" ]; then
    echo ""
    echo "Endmarke nennt ${CE_BLOCKS_ERWARTET} Bloecke, gelesen wurden ${CE_BLOCKS}."
    ce_abbruch "Registrierungs-Protokoll inkonsistent -- Endmarke und Blockzahl widersprechen sich." 2
fi

: > "${_ce_tmp}/uebersprungene_bloecke.txt"
: > "${_ce_tmp}/phantom_bloecke.txt"
CE_BLOCKS_AKTIV=0
CE_BLOCKS_UEBER=0

# Null Bloecke ist KEIN gueltiger Zustand, sondern der alte Blindfleck in neuer
# Form: verschwinden die comdare_registrierung_vermerken()-Aufrufe, saehe die
# Wache wieder nur "ein bisschen weniger Tests" und meldete gruen. Wer die
# bedingten Bloecke wirklich abschafft, schafft auch diese Pruefung ab
# (cmake/registrierungs_protokoll.cmake) -- das ist eine Entscheidung, die man
# trifft, keine, in die man hineinrutscht.
if [ "$CE_BLOCKS" -eq 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "Das Protokoll ${_ce_protokoll}"
    echo "enthaelt keinen einzigen Block. Entweder sind die Aufrufe von"
    echo "comdare_registrierung_vermerken() verschwunden -- dann ist der alte Blindfleck"
    echo "zurueck und die Wache saehe wieder nur 'ein bisschen weniger Tests'. Oder es"
    echo "gibt wirklich keinen bedingten Registrierungs-Block mehr -- dann gehoert diese"
    echo "Pruefung samt cmake/registrierungs_protokoll.cmake bewusst entfernt."
    echo "-----------------------------------------------------------------------------"
    ce_abbruch "Registrierungs-Protokoll ohne einen einzigen Block -- Nenner unbelegt (fail-closed)." 2
fi

echo ""
echo "NENNER (nie eine nackte Null) -- bedingte Registrierungs-Bloecke:"

while IFS='|' read -r _ce_bk _ce_bs _ce_bt _ce_bg; do
    [ -n "${_ce_bk:-}" ] || continue
    printf '%s\n' "${_ce_bt:-}" | tr ',' '\n' > "${_ce_tmp}/blocktests.txt"
    if [ "${_ce_bs:-}" = "AKTIV" ]; then
        CE_BLOCKS_AKTIV=$(( CE_BLOCKS_AKTIV + 1 ))
        printf '  [gelaufen     ] %-26s -> %-46s (%s)\n' "$_ce_bk" "${_ce_bt:--}" "${_ce_bg:-}"
        # GEGENPROBE: "AKTIV" ist ohne diese Zeile eine unpruefbare Behauptung.
        # Der Zeilenvergleich laeuft in awk, NICHT in grep: 'grep' ist auf diesem
        # Host je nach PATH GNU grep 3.11 oder ugrep 7.5.0, und die beiden sind
        # sich bei Muster-Eskapierung nicht einig (Fallen-Register). awk vergleicht
        # $0 == n bytegleich -- da gibt es keine Engine-Frage.
        while read -r _ce_tn; do
            [ -n "$_ce_tn" ] || continue
            [ "$_ce_tn" = "-" ] && continue
            if ! awk -v n="$_ce_tn" 'BEGIN{f=1} $0==n {f=0} END{exit f}' "${_ce_tmp}/alle.txt"; then
                echo "${_ce_bk}|${_ce_tn}" >> "${_ce_tmp}/phantom_bloecke.txt"
            fi
        done < "${_ce_tmp}/blocktests.txt"
    else
        CE_BLOCKS_UEBER=$(( CE_BLOCKS_UEBER + 1 ))
        printf '  [UEBERSPRUNGEN] %-26s -> %-46s (%s)\n' "$_ce_bk" "${_ce_bt:--}" "${_ce_bg:-}"
        while read -r _ce_tn; do
            [ -n "$_ce_tn" ] || continue
            [ "$_ce_tn" = "-" ] && continue
            echo "${_ce_bk}|${_ce_tn}" >> "${_ce_tmp}/uebersprungene_bloecke.txt"
        done < "${_ce_tmp}/blocktests.txt"
    fi
done < "${_ce_tmp}/bloecke.txt"

echo ""
echo "  ${CE_GESAMT} Tests in der Inventur; ${CE_BLOCKS} bedingte Registrierungs-Bloecke,"
echo "  davon ${CE_BLOCKS_AKTIV} gelaufen und ${CE_BLOCKS_UEBER} uebersprungen."
echo "  Quelle: ${_ce_protokoll}"

# =============================================================================
# DRITTE ACHSE (D1c, 2026-08-09): DER NENNER GEGEN EINE FREMDE QUELLE
# =============================================================================
# ABGRENZUNG ZU (c)/(d), damit hier nichts doppelt geprueft wird: das
# Registrierungs-Protokoll oben belegt den Nenner gegen den QUELLTEXT dieses
# Baums -- es faengt uebersprungene Bloecke. Es kann aber nichts darueber sagen,
# ob DIESER Baum ueberhaupt so gebaut wurde wie der Baum, in dem die Voll-Suite
# faehrt: beide Baeume haetten denselben Block ueberspringen koennen, und das
# Protokoll saehe in beiden gleich aus. Diese Achse vergleicht deshalb gegen
# eine Inventur, die ein ANDERER Job erzeugt hat (test:unit). Sie ist die
# Aussen-, nicht die Innensicht.
#
# FAIL-CLOSED. Ist COMDARE_FREMD_INVENTUR gesetzt, MUSS die Datei da und nicht
# leer sein -- sonst waere ein verlorenes Artefakt von einem Gleichstand nicht
# zu unterscheiden. Ist sie NICHT gesetzt (lokaler Lauf), wird die Achse
# ausdruecklich als NICHT GEFAHREN gemeldet und verschwindet nicht still.
#
# EXIT-KLASSE: eine Abweichung hier ist ein NENNER-BEFUND und setzt deshalb
# CE_NENNER_ROT (Exit 4), nicht CE_RC=1. Das ist dieselbe Unterscheidung, die
# (c)/(d) eingefuehrt haben: "rot, weil der Nenner nicht stimmt" muss von
# "rot, weil ein Test ungedeckt ist" unterscheidbar bleiben -- sonst beweist
# ein Koeder, der nur 'nicht 0' prueft, nichts ueber den Nenner.
# =============================================================================
CE_FREMD_STATUS="NICHT GEFAHREN"
CE_FREMD_ROT=0
_ce_fremd=${COMDARE_FREMD_INVENTUR-}
echo ""
if [ -z "$_ce_fremd" ]; then
    echo "DRITTE ACHSE (Fremd-Inventur): NICHT GEFAHREN -- COMDARE_FREMD_INVENTUR ist"
    echo "  ungesetzt. Der Nenner ist damit nur gegen den eigenen Quelltext belegt"
    echo "  (Protokoll oben), nicht gegen den Baum eines anderen Jobs."
elif [ ! -f "$_ce_fremd" ]; then
    echo "DRITTE ACHSE (Fremd-Inventur): ROT -- die angekuendigte Datei fehlt:"
    echo "  COMDARE_FREMD_INVENTUR='${_ce_fremd}'"
    echo "  Angekuendigt und nicht da wird NICHT uebersprungen (fail-closed)."
    CE_FREMD_STATUS="ROT (Datei fehlt)"
    CE_FREMD_ROT=1
else
    LC_ALL=C sort -u "$_ce_fremd" | sed '/^[[:space:]]*$/d' > "${_ce_tmp}/fremd.txt"
    CE_FREMD_N=$(wc -l < "${_ce_tmp}/fremd.txt" | tr -d ' ')
    if [ "$CE_FREMD_N" -eq 0 ]; then
        echo "DRITTE ACHSE (Fremd-Inventur): ROT -- '${_ce_fremd}' ist leer."
        echo "  Eine leere Gegenquelle ist keine Bestaetigung (fail-closed)."
        CE_FREMD_STATUS="ROT (Datei leer)"
        CE_FREMD_ROT=1
    else
        LC_ALL=C comm -23 "${_ce_tmp}/alle.txt" "${_ce_tmp}/fremd.txt" > "${_ce_tmp}/nur_hier.txt"
        LC_ALL=C comm -13 "${_ce_tmp}/alle.txt" "${_ce_tmp}/fremd.txt" > "${_ce_tmp}/nur_fremd.txt"
        _ce_nh=$(wc -l < "${_ce_tmp}/nur_hier.txt" | tr -d ' ')
        _ce_nf=$(wc -l < "${_ce_tmp}/nur_fremd.txt" | tr -d ' ')
        echo "DRITTE ACHSE (Fremd-Inventur): dieser Baum ${CE_GESAMT}, fremder Baum ${CE_FREMD_N}"
        echo "  Quelle: ${_ce_fremd}"
        if [ "$_ce_nh" -eq 0 ] && [ "$_ce_nf" -eq 0 ]; then
            echo "  -> deckungsgleich (${CE_GESAMT} == ${CE_FREMD_N}), Namen identisch."
            CE_FREMD_STATUS="GRUEN (${CE_GESAMT} == ${CE_FREMD_N})"
        else
            echo "  -> ABWEICHUNG: ${_ce_nh} nur hier, ${_ce_nf} nur im fremden Baum."
            CE_FREMD_STATUS="ROT (${CE_GESAMT} != ${CE_FREMD_N})"
            CE_FREMD_ROT=1
        fi
    fi
fi

echo ""
echo "DEKLARIERTE JOB-AUSWAHLEN (Quelle: Manifest):"

: > "${_ce_tmp}/gedeckt.txt"
: > "${_ce_tmp}/tote_namen.txt"
: > "${_ce_tmp}/ungepruefte_gates.txt"
CE_LEERE_SELEKTOREN=""
# NENNER FUER DIE GATE-ZAHL (V-1): die Bilanz unten nennt "N von M" -- eine nackte
# Zahl ungepruefter Gates waere ohne M nicht einzuordnen.
CE_GATES_GESAMT=0
CE_GATES_UNGEPRUEFT=0

for _ce_job in $CE_COV_JOBS; do
    CE_GATES_GESAMT=$(( CE_GATES_GESAMT + 1 ))
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
                    # FAIL-OPEN, UND AB HIER GEZAEHLT (D2-G3 Punkt 4). Der Zweig bleibt
                    # bewusst 'ja' -- lokal ist keine CI-Variable gesetzt, und ein Rot
                    # waere hier Daueralarm. Er wird aber nicht mehr verschwiegen: die
                    # Bilanz nennt die Zahl, damit "gedeckt" und "als gedeckt ANGENOMMEN"
                    # unterscheidbar bleiben.
                    _ce_aktiv=ja
                    _ce_grund="${_ce_var} ungesetzt -> ANNAHME deklariert (Lauf ausserhalb der CI)"
                    CE_GATES_UNGEPRUEFT=$(( CE_GATES_UNGEPRUEFT + 1 ))
                    echo "${_ce_job} (Variable ${_ce_var} ungesetzt)" >> "${_ce_tmp}/ungepruefte_gates.txt"
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

ce_werkzeug_pruefen

echo ""
echo "BILANZ: ${CE_GEDECKT} von ${CE_GESAMT} registrierten Tests werden von einem fahrenden Job ausgefuehrt."

# UNGEPRUEFTE GATES SICHTBAR MACHEN (D2-G3, 2026-08-09).
# 'declared:VAR' ist fail-OPEN: ist VAR ungesetzt, nimmt die Wache an, der Job sei
# deklariert, und schreibt ihm die Deckung GUT. Das ist fuer den lokalen Lauf
# richtig (dort ist keine CI-Variable gesetzt) -- es blieb aber unsichtbar. Wer die
# Bilanz las, konnte nicht unterscheiden, ob ein Gate GEPRUEFT und aktiv war oder
# ob es nur mangels Auskunft als aktiv GALT. Ab hier steht die Zahl in der Ausgabe:
# Deckung aus einer Annahme ist Deckung mit Sternchen, keine Messung.
# ABSICHTLICH OHNE ROT-HAERTE: in der CI setzt jeder Job diese Variablen ueber
# seinen variables:-Block, dort ist der Zaehler 0. Ihn hart zu schalten wuerde
# jeden lokalen Lauf rot machen -- eine Wache, die immer rot ist, ist so wertlos
# wie eine, die nie rot wird. Ob im CI-Kontext zusaetzlich hart geschaltet wird,
# ist eine offene Owner/Lead-Entscheidung (siehe Bericht D2, Punkt B.4).
if [ "$CE_GATES_UNGEPRUEFT" -gt 0 ]; then
    echo "        ${CE_GATES_UNGEPRUEFT} von ${CE_GATES_GESAMT} Gate(n) UNGEPRUEFT -- Deklarations-"
    echo "        variable in diesem Lauf nicht gesetzt, ihre Deckung ist ANNAHME, nicht Messung:"
    while read -r _ce_ug; do
        echo "           ANNAHME: ${_ce_ug}"
    done < "${_ce_tmp}/ungepruefte_gates.txt"
else
    echo "        ${CE_GATES_GESAMT} von ${CE_GATES_GESAMT} Gate(n) gegen eine gesetzte Variable GEPRUEFT."
fi

CE_RC=0

# -- PARTITIONS-WIDERSPRUCH ausgeben (D2-G3.2) --
# GERECHNET wird weiter oben (vor der Urteilsbildung, sonst koennte das Ergebnis
# nichts mehr bewirken); hier wird nur noch berichtet. Die Zahlen stammen aus
# denselben Variablen, die dort gesetzt wurden -- kein zweiter ctest-Aufruf, damit
# Anzeige und Urteil nicht auseinanderlaufen koennen.
if [ "$CE_PARTITION_ROT" -ne 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: PARTITIONS-WIDERSPRUCH -- '-L pmc' und '-LE pmc' muessen die Inventur"
    echo "restlos und ueberschneidungsfrei zerlegen. Sie tun es nicht:"
    echo ""
    echo "    ohne Label 'pmc' (-LE pmc)  : ${CE_OHNE_PMC}"
    echo "    mit  Label 'pmc' (-L  pmc)  : ${CE_MIT_PMC}"
    echo "    Summe                       : ${CE_PARTITION_SUMME}"
    echo "    Inventur (ctest -N)         : ${CE_GESAMT}"
    echo "    Differenz                   : $(( CE_PARTITION_SUMME - CE_GESAMT ))"
    echo ""
    echo "IST DIE SUMME GROESSER, traegt mindestens ein Test 'pmc' UND faellt zugleich"
    echo "in die Gegenauswahl -- dann zaehlt er doppelt und die Deckung ist zu gut"
    echo "gerechnet. IST SIE KLEINER, kennt die Inventur Tests, die KEINE der beiden"
    echo "Auswahlen sieht; sie laufen in keinem der beiden Job-Zweige."
    echo "SO WIRD DAS BEHOBEN: das Label am Test richtigstellen, nicht diese Pruefung."
    echo "-----------------------------------------------------------------------------"
fi

# -- Befund (c): NENNER GESCHRUMPFT -- der eigentliche D2-Defekt --
# Bis hierher hat die Wache nur gerechnet. Ohne den folgenden Block waere alles
# darueber blosse Anzeige: die uebersprungenen Bloecke stuenden in der Ausgabe
# und die letzte Zeile sagte trotzdem GRUEN. Genau so lief es bis 2026-08-09.
if [ -s "${_ce_tmp}/uebersprungene_bloecke.txt" ]; then
    _ce_fehlend=$(wc -l < "${_ce_tmp}/uebersprungene_bloecke.txt" | tr -d ' ')
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: NENNER GESCHRUMPFT -- ${CE_BLOCKS_UEBER} von ${CE_BLOCKS} bedingten Registrierungs-"
    echo "Bloecken wurden UEBERSPRUNGEN. ${_ce_fehlend} Test(s) sind in diesem Baum deshalb gar"
    echo "nicht erst registriert; die Inventur von ${CE_GESAMT} ist um sie zu klein. Jede Aussage"
    echo "dieser Wache ueber 'alle Tests' waere eine Aussage ueber die falsche Menge:"
    echo ""
    while IFS='|' read -r _ce_bk _ce_tn; do
        echo "    NICHT REGISTRIERT: ${_ce_tn}   [Block '${_ce_bk}']"
    done < "${_ce_tmp}/uebersprungene_bloecke.txt"
    echo ""
    echo "SO WIRD DAS BEHOBEN (nicht durch Aufweichen der Wache):"
    echo "  * Der Regelfall ist ein Bau-Baum, in dem ein 2-Pass-Werkzeug fehlt. Der Grund"
    echo "    steht oben je Block im Klartext; 'make inventar' baut ALLE Werkzeuge und"
    echo "    konfiguriert danach neu -- danach ist der Block AKTIV."
    echo "  * Soll ein Block in dieser Konfiguration wirklich nicht laufen, dann ist DIESER"
    echo "    Bau-Baum nicht der Wahrheitsbegriff der Wache. Die Wache gehoert an den Baum,"
    echo "    der die Voll-Suite faehrt -- sie wird nicht an den kleineren Baum angepasst."
    echo "-----------------------------------------------------------------------------"
    CE_NENNER_ROT=1
fi

# -- Befund (d): NENNER-WIDERSPRUCH -- die Gegenrichtung von (c) --
# Ein Block, der sich AKTIV meldet, dessen Test aber nicht in der Inventur steht.
# Ohne diese Probe waere "AKTIV" eine unpruefbare Behauptung -- und ein Protokoll,
# das man nicht widerlegen kann, ist kein Beleg, sondern eine zweite Meinung.
if [ -s "${_ce_tmp}/phantom_bloecke.txt" ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: NENNER-WIDERSPRUCH -- ein Registrierungs-Block meldet sich als GELAUFEN,"
    echo "sein Test steht aber nicht in der Inventur (${CE_GESAMT} Tests). Entweder wurde der Test"
    echo "umbenannt, ohne den Vermerk nachzuziehen, oder die Registrierung ist unterwegs"
    echo "verschluckt worden. Beides macht das Protokoll als Beleg wertlos:"
    echo ""
    while IFS='|' read -r _ce_bk _ce_tn; do
        echo "    BEHAUPTET, FEHLT: ${_ce_tn}   [Block '${_ce_bk}' meldet AKTIV]"
    done < "${_ce_tmp}/phantom_bloecke.txt"
    echo ""
    echo "SO WIRD DAS BEHOBEN: den Namen im comdare_registrierung_vermerken(TESTS ...)"
    echo "an den echten comdare_add_test()/add_test(NAME ...) angleichen -- beide stehen"
    echo "in derselben Datei, wenige Zeilen auseinander."
    echo "-----------------------------------------------------------------------------"
    CE_NENNER_ROT=1
fi

# -- Befund (e): NENNER GEGEN FREMDE QUELLE (D1c) -- die Aussensicht auf (c)/(d) --
if [ "$CE_FREMD_ROT" -ne 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: NENNER-ABWEICHUNG gegen die fremde Inventur (${CE_FREMD_STATUS})."
    echo "Das Protokoll oben belegt den Nenner gegen den QUELLTEXT DIESES Baums. Diese"
    echo "Achse belegt ihn gegen einen ANDEREN Baum, der denselben Quellstand baut."
    echo "Weichen die ab, ist mindestens einer der beiden nicht so gebaut wie gedacht --"
    echo "und eine Abdeckung ueber einem zu kleinen Nenner deckt nichts."
    if [ -s "${_ce_tmp}/nur_hier.txt" ]; then
        echo ""
        echo "  NUR IN DIESEM Baum registriert (im fremden fehlend):"
        while read -r _ce_t; do echo "    + ${_ce_t}"; done < "${_ce_tmp}/nur_hier.txt"
    fi
    if [ -s "${_ce_tmp}/nur_fremd.txt" ]; then
        echo ""
        echo "  NUR IM FREMDEN Baum registriert (hier fehlend):"
        while read -r _ce_t; do echo "    - ${_ce_t}"; done < "${_ce_tmp}/nur_fremd.txt"
    fi
    echo ""
    echo "SO WIRD DAS BEHOBEN (nicht durch Abschalten der Achse):"
    echo "  * Beide Jobs muessen denselben Bauweg fahren (./configure.sh && make bzw."
    echo "    'make inventar'). Fehlt in einem Baum das Reconfigure nach dem"
    echo "    Codegen-Bau, registriert CMake dort weniger Tests."
    echo "  * Fehlt die Datei, ist das Artefakt verlorengegangen oder 'needs:' zieht"
    echo "    es nicht -- ein CI-Verdrahtungsfehler, kein Test-Befund."
    echo "-----------------------------------------------------------------------------"
    CE_NENNER_ROT=1
fi

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

# -- HAUPTBEFUND: die eigentliche Invariante --
if [ "$CE_LUECKE" -gt 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: ABDECKUNGSLUECKE -- ${CE_LUECKE} Test(s) werden GEBAUT, aber von KEINEM"
    echo "fahrenden CI-Job ausgefuehrt. Ihre Zusicherungen sind wirkungslos:"
    echo ""
    # T-6 SCHWESTERSTELLE zum awk-Vergleich oben (2026-08-09): hier stand zweimal
    # 'grep -qx'. Dasselbe Muster, dieselbe Falle -- das blanke 'grep' ist je nach
    # PATH GNU grep 3.11 oder ugrep 7.5.0. Ein Testname mit Regex-Sonderzeichen
    # (die Suite hat welche mit '+') haette hier je nach Engine anders getroffen und
    # damit die falsche LABEL-Marke gedruckt. awk vergleicht $0 == n bytegleich.
    ce_namen -L pmc      > "${_ce_tmp}/label_pmc.txt"
    ce_namen -L contract > "${_ce_tmp}/label_contract.txt"
    ce_werkzeug_pruefen
    while read -r _ce_t; do
        _ce_marke="ohne contract/pmc-Label"
        if awk -v n="$_ce_t" 'BEGIN{f=1} $0==n {f=0} END{exit f}' "${_ce_tmp}/label_pmc.txt"; then
            _ce_marke="Label 'pmc' (Hardware-Klasse)"
        elif awk -v n="$_ce_t" 'BEGIN{f=1} $0==n {f=0} END{exit f}' "${_ce_tmp}/label_contract.txt"; then
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

# Der Nenner-Befund hat VORRANG vor der Abdeckungsluecke, und zwar mit eigenem
# Code. Beide zusammen sind moeglich; dann ist die Luecke die kleinere Nachricht:
# ueber einer nachweislich unvollstaendigen Inventur ist "X von Y gedeckt" gar
# nicht erst beantwortbar. Wer zuerst den Gegenstand richtigstellt, rechnet
# danach ohnehin neu.
if [ "$CE_NENNER_ROT" -ne 0 ]; then
    CE_RC=4
fi

if [ "$CE_RC" -eq 0 ]; then
    echo ""
    echo "PARTITIONS-BELEG (dieselbe Rechnung, nur andersherum gelesen):"
    echo "  bedingte Registrierungs-Bloecke     : ${CE_BLOCKS_AKTIV} von ${CE_BLOCKS} gelaufen (kein stiller Schwund)"
    # KEIN ZWEITER CTEST-AUFRUF: die drei Zahlen stehen schon fest, sie wurden oben
    # erhoben UND VERGLICHEN. Wuerde hier neu gemessen, koennte die gedruckte Zeile
    # von der geprueften abweichen -- dann belegte der "Beleg" wieder etwas anderes
    # als das, worueber geurteilt wurde.
    echo "  ohne Label 'pmc' (test:unit)        : ${CE_OHNE_PMC}"
    echo "  mit  Label 'pmc' (pmc:amd/intel)    : ${CE_MIT_PMC}"
    echo "  Summe (VERGLICHEN, nicht behauptet) : ${CE_PARTITION_SUMME}  ==  Inventur ${CE_GESAMT}"
    echo "  -> -L und -LE mit demselben Muster sind komplementaer; ein dritter Fall"
    echo "     existiert nicht. Die Deckung ist damit strukturell, nicht durch Audit."
    echo ""
    echo "  Untergrenze (committet)             : ${CE_GESAMT} >= ${CE_FLOOR}  (Quelle: ${_ce_floor_pfad})"
    echo "  Nenner gegen FREMDE Quelle (e)      : ${CE_FREMD_STATUS}"
    _ce_gates_ok=$(( CE_GATES_GESAMT - CE_GATES_UNGEPRUEFT ))
    echo "  Gates gegen gesetzte Variable       : ${_ce_gates_ok} von ${CE_GATES_GESAMT} (${CE_GATES_UNGEPRUEFT} ANNAHME)"
    echo ""
    echo "ABDECKUNGS-WACHE: GRUEN -- kein Test ohne fahrenden Job,"
    echo "und der Nenner ist belegt: ${CE_BLOCKS_AKTIV} von ${CE_BLOCKS} bedingten Bloecken gelaufen, 0 uebersprungen."
elif [ "$CE_RC" -eq 4 ]; then
    echo ""
    echo "ABDECKUNGS-WACHE: ROT (NENNER) -- diese Wache kennt ihren eigenen Gegenstand"
    echo "nicht vollstaendig. Sie sagt hier ausdruecklich NICHTS ueber die Abdeckung:"
    echo "eine Wache mit unvollstaendigem Nenner meldet Vollstaendigkeit und deckt nichts."
else
    echo ""
    echo "ABDECKUNGS-WACHE: ROT -- Nenner belegt (${CE_BLOCKS_AKTIV} von ${CE_BLOCKS} Bloecken gelaufen),"
    echo "die Abdeckung ueber dieser vollstaendigen Menge haelt aber nicht."
fi

exit "$CE_RC"
