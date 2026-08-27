# shellcheck shell=sh
# =============================================================================
#  PRE-PUSH-LANDE-GATES: die Lande-Gate-Kette als WERKZEUG, nicht als Disziplin
#  (scripts/pre_push_lande_gates.sh)                    A2.5-Fixstrecke-2, G6/F9
# =============================================================================
#
# WARUM ES DIESES SKRIPT GIBT (19.08.2026, K3/H6 "Werkzeug schlaegt Disziplin"):
# die Lande-Gate-Kette vor einem Push existierte als KOPF-WISSEN -- sechs Gates,
# jedes einzeln vorhanden, aber niemand fuhr sie ZUSAMMEN, in fester Reihenfolge,
# mit Nenner je Gate. Der H6-Befund der Fixstrecke: was nur Disziplin ist, faellt
# unter Takt zuerst (dieselbe Klasse wie der 07.08.-Befund im Kopf von
# scripts/vor_push_alle_wachen.sh und der ledger_nachtrag.sh-Grund: eine REGEL
# ohne MECHANISMUS wird vergessen, ein Werkzeug nicht). Ab hier ist die Kette
# EIN Aufruf.
#
# AUFRUF:
#   sh scripts/pre_push_lande_gates.sh [<basis>] [<spitze>]
#       Default-Basis:  origin/development  (fehlt sie lokal: ABBRUCH, Basis
#       ausdruecklich angeben -- z.B. den dokumentierten development-Anker).
#       Default-Spitze: HEAD.
#   sh scripts/pre_push_lande_gates.sh --nur=<gate> [<basis>] [<spitze>]
#       <gate> in: diff | format | gitleaks | lock | floor | tabu.
#       Einzel-Gate ist ein PROBE-Modus: die nicht gefahrenen Gates werden am
#       Ende AUSDRUECKLICH als NICHT GEFAHREN ausgewiesen, nie still gruen.
#   Umgebung:
#       COMDARE_PRE_PUSH_BUILD_DIR  Bau-Baum fuer das Floor-Gate: PFLICHT, sobald das
#                                   Floor-Gate faehrt (KEIN Default seit H-1 25.08.2026,
#                                   s. [5/6]); zugleich erste Suchstelle fuer [4/6].
#       COMDARE_AXIS_LOCK_BIN       axis_version_lock-Binary (sonst Suche erst im
#                                   genannten Baum, dann in den vier Preset-Baeumen
#                                   build/<cc>-<cfg>).
#       COMDARE_GITLEAKS            gitleaks-Binary (sonst ~/.local/bin, PATH).
#
# KEIN HOOK-INSTALL: dieses Skript ist das WERKZEUG. Ob und wie es als
# pre-push-Hook haengt, entscheidet der Lead separat -- F9 liefert die Mechanik,
# nicht die Montage. Haendisch vor JEDEM Push fahren, bis der Hook haengt.
#
# EXIT:
#   0 = alle GEFAHRENEN Gates gruen (bei --nur: NUR das eine, s. Ausweis).
#   1 = ein Gate ist ROT. FAIL-FAST: das ERSTE rote Gate beendet den Lauf HART,
#       spaetere Gates laufen dann nicht -- wer weiterprueft, prueft einen Baum,
#       der so nicht gepusht werden darf.
#   2 = ABBRUCH (Werkzeug fehlt, Bereich unbestimmbar, Baum nicht pruefbar).
#       Ausdruecklich KEIN Gruen: ein nicht gelaufenes Gate ist kein bestandenes.
#
# DIE SECHS GATES (Reihenfolge: billig vor teuer, Bereich vor Bestand):
#   [1/6] DIFF-HYGIENE, KUMULATIV ueber den Bereich (ASCII + Spaltenbreite) --
#         delegiert an scripts/ci_diff_ascii_width_guard.sh --bereich. Kumulativ
#         heisst: EIN Diff <basis>...<spitze>, nicht je Commit (Schwesterfrage
#         und 58-Verstoesse-Befund: Kopf von vor_push_alle_wachen.sh).
#   [2/6] CLANG-FORMAT in CI-FORMEL-PARITAET (Vollmenge wie Job lint:format) --
#         delegiert an scripts/vor_push_alle_wachen.sh --nur=format. Die Formel
#         lebt an GENAU EINER Stelle; eine zweite Abschrift hier wuerde driften.
#   [3/6] GITLEAKS, zweistufig -- erst KOEDER-SELBSTBISS: ein zur Laufzeit
#         zusammengesetzter glpat-Koeder (Laenge-26-ASSERT: 6 Praefix + 20 Rumpf,
#         die Form der gitlab-pat-Regel; Herkunft der Regel: .gitleaks.toml
#         [extend] useDefault) MUSS gefunden werden, sonst ist Werkzeug oder
#         Config stumpf und jedes Gruen des Echt-Scans wertlos (ABBRUCH, kein
#         Rot). Dann ECHT-SCAN: git log -p ueber <basis>..<spitze> durch
#         gitleaks stdin MIT --config (stdin ohne --config war die 08.08.-Falle:
#         Scan ohne Projekt-Regeln). Der Koeder steht NIRGENDS als Literal --
#         weder hier noch in Commits (die stdin-Push-Wache wuerde ihn beissen);
#         der Rumpf entsteht aus Zeichencodes 122..103, Entropie-tauglich fuer
#         die Regel (A*20 beisst NICHT, am 19.08.2026 am Objekt gemessen).
#   [4/6] AXIS-VERSION-LOCK --check (Digest-Tripwire der Achsen-/Overlay-Header
#         gegen tools/axis_version_lock/axis_version.lock). Exit 3 des Tools
#         ("Version-Bump ok, Lock-Regen fehlt") ist fuer einen Push ROT, kein
#         Hinweis: die CI erzwingt --write + git diff --exit-code.
#   [5/6] FLOOR-SCANNER: ctest -N im Bau-Baum gegen den EXAKT-Anker der
#         Host-Klasse aus scripts/ci_test_inventory_floor.txt. BEIDE Richtungen
#         rot (unter dem Anker: Tests verschwunden; darueber: Floor-Nachzug
#         fehlt, #39). Host-Klasse aus der CMakeCache DESSELBEN Baums
#         (Wert 1 = ja, leer = nein, alles andere ABBRUCH -- Kurzform der
#         Leiter aus ci_test_coverage_guard.sh; die dortigen Tiefenproben
#         _COMPILED/_EXITCODE fahren weiter NUR dort, s. GRENZEN unten).
#         Der Baum wird GENANNT (COMDARE_PRE_PUSH_BUILD_DIR, kein Default: H-1)
#         und muss drei Proben bestehen -- Cache + CTestTestfile vorhanden,
#         konfiguriert aus DIESEM Checkout (CMAKE_HOME_DIRECTORY), keine
#         getrackte CMake-Datei juenger als seine Registrierung. Sein
#         Testing/Temporary/LastTest.log bleibt byteidentisch (ctest -N wuerde
#         es sonst zum 121-Byte-Stub machen, RV-03).
#   [6/6] TABU-CRC-PROBE, dreiteilig: (a) der CRC64-Anker 0x56F1B721C72DC10E
#         steht GENAU EINMAL in libs/cache_engine/profile_facade/
#         source_catalog.hpp (kNewGolden131072Crc64); (b) die fuenf TABU-Dateien
#         (permutation_axes.xml + golden_fullpilot_320_binary_ids*.txt) tragen
#         byteidentisch ihre eingefrorenen sha256-SOLLs; (c) der Bereich
#         <basis>..<spitze> BEWEGT keine dieser Dateien. Ein legitimer Re-Anker
#         (golden-Ereignis B-10.3) aendert Anker, SOLLs und DIESES Skript in
#         EINEM bewussten Commit im golden-Fenster -- NIE nebenbei; bis dahin
#         ist jede Abweichung ROT und niemals ein Grund zum Regenerieren.
#
# GRENZEN, EHRLICH BENANNT (dieses Skript prueft NICHT):
#   * Bau + ctest-LAUF (Kombibau-Regel) -- gehoeren in den Bau-Schritt; das
#     Floor-Gate zaehlt die REGISTRIERUNG, nicht den Erfolg der Tests.
#   * cppcheck (lint:static) -- eigener Voll-Scope-Lauf, s.
#     vor_push_alle_wachen.sh ohne Schalter.
#   * die drei Achsen der Abdeckungs-Wache (ci_test_coverage_guard.sh) samt
#     CMakeCache-Tiefenproben -- CI-Gegenstand mit eigenem Manifest.
#   * Submodul-/Fremdrepo-Staende, LaTeX-Gates, CI-only-Wachen.
#
# POSIX-sh, kein bash-ismus, kein grep -P; Exits werden NIE hinter einer Pipe
# gemessen (K11-PIPESTATUS-Falle), jede Stufe schreibt in eine Datei.
# =============================================================================

