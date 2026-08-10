#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  BAUWEG-WACHE -- jedes Binary, das ein Test AUFRUFT, muss im Bauweg haengen.
#  D1b/D1e, GOAL v8 Teil VIII (T-7: Registrierung IST Teil des Tests)
# =============================================================================
#
# DIE LUECKE, DIE SIE SCHLIESST. T-7 hat ZWEI Haelften:
#   (1) "ein Test existiert erst, wenn er in `ctest -N` erscheint"   -> gedeckt von
#       scripts/ci_test_sichtbarkeit_wache.sh (W-1).
#   (2) "UND sein Binary im Bauweg haengt"                           -> war UNGEDECKT.
# Fuer Haelfte (2) gab es bis heute kein Werkzeug. Die Abdeckungs-Wache rechnet
# Job-Auswahlen gegen `ctest -N`, die Sichtbarkeits-Wache Quelltext gegen `ctest -N`.
# BEIDE sind gruen, wenn ein Test sauber registriert ist und sein Binary trotzdem an
# keinem Sammel-Target haengt -- der Test faellt dann erst im Job auf, als "Not Run"
# oder als Treiber-Abbruch "Binary nicht gebaut".
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST (D1e, 2026-08-09): f15_compare_cli_smoke rief
# comdare_f15_compare_cli auf; das Ziel stand in KEINEM Sammel-Target. Geheilt wurde mit
# EINER Zeile (set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS ...)). Eine Zeile,
# die jemand beim naechsten rohen add_test() wieder vergisst -- die Heilung deckt den
# heutigen Baum, nicht den naechsten Test. GOAL v8 Teil VII: "Zu jeder Abnahme gehoert die
# Antwort auf 'was erzwingt das Halten?'. Zulaessig sind ein WERKZEUG oder eine
# ausdruecklich als ungedeckt benannte Stelle." Das hier ist das Werkzeug.
#
# WARUM DAS NICHT DURCH 'make' ERLEDIGT IST. Seit der GNU-Bauweg-Umstellung (08.08.2026)
# baut die CI ueber `make` (= ninja 'all'). 'all' ist die Obermenge und deckt den Normalfall.
# Es bleiben aber ZWEI Wege, auf denen nur comdare_tests gebaut wird:
#   * die Sanitizer-Jobs bauen benannte Ziele, nicht 'all';
#   * super zieht die ce per add_subdirectory(... EXCLUDE_FROM_ALL) ein -- damit ist ce's
#     'all' im super-Sub-Build WIRKUNGSLOS, und comdare_tests ist der einzige Griff.
# Ein Binary, das nur in 'all' haengt, ist deshalb nicht gedeckt, sondern nur nicht
# aufgefallen. Die Wache trennt beide Faelle und benennt sie einzeln.
#
# ZWEI QUELLEN, KEINE SELBST-INVENTUR (V-7). Verglichen werden zwei getrennt erzeugte
# Artefakte desselben Configure-Laufs:
#   MENGE A  build/**/CTestTestfile.cmake  -- was ctest AUFRUFT (CTest-Registry)
#   MENGE B  `ninja -t inputs comdare_tests` -- was der Bauweg HERSTELLT (Ninja-Graph)
# Beide stammen aus CMake, entstehen aber ueber verschiedene Generatoren und sind
# unabhaengig voneinander kaputt zu bekommen -- genau der D1e-Fall war A ohne B.
#
# WARUM CTestTestfile.cmake UND NICHT `ctest --show-only=json-v1` (Messgeraet-Befund
# 2026-08-09, ctest 4.3.4): die JSON-Ausgabe LAESST DAS FELD "command" WEG, wenn die
# Binary noch nicht gebaut ist. Am Objekt gezaehlt in einem frisch konfigurierten Baum:
# 453 von 460 Tests ohne "command" -- 98 Prozent der Kommandozeilen unsichtbar, ohne
# Fehlermeldung. Eine Wache auf dieser Quelle waere in genau dem Zustand blind, fuer den
# sie gebaut ist. CTestTestfile.cmake traegt die Kommandozeile immer.
#
# WAS SIE PRUEFT, je referenziertem Pfad UNTERHALB des Bau-Verzeichnisses:
#   kein Bauprodukt (ninja kennt keine Regel dafuer) .... Daten-/Ausgabepfad, gezaehlt
#   Bauprodukt UND in comdare_tests ..................... OK
#   Bauprodukt, NICHT in comdare_tests, aber in 'all' ... BEFUND "nur-ALL"
#   Bauprodukt, weder comdare_tests noch 'all' .......... BEFUND "nirgends"
#
# GRENZE, EHRLICH BENANNT -- was diese Wache NICHT deckt:
#   (i)  Kommandos AUSSERHALB des Bau-Verzeichnisses (z.B. /usr/bin/cmake) werden nicht
#        verfolgt. Darunter liegen die drei Negativ-Compile-Fixturen, die per
#        `cmake --build --target X` ein EXCLUDE_FROM_ALL-Ziel bauen, das bewusst an keinem
#        Sammel-Target haengt. Ihre Zahl steht bei jedem Lauf in der Ausgabe.
#   (ii) Tests, die in DIESEM Baum gar nicht registriert sind, koennen nicht geprueft
#        werden. Das ist der gefaehrliche Zustand, weil die Wache dann gruen meldet, ohne
#        die Sache geprueft zu haben. Deshalb liest sie das Registrierungs-Protokoll
#        (comdare_registrierungs_protokoll.txt) als DRITTE Quelle und weist jeden
#        UEBERSPRUNGENen Block NAMENTLICH aus. Ein Baum ohne `make inventar` prueft
#        weniger -- und sagt das dann auch.
#
# AUFRUF:  sh scripts/ci_test_bauweg_wache.sh <build-verzeichnis>
# EXIT:    0 = jedes aufgerufene Bauprodukt haengt an comdare_tests oder ist begruendet
#          1 = mindestens ein Bauprodukt haengt nicht dort UND ist nicht begruendet
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen)
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

