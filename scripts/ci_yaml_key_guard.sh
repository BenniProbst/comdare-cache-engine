# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- SCHLUESSEL-WACHE fuer .gitlab-ci.yml (R4, 2026-08-06)
# =============================================================================
# BEHAUPTUNG, DIE HIER GEPRUEFT WIRD (die Invariante):
#
#     Jeder Top-Level-Schluessel in .gitlab-ci.yml ist GENAU EINMAL definiert.
#
# BEFUND, DER DIESE WACHE ERZWINGT (2026-08-06): 'contract:axis-version-lock'
# stand ZWEIMAL in der Datei. YAML-Mappings sind aber Abbildungen: bei einem
# doppelten Schluessel gewinnt kommentarlos die LETZTE Definition, die erste
# wird verworfen. Live nachgerechnet: 31 Schluessel-Zeilen in der Datei, 30
# Schluessel nach dem Parsen -- einer war spurlos weg.
#
# WARUM DAS SO GEFAEHRLICH IST -- es ist ein Ausfall OHNE rotes Signal:
#   * GitLab meldet KEINEN Fehler. Der Lint bleibt gruen, die Pipeline bleibt
#     gruen. Es sieht exakt so aus wie ein gesunder Zustand.
#   * Verloren geht die Definition, die im Text WEITER OBEN steht -- also
#     typischerweise die, die jemand spaeter bewusst eingefuegt hat, weil ein
#     Gate verschaerft werden sollte. Der Verschaerfung geht damit genau die
#     Wirkung verloren, um derentwillen sie geschrieben wurde.
#   * Im konkreten Fall verlor ein UNBEDINGTES Gate gegen eine aeltere,
#     rules-gegatete INERT-Fassung: der Tripwire war ab dem Tag seiner
#     "Verschaerfung" faktisch abgeschaltet, drei Wochen lang, gruen.
#
# Das ist dieselbe Fehlerklasse wie die Abdeckungsluecke, die
# scripts/ci_test_coverage_guard.sh erzwungen hat (Bestandsfehler unter gruener
# CI): etwas ist gebaut und gemeint, laeuft aber nicht -- und nichts wird rot.
# Dort war die Frage "faehrt zu jedem Test ein Job?", hier ist sie "ueberlebt
# jeder Job ueberhaupt das Parsen?". Die Schluessel-Wache sitzt damit eine
# Stufe FRUEHER als die Abdeckungs-Wache: sie prueft den Bestand der Jobs,
# bevor die Abdeckungs-Wache prueft, was die Jobs abdecken.
#
# WIE (und warum das nicht raten kann):
#   * Die Eingabe ist die Gesamtmenge aller Top-Level-Zeilen der Datei, nicht
#     eine gepflegte Liste. Ein neuer Job ist damit ohne Zutun Teil der
#     Pruefung -- er kann nicht "vergessen" werden.
#   * Der Schluessel wird nach der YAML-Regel fuer Block-Mappings abgegrenzt:
#     Schluessel-Ende ist der ERSTE Doppelpunkt, dem ein Leerzeichen oder das
#     Zeilenende folgt. Nur so ist 'contract:axis-version-lock:' EIN Schluessel
#     und nicht 'contract' -- der naive Schnitt am ersten Doppelpunkt wirft
#     10 verschiedene contract-Jobs in einen Topf und meldet Phantom-Treffer.
#   * Zeilen mit Einrueckung koennen definitionsgemaess keine Top-Level-
#     Schluessel sein (auch Block-Skalar-Inhalt ist immer tiefer eingerueckt
#     als sein Schluessel und faellt damit heraus).
#   * KEINE STILLE LUECKE: eine Top-Level-Zeile, die die Wache nicht als
#     Schluessel deuten kann, ist ABBRUCH (Exit 2), nicht "unauffaellig".
#     Die Wache kann also nicht dadurch gruen bleiben, dass sie wegsieht.
#
# GRENZE, EHRLICH BENANNT: die Wache lebt in der Datei, die sie prueft. Wird
# der Job, der sie ruft, SELBST dupliziert und von einer inerten Fassung
# ueberschrieben, faehrt sie nicht mehr. Vollstaendig loesbar ist das nur
# ausserhalb der Datei; was hier erreichbar ist, ist erreicht: jede Duplizierung
# EINES ANDEREN Schluessels faellt auf, und die eigene faellt auf, solange die
# gewinnende Fassung den Job noch fahren laesst.
#
# AUFRUF:  sh scripts/ci_yaml_key_guard.sh [yaml-datei ...]   (Default: .gitlab-ci.yml)
# EXIT:    0 = Invariante haelt | 1 = doppelter Schluessel (Name + Zeilen im Klartext)
#          2 = Bedienung/Umgebung/undeutbare Zeile
#
# KEIN Python, kein yq, keine Fremdbibliothek -- POSIX sh + awk (Leitplanke:
# der Buildchain-Kanon verbietet Python, und eine Wache darf keine Abhaengigkeit
# einschleppen, die auf einem Runner fehlen kann).
# ASCII-only (Leitplanke).
# =============================================================================