set -eu

abbruch() {
    echo ""
    echo "PRE-PUSH-LANDE-GATES: ABBRUCH -- $1" >&2
    exit 2
}

gate_rot() {
    echo ""
    echo "============================================================================="
    echo "PRE-PUSH-LANDE-GATES: ROT am GATE $1 -- $2"
    echo "FAIL-FAST: spaetere Gates sind NICHT gelaufen und damit NICHT gruen."
    echo "============================================================================="
    exit 1
}

command -v git >/dev/null 2>&1 || abbruch "git fehlt"
git rev-parse --git-dir >/dev/null 2>&1 || abbruch "kein git-Repository"
REPO_ROOT=$(git rev-parse --show-toplevel) || abbruch "Repo-Wurzel nicht bestimmbar"
cd "$REPO_ROOT" || abbruch "cd in die Repo-Wurzel fehlgeschlagen"

TMPD=$(mktemp -d) || abbruch "mktemp -d fehlgeschlagen"
trap 'rm -rf "$TMPD"' EXIT INT TERM

# -- Aufruf lesen -------------------------------------------------------------
NUR=""
BASIS_ARG=""
SPITZE_ARG=""
for _arg in "$@"; do
    case "$_arg" in
        --nur=diff|--nur=format|--nur=gitleaks|--nur=lock|--nur=floor|--nur=tabu)
            [ -z "$NUR" ] || abbruch "mehr als ein --nur-Schalter"
            NUR=${_arg#--nur=}
            ;;
        --nur=*) abbruch "unbekanntes Gate '${_arg#--nur=}' (bekannt: diff format gitleaks lock floor tabu)" ;;
        --*) abbruch "unbekannter Schalter '${_arg}'" ;;
        *)
            if [ -z "$BASIS_ARG" ]; then BASIS_ARG="$_arg"
            elif [ -z "$SPITZE_ARG" ]; then SPITZE_ARG="$_arg"
            else abbruch "zu viele Argumente ('${_arg}' ist das dritte)"
            fi
            ;;
    esac
