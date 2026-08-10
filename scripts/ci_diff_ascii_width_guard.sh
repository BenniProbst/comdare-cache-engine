# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- DIFF-HYGIENE-WACHE: ASCII + Spaltenbreite (2026-08-06)
# =============================================================================
# BEHAUPTUNG, DIE HIER GEPRUEFT WIRD (die Invariante):
#
#     Jede HINZUGEFUEGTE Zeile in selbst verfassten Code-Dateien ist 7-Bit-ASCII
#     (Ausnahme: das Paragraf-Zeichen SS, per Doktrin erlaubt) und nicht laenger
#     als 120 Byte (ColumnLimit aus .clang-format).
#
# BEFUND, DER DIESE WACHE ERZWINGT (2026-08-06, Bestandsaufnahme derselben
# Session): das bisherige Verfahren war KEIN Skript, sondern eine Prozedur, die
# jeder Agent von Hand vor jeder Paketmeldung eintippte:
#
#     git diff -U0 -- "$f" | grep '^+' | grep -v '^\+\+\+' | grep -P '[^\x00-\x7F]'
#
# Diese Zeile ist auf JEDER grep-Engine kaputt, ueber zwei verschiedene Wege:
#   * Unter ugrep (hier: die grep-Emulation, die Claude Codes Bash-Tool fuer
#     das blosse Wort 'grep' unterschiebt) bricht die mittlere Stufe
#     `grep -v '^\+\+\+'` mit einem Syntaxfehler ab (Exit 2). Ohne pipefail
#     bestimmt die LETZTE Stufe den Gesamt-Exit -- die sieht leere Eingabe und
#     meldet "kein Treffer" (Exit 1), ununterscheidbar von einem sauberen
#     Befund.
#   * Unter ECHTEM GNU grep 3.11 (verifiziert auf diesem Host per `command
#     grep`/absolutem Pfad) gibt es KEINEN Fehler -- aber ein ANDERES, stilleres
#     Problem: `\+` ist in GNU-BRE die Erweiterung "ein-oder-mehr", nicht ein
#     literales Plus. `^\+\+\+` matcht deshalb JEDE Zeile, die mit einem-oder-
#     mehreren '+' beginnt -- die Stufe wirft ALLE hinzugefuegten Diff-Zeilen
#     weg, nicht nur die '+++ b/datei'-Kopfzeile. Beleg (Eingabe
#     x/+/++/+++/+abc/abc): `grep -v '^\+\+\+'` behaelt nur x und abc, wirft
#     +abc (eine ganz normale Zeile) faelschlich mit raus.
#   * Selbst pipefail traegt hier NICHT: die rechte Pipe-Stufe endet bei leerer
#     Eingabe selbst harmlos mit Exit 1, und genau dieser Code gewinnt, egal ob
#     eine fruehere Stufe abgestuerzt ist.
#
# Kein CI-Job hat diese Zeile je automatisiert ausgefuehrt (nachgeprueft: 0
# Treffer fuer grep-P-Ketten in beiden Repos' .gitlab-ci.yml/scripts/cmake) --
# der Schaden war auf die manuelle Prozedur begrenzt. Diese Wache ersetzt die
# Prozedur durch ein Werkzeug: versioniert, ausfuehrbar, mit Biss-Beweis.
#
# WARUM DIESE WACHE DIE FALLE STRUKTURELL NICHT WIEDERHOLEN KANN:
#   * Sie verwendet KEIN grep und KEIN sed in der Kern-Logik. Die gesamte
#     Diff-Verarbeitung laeuft in EINEM awk-Programm mit einem STRUKTURELLEN
#     Zustand (Datei-Kopf vor dem ersten '@@' vs. Hunk-Inhalt danach) statt
#     einer Regex-Vermutung ueber die '+++'-Kopfzeile. Eine hinzugefuegte
#     Zeile, deren TEXT zufaellig mit '+++' beginnt, wird dadurch korrekt als
#     Inhalt erkannt (das genaue Gegenteil des GNU-grep-Befunds oben) -- die
#     Wache ist damit unabhaengig von JEDER grep-Variante und ihrer jeweiligen
#     Eskapierungsregel.
#   * Die Nicht-ASCII-Erkennung selbst ist byteweise (LC_ALL=C, 128..255 als
#     vorberechnete Menge), nicht ueber eine Regex-Zeichenklasse.
#   * Statt PIPESTATUS/`set -o pipefail` (beides keine POSIX-sh-Mittel, dash
#     als /bin/sh auf diesem Host kennt keins von beiden) schreibt jede Stufe
#     in eine Datei und wird EINZELN per $? geprueft -- das ist die portable
#     Form derselben Absicht und vermeidet genau die Masse, die pipefail hier
#     ohnehin nicht aufgeloest haette (s. Befund oben).
#   * Fehlt ein Werkzeug (awk, git, mktemp) oder liefert `git diff` selbst
#     einen Fehler-Exit, ist das FATAL (Exit 2) -- nie eine stille Null.
#
# GRENZE, EHRLICH BENANNT (was diese Wache NICHT faengt):
#   * Sie prueft nur den DIFF-BEREICH, nicht den Gesamtbestand -- Alt-Zeilen,
#     die nur durch reines Whitespace-Reformatieren zu "+"-Zeilen werden,
#     zaehlt sie trotzdem (kein `git diff -w`); das ist Aufrufer-Sache.
#   * Sie prueft nur SELBST VERFASSTEN Code (Erweiterungs-Liste unten:
#     .cpp/.hpp/.h/.hh/.cc/.cxx/.tpp/.ipp/.inl/.cmake, CMakeLists.txt). Deutsche
#     Doku-Prosa (*.md) traegt per Sprachdoktrin korrekte Umlaute und ist
#     ABSICHTLICH ausgenommen -- das ist kein blinder Fleck, sondern Scope;
#     ausgenommene Zeilen werden trotzdem GEZAEHLT und ihre Dateien NAMENTLICH
#     gemeldet, damit "ausserhalb des Scopes" nicht zu einer zweiten stillen
#     Null wird. Rohe Build-/Test-Logs mit Unicode-Trennlinien (CMake/make)
#     stehen nie in einem Diff dieser Art (sie sind keine versionierten
#     Quelldateien) und sind damit strukturell aussen vor, nicht per Ausnahme.
#   * Spaltenbreite wird BYTEWEISE gemessen (LC_ALL=C `length()`), nicht als
#     Unicode-Codepoint-Breite -- fuer den ASCII+SS-Regelbereich dieser Wache
#     ist das identisch mit der Zeichenzahl; nur eine bereits ASCII-Verstoss-
#     Zeile koennte dadurch geringfuegig zu lang gezaehlt werden (konservativ,
#     nie zu kurz).
#   * Sie kann bei stdin-Betrieb (--stdin) NICHT erkennen, ob eine leere
#     Eingabe "keine Aenderungen" oder ein abgebrochener vorgelagerter Schritt
#     bedeutet -- das muss der Aufrufer sicherstellen (PIPESTATUS/pipefail auf
#     SEINER Seite). Im Default-Modus (ohne --stdin) fuehrt DIESES Skript
#     `git diff` selbst aus und prueft dessen Exit-Code selbst -- das ist der
#     empfohlene, sichere Aufrufweg.
#   * Die Aufzaehlung der untracked Dateien im Default-Modus laeuft ueber
#     `git ls-files --others --exclude-standard`, zeilenweise gelesen. Ein
#     Dateiname mit einem ZEILENUMBRUCH darin wuerde dabei zerfallen; solche
#     Namen gibt es im Baum nicht (C++-Quellen), und `core.quotePath=false`
#     haelt Umlaut-Namen zusammen. Ignorierte Dateien (.gitignore) bleiben
#     ausgenommen -- Bauartefakte sind kein selbst verfasster Code.
#   * Der Default-Modus misst den ARBEITSSTAND, nicht die Commit-Historie: was
#     bereits committet ist, sieht er nicht mehr. Fuer "alles, was ein Push
#     uebertragen wuerde" ist scripts/vor_push_alle_wachen.sh der richtige
#     Aufrufer -- der bestimmt den Bereich selbst und bricht bei schmutzigem
#     Arbeitsstand ab. Die beiden ergaenzen sich; keiner ersetzt den anderen.
#
# AUFRUF:
#   sh scripts/ci_diff_ascii_width_guard.sh [<git-diff-args...>]
#       Ohne Argumente (DEFAULT-MODUS): der GESAMTE unfertige Arbeitsstand gegen
#       HEAD -- `git diff -U0 HEAD` (erfasst gestagte UND ungestagte Aenderungen
#       an getrackten Dateien) PLUS jede untracked, nicht ignorierte Datei
#       einzeln ueber `git diff --no-index /dev/null <datei>`. Ein leerer Nenner
#       ist in diesem Modus ein ABBRUCH (Exit 2), kein GRUEN.
#       Mit Argumenten: an `git diff -U0 --no-color --no-ext-diff` durchgereicht,
#       z.B. ein Revisionsbereich fuer CI (`origin/main...HEAD`) oder eine
#       Pfad-Einschraenkung nach `--`. Dieser Weg ist UNVERAENDERT.
#
#   sh scripts/ci_diff_ascii_width_guard.sh --bereich <basis> [<spitze>]
#       KUMULATIVER MODUS (2026-08-10). Prueft den GANZEN Stand, der von <spitze>
#       nach <basis> gehen wuerde -- nicht einen Push. <spitze> ist ohne Angabe
#       HEAD. Typischer Aufruf vor einem main-Fast-Forward:
#           sh scripts/ci_diff_ascii_width_guard.sh --bereich origin/main origin/development
#
# WARUM DIESER MODUS AM 10.08.2026 DAZUKAM (Befund am Objekt, Posten #48):
# Diese Wache konnte einen Bereich schon immer messen -- aber nur, wenn ihr jemand
# einen mitgab. Die CI gibt ihr CI_COMMIT_BEFORE_SHA..HEAD, also den PUSH. Damit
# beantwortet sie "ist DIESER Push sauber?". Die Frage, die vor einem main-FF
# zaehlt, ist eine andere: "ist der STAND sauber, der nach main geht?".
#
# Der Unterschied ist nicht theoretisch. Gemessen am 09.08.2026, DERSELBE Baum-SHA
# aebc4f2c in beiden Laeufen:
#     ce-Pipeline 15498  development  Bereich 2eb310ae..HEAD    1 Commit    GRUEN
#     ce-Pipeline 15501  main         Bereich 8fcf0c0e..HEAD   86 Commits   ROT
# 58 Verstoesse (37 Nicht-ASCII, 21 ueber 120 Spalten) in zwoelf Dateien, alle aus
# Commits desselben Tages, jeder einzelne Push davon gruen. Nachgerechnet am
# 10.08.2026 auf diesem Repo: `... 8fcf0c0e..aebc4f2c` -> Exit 1, 37 + 21 = 58.
# Solange in kleinen Schritten gepusht wird, faellt nichts auf; der main-FF ist das
# ERSTE kumulative Gate und traegt den ganzen aufgelaufenen Bereich auf einmal.
#
# WAS DIESER MODUS ZUSAETZLICH TUT (und warum es nicht dasselbe ist wie ein Bereich
# von Hand):
#   1. ER MISST AB DER ABZWEIGUNG, nicht ab dem Endpunkt. `git diff basis..spitze`
#      (zwei Punkte) vergleicht Endpunkt gegen Endpunkt: hat die BASIS eigene
#      Commits, erscheinen deren VORGAENGER-Zeilen als "hinzugefuegt" und die Wache
#      schlaegt in Code an, den auf der Spitze niemand angefasst hat. Am Objekt
#      gemessen (10.08.2026, Wegwerf-Repo): eine ueberlange Zeile aus dem
#      Altbestand, die die Basis geheilt hat und die Spitze nie beruehrte --
#      zwei Punkte: Exit 1, "q.cpp:1 (227 Byte)"; drei Punkte (merge-base):
#      Exit 0. Dieser Modus loest die merge-base AUSDRUECKLICH auf und uebergibt
#      sie git als SHA; der gedruckte Bereich ist damit derselbe Gegenstand, der
#      gemessen wurde, und keine Abkuerzung, die morgen etwas anderes bedeutet.
#   2. DER BEREICH STEHT IN DER AUSGABE -- beide Endpunkte als voller SHA, die
#      Abzweigung, die Commit-Zahl im Bereich und die Divergenz auf der Basis.
#      Ein Verdikt ohne seinen Bereich ist keine Aussage: dieselbe Wache auf
#      demselben Baum sagt GRUEN oder ROT, je nachdem, was man ihr gibt.
#   3. FAIL-CLOSED an beiden Enden: ein nicht aufloesbarer Ref (nicht geholt,
#      flacher Klon, Tippfehler) ist ABBRUCH, keine stille Null. Und ein Bereich
#      mit NULL Commits ist ebenfalls ABBRUCH -- dort waere nichts geprueft
#      worden, und ein nicht gelaufener Test ist kein bestandener. Das ist genau
#      die Asymmetrie, die der Default-Modus unten schon traegt.
#
# WARUM DER DEFAULT-MODUS AM 09.08.2026 REPARIERT WURDE (Befund am Objekt):
# Hier stand bis heute "git diff -U0 (Arbeitsverzeichnis gegen HEAD)". Das war
# FALSCH, und die Wache tat es auch nicht: `git diff` OHNE Revision vergleicht
# den Arbeitsbaum gegen den INDEX. Wer seine Arbeit vor der Pruefung `git add`-et,
# bekam "0 Zusatzzeilen ... GRUEN" -- und untracked NEUE Dateien sieht `git diff`
# in gar keinem Modus. Zwei blinde Flecken, beide still, beide genau in der Lage,
# in der ein Agent kurz vor der Paketmeldung steht.
#
# Der Biss-Beweis (ci_diff_ascii_width_guard.bissbeweis.txt) konnte das nicht
# auffangen: alle sieben dort protokollierten Faelle laufen ueber `--stdin` oder
# einen ausdruecklichen Bereich. KEINER fuhr die Wache ohne Argumente.
#
# Aufgeschlagen ist es im Paket D5-1 (Commit c98b4b95): gemeldet "0 nicht-ASCII in
# 189 hinzugefuegten Zeilen", tatsaechlich 570 hinzugefuegte C++-Zeilen mit 7
# nicht-ASCII und 6 Zeilen ueber 120 Spalten. Die neue Testdatei war gestagt UND
# neu -- beide blinden Flecken auf einmal. Die Differenz 570-189 IST der blinde
# Fleck, in Zeilen gemessen. Ausfuehrbarer Nachweis beider Flecken und ihrer
# Reparatur: scripts/ci_diff_ascii_width_guard.selbsttest.sh (Faelle 2, 3, 6, 7).
#   sh scripts/ci_diff_ascii_width_guard.sh --stdin < fertiger-diff.txt
#       Liest einen bereits erzeugten Unified-Diff von stdin (Tests, oder wenn
#       ein Aufrufer den Diff bereits selbst erzeugt und PIPESTATUS-geprueft
#       hat).
#
# EXIT:  0 = sauber (Zusammenfassung mit Nenner wird immer gedruckt)
#        1 = mindestens ein Verstoss (Nicht-ASCII und/oder >120 Spalten)
#        2 = Bedienung/Umgebung/undeutbares Hunk-Format -- NIE eine stille 0
#
# KEIN Python (Buildchain-Kanon), POSIX sh + awk, ASCII-only.
# =============================================================================

