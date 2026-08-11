#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  comdare-cache-engine -- STUFEN-TOPOLOGIE-WACHE ueber .gitlab-ci.yml
#  Zaehlt die needs-Kanten und die VORWAERTS-Kanten.            (2026-08-11)
# =============================================================================
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST:
# GitLab weist ein 'needs' auf eine SPAETERE Stufe hart ab -- die Pipeline wird
# gar nicht erst erzeugt. Das ist kein roter Job, den man im Log findet, sondern
# eine Pipeline, die nicht existiert. Wer die Stufen-Reihenfolge aendert, kann
# das ausloesen, ohne eine einzige needs-Zeile anzufassen.
# Genau diese Aenderung steht in diesem Commit an: 'test' wandert vor 'contract',
# damit test:coverage-guard die Inventur von test:unit ueberhaupt anfordern KANN.
# Die Begruendung dafuer lautete "alle Kanten zeigen auf lint:secrets, also macht
# der Tausch keine Kante vorwaerts". Das ist eine ZAHL, keine Meinung -- und
# solange sie nur im Kommentar steht, altert sie still mit. Diese Wache rechnet
# sie bei jedem Lauf nach.
#
# WAS SIE ZUSICHERT:
#   R1  Jede needs-Kante zeigt auf eine Stufe, die NICHT SPAETER ist als die des
#       fordernden Jobs. Gemeldet wird "N Kanten, M vorwaerts" -- mit Nenner.
#   R2  Die Stufe JEDES an einer Kante beteiligten Jobs ist aufloesbar. Ist sie
#       es nicht, bricht die Wache ab (rc=2) und nennt den Job. Ein Job, dessen
#       Stufe unbekannt ist, koennte die Vorwaerts-Kante gerade verbergen --
#       ihn zu ueberspringen waere die stille Null.
#   R3  Der Bestand ist nicht leer: 0 Kanten waeren zwangslaeufig 0 vorwaerts.
#       Kein Nenner, kein Freispruch -> rc=2.
#
# WAS SIE NICHT PRUEFT -- ausdruecklich:
#   * ob ein needs-Ziel ueberhaupt existiert. Das weist GitLab ebenfalls ab, es
#     ist aber eine andere Fehlerklasse (Name statt Reihenfolge).
#   * 'needs' mit 'pipeline:'/'project:' (Kanten in FREMDE Pipelines). Sie haben
#     keine Stufe in dieser Datei; sie werden GEZAEHLT und ausdruecklich als
#     'nicht bewertbar' ausgewiesen, nicht stillschweigend weggelassen.
#   * YAML-Anker und !reference. Der Parser liest Zeilen, keinen YAML-Baum --
#     dieselbe Grenze wie bei scripts/ci_yaml_key_guard.sh. Eine needs-Liste,
#     die per Anker hereinkommt, sieht er nicht.
#   * ob die Reihenfolge fachlich SINNVOLL ist. Sie ist hier nur zulaessig.
#
# STUFEN-AUFLOESUNG, in dieser Reihenfolge:
#   (1) eigener 'stage:'-Schluessel des Jobs
#   (2) 'extends' auf ein Template, das IN DIESER Datei steht (rekursiv)
#   (3) scripts/ci_stage_topologie_allowlist.txt -- fuer Templates aus dem
#       fremden Projekt comdare/cluster/ci-templates, dessen Text im CI-Lauf
#       nicht als Datei vorliegt. Jede Zeile dort traegt ihre Fundstelle.
#
# AUFRUF:  sh scripts/ci_stage_topologie_wache.sh [<yaml>]
#          (ohne Argument: .gitlab-ci.yml im Repo-Wurzelverzeichnis)
# EXIT:    0 = keine Vorwaerts-Kante
#          1 = mindestens eine Vorwaerts-Kante (mit Namen und Stufen benannt)
#          2 = konnte nicht pruefen (YAML fehlt, Stufe unaufloesbar, 0 Kanten,
#              Allowlist-Zeile ohne Begruendung) -- ausdruecklich KEIN Gruen
#
# POSIX sh + awk. ASCII-only (Leitplanke). Kein Python in der Buildchain.
# =============================================================================

set -u