done

# Welche Gates faehrt dieser Lauf?
faehrt() {
    [ -z "$NUR" ] && return 0
    [ "$NUR" = "$1" ]
}

echo "============================================================================="
if [ -n "$NUR" ]; then
    echo " PRE-PUSH-LANDE-GATES (--nur=${NUR}): EIN Gate, PROBE-Modus"
else
    echo " PRE-PUSH-LANDE-GATES: sechs Gates, fail-fast, Nenner je Gate"
fi
echo "============================================================================="

# -- Bereich bestimmen (nur wenn ein bereichsgebundenes Gate faehrt) ----------
BEREICH_NOETIG=0
for _g in diff gitleaks tabu; do faehrt "$_g" && BEREICH_NOETIG=1; done
BASIS=""
SPITZE=""
ANZ_COMMITS=""
if [ "$BEREICH_NOETIG" -eq 1 ]; then
    BASIS="${BASIS_ARG:-origin/development}"
    SPITZE="${SPITZE_ARG:-HEAD}"
    git rev-parse --verify --quiet "$BASIS^{commit}" >/dev/null \
        || abbruch "Basis '$BASIS' existiert hier nicht. KEIN stiller Ersatz (Nenner-0-Falle) -- \
Basis ausdruecklich angeben: sh scripts/pre_push_lande_gates.sh <basis> [<spitze>]"
    git rev-parse --verify --quiet "$SPITZE^{commit}" >/dev/null \
        || abbruch "Spitze '$SPITZE' existiert hier nicht"
    ANZ_COMMITS=$(git rev-list --count "$BASIS..$SPITZE") || abbruch "rev-list ueber $BASIS..$SPITZE scheitert"
    echo ""
    echo "BEREICH: $BASIS..$SPITZE   ($ANZ_COMMITS Commits)"
    echo "  Basis:  $(git rev-parse --short "$BASIS")   Spitze: $(git rev-parse --short "$SPITZE")"
    if [ "$ANZ_COMMITS" -eq 0 ]; then
        abbruch "0 Commits im Bereich -- nichts zu pushen oder falsche Basis. Ein leerer Bereich ist KEIN Gruen."
    fi
fi
echo ""

# =============================================================================
# [1/6] DIFF-HYGIENE, kumulativ ueber den Bereich
# =============================================================================
if faehrt diff; then
    echo "============================================================================="
    echo " [1/6] DIFF-HYGIENE (ASCII + Spaltenbreite), KUMULATIV $BASIS...$SPITZE"
    echo "============================================================================="
    [ -f scripts/ci_diff_ascii_width_guard.sh ] \
        || abbruch "scripts/ci_diff_ascii_width_guard.sh fehlt -- die CI faehrt sie, also muss sie hier laufen"
    set +e
    sh scripts/ci_diff_ascii_width_guard.sh --bereich "$BASIS" "$SPITZE"
    _rc=$?
    set -e
    case "$_rc" in
        0) ;;
        1) gate_rot "[1/6] DIFF-HYGIENE" "Verstoesse im kumulativen Diff (Liste oben)" ;;
        *) abbruch "Diff-Hygiene-Wache endete mit Exit $_rc (kein Verdikt)" ;;
    esac
    echo "GATE [1/6] GRUEN. NENNER: siehe Wachen-Ausgabe oben (Bereich, Dateien, Zusatzzeilen)."
    echo ""
fi

# =============================================================================
# [2/6] CLANG-FORMAT in CI-Formel-Paritaet (Vollmenge)
# =============================================================================
if faehrt format; then
    echo "============================================================================="
    echo " [2/6] CLANG-FORMAT -- CI-Formel-Paritaet (delegiert: vor_push_alle_wachen.sh --nur=format)"
    echo "============================================================================="
    [ -f scripts/vor_push_alle_wachen.sh ] || abbruch "scripts/vor_push_alle_wachen.sh fehlt"
    set +e
    sh scripts/vor_push_alle_wachen.sh --nur=format
    _rc=$?
    set -e
    case "$_rc" in
        0) ;;
        1) gate_rot "[2/6] CLANG-FORMAT" "Format-Abweichungen in der CI-Vollmenge (Liste oben)" ;;
        *) abbruch "Format-Wache endete mit Exit $_rc (kein Verdikt)" ;;
    esac
    echo "GATE [2/6] GRUEN. NENNER: siehe Wachen-Ausgabe oben (Vollmenge/geprueft/abweichend)."
    echo ""