set -u
export LC_ALL=C

ce_abbruch() {
    echo ""
    echo "DIFF-HYGIENE-WACHE: ABBRUCH -- $1" >&2
    exit 2
}

command -v awk >/dev/null 2>&1 || ce_abbruch "awk ist nicht im PATH."
command -v mktemp >/dev/null 2>&1 || ce_abbruch "mktemp ist nicht im PATH."

_ce_stdin_modus=0
_ce_bereich_modus=0
_ce_bereich_basis=""
_ce_bereich_spitze=""
_ce_bereich_satz=""
if [ "${1:-}" = "--stdin" ]; then
    _ce_stdin_modus=1
    shift
elif [ "${1:-}" = "--bereich" ]; then
    # KUMULATIVER MODUS. Die Argumente werden hier NUR eingesammelt; aufgeloest
    # wird erst unten, wo die Repo-Wurzel feststeht -- eine Ref-Aufloesung ohne
    # bekanntes Repo waere eine Aussage ueber das falsche Verzeichnis.
    _ce_bereich_modus=1
    shift
    [ "$#" -ge 1 ] \
        || ce_abbruch "--bereich braucht eine BASIS (z.B. 'origin/main'), optional eine SPITZE \
(Default HEAD). Aufruf: --bereich <basis> [<spitze>]"
    _ce_bereich_basis="$1"
    shift
    if [ "$#" -ge 1 ]; then
        _ce_bereich_spitze="$1"
        shift
    else
        _ce_bereich_spitze="HEAD"
    fi
    [ "$#" -eq 0 ] \
        || ce_abbruch "--bereich nimmt hoechstens zwei Argumente (basis, spitze) -- \
uebrig geblieben: $*. Pfad-Einschraenkungen gehoeren in den Durchreich-Modus."
fi

