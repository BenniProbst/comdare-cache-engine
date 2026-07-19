#!/usr/bin/env bash
# DEPRECATED (W9.5/G4, 2026-07-19) -- NICHT LOESCHEN, nur als Referenz erhalten.
#   Superseded durch die CI-Mess-Rueckschreibungs-Pipeline (measurement/ + WRITE-Token, autonom persistiert).
#   Zusaetzlich obsolet: der UNC-Zielhost \\backup1.comdare.de ist abgeschaltet (.de -> .local). Manuelle NAS-Ablage
#   ist nur noch Notfall-Fallback; der offizielle Weg schreibt die Roh-CSVs ueber die Pipeline zurueck.
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