BUILD="${1:-}"
if [ -z "$BUILD" ]; then
    echo "AUFRUF: sh scripts/ci_test_bauweg_wache.sh <build-verzeichnis>" >&2
    exit 2
fi
if [ ! -d "$BUILD" ]; then
    echo "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
    exit 2
fi

WURZEL=$(git rev-parse --show-toplevel 2>/dev/null) || {
    echo "ABBRUCH: kein git-Arbeitsbaum -- die Repo-Wurzel ist nicht bestimmbar." >&2
    exit 2
}
# ABSOLUT, bevor wir das Arbeitsverzeichnis wechseln: CTestTestfile.cmake traegt absolute
# Pfade, und ein relatives $BUILD zeigt nach dem `cd` woandershin.
BUILD_ABS=$(cd "$BUILD" && pwd) || exit 2
cd "$WURZEL"

command -v ninja >/dev/null 2>&1 || { echo "ABBRUCH: ninja nicht im PATH." >&2; exit 2; }
if [ ! -f "$BUILD_ABS/build.ninja" ]; then
    echo "ABBRUCH: '$BUILD_ABS/build.ninja' fehlt -- dieser Baum ist nicht mit -G Ninja" >&2
    echo "         konfiguriert. Der Bauweg ist damit nicht auslesbar (fail-closed)." >&2
    exit 2
fi

ALLOW="scripts/ci_test_bauweg_allowlist.txt"
PROTOKOLL="$BUILD_ABS/comdare_registrierungs_protokoll.txt"
TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

echo "============================================================================="
echo " BAUWEG-WACHE   Bau-Baum: $BUILD_ABS"
echo "============================================================================="

# -- MENGE A: was ctest aufruft ----------------------------------------------
# JEDES quotierte Argument, nicht nur das erste. Bei f15_compare_cli_smoke reist die
# gepruefte Binary als "-DCLI=<pfad>" -- das erste Argument ist dort /usr/bin/cmake.
# Eine Wache, die nur COMMAND[0] liest, waere am D1e-Fall selbst vorbeigelaufen.
find "$BUILD_ABS" -name CTestTestfile.cmake -type f > "$TMP/testfiles.txt" 2>/dev/null || :
N_TESTFILES=$(awk 'END{print NR+0}' "$TMP/testfiles.txt")
[ "$N_TESTFILES" -gt 0 ] || {
    echo "ABBRUCH: 0 CTestTestfile.cmake unter '$BUILD_ABS' -- Baum nicht konfiguriert." >&2
    exit 2
}