_ce_diff_datei="$(mktemp "${TMPDIR:-/tmp}/ce_diff_guard.XXXXXX")" \
    || ce_abbruch "mktemp fuer die Diff-Datei fehlgeschlagen."
_ce_awk_out="$(mktemp "${TMPDIR:-/tmp}/ce_diff_guard_out.XXXXXX")" \
    || ce_abbruch "mktemp fuer die Ausgabedatei fehlgeschlagen."
_ce_untracked_liste="$(mktemp "${TMPDIR:-/tmp}/ce_diff_guard_unt.XXXXXX")" \
    || ce_abbruch "mktemp fuer die Untracked-Liste fehlgeschlagen."
trap 'rm -f "$_ce_diff_datei" "$_ce_awk_out" "$_ce_untracked_liste"' EXIT INT TERM HUP

# DEFAULT-MODUS = ohne Argumente und ohne --stdin. Nur dort wird der Arbeitsstand
# selbst zusammengestellt (gegen HEAD + untracked) und nur dort ist ein leerer
# Nenner ein Abbruch. Der Bereichs- und der stdin-Weg bleiben unberuehrt.
_ce_default_modus=0

echo "============================================================================="
echo " DIFF-HYGIENE-WACHE: ASCII + Spaltenbreite  (scripts/ci_diff_ascii_width_guard.sh)"
echo "============================================================================="

