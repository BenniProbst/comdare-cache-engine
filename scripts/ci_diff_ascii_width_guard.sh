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
#
# AUFRUF:
#   sh scripts/ci_diff_ascii_width_guard.sh [<git-diff-args...>]
#       Ohne Argumente: git diff -U0 (Arbeitsverzeichnis gegen HEAD, ersetzt
#       die bisherige manuelle Vor-Paketmeldung-Pruefung 1:1).
#       Mit Argumenten: an `git diff -U0 --no-color --no-ext-diff` durchgereicht,
#       z.B. ein Revisionsbereich fuer CI (`origin/main...HEAD`) oder eine
#       Pfad-Einschraenkung nach `--`.
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
if [ "${1:-}" = "--stdin" ]; then
    _ce_stdin_modus=1
    shift
fi

_ce_diff_datei="$(mktemp "${TMPDIR:-/tmp}/ce_diff_guard.XXXXXX")" \
    || ce_abbruch "mktemp fuer die Diff-Datei fehlgeschlagen."
_ce_awk_out="$(mktemp "${TMPDIR:-/tmp}/ce_diff_guard_out.XXXXXX")" \
    || ce_abbruch "mktemp fuer die Ausgabedatei fehlgeschlagen."
trap 'rm -f "$_ce_diff_datei" "$_ce_awk_out"' EXIT INT TERM HUP

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
fi

awk '
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
    }
    function is_scoped(fname,    base, i, last_dot, ext) {
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
echo "-----------------------------------------------------------------------------"

rm -f "${_ce_awk_out}.meta" "${_ce_awk_out}.skipped"

echo ""
if [ "$_ce_ascii" -eq 0 ] && [ "$_ce_width" -eq 0 ]; then
    echo "DIFF-HYGIENE-WACHE: GRUEN."
    exit 0
else
    echo "DIFF-HYGIENE-WACHE: ROT."
    exit 1
fi