: > "$TMP/addtest.txt"
while IFS= read -r F; do
    [ -f "$F" ] || continue
    grep -h '^add_test(' "$F" >> "$TMP/addtest.txt" 2>/dev/null || :
done < "$TMP/testfiles.txt"
N_ADDTEST=$(awk 'END{print NR+0}' "$TMP/addtest.txt")
[ "$N_ADDTEST" -gt 0 ] || {
    echo "ABBRUCH: 0 add_test-Zeilen gefunden -- ohne -DCOMDARE_BUILD_TESTS=ON konfiguriert?" >&2
    exit 2
}

# awk statt Shell-Schleife: `print` beendet jede Zeile mit ORS. Ein sed-Anhang ohne
# abschliessenden Newline verkettet sonst zwei Pfade lautlos zu einem dritten.
awk -v BD="$BUILD_ABS/" -v UNLESBAR="$TMP/unlesbar.txt" '
function entziffern(tok, name,   p, roh) {
    # Ein Token kann den Pfad EINGEBETTET tragen ("-DCLI=/pfad/x") und eine
    # CMake-Liste sein ("a;b;c"). Beides wird hier aufgeloest.
    while ((p = index(tok, BD)) > 0) {
        tok = substr(tok, p + length(BD))
        roh = tok
        sub(/;.*$/, "", roh)
        # "/./" ist derselbe Pfad wie "/" -- ninja fuehrt ihn normalisiert. Ohne diese
        # Normalisierung gilt ein bekanntes Bauprodukt faelschlich als Datenpfad.
        while (sub(/\/\.\//, "/", roh)) { }
        if (roh != "") { print name "\t" roh }
        sub(/^[^;]*;?/, "", tok)
        if (tok == "") { break }
    }
}
{
    name = ""
    if (match($0, /^add_test\(\[=\[/)) {
        rest = substr($0, RLENGTH + 1)
        q = index(rest, "]=]")
        if (q > 1) { name = substr(rest, 1, q - 1) }
    }
    if (name == "") {
        print FILENAME ": " substr($0, 1, 80) >> UNLESBAR
        next
    }
    n = split($0, teile, "\"")
    for (i = 2; i <= n; i += 2) { entziffern(teile[i], name) }
}
' "$TMP/addtest.txt" | LC_ALL=C sort -u > "$TMP/a_paare.txt"

N_UNLESBAR=0
[ -f "$TMP/unlesbar.txt" ] && N_UNLESBAR=$(awk 'END{print NR+0}' "$TMP/unlesbar.txt")
cut -f2 "$TMP/a_paare.txt" | LC_ALL=C sort -u > "$TMP/a_pfade.txt"
N_A=$(awk 'END{print NR+0}' "$TMP/a_pfade.txt")
[ "$N_A" -gt 0 ] || {
    echo "ABBRUCH: 0 Bau-Verzeichnis-Pfade in den add_test-Kommandos -- die Suche griff nicht." >&2
    exit 2
}

# -- MENGE B: was der Bauweg herstellt ---------------------------------------
ninja -C "$BUILD_ABS" -t inputs comdare_tests > "$TMP/b_roh.txt" 2>/dev/null || {
    echo "ABBRUCH: 'ninja -t inputs comdare_tests' schlug fehl -- gibt es das Sammel-Target?" >&2
    exit 2
}
LC_ALL=C sort -u "$TMP/b_roh.txt" > "$TMP/b.txt"
N_B=$(awk 'END{print NR+0}' "$TMP/b.txt")
[ "$N_B" -gt 0 ] || { echo "ABBRUCH: comdare_tests hat 0 Eingaenge -- das kann nicht stimmen." >&2; exit 2; }

ninja -C "$BUILD_ABS" -t inputs all > "$TMP/all_roh.txt" 2>/dev/null || : > "$TMP/all_roh.txt"
LC_ALL=C sort -u "$TMP/all_roh.txt" > "$TMP/all.txt"
N_ALL=$(awk 'END{print NR+0}' "$TMP/all.txt")

# Was ninja ueberhaupt herstellen KANN. Alles andere ist Daten-/Ausgabepfad.
ninja -C "$BUILD_ABS" -t targets all 2>/dev/null | sed 's/:.*$//' \
    | LC_ALL=C sort -u > "$TMP/bekannt.txt"
N_BEKANNT=$(awk 'END{print NR+0}' "$TMP/bekannt.txt")
[ "$N_BEKANNT" -gt 0 ] || { echo "ABBRUCH: 'ninja -t targets all' lieferte 0 Ziele." >&2; exit 2; }

# -- Einordnung --------------------------------------------------------------
LC_ALL=C comm -12 "$TMP/a_pfade.txt" "$TMP/bekannt.txt" > "$TMP/bauprodukte.txt"
LC_ALL=C comm -23 "$TMP/a_pfade.txt" "$TMP/bekannt.txt" > "$TMP/kein_bauprodukt.txt"
N_BAUPRODUKT=$(awk 'END{print NR+0}' "$TMP/bauprodukte.txt")
N_DATEN=$(awk 'END{print NR+0}' "$TMP/kein_bauprodukt.txt")
# NULL GEPRUEFTE BAUPRODUKTE IST KEIN GRUEN. Ein Baum, in dem kein einziger Test ein
# Bauprodukt aufruft, existiert nicht -- wohl aber ein kaputter Abgleich (falsches
# Bau-Verzeichnis, verschobene Pfad-Normalisierung). Ohne diese Klammer meldete die Wache
# genau dann OK, wenn sie nichts mehr sieht.
[ "$N_BAUPRODUKT" -gt 0 ] || {
    echo "ABBRUCH: 0 der $N_A referenzierten Pfade sind Bauprodukte -- der Abgleich griff" >&2
    echo "         nicht (fail-closed). Zeigt '$BUILD_ABS' wirklich auf diesen Baum?" >&2
    exit 2
}

LC_ALL=C comm -23 "$TMP/bauprodukte.txt" "$TMP/b.txt" > "$TMP/nicht_in_tests.txt"
N_NICHT=$(awk 'END{print NR+0}' "$TMP/nicht_in_tests.txt")

# -- Allowlist ---------------------------------------------------------------
# Format je Zeile: <pfad><Leerzeichen oder TAB><Begruendung>. '#' ist Kommentar.
# Eine Zeile OHNE Begruendung ist selbst ein Fehler -- sonst waere die Allowlist der
# stille Rueckfall, gegen den die Wache gebaut ist.
: > "$TMP/erlaubt.txt"
: > "$TMP/ohne_grund.txt"
if [ -f "$ALLOW" ]; then
    while IFS= read -r Z; do
        case "$Z" in ''|\#*) continue ;; esac
        P=$(printf '%s' "$Z" | awk '{print $1}')
        # awk trennt an Leerzeichen UND Tabulator -- `cut -d' '` gibt eine Zeile ohne
        # Leerzeichen VOLLSTAENDIG zurueck und liesse "pfad<TAB>" als begruendet gelten.
        G=$(printf '%s' "$Z" | awk '{ $1 = ""; sub(/^[[:space:]]+/, ""); print }')
        [ -n "$P" ] || continue
        if [ -z "$G" ] || [ "$G" = "$P" ]; then
            echo "$P" >> "$TMP/ohne_grund.txt"
        else
            echo "$P" >> "$TMP/erlaubt.txt"
        fi
    done < "$ALLOW"
fi
N_ALLOW=$(awk 'END{print NR+0}' "$TMP/erlaubt.txt")
N_OHNE_GRUND=$(awk 'END{print NR+0}' "$TMP/ohne_grund.txt")

: > "$TMP/befund.txt"
: > "$TMP/gedeckt.txt"
while IFS= read -r P; do
    [ -n "$P" ] || continue
    if grep -Fxq -- "$P" "$TMP/erlaubt.txt" 2>/dev/null; then
        echo "$P" >> "$TMP/gedeckt.txt"
    else
        echo "$P" >> "$TMP/befund.txt"
    fi
done < "$TMP/nicht_in_tests.txt"
N_BEFUND=$(awk 'END{print NR+0}' "$TMP/befund.txt")
N_GEDECKT=$(awk 'END{print NR+0}' "$TMP/gedeckt.txt")

# Allowlist-Zeilen, die in diesem Baum nichts mehr decken: die Ursache wurde geheilt und
# die Ausnahme steht noch. Bericht, kein Fehler -- aber sie gehoert dann geloescht.
: > "$TMP/verwaist.txt"
while IFS= read -r P; do
    [ -n "$P" ] || continue
    grep -Fxq -- "$P" "$TMP/nicht_in_tests.txt" 2>/dev/null || echo "$P" >> "$TMP/verwaist.txt"
done < "$TMP/erlaubt.txt"
N_VERWAIST=$(awk 'END{print NR+0}' "$TMP/verwaist.txt")

# -- DRITTE QUELLE: was dieser Baum gar nicht erst registriert hat ------------
# ANKER '^BLOCK|' IST PFLICHT, NICHT KOSMETIK (Befund beim ersten Lauf dieser Wache):
# ohne ihn zaehlt die KOPFZEILE des Protokolls mit -- sie erklaert das Format und enthaelt
# das Wort UEBERSPRUNGEN als Feldnamen. Die Wache meldete 3 uebersprungene Bloecke und
# listete 2. Zwei Zahlen derselben Wache, die einander widersprechen, sind schlimmer als
# gar keine Zahl: sie sehen nach Messung aus.
N_UEBERSPRUNGEN=0
if [ -f "$PROTOKOLL" ]; then
    N_UEBERSPRUNGEN=$(awk '/^BLOCK\|/ && /\|UEBERSPRUNGEN\|/ { n++ } END { print n+0 }' "$PROTOKOLL")
fi

# -- Bericht -----------------------------------------------------------------
if [ "$N_BEFUND" -gt 0 ]; then
    echo ""
    echo "BEFUND -- aufgerufen, aber NICHT im Sammel-Target comdare_tests:"
    while IFS= read -r P; do
        [ -n "$P" ] || continue
        if grep -Fxq -- "$P" "$TMP/all.txt" 2>/dev/null; then
            WO="haengt nur an 'all' -- faellt aus, wo nur comdare_tests gebaut wird"
        else
            WO="haengt WEDER an comdare_tests NOCH an 'all' -- wird nirgends gebaut"
        fi
        echo "  $P"
        echo "      $WO"
        grep -F "	$P" "$TMP/a_paare.txt" 2>/dev/null | cut -f1 | sort -u | while IFS= read -r T; do
            echo "      aufgerufen von Test: $T"
        done
    done < "$TMP/befund.txt"
fi

if [ "$N_GEDECKT" -gt 0 ]; then
    echo ""
    echo "BEGRUENDET AUSGENOMMEN (steht bei JEDEM Lauf hier, damit es nicht verschwindet):"
    while IFS= read -r P; do
        [ -n "$P" ] || continue
        G=$(grep -F -- "$P" "$ALLOW" 2>/dev/null | head -1 \
            | awk '{ $1 = ""; sub(/^[[:space:]]+/, ""); print }')
        echo "  $P -- $G"
    done < "$TMP/gedeckt.txt"
fi

if [ "$N_VERWAIST" -gt 0 ]; then
    echo ""
    echo "ALLOWLIST-EINTRAEGE OHNE WIRKUNG IN DIESEM BAUM (Bericht, kein Fehler):"
    echo "  (Die Ursache ist geheilt. Greift die Zeile nirgends mehr, gehoert sie"
    echo "   ersatzlos entfernt statt auf Vorrat behalten.)"
    cat "$TMP/verwaist.txt"
fi

if [ "$N_DATEN" -gt 0 ]; then
    echo ""
    echo "NICHT GEPRUEFT -- referenziert, aber kein Bauprodukt (ninja kennt keine Regel):"
    echo "  (Daten-, Ausgabe- und Arbeitsverzeichnisse. Eine benannte Grenze ist eine"
    echo "   Grenze; eine bloss gezaehlte waere ein Stellvertreter fuer Sicherheit.)"
    sed 's/^/  /' "$TMP/kein_bauprodukt.txt"
fi

if [ "$N_UEBERSPRUNGEN" -gt 0 ]; then
    echo ""
    echo "NICHT VON DIESEM LAUF GEPRUEFT -- Bloecke, die dieser Baum uebersprungen hat:"
    echo "  (Diese Tests sind hier nicht registriert; ueber ihre Binaries sagt dieser Lauf"
    echo "   NICHTS. Voll geprueft wird nur ein Baum nach 'make inventar'.)"
    sed -n 's/^BLOCK|\([^|]*\)|UEBERSPRUNGEN|\([^|]*\)|.*$/  \1 -> \2/p' "$PROTOKOLL"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $N_TESTFILES CTestTestfile.cmake gelesen, darin $N_ADDTEST add_test-Zeile(n)."
echo "  $N_A eindeutige(r) Pfad(e) unterhalb des Bau-Verzeichnisses referenziert."
echo "  davon $N_BAUPRODUKT Bauprodukt(e) und $N_DATEN Daten-/Ausgabepfad(e) (ninja kennt"
echo "  fuer sie keine Regel -- VON DIESER WACHE NICHT GEPRUEFT)."
echo "  comdare_tests stellt $N_B Pfad(e) her, 'all' $N_ALL."
echo "  $N_NICHT Bauprodukt(e) nicht in comdare_tests, davon $N_GEDECKT begruendet"
echo "  und $N_BEFUND OHNE Begruendung."
echo "  Allowlist: $N_ALLOW Eintrag/Eintraege mit Begruendung, davon $N_VERWAIST wirkungslos."
echo "  $N_UNLESBAR add_test-Zeile(n) ohne entzifferbaren Namen."
echo "  $N_UEBERSPRUNGEN uebersprungene(r) Registrierungs-Block/Bloecke in diesem Baum."
echo "-----------------------------------------------------------------------------"

if [ "$N_UNLESBAR" -gt 0 ]; then
    echo "FEHLER: $N_UNLESBAR add_test-Zeile(n) konnten nicht entziffert werden -- die Wache" >&2
    echo "        haette sie stillschweigend uebersprungen. Das ist fail-closed." >&2
    cat "$TMP/unlesbar.txt" >&2
    exit 2
fi

if [ "$N_OHNE_GRUND" -gt 0 ]; then
    echo "FEHLER: $N_OHNE_GRUND Allowlist-Zeile(n) ohne Begruendung -- eine Allowlist ohne" >&2
    echo "        Begruendung ist der stille Rueckfall, gegen den diese Wache gebaut ist." >&2
    exit 1
fi

if [ "$N_BEFUND" -gt 0 ]; then
    echo "FEHLER: $N_BEFUND von $N_BAUPRODUKT aufgerufenen Bauprodukt(en) haengen nicht an" >&2
    echo "        comdare_tests. Heilung ist EINE Zeile neben dem add_test():" >&2
    echo "          set_property(GLOBAL APPEND PROPERTY COMDARE_TEST_TARGETS <ziel>)" >&2
    echo "        Oder die Stelle gehoert mit Begruendung in $ALLOW." >&2
    exit 1
fi

echo "BAUWEG-WACHE: OK ($N_BAUPRODUKT aufgerufene Bauprodukte, $N_BEFUND unbegruendet ausserhalb)."
exit 0

# -----------------------------------------------------------------------------
# ABLOESUNGS-VERMERK (Lead, 10.08.2026) -- diese Datei ist eine WAISE auf Zeit.
#
# Owner-KERN vom 09.08.2026 14:34 (Rohtranskript Zeile 25562, promptSource=typed):
#   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das?
#    Es waere sauberer im cmake-Debug Modus standard google Tests zu fahren und
#    diese in Release zu wiederholen aufgrund von compile regressionen.
#    SKRIPTE SAGEN GAR NICHTS. ... die C++ Implementierung dazu."
#
# Diese Wache entstand in derselben Nacht, BEVOR der Lead das Owner-Wort im
# Transkript gefunden hatte. Sie schliesst eine echte, am Objekt belegte Luecke
# (T-7 zweite Haelfte: ein Test existiert erst, wenn sein Binary im Bauweg haengt)
# und ist verifiziert -- deshalb wird sie gelandet und nicht verworfen.
#
# ABER SIE IST DER ZIELFORM NACH FALSCH. Ihr Umbau nach C++/GoogleTest ist als
# Posten gefuehrt; das Vorbild steht im Haus: Code/ci_wachen/ mit Testwerkbank,
# und ci/tests/xml_wellformed_probe.sh traegt seit dem 09.08. denselben Vermerk.
#
# WER DIESE DATEI ANFASST, baut sie um -- er erweitert sie nicht.
# -----------------------------------------------------------------------------