if [ "$_ce_stdin_modus" -eq 1 ]; then
    echo ""
    echo "MODUS: --stdin (fertiger Diff wird gelesen; der Aufrufer traegt die"
    echo "Verantwortung fuer den Exit-Code seiner EIGENEN vorgelagerten Stufe)."
    cat > "$_ce_diff_datei"
    _ce_rc=$?
    [ "$_ce_rc" -eq 0 ] || ce_abbruch "Lesen von stdin fehlgeschlagen (rc=${_ce_rc})."
else
    command -v git >/dev/null 2>&1 || ce_abbruch "git ist nicht im PATH."
    _ce_script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || ce_abbruch "Skript-Verzeichnis nicht aufloesbar."
    _ce_repo_root=$(CDPATH= cd -- "$_ce_script_dir/.." && pwd) || ce_abbruch "Repo-Wurzel nicht aufloesbar."

    # -----------------------------------------------------------------------
    # KUMULATIVER MODUS: den Bereich AUFLOESEN, ihn AUSSPRECHEN, dann messen.
    # Aufgeloest wird zu vollen SHAs, und gemessen wird mit genau diesen SHAs --
    # nicht mit den Ref-Namen. Ein Ref bewegt sich; eine Ausgabe, die "origin/main"
    # sagt und einen anderen Commit gemessen hat, ist keine Aussage, sondern eine
    # Fussnote. (V-1: der Pruefbereich ist Teil der Aussage.)
    # -----------------------------------------------------------------------
    if [ "$_ce_bereich_modus" -eq 1 ]; then
        _ce_basis_sha=$(git -C "$_ce_repo_root" rev-parse --verify --quiet "${_ce_bereich_basis}^{commit}") \
            || ce_abbruch "BASIS '${_ce_bereich_basis}' ist in diesem Repo nicht aufloesbar. \
Kein Gruen daraus: erst 'git fetch origin', in der CI zusaetzlich GIT_DEPTH=0 (ein flacher Klon \
kennt weder den fremden Zweig noch die Abzweigung)."
        _ce_spitze_sha=$(git -C "$_ce_repo_root" rev-parse --verify --quiet "${_ce_bereich_spitze}^{commit}") \
            || ce_abbruch "SPITZE '${_ce_bereich_spitze}' ist in diesem Repo nicht aufloesbar \
(s. den Hinweis zur BASIS)."
        _ce_mb=$(git -C "$_ce_repo_root" merge-base "$_ce_basis_sha" "$_ce_spitze_sha" 2>/dev/null) \
            || ce_abbruch "Keine Abzweigung zwischen '${_ce_bereich_basis}' und '${_ce_bereich_spitze}' \
bestimmbar (zusammenhanglose Historien oder abgeschnittener Klon). Fail-closed: das ist ABBRUCH."
        [ -n "$_ce_mb" ] \
            || ce_abbruch "merge-base lieferte eine LEERE Antwort -- undeutbar, also ABBRUCH."
        _ce_n_bereich=$(git -C "$_ce_repo_root" rev-list --count "${_ce_mb}..${_ce_spitze_sha}") \
            || ce_abbruch "Commit-Zahl im Bereich nicht bestimmbar -- ohne Nenner kein Urteil."
        _ce_n_diverg=$(git -C "$_ce_repo_root" rev-list --count "${_ce_mb}..${_ce_basis_sha}") \
            || ce_abbruch "Divergenz auf der Basis nicht bestimmbar -- ohne Nenner kein Urteil."
        _ce_bereich_satz="${_ce_mb}..${_ce_spitze_sha} (${_ce_n_bereich} Commits)"
        echo ""
        echo "MODUS: --bereich (KUMULATIV -- der ganze Stand, der nach '${_ce_bereich_basis}' ginge,"
        echo "       NICHT ein einzelner Push)"
        echo "BEREICH (Teil der Aussage, nicht Beiwerk):"
        echo "  BASIS      ${_ce_bereich_basis} = ${_ce_basis_sha}"
        echo "  SPITZE     ${_ce_bereich_spitze} = ${_ce_spitze_sha}"
        echo "  ABZWEIGUNG (merge-base) = ${_ce_mb}"
        echo "  ${_ce_n_bereich} Commit(s) im Bereich; ${_ce_n_diverg} Commit(s) nur auf der BASIS (Divergenz)."
        if [ "$_ce_n_diverg" -gt 0 ]; then
            echo "  HINWEIS: die BASIS ist divergiert -- ein Fast-Forward ist damit NICHT moeglich."
            echo "  Gemessen wird trotzdem ab der ABZWEIGUNG: das ist die Menge, die die SPITZE"
            echo "  hinzufuegt. Der Endpunkt-Diff wuerde stattdessen fremde, laengst geheilte"
            echo "  Zeilen als 'hinzugefuegt' anrechnen."
        fi
        # NULL COMMITS IST ABBRUCH, NICHT GRUEN. Hier hat der Aufrufer nicht nach
        # einem beliebigen Diff gefragt, sondern nach einem URTEIL ueber einen
        # Stand -- und ueber einen leeren Stand gibt es keines. Ein falsch
        # verdrahteter Aufruf (`--bereich HEAD`, eine leere CI-Variable, zweimal
        # derselbe Ref) endete sonst mit genau der Quittung, gegen die diese
        # Wache gebaut ist: gruen, ohne hingesehen zu haben.
        if [ "$_ce_n_bereich" -eq 0 ]; then
            ce_abbruch "0 Commits zwischen Abzweigung und SPITZE -- es wurde NICHTS geprueft, \
also ist nichts bestanden. Sind BASIS und SPITZE wirklich derselbe Stand, dann gibt es auch \
nichts zu uebertragen und dieser Aufruf ist ueberfluessig, nicht gruen."
        fi
        # Gemessen wird mit den SHAs, die oben gedruckt stehen -- Endpunkt-Form
        # ueber die AUFGELOESTE Abzweigung ist exakt der Drei-Punkt-Bereich, nur
        # nachpruefbar: beide Enden sind sichtbar.
        set -- "$_ce_mb" "$_ce_spitze_sha"
    fi

    # OHNE ARGUMENTE: ausdruecklich gegen HEAD statt gegen den INDEX. `git diff`
    # ohne Revision vergleicht den Arbeitsbaum mit dem INDEX -- gestagte Arbeit
    # waere unsichtbar (s. Kopf, Befund D5-1). `HEAD` erfasst beides auf einmal.
    if [ "$#" -eq 0 ]; then
        _ce_default_modus=1
        set -- HEAD
    fi
    echo ""
    echo "MODUS: git diff selbst ausgefuehrt (Repo: ${_ce_repo_root})"
    echo "ARGUMENTE: git diff -U0 --no-color --no-ext-diff $*"
    git -C "$_ce_repo_root" diff -U0 --no-color --no-ext-diff "$@" > "$_ce_diff_datei" 2>"${_ce_diff_datei}.err"
    _ce_rc=$?
    if [ "$_ce_rc" -ne 0 ]; then
        echo "" >&2
        echo "git diff stderr:" >&2
        cat "${_ce_diff_datei}.err" >&2
        rm -f "${_ce_diff_datei}.err"
        ce_abbruch "git diff selbst ist fehlgeschlagen (rc=${_ce_rc}) -- KEINE stille Null, echter Abbruch."
    fi
    rm -f "${_ce_diff_datei}.err"

    # UNTRACKED DATEIEN: `git diff` sieht sie in KEINEM Modus, auch nicht gegen
    # HEAD -- eine brandneue Datei ist fuer ihn schlicht nicht vorhanden. Genau
    # diese Lage hatte die neue D5-1-Testdatei. Jede untracked, nicht ignorierte
    # Datei wird deshalb einzeln gegen /dev/null gestellt; das erzeugt einen
    # regulaeren Unified-Diff ("+++ b/<pfad>", alle Zeilen als "+"), den die
    # Zustandsmaschine unten unveraendert verarbeitet.
    if [ "$_ce_default_modus" -eq 1 ]; then
        git -C "$_ce_repo_root" ls-files --others --exclude-standard \
            > "$_ce_untracked_liste" 2>/dev/null
        _ce_rc=$?
        [ "$_ce_rc" -eq 0 ] \
            || ce_abbruch "git ls-files --others ist fehlgeschlagen (rc=${_ce_rc}) -- keine stille Null."
        _ce_unt_n=0
        while IFS= read -r _ce_uf; do
            [ -n "$_ce_uf" ] || continue
            [ -f "${_ce_repo_root}/${_ce_uf}" ] || continue
            git -C "$_ce_repo_root" -c core.quotePath=false \
                diff -U0 --no-color --no-ext-diff --no-index -- /dev/null "$_ce_uf" \
                >> "$_ce_diff_datei" 2>/dev/null
            _ce_rc=$?
            # --no-index: 0 = identisch, 1 = Unterschied (hier IMMER der Normalfall,
            # denn die Datei existiert und /dev/null ist leer), >1 = echter Fehler.
            [ "$_ce_rc" -le 1 ] \
                || ce_abbruch "git diff --no-index auf '${_ce_uf}' fehlgeschlagen (rc=${_ce_rc})."
            _ce_unt_n=$(( _ce_unt_n + 1 ))
        done < "$_ce_untracked_liste"
        echo "UNTRACKED: ${_ce_unt_n} neue, nicht ignorierte Datei(en) zusaetzlich einbezogen"
        echo "           (git diff sieht sie sonst in keinem Modus)."
    fi
