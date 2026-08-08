#!/bin/sh
# =============================================================================
#  KOEDER-BATTERIE: die Mess-Gates-Grammatik bindet compile-hart
#  scripts/koeder_clang_constexpr_grammatik.sh
# =============================================================================
#
# WELCHEN DEFEKT SIE NACHWEIST
# abi/mess_gates_glied.hpp traegt EINE Grammatik-Bildung, an die eine
# Praeprozessor-Fassung per static_assert gebunden ist. Die Bildung gab bis zum
# 08.08.2026 einen `constexpr std::string` zurueck -- clang 22.1 kann das nicht
# auswerten, g++ 15.3 schon. Die Wache gegen die Drift-Klasse D-1 hielt also nur
# auf EINEM der beiden offiziellen Compiler, und der ce-Vollbau mit clang brach
# an ihr ab.
#   Heilung:      ce development 8d5ba807   (Bau-Commit b83e872c)
#   Wurzel:       nur std::string AUS EINEM string_view bricht -- s. die Probe
#                 scripts/koeder_clang_constexpr_op_matrix.cpp, die diese Datei mitfaehrt.
# Die Bildung liefert seither abi::MessGatesGliedText (std::array + Laenge).
# DIESE BATTERIE prueft, dass die neue Bildung die Grammatik WIRKLICH bindet und
# nicht bloss constexpr-tauglich geworden ist.
#
# WIE MAN SIE FAEHRT
#     sh scripts/koeder_clang_constexpr_grammatik.sh
# Kein Bau noetig, keine Argumente. Sie braucht /usr/bin/g++ und clang++ im PATH
# und arbeitet auf Kopien -- der Arbeitsbaum wird NICHT veraendert.
#
# ERWARTETE AUSGABE (Stand 08.08.2026)
#   K0      beide Compiler BRECHEN mit "K0 WERKZEUG-PROBE"
#   NENNER  beide Compiler BAUEN DURCH
#   K1      beide BRECHEN, Text "R-3/W3: COMDARE_MEASUREMENT_ON ..."
#   K2      beide BRECHEN, Text "R-3: die Praeprozessor-Bildung ..."
#   K3      beide BRECHEN, Text "R-3: die Praeprozessor-Bildung ..."
#   K4      beide BRECHEN, Text "R-3/08.08.: kMessGatesGliedMaxLen ..."
#   K5      beide BRECHEN -- ABER UNBENANNT (s.u.)
#   K6      beide BRECHEN, Text "R-3: die Praeprozessor-Bildung ..."
# K2/K3/K6 sind der eigentliche Beweis: sie schlagen AUSSCHLIESSLICH ueber den
# Binde-static_assert an, nicht ueber die W3-Asserts.
#
# ZWEI DINGE, DIE MAN BEIM LESEN WISSEN MUSS
#  * K0 IST DIE WERKZEUG-PROBE und steht bewusst VOR allem anderen: sie belegt,
#    dass dieser Pruefaufruf ueberhaupt Fehler SIEHT. Ein Koeder-Lauf, bei dem K0
#    "BAUT DURCH" meldet, ist wertlos -- dann misst das Skript nichts.
#  * K5 bricht UNBENANNT (clang: "static assertion expression is not an integral
#    constant expression", g++: "non-constant condition for static assertion",
#    beide ohne Text). Das ist das .at() im Puffer, kein benannter static_assert.
#    Bekannt und im Header dokumentiert -- es ist KEIN Fehler dieser Batterie.
#
# POSIX-sh, ASCII-only.
# =============================================================================
set -u

command -v git >/dev/null 2>&1 || { echo "ABBRUCH: git fehlt" >&2; exit 2; }
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null) || { echo "ABBRUCH: kein git-Repository" >&2; exit 2; }
QUELL="$REPO_ROOT/libs/cache_engine/include/cache_engine/abi/mess_gates_glied.hpp"
PROBE="$REPO_ROOT/scripts/koeder_clang_constexpr_op_matrix.cpp"
[ -f "$QUELL" ] || { echo "ABBRUCH: $QUELL fehlt" >&2; exit 2; }
[ -f "$PROBE" ] || { echo "ABBRUCH: $PROBE fehlt" >&2; exit 2; }
for CXX in /usr/bin/g++ clang++; do
    command -v "$CXX" >/dev/null 2>&1 || { echo "ABBRUCH: $CXX fehlt -- ein nicht gefahrener Test ist kein bestandener" >&2; exit 2; }
done

ARB=$(mktemp -d) || { echo "ABBRUCH: kein Arbeitsverzeichnis" >&2; exit 2; }
trap 'rm -rf "$ARB"' EXIT INT TERM

ROT=0

