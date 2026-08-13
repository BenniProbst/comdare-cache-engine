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
#   COMDARE_FREMD_HOST_PROBE=<datei>  (F1b, 2026-08-12) PFLICHT, sobald
#   COMDARE_FREMD_INVENTUR gesetzt ist: die Cache-Zeilen des fremden Baums
#   (COMDARE_HOST_RUNS_* samt _COMPILED/_EXITCODE) plus 'HOST=<hostname>', wie
#   test:unit sie nach build/Testing/ctest_unit_host_probe.txt schreibt. Damit
#   vergleicht die Achse KLASSEN, nicht blind Mengen. Fehlt/inkonsistent = ROT (4).
#   COMDARE_FREMD_SOLL_DELTA_BLOECKE="<kennung ...>"  (F1a) deklariert GEWOLLTE
#   Differenzen als Block-Kennungen des Registrierungs-Protokolls (Leerzeichen-
#   Liste). Abgezogen wird nur deklariert+AKTIV+nur-hier; fehlender/inaktiver
#   Block und stale Deklaration sind benannte ROT-Befunde (4). Abzug immer
#   ausgewiesen. Details am Achsen-Block unten.
#   COMDARE_WACHE_STRIKT=1  macht aus 'declared:VAR ungesetzt' einen Fehler statt
#   einer Annahme (s.u.). Ungesetzt/leer = weich; jeder andere Wert = Abbruch.
#   COMDARE_D2_FLOOR_PFAD=<datei>  lenkt die Untergrenze um (Selbsttest).
# EXIT:    0 = Invariante haelt
#          1 = Abdeckungsluecke: ein registrierter Test faehrt in keinem Job
#          2 = Bedienung/Umgebung. Dazu zaehlt ein fehlendes, leeres oder
#              abgerissenes Registrierungs-Protokoll: wer seinen Nenner nicht
#              belegen kann, meldet nicht gruen (fail-closed). Ebenso eine
#              fehlende/unlesbare Untergrenze, ein abgestuerztes ctest und --
#              nur bei COMDARE_WACHE_STRIKT=1 -- ein Gate ohne gesetzte
#              Deklarationsvariable (Verdrahtungsfehler: die Testmenge stimmt,
#              die Auskunft ueber die Jobs fehlt). Seit D2-G5 ausserdem: eine
#              nicht bestimmbare HOST-KLASSE (CMakeCache fehlt, ISA-Probe nicht
#              uebersetzt, Variable von aussen gesetzt, Wert unverstanden,
#              AVX-512F ohne AVX2) und eine Untergrenzen-Datei, die nicht fuer
#              alle drei Klassen genau eine Ganzzahl nennt. Eine Wache, die
#              nicht weiss, WELCHE Zahl fuer diesen Baum gilt, meldet nicht
#              gruen -- und raet erst recht nicht die niedrigste. NACHGEBESSERT
#              2026-08-10: "Variable von aussen gesetzt" heisst nicht mehr nur
#              "_COMPILED fehlt". Erzwingt jemand die Klasse auf einem BEREITS
#              konfigurierten Baum, ueberleben _COMPILED und _EXITCODE aus dem
#              ehrlichen Lauf -- geprueft wird deshalb der TYP der Wertzeile
#              (INTERNAL) und die Deckung von Wert und _EXITCODE.
#          3 = Manifest defekt
#          4 = NENNER-BEFUND: die Inventur ist nachweislich kleiner als das, was
#              dieser Baum registrieren muesste (uebersprungener Block), oder sie
#              widerspricht dem Protokoll (Block meldet AKTIV, Test fehlt), oder
#              sie weicht von der Inventur eines anderen Jobs ab (Befund (e)),
#              oder sie verfehlt den committeten Klassen-Anker -- seit #39
#              (2026-08-13) in BEIDE Richtungen: UNTER dem Anker wie bisher,
#              und UEBER dem Anker ohne Floor-Nachzug im selben Change.
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

# =============================================================================
# STRENG-MODUS (Lead-Entscheid 2026-08-09) -- ein eigener Schalter, kein Merkmal
# =============================================================================
# WAS ER BEANTWORTET: nicht "laufe ich in GitLab?", sondern "WIRD HIER
# VOLLSTAENDIGKEIT ERWARTET?". Das ist bewusst NICHT $CI_JOB_ID. Ein
# Umgebungs-Merkmal waere ein Stellvertreter fuer die eigentliche Frage, und die
# beiden fallen auseinander, sobald jemand die Wache von Hand fuer eine Abnahme
# faehrt: auf dem baremetal-Weg des Par. 61 Dual-Wegs waere $CI_JOB_ID leer, die
# Wache liefe weich, und die Abnahme saehe aus wie ein Freispruch. Genau die
# Sorte Stillschweigen, gegen die dieser Bogen gebaut ist.
#
# WAS ER TUT: 'declared:VAR' ist im weichen Modus fail-OPEN -- ein Gate ohne
# gesetzte Variable gilt als deklariert und bekommt die Deckung gutgeschrieben.
# Lokal ist das richtig (dort setzt niemand CI-Variablen). Im strengen Modus ist
# eine fehlende Auskunft ein VERDRAHTUNGS-Fehler und damit Exit 2: der Job hat
# seine Variable nicht gesetzt, und eine Deckung aus einer Annahme ist keine.
#
# WERTE: '1' = streng, ungesetzt/leer = weich. JEDER ANDERE WERT IST ABBRUCH --
# dieselbe Doktrin wie bei der Untergrenze: ein unlesbarer Schalter wird nicht
# als "dann eben nicht" gelesen. Wer 'COMDARE_WACHE_STRIKT=ja' schreibt, meinte
# streng und bekaeme sonst still weich.
CE_STRIKT=${COMDARE_WACHE_STRIKT-}
case "$CE_STRIKT" in
    '')  CE_STRIKT_MODUS="WEICH  (COMDARE_WACHE_STRIKT ungesetzt)" ;;
    1)   CE_STRIKT_MODUS="STRENG (COMDARE_WACHE_STRIKT=1)" ;;
    *)
        _ce_m="COMDARE_WACHE_STRIKT hat den Wert '${CE_STRIKT}'. Erlaubt ist '1'"
        _ce_m="${_ce_m} (streng) oder ungesetzt/leer (weich). Ein unlesbarer Schalter"
        _ce_m="${_ce_m} wird nicht als 'weich' gelesen -- sonst liefe ein als streng"
        _ce_m="${_ce_m} gemeinter Lauf still weich durch."
        ce_abbruch "$_ce_m" 2
        ;;
esac