fi

# =============================================================================
# [3/6] GITLEAKS: Koeder-Selbstbiss (Laenge-26-Assert), dann Echt-Scan Bereich
# =============================================================================
if faehrt gitleaks; then
    echo "============================================================================="
    echo " [3/6] GITLEAKS -- Selbstbiss, dann Echt-Scan ueber $BASIS..$SPITZE"
    echo "============================================================================="
    GL="${COMDARE_GITLEAKS:-}"
    if [ -n "$GL" ]; then
        command -v "$GL" >/dev/null 2>&1 || abbruch "COMDARE_GITLEAKS='$GL' ist nicht ausfuehrbar"
    else
        for _k in "$HOME/.local/bin/gitleaks" gitleaks; do
            command -v "$_k" >/dev/null 2>&1 && { GL="$_k"; break; }
        done
        [ -n "$GL" ] || abbruch "kein gitleaks gefunden (gesucht: ~/.local/bin/gitleaks, PATH). COMDARE_GITLEAKS setzen"
    fi
    GL_VER=$("$GL" version 2>/dev/null) || abbruch "'$GL version' scheitert"
    [ -f .gitleaks.toml ] || abbruch ".gitleaks.toml fehlt -- stdin-Scan OHNE Config war die 08.08.-Falle"
    echo "WERKZEUG: $GL (Version $GL_VER), Config .gitleaks.toml"

    # (a) KOEDER-SELBSTBISS. Der Rumpf entsteht aus Zeichencodes (20 verschiedene
    # Zeichen, z..g absteigend) -- NIE als Literal, sonst beisst die stdin-Wache
    # jeden Commit dieses Skripts. Das Laenge-26-Assert pinnt die Form der Regel
    # (glpat- + 20): ein verkuerzter Koeder truege die Regel nicht mehr und der
    # Selbstbiss wuerde zur Attrappe.
    _k_rumpf=$(awk 'BEGIN { for (i = 122; i > 102; i--) printf "%c", i }')
    _koeder="glpat-${_k_rumpf}"
    [ "${#_koeder}" -eq 26 ] \
        || abbruch "Koeder-Laenge ${#_koeder} != 26 -- der Selbstbiss traefe nicht die gitlab-pat-Regel"
    set +e
    printf 'koeder_probe = %s\n' "$_koeder" | "$GL" stdin --config .gitleaks.toml --no-banner --redact \
        > "$TMPD/selbstbiss.log" 2>&1
    _rc=$?
    set -e
    if [ "$_rc" -eq 0 ]; then
        abbruch "SELBSTBISS FEHLGESCHLAGEN: der 26er-glpat-Koeder wurde NICHT gefunden. Werkzeug oder \
Config ist stumpf -- ein Gruen des Echt-Scans waere jetzt wertlos (fail-closed, kein Gruen)"
    elif [ "$_rc" -ne 1 ]; then
        abbruch "gitleaks-Selbstbiss endete mit Exit $_rc (weder Fund noch sauber) -- rohes Log-Ende: \
$(tail -3 "$TMPD/selbstbiss.log" | tr '\n' ' ')"
    fi
    echo "SELBSTBISS: gebissen (Koeder-Laenge 26, gitleaks-Exit 1) -- Werkzeug und Config sind scharf."

    # (b) ECHT-SCAN ueber den Bereich: git log -p deckt JEDEN Commit des Pushs,
    # nicht nur den End-Diff -- ein in Commit 5 eingecheckter und in Commit 20
    # geloeschter Schluessel liegt trotzdem im Push und wird hier gefunden.
    set +e
    git log -p --no-color "$BASIS..$SPITZE" > "$TMPD/bereich.patch" 2>"$TMPD/gitlog.err"
    _rc=$?
    set -e
    [ "$_rc" -eq 0 ] || abbruch "git log -p scheitert: $(tr '\n' ' ' < "$TMPD/gitlog.err")"
    _bytes=$(wc -c < "$TMPD/bereich.patch" | tr -d ' ')
    [ "$_bytes" -gt 0 ] || abbruch "leerer Patch ueber $ANZ_COMMITS Commits -- das passt nicht zusammen"
    set +e
    "$GL" stdin --config .gitleaks.toml --no-banner --redact < "$TMPD/bereich.patch" \
        > "$TMPD/echtscan.log" 2>&1
    _rc=$?
    set -e
    if [ "$_rc" -eq 1 ]; then
        echo "FUNDE (redacted):"
        sed 's/^/  /' "$TMPD/echtscan.log" | tail -40
        gate_rot "[3/6] GITLEAKS" "Echt-Scan meldet Funde im Bereich (oben, redacted)"
    elif [ "$_rc" -ne 0 ]; then
        abbruch "gitleaks-Echt-Scan endete mit Exit $_rc -- rohes Log-Ende: \
$(tail -3 "$TMPD/echtscan.log" | tr '\n' ' ')"
    fi
    echo "GATE [3/6] GRUEN. NENNER: $ANZ_COMMITS Commits, $_bytes Bytes gescannt, 0 Funde;"
    echo "  Selbstbiss davor: 1 Fund aus 1 Koeder (Pflicht)."
    echo ""
