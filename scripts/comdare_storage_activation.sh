# shellcheck shell=sh
# comdare_storage_activation.sh -- S3 (#46a): das BARE-METAL Gegenstueck zur CI-.storage_cache_activation
# (super .gitlab-ci.yml). Der Dual-Weg (§61) verlangt, dass JEDE Funktion SOWOHL ueber GitLab-CI ALS AUCH lokal
# (cmake + Shell, offizielle Targets) laeuft -- dies aktiviert den Zwei-Ebenen-Storage (ArtifactCache::from_env)
# fuer einen bare-metal Treiber-Lauf auf prod1/prod2 GENAU wie es die CI-Naht im Runner tut.
#
# QUELLE MIT `.` / `source` (NICHT ausfuehren): die env-Exports muessen in die AUFRUFENDE Shell wirken, damit der
# anschliessende cmake-Bau + comdare-messung-driver-Lauf sie sieht (gleiche Shell wie der CI-Folge-Schritt):
#     set -a; . /pfad/zu/secrets.env; set +a      # Operator legt die Roh-Creds NUR hier ab (0600, nie im Repo)
#     . scripts/comdare_storage_activation.sh
#     cmake --build build --target comdare-messung-driver && ./build/.../comdare-messung-driver ...
#
# KONSUMIERTE ENV (NUR NAMEN dokumentiert, NIE Werte -- Sicherheits-Invariante):
#   Schalter:   COMDARE_STORAGE_CACHE            ("true" = scharf; sonst INERT => from_env() No-Op, byte-neutral)
#   Ebene B in: MINIO_ACCESS_KEY MINIO_SECRET_KEY (Roh-Creds; werden in MC_HOST_prodcache gefaltet + danach unset)
#               COMDARE_MINIO_ENDPOINT           (bare-metal = die S3-URL; wird zum mc-ALIAS 'prodcache' umgesetzt)
#               COMDARE_MINIO_BUCKET             (Ziel-Bucket)   COMDARE_MINIO_PREFIX (optional)
#   Ebene C in: COMDARE_MEASUREMENT_DROP_URL COMDARE_NFS_DROP_TOKEN   COMDARE_NFS_DROP_USER (optional)
#   Tuning:     COMDARE_MC_BIN COMDARE_CURL_BIN COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S COMDARE_ARTEFAKT_MAX_TIME_S
#               COMDARE_ARTEFAKT_TRIES COMDARE_ARTEFAKT_RETRY_SLEEP_S COMDARE_ARTEFAKT_PULL_TRIES
#               COMDARE_ARTEFAKT_PULL_RETRY_SLEEP_S
#   Mess-Combo: COMDARE_MEASUREMENT_COMBO (fliesst in den Objekt-Key +mtool; leer/[all] = Default)
# GESETZTE ENV (Ausgabe an den Treiber):
#   MC_HOST_prodcache  (traegt die Creds -- NIE in argv/ps/Log; einziger Cred-Traeger fuer mc)
#   COMDARE_MINIO_ENDPOINT=prodcache  (ArtifactCache liest dies als mc-ALIAS-Namen)
#
# SICHERHEIT: Credentials gehen NIE in argv/ps oder ins Log. mc zieht sie AUSSCHLIESSLICH aus MC_HOST_<alias>
# (Environment), der measure-drop-Token AUSSCHLIESSLICH ueber die 0600-curl-Config im ArtifactCache. Dieses Skript
# echot NIEMALS einen Schluessel/Token/die MC_HOST-URL -- nur den festen Alias-Namen 'prodcache' + Ebenen-Status.
# Fehler (mc fehlt / Env unvollstaendig) => die betroffene Ebene wird INERT, der Betrieb laeuft WEITER (nie Abbruch;
# Fehlerklassen-Doktrin). Source-sicher: KEIN exit, KEIN 'set -e' (wuerde die aufrufende Shell beeinflussen).

if [ "${COMDARE_STORAGE_CACHE:-}" = "true" ]; then
    echo "== Storage bare-metal scharf (COMDARE_STORAGE_CACHE=true) =="
    # Ebene B (minio): bare-metal traegt COMDARE_MINIO_ENDPOINT die S3-URL -> Alias 'prodcache' bauen, Var auf Alias.
    if command -v "${COMDARE_MC_BIN:-mc}" >/dev/null 2>&1; then
        if [ -n "${MINIO_ACCESS_KEY:-}" ] && [ -n "${MINIO_SECRET_KEY:-}" ] && [ -n "${COMDARE_MINIO_ENDPOINT:-}" ] &&
            [ -n "${COMDARE_MINIO_BUCKET:-}" ]; then
            _mh="${COMDARE_MINIO_ENDPOINT#https://}"
            _mh="${_mh#http://}" # nur Host:Port -> in die MC_HOST-URL
            MC_HOST_prodcache="https://${MINIO_ACCESS_KEY}:${MINIO_SECRET_KEY}@${_mh}"
            export MC_HOST_prodcache
            COMDARE_MINIO_ENDPOINT="prodcache" # ArtifactCache liest dies als mc-ALIAS-Namen
            export COMDARE_MINIO_ENDPOINT
            unset MINIO_ACCESS_KEY MINIO_SECRET_KEY _mh # Roh-Creds aus der Shell nehmen (nur noch im MC_HOST-env)
            echo "  Ebene B aktiv: mc-Alias=prodcache (Bucket-Var COMDARE_MINIO_BUCKET gesetzt; Creds via MC_HOST_prodcache, nie argv/Log)"
        else
            echo "  [Infra-Fehler: minio-Env unvollstaendig (MINIO_ACCESS_KEY/SECRET_KEY/COMDARE_MINIO_ENDPOINT/_BUCKET) -> Ebene B inert, Betrieb laeuft weiter]"
            unset COMDARE_MINIO_ENDPOINT COMDARE_MINIO_BUCKET 2>/dev/null || true
        fi
    else
        echo "  [Infra-Fehler: mc-Binary fehlt (COMDARE_MC_BIN/PATH) -> Ebene B inert, Betrieb laeuft weiter]"
        unset COMDARE_MINIO_ENDPOINT COMDARE_MINIO_BUCKET 2>/dev/null || true
    fi
    # Ebene C (measure-drop HTTPS-PUT): aktiv nur wenn beide Vars da (Token nie in argv; ArtifactCache 0600-curl-config).
    if [ -n "${COMDARE_MEASUREMENT_DROP_URL:-}" ] && [ -n "${COMDARE_NFS_DROP_TOKEN:-}" ]; then
        echo "  Ebene C aktiv: measure-drop gesetzt (Var COMDARE_MEASUREMENT_DROP_URL; Token COMDARE_NFS_DROP_TOKEN nie geloggt)"
    else
        echo "  [Info: measure-drop-Env fehlt (COMDARE_MEASUREMENT_DROP_URL/COMDARE_NFS_DROP_TOKEN) -> Ebene C inert; Git-Writeback bleibt Persistenz-Pfad]"
        unset COMDARE_MEASUREMENT_DROP_URL COMDARE_NFS_DROP_TOKEN 2>/dev/null || true
    fi
else
    echo "== Storage bare-metal INERT (COMDARE_STORAGE_CACHE != true) -> from_env() No-Op, byte-neutral =="
fi
