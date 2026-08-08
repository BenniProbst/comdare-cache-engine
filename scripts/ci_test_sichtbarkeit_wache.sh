#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  TEST-SICHTBARKEITS-WACHE -- kein Test darf UNBEMERKT verschwinden. (2026-08-08)
#  W-1 / ST-CTestWache, GOAL v8 Teil VIII (T-7: Registrierung IST Teil des Tests)
# =============================================================================
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST (am Objekt gemessen, 08.08.2026):
# enable_testing() stand in der Wurzel-CMakeLists.txt bei Zeile 717, also NACH
# add_subdirectory(libs/cache_engine) bei :674. CMake verarbeitet Verzeichnisse in
# Reihenfolge -- ein add_test/gtest_discover_tests in einem Unterbaum, der VOR
# enable_testing() betreten wird, erzeugt keinen ctest-Eintrag. Lautlos, ohne Warnung.
#
# Betroffen war genau eine Stelle:
#   libs/cache_engine/builder/commands/tests/CMakeLists.txt:36-40
#     -> test_commands (20 Faelle) + test_engine_adapters (7 Faelle) = 27
# `ctest -N` kannte davon 0. Darunter die Welch-t-Test-Faelle, auf denen die
# Signifikanz-Aussage der Diplomarbeit steht.
# Nach dem Vorziehen von enable_testing(): 429 -> 456 registrierte Tests.
#
# WARUM EINE WACHE UND NICHT NUR DIE HEILUNG:
# Die Heilung deckt den heutigen Baum. Sie deckt NICHT den naechsten Unterbaum, den
# jemand hinzufuegt, und nicht den naechsten Test hinter einer neuen if()-Bedingung.
# GOAL v8 Teil VII: "Zu jeder Abnahme gehoert die Antwort auf 'was erzwingt das
# Halten?'. Zulaessig sind ein WERKZEUG oder eine ausdruecklich als ungedeckt
# benannte Stelle." Das hier ist das Werkzeug.
#
# WAS SIE PRUEFT -- und was ausdruecklich NICHT:
# Sie vergleicht SOLL (Quelltext-Scan ueber alle CMakeLists.txt) gegen IST
# (`ctest -N` im gebauten Baum) und nennt die Differenz NAMENTLICH.
# Sie verlangt NICHT, dass jede Registrierung sichtbar ist -- das waere unerfuellbar:
# ein Test hinter `if(COMDARE_HOST_RUNS_AVX512F)` ist auf einer Maschine ohne AVX-512
# LEGITIM abwesend. Am Objekt gezaehlt: 18 Registrierungen liegen in if()-Bloecken
# unter 14 verschiedenen Bedingungen.
# Sie verlangt RECHENSCHAFT: jede unsichtbare Registrierung muss entweder in der
# Allowlist stehen (mit Begruendung) oder sie ist ein Befund. Der Unterschied ist der
# zwischen "alle Tests muessen laufen" (falsch, unerfuellbar) und "kein Test darf
# UNBEMERKT verschwinden" (richtig, baubar).
#
# AUFRUF:  sh scripts/ci_test_sichtbarkeit_wache.sh <build-verzeichnis>
# EXIT:    0 = jede Registrierung ist sichtbar oder begruendet abwesend
#          1 = mindestens eine unsichtbare Registrierung OHNE Begruendung
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen)
#
# ZAEHLWEISE, ausdruecklich benannt: gezaehlt werden die REGISTRIERUNGS-AUFRUFE im
# Quelltext, nicht die gtest-Faelle. Ein gtest_discover_tests(X) ist EINE
# Registrierung, die nach dem Bau in N ctest-Eintraege expandiert. Deshalb ist die
# ctest-Gesamtzahl KEIN gueltiger Vergleichswert -- verglichen wird je NAME.
#
# GRENZE, EHRLICH BENANNT -- was diese Wache NICHT deckt:
# Sie vergleicht NAMEN. Steht der Name nicht im Quelltext, sondern in einer CMake-
# Variablen (`add_test(NAME ${_dt} COMMAND ${_dt})`, typisch in einer foreach-Schleife
# ueber eine Ziel-Liste), kann sie ihn statisch nicht aufloesen. Am Objekt gezaehlt
# (08.08.2026): 19 solche Aufrufe -- 18 echte Aufrufstellen in tests/unit/CMakeLists.txt
# und einer im RUMPF der Hausfunktion comdare_add_test (cmake/gtest_setup.cmake:83),
# dessen Aufrufer selbst literale Namen tragen und damit gedeckt SIND. Der Rumpf wird
# bewusst mitgezaehlt: eine Luecke zu gross auszuweisen ist die sichere Richtung.
# Bricht eine dieser Schleifen, bleibt diese Wache GRUEN -- das ist kein Versehen, sondern die
# Grenze des Verfahrens, und sie steht deshalb bei JEDEM Lauf als eigene Zahl in der
# Ausgabe. Eine Luecke mit Zahl ist eine benannte Luecke; eine ohne waere ein
# Stellvertreter fuer Sicherheit. Wer sie schliessen will, braucht eine andere SOLL-
# Quelle als den Quelltext (z.B. einen cmake --trace-Lauf), nicht eine schaerfere Regex.
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