fi

# =============================================================================
# [4/6] AXIS-VERSION-LOCK --check
# =============================================================================
if faehrt lock; then
    echo "============================================================================="
    echo " [4/6] AXIS-VERSION-LOCK --check (Digest-Tripwire gegen axis_version.lock)"
    echo "============================================================================="
    LOCKF="tools/axis_version_lock/axis_version.lock"
    [ -f "$LOCKF" ] || abbruch "$LOCKF fehlt"
    LB="${COMDARE_AXIS_LOCK_BIN:-}"
    if [ -n "$LB" ]; then
        [ -x "$LB" ] || abbruch "COMDARE_AXIS_LOCK_BIN='$LB' ist nicht ausfuehrbar"
    else
        # Suchreihenfolge: erst der GENANNTE Baum (COMDARE_PRE_PUSH_BUILD_DIR, H-1), dann die vier
        # Preset-Baeume aus CMakePresets.json (build/<cc>-<cfg>). Nichts davon ist ein Default-Urteil:
        # fehlt das Werkzeug ueberall, ist das ABBRUCH, kein Gruen.
        _gesehen=""
        _baeume="build/gcc-release build/clang-release build/gcc-debug build/clang-debug"
        [ -z "${COMDARE_PRE_PUSH_BUILD_DIR:-}" ] || _baeume="$COMDARE_PRE_PUSH_BUILD_DIR $_baeume"
        for _b in $_baeume; do
            _k="$_b/tools/axis_version_lock/comdare_axis_version_lock"
            _gesehen="$_gesehen $_k"
            [ -x "$_k" ] && { LB="$_k"; break; }
        done
        [ -n "$LB" ] || abbruch "kein comdare_axis_version_lock-Binary gefunden. Gesucht:$_gesehen. \
Bauen: cmake --build <bau-baum> --target comdare_axis_version_lock (EXCLUDE_FROM_ALL), \
oder COMDARE_AXIS_LOCK_BIN setzen"
    fi
    echo "WERKZEUG: $LB"
    set +e
    "$LB" --check "$LOCKF" --root . > "$TMPD/lock.log" 2>&1
    _rc=$?
    set -e
    cat "$TMPD/lock.log"
    _n_rot=$(grep -c ' ROT ' "$TMPD/lock.log" 2>/dev/null || true)
    [ -n "$_n_rot" ] || _n_rot=0
    _n_bestand=$(grep -c ' BESTAND ' "$TMPD/lock.log" 2>/dev/null || true)
    [ -n "$_n_bestand" ] || _n_bestand=0
    echo "NENNER: Tool-Exit $_rc, $_n_rot ROT-Befund(e), $_n_bestand BESTAND-Zeile(n)."
    case "$_rc" in
        0) ;;
        3) gate_rot "[4/6] AXIS-VERSION-LOCK" \
             "Version-Bump ohne Lock-Regen (Tool-Exit 3): --write fahren und den Regen committen" ;;
        1) gate_rot "[4/6] AXIS-VERSION-LOCK" \
             "Digest-Drift ohne Version-Bump ($_n_rot Befunde oben): Version bumpen UND Lock-Regen committen" ;;
        2) abbruch "axis_version_lock meldet Exit 2 (fail-closed, kein Verdikt) -- Log oben" ;;
        *) abbruch "axis_version_lock endete mit unbekanntem Exit $_rc" ;;
    esac
    echo "GATE [4/6] GRUEN."
    echo ""
fi