fi

# ---------------------------------------------------------------------------
# VENDOR-AUSNAHME (2026-08-07): die Wache verspricht in ihrem Kopf, "nur SELBST
# VERFASSTEN Code" zu pruefen -- implementiert war das aber allein ueber die
# DATEIENDUNG. Vendorierter Fremdcode traegt dieselben Endungen und fiel damit
# hinein. Aufgefallen beim ersten Vendor-Snapshot, der ueberhaupt in einem Diff
# lag (libxlsxwriter/zlib): 20 Treffer, ALLE im Fremdcode -- ein "Cafe" in einem
# Upstream-Doku-Kommentar, ein russisches Beispiel, ueberlange Zeilen in minizip.
# Die Wache haette den Autor gezwungen, FREMDEN Quelltext umzuschreiben; genau
# das verbietet die Vendoring-Doktrin (Stufe "faithful": treuer Snapshot).
#
# KEINE pauschale ext/-Ausnahme -- die wuerde auch comdare-EIGENE Dateien dort
# blind stellen (ext/CMakeLists.txt, die Vendor-Wrapper). Stattdessen der Marker,
# den das Haus fuer Vendor-Baeume ohnehin setzt: COMDARE-VENDOR-PROVENANCE.md.
# Wer einen Snapshot einbringt, dokumentiert seine Herkunft -- und genau dieses
# Dokument schaltet die Wache fuer diesen Baum ab. Kein zweiter Mechanismus.
#
# Die Ausnahme ist NICHT still: die uebersprungenen Vendor-Wurzeln werden unten
# im Verdikt namentlich genannt. Eine stille Ausnahme waere die naechste Falle.
_ce_vendor_roots=""
if [ -d "${_ce_repo_root:-.}" ]; then
    _ce_vendor_roots="$(cd "${_ce_repo_root:-.}" 2>/dev/null && \
        find . -name 'COMDARE-VENDOR-PROVENANCE.md' -type f 2>/dev/null \
        | sed 's|^\./||; s|/COMDARE-VENDOR-PROVENANCE\.md$||' | sort | tr '\n' ':')"