BUILD="${1:-}"
if [ -z "$BUILD" ]; then
    echo "AUFRUF: sh scripts/ci_test_sichtbarkeit_wache.sh <build-verzeichnis>" >&2
    exit 2
fi
if [ ! -d "$BUILD" ]; then
    echo "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
    exit 2
fi

WURZEL=$(git rev-parse --show-toplevel 2>/dev/null) || {
    echo "ABBRUCH: kein git-Arbeitsbaum -- die Repo-Wurzel ist nicht bestimmbar." >&2
    echo "         (Die Wurzel aus \$0 abzuleiten waere die Falle, die bei" >&2
    echo "          ci_diff_ascii_width_guard.sh schon einmal den falschen Baum mass.)" >&2
    exit 2
}
cd "$WURZEL"

command -v ctest >/dev/null 2>&1 || { echo "ABBRUCH: ctest nicht im PATH." >&2; exit 2; }

ALLOW="scripts/ci_test_sichtbarkeit_allowlist.txt"
TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

echo "============================================================================="
echo " TEST-SICHTBARKEITS-WACHE   Bau-Baum: $BUILD"
echo "============================================================================="

# -- SOLL: jede Registrierung im Quelltext -----------------------------------
# Erfasst werden add_test(NAME X ...), gtest_discover_tests(X ...) und die
# Hausfunktion comdare_add_test(X ...). Kommentarzeilen fallen raus (^\s*#).
# AUSGESCHLOSSEN: ext/ traegt VENDORIERTEN FREMDCODE (snmalloc, mimalloc, googletest u.a.).
# Deren add_test-Aufrufe sind nicht Teil unseres Bauwegs und werden hier nie sichtbar --
# sie als Befund zu fuehren waere eine Dauerklage ueber fremde Repositories.
# Am Objekt gefunden: ext/allocator/A07-snmalloc (header-only, static-shim),
# ext/allocator/A04-mimalloc (test-*), ext/traversal/P05-START (art_test).
# Der Ausschluss ist ein SCHNITT, kein Zudecken: er steht hier, nicht in der Allowlist,
# weil er eine Eigenschaft des Verzeichnisses ist und keine Einzelfall-Begruendung.
git ls-files '*CMakeLists.txt' '*.cmake' | grep -v '^ext/' > "$TMP/cmakelisten.txt"
N_DATEIEN=$(awk 'END{print NR+0}' "$TMP/cmakelisten.txt")
[ "$N_DATEIEN" -gt 0 ] || { echo "ABBRUCH: 0 CMakeLists.txt gefunden -- das kann nicht stimmen." >&2; exit 2; }

