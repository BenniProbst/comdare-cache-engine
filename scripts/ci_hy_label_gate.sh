#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  hy_label_gate -- ABI-ETIKETT- UND ANKER-WACHE ueber die Hybrid-Doku (2026-08-08)
#  Paket HY-0, GOAL v8 (T-1 rot zuerst, T-3 Nenner FREMD, T-7 Registrierung ist Teil
#  des Tests). POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST (am Objekt gemessen, 08.08.2026):
# libs/cache_engine/hybrid/README.md:50 sagte "die bestehende Anatomy-ABI (Major 7,
# anatomy_module_abi_v1_decl.hpp:63)". Der Decl-Header sagt seit E-24 C8 (04.08.2026,
# ce 4f569051) Major 8, Magic .A8. Das als bindend markierte Design-Dokument
# docs/architecture/20260802-hybrid_tier_stufe_soll_design.md:62-63 trug DENSELBEN
# Satz. Wer die Hybrid-Doku liest und ihr folgt, baut gegen eine ABI-Major, die es
# nicht mehr gibt -- und der Loader lehnt das Ergebnis per Major-Mismatch ab.
# Die Zeilen-Anker waren zum Zeitpunkt ihres Schreibens RICHTIG: gegen 6b8eee0f (den
# Basisstand, den das Design-Dokument in Abschnitt 11 selbst nennt) wie gegen 53ca992d
# (den letzten Stand vor dem Bump) zeigt :62 wirklich auf den MAJOR-define. Sie sind
# nicht falsch geschrieben, sie sind GEWANDERT. Genau deshalb ist die zweite Haelfte
# dieser Wache eine ANKER-Pruefung und keine Zahlenpruefung.
#
# WARUM EINE WACHE UND NICHT NUR DIE HEILUNG: die Heilung deckt den heutigen Text.
# Der naechste Major-Bump verschiebt Zahl UND Zeilennummern erneut, lautlos, weil
# kein Compiler Prosa uebersetzt. GOAL v8 Teil VII verlangt zu jeder Abnahme die
# Antwort auf "was erzwingt das Halten?" -- das hier ist das Werkzeug.
#
# DER NENNER IST FREMD (T-3): die Wahrheit kommt aus dem Decl-Header selbst, nicht
# aus einer Konstanten neben dem Text. Diese Wache traegt NIRGENDS eine eigene
# Major-Zahl. Sie liest COMDARE_ANATOMY_ABI_MAJOR und COMDARE_ANATOMY_ABI_MAGIC aus
# dem Header und rechnet alles Weitere daraus aus.
#
# DAS UNABHAENGIGE ORAKEL (T-5): die Magic KODIERT den Major. Aus dem Major wird die
# erwartete Magic-Byte-Folge GERECHNET und gegen das Literal im Header gehalten:
#   "COMDA" = 43 4F 4D 44 41 | "A" = 41 | Ziffer = 0x3<d> | "." = 2E
#   Major 8 -> 434F4D444141 . 38 . 2E -> 0x434F4D444141382EULL
# Widersprechen sich MAJOR und MAGIC, bricht die Wache ab (rc=2) statt sich fuer eine
# der beiden Zahlen zu entscheiden -- dann ist der Header selbst der Befund.
#
# ============================ WAS GEPRUEFT WIRD ==============================
# TEIL A -- ETIKETTEN in der Hybrid-Doku (Liste unten, jede Datei MUSS existieren):
#   Jede Angabe "Major <N>", ".A<N>." oder ein Magic-Hex-Literal wird eingesammelt.
#     * N == lebender Major  -> LEBEND.
#     * N != lebender Major  -> nur zulaessig MIT dem Marker auf DERSELBEN Zeile:
#                                 ABI-HISTORIE gegen SHA <hex>
#                               sonst BEFUND.
#   Zusaetzlich MUSS jede Datei den lebenden Major mindestens einmal nennen. Ohne
#   diese Regel waere eine Doku, die nur noch markierte Historie enthaelt, gruen --
#   also eine Datei, die den heutigen Stand verschweigt (T-2: Aussage statt
#   Anwesenheit).
#
# TEIL B -- LEBEND-BEHAUPTUNGEN im GESAMTEN Repo (Nenner: alle getrackten Dateien
#   ausser ext/). Eine Zeile, die eine Lebend-Formulierung zusammen mit einer
#   Major-Zahl nennt, BEHAUPTET den heutigen Stand. Sie muss mit dem Header
#   uebereinstimmen. Dieser Teil hat KEINE Liste, die jemand pflegen muesste -- er
#   findet die Schwesterstelle auch in einer Datei, an die beim Bau dieser Wache
#   niemand gedacht hat.
#   GRAMMATIK (Nachsatz 08.08., Review-Mangel geschlossen): die erste Fassung sah
#   nur "aktuell" und "lebender stand" und wertete nur den ERSTEN Major-Treffer der
#   Zeile. Das Paket HY-0 hatte SELBST eine Lebend-Behauptung in der Formulierung
#   "Lebend gilt seitdem: Major <n>" gepflanzt -- unsichtbar fuer die eigene Wache
#   (Koeder-Beleg: dieselbe Formulierung mit gewuerfelter Major-Ziffer blieb gruen;
#   die Ziffer steht hier bewusst NICHT, ein Zitat veraltet wie das Original).
#   Jetzt triggern "aktuell", "lebender stand", "lebend gilt", "lebend seit" und
#   "lebend:" (benannte LISTE, s. Grenze 6), und ALLE Major-Treffer der Zeile werden
#   gewertet, nicht nur der erste. Teil-B-Funde MIT Anker laufen zusaetzlich durch
#   die Anker-Pruefung von Teil C (Dubletten zur Teil-A-Liste ausgenommen).
#
# TEIL D -- POD-MASSE (Schwesterklasse, am 08.08. per T-6 gefunden): dieselbe
#   Fehlerklasse -- eine Zahl, die anderswo lebt, als Kopie im Kommentar -- trifft im
#   selben Header die Observer-POD-Masse. observable_tier.hpp:15 und :44 nannten
#   axis_stats[17][8] / seg_ns[17], waehrend `kV3AxisCount` (:61) auf 18 steht; die
#   17 stammt aus der Zeit vor dem 6->7-Bump. Der Befund war seit dem 04.08. als
#   Kandidat 45 im super-Repo notiert und dort NICHT gezogen ("gleiche Klasse wie
#   C11-OP-9"). Teil D haelt jede LITERALE Angabe axis_stats[<N>][ oder seg_ns[<N>]
#   in den geprueften Dateien gegen kV3AxisCount -- mit derselben Marker-Regel wie
#   Teil A. Symbolische Formen (axis_stats[kV3AxisCount][8]) sind KEINE Kopie und
#   werden nicht angefasst; sie sind der richtige Weg, diese Wache leerlaufen zu lassen.
#   NACHSATZ 08.08. (Review-Maengel geschlossen): Teil D prueft ZUSAETZLICH
#     * "ALLE <N> ...Achsen" / "ALLER <N> ...Achsen" -- die Totalitaets-Behauptung
#       ueber die Achsenzahl in Prosa (der blockierende Mangel stand als
#       "sind ALLE 17 Achsen befuellt" in der bereits GEHEILTEN Datei, 10 Zeilen
#       unter der Heilung, und diese Wache sah ihn nicht),
#     * "T0..T<M>" -- die Bereichs-Obergrenze muss kV3AxisCount-1 sein,
#   und er laeuft ueber eine EIGENE Dateiliste: die Teil-A-Liste PLUS die von
#   observable_tier.hpp direkt eingebundene Schwesterdatei measurable_workload.hpp
#   (dort lebte die Klasse weiter: seg_ns[17], ALLE(R) 17, T0..T16).
#
# TEIL C -- ANKER (die eigentliche Anker-Wache): traegt eine Zeile aus Teil A oder
#   Teil B einen Anker der Form anatomy_module_abi_v1_decl.hpp:<L>, dann MUSS die
#   Zeile <L>
#     * bei LEBEND-Zeilen: im HEUTIGEN Header ein #define mit der behaupteten Zahl
#       tragen (MAJOR <N> ODER MAGIC mit der zu <N> gerechneten Byte-Folge -- beide
#       kodieren denselben ABI-Stand, eine Zeile darf auf beide zeigen),
#     * bei HISTORIE-Zeilen: in `git show <SHA>:<Header>` dasselbe tragen.
#   Damit ist "gegen SHA <pin> verifizierte Historie" keine Behauptung mehr, sondern
#   eine bei jedem Lauf nachgerechnete Aussage -- und ein LEBEND-Anker kann nicht
#   still verrotten, wenn sich der Header verschiebt.
#   FAIL-CLOSED (Nachsatz 08.08., Review-Mangel geschlossen): ein Historien-SHA,
#   dessen Objekt in einem VOLLEN Klon fehlt, ist ab jetzt ein BEFUND (erfunden,
#   vertippt oder nie gepusht -- Koeder-Beleg: ein aus /dev/urandom gewuerfeltes
#   40-Hex-SHA lief vorher als "nicht pruefbar" mit rc=0 durch). NUR in einem
#   FLACHEN Klon (git rev-parse --is-shallow-repository = true) bleibt das fehlende
#   Objekt eine gezaehlte, benannte Luecke -- dort fehlt der Klon, nicht die Aussage.
#
# ====================== GRENZEN, EHRLICH BENANNT =============================
#  1. TEIL A hat eine LISTE. Ein NEUES Hybrid-Dokument, das niemand eintraegt, wird
#     von Teil A nicht gesehen. Aufgefangen wird davon nur die schaerfste Klasse:
#     eine ausdrueckliche LEBEND-Behauptung faellt Teil B auch ohne Liste auf.
#     Nicht aufgefangen: eine neue Datei, die eine ueberholte Major-Zahl OHNE das
#     Wort "aktuell" schreibt. Das ist der Preis dafuer, dass Teil B keine Dauerklage ueber jede
#     historisch korrekte Bump-Erzaehlung im Repo fuehrt (am Objekt: 127 Fundstellen
#     mit Major-/Magic-Angabe, die grosse Mehrheit legitime Historie).
#  2. TEIL C sieht nur Anker MIT Dateinamen. Die Kurzform `:66`, die sich auf eine
#     frueher in der Zeile genannte Datei bezieht (so geschrieben in
#     20260802-hybrid_tier_stufe_soll_design.md:63), kann diese Wache NICHT auf-
#     loesen, ohne den Bezug zu raten. Solche Anker werden GEZAEHLT und namentlich
#     ausgewiesen -- eine Luecke mit Zahl statt eines Schweigens.
#  3. Diese Wache prueft ETIKETTEN, nicht ARCHITEKTUR. Ob die Hybrid-Stufe wirklich
#     ohne eigenen ABI-Schritt auskommt, sagt sie nicht -- nur, dass die Doku ueber
#     die ABI dasselbe sagt wie die ABI.
#  3b. TEIL D deckt die POD-Masse axis_stats[N][8] und seg_ns[N], die Totalitaets-
#     Form "ALLE(R) <N> ...Achsen" und die Bereichs-Obergrenze "T0..T<M>" gegen
#     kV3AxisCount. NICHT gedeckt bleiben: Teilmengen-Zaehlungen ("13 der Achsen
#     sind passiv", "4 instrumentierte Achsen" -- ihr Soll ist NICHT kV3AxisCount,
#     eine Pruefung dagegen waere eine Falschklage), Slot-Angaben ohne das Wort
#     Achsen ("17 Slots"), Zahlangaben ohne ALLE(R) davor ("die 18 ...-Achsen"),
#     sowie sizeof 1344, kV3FieldCount 8 und organ_count(). Die geheilten Stellen
#     dieser Klassen tragen deshalb KEINE Kopie mehr, sondern den Verweis.
#  4. Terminologie (Gattung/Genus-Sprache in Kommentaren) ist AUSDRUECKLICH NICHT
#     gedeckt. Eine Sprach-Wache ueber Prosa waere unverhaeltnismaessig und wuerde
#     an Synonymen scheitern; sie bliebe entweder blind oder eine Dauerklage.
#  5. FLACHER KLON: NUR dort (GitLab klont per Default flach) ist ein fehlendes
#     SHA-Objekt eine Eigenschaft des Klons, nicht der Aussage -- solche Anker werden
#     GEZAEHLT und bei jedem Lauf einzeln genannt, statt rot oder still gruen zu
#     sein. In einem VOLLEN Klon ist dasselbe fehlende Objekt ein harter BEFUND
#     (fail-closed, s. Teil C). Nicht unterschieden wird ein PARTIELLER Klon
#     (--filter/promisor): er meldet --is-shallow-repository=false und wuerde wie
#     ein voller behandelt -- dort kann ein realer, nur nicht geholter Pin faelschlich
#     rot werden. Das ist die fail-closed-Richtung und bleibt benannt statt geraten.
#  6. TEIL B ist eine benannte GRAMMATIK-LISTE (aktuell / lebender stand /
#     lebend gilt / lebend seit / lebend:). Eine Lebend-Behauptung in einer
#     Formulierung AUSSERHALB der Liste sieht er nicht ("ist derzeit Major N").
#     Bewusst NICHT aufgenommen: das nackte Wort "lebend" -- es traefe legitime
#     Saetze wie "der eingefrorene Major-7-Host ist am lebenden Host NICHT
#     ladefaehig" (tests/unit/test_v41_anatomy_module_abi.cpp) und machte die
#     Wache zur Dauerklage.
#
# WERKZEUG-DISZIPLIN: /usr/bin/grep explizit (das blosse Wort 'grep' ist in dieser
# Umgebung eine ugrep-Emulation), Kern-Logik in EINEM awk-Programm ohne Pipe in eine
# Schleife (eine Pipe legt die Schleife in eine Subshell und verwirft ihre Zaehler).
# Kann die Wache ueberhaupt nicht messen -- fehlender Header, mehrdeutige Makros, sich
# widersprechende Quelle, leerer Nenner --, ist das rc=2 und ausdruecklich KEIN Gruen.
# Die EINE Ausnahme ist Grenze 5: dort fehlt nicht die Messfaehigkeit, sondern ein
# einzelnes Objekt, und die Luecke traegt eine Zahl.
#
# AUFRUF:  sh scripts/ci_hy_label_gate.sh [-w <repo-wurzel>] [datei ...]
#          ohne Dateien: die Hybrid-Doku-Liste unten (der Regelfall, auch in ctest)
#          mit Dateien:  genau diese (Koeder-Fixture, Einzelpruefung)
# EXIT:    0 = jede Angabe lebt oder ist als Historie verifiziert
#          1 = mindestens ein Etikett/Anker haelt nicht
#          2 = die Wache konnte nicht pruefen (fail-closed)
# =============================================================================

