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
#   * Sie prueft ALLES ausser zwei ausdruecklich benannten Ausnahmen (s.
#     skip_grund() unten): deutsche Doku-Prosa (*.md) traegt per Sprachdoktrin
#     korrekte Umlaute, und Vendor-Baeume mit COMDARE-VENDOR-PROVENANCE.md sind
#     fremder Quelltext, den niemand umschreiben darf. Ausgenommene Zeilen werden
#     trotzdem GEZAEHLT und ihre Dateien NAMENTLICH samt GRUND gemeldet, damit
#     "ausserhalb des Scopes" nicht zu einer zweiten stillen Null wird. Rohe
#     Build-/Test-Logs mit Unicode-Trennlinien (CMake/make) stehen nie in einem
#     Diff dieser Art (sie sind keine versionierten Quelldateien) und sind damit
#     strukturell aussen vor, nicht per Ausnahme.
#
# WARUM DIE RICHTUNG AM 10.08.2026 UMGEDREHT WURDE (Befund am Objekt):
# Bis heute stand hier eine WHITELIST von Endungen -- .cpp/.hpp/.h/.hh/.cc/.cxx/
# .tpp/.ipp/.inl/.cmake plus CMakeLists.txt. Eine Whitelist laesst alles durch,
# was sie NICHT nennt, und sie nennt sich dabei nicht selbst: .sh, .c, .txt,
# .yml, .py und .tcc fielen lautlos heraus. Die Wache meldete GRUEN und hatte die
# Datei nie angesehen.
#
# DER SCHADEN WAR REAL, nicht bloss potenziell: ein Strang kuerzte seine
# Bilanz-Zeile von 159 auf 121 und dann auf 122 Byte, ohne nachzumessen, und kam
# durch alle Tore. Sein Satz dazu: "ihr GRUEN ueber meinen Bereich war KEIN
# Urteil ueber die Datei, die ich geaendert habe."
#
# GEMESSEN VOR DEM UMBAU (10.08.2026, ce/development 9f932e91, Nenner aus
# `git ls-files` -- also aus einer ANDEREN Quelle als dieser Wache):
#   * 143 getrackte *.sh mit 10.084 Zeilen. Davon 24 im eigenen Baum (5.872
#     Zeilen) mit 42 Nicht-ASCII und 55 ueberlangen Zeilen = 97 Verstoesse in 14
#     Dateien; 119 unter ext/ (4.212 Zeilen) mit 29 ueberlangen Zeilen.
#   * Die Wache meldete darauf 0 -- sie hat *.sh nie angesehen. 0 gegen 126 ist
#     der blinde Fleck, in Verstoessen gemessen.
# Dieser BESTAND ist mit dem Umbau NICHT beseitigt: die Wache ist ein DIFF-Gate
# und sieht nur HINZUGEFUEGTE Zeilen. Der Umbau stoppt den ZUWACHS. Der Bestand
# ist ein eigener Posten (analog zum ASCII-Zaehler des Wellenplans fuer W7) --
# ausdruecklich UNGEDECKT und hier benannt, damit er nicht still bleibt.
#
# WAS DER UMBAU KOSTET, auf ECHTEN Bereichen gemessen statt geschaetzt (dieselbe
# Wache, einmal mit alter und einmal mit neuer skip_grund(), gegen dieselben
# Diffs): HEAD~1 0 Verstoesse, HEAD~10 1, HEAD~25 1, HEAD~50 38, HEAD~100 79
# (davon 58 in *.sh -- genau der Defekt, den dieser Umbau schliesst),
# 1880f296..HEAD (der main-FF-Bereich, 2.462 Zusatzzeilen) 0. Der CI-Bereich ist
# CI_COMMIT_BEFORE_SHA..HEAD, also ein Push -- die realistische Gate-Last liegt
# bei 0 bis 1 Verstoessen. Der Umbau macht den naechsten Push nicht rot.
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
if [ "${1:-}" = "--stdin" ]; then
    _ce_stdin_modus=1
    shift
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
    # core.quotePath=false: sonst liefert git Pfade mit Nicht-ASCII als
    # C-quotierte Zeichenkette samt Anfuehrungszeichen ("b/...\302\247....md").
    # Die awk-Seite streift die Klammerung zwar ohnehin ab (s. dort), aber hier
    # entsteht sie erst gar nicht -- und die Dateinamen im Nenner bleiben lesbar
    # statt als Oktal-Wueste. Der untracked-Zweig unten setzt es bereits seit
    # 09.08.2026; dass der Haupt-Aufruf es NICHT tat, war die Asymmetrie.
    git -C "$_ce_repo_root" -c core.quotePath=false \
        diff -U0 --no-color --no-ext-diff "$@" > "$_ce_diff_datei" 2>"${_ce_diff_datei}.err"
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
    # BLACKLIST statt Endungs-WHITELIST (10.08.2026, s. Kopf). Rueckgabe: die
    # leere Zeichenkette heisst GEPRUEFT, jede andere ist der GRUND der Ausnahme
    # und wird im Nenner je Datei mitgedruckt. Genau ZWEI Gruende sind zulaessig;
    # wer einen dritten braucht, aendert diese Funktion und begruendet ihn hier.
    #
    # WARUM DIE RICHTUNG ZAEHLT: eine Whitelist muss jede zu pruefende Endung
    # kennen und schweigt ueber jede neue -- sie faellt zur falschen Seite aus.
    # Eine Blacklist faellt zur richtigen Seite aus: eine unbekannte Endung wird
    # GEPRUEFT. Kommt morgen eine .rs, .tcc oder .toml ins Repo, ist sie ohne
    # Zutun gedeckt. Das ist dieselbe Fail-Closed-Richtung, die diese Wache auch
    # bei fehlendem Werkzeug und bei leerem Nenner einhaelt.
    function skip_grund(fname,    base, i, last_dot, ext) {
        if (is_vendor(fname)) return "Vendor-Baum, COMDARE-VENDOR-PROVENANCE.md"
        base = fname
        i = length(base)
        while (i > 0 && substr(base, i, 1) != "/") i--
        base = substr(base, i + 1)
        last_dot = 0
        for (i = 1; i <= length(base); i++) if (substr(base, i, 1) == ".") last_dot = i
        ext = (last_dot == 0) ? "" : substr(base, last_dot)
        # *.md: deutsche Doku-Prosa traegt per Sprachdoktrin korrekte Umlaute.
        # Das ist die urspruengliche und weiterhin richtige Begruendung -- sie
        # galt nie fuer Quelltext, der nur zufaellig nicht auf der Liste stand.
        if (ext == ".md") return "Doku-Prosa *.md, Sprachdoktrin"
        return ""
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
    function note_skip(f, grund) {
        # Der GRUND wird je Datei festgehalten, nicht bloss die Tatsache des
        # Ueberspringens: "ausserhalb des Scopes" ohne Grund ist genau das
        # irrefuehrende Etikett, an dem die Shell-Blindheit so lange vorbeikam.
        if (!(f in skipped_files)) { skipped_files[f] = grund; skipped_files_n++ }
    }
    {
        line = $0
        sub(/\r$/, "", line)

        if (substr(line, 1, 10) == "diff --git") { in_hunk = 0; curfile = "(unbekannt)"; next }

        if (in_hunk == 0) {
            if (substr(line, 1, 4) == "+++ ") {
                f = substr(line, 5)
                tabpos = index(f, "\t")
                if (tabpos > 0) f = substr(f, 1, tabpos - 1)
                # C-QUOTIERUNG ABSTREIFEN (git core.quotePath). Pfade mit
                # Nicht-ASCII oder Sonderzeichen liefert git als
                #     +++ "b/docs/sessions/...-\302\2478-PILOT-G1G3.md"
                # also MIT Anfuehrungszeichen und mit Oktal-Escapes. Ohne dieses
                # Abstreifen endet der Basename auf .md" statt auf .md -- die
                # *.md-Ausnahme greift dann nicht mehr, und deutsche Doku-Prosa
                # wird geprueft und rot. Am Objekt belegt (10.08.2026): der
                # Commit 163db10d faerbte so 37 Verstoesse in einer reinen
                # Session-Doku, obwohl an ihr nichts falsch ist.
                # Unter der alten WHITELIST fiel das nicht auf: dort war ".md\""
                # ebenso wenig gescoped wie ".md" -- die Fehlbehandlung und die
                # gewollte Ausnahme sahen zufaellig gleich aus. Erst die
                # Blacklist macht den Unterschied sichtbar, weil sie zur anderen
                # Seite ausfaellt. Der Default- und der Bereichsmodus setzen
                # zusaetzlich core.quotePath=false (s. unten); dieser Griff hier
                # deckt den --stdin-Weg ab, wo der Aufrufer den Diff erzeugt hat.
                if (length(f) >= 2 && substr(f, 1, 1) == "\"" \
                    && substr(f, length(f), 1) == "\"") {
                    f = substr(f, 2, length(f) - 2)
                }
                if (substr(f, 1, 2) == "b/") f = substr(f, 3)
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
            grund = skip_grund(curfile)
            if (grund == "") {
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
                note_skip(curfile, grund)
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
        # Tabulator als Trenner: Dateinamen im Baum enthalten keinen, und die
        # Shell-Seite liest Pfad und Grund damit ohne Rateschritt auseinander.
        for (f in skipped_files) print f "\t" skipped_files[f] > "'"$_ce_awk_out"'.skipped"
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
echo "  ${_ce_added} Zusatzzeilen insgesamt im Diff; ${_ce_skipped} davon uebersprungen."
echo "  GEPRUEFT WIRD ALLES; ausgenommen sind genau zwei Klassen, je Datei unten"
echo "  mit GRUND benannt: Doku-Prosa *.md und Vendor-Baeume. Die frueher hier"
echo "  gedruckte Sammelbezeichnung \"(Doku-Prosa/Sonstiges)\" war irrefuehrend --"
echo "  unter \"Sonstiges\" lagen 143 getrackte *.sh mit 10.084 Zeilen Quelltext."
if [ -f "${_ce_awk_out}.skipped" ] && [ -s "${_ce_awk_out}.skipped" ]; then
    echo "  Uebersprungene Dateien (namentlich MIT GRUND, keine zweite stille Null):"
    while IFS="$(printf '\t')" read -r _ce_sf _ce_sg; do
        [ -n "$_ce_sf" ] && echo "    - ${_ce_sf}  [${_ce_sg}]"
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
# (c) DER BESTAND IST EIN EIGENER POSTEN -- und er wird hier BENANNT, nicht
# stillschweigend mitgemeint. Diese Wache ist ein DIFF-Gate: sie urteilt ueber
# HINZUGEFUEGTE Zeilen und ueber sonst nichts. Ein GRUEN heisst "der Zuwachs ist
# sauber", NIE "die Datei ist sauber" -- genau diese Verwechslung war der
# Schaden vom 09.08.2026. Bewusst OHNE Zahl: eine hier eingedruckte Bestandszahl
# veraltet mit dem naechsten Commit und wuerde zur zweiten stillen Null. Die
# gemessene Zahl steht datiert im Kopf dieser Datei.
echo "  GRENZE (ungedeckt, ausdruecklich): geprueft ist der ZUWACHS in diesem Diff,"
echo "  nicht der Bestand der beruehrten Dateien und nicht der des Baums. Wer ueber"
echo "  eine DATEI urteilen will, misst die Datei -- nicht diesen Bereich."
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
if [ "$_ce_ascii" -eq 0 ] && [ "$_ce_width" -eq 0 ]; then
    echo "DIFF-HYGIENE-WACHE: GRUEN."
    exit 0
else
    echo "DIFF-HYGIENE-WACHE: ROT."
    exit 1
fi