echo "============================================================================="
echo " ABDECKUNGS-WACHE der CI-Test-Auswahl  (scripts/ci_test_coverage_guard.sh)"
echo " Bau-Baum : ${CE_BUILD_DIR}"
echo " Manifest : ${_ce_manifest}"
# DER MODUS STEHT IMMER DA, in beiden Faellen. Sonst ist einem gruenen Ergebnis
# nicht anzusehen, ob es streng zustande kam -- und ein Freispruch, dessen
# Massstab man nicht kennt, ist keiner.
echo " Modus    : ${CE_STRIKT_MODUS}"
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
# und aendert sich nur durch einen Menschen, seit #39 im SELBEN Change wie die
# Registrierung, die sie ueberholt.
#
# FAIL-CLOSED: fehlt die Datei oder ist ihr Inhalt keine einzelne Ganzzahl, ist
# das ABBRUCH (Exit 2), nicht 'dann eben ohne Untergrenze'. Eine Wache, die ihre
# Vergleichsgroesse verliert und trotzdem gruen meldet, hat die Pruefung nur
# aufgehuebscht.
#
# UEBERSCHREITEN IST SEIT #39 (2026-08-13) EIN NENNER-BEFUND: die Zahl je Klasse
# ist ein EXAKTER Anker, keine blosse Untergrenze mehr. Eine Inventur UEBER dem
# Anker heisst: jemand hat registriert, ohne die Sprossen nachzuziehen -- genau
# so sind 5 Alt-Pakete mit 7 Tests durchgerutscht (S-3-Abnahme 2026-08-13,
# Wiederholungsmuster). Der Guard erzwingt damit BAUM-Konsistenz: Floor-Datei
# und Registrierungen gehoeren in DENSELBEN Change (Commit-Granularitaet kann
# er nicht sehen). Die Wache schreibt die Datei weiterhin NIEMALS selbst: ein
# Anker, der sich selbst nachfuehrt, ist keiner.
#
# HISTORIE (Wortlaut bis #39, deprecated): "UEBERSCHREITEN IST KEIN FEHLER:
# waechst die Testzahl, ist das der Normalfall. Die Wache sagt dann, dass die
# Datei nachgezogen gehoert -- in einem eigenen, im Diff sichtbaren Commit."
# =============================================================================
# -----------------------------------------------------------------------------
# ZUERST: DIE HOST-KLASSE. EINE ZAHL OHNE HOST IST EINE ZAHL OHNE GEGENSTAND.
# (D2-G5, 2026-08-10 -- der Nachtrag, ohne den die Untergrenze oben nicht gilt)
# -----------------------------------------------------------------------------
# WAS VORHER FALSCH WAR: es gab GENAU EINE committete Zahl, und ihr Kopf behauptete,
# sie gelte "auf JEDER Hardware-Klasse". Das war nachrechenbar unwahr. Sechs
# ctest-Registrierungen dieses Repos haengen an der ISA des BAU-HOSTS
# (tests/unit/CMakeLists.txt:4165/5330/5343/5372/5375, Stand 83a6d443 nachgemessen --
# die frueher hier stehenden Anker 4134/5249/5262/5291/5294 galten nur vor dem Merge
# b7d82d73 und loesen heute auf Kommentare auf) -- vier an AVX-512F, zwei an
# AVX2. Eine Maschine ohne AVX2 registriert also sechs Tests weniger als diese hier,
# voellig zu Recht, und riss damit eine Untergrenze, die nur vier abgezogen hatte.
# Die Wache haette rot gemeldet, und der Baum waere in Ordnung gewesen.
#
# WOHER DIE KLASSE KOMMT -- und warum NICHT aus /proc/cpuinfo: gefragt ist nicht,
# welche ISA die Maschine kann, auf der diese Wache gerade laeuft, sondern welche
# der BAU dieses Baums vorgefunden hat. Das sind zwei verschiedene Dinge, sobald
# Container, Cross-Build oder ein zweiter Compiler im Spiel sind. Die einzige
# Quelle, die genau das festhaelt, ist der CMakeCache DESSELBEN Baums: CMake hat
# die Probe (check_cxx_source_runs mit __builtin_cpu_supports) dort wirklich
# gefahren und ihr Ergebnis hinterlegt.
#
# FAIL-CLOSED, UND ZWAR SCHAERFER ALS ES AUSSIEHT: der blosse Wert genuegt nicht.
# AM OBJEKT GEMESSEN (2026-08-10, CMake 4.3.4, /tmp/d2probe_483e0110), und im
# Modul-Quelltext gegengelesen (Modules/Internal/CheckSourceRuns.cmake:95 try_run
# schreibt _EXITCODE und _COMPILED; :113 setzt den Wert auf 1; :121 auf LEER):
#
#   FALL                              WERTZEILE               _COMPILED   _EXITCODE
#   Probe laeuft, Exit 0 ........... VAR:INTERNAL=1           TRUE        0
#   Probe laeuft, Exit 1 ........... VAR:INTERNAL=            TRUE        1
#   Probe kompiliert nicht ......... VAR:INTERNAL=            FALSE       (fehlt)
#   -D auf FRISCHEN Baum ........... VAR:UNINITIALIZED=0      (fehlt)     (fehlt)
#   -D:BOOL auf frischen Baum ...... VAR:BOOL=0               (fehlt)     (fehlt)
#   -D auf KONFIGURIERTEN Baum ..... VAR:UNINITIALIZED=0      TRUE        0
#   -D:BOOL auf konfigurierten Baum  VAR:BOOL=0               TRUE        0
#   -D:INTERNAL=0 auf konfiguriert . VAR:INTERNAL=0           TRUE        0
#   -D:INTERNAL=  auf konfiguriert . VAR:INTERNAL=            TRUE        0
#
# DIE VIER UNTEREN ZEILEN SIND DER GRUND FUER DIESE NACHBESSERUNG (2026-08-10).
# CMake entfernt _COMPILED und _EXITCODE beim Erzwingen NIE -- sie ueberleben aus
# dem ehrlichen Lauf davor. Eine Wache, die bloss die ANWESENHEIT von _COMPILED
# prueft, nimmt deshalb eine ERZWUNGENE Klasse als gemessene an. Am Objekt
# vorgefuehrt: derselbe Baum, 490 Tests, ehrlicher Cache -> Exit 4 "UNTERSCHRITTEN
# um 6 Test(e)"; erzwungener Cache -> "HOST-KLASSE (gemessen): basis" und kein
# Befund. Sechs fehlende Tests verschwanden lautlos. Und das Verfahren, das dahin
# fuehrt, ist kein Missbrauch: der Kopf von ci_test_inventory_floor.txt schreibt es
# als Gegenorakel selbst vor.
#
# DREI UNABHAENGIGE MERKMALE, ALLE DREI PFLICHT:
#  (1) DER TYP DER WERTZEILE MUSS 'INTERNAL' SEIN. check_cxx_source_runs schreibt
#      'CACHE INTERNAL'; jede Form von aussen schreibt einen anderen Typ
#      (UNINITIALIZED ohne Typangabe, sonst den angegebenen). Der TYP ist das
#      unterscheidende Merkmal -- nicht die Anwesenheit von _COMPILED, und auch
#      nicht das Wort 'UNINITIALIZED': '-DVAR:BOOL=0' umginge eine schwarze Liste.
#  (2) _COMPILED MUSS 'TRUE' SEIN. Sonst ist der leere Wert zweideutig: "diese CPU
#      kann es nicht" und "der Compiler hat die Probe nicht uebersetzt" sehen
#      identisch aus, und ein kaputter Werkzeugkasten wuerde als 'basis' gegen die
#      NIEDRIGSTE Untergrenze verglichen.
#  (3) WERT UND _EXITCODE MUESSEN SICH DECKEN. Der Wert ist 1 GENAU DANN, wenn der
#      Exit-Code 0 war (:112-113); leer sonst (:121). Eine 0 als Wert hat nie eine
#      Probe geschrieben. Ohne (3) genuegte EIN ':INTERNAL' mehr auf derselben
#      Kommandozeile, um (1) zu besiegen -- gemessen, siehe die letzten zwei Zeilen
#      der Tabelle. (3) prueft die Wertzeile gegen einen Beleg daneben, den das
#      Erzwingen nicht mitschreibt.
# Faellt eines der drei, ist die Klasse UNBEKANNT -- und das ist ABBRUCH, nicht
# 'basis' und damit die niedrigste Untergrenze.
_ce_cache="${CE_BUILD_DIR}/CMakeCache.txt"
if [ ! -f "$_ce_cache" ]; then
    _ce_m="die Host-Klasse ist nicht bestimmbar: '${_ce_cache}' fehlt. Ohne sie"
    _ce_m="${_ce_m} waere jede Untergrenze eine Zahl ohne Gegenstand. Fail-closed:"
    _ce_m="${_ce_m} das ist ABBRUCH, nicht die hoechste und nicht die niedrigste Annahme."
    ce_abbruch "$_ce_m" 2
fi