set -u

ce_abbruch() {
    echo ""
    echo "SCHLUESSEL-WACHE: ABBRUCH -- $1" >&2
    exit 2
}

command -v awk >/dev/null 2>&1 || ce_abbruch "awk ist nicht im PATH."

if [ "$#" -eq 0 ]; then
    set -- .gitlab-ci.yml
fi

echo "============================================================================="
echo " SCHLUESSEL-WACHE fuer YAML-Top-Level  (scripts/ci_yaml_key_guard.sh)"
echo "============================================================================="

CE_RC=0

for _ce_datei in "$@"; do
    [ -f "$_ce_datei" ] || ce_abbruch "Datei nicht gefunden: ${_ce_datei}"

    echo ""
    echo "PRUEFLING: ${_ce_datei}"

    # -- Schluessel-Zeilen extrahieren: "<zeilennr><TAB><schluessel>" --
    # Abgrenzung nach YAML-Block-Mapping-Regel: erster ':' mit folgendem
    # Leerzeichen/Tab oder Zeilenende beendet den Schluessel.
    _ce_keys=$(awk '
        # Nur Spalte-0-Zeilen sind Top-Level-Kandidaten.
        /^[[:space:]]/ { next }
        /^$/           { next }
        /^#/           { next }
        /^---[[:space:]]*$/ { next }
        /^\.\.\.[[:space:]]*$/ { next }
        {
            zeile = $0
            sub(/\r$/, "", zeile)
            ende = 0
            for (i = 1; i <= length(zeile); i++) {
                if (substr(zeile, i, 1) == ":") {
                    naechstes = substr(zeile, i + 1, 1)
                    if (naechstes == "" || naechstes == " " || naechstes == "\t") {
                        ende = i
                        break
                    }
                }
            }
            if (ende == 0) {
                # Undeutbare Top-Level-Zeile -> lieber Abbruch als stilles Uebergehen.
                printf "UNDEUTBAR\t%d\t%s\n", NR, zeile
                next
            }
            printf "KEY\t%d\t%s\n", NR, substr(zeile, 1, ende - 1)
        }
    ' "$_ce_datei") || ce_abbruch "awk-Lauf ueber '${_ce_datei}' fehlgeschlagen."

    _ce_undeutbar=$(printf '%s\n' "$_ce_keys" | grep '^UNDEUTBAR' || true)
    if [ -n "$_ce_undeutbar" ]; then
        echo ""
        echo "-----------------------------------------------------------------------------"
        echo "ABBRUCH: Top-Level-Zeile ohne erkennbaren Schluessel -- die Wache haette hier"
        echo "eine Luecke, also meldet sie sich statt gruen zu bleiben:"
        printf '%s\n' "$_ce_undeutbar" | while IFS="$(printf '\t')" read -r _ce_m _ce_n _ce_t; do
            echo "  * ${_ce_datei}:${_ce_n}: ${_ce_t}"
        done
        echo "-----------------------------------------------------------------------------"
        exit 2
    fi

    _ce_nur_keys=$(printf '%s\n' "$_ce_keys" | grep '^KEY' || true)
    _ce_anzahl=$(printf '%s\n' "$_ce_nur_keys" | grep -c '^KEY' || true)
    [ "$_ce_anzahl" -gt 0 ] || ce_abbruch "keine Top-Level-Schluessel in '${_ce_datei}' gefunden -- leer oder kein Block-Mapping."

    _ce_eindeutig=$(printf '%s\n' "$_ce_nur_keys" | cut -f3 | LC_ALL=C sort -u | wc -l | tr -d ' ')
    echo "  Top-Level-Schluessel-Zeilen : ${_ce_anzahl}"
    echo "  davon verschiedene Namen    : ${_ce_eindeutig}"

    _ce_dupes=$(printf '%s\n' "$_ce_nur_keys" | cut -f3 | LC_ALL=C sort | uniq -d)

    if [ -n "$_ce_dupes" ]; then
        echo ""
        echo "-----------------------------------------------------------------------------"
        echo "FEHLER: DOPPELTER TOP-LEVEL-SCHLUESSEL in ${_ce_datei}."
        echo "YAML verwirft bei doppeltem Schluessel kommentarlos ALLE Definitionen"
        echo "ausser der LETZTEN. GitLab meldet das nicht: die Pipeline bleibt gruen,"
        echo "waehrend die verworfene Definition wirkungslos in der Datei steht."
        echo ""
        printf '%s\n' "$_ce_dupes" | while read -r _ce_k; do
            [ -n "$_ce_k" ] || continue
            echo "  DOPPELT: '${_ce_k}'"
            _ce_zeilen=$(printf '%s\n' "$_ce_nur_keys" | awk -F"\t" -v k="$_ce_k" '$3 == k { print $2 }')
            _ce_letzte=$(printf '%s\n' "$_ce_zeilen" | tail -1)
            printf '%s\n' "$_ce_zeilen" | while read -r _ce_z; do
                [ -n "$_ce_z" ] || continue
                if [ "$_ce_z" = "$_ce_letzte" ]; then
                    echo "    ${_ce_datei}:${_ce_z}   <- GEWINNT (diese Fassung laeuft)"
                else
                    echo "    ${_ce_datei}:${_ce_z}   <- VERWORFEN (steht da, wirkt aber nicht)"
                fi
            done
        done
        echo ""
        echo "SO WIRD DAS BEHOBEN (nicht durch Loeschen der schwaecheren Fassung):"
        echo "  * Pruefen, WAS die verworfene Fassung tat, das die gewinnende NICHT tut"
        echo "    (haeufig: ein rules-Gate, das die eine Fassung inert macht, und ein"
        echo "    Fehlen desselben Gates in der anderen -- dann ist die STRENGERE die"
        echo "    gemeinte)."
        echo "  * Beide zu EINER Definition zusammenfuehren, die die volle Pruefwirkung"
        echo "    beider behaelt -- oder, wenn sie wirklich Verschiedenes pruefen, in"
        echo "    ZWEI verschieden benannte Jobs trennen."
        echo "-----------------------------------------------------------------------------"
        CE_RC=1
    else
        echo "  -> kein doppelter Schluessel."
    fi
done

echo ""
if [ "$CE_RC" -eq 0 ]; then
    echo "SCHLUESSEL-WACHE: GRUEN -- jeder Top-Level-Schluessel genau einmal definiert."
else
    echo "SCHLUESSEL-WACHE: ROT."
fi

exit "$CE_RC"