fi

awk -v vendor_roots="$_ce_vendor_roots" '
    # -------------------------------------------------------------------
    # Zustandsmaschine statt Regex-Vermutung: "vor dem ersten @@" heisst
    # Datei-Kopf (--- / +++ / index / mode ...), danach ist jede "+"-Zeile
    # ECHTER hinzugefuegter Inhalt -- selbst wenn ihr TEXT mit "+++"
    # beginnt. Kein grep, kein sed; nur substr/index/split.
    # -------------------------------------------------------------------
    BEGIN {
        curfile = "(unbekannt)"
        in_hunk = 0
        newline_no = 0
        total_added = 0
        total_scoped = 0
        total_skipped = 0
        ascii_viol = 0
        width_viol = 0
        skipped_files_n = 0
        fatal = 0
        for (n = 128; n <= 255; n++) { is_high[sprintf("%c", n)] = 1 }
        sigma = sprintf("%c%c", 194, 167)   # SS als UTF-8: 0xC2 0xA7 -- erlaubte Ausnahme
        vendor_n = 0
        if (vendor_roots != "") vendor_n = split(vendor_roots, vendor_list, ":")
        # split() liefert bei abschliessendem ":" ein leeres letztes Feld -- is_vendor
        # faengt das ueber die r != ""-Pruefung ab.
    }
    function is_vendor(fname,    k, r) {
        # Liegt die Datei unter einem Baum mit COMDARE-VENDOR-PROVENANCE.md?
        # Praefix-Vergleich mit abschliessendem "/" -- damit "ext/io" nicht
        # "ext/iotools" mitnimmt.
        for (k = 1; k <= vendor_n; k++) {
            r = vendor_list[k]
            if (r != "" && substr(fname, 1, length(r) + 1) == r "/") return 1
        }
        return 0
    }
    function is_scoped(fname,    base, i, last_dot, ext) {
        if (is_vendor(fname)) return 0
        base = fname
        i = length(base)
        while (i > 0 && substr(base, i, 1) != "/") i--
        base = substr(base, i + 1)
        if (base == "CMakeLists.txt") return 1
        last_dot = 0
        for (i = 1; i <= length(base); i++) if (substr(base, i, 1) == ".") last_dot = i
        if (last_dot == 0) return 0
        ext = substr(base, last_dot)
        if (ext == ".cpp" || ext == ".hpp" || ext == ".h"   || ext == ".hh"  || \
            ext == ".cc"  || ext == ".cxx" || ext == ".tpp" || ext == ".ipp" || \
            ext == ".inl" || ext == ".cmake") return 1
        return 0
    }
    function strip_sigma(s,    i, out, two) {
        out = ""
        i = 1
        while (i <= length(s)) {
            two = substr(s, i, 2)
            if (two == sigma) { i += 2 } else { out = out substr(s, i, 1); i += 1 }
        }
        return out
    }
    function note_skip(f) {
        if (!(f in skipped_files)) { skipped_files[f] = 1; skipped_files_n++ }
    }
    {
        line = $0
        sub(/\r$/, "", line)

        if (substr(line, 1, 10) == "diff --git") { in_hunk = 0; curfile = "(unbekannt)"; next }

        if (in_hunk == 0) {
            if (substr(line, 1, 4) == "+++ ") {
                f = substr(line, 5)
                if (substr(f, 1, 2) == "b/") f = substr(f, 3)
                tabpos = index(f, "\t")
                if (tabpos > 0) f = substr(f, 1, tabpos - 1)
                if (f != "/dev/null") curfile = f
                next
            }
            if (substr(line, 1, 2) == "@@") {
                n = split(line, parts, " ")
                if (n < 3 || substr(parts[3], 1, 1) != "+") {
                    print "FATAL: Hunk-Kopf ohne erkennbares +Start-Feld: " line > "/dev/stderr"
                    fatal = 1
                    exit
                }
                nf = substr(parts[3], 2)
                cpos = index(nf, ",")
                newline_no = (cpos > 0 ? substr(nf, 1, cpos - 1) : nf) + 0
                in_hunk = 1
                next
            }
            next   # sonstige Vorspann-Zeilen (---, index, mode, similarity, ...)
        }

        # in_hunk == 1
        if (substr(line, 1, 2) == "@@") {
            n = split(line, parts, " ")
            if (n < 3 || substr(parts[3], 1, 1) != "+") {
                print "FATAL: Hunk-Kopf ohne erkennbares +Start-Feld: " line > "/dev/stderr"
                fatal = 1
                exit
            }
            nf = substr(parts[3], 2)
            cpos = index(nf, ",")
            newline_no = (cpos > 0 ? substr(nf, 1, cpos - 1) : nf) + 0
            next
        }

        prefix = substr(line, 1, 1)
        if (prefix == "+") {
            total_added++
            content = substr(line, 2)
            if (is_scoped(curfile)) {
                total_scoped++
                checked = strip_sigma(content)
                viol_ascii = 0
                for (i = 1; i <= length(checked); i++) {
                    if (substr(checked, i, 1) in is_high) { viol_ascii = 1; break }
                }
                if (viol_ascii) {
                    ascii_viol++
                    printf "  NICHT-ASCII   %s:%d\n", curfile, newline_no
                }
                if (length(content) > 120) {
                    width_viol++
                    printf "  >120-SPALTEN  %s:%d (%d Byte)\n", curfile, newline_no, length(content)
                }
            } else {
                total_skipped++
                note_skip(curfile)
            }
            newline_no++
            next
        }
        if (prefix == " ") { newline_no++; next }
        next   # "-"-Zeilen und alles andere (z.B. "\ No newline at end of file")
    }
    END {
        if (fatal) { exit 2 }
        printf "SUMMARY_ADDED=%d\n",   total_added   > "'"$_ce_awk_out"'.meta"
        printf "SUMMARY_SCOPED=%d\n",  total_scoped  > "'"$_ce_awk_out"'.meta"
        printf "SUMMARY_SKIPPED=%d\n", total_skipped > "'"$_ce_awk_out"'.meta"
        printf "SUMMARY_ASCII=%d\n",   ascii_viol     > "'"$_ce_awk_out"'.meta"
        printf "SUMMARY_WIDTH=%d\n",   width_viol     > "'"$_ce_awk_out"'.meta"
        for (f in skipped_files) print f > "'"$_ce_awk_out"'.skipped"
    }