# =============================================================================
# [5/6] FLOOR-SCANNER: ctest -N EXAKT gegen den Anker der Host-Klasse
# =============================================================================
if faehrt floor; then
    echo "============================================================================="
    echo " [5/6] FLOOR-SCANNER (ctest -N gegen scripts/ci_test_inventory_floor.txt, EXAKT)"
    echo "============================================================================="
    # H-1 (25.08.2026, Raeumung Q3): KEIN Default-Baum mehr. Der alte Default build/gcc-release war der Preset-
    # Pfad (CMakePresets.json binaryDir); am Objekt stand dort zuletzt ein ALT-Baum (2168f60c) neben dem echten
    # Floor-Gate-Baum build/a25-gcc-release (d3b5a393) -- ein Lauf ohne Variable haette den ALTEN gezaehlt und bei
    # 545 == 545 GRUEN gemeldet: ein richtiges Messgeraet am falschen Gegenstand (V6.6). Deshalb NENNT der
    # Aufrufer den Baum, und der Baum muss beweisbar der Gegenstand DIESES Pushs sein (drei Proben unten). Kein
    # Raten ("juengster Baum"): die Baum-Namen der Landungen (build-l1, build-b10, build/a25-...) folgen keinem Glob.
    BD="${COMDARE_PRE_PUSH_BUILD_DIR:-}"
    if [ -z "$BD" ]; then
        _kand=""
        for _c in build/*/CMakeCache.txt build-*/CMakeCache.txt; do
            [ -f "$_c" ] && _kand="$_kand ${_c%/CMakeCache.txt}"
        done
        abbruch "COMDARE_PRE_PUSH_BUILD_DIR ist nicht gesetzt -- es gibt KEINEN Default-Baum mehr (H-1, 25.08.2026). \
Den Bau-Baum des Floor-Gates ausdruecklich nennen: \
COMDARE_PRE_PUSH_BUILD_DIR=<baum> sh scripts/pre_push_lande_gates.sh ... \
-- konfigurierte Baeume in diesem Checkout (HINWEIS, nichts wird gewaehlt):${_kand:- keiner}"
    fi
    [ -d "$BD" ] || abbruch "Bau-Baum '$BD' fehlt (COMDARE_PRE_PUSH_BUILD_DIR zeigt ins Leere)"
    [ -f "$BD/CMakeCache.txt" ] || abbruch "'$BD/CMakeCache.txt' fehlt -- ohne Cache keine Host-Klasse"
    [ -f "$BD/CTestTestfile.cmake" ] || abbruch "'$BD/CTestTestfile.cmake' fehlt -- kein konfigurierter Test-Baum"
    # PROBE 1 (Gegenstand): der Baum wurde aus DIESEM Checkout konfiguriert. Der Baum eines anderen Worktrees
    # zaehlt DESSEN Registrierung -- am 25.08.2026 am Objekt: ein Worktree-Lauf gegen den Hauptklon-Baum meldete
    # 545 == 545 GRUEN, ohne dass eine Zeile des Worktrees je gezaehlt worden waere.
    _home=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$BD/CMakeCache.txt" | sed -n '1p')
    [ -n "$_home" ] || abbruch "CMAKE_HOME_DIRECTORY fehlt in '$BD/CMakeCache.txt' -- Herkunft des Baums unbestimmbar"
    _home_r=$(cd "$_home" 2>/dev/null && pwd -P) || abbruch "Quellpfad '$_home' des Baums '$BD' existiert nicht mehr"
    _root_r=$(pwd -P)
    [ "$_home_r" = "$_root_r" ] || abbruch "Bau-Baum '$BD' gehoert zu einem ANDEREN Checkout: konfiguriert aus \
$_home_r, Repo-Wurzel ist $_root_r -- eine fremde Registrierung ist kein Gegenstand dieses Pushs"
    # PROBE 2 (Frische): keine getrackte CMake-Datei ist juenger als die Registrierung des Baums. ctest -N liest
    # NUR den letzten Configure; Ninja nimmt dieselbe mtime-Regel fuer seinen Re-Configure. Juengere Datei => erst
    # 'cmake <baum>' fahren, dann das Gate -- sonst zaehlt es eine Registrierung, die die Spitze so nicht mehr hat.
    git ls-files -z -- 'CMakeLists.txt' '*/CMakeLists.txt' '*.cmake' > "$TMPD/cmake_dateien.z" \
        || abbruch "git ls-files (CMake-Dateien) scheitert -- Frische-Probe unmoeglich"
    [ -s "$TMPD/cmake_dateien.z" ] || abbruch "keine getrackten CMake-Dateien -- Frische-Probe unmoeglich"
    _juenger=$(xargs -0 -r sh -c 'find "$@" -newer "$0" -print 2>/dev/null' "$BD/CTestTestfile.cmake" \
        < "$TMPD/cmake_dateien.z" | head -5)
    [ -z "$_juenger" ] || abbruch "Registrierung in '$BD' ist AELTER als CMake-Dateien des Checkouts (max. 5 gezeigt): \
$(echo "$_juenger" | tr '\n' ' ')-- erst 'cmake $BD' (Re-Configure), dann das Gate"
    FLOORF="scripts/ci_test_inventory_floor.txt"
    [ -f "$FLOORF" ] || abbruch "$FLOORF fehlt"

    # Host-Klasse (Kurzform der Leiter aus ci_test_coverage_guard.sh: Wert 1 = ja,
    # leer = nein, ALLES ANDERE Abbruch; avx512f ohne avx2 beschreibt keine reale
    # Maschine und ist ebenfalls Abbruch, nicht die naechstbeste Annahme).
    _hk_wert() {
        sed -n "s/^$1:[^=]*=//p" "$BD/CMakeCache.txt" | sed -n '1p'
    }
    _v2=$(_hk_wert COMDARE_HOST_RUNS_AVX2)
    _v512=$(_hk_wert COMDARE_HOST_RUNS_AVX512F)
    case "$_v2" in
        1)  _j2=ja ;;
        '') _j2=nein ;;
        *)  abbruch "COMDARE_HOST_RUNS_AVX2='$_v2' -- erwartet 1 oder leer, nichts wird gerundet" ;;
    esac
    case "$_v512" in
        1)  _j512=ja ;;
        '') _j512=nein ;;
        *)  abbruch "COMDARE_HOST_RUNS_AVX512F='$_v512' -- erwartet 1 oder leer, nichts wird gerundet" ;;
    esac
    if [ "$_j2" = ja ] && [ "$_j512" = ja ]; then KLASSE=avx512f
    elif [ "$_j2" = ja ]; then KLASSE=avx2
    elif [ "$_j512" = nein ]; then KLASSE=basis
    else abbruch "avx512f ohne avx2 im Cache '$BD' -- keine reale Maschine, Cache unglaubwuerdig"
    fi

    # Anker der Klasse aus der Floor-Datei ('<klasse> <ganzzahl>'; # und leer erlaubt).
    ANKER=$(awk -v k="$KLASSE" '/^[[:space:]]*(#|$)/ { next } $1 == k { print $2; exit }' "$FLOORF")
    [ -n "$ANKER" ] || abbruch "keine Zeile fuer Klasse '$KLASSE' in $FLOORF"
    case "$ANKER" in *[!0-9]*) abbruch "Anker '$ANKER' fuer '$KLASSE' ist keine Ganzzahl" ;; esac

    # BEWEIS-SCHUTZ: ctest -N ueberschreibt Testing/Temporary/LastTest.log mit einem 121-Byte-Stub (am Objekt
    # 25.08.2026, cmake 4.3.4; RV-03) -- das Vollauf-Protokoll des Baums waere danach eine Falsch-Null. Sichern,
    # zaehlen, byteidentisch zurueck; gab es keins, bleibt auch keins (der Stub wird entfernt).
    _ltl="$BD/Testing/Temporary/LastTest.log"
    _ltl_da=0
    if [ -f "$_ltl" ]; then
        cp -p "$_ltl" "$TMPD/LastTest.log.vorher" || abbruch "LastTest.log nicht sicherbar -- Gate faehrt nicht"
        _ltl_da=1
    fi
    set +e
    ( cd "$BD" && ctest -N ) > "$TMPD/ctestn.log" 2>&1
    _rc=$?
    set -e
    if [ "$_ltl_da" -eq 1 ]; then
        cp -p "$TMPD/LastTest.log.vorher" "$_ltl" || abbruch "LastTest.log nicht wiederherstellbar: $_ltl"
    else
        rm -f "$_ltl"
    fi
    [ "$_rc" -eq 0 ] || abbruch "ctest -N in '$BD' scheitert (Exit $_rc): $(tail -2 "$TMPD/ctestn.log" | tr '\n' ' ')"
    IST=$(sed -n 's/^Total Tests: \([0-9][0-9]*\)$/\1/p' "$TMPD/ctestn.log" | sed -n '1p')
    [ -n "$IST" ] || abbruch "'Total Tests: N' nicht in der ctest-Ausgabe -- Inventur unlesbar"
    [ "$IST" -gt 0 ] || abbruch "ctest-Inventur 0 -- leerer Bau-Baum ist kein gruener Bau-Baum"

    echo "NENNER: Inventur $IST (ctest -N, $BD, konfiguriert aus $_home_r)"
    echo "        gegen Anker $ANKER (Klasse $KLASSE, $FLOORF)."
    if [ "$IST" -lt "$ANKER" ]; then
        gate_rot "[5/6] FLOOR-SCANNER" "Inventur $IST UNTER Anker $ANKER -- $((ANKER - IST)) Test(s) verschwunden"
    elif [ "$IST" -gt "$ANKER" ]; then
        gate_rot "[5/6] FLOOR-SCANNER" \
            "Inventur $IST UEBER Anker $ANKER -- Floor-Nachzug fehlt (#39: alle drei Sprossen im SELBEN Change)"
    fi
    echo "GATE [5/6] GRUEN: $IST == $ANKER (exakt)."
    echo ""
