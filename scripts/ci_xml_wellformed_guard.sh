#!/bin/sh
# ci_xml_wellformed_guard.sh -- XML-Wohlgeformtheits-Wache (2026-08-08).
#
# WORUM ES GEHT: am 08.08. waren FUENF getrackte XML-Dateien in den beiden Repos nicht
# wohlgeformt, darunter das golden-Profil der Abgabe-Messung
# (algorithm_profiles/thesis_profiles/all_axes_golden.profile.xml). Kein Job, kein Test und
# keine Wache hat das gemeldet. Die Dateien lagen ueber Wochen kaputt im Baum.
#
# ALLE FUENF trugen dieselbe Fehlerklasse: ein "--" INNERHALB eines XML-Kommentars. XML
# verbietet das ausnahmslos (XML 1.0, Abschnitt 2.5: der Text eines Kommentars darf die
# Zeichenfolge "--" nicht enthalten). Zwei der Dateien sind MASCHINELL erzeugt, ihre
# Generatoren haben den Defekt mitgeschrieben -- beide wurden im selben Zug mitgeheilt
# (libs/cache_engine/profile_facade/h2_score_akte.hpp,
# tools/system_axis_registry_gen/main.cpp). Diese Wache faengt den Rueckfall.
#
# WAS SIE PRUEFT: JEDE getrackte *.xml des Repos, nicht eine Auswahl. Die bestehende
# XML-Maschinerie im super-Repo (Code/tests/xml_canonical_utils.cmake) liest ebenfalls ueber
# xmllint, aber sie bedient GENAU ZWEI Fixture-Wachen mit fest benannten Dateien. Genau
# deshalb ist ihr keine der fuenf kaputten Dateien aufgefallen: sie kamen in ihrer Liste
# nicht vor. Diese Wache hat keine Liste -- sie fragt git nach dem Bestand.
#
# DER NENNER (Hausregel): die Wache nennt IMMER, wie viele Dateien sie geprueft hat. Eine
# nackte Null ohne Nenner meldet Vollstaendigkeit und deckt nichts. Deshalb sind hier ZWEI
# Faelle ausdruecklich ein ABBRUCH (rc=2) und kein Gruen:
#   (a) xmllint fehlt      -> die Wache kann keine Aussage treffen. Ein Skip waere das stille
#                             Abschalten der Wache (dieselbe Doktrin, die
#                             xml_canonical_utils.cmake fuer sich schon festgeschrieben hat).
#   (b) NULL Dateien       -> ein leerer Nenner ist kein bestandener Lauf, sondern ein Zeichen,
#                             dass die Aufzaehlung nicht greift (falsches Verzeichnis, kein
#                             git-Baum, Pfad-Muster kaputt).
#
# DER STDERR-RIEGEL (uebernommen aus Code/tests/xml_canonical_utils.cmake, Befund B14-NB4/B3):
# xmllint kann sich ueber ein Dokument beschweren und TROTZDEM mit rc=0 enden -- ein per
# --nonet blockierter externer Entity-Load meldet sich AUSSCHLIESSLICH auf stderr. Wer nur den
# Rueckgabewert liest, laesst diese Klasse gruen durch. Die Regel ist deshalb absolut:
# bei rc=0 MUSS stderr LEER sein. Am Objekt geprueft -- eine wohlgeformte Datei erzeugt
# 0 Byte stderr (auch thesis/_archiv_entwurf1/main.run.xml mit seinem internen DOCTYPE).
#
# VOLLSTAENDIGKEIT DES NENNERS: die Dateiliste wird zusaetzlich gegen die NUL-getrennte
# Ausgabe derselben git-Abfrage gegengezaehlt. Weichen die Zahlen ab, enthaelt ein Dateiname
# ein Zeilenende, die zeilenweise Schleife wuerde ihn zerreissen und der Nenner waere still
# zu klein -- dann bricht die Wache ab, statt eine zu kleine Zahl zu melden.
#
# SUBMODULE: `git ls-files` steigt nicht in Submodule ab. Das ist gewollt -- das ce lintet
# seinen Bestand in SEINER Pipeline, das super seinen in seiner (dasselbe Schnittmuster wie
# bei lint:diff-hygiene im super-Repo).
#
# WERKZEUG: /usr/bin/grep explizit (kein PATH-Alias, keine ugrep-Falle -- FALLEN-REGISTER:
# ugrep liefert auf manchen Mustern eine STILLE Null auf beiden Engines).
#
# KEIN PIPE-IN-DIE-SCHLEIFE: der Zaehler wird in der Schleife hochgezaehlt. Eine Pipe wuerde
# die Schleife in eine Subshell legen und den Zaehler beim Verlassen verwerfen (die Wache
# meldete dann immer 0 kaputte Dateien). Die Liste kommt deshalb per Here-Doc.
#
# AUFRUF:
#   sh scripts/ci_xml_wellformed_guard.sh   (aus der Repo-Wurzel)
#   rc=0 alles wohlgeformt | rc=1 mindestens eine kaputt | rc=2 Abbruch (s.o.)
#
# POSIX-sh, kein bash-ismus, kein Python (Hausdoktrin: kein Python im Buildsystem).

set -eu

GREP="/usr/bin/grep"
NAME="ci_xml_wellformed_guard"

