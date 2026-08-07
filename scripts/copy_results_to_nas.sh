#!/usr/bin/env bash
# DEPRECATED (W9.5/G4, 2026-07-19) -- NICHT LOESCHEN, nur als Referenz erhalten.
#   Superseded durch die CI-Mess-Rueckschreibungs-Pipeline (measurement/ + WRITE-Token, autonom persistiert).
#   Manuelle NAS-Ablage ist nur noch Notfall-Fallback; der offizielle Weg schreibt die Roh-CSVs ueber die
#   Pipeline zurueck (Owner-Entscheid A vom 2026-07-18: measure-drop HTTPS-PUT, KEIN POSIX-NFS-Mount).
#
# KORREKTUR 2026-08-07 (Owner-Wort + eigene Messung): der Satz "der UNC-Zielhost \\backup1.comdare.de ist
#   abgeschaltet (.de -> .local)" stand hier und war FALSCH. Am 2026-08-07 von prod1 nachgemessen:
#   `getent hosts backup1.comdare.de` -> 10.0.10.243, `curl -I https://backup1.comdare.de/` -> HTTP/2 200.
#   backup1 ist gesund und in Betrieb. Die ".de -> .local"-Umstellung vom 2026-07-13 betraf AUSSCHLIESSLICH
#   die GitLab-Domain (die HAProxy-SNI-Route fuer .de mappt nicht mehr aufs GitLab-Backend) -- nicht den
#   eigenen A-Record und nicht die eigene SNI-Route von backup1. Der Grund, warum dieses Skript deprecated
#   bleibt, ist ALLEIN der Owner-Entscheid oben, NICHT ein toter Host.
#
# ZWEI GERAETE, NICHT EINS -- die Namen sind leicht zu verwechseln, die Kapazitaeten unterscheiden sich stark:
#   backup1 = WD PR4100 (4-Bay, prod-Realm), V20 10.0.20.241, VIP 10.0.10.243, backup1.comdare.de
#   backup2 = WD PR2100 (2-Bay, ARM, dev-Realm),  V20 10.0.20.242, VIP 10.0.10.244, backup2.comdare.local
#   BEIDE exportieren einen Pfad namens `Cluster_NFS` -- wer nur den Freigabenamen prueft, trifft das
#   falsche Geraet. Immer die IP mitfuehren.
# copy_results_to_nas.sh — robuste NAS-Ablage der Mess-Roh-CSVs (User-Direktive 2026-06-08).
#
# Das NAS (\\backup1.comdare.de\Cluster_NFS\experiment results) unterstützt nur UNC-Pfade + bash, und die
# Verbindung hängt zeitweise einige Sekunden → JEDE Ablage läuft als Retry-Schleife mit Ziel-Größen-
# Verifikation; die lokale benannte Quell-Kopie bleibt IMMER als Fallback erhalten (wird nie gelöscht).
#
# Aufruf (Git-Bash):  bash scripts/copy_results_to_nas.sh <lokale-datei> [ziel-verzeichnis-unc]
# Exit 0 = Kopie auf dem NAS größen-verifiziert; Exit 1 = nach allen Versuchen nicht verifiziert
# (lokale Datei unangetastet — späterer erneuter Aufruf genügt).

set -u

SRC="${1:?Aufruf: copy_results_to_nas.sh <lokale-datei> [ziel-unc-verzeichnis]}"
DEST_DIR="${2:-//backup1.comdare.de/Cluster_NFS/experiment results}"
TRIES="${COMDARE_NAS_TRIES:-12}"
SLEEP_S="${COMDARE_NAS_SLEEP:-20}"

if [ ! -f "$SRC" ]; then echo "FEHLER: Quelldatei fehlt: $SRC"; exit 1; fi
BASE="$(basename "$SRC")"
SRC_SIZE="$(stat -c %s "$SRC")"
echo "NAS-Ablage: $BASE ($SRC_SIZE Bytes) -> $DEST_DIR  (max $TRIES Versuche, ${SLEEP_S}s Pause)"

for i in $(seq 1 "$TRIES"); do
    # Verzeichnis-Probe (weckt eine eingeschlafene SMB-Session) + Kopie + Verifikation. Jeder Schritt darf
    # an der instabilen Verbindung scheitern — dann Pause + nächster Versuch.
    if ls "$DEST_DIR" >/dev/null 2>&1 && cp "$SRC" "$DEST_DIR/$BASE" 2>/dev/null; then
        DST_SIZE="$(stat -c %s "$DEST_DIR/$BASE" 2>/dev/null || echo -1)"
        if [ "$DST_SIZE" = "$SRC_SIZE" ]; then
            echo "OK: $DEST_DIR/$BASE groessen-verifiziert ($DST_SIZE Bytes, Versuch $i/$TRIES)"
            exit 0
        fi
        echo "Versuch $i/$TRIES: Groesse weicht ab (src=$SRC_SIZE dst=$DST_SIZE) — erneut"
    else
        echo "Versuch $i/$TRIES: NAS nicht erreichbar/Kopie fehlgeschlagen — warte ${SLEEP_S}s"
    fi
    sleep "$SLEEP_S"
done

echo "FEHLER: NAS-Ablage nach $TRIES Versuchen NICHT verifiziert — lokale Kopie bleibt erhalten: $SRC"
exit 1