# =============================================================================
# DIE KLASSEN-ABLEITUNG ALS FUNKTION (F1b, 2026-08-12): ZWEI SEITEN, EIN VERFAHREN
# =============================================================================
# Bis heute lief diese Ableitung inline und kannte genau EINEN Gegenstand: den
# CMakeCache DIESES Baums. Die dritte Achse unten braucht dieselbe Ableitung fuer
# die GEGENSEITE -- test:unit veroeffentlicht seine Cache-Zeilen als Host-Probe
# (COMDARE_FREMD_HOST_PROBE), und zwei Abschriften desselben Verfahrens waeren
# die Bauform, aus der Drift entsteht. Deshalb EINE Funktion mit zwei Modi:
#   eigen -> jeder Defekt ist ABBRUCH (Exit 2), wie bisher: ohne die eigene
#            Klasse hat KEINE Zahl dieses Laufs einen Gegenstand.
#   fremd -> jeder Defekt ist ein BENANNTER BEFUND (Rueckgabe 1, Meldung auf
#            stdout); der Rufer macht daraus CE_FREMD_ROT und damit Exit 4.
#            Eine kaputte Probe faellt der dritten Achse zur Last, nicht dem
#            ganzen Lauf -- aber sie faellt (fail-closed), sie verschwindet nie.
# Alle Merkmals-Pruefungen (Typ INTERNAL, _COMPILED, Wert<->_EXITCODE, Leiter)
# gelten fuer BEIDE Seiten unveraendert: die Probe ist eine woertliche Abschrift
# der Cache-Zeilen, also gilt fuer sie derselbe Faelschungs-Schutz.
# Ergebnis steht in CE_HK_KLASSE / CE_HK_AVX2 / CE_HK_AVX512F.
ce_hk_befund() {
    if [ "$_ce_hk_modus" = eigen ]; then
        ce_abbruch "$1" 2
    fi
    echo "  HOST-PROBE (fremd) UNBRAUCHBAR: $1"
    return 1
}

ce_host_klasse_lesen() {   # $1 = Datei mit CMakeCache-Zeilen, $2 = eigen|fremd
    _ce_hk_datei=$1
    _ce_hk_modus=$2
    CE_HK_KLASSE=""
    CE_HK_AVX2=""
    CE_HK_AVX512F=""
    # Die Schleife laeuft ABSICHTLICH in der aktuellen Shell und nicht in $( ): ein
    # ce_abbruch in einer Kommando-Substitution beendete nur die Subshell, und die
    # Wache liefe mit leerer Klasse weiter -- derselbe Riss, den ce_namen() oben
    # bereits einmal hatte. (Als Funktion unveraendert wahr: der Funktionsrumpf
    # laeuft in der aktuellen Shell, 'return 1' erreicht den Rufer.)
    for _ce_hv in COMDARE_HOST_RUNS_AVX2 COMDARE_HOST_RUNS_AVX512F; do
        _ce_hv_zeile=$(sed -n "/^${_ce_hv}:/p" "$_ce_hk_datei" | sed -n '1p')
        _ce_hv_da=$(sed -n "/^${_ce_hv}:/p" "$_ce_hk_datei" | wc -l | tr -d ' ')
        if [ "$_ce_hv_da" -eq 0 ]; then
            _ce_m="'${_ce_hv}' steht nicht in '${_ce_hk_datei}'. Dieser Baum wurde ohne die"
            _ce_m="${_ce_m} ISA-Probe konfiguriert (Cross-Build?) -- die Host-Klasse ist damit"
            _ce_m="${_ce_m} UNBEKANNT, nicht 'basis'."
            ce_hk_befund "$_ce_m" || return 1
        fi
        # ... und GENAU EINMAL. AM OBJEKT GEMESSEN (2026-08-10): haengt man eine zweite
        # Wertzeile an, nimmt CMake beim Laden die LETZTE -- diese Wache liest die erste.
        # Ein Cache mit einer ehrlichen Zeile oben und einer erzwungenen unten liefe hier
        # als 'gemessen' durch, waehrend der Bau die untere benutzt hat. CMake selbst legt
        # jeden Eintrag nur einmal an; zwei Zeilen heisst von Hand bearbeitet.
        if [ "$_ce_hv_da" -gt 1 ]; then
            _ce_m="'${_ce_hv}' steht ${_ce_hv_da}-mal in '${_ce_hk_datei}'. CMake schreibt jeden"
            _ce_m="${_ce_m} Eintrag genau einmal und nimmt beim Lesen die LETZTE Zeile; welche"
            _ce_m="${_ce_m} davon den Bau bestimmt hat, ist von aussen nicht mehr entscheidbar."
            _ce_m="${_ce_m} Ein von Hand bearbeiteter Cache ist kein Messergebnis."
            ce_hk_befund "$_ce_m" || return 1
        fi

        # MERKMAL (1): DER TYP. 'NAME:TYP=WERT' -- alles zwischen dem ersten ':' und dem
        # ersten '=' ist der Typ. Nur 'INTERNAL' stammt aus check_cxx_source_runs; jede
        # andere Angabe kommt von aussen. Das wird VOR _COMPILED geprueft, denn _COMPILED
        # ueberlebt das Erzwingen und beweist deshalb fuer sich genommen gar nichts.
        _ce_hv_typ=${_ce_hv_zeile#*:}
        _ce_hv_typ=${_ce_hv_typ%%=*}
        if [ "$_ce_hv_typ" != "INTERNAL" ]; then
            _ce_m="'${_ce_hv}' steht in '${_ce_hk_datei}' mit dem Typ '${_ce_hv_typ}', nicht"
            _ce_m="${_ce_m} INTERNAL. check_cxx_source_runs schreibt ausschliesslich INTERNAL --"
            _ce_m="${_ce_m} diese Zeile hat also ein '-D' geschrieben, keine Probe. Eine daneben"
            _ce_m="${_ce_m} stehende '${_ce_hv}_COMPILED'-Zeile aendert daran NICHTS: sie ueberlebt"
            _ce_m="${_ce_m} das Erzwingen aus dem ehrlichen Lauf davor. Eine behauptete Host-Klasse"
            _ce_m="${_ce_m} ist keine gemessene."
            ce_hk_befund "$_ce_m" || return 1
        fi

        # MERKMAL (2): _COMPILED. Fehlt sie, lief die Probe nie (frischer Baum + -D).
        _ce_hv_comp_da=$(sed -n "/^${_ce_hv}_COMPILED:/p" "$_ce_hk_datei" | wc -l | tr -d ' ')
        if [ "$_ce_hv_comp_da" -eq 0 ]; then
            _ce_m="'${_ce_hv}' steht in '${_ce_hk_datei}', aber '${_ce_hv}_COMPILED' fehlt."
            _ce_m="${_ce_m} Die Probe wurde also nie gefahren -- die Variable ist von aussen"
            _ce_m="${_ce_m} gesetzt worden. Eine behauptete Host-Klasse ist keine gemessene."
            ce_hk_befund "$_ce_m" || return 1
        fi
        _ce_hv_comp=$(sed -n "s/^${_ce_hv}_COMPILED:[^=]*=//p" "$_ce_hk_datei" | sed -n '1p')
        if [ "$_ce_hv_comp" != "TRUE" ]; then
            _ce_m="'${_ce_hv}_COMPILED' ist '${_ce_hv_comp}', nicht TRUE: die ISA-Probe hat"
            _ce_m="${_ce_m} nicht einmal uebersetzt. Ihr leeres Ergebnis heisst deshalb 'unbekannt',"
            _ce_m="${_ce_m} nicht 'diese CPU kann es nicht'."
            ce_hk_befund "$_ce_m" || return 1
        fi

        # Der WERT. Eine Probe schreibt 1 (CheckSourceRuns.cmake:113) oder LEER (:121) --
        # eine 0 hat nie eine geschrieben, auch wenn der Typ INTERNAL lautet.
        _ce_hv_wert=$(sed -n "s/^${_ce_hv}:[^=]*=//p" "$_ce_hk_datei" | sed -n '1p')
        case "$_ce_hv_wert" in
            1)  _ce_hf=ja ;;
            '') _ce_hf=nein ;;
            *)
                _ce_m="'${_ce_hv}' hat den Wert '${_ce_hv_wert}'. Erwartet ist 1 oder leer --"
                _ce_m="${_ce_m} etwas anderes schreibt check_cxx_source_runs nicht. Ein"
                _ce_m="${_ce_m} unverstandener Wert wird nicht zu 'nein' gerundet."
                ce_hk_befund "$_ce_m" || return 1
                ;;
        esac

        # MERKMAL (3): WERT GEGEN _EXITCODE. Der zweite, unabhaengige Beleg -- er liegt in
        # einer Zeile, die das Erzwingen NICHT mitschreibt, und deckt deshalb auch eine
        # Faelschung ab, die den Typ INTERNAL korrekt trifft.
        _ce_hv_ec_da=$(sed -n "/^${_ce_hv}_EXITCODE:/p" "$_ce_hk_datei" | wc -l | tr -d ' ')
        if [ "$_ce_hv_ec_da" -eq 0 ]; then
            _ce_m="'${_ce_hv}_COMPILED' ist TRUE, aber '${_ce_hv}_EXITCODE' fehlt in"
            _ce_m="${_ce_m} '${_ce_hk_datei}'. try_run legt beide gemeinsam an -- fehlt eine,"
            _ce_m="${_ce_m} ist dieser Cache von Hand bearbeitet und kein Messergebnis."
            ce_hk_befund "$_ce_m" || return 1
        fi
        _ce_hv_ec=$(sed -n "s/^${_ce_hv}_EXITCODE:[^=]*=//p" "$_ce_hk_datei" | sed -n '1p')
        if [ "$_ce_hf" = ja ] && [ "$_ce_hv_ec" != "0" ]; then
            _ce_m="'${_ce_hv}' ist 1, aber '${_ce_hv}_EXITCODE' ist '${_ce_hv_ec}'."
            _ce_m="${_ce_m} CMake setzt den Wert GENAU DANN auf 1, wenn der Exit-Code 0 war"
            _ce_m="${_ce_m} (CheckSourceRuns.cmake:112-113). Wert und Beleg widersprechen sich --"
            _ce_m="${_ce_m} das ist kein Messergebnis."
            ce_hk_befund "$_ce_m" || return 1
        fi
        if [ "$_ce_hf" = nein ] && [ "$_ce_hv_ec" = "0" ]; then
            _ce_m="'${_ce_hv}' ist leer, aber '${_ce_hv}_EXITCODE' ist 0: die Probe lief und"
            _ce_m="${_ce_m} war ERFOLGREICH. Dann haette CMake 1 geschrieben"
            _ce_m="${_ce_m} (CheckSourceRuns.cmake:112-113). Der leere Wert behauptet 'diese CPU"
            _ce_m="${_ce_m} kann es nicht' und wird vom Beleg daneben widerlegt -- Klasse UNBEKANNT."
            ce_hk_befund "$_ce_m" || return 1
        fi

        case "$_ce_hv" in
            COMDARE_HOST_RUNS_AVX2)    CE_HK_AVX2=$_ce_hf ;;
            COMDARE_HOST_RUNS_AVX512F) CE_HK_AVX512F=$_ce_hf ;;
        esac
    done

    # Die Leiter hat drei Sprossen und KEINE vierte. 'AVX-512F ohne AVX2' gibt es auf
    # keiner x86-Maschine; stuende es da, waere nicht die Klasse ungewoehnlich, sondern
    # der Cache unglaubwuerdig -- und dann taugt auch die andere Zeile nichts.
    if [ "$CE_HK_AVX2" = ja ] && [ "$CE_HK_AVX512F" = ja ]; then
        CE_HK_KLASSE=avx512f
    elif [ "$CE_HK_AVX2" = ja ] && [ "$CE_HK_AVX512F" = nein ]; then
        CE_HK_KLASSE=avx2
    elif [ "$CE_HK_AVX2" = nein ] && [ "$CE_HK_AVX512F" = nein ]; then
        CE_HK_KLASSE=basis
    else
        _ce_m="widerspruechliche Host-Klasse in '${_ce_hk_datei}': AVX-512F=${CE_HK_AVX512F},"
        _ce_m="${_ce_m} AVX2=${CE_HK_AVX2}. AVX-512F ohne AVX2 existiert nicht -- dieser Cache"
        _ce_m="${_ce_m} beschreibt keine reale Maschine."
        ce_hk_befund "$_ce_m" || return 1
    fi
    return 0
}