[ -x "$GREP" ] || { echo "$NAME: ABBRUCH -- $GREP fehlt (POSIX-Grep vorausgesetzt)."; exit 2; }

# Die Wurzel kommt von git selbst, NICHT aus `cd "$(dirname "$0")/.."`. Grund am Objekt gelernt:
# faellt `dirname` aus (fehlendes Werkzeug, kaputter PATH), liefert die Ersetzung einen leeren
# String, `cd "/.."` landet lautlos auf `/` -- und die Wache liefe gegen den FALSCHEN Baum, ohne
# dass set -e anschlaegt (cd war ja erfolgreich). git ist ohnehin die Quelle des Nenners; sie hier
# auch als Wurzelquelle zu nehmen, entfernt die Abhaengigkeit und die stille Fehlbahn zugleich.
WURZEL=$(git rev-parse --show-toplevel 2>/dev/null || true)
if [ -z "$WURZEL" ]; then
    echo "$NAME: ABBRUCH -- kein git-Arbeitsbaum (aus '$(pwd)' heraus aufgerufen). Der Bestand"
    echo "                 wird ueber 'git ls-files' aufgezaehlt; ohne git gibt es keinen"
    echo "                 belastbaren Nenner, also auch kein Gruen."
    exit 2
fi
cd "$WURZEL" || { echo "$NAME: ABBRUCH -- Wechsel in die Repo-Wurzel '$WURZEL' fehlgeschlagen."; exit 2; }

XMLLINT=$(command -v xmllint 2>/dev/null || true)
if [ -z "$XMLLINT" ]; then
    echo "$NAME: ABBRUCH -- xmllint (libxml2) wurde im PATH NICHT gefunden."
    echo "  Diese Wache liest XML ausschliesslich ueber einen echten Parser; ohne ihn kann sie"
    echo "  KEINE Aussage treffen. Ein stiller Skip waere das Abschalten der Wache, nicht ihr"
    echo "  Bestehen. Nachinstallieren: Debian/Ubuntu 'libxml2-utils', RHEL/Fedora/SUSE"
    echo "  'libxml2', Alpine 'libxml2-utils', Arch 'libxml2'."
    exit 2
fi

LISTE=$(git -c core.quotePath=off ls-files '*.xml')
ANZ=$(printf '%s\n' "$LISTE" | "$GREP" -c . || true)
[ -n "$ANZ" ] || ANZ=0

# Gegenzaehlung ueber die NUL-getrennte Ausgabe: deckt Dateinamen mit Zeilenende auf.
ANZ_NUL=$(git -c core.quotePath=off ls-files -z '*.xml' | tr -dc '\0' | wc -c | tr -d ' ')
[ -n "$ANZ_NUL" ] || ANZ_NUL=0

if [ "$ANZ" -ne "$ANZ_NUL" ]; then
    echo "$NAME: ABBRUCH -- Nenner nicht vertrauenswuerdig: zeilenweise $ANZ, NUL-getrennt $ANZ_NUL."
    echo "  Ein Dateiname enthaelt ein Zeilenende. Die Wache wuerde einen zu kleinen Nenner"
    echo "  melden und damit Vollstaendigkeit vortaeuschen."
    exit 2
fi

if [ "$ANZ" -eq 0 ]; then
    echo "$NAME: ABBRUCH -- NULL getrackte *.xml gefunden."
    echo "  Ein leerer Nenner ist kein bestandener Lauf. Entweder laeuft die Wache im falschen"
    echo "  Verzeichnis, oder die Aufzaehlung greift nicht mehr."
    exit 2
fi

echo "$NAME: pruefe $ANZ getrackte *.xml mit $XMLLINT ..."
echo ""
echo "NICHT WOHLGEFORMTE DATEIEN:"

KAPUTT=0
while IFS= read -r DATEI; do
    [ -n "$DATEI" ] || continue
    [ -f "$DATEI" ] || continue   # im Index, im Baum nicht da (z.B. sparse checkout)
    if AUSGABE=$("$XMLLINT" --noout --nonet "$DATEI" 2>&1); then
        RC=0
    else
        RC=$?
    fi
    if [ "$RC" -ne 0 ]; then
        KAPUTT=$((KAPUTT + 1))
        echo "  $DATEI  (xmllint rc=$RC)"
        printf '%s\n' "$AUSGABE" | "$GREP" -m3 . | sed 's/^/      /' || true
    elif [ -n "$AUSGABE" ]; then
        KAPUTT=$((KAPUTT + 1))
        echo "  $DATEI  (rc=0, aber stderr NICHT leer -- FALSE-GREEN, s. Kopf-Kommentar)"
        printf '%s\n' "$AUSGABE" | "$GREP" -m3 . | sed 's/^/      /' || true
    fi
done <<LISTE_ENDE
$LISTE
LISTE_ENDE

[ "$KAPUTT" -eq 0 ] && echo "  (keine)"
echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  $ANZ XML-Dateien geprueft, $KAPUTT nicht wohlgeformt."
echo "-----------------------------------------------------------------------------"

if [ "$KAPUTT" -gt 0 ]; then
    echo "$NAME: FAILED -- $KAPUTT von $ANZ getrackten XML-Dateien sind nicht wohlgeformt."
    exit 1
fi
echo "$NAME: OK ($ANZ XML-Dateien geprueft, alle wohlgeformt)."
exit 0