set -eu

NAME="hy_label_gate"
GREP="/usr/bin/grep"
DECL_REL="libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp"

# Die Hybrid-Doku plus die eine Code-Stelle, die eine LEBEND-Behauptung ueber
# dieselbe ABI traegt. README und das von ihr als "Design (bindend)" bezeichnete
# Architektur-Dokument gehoeren zusammen -- der Drift stand am 08.08. in BEIDEN.
# observable_tier.hpp steht hier, weil die Schwestersuche (T-6) sie am selben Tag
# mit einer zwei Majors alten LEBEND-Behauptung fand: dieselbe Fehlerklasse, seit
# dem 26.07. Teil B faende sie auch ohne diese Liste -- in der Liste bekommt sie
# zusaetzlich die Anker- und die Lebend-Pflicht aus Teil A/A2.
#
# EIGENER BISS (08.08., bei der Abnahme): genau diese beiden Kommentarzeilen trugen
# vorher die Zahlen als Zitat neben dem Wort "aktuell" -- Teil B hat die WACHE SELBST
# gemeldet, sobald ihre Datei im Index lag. Das ist kein Schoenheitsfehler, sondern
# der Beweis, dass Teil B keine Ausnahme fuer sich kennt: eine Wache, die ihre eigene
# Datei ueberspringt, hat einen blinden Fleck genau dort, wo sie am meisten schreibt.
# Ein Zitat einer fremden Zahl veraltet wie das Original.
DOKU_STANDARD="libs/cache_engine/hybrid/README.md
docs/architecture/20260802-hybrid_tier_stufe_soll_design.md
libs/cache_engine/anatomy/observable_tier.hpp"