# Die Sprossen-Ordnung der Leiter als Zahl -- fuer den Klassen-VERGLEICH der
# dritten Achse (wer steht hoeher?). KEINE vierte Sprosse, wie oben.
ce_klasse_rang() {
    case "$1" in
        avx512f) echo 3 ;;
        avx2)    echo 2 ;;
        basis)   echo 1 ;;
        *)       echo 0 ;;
    esac
}

ce_host_klasse_lesen "$_ce_cache" eigen
CE_HOST_AVX2=$CE_HK_AVX2
CE_HOST_AVX512F=$CE_HK_AVX512F
CE_HOST_KLASSE=$CE_HK_KLASSE

_ce_hostname=$( (hostname 2>/dev/null || echo unbekannt) | sed -n '1p')
echo ""
echo "HOST-KLASSE (gemessen, ${_ce_cache}): ${CE_HOST_KLASSE}"
echo "  COMDARE_HOST_RUNS_AVX2=${CE_HOST_AVX2}, COMDARE_HOST_RUNS_AVX512F=${CE_HOST_AVX512F}, Host: ${_ce_hostname}"

_ce_floor_pfad=${COMDARE_D2_FLOOR_PFAD:-${_ce_dir}/ci_test_inventory_floor.txt}
if [ ! -f "$_ce_floor_pfad" ]; then
    _ce_m="die committete Untergrenze fehlt: '${_ce_floor_pfad}'."
    _ce_m="${_ce_m} Ohne sie hat diese Wache keine einzige Zahl, die nicht aus dem"
    _ce_m="${_ce_m} geprueften Baum selbst stammt (V-7). Anlegen: eine Zeile JE"
    _ce_m="${_ce_m} HOST-KLASSE, Form '<klasse> <ganzzahl>' fuer avx512f, avx2 und"
    _ce_m="${_ce_m} basis; '#'-Kommentare sind erlaubt."
    ce_abbruch "$_ce_m" 2
fi

# Kommentare weg, Rand-Leerraum weg, innere Leerraum-Laeufe auf EIN Leerzeichen,
# Leerzeilen weg. Frueher stand hier 's/[[:space:]]//g' -- das entfernte AUCH das
# Trennzeichen zwischen Klasse und Zahl und ist mit dem Format unvereinbar.
# Das abschliessende 'echo' erzwingt einen Zeilenabschluss, damit eine Datei ohne
# schliessenden Zeilenumbruch nicht als 0 Zeilen durchgeht.
{
    sed -e 's/#.*$//' -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' \
        -e 's/[[:space:]][[:space:]]*/ /g' "$_ce_floor_pfad"
    echo
} | sed '/^$/d' > "${_ce_tmp}/floor.txt"