fi

# =============================================================================
# [6/6] TABU-CRC-PROBE: Anker + sha256-SOLLs + Bereichs-Unbeweglichkeit
# =============================================================================
if faehrt tabu; then
    echo "============================================================================="
    echo " [6/6] TABU-CRC-PROBE (golden_fullpilot_320* + permutation_axes.xml + CRC-Anker)"
    echo "============================================================================="
    # SOLLs eingefroren 19.08.2026 (A2.5-Fixstrecke-2, identisch ueber G1..G6 gemessen).
    # Ein legitimer Re-Anker (B-10.3, golden-Fenster) aendert diese Liste UND den
    # Code-Anker in EINEM bewussten Commit -- alles andere ist ROT und wird NIE
    # durch Regenerieren "geheilt" (Meldung an den Lead, Stopp).
    ANKER_HEX="0x56F1B721C72DC10EULL"
    ANKER_DATEI="libs/cache_engine/profile_facade/source_catalog.hpp"
    # Format: ZEILENPAARE (Pfad, dann sha256) -- kein 'hash  pfad' in EINER Zeile,
    # damit dieses Skript selbst unter der 120-Byte-Diff-Hygiene bleibt.
    TABU_LISTE="$TMPD/tabu_soll.txt"
    cat > "$TABU_LISTE" <<'SOLL'