# Teil D laeuft ZUSAETZLICH ueber die von observable_tier.hpp:47 direkt eingebundene
# Schwesterdatei (Nachsatz 08.08., Review-Mangel geschlossen: dort lebte dieselbe
# Klasse weiter -- seg_ns[17], "ALLE(R) 17 ...Achsen", T0..T16) und ueber
# perm_runner.hpp (bei der Nachsatz-T-6-Suche gefunden: vier lebende 17er-Aussagen
# derselben Klasse, u.a. "ALLE 17 Achsen ... warmup-frei"). Beide stehen bewusst
# NICHT in der Teil-A-Liste: sie tragen keine Major-/Magic-Etiketten, die
# Lebend-Pflicht A2 waere dort eine leere Forderung und damit eine Dauerklage.
# NICHT aufgenommen: result_ingest.hpp -- seine "17-Segment-id" beschreibt das
# ALT-Format (Zeilen vor ORG-18 tragen konstruktionsbedingt 17 Segmente), eine
# korrekte Aussage, die gegen kV3AxisCount zu halten eine Falschklage waere.
# Repo-weit laufen die D-Muster NICHT: 161 "alle N ...Achsen"-Zeilen in 93 Dateien
# tragen ueberwiegend FREMDE Nenner (11 Baustein-, 19 Alt-, 22 Registry-Achsen)
# oder legitime Bump-Erzaehlung -- das waere die Dauerklage aus Grenze 1.
POD_ZUSATZ="libs/cache_engine/anatomy/measurable_workload.hpp
libs/cache_engine/harness/perm_runner.hpp"

abbruch() {
    echo "$NAME: ABBRUCH -- $1" >&2
    shift
    for _z in "$@"; do echo "  $_z" >&2; done
    exit 2
}

WURZEL=""
if [ "${1:-}" = "-w" ]; then
    [ $# -ge 2 ] || abbruch "-w ohne Pfad."
    WURZEL="$2"
    shift 2
fi

[ -x "$GREP" ] || abbruch "$GREP fehlt (POSIX-Grep vorausgesetzt)."
command -v awk >/dev/null 2>&1 || abbruch "awk nicht im PATH."

# Die Wurzel kommt von git, nicht aus dirname $0: faellt dirname aus, landet
# cd "/.." lautlos auf / und die Wache misst den falschen Baum, ohne dass set -e
# anschlaegt (cd war ja erfolgreich).
if [ -z "$WURZEL" ]; then
    WURZEL=$(git rev-parse --show-toplevel 2>/dev/null || true)
    [ -n "$WURZEL" ] || abbruch "kein git-Arbeitsbaum (aus '$(pwd)' heraus aufgerufen)." \
        "Ohne Wurzel gibt es keinen belastbaren Nenner, also auch kein Gruen." \
        "Alternative: sh $0 -w <repo-wurzel>"
fi
[ -d "$WURZEL" ] || abbruch "'$WURZEL' ist kein Verzeichnis."

DECL="$WURZEL/$DECL_REL"
[ -f "$DECL" ] || abbruch "Decl-Header nicht gefunden: $DECL" \
    "Die Wahrheit dieser Wache kommt ausschliesslich aus diesem Header." \
    "Ohne ihn kann sie KEINE Aussage treffen -- ein Skip waere ihr Abschalten."

# -- Nenner FREMD: Major und Magic aus dem Header ------------------------------
# Genau EIN define je Makro. Zwei (z.B. hinter #ifdef) hiesse: die Wache muesste
# raten, welches gilt -- dann bricht sie lieber ab.
MAJOR=$(awk '$1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAJOR" { n++; v=$3 } END { if (n==1) print v }' "$DECL")
MAJOR_N=$(awk '$1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAJOR" { n++ } END { print n+0 }' "$DECL")
MAGIC_ROH=$(awk '$1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAGIC" { n++; v=$3 } END { if (n==1) print v }' "$DECL")
MAGIC_N=$(awk '$1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAGIC" { n++ } END { print n+0 }' "$DECL")

[ "$MAJOR_N" = "1" ] || abbruch "COMDARE_ANATOMY_ABI_MAJOR steht $MAJOR_N mal im Header (erwartet: genau 1)." \
    "Datei: $DECL_REL"
[ "$MAGIC_N" = "1" ] || abbruch "COMDARE_ANATOMY_ABI_MAGIC steht $MAGIC_N mal im Header (erwartet: genau 1)." \
    "Datei: $DECL_REL"

case "$MAJOR" in
    ''|*[!0-9]*) abbruch "COMDARE_ANATOMY_ABI_MAJOR ist keine reine Zahl: '$MAJOR'." ;;