_st_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 2
_st_repo=$(CDPATH= cd -- "${_st_dir}/.." && pwd) || exit 2
ST_YAML=${1:-${_st_repo}/.gitlab-ci.yml}
ST_ALLOW=${COMDARE_STAGE_ALLOWLIST:-${_st_dir}/ci_stage_topologie_allowlist.txt}

st_abbruch() {
    echo ""
    echo "STUFEN-TOPOLOGIE-WACHE: ABBRUCH -- $1" >&2
    exit 2
}

command -v awk >/dev/null 2>&1 || st_abbruch "awk ist nicht im PATH."
[ -f "$ST_YAML" ]  || st_abbruch "'${ST_YAML}' nicht gefunden."
[ -f "$ST_ALLOW" ] || st_abbruch "Allowlist '${ST_ALLOW}' nicht gefunden (fail-closed)."

echo "============================================================================="
echo " STUFEN-TOPOLOGIE-WACHE ueber .gitlab-ci.yml"
echo "  YAML      : ${ST_YAML}"
echo "  Allowlist : ${ST_ALLOW}"
echo "============================================================================="

awk -v allowdatei="$ST_ALLOW" '
# ---------------------------------------------------------------------------
# Allowlist zuerst: <name> <stufe> <begruendung...>. Eine Zeile ohne
# Begruendung ist ein Fehler -- sonst waere die Datei der stille Rueckfall.
# ---------------------------------------------------------------------------
BEGIN {
    fehler = 0
    while ((getline zeile < allowdatei) > 0) {
        if (zeile ~ /^[ \t]*#/ || zeile ~ /^[ \t]*$/) continue
        n = split(zeile, f, /[ \t]+/)
        if (n < 3) {
            printf("ALLOWLIST-FEHLER: Zeile ohne Begruendung: >>%s<<\n", zeile)
            fehler = 1
            continue
        }
        allow[f[1]] = f[2]
        n_allow++
    }
    close(allowdatei)
    if (fehler) { print "ABBRUCH_ALLOWLIST"; exit }
}

# ---------------------------------------------------------------------------
# stages-Block: die Reihenfolge selbst. Eingerueckte Kommentarzeilen gehoeren
# dazu (dieser Commit schreibt welche hinein); eine Zeile in Spalte 1 beendet
# den Block.
# ---------------------------------------------------------------------------
/^stages:[ \t]*$/ { in_stages = 1; next }
in_stages {
    if ($0 ~ /^[^ \t]/) { in_stages = 0 }
    else {
        if ($0 ~ /^[ \t]+#/ || $0 ~ /^[ \t]*$/) next
        if (match($0, /^[ \t]+-[ \t]*[A-Za-z0-9_-]+/)) {
            s = $0
            sub(/^[ \t]+-[ \t]*/, "", s)
            sub(/[ \t].*$/, "", s)
            sub(/#.*$/, "", s)
            n_stages++
            stufe_idx[s] = n_stages
            stufe_name[n_stages] = s
            next
        }
        in_stages = 0
    }
}

# ---------------------------------------------------------------------------
# Top-Level-Schluessel: Job oder Template. Beendet jeden offenen needs-Block.
# ---------------------------------------------------------------------------
/^[A-Za-z_.][A-Za-z0-9_:.-]*:[ \t]*(#.*)?$/ {
    schluessel = $0
    sub(/:[ \t]*(#.*)?$/, "", schluessel)
    aktuell = schluessel
    if (!(aktuell in gesehen)) { gesehen[aktuell] = 1; reihenfolge[++n_keys] = aktuell }
    in_needs = 0
    next
}

aktuell == "" { next }

# -- stage: ---------------------------------------------------------------
/^[ \t]+stage:[ \t]*[A-Za-z0-9_-]+/ {
    s = $0
    sub(/^[ \t]+stage:[ \t]*/, "", s)
    sub(/[ \t]*(#.*)?$/, "", s)
    stage_von[aktuell] = s
    in_needs = 0
    next
}

# -- extends: (".a" oder "[.a, .b]") --------------------------------------
/^[ \t]+extends:[ \t]*[^ \t]/ {
    s = $0
    sub(/^[ \t]+extends:[ \t]*/, "", s)
    sub(/#.*$/, "", s)
    gsub(/[\[\]",]/, " ", s)
    extends_von[aktuell] = s
    in_needs = 0
    next
}

# -- needs: [ ... ] (einzeilig) -------------------------------------------
/^[ \t]+needs:[ \t]*\[/ {
    s = $0
    sub(/^[ \t]+needs:[ \t]*\[/, "", s)
    sub(/\].*$/, "", s)
    gsub(/["'"'"']/, "", s)
    hat_needs[aktuell] = 1
    n = split(s, ziele, /,/)
    for (i = 1; i <= n; i++) {
        z = ziele[i]
        gsub(/^[ \t]+|[ \t]+$/, "", z)
        if (z == "") continue
        kante_von[++n_kanten] = aktuell
        kante_zu[n_kanten] = z
    }
    in_needs = 0
    next
}

# -- needs: (Blockform, Eintraege folgen eingerueckt) ----------------------
/^[ \t]+needs:[ \t]*(#.*)?$/ { hat_needs[aktuell] = 1; in_needs = 1; next }

in_needs {
    # Ein neuer Schluessel auf needs-Ebene beendet den Block. Die Unterschluessel
    # eines needs-Eintrags (job:, artifacts:, ...) beenden ihn NICHT.
    ist_key   = ($0 ~ /^[ \t]+[A-Za-z_][A-Za-z0-9_-]*:[ \t]*/ && $0 !~ /^[ \t]+-/)
    ist_unter = ($0 ~ /^[ \t]+(job|artifacts|optional|pipeline|project|ref|parallel):/)
    if (ist_key && !ist_unter) {
        in_needs = 0
    } else {
        if ($0 ~ /^[ \t]+#/ || $0 ~ /^[ \t]*$/) next
        z = ""
        if (match($0, /^[ \t]+-[ \t]*job:[ \t]*/)) {
            z = $0; sub(/^[ \t]+-[ \t]*job:[ \t]*/, "", z)
        } else if (match($0, /^[ \t]+-[ \t]*["'"'"']?[A-Za-z_.]/)) {
            z = $0; sub(/^[ \t]+-[ \t]*/, "", z)
        } else if (match($0, /^[ \t]+(pipeline|project):[ \t]*/)) {
            # Kante in eine FREMDE Pipeline: gezaehlt, aber nicht bewertbar.
            n_fremd++
            next
        } else {
            next
        }
        sub(/#.*$/, "", z)
        gsub(/["'"'"']/, "", z)
        gsub(/^[ \t]+|[ \t]+$/, "", z)
        if (z == "") next
        kante_von[++n_kanten] = aktuell
        kante_zu[n_kanten] = z
        next
    }
}

# ---------------------------------------------------------------------------
END {
    if (fehler) exit
    # -- Stufe eines Schluessels aufloesen (rekursiv ueber extends) ----------
    print ""
    printf("STUFEN (Quelle: stages-Block): %d\n", n_stages)
    if (n_stages < 2) {
        printf("ABBRUCH: %d Stufe(n) gelesen -- ohne Reihenfolge gibt es kein Vorwaerts.\n", n_stages)
        print "ABBRUCH_STAGES"
        exit
    }
    zeile = "  "
    for (i = 1; i <= n_stages; i++) zeile = zeile sprintf("%d=%s  ", i, stufe_name[i])
    print zeile

    printf("\nKANTEN: %d needs-Kante(n) aus %d Top-Level-Schluessel(n)", n_kanten, n_keys)
    if (n_fremd > 0) printf(", zusaetzlich %d Kante(n) in fremde Pipelines (nicht bewertbar)", n_fremd)
    print ""
    if (n_kanten < 1) {
        print "ABBRUCH: 0 Kanten. Eine Null ohne Nenner ist kein Freispruch."
        print "ABBRUCH_KANTEN"
        exit
    }

    n_vorwaerts = 0
    for (k = 1; k <= n_kanten; k++) {
        a = kante_von[k]; b = kante_zu[k]
        sa = loese(a); sb = loese(b)
        if (sa == "") { unaufl_job(a, "fordernde Seite der Kante -> " b); unaufl++; continue }
        if (sb == "") { unaufl_job(b, "Ziel der Kante " a " -> " b);      unaufl++; continue }
        ia = stufe_idx[sa]; ib = stufe_idx[sb]
        if (ia == 0) { unaufl_stufe(sa, a); unaufl++; continue }
        if (ib == 0) { unaufl_stufe(sb, b); unaufl++; continue }
        ziel_zaehler[b]++
        if (ib > ia) {
            n_vorwaerts++
            printf("  [VORWAERTS] %s (Stufe %d=%s) needs %s (Stufe %d=%s) -- GitLab weist die Pipeline ab.\n",
                   a, ia, sa, b, ib, sb)
        }
    }

    if (unaufl > 0) {
        printf("\nABBRUCH: %d Kante(n) mit unaufloesbarer Stufe. Ein uebersprungener Job\n", unaufl)
        print   "         koennte die Vorwaerts-Kante gerade verbergen -- KEIN stilles Gruen."
        print "ABBRUCH_UNAUFLOESBAR"
        exit
    }

    print ""
    print "ZIELE der Kanten (Nenner je Ziel):"
    for (z in ziel_zaehler) printf("  %-28s %d Kante(n) -> Stufe %s\n", z, ziel_zaehler[z], loese(z))

    print ""
    print "-----------------------------------------------------------------------------"
    printf("NENNER: %d von %d Kanten zeigen VORWAERTS.\n", n_vorwaerts, n_kanten)
    print "-----------------------------------------------------------------------------"
    if (n_vorwaerts > 0) { print "ERGEBNIS_ROT"; exit }
    print "ERGEBNIS_GRUEN"
}

# Meldungen der beiden Unaufloesbar-Faelle. Eigene Funktionen, damit die
# Zeilen unter der 120-Byte-Grenze der Diff-Hygiene-Wache bleiben.
function unaufl_job(j, wo) {
    printf("UNAUFLOESBAR: Job \047%s\047 (%s)\n", j, wo)
}
function unaufl_stufe(s, j) {
    printf("UNAUFLOESBAR: Stufe \047%s\047 von Job \047%s\047 fehlt im stages-Block\n", s, j)
}

# Stufe eines Schluessels: eigener stage -> extends (rekursiv, in dieser Datei)
# -> Allowlist. Leerer Rueckgabewert heisst UNAUFLOESBAR, nie "egal".
function loese(k,   e, n, i, teile, r) {
    if (k in stage_von) return stage_von[k]
    if (k in allow)     return allow[k]
    if (k in extends_von) {
        if (k in in_arbeit) return ""      # Zyklus: lieber unaufloesbar als falsch
        in_arbeit[k] = 1
        n = split(extends_von[k], teile, /[ \t]+/)
        for (i = 1; i <= n; i++) {
            if (teile[i] == "") continue
            r = loese(teile[i])
            if (r != "") { delete in_arbeit[k]; return r }
        }
        delete in_arbeit[k]
    }
    return ""
}
' "$ST_YAML" > "${TMPDIR:-/tmp}/ce_stagetopo.$$" 2>&1
_st_awk_rc=$?

cat "${TMPDIR:-/tmp}/ce_stagetopo.$$"
_st_aus=$(cat "${TMPDIR:-/tmp}/ce_stagetopo.$$")
rm -f "${TMPDIR:-/tmp}/ce_stagetopo.$$"

[ "$_st_awk_rc" -eq 0 ] || st_abbruch "awk selbst ist fehlgeschlagen (rc=${_st_awk_rc}) -- KEINE stille Null."

case "$_st_aus" in
    *ABBRUCH_ALLOWLIST*)   st_abbruch "die Allowlist traegt eine Zeile ohne Begruendung." ;;
    *ABBRUCH_STAGES*)      st_abbruch "der stages-Block ist nicht lesbar." ;;
    *ABBRUCH_KANTEN*)      st_abbruch "0 needs-Kanten gefunden -- ohne Nenner kein Freispruch." ;;
    *ABBRUCH_UNAUFLOESBAR*) st_abbruch "mindestens eine Stufe ist nicht aufloesbar (s. oben)." ;;
    *ERGEBNIS_ROT*)
        echo ""
        echo "STUFEN-TOPOLOGIE-WACHE: ROT -- mindestens eine needs-Kante zeigt vorwaerts." >&2
        exit 1 ;;
    *ERGEBNIS_GRUEN*)
        echo ""
        echo "STUFEN-TOPOLOGIE-WACHE: GRUEN."
        exit 0 ;;
esac

st_abbruch "die Auswertung hat kein Ergebnis geliefert -- interner Fehler, keine stille Null."