CE_FLOOR=""
# F1b (2026-08-12): ALLE DREI Sprossen werden behalten, nicht nur die eigene.
# Die dritte Achse vergleicht unten zwei KLASSEN miteinander; die erwartete
# Differenz zweier Klassen ist die Differenz ihrer Leiter-Zahlen -- dieselbe
# committete Quelle, NUR GELESEN, nie fortgeschrieben.
CE_FLOOR_AVX512F=""
CE_FLOOR_AVX2=""
CE_FLOOR_BASIS=""
_ce_floor_gesehen=""
while IFS= read -r _ce_fz; do
    _ce_fk=${_ce_fz%% *}
    _ce_fw=${_ce_fz#* }
    if [ "$_ce_fk" = "$_ce_fz" ]; then
        # Genau die alte Bauform: eine nackte Zahl ohne Klasse. Sie wird NICHT
        # stillschweigend als 'gilt ueberall' gelesen -- das war der Defekt.
        _ce_m="'${_ce_floor_pfad}' enthaelt die Zeile '${_ce_fz}' ohne Host-Klasse."
        _ce_m="${_ce_m} Die alte Bauform (eine nackte Zahl fuer alle Maschinen) ist seit"
        _ce_m="${_ce_m} D2-G5 ungueltig: sie galt nachweislich nicht auf jeder Klasse."
        _ce_m="${_ce_m} Erwartet ist '<klasse> <ganzzahl>' fuer avx512f, avx2 und basis."
        ce_abbruch "$_ce_m" 2
    fi
    case "$_ce_fk" in
        avx512f | avx2 | basis) ;;
        *)
            _ce_m="'${_ce_floor_pfad}' nennt die Host-Klasse '${_ce_fk}'. Bekannt sind"
            _ce_m="${_ce_m} genau drei: avx512f, avx2, basis. Eine unbekannte Klasse wird"
            _ce_m="${_ce_m} nicht uebergangen -- sie hiesse, dass die Leiter nicht mehr stimmt."
            ce_abbruch "$_ce_m" 2
            ;;
    esac
    case " ${_ce_floor_gesehen} " in
        *" ${_ce_fk} "*)
            _ce_m="'${_ce_floor_pfad}' nennt die Klasse '${_ce_fk}' mehrfach, erwartet ist"
            _ce_m="${_ce_m} GENAU EINE Wertzeile je Klasse. Zwei Zahlen fuer dieselbe"
            _ce_m="${_ce_m} Klasse sind keine Untergrenze, sondern eine offene Frage."
            ce_abbruch "$_ce_m" 2
            ;;
    esac
    case "$_ce_fw" in
        '' | *[!0-9]*)
            _ce_m="die Untergrenze der Klasse '${_ce_fk}' in '${_ce_floor_pfad}' ist"
            _ce_m="${_ce_m} keine reine Ganzzahl, sondern '${_ce_fw}'. Fail-closed: eine"
            _ce_m="${_ce_m} unlesbare Untergrenze wird nicht als 'keine' behandelt."
            ce_abbruch "$_ce_m" 2
            ;;
    esac
    _ce_floor_gesehen="${_ce_floor_gesehen} ${_ce_fk}"
    case "$_ce_fk" in
        avx512f) CE_FLOOR_AVX512F=$_ce_fw ;;
        avx2)    CE_FLOOR_AVX2=$_ce_fw ;;
        basis)   CE_FLOOR_BASIS=$_ce_fw ;;
    esac
    [ "$_ce_fk" = "$CE_HOST_KLASSE" ] && CE_FLOOR=$_ce_fw
done < "${_ce_tmp}/floor.txt"

# Nachschlagen einer Sprosse fuer den Klassen-Vergleich der dritten Achse --
# NACH der Schleife definiert, damit klar ist: es liest nur, was oben bereits
# vollstaendig validiert wurde (alle drei Klassen, genau eine Zahl je Klasse).
ce_klasse_floor() {
    case "$1" in
        avx512f) echo "$CE_FLOOR_AVX512F" ;;
        avx2)    echo "$CE_FLOOR_AVX2" ;;
        basis)   echo "$CE_FLOOR_BASIS" ;;
    esac
}

# ALLE DREI muessen dastehen, nicht nur die, die dieser Host gerade braucht.
# Sonst faellt eine fehlende Klasse erst auf der Maschine auf, die sie braucht --
# also genau dort, wo niemand hinschaut, und Monate spaeter.
for _ce_fk in avx512f avx2 basis; do
    case " ${_ce_floor_gesehen} " in
        *" ${_ce_fk} "*) ;;
        *)
            _ce_m="'${_ce_floor_pfad}' hat keine Zeile fuer die Host-Klasse '${_ce_fk}'."
            _ce_m="${_ce_m} Verlangt sind alle drei (avx512f, avx2, basis) -- eine fehlende"
            _ce_m="${_ce_m} Klasse faellt sonst erst auf der Maschine auf, die sie braucht."
            ce_abbruch "$_ce_m" 2
            ;;
    esac
done

echo "UNTERGRENZE (committet, ${_ce_floor_pfad}) fuer Klasse ${CE_HOST_KLASSE}: ${CE_FLOOR}"
if [ "$CE_GESAMT" -lt "$CE_FLOOR" ]; then
    echo "  -> UNTERSCHRITTEN um $(( CE_FLOOR - CE_GESAMT )) Test(e):"
    echo "     ${CE_GESAMT} von mindestens ${CE_FLOOR} fuer Klasse ${CE_HOST_KLASSE} (Host: ${_ce_hostname})."
    CE_NENNER_ROT=1