libs/cache_engine/algorithm_profiles/permutation_axes.xml
cfa3b020e750e2e4cd7b08733c7a235d9a882387419811c8ce8fcbfecdb030e8
tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt
65e35415323f1f73439ec698ce9b4f0fbae26e7206ee2b08f9864c5449eb410a
tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids_abi4.txt
665db1661eba5db16050454a9f5178fc9fcc794aa6b8efb8257124bfd8bc5fd9
tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids_abi5.txt
3e11f402df381e10da8e46690bd81ac4d3649973c0c0da1fcdaedf1163a92d4c
tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids_abi6.txt
82ce616884358d2d12e3af2a8ff82b7138815723e5e06b76a8c01a6803babb0f
SOLL

    # (a) der CRC-Anker im Code: GENAU EINMAL.
    [ -f "$ANKER_DATEI" ] || abbruch "$ANKER_DATEI fehlt"
    _n_anker=$(grep -c "$ANKER_HEX" "$ANKER_DATEI" || true)
    [ -n "$_n_anker" ] || _n_anker=0
    if [ "$_n_anker" -ne 1 ]; then
        gate_rot "[6/6] TABU-CRC-PROBE" \
            "CRC-Anker $ANKER_HEX steht ${_n_anker}x in $ANKER_DATEI (SOLL: genau 1x) -- STOPP, Lead melden"
    fi

    # (b) sha256-SOLLs, byteweise. Fehlende Datei ist ABBRUCH (nicht pruefbar),
    # abweichender Hash ist ROT (bewegt).
    _t_rot=0
    while IFS= read -r _pfad && IFS= read -r _soll; do
        [ -f "$_pfad" ] || abbruch "TABU-Datei fehlt: $_pfad -- nicht pruefbar ist nicht gruen"
        _ist=$(sha256sum "$_pfad") || abbruch "sha256sum scheitert an $_pfad"
        _ist=${_ist%% *}
        if [ "$_ist" != "$_soll" ]; then
            echo "  TABU-BEWEGT  $_pfad"
            echo "    soll $_soll"
            echo "    ist  $_ist"
            _t_rot=1
        fi
    done < "$TABU_LISTE"
    [ "$_t_rot" -eq 0 ] || gate_rot "[6/6] TABU-CRC-PROBE" \
        "TABU-Datei(en) bewegt (oben) -- STOPP, NIE regenerieren, Lead melden (Re-Anker nur via B-10.3)"

    # (c) der Bereich bewegt keine TABU-Datei (auch nicht hin und zurueck).
    _t_diff=$(git diff --name-only "$BASIS..$SPITZE" -- \
        libs/cache_engine/algorithm_profiles/permutation_axes.xml \
        "tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids*.txt" | grep -c . || true)
    [ -n "$_t_diff" ] || _t_diff=0
    _t_log=$(git log --oneline "$BASIS..$SPITZE" -- \
        libs/cache_engine/algorithm_profiles/permutation_axes.xml \
        "tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids*.txt" | grep -c . || true)
    [ -n "$_t_log" ] || _t_log=0
    if [ "$_t_diff" -ne 0 ] || [ "$_t_log" -ne 0 ]; then
        git log --oneline "$BASIS..$SPITZE" -- \
            libs/cache_engine/algorithm_profiles/permutation_axes.xml \
            "tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids*.txt" | sed 's/^/  /'
        gate_rot "[6/6] TABU-CRC-PROBE" \
            "der Bereich beruehrt TABU-Dateien ($_t_diff im End-Diff, $_t_log Commits) -- STOPP, Lead melden"
    fi
    echo "GATE [6/6] GRUEN. NENNER: 5 Dateien byteidentisch, Anker 1x in $ANKER_DATEI,"
    echo "  0 TABU-Beruehrungen in $ANZ_COMMITS Commits ($BASIS..$SPITZE)."
    echo ""
fi

# =============================================================================
# Verdikt
# =============================================================================
echo "============================================================================="
if [ -n "$NUR" ]; then
    echo "PRE-PUSH-LANDE-GATES (--nur=${NUR}): GRUEN -- NUR dieses eine Gate."
    echo "NICHT GEFAHREN (und damit OHNE Urteil):"
    for _g in diff format gitleaks lock floor tabu; do
        [ "$_g" = "$NUR" ] || echo "   * $_g"
    done
    echo "Der volle Lauf vor dem Push bleibt Pflicht."
else
    echo "PRE-PUSH-LANDE-GATES: GRUEN -- alle sechs Gates ueber diesem Baum."
    echo "KEINE Aussage ueber: Bau/ctest-Lauf, cppcheck, Abdeckungs-Wache (s. GRENZEN im Kopf)."
fi
echo "============================================================================="
exit 0