esac
if [ "$MAJOR" -gt 9 ]; then
    abbruch "Major $MAJOR passt nicht mehr in die Magic-Kodierung." \
        "Die Magic traegt GENAU EIN Byte fuer die Ziffer (\"COMDA\" A <ziffer> \".\")." \
        "Ab Major 10 muss das Schema selbst entschieden werden -- das ist eine" \
        "Architektur-Entscheidung und keine, die eine Wache still ueberspringen darf."
fi

# Magic-Literal normalisieren: 0x....ULL -> reine Hex-Ziffern, Grossschreibung.
MAGIC_HEX=$(printf '%s' "$MAGIC_ROH" | awk '{ s=toupper($0); sub(/^0X/, "", s); sub(/U*L*$/, "", s); print s }')
MAGIC_ERWARTET="434F4D4441413${MAJOR}2E"
if [ "$MAGIC_HEX" != "$MAGIC_ERWARTET" ]; then
    abbruch "Header widerspricht sich selbst: MAJOR=$MAJOR, aber MAGIC=0x$MAGIC_HEX." \
        "Aus MAJOR $MAJOR folgt die Byte-Folge 0x$MAGIC_ERWARTET (\"COMDA\" 'A' '$MAJOR' '.')." \
        "Die Wache entscheidet sich NICHT fuer eine der beiden Zahlen -- hier ist der" \
        "Header selbst der Befund, und er ist die Quelle aller anderen Pruefungen."
fi

# -- Dateiliste ----------------------------------------------------------------
TMP=$(mktemp -d) || abbruch "mktemp fehlgeschlagen."
trap 'rm -rf "$TMP"' EXIT INT TERM