elif [ "$CE_GESAMT" -gt "$CE_FLOOR" ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: FLOOR-NACHZUG FEHLT -- die Inventur liegt UEBER dem committeten Anker:"
    echo "  ${CE_GESAMT} registriert gegen Anker ${CE_FLOOR} fuer Klasse ${CE_HOST_KLASSE}, also"
    echo "  $(( CE_GESAMT - CE_FLOOR )) Test(e) ohne Nachzug (Host: ${_ce_hostname})."
    echo "Seit #39 (2026-08-13) ist die Zahl je Klasse ein EXAKTER Anker, keine blosse"
    echo "Untergrenze: 5 Alt-Pakete registrierten 7 Tests, ohne die Sprossen anzufassen"
    echo "(S-3-Abnahme 2026-08-13) -- dieser Schlupf ist damit zu."
    echo "SO WIRD DAS BEHOBEN (nicht durch Aufweichen der Wache):"
    echo "  * die drei Sprossen LIVE nachmessen -- Rezept im Kopf von"
    echo "    scripts/ci_test_inventory_floor.txt: Klassen per cmake erzwingen, danach"
    echo "    'cmake -U' auf beide Variablen, Namensdiffs mit LC_ALL=C comm in BEIDE"
    echo "    Richtungen --"
    echo "  * und ALLE DREI Klassenzeilen im SELBEN Change nachziehen wie die"
    echo "    Registrierung, die sie ueberholt hat."
    echo "-----------------------------------------------------------------------------"
    CE_NENNER_ROT=1
else
    echo "  -> genau erreicht: ${CE_GESAMT} von mindestens ${CE_FLOOR} fuer Klasse"
    echo "     ${CE_HOST_KLASSE} (Host: ${_ce_hostname})."
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

# -- DER FREISPRUCH BRAUCHT EINEN BELEG (2026-08-10) --------------------------
# Bis heute wurde das ERGEBNIS dieser Rechnung an genau zwei Stellen sichtbar:
# im Widerspruchs-Block (Exit 4) und im PARTITIONS-BELEG ganz unten -- und der
# laeuft NUR bei CE_RC == 0. Auf JEDEM Baum mit Exit 1 (Phantom-Gate, toter
# Name, Abdeckungsluecke) war der Ausgang dieser Rechnung damit UNSICHTBAR, und
# die Schlusszeile sagte trotzdem "Nenner belegt".
#
# WARUM DAS NICHT NUR KOSMETIK IST: eine Gegenprobe ("eine heile Partition MUSS
# schweigen") kann ohne diese Zeile nicht geschrieben werden. Sie haette nur die
# ABWESENHEIT des Widerspruchs zu pruefen -- und die haelt auch dann, wenn hier
# nie gerechnet wurde oder das Werkzeug gar nicht erst startete. Das ist die
# Klasse "gruenes Gate ohne Gegenstand".
#
# Es ist dieselbe Lehre wie im Kopf dieses Abschnitts, eine Ebene tiefer: dort
# war das '==' eine ZEICHENKETTE statt eines Vergleichs, hier ist der Beleg an
# den gruenen Zweig gebunden -- ein Beleg, den nur der gruene Zweig druckt,
# belegt den gruenen Zweig und sonst nichts.
#
# Die Zeile steht DESHALB unbedingt, traegt alle fuenf Zahlen und ist in beiden
# Ausgaengen dieselbe: der Unterschied liegt in 'differenz', nicht in der Form.
# Der ausfuehrliche Widerspruchs-Block unten bleibt unveraendert: er ERKLAERT
# den Befund, diese Zeile BELEGT die Rechnung. Zwei Aufgaben, zwei Stellen.
_ce_pdiff=$(( CE_PARTITION_SUMME - CE_GESAMT ))
echo ""
printf 'PARTITIONS-RECHNUNG: ohne=%s mit=%s summe=%s inventur=%s differenz=%s\n' \
    "$CE_OHNE_PMC" "$CE_MIT_PMC" "$CE_PARTITION_SUMME" "$CE_GESAMT" "$_ce_pdiff"

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
# -----------------------------------------------------------------------------
# F1 (2026-08-12): DIE ACHSE WIRD KLASSENBEWUSST UND KENNT EIN DEKLARIERTES DELTA.
# Der nackte Mengen-Vergleich oben (jede Differenz = rot) hatte zwei blinde
# Flecken, beide am Objekt aufgetreten:
#   * GEWOLLTE Differenz: der Guard-Baum laedt den Fixture-Pruefling (T-4),
#     test:unit absichtlich nicht -- ein Test Unterschied, fuer immer, by design.
#     Ohne Deklaration waere JEDE Pipeline am Guard rot (W3).
#     -> COMDARE_FREMD_SOLL_DELTA_BLOECKE: Block-Kennungen (Leerzeichen-Liste)
#        aus dem Registrierungs-Protokoll. Abgezogen wird NUR, was (a) deklariert,
#        (b) im Protokoll AKTIV gemeldet und (c) wirklich 'nur hier' ist -- alles
#        andere ist ein benannter Befund (fehlender Block, inaktiver Block,
#        stale Deklaration), kein stiller Abzug.
#   * KLASSEN-Differenz: bauen beide Jobs auf verschiedenen Hardware-Klassen
#     (avx512f/avx2/basis), registriert der reichere Baum mehr ISA-gebundene
#     Tests -- voellig zu Recht. Die erwartete Groesse dieser Differenz steht in
#     der committeten Klassenleiter (ci_test_inventory_floor.txt, NUR GELESEN).
#     -> COMDARE_FREMD_HOST_PROBE: die Cache-Zeilen des fremden Baums (test:unit
#        publiziert sie neben der Inventur). PFLICHT, sobald die Inventur-Achse
#        faehrt: ohne Klasse der Gegenseite ist eine ISA-Differenz von einem
#        Defekt nicht unterscheidbar. Fehlt/inkonsistent = benannter Befund.
# VERGLEICHSREGELN (fail-closed, kein Skip-Zweig):
#   Klassen GLEICH  -> nur_hier == SOLL-DELTA exakt UND nur_fremd leer.
#   eigen REICHER   -> nur_fremd leer UND |nur_hier ohne SOLL-DELTA| ==
#                      floor(eigen) - floor(fremd); Restnamen ausgewiesen.
#   eigen AERMER    -> spiegelbildlich: nur_hier ohne SOLL-DELTA leer UND
#                      |nur_fremd| == floor(fremd) - floor(eigen); Namen ausgewiesen.
# Jede Abweichung ist ROT (Exit 4). Die Bilanzzeile nennt IMMER beide Klassen,
# beide Hosts und beide Zahlen -- ein Freispruch ohne Massstab ist keiner.
# -----------------------------------------------------------------------------
CE_FREMD_STATUS="NICHT GEFAHREN"
CE_FREMD_ROT=0
_ce_fremd=${COMDARE_FREMD_INVENTUR-}
_ce_fremd_probe=${COMDARE_FREMD_HOST_PROBE-}
_ce_fremd_delta=${COMDARE_FREMD_SOLL_DELTA_BLOECKE-}
CE_FREMD_KLASSE=""
CE_FREMD_HOST=""
: > "${_ce_tmp}/fremd_befunde.txt"
: > "${_ce_tmp}/solldelta.txt"
: > "${_ce_tmp}/solldelta_abgezogen.txt"
: > "${_ce_tmp}/nur_hier_rest.txt"
: > "${_ce_tmp}/nur_fremd.txt"

# Ein benannter Befund der dritten Achse: sofort sichtbar UND fuer den
# Befund-(e)-Block unten gesammelt -- eine Meldung, zwei Leser.
ce_fremd_befund() {
    echo "  ! $1"
    echo "$1" >> "${_ce_tmp}/fremd_befunde.txt"
    CE_FREMD_ROT=1
}

echo ""
if [ -z "$_ce_fremd" ]; then
    # HALBER ANSCHLUSS IST VERDRAHTUNG, KEIN BETRIEBSZUSTAND: Delta oder Probe
    # ohne die Inventur-Achse liefe als stilles Nichts durch -- wer deklariert,
    # meint die Achse, und eine ungefahrene Achse darf seine Deklaration nicht
    # kommentarlos schlucken.
    if [ -n "$_ce_fremd_probe" ] || [ -n "$_ce_fremd_delta" ]; then
        _ce_m="COMDARE_FREMD_HOST_PROBE bzw. COMDARE_FREMD_SOLL_DELTA_BLOECKE sind gesetzt,"
        _ce_m="${_ce_m} aber COMDARE_FREMD_INVENTUR ist es nicht. Ein halber Anschluss ist ein"
        _ce_m="${_ce_m} Verdrahtungsfehler, kein 'dann eben ohne Achse' (fail-closed)."
        ce_abbruch "$_ce_m" 2
    fi
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

        # -- (i) HOST-PROBE: PFLICHT, sobald diese Achse faehrt --------------------
        if [ -z "$_ce_fremd_probe" ]; then
            _ce_m="HOST-PROBE FEHLT: COMDARE_FREMD_HOST_PROBE ist ungesetzt. PFLICHT, sobald"
            _ce_m="${_ce_m} COMDARE_FREMD_INVENTUR gesetzt ist -- ohne die Klasse der Gegenseite"
            _ce_m="${_ce_m} ist eine ISA-Differenz von einem Defekt nicht unterscheidbar."
            ce_fremd_befund "$_ce_m"
        elif [ ! -f "$_ce_fremd_probe" ]; then
            _ce_m="HOST-PROBE FEHLT: angekuendigt ('${_ce_fremd_probe}'), aber nicht da."
            _ce_m="${_ce_m} Angekuendigt und fehlend wird NICHT uebersprungen (fail-closed)."
            ce_fremd_befund "$_ce_m"
        else
            if ce_host_klasse_lesen "$_ce_fremd_probe" fremd; then
                CE_FREMD_KLASSE=$CE_HK_KLASSE
                _ce_fh_da=$(sed -n '/^HOST=/p' "$_ce_fremd_probe" | wc -l | tr -d ' ')
                if [ "$_ce_fh_da" -ne 1 ]; then
                    _ce_m="HOST-PROBE INKONSISTENT: '${_ce_fremd_probe}' hat ${_ce_fh_da}"
                    _ce_m="${_ce_m} 'HOST='-Zeilen, erwartet GENAU EINE (der Erzeuger schreibt"
                    _ce_m="${_ce_m} sie hinter die Cache-Zeilen)."
                    ce_fremd_befund "$_ce_m"
                else
                    CE_FREMD_HOST=$(sed -n 's/^HOST=//p' "$_ce_fremd_probe" | sed -n '1p')
                    _ce_m="  HOST-PROBE (fremd, ${_ce_fremd_probe}):"
                    echo "${_ce_m} Klasse ${CE_FREMD_KLASSE}, Host ${CE_FREMD_HOST}"
                fi
            else
                _ce_m="HOST-PROBE UNBRAUCHBAR: '${_ce_fremd_probe}' -- Grund unmittelbar"
                _ce_m="${_ce_m} darueber. Eine Klasse, die nicht gemessen vorliegt, wird nicht geraten."
                ce_fremd_befund "$_ce_m"
            fi
        fi

        # -- (ii) SOLL-DELTA: nur deklariert UND AKTIV UND 'nur hier' zaehlt -------
        if [ -n "$_ce_fremd_delta" ]; then
            echo "  SOLL-DELTA (COMDARE_FREMD_SOLL_DELTA_BLOECKE):${_ce_fremd_delta:+ }${_ce_fremd_delta}"
            for _ce_db in $_ce_fremd_delta; do
                _ce_db_zeile=$(awk -F'|' -v k="$_ce_db" '$1==k {print; exit}' "${_ce_tmp}/bloecke.txt")
                if [ -z "$_ce_db_zeile" ]; then
                    _ce_m="SOLL-DELTA-BLOCK FEHLT: '${_ce_db}' steht nicht im Registrierungs-"
                    _ce_m="${_ce_m}Protokoll dieses Baums -- die Deklaration zeigt ins Leere."
                    ce_fremd_befund "$_ce_m"
                    continue
                fi
                _ce_db_status=$(printf '%s\n' "$_ce_db_zeile" | awk -F'|' '{print $2}')
                if [ "$_ce_db_status" != "AKTIV" ]; then
                    _ce_m="SOLL-DELTA-BLOCK NICHT AKTIV: '${_ce_db}' meldet '${_ce_db_status}'"
                    _ce_m="${_ce_m} -- ein Delta aus einem Block, der gar nicht lief, ist keins."
                    ce_fremd_befund "$_ce_m"
                    continue
                fi
                printf '%s\n' "$_ce_db_zeile" | awk -F'|' '{print $3}' | tr ',' '\n' \
                    | sed '/^$/d;/^-$/d' >> "${_ce_tmp}/solldelta_roh.txt"
            done
            if [ -s "${_ce_tmp}/solldelta_roh.txt" ]; then
                LC_ALL=C sort -u "${_ce_tmp}/solldelta_roh.txt" > "${_ce_tmp}/solldelta.txt"
            fi
            while IFS= read -r _ce_dn; do
                [ -n "$_ce_dn" ] || continue
                if awk -v n="$_ce_dn" 'BEGIN{f=1} $0==n {f=0} END{exit f}' "${_ce_tmp}/nur_hier.txt"; then
                    echo "$_ce_dn" >> "${_ce_tmp}/solldelta_abgezogen.txt"
                else
                    _ce_m="STALE DEKLARATION: '${_ce_dn}' (SOLL-DELTA) ist NICHT 'nur hier'"
                    _ce_m="${_ce_m} -- der Name steht auch im fremden Baum oder fehlt in diesem."
                    _ce_m="${_ce_m} Eine Deklaration, die nichts mehr abzieht, gehoert entfernt,"
                    _ce_m="${_ce_m} nicht mitgeschleppt."
                    ce_fremd_befund "$_ce_m"
                fi
            done < "${_ce_tmp}/solldelta.txt"
            # ABGEZOGENE NAMEN IMMER AUSWEISEN -- ein stiller Abzug waere ein
            # zweiter Blindfleck an genau der Stelle, die den ersten heilt.
            if [ -s "${_ce_tmp}/solldelta_abgezogen.txt" ]; then
                echo "  SOLL-DELTA abgezogen (deklariert, AKTIV und nur hier):"
                while IFS= read -r _ce_dn; do
                    echo "    ~ ${_ce_dn}"
                done < "${_ce_tmp}/solldelta_abgezogen.txt"
            fi
        fi
        LC_ALL=C comm -23 "${_ce_tmp}/nur_hier.txt" "${_ce_tmp}/solldelta_abgezogen.txt" \
            > "${_ce_tmp}/nur_hier_rest.txt"
        _ce_nhr=$(wc -l < "${_ce_tmp}/nur_hier_rest.txt" | tr -d ' ')

        # -- BILANZZEILE: IMMER beide Klassen, beide Hosts, beide Zahlen -----------
        # (EINE Ausgabezeile; nur der Quelltext ist umbrochen.)
        _ce_m="  BILANZ: eigen=${CE_GESAMT} (Klasse ${CE_HOST_KLASSE}, Host ${_ce_hostname})"
        _ce_m="${_ce_m} gegen fremd=${CE_FREMD_N} (Klasse ${CE_FREMD_KLASSE:-unbestimmt},"
        _ce_m="${_ce_m} Host ${CE_FREMD_HOST:-unbestimmt})"
        echo "$_ce_m"

        # -- (iii) VERGLEICHSREGELN -- nur ueber einem sauberen Fundament ----------
        if [ "$CE_FREMD_ROT" -ne 0 ]; then
            echo "  -> KEIN URTEIL nach den Vergleichsregeln: erst die benannten Befunde"
            echo "     oben beheben -- ueber kaputter Probe/Deklaration waere jede Regel"
            echo "     eine Rechnung ohne Gegenstand."
            CE_FREMD_STATUS="ROT (Probe/SOLL-DELTA, benannte Befunde)"
        else
            _ce_er=$(ce_klasse_rang "$CE_HOST_KLASSE")
            _ce_fr=$(ce_klasse_rang "$CE_FREMD_KLASSE")
            if [ "$_ce_er" -eq "$_ce_fr" ]; then
                if [ "$_ce_nhr" -eq 0 ] && [ "$_ce_nf" -eq 0 ]; then
                    if [ -s "${_ce_tmp}/solldelta_abgezogen.txt" ]; then
                        _ce_abz=$(wc -l < "${_ce_tmp}/solldelta_abgezogen.txt" | tr -d ' ')
                        _ce_m="  -> deckungsgleich nach SOLL-DELTA: ${CE_GESAMT} - ${_ce_abz}"
                        echo "${_ce_m} == ${CE_FREMD_N} (Klassen gleich: ${CE_HOST_KLASSE})."
                        CE_FREMD_STATUS="GRUEN (${CE_GESAMT} - ${_ce_abz} == ${CE_FREMD_N}, klassengleich)"
                    else
                        echo "  -> deckungsgleich (${CE_GESAMT} == ${CE_FREMD_N}), Namen identisch."
                        CE_FREMD_STATUS="GRUEN (${CE_GESAMT} == ${CE_FREMD_N})"
                    fi
                else
                    _ce_m="  -> ABWEICHUNG (Klassen gleich: ${CE_HOST_KLASSE}): ${_ce_nhr} nur"
                    echo "${_ce_m} hier (nach SOLL-DELTA), ${_ce_nf} nur im fremden Baum -- erwartet 0 und 0."
                    CE_FREMD_STATUS="ROT (klassengleich, ${_ce_nhr} nur hier / ${_ce_nf} nur fremd)"
                    CE_FREMD_ROT=1
                fi
            else
                if [ "$_ce_er" -gt "$_ce_fr" ]; then
                    _ce_reich=$CE_HOST_KLASSE; _ce_arm=$CE_FREMD_KLASSE
                    _ce_soll_diff=$(( $(ce_klasse_floor "$_ce_reich") - $(ce_klasse_floor "$_ce_arm") ))
                    _ce_ist_diff=$_ce_nhr
                    _ce_gegen_leer=$_ce_nf
                    _ce_seite="nur hier (nach SOLL-DELTA)"
                    _ce_gegen="nur fremd"
                    _ce_rest_datei="${_ce_tmp}/nur_hier_rest.txt"
                    _ce_rest_marke="+"
                else
                    _ce_reich=$CE_FREMD_KLASSE; _ce_arm=$CE_HOST_KLASSE
                    _ce_soll_diff=$(( $(ce_klasse_floor "$_ce_reich") - $(ce_klasse_floor "$_ce_arm") ))
                    _ce_ist_diff=$_ce_nf
                    _ce_gegen_leer=$_ce_nhr
                    _ce_seite="nur fremd"
                    _ce_gegen="nur hier (nach SOLL-DELTA)"
                    _ce_rest_datei="${_ce_tmp}/nur_fremd.txt"
                    _ce_rest_marke="-"
                fi
                _ce_m="  Leiter-Ausweis: floor(${_ce_reich})=$(ce_klasse_floor "$_ce_reich")"
                _ce_m="${_ce_m} - floor(${_ce_arm})=$(ce_klasse_floor "$_ce_arm")"
                echo "${_ce_m} = ${_ce_soll_diff} erwartete klassengebundene Differenz."
                if [ "$_ce_gegen_leer" -eq 0 ] && [ "$_ce_ist_diff" -eq "$_ce_soll_diff" ]; then
                    _ce_m="  -> KLASSEN-DIFFERENZ ERKLAERT: genau ${_ce_ist_diff} Name(n)"
                    echo "${_ce_m} ${_ce_seite}, 0 ${_ce_gegen}:"
                    while IFS= read -r _ce_t; do
                        echo "    ${_ce_rest_marke} ${_ce_t}"
                    done < "$_ce_rest_datei"
                    _ce_m="GRUEN (Klassen-Differenz ${_ce_soll_diff} erklaert,"
                    CE_FREMD_STATUS="${_ce_m} ${CE_HOST_KLASSE} gegen ${CE_FREMD_KLASSE})"
                else
                    _ce_m="  -> ABWEICHUNG (Klassen ${CE_HOST_KLASSE} gegen ${CE_FREMD_KLASSE}):"
                    _ce_m="${_ce_m} erwartet ${_ce_soll_diff} Name(n) ${_ce_seite} und 0 ${_ce_gegen};"
                    echo "${_ce_m} gefunden ${_ce_ist_diff} ${_ce_seite}, ${_ce_gegen_leer} ${_ce_gegen}."
                    _ce_m="ROT (Klassen-Differenz ${_ce_soll_diff} erwartet,"
                    CE_FREMD_STATUS="${_ce_m} ${_ce_ist_diff}/${_ce_gegen_leer} gefunden)"
                    CE_FREMD_ROT=1
                fi
            fi
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

# STRENG-MODUS: hier wird aus der Sichtbarkeit eine Haerte.
# ERST NACH DER SCHLEIFE, nicht darin: ein Abbruch beim ersten fehlenden Gate
# naennte genau eines und verschwiege die uebrigen. Wer eine CI-Verdrahtung
# richtigstellt, will alle fehlenden Variablen in EINEM Lauf sehen, nicht eine
# pro Lauf.
if [ "$CE_STRIKT" = 1 ] && [ "$CE_GATES_UNGEPRUEFT" -gt 0 ]; then
    echo ""
    echo "-----------------------------------------------------------------------------"
    echo "FEHLER: STRENG-MODUS UND UNGEPRUEFTE GATES -- ${CE_GATES_UNGEPRUEFT} von ${CE_GATES_GESAMT}."
    echo "COMDARE_WACHE_STRIKT=1 heisst: hier wird VOLLSTAENDIGKEIT erwartet. Ein Gate,"
    echo "dessen Deklarationsvariable niemand gesetzt hat, wird dann NICHT mehr als"
    echo "deklariert angenommen -- seine Deckung waere eine Annahme, und eine Annahme"
    echo "ist im strengen Lauf keine Deckung. Betroffen:"
    echo ""
    while read -r _ce_ug; do
        echo "    OHNE AUSKUNFT: ${_ce_ug}"
    done < "${_ce_tmp}/ungepruefte_gates.txt"
    echo ""
    echo "SO WIRD DAS BEHOBEN: die genannte Variable im variables:-Block des Jobs setzen"
    echo "(oder auf dem Dual-Weg von Hand mitgeben). Das ist ein VERDRAHTUNGS-Fehler und"
    echo "deshalb Exit 2 -- kein Abdeckungs-Befund (1) und kein Nenner-Befund (4): die"
    echo "Testmenge ist in Ordnung, die Auskunft ueber die Jobs fehlt."
    echo "NICHT so beheben: COMDARE_WACHE_STRIKT wegnehmen. Dann meldet derselbe Lauf"
    echo "gruen, und der Unterschied zwischen 'geprueft' und 'angenommen' ist wieder weg."
    echo "-----------------------------------------------------------------------------"
    ce_abbruch "Streng-Modus: ${CE_GATES_UNGEPRUEFT} Gate(n) ohne gesetzte Deklarationsvariable." 2
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
    if [ -s "${_ce_tmp}/fremd_befunde.txt" ]; then
        echo ""
        echo "  BENANNTE BEFUNDE der dritten Achse (Wortlaut wie oben):"
        while IFS= read -r _ce_t; do echo "    ! ${_ce_t}"; done < "${_ce_tmp}/fremd_befunde.txt"
    fi
    if [ -s "${_ce_tmp}/solldelta_abgezogen.txt" ]; then
        echo ""
        echo "  SOLL-DELTA abgezogen (deklariert, AKTIV und nur hier -- auch im Fehlerfall ausgewiesen):"
        while IFS= read -r _ce_t; do echo "    ~ ${_ce_t}"; done < "${_ce_tmp}/solldelta_abgezogen.txt"
    fi
    if [ -s "${_ce_tmp}/nur_hier_rest.txt" ]; then
        echo ""
        echo "  NUR IN DIESEM Baum registriert (nach SOLL-DELTA-Abzug; im fremden fehlend):"
        while read -r _ce_t; do echo "    + ${_ce_t}"; done < "${_ce_tmp}/nur_hier_rest.txt"
    fi
    if [ -s "${_ce_tmp}/nur_fremd.txt" ]; then
        echo ""
        echo "  NUR IM FREMDEN Baum registriert (hier fehlend):"
        while read -r _ce_t; do echo "    - ${_ce_t}"; done < "${_ce_tmp}/nur_fremd.txt"
    fi
    echo ""
    echo "SO WIRD DAS BEHOBEN (nicht durch Abschalten der Achse):"
    echo "  * Ist die Differenz GEWOLLT (ein Registrierungs-Block, der nur in DIESEM"
    echo "    Baum laedt), gehoert sie DEKLARIERT: COMDARE_FREMD_SOLL_DELTA_BLOECKE am"
    echo "    Aufruf um die Block-Kennung ergaenzen bzw. eine stale Deklaration"
    echo "    entfernen -- die Namen kommen aus dem Registrierungs-Protokoll, nie von Hand."
    echo "  * Die HOST-PROBE pruefen (COMDARE_FREMD_HOST_PROBE): weichen die KLASSEN ab,"
    echo "    erklaert die Klassenleiter die erwartete Differenz -- nicht jede Differenz"
    echo "    ist ein Defekt, aber jede unerklaerte ist einer."
    echo "  * Bei ISA-ZUWACHS (neue klassengebundene Tests) die Klassenleiter"
    echo "    scripts/ci_test_inventory_floor.txt im SELBEN Change nachziehen --"
    echo "    alle drei Klassen im selben Zug (dieselbe Doktrin wie beim"
    echo "    Untergrenzen-Ausweis oben)."
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
    echo "  Untergrenze (committet)             : ${CE_GESAMT} == ${CE_FLOOR} fuer Klasse ${CE_HOST_KLASSE}"
    echo "                                        (Host: ${_ce_hostname}, Quelle: ${_ce_floor_pfad},"
    echo "                                         Klasse gemessen aus ${_ce_cache})"
    echo "  Nenner gegen FREMDE Quelle (e)      : ${CE_FREMD_STATUS}"
    _ce_gates_ok=$(( CE_GATES_GESAMT - CE_GATES_UNGEPRUEFT ))
    echo "  Gates mit gesetzter Variable        : ${_ce_gates_ok}/${CE_GATES_GESAMT}, ${CE_GATES_UNGEPRUEFT} ANNAHME"
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