lauf() { # $1 = Koeder-Name ($ARB/h.hpp steht bereit)
    for CXX in /usr/bin/g++ clang++; do
        AUS=$($CXX -std=c++23 -fsyntax-only -Wno-pragma-once-outside-header -x c++ "$ARB/h.hpp" 2>&1)
        RC=$?
        KURZ=$(printf '%s' "$AUS" | grep -oE 'R-3[:/][^"]*' | head -1 | cut -c1-64)
        [ -z "$KURZ" ] && KURZ=$(printf '%s' "$AUS" | grep -E 'error:' | head -1 | sed 's/.*error: //' | cut -c1-64)
        if [ $RC -eq 0 ]; then
            if [ "$1" = "NENNER" ]; then
                printf '  %-9s %-8s BAUT DURCH  (so soll es sein)\n' "$1" "$(basename "$CXX")"
            else
                printf '  %-9s %-8s BAUT DURCH  <-- KOEDER BEISST NICHT, eine Wache ist weg\n' "$1" "$(basename "$CXX")"
                ROT=1
            fi
        else
            if [ "$1" = "NENNER" ]; then
                printf '  %-9s %-8s BRICHT      <-- BATTERIE KAPUTT, nicht der Header: %s\n' "$1" "$(basename "$CXX")" "$KURZ"
                ROT=1
            else
                printf '  %-9s %-8s BRICHT      %s\n' "$1" "$(basename "$CXX")" "$KURZ"
            fi
        fi
    done
}

echo "=============================================================================="
echo " TEIL 1 -- DIE WURZEL: welche std::string-Operation bricht wirklich?"
echo "          (Probe scripts/koeder_clang_constexpr_op_matrix.cpp)"
echo "=============================================================================="
for P in P1 P2 P3 P4 P5 P6 P7; do
    printf '  %-4s' "$P"
    for CXX in /usr/bin/g++ clang++; do
        printf ' %s: ' "$(basename "$CXX")"
        if $CXX -std=c++23 -fsyntax-only "-D$P" "$PROBE" >/dev/null 2>&1; then printf 'OK   '; else printf 'FEHL '; fi
    done
    [ "$P" = "P1" ] && printf '  <-- die EINE brechende Operation'
    echo ""
done

echo ""
echo "=============================================================================="
echo " TEIL 2 / K0  WERKZEUG-PROBE: sieht der Pruefaufruf ueberhaupt einen Fehler?"
echo "=============================================================================="
{ cat "$QUELL"; echo 'static_assert(false, "K0 WERKZEUG-PROBE");'; } > "$ARB/h.hpp"
lauf K0

echo ""
echo "=============================================================================="
echo " NENNER: der UNVERAENDERTE Header muss mit BEIDEN Compilern durchbauen"
echo "=============================================================================="
cp "$QUELL" "$ARB/h.hpp"
lauf NENNER

echo ""
echo "=============================================================================="
echo " K1..K6  DIE ECHTEN KOEDER"
echo "=============================================================================="

# K1: die PRAEPROZESSOR-Fassung luegt ueber das Measurement-Gate (AUS-Zweig sagt "m1").
sed 's/^#define COMDARE_MESS_GATES_SEG_M "m0"$/#define COMDARE_MESS_GATES_SEG_M "m1"/' "$QUELL" > "$ARB/h.hpp"
lauf K1

# K2: die KOMPONISTEN-Seite weicht ab (Segment-Tabelle: "m0" -> "mX").
sed 's/{"m0", "m1"}/{"mX", "m1"}/' "$QUELL" > "$ARB/h.hpp"
lauf K2

# K3: die Segment-Tabelle wird UMSORTIERT (Statistics und Experiment tauschen die Zeile).
sed 's/{"s0", "s1"}, {"x0", "x1"}/{"x0", "x1"}, {"s0", "s1"}/' "$QUELL" > "$ARB/h.hpp"
lauf K3

# K4: die Kapazitaet wird ZU GROSS hart gesetzt (die Rechnung aus der Tabelle umgangen).
sed 's/^inline constexpr std::size_t kMessGatesGliedMaxLen = \[\] {/inline constexpr std::size_t kMessGatesGliedMaxLen = 30; inline constexpr std::size_t kUnbenutztK4 = [] {/' "$QUELL" > "$ARB/h.hpp"
lauf K4

# K5: die Kapazitaet wird ZU KLEIN hart gesetzt -- der Puffer muss ueberlaufen (unbenannt, s. Kopf).
sed 's/^inline constexpr std::size_t kMessGatesGliedMaxLen = \[\] {/inline constexpr std::size_t kMessGatesGliedMaxLen = 20; inline constexpr std::size_t kUnbenutztK5 = [] {/' "$QUELL" > "$ARB/h.hpp"
lauf K5

# K6: der FELD-TRENNER der Bildung weicht von der Praeprozessor-Fassung ab.
sed "s/^inline constexpr char kMessGatesFeldTrenner = ';';/inline constexpr char kMessGatesFeldTrenner = ',';/" "$QUELL" > "$ARB/h.hpp"
lauf K6

echo ""
if [ "$ROT" -eq 0 ]; then
    echo "KOEDER-BATTERIE: GRUEN -- K0 meldet, der Nenner baut durch, alle Koeder beissen."
    exit 0
fi
echo "KOEDER-BATTERIE: ROT -- entweder der Nenner bricht (dann ist die Batterie kaputt,"
echo "nicht der Header) oder ein Koeder beisst nicht mehr (dann ist eine Wache weg)."
exit 1