' "$_ce_diff_datei" > "$_ce_awk_out"
_ce_awk_rc=$?

if [ "$_ce_awk_rc" -ne 0 ]; then
    cat "$_ce_awk_out" 2>/dev/null
    rm -f "${_ce_awk_out}.meta" "${_ce_awk_out}.skipped"
    ce_abbruch "awk-Verarbeitung des Diffs fehlgeschlagen (rc=${_ce_awk_rc}) -- s. FATAL-Zeile oben."
fi

[ -f "${_ce_awk_out}.meta" ] \
    || ce_abbruch "awk hat keine Zusammenfassung geschrieben -- interner Fehler, keine stille Null."

echo ""
echo "VERSTOESSE (falls vorhanden):"
if [ -s "$_ce_awk_out" ]; then
    cat "$_ce_awk_out"
else
    echo "  (keine)"
fi

_ce_added=0
_ce_scoped=0
_ce_skipped=0
_ce_ascii=0
_ce_width=0
while IFS='=' read -r _ce_k _ce_v; do
    case "$_ce_k" in
        SUMMARY_ADDED)   _ce_added=$_ce_v ;;
        SUMMARY_SCOPED)  _ce_scoped=$_ce_v ;;
        SUMMARY_SKIPPED) _ce_skipped=$_ce_v ;;
        SUMMARY_ASCII)   _ce_ascii=$_ce_v ;;
        SUMMARY_WIDTH)   _ce_width=$_ce_v ;;
    esac