: > "$TMP/dateien.txt"
if [ $# -gt 0 ]; then
    QUELLE="Aufruf-Argumente"
    for F in "$@"; do
        [ -f "$F" ] || abbruch "uebergebene Datei existiert nicht: $F"
        echo "$F" >> "$TMP/dateien.txt"
    done
else
    QUELLE="Standard-Liste (Hybrid-Doku)"
    echo "$DOKU_STANDARD" | while IFS= read -r D; do
        [ -n "$D" ] || continue
        echo "$WURZEL/$D"
    done >> "$TMP/dateien.txt"
    while IFS= read -r F; do
        [ -f "$F" ] || abbruch "Doku-Datei der Standard-Liste fehlt: $F" \
            "Eine verschwundene Datei darf den Nenner nicht still schrumpfen lassen." \
            "Wurde sie umbenannt, gehoert die Liste in $0 nachgezogen."
    done < "$TMP/dateien.txt"
fi
N_DATEIEN=$(awk 'END { print NR+0 }' "$TMP/dateien.txt")
[ "$N_DATEIEN" -gt 0 ] || abbruch "0 Dateien zu pruefen -- ein leerer Nenner ist kein bestandener Lauf."

# -- Teil-D-Dateiliste: Teil-A-Liste plus POD_ZUSATZ (nur im Standard-Modus; im
#    Koeder-/Einzelmodus prueft Teil D genau die uebergebenen Dateien).
cp "$TMP/dateien.txt" "$TMP/dateien_d.txt"
if [ $# -eq 0 ]; then
    echo "$POD_ZUSATZ" | while IFS= read -r D; do
        [ -n "$D" ] || continue
        [ -f "$WURZEL/$D" ] || abbruch "Teil-D-Datei fehlt: $WURZEL/$D" \
            "Eine verschwundene Datei darf den Nenner nicht still schrumpfen lassen."
        echo "$WURZEL/$D"
    done >> "$TMP/dateien_d.txt"
fi
N_DATEIEN_D=$(awk 'END { print NR+0 }' "$TMP/dateien_d.txt")

# -- Klon-Beschaffenheit EINMAL bestimmen (Teil C, fail-closed): kann git die Frage
#    nicht beantworten, kann diese Wache "fehlendes Objekt" nicht von "flacher Klon"
#    unterscheiden -- dann darf sie nicht gruen werden.
SHALLOW=$(git -C "$WURZEL" rev-parse --is-shallow-repository 2>/dev/null) || SHALLOW=""
[ "$SHALLOW" = "true" ] || [ "$SHALLOW" = "false" ] || abbruch \
    "Klon-Beschaffenheit unbestimmbar (git rev-parse --is-shallow-repository in '$WURZEL')." \
    "Ohne diese Antwort ist 'SHA-Objekt fehlt' nicht von 'Klon ist flach' zu trennen," \
    "und Teil C waere entweder fail-open oder eine Falschklage."

echo "============================================================================="
echo " $NAME -- ABI-ETIKETT- UND ANKER-WACHE"
echo "============================================================================="
echo "Quelle der Wahrheit (FREMD): $DECL_REL"
echo "  lebender Major = $MAJOR   Magic = 0x$MAGIC_HEX   (aus MAJOR gerechnet: 0x$MAGIC_ERWARTET, deckungsgleich)"
echo "Geprueft: $N_DATEIEN Datei(en) aus $QUELLE."

# -- TEIL A: Etiketten einsammeln ---------------------------------------------
# EIN awk-Programm, ein print je Fund (awk beendet jeden print mit ORS -- die
# Verkettungs-Falle fehlender Zeilenenden ist damit strukturell ausgeschlossen).
# Satzformat, TAB-getrennt:
#   KLASSE  DATEI  ZEILE  ART  N  SHA  ANKER  TEXT
# RSTART/RLENGTH SIND GLOBAL -- die Falle, die diese Wache beim ersten Lauf selbst
# gestellt hat (08.08., 99,9 % CPU, kein Fortschritt): match() setzt RSTART/RLENGTH
# GLOBAL, nicht je Funktionsrahmen. Die Schleifen unten rufen zwischen match() und
# `substr(rest, RSTART + RLENGTH)` eine Funktion auf, die SELBST match() benutzt --
# deren FEHLSCHLAG hinterlaesst RSTART=0, RLENGTH=-1. Aus `substr(rest, -1)` wird
# die GANZE Zeichenkette, die Schleife tritt auf der Stelle und laeuft ewig.
# Deshalb wird nach JEDEM match() sofort in lokale st/ln gesichert und danach nur
# noch mit diesen gerechnet.
awk -v LEBEND="$MAJOR" '
    function marker_sha(zeile,   s) {
        # ABI-HISTORIE gegen SHA <hex>   -- Intervall-Regex {7,40} bewusst vermieden
        # (nicht jede awk-Implementierung traegt sie); 7+ Hex-Zeichen ausgeschrieben.
        if (match(zeile, /ABI-HISTORIE gegen SHA [0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]+/)) {
            s = substr(zeile, RSTART, RLENGTH)
            sub(/^ABI-HISTORIE gegen SHA /, "", s)
            return s
        }
        return "-"
    }
    function anker(zeile,   rest, liste, t, st, ln) {
        liste = ""
        rest = zeile
        while (match(rest, /anatomy_module_abi_v1_decl\.hpp:[0-9]+/)) {
            st = RSTART; ln = RLENGTH
            t = substr(rest, st, ln)
            sub(/^anatomy_module_abi_v1_decl\.hpp:/, "", t)
            liste = (liste == "") ? t : liste "," t
            rest = substr(rest, st + ln)
        }
        return (liste == "") ? "-" : liste
    }
    function kurzanker(zeile,   rest, c, st, ln) {
        # Kurzform `:NN` OHNE Dateinamen -- nicht aufloesbar, aber zaehlbar.
        c = 0
        rest = zeile
        while (match(rest, /`:[0-9]+/)) {
            st = RSTART; ln = RLENGTH
            c++
            rest = substr(rest, st + ln)
        }
        return c
    }
    function melde(art, n, dat, zeile, txt,   kl, sha, ank) {
        sha = marker_sha(txt)
        ank = anker(txt)
        if (n == LEBEND)      kl = "LEBEND"
        else if (sha != "-")  kl = "HISTORIE"
        else                  kl = "BEFUND"
        gsub(/\t/, " ", txt)
        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", kl, dat, zeile, art, n, sha, ank, txt
    }
    {
        z = $0
        up = toupper(z)

        # (1) "Major <N>" / "Major-<N>" / "ABI-Major <N>" -- und GROSS geschrieben auch
        # "COMDARE_ANATOMY_ABI_MAJOR <N>". Die Grossform wurde beim T-1-Lauf am 08.08.
        # nachgezogen: sie stand in 20260802-hybrid_tier_stufe_soll_design.md:453 als
        # Tabellen-Inhalt eines Ist-Anker-Blocks und waere der ersten Fassung dieser
        # Wache (nur Kleinschreibung) entgangen -- eine stale Angabe MIT gewandertem
        # Anker, also genau der Fall, gegen den sie gebaut ist. Gesucht wird deshalb in
        # der grossgeschriebenen Kopie der Zeile; gemeldet wird das Original.
        rest = up
        while (match(rest, /MAJOR[ ]*-?[ ]*[0-9]+/)) {
            st = RSTART; ln = RLENGTH
            t = substr(rest, st, ln)
            sub(/^MAJOR[ ]*-?[ ]*/, "", t)
            melde("MAJOR", t + 0, FILENAME, FNR, z)
            rest = substr(rest, st + ln)
        }
        # (2) ".A<N>." -- die Magic-Schreibweise der Doku
        rest = z
        while (match(rest, /\.A[0-9]+\./)) {
            st = RSTART; ln = RLENGTH
            t = substr(rest, st, ln)
            gsub(/[.A]/, "", t)
            melde("MAGIC", t + 0, FILENAME, FNR, z)
            rest = substr(rest, st + ln)
        }
        # (3) Magic als Hex-Literal (dieselbe Aussage, andere Notation)
        rest = up
        while (match(rest, /0X434F4D4441413[0-9]2E/)) {
            st = RSTART; ln = RLENGTH
            t = substr(rest, st + 15, 1)
            melde("MAGIC", t + 0, FILENAME, FNR, z)
            rest = substr(rest, st + ln)
        }
        ka = kurzanker(z)
        if (ka > 0) printf "KURZANKER\t%s\t%s\t-\t-\t-\t%s\t\n", FILENAME, FNR, ka
    }
' $(cat "$TMP/dateien.txt") > "$TMP/funde.txt"

awk -F'\t' '$1=="LEBEND"'   "$TMP/funde.txt" > "$TMP/lebend.txt"
awk -F'\t' '$1=="HISTORIE"' "$TMP/funde.txt" > "$TMP/historie.txt"
awk -F'\t' '$1=="BEFUND"'   "$TMP/funde.txt" > "$TMP/befund_a.txt"
awk -F'\t' '$1=="KURZANKER"' "$TMP/funde.txt" > "$TMP/kurzanker.txt"

N_LEBEND=$(awk 'END { print NR+0 }' "$TMP/lebend.txt")
N_HIST=$(awk 'END { print NR+0 }' "$TMP/historie.txt")
N_BEFUND_A=$(awk 'END { print NR+0 }' "$TMP/befund_a.txt")
N_KURZ=$(awk -F'\t' '{ s += $7 } END { print s+0 }' "$TMP/kurzanker.txt")
N_ANGABEN=$((N_LEBEND + N_HIST + N_BEFUND_A))

[ "$N_ANGABEN" -gt 0 ] || abbruch "0 Major-/Magic-Angaben in $N_DATEIEN Datei(en) gefunden." \
    "Ein leerer Nenner ist kein bestandener Lauf: entweder greift die Suche nicht mehr," \
    "oder die Doku sagt ueber die ABI gar nichts -- beides ist zu melden, nicht zu bestehen."

# -- TEIL A, zweite Haelfte: jede Datei MUSS den lebenden Stand nennen ---------
: > "$TMP/ohne_lebend.txt"
while IFS= read -r F; do
    [ -n "$F" ] || continue
    if ! awk -F'\t' -v f="$F" '$1=="LEBEND" && $2==f { gefunden=1 } END { exit !gefunden }' "$TMP/funde.txt"; then
        echo "$F" >> "$TMP/ohne_lebend.txt"
    fi
done < "$TMP/dateien.txt"
N_OHNE_LEBEND=$(awk 'END { print NR+0 }' "$TMP/ohne_lebend.txt")

# -- TEIL B: LEBEND-Behauptungen im GESAMTEN Repo (VOR Teil C: seine Anker laufen
#    dort mit) -----------------------------------------------------------------
# Nenner FREMD und ohne Liste: git nennt den Bestand. Laeuft die Wache auf einer
# uebergebenen Datei (Koeder), entfaellt Teil B -- er hat dann keinen Bezug.
# Ausgabeformat = das 8-Spalten-Format von Teil A, damit Teil C beide lesen kann.
N_B_DATEIEN=0
N_B_TREFFER=0
N_B_BEFUND=0
: > "$TMP/b_funde.txt"
: > "$TMP/befund_b.txt"
if [ $# -eq 0 ]; then
    if ( cd "$WURZEL" && git ls-files -z >/dev/null 2>&1 ); then
        ( cd "$WURZEL" && git -c core.quotePath=off ls-files ) | "$GREP" -v '^ext/' > "$TMP/bestand.txt" || true
        N_B_DATEIEN=$(awk 'END { print NR+0 }' "$TMP/bestand.txt")
        [ "$N_B_DATEIEN" -gt 0 ] || abbruch "git ls-files nennt 0 Dateien -- Teil B haette keinen Nenner."
        ( cd "$WURZEL" && awk -v LEBEND="$MAJOR" '
            function marker_sha(zeile,   s) {
                if (match(zeile, /ABI-HISTORIE gegen SHA [0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]+/)) {
                    s = substr(zeile, RSTART, RLENGTH)
                    sub(/^ABI-HISTORIE gegen SHA /, "", s)
                    return s
                }
                return "-"
            }
            function anker(zeile,   rest, liste, t, st, ln) {
                liste = ""
                rest = zeile
                while (match(rest, /anatomy_module_abi_v1_decl\.hpp:[0-9]+/)) {
                    st = RSTART; ln = RLENGTH
                    t = substr(rest, st, ln)
                    sub(/^anatomy_module_abi_v1_decl\.hpp:/, "", t)
                    liste = (liste == "") ? t : liste "," t
                    rest = substr(rest, st + ln)
                }
                return (liste == "") ? "-" : liste
            }
            {
                z = $0
                lz = tolower(z)
                # Benannte Grammatik-LISTE (s. Grenze 6). Das nackte "lebend" ist
                # bewusst nicht dabei -- es traefe legitime Saetze.
                if (lz !~ /aktuell/ && lz !~ /lebend(er)?[ ]+(stand|gilt|seit)/ && lz !~ /lebend:/) next
                # ALLE Major-Treffer der Zeile, nicht nur der erste (Randnotiz des
                # Reviews: der erste Treffer entschied, der zweite blieb ungesehen).
                sha = marker_sha(z)
                ank = anker(z)
                zz = z; gsub(/\t/, " ", zz)
                rest = lz
                while (match(rest, /major[ ]*-?[ ]*[0-9]+/)) {
                    st = RSTART; ln = RLENGTH
                    t = substr(rest, st, ln)
                    sub(/^major[ ]*-?[ ]*/, "", t)
                    if (t + 0 == LEBEND + 0)  kl = "LEBEND"
                    else if (sha != "-")      kl = "HISTORIE"
                    else                      kl = "BEFUND"
                    printf "%s\t%s\t%s\tMAJOR\t%s\t%s\t%s\t%s\n", kl, FILENAME, FNR, t + 0, sha, ank, zz
                    rest = substr(rest, st + ln)
                }
            }
        ' $(cat "$TMP/bestand.txt") ) > "$TMP/b_funde.txt" 2>/dev/null || true
        N_B_TREFFER=$(awk 'END { print NR+0 }' "$TMP/b_funde.txt")
        awk -F'\t' '$1=="BEFUND"' "$TMP/b_funde.txt" > "$TMP/befund_b.txt"
        N_B_BEFUND=$(awk 'END { print NR+0 }' "$TMP/befund_b.txt")
    else
        abbruch "Teil B braucht git ls-files in '$WURZEL' -- ohne Bestand kein Nenner."
    fi
fi

# -- C-Futter: Teil-A-Funde plus Teil-B-Funde ausserhalb der Teil-A-Liste --------
# (Teil-B-Pfade sind repo-relativ -> absolut machen; Dateien der Teil-A-Liste sind
#  dort bereits mit denselben Zeilen vertreten und wuerden doppelt gezaehlt.)
awk -F'\t' -v OFS='\t' -v w="$WURZEL" '
    NR==FNR { seen[$0] = 1; next }
    { p = w "/" $2; if (p in seen) next; $2 = p; print }
' "$TMP/dateien.txt" "$TMP/b_funde.txt" > "$TMP/b_funde_extern.txt"
cat "$TMP/funde.txt" "$TMP/b_funde_extern.txt" > "$TMP/anker_quelle.txt"

# -- TEIL C: Anker nachrechnen ------------------------------------------------
# Fuer jede Angabe mit Anker: zeigt die Zeile wirklich auf das behauptete #define?
: > "$TMP/anker_befund.txt"
: > "$TMP/anker_unpruefbar.txt"
N_ANKER=0
N_ANKER_OK=0
N_ANKER_UNPRUEFBAR=0
while IFS="$(printf '\t')" read -r KL DAT ZL ART N SHA ANK TXT; do
    [ "$KL" = "LEBEND" ] || [ "$KL" = "HISTORIE" ] || continue
    [ "$ANK" != "-" ] || continue
    ERW_MAGIC="434F4D4441413${N}2E"
    OLDIFS=$IFS; IFS=,
    for L in $ANK; do
        IFS=$OLDIFS
        N_ANKER=$((N_ANKER + 1))
        if [ "$KL" = "LEBEND" ]; then
            ZEILE=$(sed -n "${L}p" "$DECL" 2>/dev/null || true)
            HERKUNFT="heutiger Header"
        else
            # FAIL-CLOSED (Nachsatz 08.08.): fehlt das SHA-Objekt, entscheidet die
            # Klon-Beschaffenheit. In einem FLACHEN Klon (GitLab GIT_DEPTH) fehlt der
            # KLON, nicht die Aussage -- gezaehlte, benannte Luecke wie bisher. In
            # einem VOLLEN Klon MUSS jedes je gepushte Objekt da sein: ein fehlendes
            # ist erfunden, vertippt oder nie gepusht -- ein harter BEFUND. Vorher
            # lief ein gewuerfeltes 40-Hex-SHA hier als "nicht pruefbar" mit rc=0
            # durch (Koeder-Beleg im Nachsatz-Commit): genau die stille Null.
            if ! git -C "$WURZEL" cat-file -e "${SHA}^{commit}" 2>/dev/null; then
                if [ "$SHALLOW" = "true" ]; then
                    N_ANKER_UNPRUEFBAR=$((N_ANKER_UNPRUEFBAR + 1))
                    echo "  $DAT:$ZL  Anker -> ${DECL_REL}:${L}  (SHA $SHA liegt nicht in diesem FLACHEN Klon)" >> "$TMP/anker_unpruefbar.txt"
                else
                    {
                        echo "  $DAT:$ZL  Anker -> ${DECL_REL}:${L}  (SHA $SHA)"
                        echo "      behauptet: verifizierte Historie gegen SHA $SHA"
                        echo "      dort steht: <kein solches Objekt in diesem VOLLEN Klon -- erfunden, vertippt oder nie gepusht>"
                    } >> "$TMP/anker_befund.txt"
                fi
                IFS=,
                continue
            fi
            ZEILE=$(git -C "$WURZEL" show "${SHA}:${DECL_REL}" 2>/dev/null | sed -n "${L}p") || ZEILE=""
            HERKUNFT="SHA $SHA"
        fi
        # ODER-Treffer: MAJOR-define mit N oder MAGIC-define mit der aus N gerechneten
        # Byte-Folge -- beide kodieren denselben ABI-Stand. Eine Zeile wie "Major 8,
        # Magic .A8. -- Belege :89 und :93" traegt EINE Angabe mit ZWEI Ankern; jeder
        # der beiden ist mit jeder der beiden Formen gedeckt.
        printf '%s\n' "$ZEILE" | awk -v n="$N" -v h="$ERW_MAGIC" '
            $1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAJOR" && $3+0==n+0            { ok=1 }
            $1=="#define" && $2=="COMDARE_ANATOMY_ABI_MAGIC" && toupper($3) ~ ("^0X" h) { ok=1 }
            END { exit !ok }' && TREFFER=1 || TREFFER=0
        ERWARTET="#define COMDARE_ANATOMY_ABI_MAJOR $N  ODER  #define COMDARE_ANATOMY_ABI_MAGIC 0x${ERW_MAGIC}ULL"
        if [ "$TREFFER" -eq 1 ]; then
            N_ANKER_OK=$((N_ANKER_OK + 1))
        else
            {
                echo "  $DAT:$ZL  Anker -> ${DECL_REL}:${L}  ($HERKUNFT)"
                echo "      behauptet: $ERWARTET"
                echo "      dort steht: ${ZEILE:-<Zeile existiert nicht>}"
            } >> "$TMP/anker_befund.txt"
        fi
        IFS=,
    done
    IFS=$OLDIFS
done < "$TMP/anker_quelle.txt"
N_ANKER_BEFUND=$((N_ANKER - N_ANKER_OK - N_ANKER_UNPRUEFBAR))

# -- TEIL D: POD-Masse gegen kV3AxisCount --------------------------------------
# Die Quelle ist wieder FREMD und wieder genau eine: die constexpr-Konstante im
# Observer-Header. Findet sie sich nicht oder mehrfach, ist das ABBRUCH -- eine
# geratene Achsenzahl waere schlimmer als keine Pruefung.
POD_HDR="$WURZEL/libs/cache_engine/anatomy/observable_tier.hpp"
: > "$TMP/befund_d.txt"
N_D_ANGABEN=0
N_D_BEFUND=0
if [ -f "$POD_HDR" ]; then
    ACHSEN=$(awk '/kV3AxisCount[ ]*=[ ]*[0-9]+/ { n++; s=$0; sub(/^.*kV3AxisCount[ ]*=[ ]*/, "", s); sub(/[^0-9].*$/, "", s); v=s } END { if (n==1) print v }' "$POD_HDR")
    ACHSEN_N=$(awk '/kV3AxisCount[ ]*=[ ]*[0-9]+/ { n++ } END { print n+0 }' "$POD_HDR")
    [ "$ACHSEN_N" = "1" ] || abbruch "kV3AxisCount steht $ACHSEN_N mal in observable_tier.hpp (erwartet: genau 1)." \
        "Teil D braucht GENAU EINE Quelle fuer die Achsenzahl; raten ist keine Option."
    case "$ACHSEN" in ''|*[!0-9]*) abbruch "kV3AxisCount ist keine reine Zahl: '$ACHSEN'." ;; esac

    awk -v N="$ACHSEN" '
        function marker(z) {
            return (z ~ /ABI-HISTORIE gegen SHA [0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]+/)
        }
        # muster ist eine ZEICHENKETTE, kein Regex-Literal. Ein /re/ als Argument
        # wuerde in awk zu ($0 ~ /re/) ausgewertet und als "0" oder "1" ankommen --
        # match(rest, "0") trifft dann jede Null der Zeile. Genau das ist beim ersten
        # Lauf am 08.08. passiert: 386 erfundene Befunde, darunter "axis_stats[0]" in
        # einer Zeile ohne jedes axis_stats. Dynamische Regexe gehoeren in Strings.
        # quelle: der Text, in dem gesucht wird (Original oder GROSS-Kopie);
        # soll: der Wert, den die Angabe tragen muss (N bzw. N-1 bei T0..TM);
        # praefix: was vor der Zahl weggeschnitten wird (statt gsub aller
        # Nicht-Ziffern -- "T0..T16" enthaelt ZWEI Zahlengruppen, gsub machte
        # daraus "016").
        function pruefe(feld, muster, quelle, soll, praefix,   rest, st, ln, t, zz) {
            rest = quelle
            while (match(rest, muster)) {
                st = RSTART; ln = RLENGTH        # global, s. Teil A
                t = substr(rest, st, ln)
                sub(praefix, "", t)
                sub(/[^0-9].*$/, "", t)
                gesamt++
                if (t + 0 != soll + 0 && !marker($0)) {
                    zz = $0
                    gsub(/\t/, " ", zz)
                    printf "BEFUND\t%s\t%s\t%s\t%s\t%s\t%s\n", FILENAME, FNR, feld, t + 0, soll + 0, zz
                }
                rest = substr(rest, st + ln)
            }
        }
        {
            up = toupper($0)
            pruefe("axis_stats[N]", "axis_stats\\[[0-9]+\\]", $0, N, "^axis_stats\\[")
            pruefe("seg_ns[N]",     "seg_ns\\[[0-9]+\\]",     $0, N, "^seg_ns\\[")
            # Totalitaets-Behauptung in Prosa: "ALLE 17 Achsen", "ALLER 17
            # SearchAlgorithm-Achsen". Gross-Kopie, damit Gross-/Kleinschreibung
            # keine Rolle spielt; gemeldet wird das Original. Teilmengen ("13 der
            # Achsen") und "die 18 ..." OHNE alle(r) sind bewusst NICHT gedeckt
            # (Grenze 3b) -- ihr Soll ist nicht kV3AxisCount.
            pruefe("ALLE-N-ACHSEN", "ALLER?[ ]+[0-9]+[ ]+[A-Z0-9_-]*ACHSEN", up, N, "^ALLER?[ ]+")
            # Bereichs-Obergrenze: T0..T<M> muss T0..T<kV3AxisCount-1> sein.
            pruefe("T0..TM", "T0\\.\\.T[0-9]+", up, N - 1, "^T0\\.\\.T")
        }
        END { printf "GESAMT\t%d\n", gesamt + 0 }
    ' $(cat "$TMP/dateien_d.txt") > "$TMP/pod.txt"

    N_D_ANGABEN=$(awk -F'\t' '$1=="GESAMT" { s += $2 } END { print s+0 }' "$TMP/pod.txt")
    awk -F'\t' '$1=="BEFUND"' "$TMP/pod.txt" > "$TMP/befund_d.txt"
    N_D_BEFUND=$(awk 'END { print NR+0 }' "$TMP/befund_d.txt")
else
    abbruch "Observer-Header nicht gefunden: $POD_HDR" \
        "Teil D kann ohne kV3AxisCount keine Aussage treffen."
fi

# (Teil B laeuft seit dem Nachsatz 08.08. VOR Teil C -- seine Anker gehen dort mit.)

# -- Ausgabe ------------------------------------------------------------------
if [ "$N_BEFUND_A" -gt 0 ]; then
    echo ""
    echo "A) ETIKETT OHNE DECKUNG -- Zahl weicht vom lebenden Major ab und traegt keinen"
    echo "   Marker 'ABI-HISTORIE gegen SHA <hex>' auf derselben Zeile:"
    awk -F'\t' -v m="$MAJOR" '{ printf "  %s:%s  %s %s (lebend: %s)\n      %s\n", $2, $3, $4, $5, m, $8 }' "$TMP/befund_a.txt"
fi

if [ "$N_OHNE_LEBEND" -gt 0 ]; then
    echo ""
    echo "A2) DOKU OHNE LEBENDEN STAND -- die Datei nennt Major $MAJOR nirgends:"
    echo "    (Eine Doku, die nur markierte Historie traegt, verschweigt den heutigen"
    echo "     Stand. Sie waere formal widerspruchsfrei und trotzdem irrefuehrend.)"
    while IFS= read -r F; do echo "  $F"; done < "$TMP/ohne_lebend.txt"
fi

if [ "$N_ANKER_BEFUND" -gt 0 ]; then
    echo ""
    echo "C) ANKER ZEIGT INS LEERE -- die genannte Zeile traegt nicht, was der Text behauptet:"
    cat "$TMP/anker_befund.txt"
fi

if [ "$N_B_BEFUND" -gt 0 ]; then
    echo ""
    echo "B) LEBEND-BEHAUPTUNG IM REPO WIDERSPRICHT DEM HEADER:"
    awk -F'\t' -v m="$MAJOR" '{ printf "  %s:%s  behauptet Major %s, Header sagt %s\n      %s\n", $2, $3, $5, m, $8 }' "$TMP/befund_b.txt"
fi

if [ "$N_D_BEFUND" -gt 0 ]; then
    echo ""
    echo "D) POD-MASS OHNE DECKUNG -- Kopie einer Zahl, die in kV3AxisCount lebt:"
    awk -F'\t' '{ printf "  %s:%s  %s traegt %s, Soll (aus kV3AxisCount) ist %s\n      %s\n", $2, $3, $4, $5, $6, $7 }' "$TMP/befund_d.txt"
fi

if [ "$N_ANKER_UNPRUEFBAR" -gt 0 ]; then
    echo ""
    echo "NICHT PRUEFBAR IN DIESEM KLON -- $N_ANKER_UNPRUEFBAR Historien-Anker ohne ihr SHA-Objekt:"
    echo "  (Flacher Klon: das Objekt fehlt, die Aussage ist HIER weder bestaetigt noch"
    echo "   widerlegt. Ein voller Klon -- lokal, oder ein Runner mit GIT_DEPTH=0 --"
    echo "   entscheidet sie. Diese Zahl steht bei JEDEM Lauf da, damit sie nicht zur"
    echo "   stillen Null wird.)"
    cat "$TMP/anker_unpruefbar.txt"
fi

if [ "$N_KURZ" -gt 0 ]; then
    echo ""
    echo "NICHT GEDECKT -- $N_KURZ Kurz-Anker der Form \`:NN\` ohne Dateinamen:"
    echo "  (Sie beziehen sich auf eine frueher im Text genannte Datei. Diese Wache"
    echo "   loest den Bezug NICHT auf, statt ihn zu raten. Die Luecke hat damit eine"
    echo "   Zahl statt eines Schweigens.)"
    awk -F'\t' '{ printf "  %s:%s  (%s Stueck)\n", $2, $3, $7 }' "$TMP/kurzanker.txt"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null) -- jede Zahl mit ihrem Zustand:"
echo "  A  Hybrid-Doku, Zustand 'getrackter Baum, wie er auf der Platte liegt':"
echo "     $N_DATEIEN Datei(en), $N_ANGABEN Major-/Magic-Angabe(n) gefunden."
echo "     davon $N_LEBEND lebend (== Major $MAJOR), $N_HIST als Historie markiert,"
echo "     $N_BEFUND_A ohne Deckung; $N_OHNE_LEBEND Datei(en) ohne lebenden Stand."
echo "  C  Anker, Zustand 'Zeile im heutigen Header bzw. im gepinnten SHA-Objekt'"
echo "     (Quelle: Teil-A-Funde plus Teil-B-Funde ausserhalb der Teil-A-Liste; Klon: $( [ "$SHALLOW" = "true" ] && echo flach || echo voll )):"
echo "     $N_ANKER aufloesbare(r) Anker gesehen, $N_ANKER_OK treffen, $N_ANKER_BEFUND nicht"
echo "     (fehlendes SHA-Objekt im vollen Klon zaehlt als Befund, fail-closed),"
echo "     $N_ANKER_UNPRUEFBAR nicht pruefbar (nur im flachen Klon moeglich)."
echo "     $N_KURZ Kurz-Anker \`:NN\` NICHT gedeckt (kein Dateiname im Anker)."
if [ $# -eq 0 ]; then
    echo "  B  Repo, Zustand 'git ls-files ohne ext/, Arbeitsbaum', Grammatik-Liste"
    echo "     aktuell/lebender stand/lebend gilt/lebend seit/lebend: (alle Treffer je Zeile):"
    echo "     $N_B_DATEIEN Datei(en) gescannt, $N_B_TREFFER ausdrueckliche LEBEND-Behauptung(en),"
    echo "     davon $N_B_BEFUND im Widerspruch zum Header."
else
    echo "  B  Repo-Scan uebersprungen (Aufruf mit expliziten Dateien -- Teil B haette"
    echo "     dafuer keinen Bezug). Das ist der Koeder-/Einzelpruefungs-Modus."
fi
echo "  D  POD-Masse, Zustand '$N_DATEIEN_D Datei(en) (Teil-A-Liste + Schwesterdatei) gegen kV3AxisCount = $ACHSEN':"
echo "     $N_D_ANGABEN literale Angabe(n) axis_stats[N]/seg_ns[N]/ALLE-N-Achsen/T0..TM, davon $N_D_BEFUND ohne Deckung."
echo "-----------------------------------------------------------------------------"

GESAMT_BEFUND=$((N_BEFUND_A + N_OHNE_LEBEND + N_ANKER_BEFUND + N_B_BEFUND + N_D_BEFUND))
if [ "$GESAMT_BEFUND" -gt 0 ]; then
    echo "$NAME: FAILED -- $GESAMT_BEFUND Befund(e): $N_BEFUND_A Etikett(en), $N_OHNE_LEBEND Datei(en) ohne" >&2
    echo "        lebenden Stand, $N_ANKER_BEFUND Anker, $N_B_BEFUND LEBEND-Behauptung(en), $N_D_BEFUND POD-Mass(e)." >&2
    echo "        Ein Leser dieser Doku baut gegen eine ABI-Major, die es so nicht gibt." >&2
    echo "        Nachziehen ist ADDITIV: den lebenden Stand ergaenzen, die alte Aussage mit" >&2
    echo "        'ABI-HISTORIE gegen SHA <hex>' kennzeichnen -- nicht stillschweigend ueberschreiben." >&2
    exit 1
fi

echo "$NAME: OK ($N_ANGABEN Angabe(n) in $N_DATEIEN Datei(en), $N_ANKER Anker, lebender Major $MAJOR)."
exit 0