# NEWLINE-SICHERHEIT: eine sed-Ausgabe an eine Datei OHNE abschliessenden Newline
# anzuhaengen verkettet zwei Namen lautlos zu einem dritten, den es nicht gibt. Am
# Objekt aufgetreten: 'art_test' + 'test_commands' wurden zu 'art_testtest_commands'.
# Der Scanner unten ist EIN awk-Programm je Datei -- awk beendet jeden `print` mit ORS,
# damit ist die Falle strukturell ausgeschlossen statt nachtraeglich repariert.
#
# WARUM EIN ZUSTANDSAUTOMAT UND KEINE ZEILEN-REGEX (Befund 08.08. bei der Abnahme):
# Die erste Fassung suchte je EINE Zeile. Sie war damit blind fuer die MEHRZEILIGE
# Schreibweise --  add_test(  \n  NAME test_x  \n  COMMAND ...) -- und hat drei real
# registrierte Tests nie gesehen: test_axis_registry_roundtrip,
# test_system_axis_registry_roundtrip, test_measurement_axis_registry_roundtrip
# (tests/unit/CMakeLists.txt:5009/5039/5054). Alle drei sind heute sichtbar; die Wache
# haette ihr Verschwinden aber NICHT gemeldet, weil sie gar nicht im SOLL standen.
# Eine Wache, die den kranken Fall nicht sieht, ist keine Wache.
# Ausserdem faellt die Kurzform add_test(<name> <command>) ohne NAME-Schluesselwort
# hier mit an -- sie ist gueltiges CMake und kam in der Zeilen-Regex nicht vor.
: > "$TMP/soll.txt"
: > "$TMP/unaufloesbar.txt"
while IFS= read -r F; do
    [ -f "$F" ] || continue
    awk -v DATEI="$F" -v UNAUF="$TMP/unaufloesbar.txt" '
        function ausgeben(text, zeile,   rest, kopf, name) {
            # text ist der zusammengezogene Aufruf ab dem Funktionsnamen.
            kopf = text; sub(/[[:space:]]*\(.*$/, "", kopf)
            rest = text; sub(/^[^(]*\([[:space:]]*/, "", rest)
            # add_test(NAME <x> ...) -- das Schluesselwort faellt weg, der Name folgt.
            if (kopf == "add_test" && rest ~ /^NAME[[:space:]]/) {
                sub(/^NAME[[:space:]]+/, "", rest)
            }
            name = rest
            sub(/[[:space:]].*$/, "", name)       # erstes Wort
            sub(/\).*$/, "", name)                # einargumentige Aufrufe: add_test(x)
            if (name ~ /^[A-Za-z0-9_]+$/) {
                print name
            } else if (name != "") {
                # Name kommt aus einer CMake-Variablen (${...}) oder einem
                # Generator-Ausdruck. Statisch NICHT aufloesbar -- wird gezaehlt und
                # NAMENTLICH ausgewiesen, nie stillschweigend verworfen.
                print DATEI ":" zeile ": " name >> UNAUF
            }
        }
        {
            s = $0
            sub(/^[[:space:]]+/, "", s); sub(/[[:space:]]+$/, "", s)
            if (s ~ /^#/) { next }
            if (sammeln > 0) {
                puffer = puffer " " s
                sammeln--
                if (index(puffer, ")") > 0 || sammeln == 0) {
                    ausgeben(puffer, startzeile); sammeln = 0; puffer = ""
                }
                next
            }
            if (s ~ /^(add_test|gtest_discover_tests|comdare_add_test)[[:space:]]*\(/) {
                puffer = s; startzeile = NR
                if (index(s, ")") > 0) {
                    ausgeben(puffer, startzeile); puffer = ""
                } else {
                    sammeln = 5   # der Name steht immer in den ersten Argumenten
                }
            }
        }
        END { if (puffer != "") ausgeben(puffer, startzeile) }
    ' "$F" >> "$TMP/soll.txt"
done < "$TMP/cmakelisten.txt"

sort -u "$TMP/soll.txt" > "$TMP/soll_u.txt"
N_SOLL=$(awk 'END{print NR+0}' "$TMP/soll_u.txt")
N_UNAUF=$(awk 'END{print NR+0}' "$TMP/unaufloesbar.txt")
[ "$N_SOLL" -gt 0 ] || { echo "ABBRUCH: 0 Registrierungen im Quelltext gefunden -- die Suche griff nicht." >&2; exit 2; }

# -- IST: was ctest kennt ----------------------------------------------------
ctest --test-dir "$BUILD" -N > "$TMP/ctest_roh.txt" 2>/dev/null || {
    echo "ABBRUCH: 'ctest -N' im Baum '$BUILD' schlug fehl -- ist er konfiguriert?" >&2
    exit 2
}
sed -n 's/^[[:space:]]*Test[[:space:]]*#[0-9]\{1,\}:[[:space:]]*\(.*\)$/\1/p' "$TMP/ctest_roh.txt" > "$TMP/ist.txt"
N_IST=$(awk 'END{print NR+0}' "$TMP/ist.txt")
[ "$N_IST" -gt 0 ] || { echo "ABBRUCH: 'ctest -N' meldet 0 Tests -- der Baum ist ohne -DCOMDARE_BUILD_TESTS=ON konfiguriert." >&2; exit 2; }

# ZWEITE IST-QUELLE: die Kommandozeilen. `gtest_discover_tests(X)` registriert die
# Eintraege unter ihren GTEST-Namen ("Suite.Fall") -- der Zielname X kommt darin
# UEBERHAUPT NICHT vor. Ein reiner Namensvergleich meldet solche Ziele deshalb faelschlich
# als unsichtbar; genau das tat die erste Fassung dieser Wache bei test_commands und
# test_engine_adapters, die in Wahrheit 27 Faelle beisteuern.
# `ctest -N -V` druckt zu jedem Eintrag den ausfuehrbaren Pfad. Wir suchen den Zielnamen
# dort als Pfad-BASISNAME -- das misst, was wir meinen: laeuft das Binary in irgendeinem
# ctest-Eintrag mit?
ctest --test-dir "$BUILD" -N -V > "$TMP/ctest_v.txt" 2>/dev/null || : > "$TMP/ctest_v.txt"
# GENAU DIE 'Test command:'-ZEILEN, GENAU DAS ERSTE FELD DANACH (Befund 08.08.):
# Die erste Fassung zog den Basisnamen per `sed 's|.*/\(...\)|\1|'` aus JEDER Zeile mit
# einem Schraegstrich. Beides war falsch:
#   * `.*/` ist gierig und greift den LETZTEN Schraegstrich der Zeile. Bei
#     `Test command: /pfad/test_x "--daten=/tmp/y.csv"` kam 'y.csv' heraus, nicht
#     'test_x' -- ein sichtbares Ziel haette als unsichtbar gegolten (Dauer-Alarm).
#   * Ohne Zeilenfilter landeten Arbeitsverzeichnisse, .so-, .csv- und .xml-Namen in
#     derselben Menge: am Objekt 446 Tokens, von denen die Mehrzahl kein Testbinary ist.
#     Jede davon konnte einen gleichnamigen unsichtbaren Test faelschlich decken.
# Jetzt: nur 'Test command:'-Zeilen, davon das Feld direkt hinter 'command:', davon der
# Basisname. Das misst, was gemeint ist -- welches Binary in einem ctest-Eintrag laeuft.
awk '
    /Test command:/ {
        for (i = 1; i < NF; i++) {
            if ($i == "command:") {
                p = $(i + 1)
                gsub(/^"|"$/, "", p)
                sub(/.*\//, "", p)
                if (p != "") { print p }
                break
            }
        }
    }
' "$TMP/ctest_v.txt" | sort -u > "$TMP/ist_binaries.txt"

# -- Abgleich je NAME --------------------------------------------------------
# Sichtbar heisst: exakter ctest-Name ODER ein ctest-Eintrag mit dem Ziel als Praefix
# ("X.Fall") ODER der Zielname taucht als Binary in einer ctest-Kommandozeile auf.
# Die dritte Bedingung ist die, die gtest_discover_tests ueberhaupt erst erfasst.
: > "$TMP/unsichtbar.txt"
while IFS= read -r NAME; do
    [ -n "$NAME" ] || continue
    if grep -Fxq -- "$NAME" "$TMP/ist.txt"; then continue; fi
    if grep -q "^${NAME}\." "$TMP/ist.txt"; then continue; fi
    if grep -Fxq -- "$NAME" "$TMP/ist_binaries.txt"; then continue; fi
    echo "$NAME" >> "$TMP/unsichtbar.txt"
done < "$TMP/soll_u.txt"

N_UNSICHTBAR=$(awk 'END{print NR+0}' "$TMP/unsichtbar.txt")

# -- Allowlist ---------------------------------------------------------------
# Format je Zeile:  <name><TAB oder Leerzeichen><Begruendung>
# Zeilen mit '#' am Anfang sind Kommentar. Eine Allowlist-Zeile OHNE Begruendung
# ist selbst ein Fehler -- sonst waere die Allowlist der stille Rueckfall.
: > "$TMP/erlaubt.txt"
: > "$TMP/ohne_grund.txt"
if [ -f "$ALLOW" ]; then
    while IFS= read -r Z; do
        case "$Z" in ''|\#*) continue ;; esac
        NAME=$(printf '%s' "$Z" | awk '{print $1}')
        # TABULATOR-FALLE (Befund 08.08.): `cut -d' ' -f2-` gibt eine Zeile OHNE
        # Leerzeichen VOLLSTAENDIG zurueck. Die Zeile "name<TAB>" -- Tabulator ist laut
        # Kopf dieser Datei ein zulaessiger Trenner -- lieferte damit GRUND="name<TAB>",
        # ungleich NAME, und galt als begruendet. Genau der stille Rueckfall, gegen den
        # die Allowlist-Pruefung gebaut ist. awk trennt an Leerzeichen UND Tabulator.
        GRUND=$(printf '%s' "$Z" | awk '{ $1 = ""; sub(/^[[:space:]]+/, ""); print }')
        [ -n "$NAME" ] || continue
        if [ -z "$GRUND" ] || [ "$GRUND" = "$NAME" ]; then
            echo "$NAME" >> "$TMP/ohne_grund.txt"
        else
            echo "$NAME" >> "$TMP/erlaubt.txt"
        fi
    done < "$ALLOW"
fi
N_ALLOW=$(awk 'END{print NR+0}' "$TMP/erlaubt.txt")
N_OHNE_GRUND=$(awk 'END{print NR+0}' "$TMP/ohne_grund.txt")

: > "$TMP/befund.txt"
: > "$TMP/gedeckt.txt"
while IFS= read -r NAME; do
    [ -n "$NAME" ] || continue
    if grep -Fxq -- "$NAME" "$TMP/erlaubt.txt"; then
        echo "$NAME" >> "$TMP/gedeckt.txt"
    else
        echo "$NAME" >> "$TMP/befund.txt"
    fi
done < "$TMP/unsichtbar.txt"
N_BEFUND=$(awk 'END{print NR+0}' "$TMP/befund.txt")
N_GEDECKT=$(awk 'END{print NR+0}' "$TMP/gedeckt.txt")

# AUSNAHMEN, DIE IHREN GRUND UEBERLEBT HABEN: ein Allowlist-Eintrag, dessen Test in
# DIESEM Baum laengst sichtbar ist, deckt nichts mehr -- er steht nur noch da und
# wuerde beim naechsten echten Verschwinden desselben Namens lautlos greifen.
# Das ist BERICHT, kein Fehler: welche Bedingung zutrifft, haengt am Baum (der
# prt-art-Legacy-Baum ist lokal da und in der CI nicht). Ein rc=1 daraus zu machen
# hiesse, die Wache auf EINER Maschine gruen und auf der anderen rot zu fahren.
: > "$TMP/verwaist.txt"
while IFS= read -r NAME; do
    [ -n "$NAME" ] || continue
    grep -Fxq -- "$NAME" "$TMP/unsichtbar.txt" || echo "$NAME" >> "$TMP/verwaist.txt"
done < "$TMP/erlaubt.txt"
N_VERWAIST=$(awk 'END{print NR+0}' "$TMP/verwaist.txt")

# -- Ausgabe -----------------------------------------------------------------
if [ "$N_BEFUND" -gt 0 ]; then
    echo ""
    echo "UNSICHTBARE REGISTRIERUNGEN OHNE BEGRUENDUNG:"
    while IFS= read -r NAME; do
        [ -n "$NAME" ] || continue
        # ORT NUR AUS EINER ECHTEN REGISTRIERUNGSZEILE (Befund 08.08. beim Koederlauf):
        # Die erste Fassung nahm die erste Datei, die den Namen IRGENDWO trug, und
        # zeigte deshalb auf einen KOMMENTAR in der Wurzel-CMakeLists statt auf die
        # Registrierung. Fuer den mehrzeiligen Aufruf fand sie gar nichts ("unbekannt"),
        # weil ihr Muster hinter dem Namen ein Zeichen verlangte -- am Zeilenende steht
        # keines. Eine Fundstelle, die auf die falsche Zeile zeigt, kostet den Leser
        # genau die Zeit, die die Wache sparen soll.
        ORT=$(awk -v n="$NAME" '
            $0 ~ ("(add_test|gtest_discover_tests|comdare_add_test)\\([[:space:]]*(NAME[[:space:]]+)?" n "([^A-Za-z0-9_]|$)") { print FILENAME ":" FNR; exit }
            $0 ~ ("^[[:space:]]*NAME[[:space:]]+" n "([^A-Za-z0-9_]|$)") { print FILENAME ":" FNR; exit }
        ' $(cat "$TMP/cmakelisten.txt") 2>/dev/null)
        echo "  $NAME   (registriert in: ${ORT:-unbekannt})"
    done < "$TMP/befund.txt"
fi

if [ "$N_UNAUF" -gt 0 ]; then
    echo ""
    echo "NICHT GEDECKT -- Name steht in einer CMake-Variablen, statisch nicht aufloesbar:"
    echo "  (Diese Aufrufe registrieren echte Tests. Ob sie ankommen, kann diese Wache"
    echo "   NICHT pruefen -- sie kennt den Namen nicht. Sie werden hier gezaehlt und"
    echo "   genannt, damit die Luecke eine ZAHL hat statt eines Schweigens.)"
    sort -u "$TMP/unaufloesbar.txt" | while IFS= read -r Z; do
        [ -n "$Z" ] || continue
        echo "  $Z"
    done
fi

if [ "$N_GEDECKT" -gt 0 ]; then
    echo ""
    echo "UNSICHTBAR, ABER BEGRUENDET (Allowlist $ALLOW):"
    while IFS= read -r NAME; do
        [ -n "$NAME" ] || continue
        G=$(grep -E "^${NAME}([[:space:]]|$)" "$ALLOW" 2>/dev/null | head -1 | cut -d' ' -f2- | sed 's/^[[:space:]]*//')
        echo "  $NAME -- $G"
    done < "$TMP/gedeckt.txt"
fi

if [ "$N_VERWAIST" -gt 0 ]; then
    echo ""
    echo "ALLOWLIST-EINTRAEGE OHNE WIRKUNG IN DIESEM BAUM (Bericht, kein Fehler):"
    echo "  (Diese Tests sind hier sichtbar -- die Ausnahme deckt gerade nichts. In einem"
    echo "   anders konfigurierten Baum kann sie noetig sein; greift sie nirgends mehr,"
    echo "   gehoert die Zeile ersatzlos entfernt statt auf Vorrat behalten.)"
    while IFS= read -r NAME; do
        [ -n "$NAME" ] || continue
        echo "  $NAME"
    done < "$TMP/verwaist.txt"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $N_DATEIEN CMake-Datei(en) gescannt."
echo "  $N_SOLL Registrierung(en) im Quelltext (SOLL, je NAME einmal gezaehlt)."
echo "  $N_IST ctest-Eintrag(e) im Baum (IST; eine Registrierung kann in mehrere expandieren)."
echo "  $N_UNSICHTBAR unsichtbar, davon $N_GEDECKT begruendet und $N_BEFUND OHNE Begruendung."
echo "  Allowlist: $N_ALLOW Eintrag/Eintraege mit Begruendung, davon $N_VERWAIST in diesem Baum wirkungslos."
echo "  $N_UNAUF Registrierung(en) mit nicht statisch aufloesbarem Namen -- VON DIESER"
echo "  WACHE NICHT GEDECKT (Namen stehen in einer CMake-Variablen, s. Liste oben)."
echo "-----------------------------------------------------------------------------"

if [ "$N_OHNE_GRUND" -gt 0 ]; then
    echo "FEHLER: $N_OHNE_GRUND Allowlist-Zeile(n) ohne Begruendung -- eine Allowlist ohne" >&2
    echo "        Begruendung ist der stille Rueckfall, gegen den diese Wache gebaut ist." >&2
    exit 1
fi

if [ "$N_BEFUND" -gt 0 ]; then
    echo "FEHLER: $N_BEFUND von $N_SOLL Registrierung(en) sind unsichtbar und nicht begruendet." >&2
    echo "        Entweder ist die Registrierung kaputt (enable_testing-Reihenfolge, fehlendes" >&2
    echo "        add_subdirectory, ungebautes Ziel) -- oder sie gehoert mit Begruendung in" >&2
    echo "        $ALLOW. Stillschweigen ist keine der beiden Moeglichkeiten." >&2
    exit 1
fi

echo "TEST-SICHTBARKEITS-WACHE: OK ($N_SOLL Registrierungen, $N_BEFUND unbegruendet unsichtbar)."
exit 0