done < "${_ce_awk_out}.meta"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  ${_ce_scoped} Zusatzzeilen in selbst verfasstem Code geprueft, davon ${_ce_ascii} Nicht-ASCII,"
echo "  davon ${_ce_width} ueber 120 Spalten."
echo "  ${_ce_added} Zusatzzeilen insgesamt im Diff; ${_ce_skipped} davon ausserhalb des Scopes"
echo "  (Doku-Prosa/Sonstiges) uebersprungen."
if [ -f "${_ce_awk_out}.skipped" ] && [ -s "${_ce_awk_out}.skipped" ]; then
    echo "  Uebersprungene Dateien (namentlich, keine zweite stille Null):"
    while IFS= read -r _ce_sf; do
        [ -n "$_ce_sf" ] && echo "    - ${_ce_sf}"
    done < "${_ce_awk_out}.skipped"
fi
if [ -n "$_ce_vendor_roots" ]; then
    echo "  VENDOR-AUSNAHME (nicht still): diese Baeume tragen COMDARE-VENDOR-PROVENANCE.md"
    echo "  und werden als Fremdcode NICHT auf ASCII/Breite geprueft --"
    printf '%s' "$_ce_vendor_roots" | tr ':' '\n' | while IFS= read -r _ce_vr; do
        [ -n "$_ce_vr" ] && echo "    - ${_ce_vr}/"
    done
    echo "  Eigener Code in diesen Baeumen (Wrapper, CMakeLists) liegt AUSSERHALB und"
    echo "  wird weiter geprueft."
fi
echo "-----------------------------------------------------------------------------"

rm -f "${_ce_awk_out}.meta" "${_ce_awk_out}.skipped"

# GRUEN MIT NENNER 0 IST ROT (Hausdoktrin, s. scripts/vor_push_alle_wachen.sh im
# Kopf). Im DEFAULT-MODUS hat der Aufrufer gesagt "pruefe meinen Arbeitsstand" --
# findet die Wache dort ueberhaupt nichts, dann hat sie NICHT gemessen, und das
# darf kein bestandenes Gate sein. Bis 09.08.2026 druckte genau dieser Fall
# "GRUEN" und lieferte Exit 0; das war die Quittung, die ein Agent nach `git add`
# bekam, ohne dass je eine Zeile geprueft worden waere.
#
# BEWUSST NUR IM DEFAULT-MODUS: ein AUSDRUECKLICH angegebener Bereich, der leer
# ist, ist eine Aussage des Aufrufers (leerer Merge-Commit, Pfad-Einschraenkung
# ohne Treffer) und darf die CI nicht abwuergen. Dieselbe Asymmetrie prueft der
# Selbsttest in den Faellen 7 und 9.
if [ "$_ce_default_modus" -eq 1 ] && [ "$_ce_added" -eq 0 ]; then
    echo ""
    ce_abbruch "DEFAULT-MODUS mit NENNER 0 -- es wurde nichts geprueft, also ist \
nichts bestanden. Der Arbeitsbaum ist gegenueber HEAD unveraendert und es liegen \
keine untracked Dateien vor. Ist die Arbeit schon committet, dann pruefe den \
BEREICH: 'sh scripts/vor_push_alle_wachen.sh' (bestimmt ihn selbst) oder \
'sh scripts/ci_diff_ascii_width_guard.sh <basis>...HEAD'."
fi

echo ""
# DER BEREICH GEHOERT AN DAS VERDIKT, nicht nur in den Kopf der Ausgabe. Wer eine
# Wache aufruft, liest ihre letzte Zeile -- und "GRUEN." allein ist ueber diese
# Wache keine Aussage: dasselbe Werkzeug auf demselben Baum sagt GRUEN oder ROT,
# je nachdem, welchen Bereich es bekommen hat (am Objekt: 09.08.2026, Baum
# aebc4f2c, Push gruen / 86 Commits rot). Ohne den Bereich am Verdikt kann ein
# Bericht die eine Zeile zitieren und dabei die halbe Aussage weglassen, ohne
# irgendetwas Falsches zu schreiben.
if [ "$_ce_ascii" -eq 0 ] && [ "$_ce_width" -eq 0 ]; then
    if [ -n "$_ce_bereich_satz" ]; then
        echo "DIFF-HYGIENE-WACHE: GRUEN.  KUMULATIV ueber ${_ce_bereich_satz}"
    else
        echo "DIFF-HYGIENE-WACHE: GRUEN."
    fi
    exit 0
else
    if [ -n "$_ce_bereich_satz" ]; then
        echo "DIFF-HYGIENE-WACHE: ROT.  KUMULATIV ueber ${_ce_bereich_satz}"
    else
        echo "DIFF-HYGIENE-WACHE: ROT."
    fi
    exit 1
fi
