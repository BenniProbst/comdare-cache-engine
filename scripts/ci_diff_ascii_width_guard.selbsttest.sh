# shellcheck shell=sh
# =============================================================================
# comdare-cache-engine -- SELBSTTEST der DIFF-HYGIENE-WACHE (2026-08-09)
#   Prueft: scripts/ci_diff_ascii_width_guard.sh
# =============================================================================
# SELBSTCHECK: dieses Skript SICHERT ZU, dass die Diff-Hygiene-Wache in ihrem
# DEFAULT-MODUS (Aufruf ohne Argumente) einen Verstoss auch dann findet, wenn er
# (a) bereits per `git add` GESTAGT oder (b) in einer noch UNTRACKED neuen Datei
# liegt, und dass sie einen leeren Nenner nicht als GRUEN ausgibt. Es SICHERT
# NICHT ZU, dass die Wache jede denkbare Unicode-Form erkennt (sie arbeitet
# byteweise 128..255), und es prueft NICHT den Inhalt des ce-Repos selbst --
# jeder Fall laeuft in einem eigenen Wegwerf-Repo unter TMPDIR.
#
# BEFUND, DER DIESEN SELBSTTEST ERZWINGT (2026-08-09, am Objekt erhoben):
# Der Biss-Beweis der Wache (ci_diff_ascii_width_guard.bissbeweis.txt) belegt
# sieben Faelle -- ALLE ueber `--stdin` oder einen ausdruecklichen Bereich.
# KEINER fuhr die Wache ohne Argumente. Genau dort lag ein unbenannter blinder
# Fleck: `git diff` ohne Revision vergleicht den Arbeitsbaum gegen den INDEX,
# nicht gegen HEAD. Wer seine Arbeit vor der Pruefung `git add`-et, bekommt
# "0 Zusatzzeilen geprueft ... GRUEN" -- und untracked NEUE Dateien sieht
# `git diff` ueberhaupt nie.
#
# Am Objekt aufgeschlagen ist das im Paket D5-1 (Commit c98b4b95): der Bauer
# meldete "0 nicht-ASCII in 189 hinzugefuegten Zeilen"; der Commit trug
# tatsaechlich 570 hinzugefuegte C++-Zeilen, davon 7 nicht-ASCII (6 Box-
# Trennlinien U+2500 plus ein Umlaut-Wort), zusaetzlich 6 Zeilen ueber 120
# Spalten. Die neue Testdatei war gestagt UND neu -- beide blinden Flecken auf
# einmal. Die Differenz 570-189 IST der blinde Fleck, in Zeilen gemessen.
#
# WARUM ALS EIGENES SKRIPT UND NICHT ALS NOTIZ IM BISS-BEWEIS: der Biss-Beweis
# ist eine erhobene Ausgabe von 2026-08-06 -- ein Protokoll, das nichts nachweist,
# wenn sich das Werkzeug spaeter aendert. Dieses Skript ist ausfuehrbar und
# wuerfelt seinen Koeder bei JEDEM Lauf neu, damit kein Fall ueber einen fest
# eingetragenen Beispielwert gruen werden kann.
#
# =============================================================================
# JE SCHICHT EIN FALL -- die Bauregel dieses Selbsttests (2026-08-10)
# =============================================================================
# BEFUND, DER SIE ERZWINGT (Pruefer, am Objekt mutationsgemessen): ein Fall darf
# nicht die zusammengesetzte WIRKUNG mehrerer Schichten pruefen, sondern muss die
# einzelne BEDINGUNG pinnen. Sonst traegt beim Wegfall einer Schicht die jeweils
# andere den Fall, und er bleibt gruen -- genau die Klasse von Test, gegen die
# die Wache selbst gebaut ist.
#
# GEMESSEN am Stand 5bd90986 (jede Mutation einzeln, Selbsttest mit 12 Faellen):
#     Mutation                                        Selbsttest    Befund
#     is_vendor()-Zweig aus skip_grund() gestrichen    12 von 12     UEBERLEBT
#     *.md-Zweig aus skip_grund() gestrichen           10 von 12     gefangen
#     awk-Entklammerung (4 Zeilen) gestrichen          12 von 12     UEBERLEBT
#     core.quotePath=false am Haupt-Aufruf gestrichen  12 von 12     UEBERLEBT
# Drei von vier Schichten waren ungedeckt. Reproduzierbar: Wache und Selbsttest
# in ein Wegwerf-Verzeichnis kopieren, EINE Schicht aus der Kopie streichen, den
# Selbsttest gegen die Kopie fahren -- der Arbeitsbaum bleibt dabei unberuehrt.
#
# WELCHER FALL WELCHE SCHICHT PINNT (bei Umbauten mitzufuehren):
#     Schicht                                     Fall   faellt sie, wird rot
#     skip_grund(): *.md, Sprachdoktrin           11     11 (und 12)
#     skip_grund(): Vendor-Baum via Provenance    13     13, nicht 11/12/15
#     Quotierung: core.quotePath=false            12     12, nicht 15
#     Quotierung: awk streift Klammerung ab       15     15, nicht 12
#     breite_frei_grund(): *.xml NUR Breite       16     16; ASCII-Haelfte: 17
#     skip_grund(): Lizenztext LICENSES/*.txt      19     19, nicht 20/21
# Zu Fall 13 gehoert Fall 14 als GEGENEINGANG: derselbe Verstoss OHNE die
# Provenance-Datei muss gemeldet werden. Ohne ihn waere Fall 13 auch von einer
# Wache zu bestehen, die pauschal alles unter ext/ ueberspringt.
# Zu Fall 16 gehoeren ZWEI Gegeneingaenge: Fall 17 (Nicht-ASCII in derselben
# Datei-Klasse MUSS beissen -- sonst waere 16 auch von einer dritten
# VOLL-Ausnahme in skip_grund() zu bestehen) und der bestehende Fall 6
# (dieselbe Ueberlaenge in .cpp MUSS beissen -- pinnt die Endungs-Bindung).
# Zu Fall 19 gehoeren ZWEI Gegeneingaenge (27.08.2026): Fall 20 (derselbe Text
# AUSSERHALB von LICENSES/ MUSS beissen -- pinnt den Pfad) und Fall 21 (eine .sh
# IN LICENSES/ MUSS beissen -- pinnt die Endung).
#
# ZAHLEN IN DIESEM KOPF TRAGEN IHREN ANKER (Stand + Kommando) oder sie stehen
# nicht hier. Eine nackte Zahl ueber einen lebenden Gegenstand verjaehrt zwischen
# Messung und Landung; die Zahl der Faelle druckt dieses Skript ohnehin selbst.
#
# AUFRUF:  sh scripts/ci_diff_ascii_width_guard.selbsttest.sh
# EXIT:    0 = alle Faelle wie erwartet
#          1 = mindestens ein Fall weicht ab (Details je Fall im Log)
#          2 = ABBRUCH (Werkzeug fehlt / Wache nicht auffindbar) -- KEIN Gruen
#
# KEIN Python (Buildchain-Kanon), POSIX sh, ASCII-only.
# =============================================================================

set -u
export LC_ALL=C

st_abbruch() {
    echo ""
    echo "WACHE-SELBSTTEST: ABBRUCH -- $1" >&2
    exit 2
}

command -v git >/dev/null 2>&1 || st_abbruch "git ist nicht im PATH."
command -v mktemp >/dev/null 2>&1 || st_abbruch "mktemp ist nicht im PATH."
command -v od >/dev/null 2>&1 || st_abbruch "od ist nicht im PATH."

_st_script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || st_abbruch "Skript-Verzeichnis nicht aufloesbar."
_st_wache="${_st_script_dir}/ci_diff_ascii_width_guard.sh"
[ -f "$_st_wache" ] || st_abbruch "Wache nicht gefunden: ${_st_wache}"

# ---------------------------------------------------------------------------
# KOEDER: bei JEDEM Lauf frisch gewuerfelt (K13 -- ein aus einer Doku
# abgeschriebener Beispielwert kann allowgelistet sein und ergibt ein falsches
# Gruen ueber die Wache selbst). Zwei unabhaengige Wuerfe:
#   * ein Zufalls-Kennzeichen (Hex aus /dev/urandom), das in jeder Koederzeile
#     steht -- es macht die Zeile unverwechselbar,
#   * das nicht-ASCII-ZEICHEN selbst, aus einer festen Menge gewuerfelt. Das
#     Paragraf-Zeichen (0xC2 0xA7) ist AUSDRUECKLICH NICHT in dieser Menge:
#     es ist die dokumentierte Ausnahme der Wache und wird in Fall 5 getrennt
#     als Gegenprobe geprueft.
# Beide werden gedruckt, damit ein roter Lauf nachvollziehbar bleibt.
# ---------------------------------------------------------------------------
_st_kennzeichen=$(od -An -N6 -tx1 /dev/urandom | tr -d ' \n') \
    || st_abbruch "Wuerfeln des Kennzeichens fehlgeschlagen."
_st_wurf=$(od -An -N1 -tu1 /dev/urandom | tr -d ' \n') \
    || st_abbruch "Wuerfeln des Zeichen-Index fehlgeschlagen."
_st_idx=$(( _st_wurf % 5 ))
case "$_st_idx" in
    0) _st_na=$(printf '\303\244'); _st_na_name="a-Umlaut U+00E4 (c3 a4)" ;;
    1) _st_na=$(printf '\303\266'); _st_na_name="o-Umlaut U+00F6 (c3 b6)" ;;
    2) _st_na=$(printf '\342\224\200'); _st_na_name="Box-Drawing U+2500 (e2 94 80)" ;;
    3) _st_na=$(printf '\342\200\224'); _st_na_name="Em-Dash U+2014 (e2 80 94)" ;;
    4) _st_na=$(printf '\342\202\254'); _st_na_name="Euro U+20AC (e2 82 ac)" ;;
    *) st_abbruch "Wuerfel-Index ausserhalb der Menge: ${_st_idx}" ;;
esac
_st_sigma=$(printf '\302\247')   # Paragraf-Zeichen: die ERLAUBTE Ausnahme

echo "============================================================================="
echo " SELBSTTEST DER DIFF-HYGIENE-WACHE"
echo "============================================================================="
echo "GEPRUEFTE WACHE: ${_st_wache}"
echo "KOEDER (frisch gewuerfelt, jeder Lauf neu):"
echo "  Kennzeichen : ${_st_kennzeichen}"
echo "  Zeichen     : ${_st_na_name}   [Wurf ${_st_wurf} -> Index ${_st_idx} von 0..4]"
echo ""

_st_gruen=0
_st_rot=0
_st_gesamt=0

# ---------------------------------------------------------------------------
# Ein Wegwerf-Repo je Fall. Die Wache loest ihre Repo-Wurzel aus dem eigenen
# Skript-Pfad auf ($0/..), deshalb wird sie nach <repo>/scripts/ kopiert.
# ---------------------------------------------------------------------------
st_neues_repo() {
    _st_repo="$(mktemp -d "${TMPDIR:-/tmp}/ce_wache_selbsttest.XXXXXX")" \
        || st_abbruch "mktemp -d fehlgeschlagen."
    mkdir -p "${_st_repo}/scripts" || st_abbruch "mkdir im Wegwerf-Repo fehlgeschlagen."
    cp "$_st_wache" "${_st_repo}/scripts/" || st_abbruch "Kopieren der Wache fehlgeschlagen."
    # init.templateDir leer + gpgsign aus: eine GLOBALE Konfiguration des Runners
    # (Hook-Vorlagen, erzwungene Signatur) darf den Selbsttest nicht kippen -- er
    # prueft die Wache, nicht die Signaturkette des Hosts.
    git -C "$_st_repo" -c init.templateDir= init -q 2>/dev/null \
        || st_abbruch "git init fehlgeschlagen."
    git -C "$_st_repo" config user.email "selbsttest@comdare.local" || st_abbruch "git config fehlgeschlagen."
    git -C "$_st_repo" config user.name "Wache-Selbsttest" || st_abbruch "git config fehlgeschlagen."
    git -C "$_st_repo" config commit.gpgsign false || st_abbruch "git config fehlgeschlagen."
    printf '// Basis-Datei, reines ASCII, unter 120 Spalten.\nint basis() { return 0; }\n' \
        > "${_st_repo}/basis.cpp"
    git -C "$_st_repo" add -A || st_abbruch "git add im Wegwerf-Repo fehlgeschlagen."
    git -C "$_st_repo" commit -q --no-verify -m "basis" \
        || st_abbruch "git commit im Wegwerf-Repo fehlgeschlagen."
}

# st_bewerte <fallname> <erwarteter_exit> <zusicherungen> <tatsaechlicher_exit> <ausgabedatei>
#
# <zusicherungen> traegt KEINE, EINE oder MEHRERE Zeilen -- je Zeile eine
# Fixed-String-Zusicherung an die Ausgabe des Falls, und ALLE muessen gelten.
# Baue die Liste mit `printf '%s\n%s\n' "..." "..."`, dann bleibt die Einrueckung
# der Quelldatei aus dem Muster heraus.
#
# WARUM MEHRERE (2026-08-10, Pruefer-Befund am Objekt): ein Fall, der nur den
# EXIT prueft, ist auch von einer Wache zu bestehen, die gar nichts angesehen hat
# -- und ein Fall mit genau EINER Zusicherung prueft die zusammengesetzte
# WIRKUNG, nicht die BEDINGUNG. Fall 12 war so gebaut und blieb gruen, wenn eine
# seiner beiden Schichten wegfiel (mutationsgemessen, s. Kopf). Erst eine zweite,
# auf die Bedingung gerichtete Zusicherung (der GRUND der Ausnahme, der lesbare
# Dateiname) pinnt die einzelne Schicht.
#
# V-1: die ZAHL der belegten Zusicherungen wird mitgedruckt. Ohne sie sieht ein
# Fall ohne jede Zusicherung im Log genauso aus wie ein scharf pruefender -- der
# Nenner gehoert in die Ausgabe des Werkzeugs, nicht in den Bericht darueber.
st_bewerte() {
    _st_fall="$1"; _st_soll="$2"; _st_muster="$3"; _st_ist="$4"; _st_datei="$5"
    _st_gesamt=$(( _st_gesamt + 1 ))
    _st_ok=1
    _st_grund=""
    _st_zusi=0
    if [ "$_st_ist" -ne "$_st_soll" ]; then
        _st_ok=0
        _st_grund="Exit ${_st_ist}, erwartet ${_st_soll}"
    elif [ -n "$_st_muster" ]; then
        printf '%s\n' "$_st_muster" > "$_st_muster_datei" \
            || st_abbruch "Schreiben der Zusicherungsliste fehlgeschlagen."
        # KEINE Pipe: `while ... done < datei` laeuft in POSIX sh in DERSELBEN
        # Shell, die Zaehler ueberleben die Schleife also. Ueber eine Pipe
        # gelesen laegen sie in einer Subshell und waeren danach verloren --
        # der Fall waere still gruen. Dieselbe Klasse wie die Falle im Kopf.
        while IFS= read -r _st_m; do
            [ -n "$_st_m" ] || continue
            if LC_ALL=C grep -q -F -- "$_st_m" "$_st_datei"; then
                _st_zusi=$(( _st_zusi + 1 ))
                continue
            fi
            _st_ok=0
            _st_grund="Exit stimmt (${_st_ist}), aber die Ausgabe enthaelt nicht: '${_st_m}'"
            break
        done < "$_st_muster_datei"
    fi
    if [ "$_st_ok" -eq 1 ]; then
        _st_gruen=$(( _st_gruen + 1 ))
        printf '  [ok ] %-34s Exit %d wie erwartet, %d Zusicherung(en)\n' \
            "$_st_fall" "$_st_ist" "$_st_zusi"
    else
        _st_rot=$(( _st_rot + 1 ))
        printf '  [ROT] %-34s %s\n' "$_st_fall" "$_st_grund"
        echo "        ----- Ausgabe des Falls -----"
        sed 's/^/        /' "$_st_datei"
        echo "        -----------------------------"
    fi
}

_st_out="$(mktemp "${TMPDIR:-/tmp}/ce_wache_selbsttest_out.XXXXXX")" \
    || st_abbruch "mktemp fuer die Ausgabedatei fehlgeschlagen."
_st_muster_datei="$(mktemp "${TMPDIR:-/tmp}/ce_wache_selbsttest_mus.XXXXXX")" \
    || st_abbruch "mktemp fuer die Zusicherungsliste fehlgeschlagen."
_st_diff="$(mktemp "${TMPDIR:-/tmp}/ce_wache_selbsttest_diff.XXXXXX")" \
    || st_abbruch "mktemp fuer den vorgefertigten Diff fehlgeschlagen."

echo "FAELLE:"

# --- Fall 1: UNGESTAGTE Aenderung in einer getrackten Datei -> muss beissen ---
# Kontrollfall: dieser Weg funktionierte schon vorher. Er steht hier, damit ein
# spaeterer Umbau des Default-Modus ihn nicht unbemerkt verliert.
st_neues_repo
printf '// KOEDER %s %s Trennlinie\n' "$_st_kennzeichen" "$_st_na" >> "${_st_repo}/basis.cpp"
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "1 ungestagt beisst" 1 "NICHT-ASCII" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 2: GESTAGTE Aenderung -> muss ebenfalls beissen ---
# DER KERNFALL. `git diff` ohne Revision vergleicht gegen den INDEX; wer seine
# Arbeit vor der Pruefung `git add`-et, ist fuer die alte Fassung unsichtbar.
st_neues_repo
printf '// KOEDER %s %s Trennlinie\n' "$_st_kennzeichen" "$_st_na" >> "${_st_repo}/basis.cpp"
git -C "$_st_repo" add basis.cpp || st_abbruch "git add (Fall 2) fehlgeschlagen."
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "2 gestagt beisst" 1 "NICHT-ASCII" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 3: UNTRACKED NEUE Datei -> muss beissen ---
# Der zweite blinde Fleck: `git diff` sieht untracked Dateien in KEINEM Modus,
# auch nicht gegen HEAD. Genau diese Lage hatte die neue D5-1-Testdatei.
st_neues_repo
printf '// NEUE DATEI KOEDER %s %s Trennlinie\nint neu() { return 1; }\n' \
    "$_st_kennzeichen" "$_st_na" > "${_st_repo}/neu_modul.cpp"
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "3 untracked beisst" 1 "NICHT-ASCII" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 4: GEGENPROBE -- gestagt UND untracked, aber sauber -> GRUEN ---
# Ohne diesen Fall wuerde eine Wache, die einfach IMMER rot meldet, die Faelle
# 1..3 bestehen. Er ist die Gegenprobe zum Koeder.
st_neues_repo
printf '// sauberer Kommentar, reines ASCII, unter 120 Spalten (%s)\n' "$_st_kennzeichen" \
    >> "${_st_repo}/basis.cpp"
git -C "$_st_repo" add basis.cpp || st_abbruch "git add (Fall 4) fehlgeschlagen."
printf '// neue Datei, ebenfalls sauber (%s)\nint sauber() { return 2; }\n' "$_st_kennzeichen" \
    > "${_st_repo}/neu_sauber.cpp"
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "4 sauber bleibt gruen" 0 "GRUEN" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 5: GEGENPROBE -- das Paragraf-Zeichen bleibt die erlaubte Ausnahme ---
# Zweite Gegenprobe, diesmal gegen UEBER-Beissen: die Wache darf nicht einfach
# jedes Byte >127 verwerfen, sondern muss ihre dokumentierte Ausnahme halten --
# und zwar auch auf dem neuen, gestagten Weg.
st_neues_repo
printf '// Paragraf-Ausnahme %s %s bleibt erlaubt\n' "$_st_sigma" "$_st_kennzeichen" \
    >> "${_st_repo}/basis.cpp"
git -C "$_st_repo" add basis.cpp || st_abbruch "git add (Fall 5) fehlgeschlagen."
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "5 Paragraf-Ausnahme haelt" 0 "GRUEN" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 6: GESTAGTE ueberlange Zeile -> muss beissen ---
# Die Spaltenbreite lief durch denselben blinden Fleck. Der D5-1-Commit trug
# neben den 7 nicht-ASCII-Zeilen auch 6 Zeilen mit 282 Byte.
st_neues_repo
_st_lang="// $(od -An -N90 -tx1 /dev/urandom | tr -d ' \n')"
printf '%s\n' "$_st_lang" >> "${_st_repo}/basis.cpp"
git -C "$_st_repo" add basis.cpp || st_abbruch "git add (Fall 6) fehlgeschlagen."
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "6 gestagt >120 Spalten beisst" 1 ">120-SPALTEN" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 7: LEERER NENNER ist kein GRUEN ---
# Hausdoktrin (scripts/vor_push_alle_wachen.sh, Kopf): "GRUEN MIT NENNER 0 IST
# ROT". Ein sauberer Baum im Default-Modus heisst "es wurde nichts gemessen" --
# das darf kein bestandenes Gate sein, sondern ist ein ABBRUCH (Exit 2).
st_neues_repo
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "7 leerer Nenner ist Abbruch" 2 "" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 8: BEREICHSMODUS unveraendert (Regressionsschutz fuer die CI) ---
# Die CI ruft die Wache MIT Bereich (.gitlab-ci.yml: "${_ce_base}..HEAD"). Dieser
# Weg darf sich durch die Reparatur des Default-Modus nicht aendern.
st_neues_repo
printf '// KOEDER %s %s im Commit\n' "$_st_kennzeichen" "$_st_na" >> "${_st_repo}/basis.cpp"
git -C "$_st_repo" add basis.cpp || st_abbruch "git add (Fall 8) fehlgeschlagen."
git -C "$_st_repo" commit -q --no-verify -m "koeder" || st_abbruch "git commit (Fall 8) fehlgeschlagen."
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh "HEAD~1..HEAD" ) > "$_st_out" 2>&1
st_bewerte "8 Bereichsmodus beisst weiter" 1 "NICHT-ASCII" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 9: BEREICHSMODUS mit leerem Diff bleibt wie bisher ---
# Bewusste ASYMMETRIE: der Nenner-0-Abbruch gilt NUR im Default-Modus. Ein
# ausdruecklich angegebener Bereich, der nichts enthaelt, ist eine Aussage des
# Aufrufers und darf die CI nicht abwuergen (leerer Merge-Commit).
st_neues_repo
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh "HEAD..HEAD" ) > "$_st_out" 2>&1
st_bewerte "9 leerer Bereich bleibt Exit 0" 0 "GRUEN" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 10: SHELL-DECKUNG -- ein Koeder in einer .sh-Datei muss beissen ---
# BEFUND, DER DIESEN FALL ERZWINGT (2026-08-10, am Objekt gemessen): is_scoped()
# war eine WHITELIST (.cpp/.hpp/.h/.hh/.cc/.cxx/.tpp/.ipp/.inl/.cmake plus
# CMakeLists.txt). ALLES andere fiel lautlos heraus -- .sh, .c, .txt, .yml, .py.
# Die Wache meldete GRUEN, ohne die Datei je angesehen zu haben. Ihr Nenner nannte
# die Zeilen zwar als "ausserhalb des Scopes", aber unter dem Etikett
# "(Doku-Prosa/Sonstiges)" -- das liest sich wie Prosa und war in Wahrheit
# Quelltext: 143 getrackte .sh-Dateien mit 10.084 Zeilen.
#
# DER SCHADEN WAR REAL, nicht bloss potenziell: ein Strang kuerzte seine
# Bilanz-Zeile von 159 auf 121 und dann auf 122 Byte, ohne nachzumessen, und kam
# durch alle Tore -- das GRUEN ueber seinen Bereich war kein Urteil ueber die
# Datei, die er geaendert hatte.
#
# Der Fall wuerfelt BEIDE Verstossklassen in EINE Shell-Datei: das nicht-ASCII-
# Zeichen (oben frisch gewuerfelt) und eine ueberlange Zeile. Vor der Reparatur
# lief er Exit 0 GRUEN durch -- protokolliert im Paket zu diesem Fall.
st_neues_repo
printf '# KOEDER %s %s Bilanz-Zeile\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/scripts/probe_bilanz.sh"
printf '# %s\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    >> "${_st_repo}/scripts/probe_bilanz.sh"
git -C "$_st_repo" add scripts/probe_bilanz.sh || st_abbruch "git add (Fall 10) fehlgeschlagen."
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "10 .sh-Deckung beisst" 1 "NICHT-ASCII" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 11: GEGENEINGANG -- die *.md-Ausnahme haelt, und .sh bleibt geprueft ---
# Ohne diesen Fall waere Fall 10 auch von einer Wache zu bestehen, die schlicht
# JEDE Datei rot faerbt -- dann waere deutsche Doku-Prosa unbenutzbar. Die
# Ausnahme fuer *.md ist die urspruengliche, richtige Begruendung des Scopes:
# deutsche Prosa traegt per Sprachdoktrin korrekte Umlaute.
# Der Fall stellt beides gleichzeitig: eine .md-Datei mit demselben gewuerfelten
# Zeichen (muss uebersprungen werden) UND eine saubere .sh-Datei (muss GEPRUEFT
# werden, damit der Nenner nicht null ist -- sonst waere das Gruen wertlos).
st_neues_repo
printf 'Deutsche Doku-Prosa mit Umlaut %s und Kennzeichen %s.\n' "$_st_na" "$_st_kennzeichen" \
    > "${_st_repo}/HANDBUCH.md"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_sauber.sh"
git -C "$_st_repo" add HANDBUCH.md scripts/probe_sauber.sh \
    || st_abbruch "git add (Fall 11) fehlgeschlagen."
# Die erste Zusicherung ist bewusst die NENNER-Zeile und nicht bloss "GRUEN":
# genau EINE der beiden Zusatzzeilen darf geprueft worden sein (die .sh), die
# andere (die .md) gehoert in die uebersprungene Menge. Ein blosses "Exit 0"
# waere auch von einer Wache zu bestehen, die BEIDE Dateien uebersieht -- also
# von genau dem Defekt, den Fall 10 aufdeckt.
# Die ZWEITE Zusicherung (2026-08-10, Pruefer-Befund) nennt den GRUND: nicht
# "die Datei wurde uebersprungen", sondern "sie wurde als *.md uebersprungen".
# Das trennt die *.md-Schicht von der Vendor-Schicht -- ohne sie wuerde der Fall
# auch dann gruen bleiben, wenn die .md aus irgendeinem anderen Grund herausfiele.
_st_erw=$(printf '%s\n%s\n' \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "- HANDBUCH.md  [Doku-Prosa *.md, Sprachdoktrin]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "11 md-Ausnahme haelt, sh geprueft" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 12: die *.md-Ausnahme haelt AUCH bei Nicht-ASCII im DATEINAMEN ---
# BEFUND, DER DIESEN FALL ERZWINGT (2026-08-10, beim Umbau auf die Blacklist am
# Objekt aufgeschlagen): git meldet Pfade mit Nicht-ASCII C-quotiert, also
#     +++ "b/docs/sessions/...-\302\2478-PILOT-G1G3.md"
# MIT Anfuehrungszeichen. Der Basename endet dann auf .md" statt auf .md, und die
# Endungs-Ausnahme greift nicht mehr. Gemessen an ce-Commit 163db10d: eine reine
# deutsche Session-Doku wurde dadurch mit 37 Verstoessen ROT.
#
# WARUM ES UNTER DER ALTEN WHITELIST NICHT AUFFIEL: dort war ".md\"" genauso
# wenig gescoped wie ".md" -- Fehlbehandlung und gewollte Ausnahme sahen zufaellig
# gleich aus. Erst die Blacklist trennt die beiden, weil sie zur anderen Seite
# ausfaellt. Ein Beispiel dafuer, dass ein Richtungswechsel alte Stillstellungen
# sichtbar macht, statt neue Fehler zu erfinden.
#
# Der Dateiname traegt das oben gewuerfelte Zeichen, ist also bei jedem Lauf ein
# anderer. Wie Fall 11 liegt eine saubere .sh daneben, damit der Nenner nicht
# null ist -- ein Gruen ueber null geprueften Zeilen waere kein Gruen.
#
# WELCHE SCHICHT DIESER FALL PINNT (2026-08-10 nachgeschaerft, Pruefer-Befund):
# der Griff gegen die C-Quotierung hat ZWEI Schichten -- core.quotePath=false am
# git-Aufruf, und das Abstreifen der Klammerung auf der awk-Seite. Bis heute
# prueft Fall 12 nur ihre ZUSAMMENGESETZTE Wirkung: mutationsgemessen blieb er
# 12 von 12 gruen, wenn EINE der beiden wegfiel -- die jeweils andere trug ihn.
# Ein Test, der die Anwesenheit der Wirkung prueft statt der Bedingung, ist genau
# die Klasse, gegen die diese Wache gebaut ist.
# Dieser Fall pinnt jetzt AUSSCHLIESSLICH core.quotePath=false, ueber den
# LESBAREN Dateinamen in der uebersprungenen Menge: faellt die Option weg,
# streift die awk-Seite die Anfuehrungszeichen zwar noch ab und der Exit bleibt
# 0, aber der Name erscheint als Oktal-Wueste ("HANDBUCH-\303\244-...") statt
# lesbar -- und die zweite Zusicherung faellt. Die awk-Schicht pinnt Fall 15
# ueber den --stdin-Weg, auf dem core.quotePath des Aufrufers nicht gilt.
st_neues_repo
_st_mdname="HANDBUCH-${_st_na}-${_st_kennzeichen}.md"
printf 'Deutsche Doku-Prosa mit Umlaut %s im Text UND im Dateinamen.\n' "$_st_na" \
    > "${_st_repo}/${_st_mdname}"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_quote.sh"
git -C "$_st_repo" add -- "$_st_mdname" scripts/probe_quote.sh \
    || st_abbruch "git add (Fall 12) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n' \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "- ${_st_mdname}  [Doku-Prosa *.md, Sprachdoktrin]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "12 md-Ausnahme trotz Umlaut im Namen" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 13: die VENDOR-Ausnahme haelt -- Fremdcode bleibt ungeprueft ---
# BEFUND, DER DIESEN FALL ERZWINGT (2026-08-10, Pruefer-Befund, mutationsgemessen):
# skip_grund() hat ZWEI Schichten -- (a) *.md und (b) Vendor-Baeume mit
# COMDARE-VENDOR-PROVENANCE.md. Von den zwoelf Faellen deckten ELF die Schicht (a)
# und KEINER die Schicht (b). Gemessen, nicht vermutet: streicht man die Zeile
# `if (is_vendor(fname)) return ...` aus der Wache, blieb der Selbsttest 12 von 12
# GRUEN. Eine Ausnahme ohne Test verschwindet beim naechsten Umbau lautlos -- oder
# laesst lautlos alles durch.
#
# DER FIXTURE-BAUM IST ECHT, kein Attrappen-Pfad: ein Verzeichnis mit einer
# wirklichen COMDARE-VENDOR-PROVENANCE.md, wie das Haus sie fuer jeden Snapshot
# anlegt (im ce-Baum heute ext/io/liburing, ext/io/libxlsxwriter, ext/io/zlib).
#
# DER VERSTOSS LIEGT BEWUSST IN EINER .c-DATEI, nicht in einer .md: laege er in
# einer .md, truege die Schicht (a) den Fall und er saehe nur so aus, als pruefe
# er die Vendor-Schicht. So ist er von (a) UNABHAENGIG -- streicht man die
# *.md-Zeile, bleibt dieser Fall gruen; streicht man die Vendor-Zeile, wird er
# rot. Das ist die geforderte EINZELNE Deckung, nicht die gemeinsame.
# Wie in den Faellen 11 und 12 liegt eine saubere .sh daneben, damit der Nenner
# nicht null ist -- ein Gruen ueber null geprueften Zeilen waere kein Gruen.
st_neues_repo
mkdir -p "${_st_repo}/ext/io/fremd_paket" \
    || st_abbruch "mkdir des Vendor-Fixtures (Fall 13) fehlgeschlagen."
printf 'Herkunft: Fremd-Snapshot, Stufe faithful. Kennzeichen %s.\n' "$_st_kennzeichen" \
    > "${_st_repo}/ext/io/fremd_paket/COMDARE-VENDOR-PROVENANCE.md"
printf '/* upstream comment %s %s */\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/ext/io/fremd_paket/upstream_modul.c"
printf '/* %s */\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    >> "${_st_repo}/ext/io/fremd_paket/upstream_modul.c"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_vendor.sh"
git -C "$_st_repo" add -- ext scripts/probe_vendor.sh \
    || st_abbruch "git add (Fall 13) fehlgeschlagen."
# Die zweite Zusicherung nennt den GRUND und die DATEI: nicht "irgendetwas wurde
# uebersprungen", sondern "genau diese Fremddatei, und zwar als Vendor-Baum".
_st_erw=$(printf '%s\n%s\n' \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "- ext/io/fremd_paket/upstream_modul.c  [Vendor-Baum, COMDARE-VENDOR-PROVENANCE.md]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "13 Vendor-Ausnahme haelt" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 14: GEGENEINGANG zu Fall 13 -- ohne die Provenance-Datei MUSS es beissen ---
# T-4/K13: zu jeder Zusicherung ein Eingang, bei dem sie NICHT gilt. Ohne diesen
# Fall waere Fall 13 auch von einer Wache zu bestehen, die pauschal alles unter
# ext/ ueberspringt -- und genau diese pauschale ext/-Ausnahme lehnt der Kopf der
# Wache ausdruecklich ab, weil sie comdare-EIGENE Dateien dort (ext/CMakeLists.txt,
# die Vendor-Wrapper) mit blind stellen wuerde.
# Der Baum ist BIS AUF DIE PROVENANCE-DATEI identisch mit Fall 13: derselbe Pfad,
# dieselbe Datei, derselbe gewuerfelte Koeder. Genau EINE Variable unterscheidet
# die beiden Faelle -- damit ist der Unterschied im Verdikt dieser Variable
# zurechenbar und keiner Begleitumstaendlichkeit.
st_neues_repo
mkdir -p "${_st_repo}/ext/io/fremd_paket" \
    || st_abbruch "mkdir des Vendor-Fixtures (Fall 14) fehlgeschlagen."
printf '/* upstream comment %s %s */\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/ext/io/fremd_paket/upstream_modul.c"
printf '/* %s */\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    >> "${_st_repo}/ext/io/fremd_paket/upstream_modul.c"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_vendor.sh"
git -C "$_st_repo" add -- ext scripts/probe_vendor.sh \
    || st_abbruch "git add (Fall 14) fehlgeschlagen."
# BEIDE Verstossklassen muessen gemeldet werden, und zwar NAMENTLICH an dieser
# Datei -- ein blosses "Exit 1" waere auch von einer Wache zu haben, die etwas
# ganz anderes anmeckert.
_st_erw=$(printf '%s\n%s\n%s\n' \
    "NICHT-ASCII" \
    ">120-SPALTEN" \
    "ext/io/fremd_paket/upstream_modul.c")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "14 ohne Provenance beisst es" 1 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 15: die awk-Entklammerung, EINZELN gepinnt (--stdin) ---
# Zweite Haelfte des Pruefer-Befunds zu Fall 12 (s. dort): der Griff gegen die
# C-Quotierung hat zwei Schichten, und Fall 12 pinnt nur core.quotePath=false.
# Die awk-Seite ist genau dort die einzige Verteidigung, wo die Wache die
# git-Konfiguration des Aufrufers NICHT kennt -- im --stdin-Betrieb. Also wird
# der Diff hier ABSICHTLICH mit core.quotePath=true erzeugt, so wie ihn ein
# Aufrufer mit Vorgabe-Konfiguration liefert, und in die Wache gefuettert.
# Mutationsgemessen: streicht man die vier awk-Zeilen, die die Klammerung
# abstreifen, blieb der Selbsttest 12 von 12 gruen. Dieser Fall wird dabei rot.
st_neues_repo
_st_mdname="HANDBUCH-${_st_na}-${_st_kennzeichen}.md"
printf 'Deutsche Doku-Prosa mit Umlaut %s im Text UND im Dateinamen.\n' "$_st_na" \
    > "${_st_repo}/${_st_mdname}"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_stdin.sh"
git -C "$_st_repo" add -- "$_st_mdname" scripts/probe_stdin.sh \
    || st_abbruch "git add (Fall 15) fehlgeschlagen."
git -C "$_st_repo" -c core.quotePath=true \
    diff -U0 --no-color --no-ext-diff HEAD > "$_st_diff"
_st_rc=$?
[ "$_st_rc" -eq 0 ] \
    || st_abbruch "Erzeugen des quotierten Diffs (Fall 15) fehlgeschlagen (rc=${_st_rc})."
# KOEDER-KONTROLLE (V-8): "Was waere der Zustand, in dem dieser Fall gruen ist
# und die Sache trotzdem nicht existiert?" -- ein Diff OHNE Klammerung. Dann
# haette die awk-Entklammerung nie etwas zu tun gehabt und das Gruen waere leer.
# Also wird hier am OBJEKT geprueft, dass die Eingabe den Koeder wirklich traegt.
LC_ALL=C grep -q -F -- '+++ "b/' "$_st_diff" \
    || st_abbruch "Fall 15: der erzeugte Diff traegt KEINE C-Quotierung -- der Koeder beisst nicht."
_st_erw=$(printf '%s\n%s\n' \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "[Doku-Prosa *.md, Sprachdoktrin]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh --stdin < "$_st_diff" ) \
    > "$_st_out" 2>&1
st_bewerte "15 awk-Entklammerung haelt (stdin)" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 16: die NUR-BREITEN-Ausnahme fuer *.xml haelt -- und sie ist nicht still ---
# BEFUND, DER DIESEN FALL ERZWINGT (2026-08-16, golden-homes-Landung, am Objekt
# gemessen): der Landebereich 0eea2a0a..369b62ce trug 131 Breiten-Verstoesse,
# davon 96 in XML -- 83 davon GENERIERTE Registry-Records (EIN Record je Zeile;
# "NICHT von Hand editieren" steht im Kopf der Datei selbst). Die 120er-Grenze
# ist der ColumnLimit aus .clang-format, ein C++-Format-Vertrag; einen
# XML-Formatierer-Vertrag gibt es in diesem Baum nicht. Die Wache stellt
# deshalb fuer *.xml NUR die Breiten-Regel frei; ASCII bleibt scharf (Fall 17),
# und dieselbe Ueberlaenge AUSSERHALB von *.xml beisst weiter (Fall 6).
# Die Zusicherungen pinnen beide Haelften der Meldung: der Nenner zaehlt die
# Zeile als GEPRUEFT (nicht uebersprungen -- das trennt diese Schicht von
# skip_grund()), und die Datei steht NAMENTLICH mit GRUND in der
# Breiten-Ausnahme. Eine stille dritte Klasse waere die naechste Falle.
st_neues_repo
printf '<!-- %s %s -->\n' "$_st_kennzeichen" "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    > "${_st_repo}/probe_daten.xml"
git -C "$_st_repo" add probe_daten.xml || st_abbruch "git add (Fall 16) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n%s\n' \
    "GRUEN" \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "- probe_daten.xml  [XML-Markup *.xml, nur Breite frei (ASCII gilt)]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "16 xml-Breiten-Ausnahme haelt" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 17: ASCII bleibt in *.xml SCHARF -- die Ausnahme ist NUR die Breite ---
# Ohne diesen Fall waere Fall 16 auch von einer dritten VOLL-Ausnahme in
# skip_grund() zu bestehen -- dann waere *.xml komplett blind, also exakt der
# Zustand der Whitelist-Aera vor dem 10.08.2026 zurueck. Der Koeder ist die
# gleiche Datei-Klasse mit dem gewuerfelten Nicht-ASCII-Zeichen; er MUSS
# NAMENTLICH gemeldet werden, nicht bloss den Exit kippen.
st_neues_repo
printf '<!-- KOEDER %s %s -->\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/probe_daten.xml"
git -C "$_st_repo" add probe_daten.xml || st_abbruch "git add (Fall 17) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n' \
    "NICHT-ASCII" \
    "probe_daten.xml")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "17 xml bleibt ASCII-geprueft" 1 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 18: KOMBI-Koeder -- ueberlang UND Nicht-ASCII in *.xml ---
# Die Breiten-Freiheit (Fall 16) darf die ASCII-Schaerfe (Fall 17) nicht
# verdecken: eine Zeile, die BEIDE Verletzungen traegt, MUSS ueber die
# ASCII-Stufe ROT werden (namentlich), obwohl ihre Breite fuer *.xml frei
# ist. Ohne diesen Fall bestuende eine Wache, die ASCII nur an kurzen
# xml-Zeilen prueft, die Faelle 16 UND 17 -- die Kombination fing keiner.
st_neues_repo
_st_pad=$(printf 'x%.0s' $(seq 1 130))
printf '<!-- KOEDER %s %s %s -->\n' "$_st_kennzeichen" "$_st_na" "$_st_pad" \
    > "${_st_repo}/probe_daten.xml"
git -C "$_st_repo" add probe_daten.xml || st_abbruch "git add (Fall 18) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n' \
    "NICHT-ASCII" \
    "probe_daten.xml")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "18 xml-Kombi ueberlang+non-ASCII bleibt ROT" 1 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 19: die LIZENZTEXT-Ausnahme haelt -- LICENSES/*.txt bleibt ungeprueft, aber benannt ---
# BEFUND, DER DIESEN FALL ERZWINGT (2026-08-27, ce-Lande-Zug lande/identitaet-2708, am Objekt
# gemessen): der REUSE-3.3-Einzug (bau/a5-reuse) brachte 28 wortgetreue Lizenztexte unter
# LICENSES/*.txt; das kumulative Gate ueber d3b5a393..60d997a6 meldete darin 795 Verstoesse
# (190 Nicht-ASCII, 605 Breite) in 18 Rechtstexten, die niemand umbrechen darf. skip_grund()
# traegt seither eine DRITTE Schicht: LICENSES/*.txt. Sie ist von (a) *.md und (b) Vendor-
# Provenance UNABHAENGIG: der Koeder liegt in einer .txt, ohne Provenance-Datei daneben --
# streicht man die Lizenztext-Zeile, wird dieser Fall rot, 11-14 bleiben gruen.
# Wie in Fall 13 liegt eine saubere .sh daneben, damit der Nenner nicht null ist.
st_neues_repo
mkdir -p "${_st_repo}/LICENSES" || st_abbruch "mkdir LICENSES (Fall 19) fehlgeschlagen."
printf 'Lizenztext-Koeder %s %s\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/LICENSES/LicenseRef-Koeder.txt"
printf '%s\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    >> "${_st_repo}/LICENSES/LicenseRef-Koeder.txt"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_lizenz.sh"
git -C "$_st_repo" add -- LICENSES scripts/probe_lizenz.sh \
    || st_abbruch "git add (Fall 19) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n' \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" \
    "- LICENSES/LicenseRef-Koeder.txt  [Lizenztext LICENSES/*.txt, REUSE 3.3 wortgetreu]")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "19 Lizenztext-Ausnahme haelt" 0 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 20: GEGENEINGANG (Pfad) -- derselbe Lizenztext AUSSERHALB von LICENSES/ MUSS beissen ---
# Ohne diesen Fall waere Fall 19 auch von einer Wache zu bestehen, die pauschal alle *.txt
# ueberspringt -- die Rueckkehr der Whitelist-Aera (bis 10.08.2026 fiel *.txt lautlos heraus).
# Bis auf den PFAD identisch mit Fall 19: dieselbe Datei, derselbe gewuerfelte Koeder.
st_neues_repo
mkdir -p "${_st_repo}/docs/lizenzen" || st_abbruch "mkdir docs/lizenzen (Fall 20) fehlgeschlagen."
printf 'Lizenztext-Koeder %s %s\n' "$_st_kennzeichen" "$_st_na" \
    > "${_st_repo}/docs/lizenzen/LicenseRef-Koeder.txt"
printf '%s\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" \
    >> "${_st_repo}/docs/lizenzen/LicenseRef-Koeder.txt"
printf '# saubere Shell-Zeile, reines ASCII (%s)\n' "$_st_kennzeichen" \
    > "${_st_repo}/scripts/probe_lizenz.sh"
git -C "$_st_repo" add -- docs scripts/probe_lizenz.sh \
    || st_abbruch "git add (Fall 20) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n%s\n' \
    "NICHT-ASCII" \
    ">120-SPALTEN" \
    "docs/lizenzen/LicenseRef-Koeder.txt")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "20 ausserhalb LICENSES/ beisst es" 1 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

# --- Fall 21: GEGENEINGANG (Endung) -- eine .sh IN LICENSES/ MUSS beissen ---
# Pinnt die Endungs-Bindung der dritten Schicht: LICENSES/ ist KEIN Blanket-Verzeichnis, nur
# der wortgetreue Lizenztext (*.txt) ist frei; eigener Code dort bleibt geprueft.
st_neues_repo
mkdir -p "${_st_repo}/LICENSES" || st_abbruch "mkdir LICENSES (Fall 21) fehlgeschlagen."
printf '# Koeder %s %s\n' "$_st_kennzeichen" "$_st_na" > "${_st_repo}/LICENSES/koeder_wrapper.sh"
printf '# %s\n' "$(od -An -N70 -tx1 /dev/urandom | tr -d ' \n')" >> "${_st_repo}/LICENSES/koeder_wrapper.sh"
git -C "$_st_repo" add -- LICENSES || st_abbruch "git add (Fall 21) fehlgeschlagen."
_st_erw=$(printf '%s\n%s\n%s\n' \
    "NICHT-ASCII" \
    ">120-SPALTEN" \
    "LICENSES/koeder_wrapper.sh")
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
st_bewerte "21 .sh in LICENSES/ beisst es" 1 "$_st_erw" "$?" "$_st_out"
rm -rf "$_st_repo"

rm -f "$_st_out" "$_st_muster_datei" "$_st_diff"

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Null):"
echo "  ${_st_gruen} von ${_st_gesamt} Faellen wie erwartet, ${_st_rot} abweichend."
echo "-----------------------------------------------------------------------------"
echo ""
if [ "$_st_rot" -eq 0 ] && [ "$_st_gesamt" -gt 0 ]; then
    echo "WACHE-SELBSTTEST: GRUEN."
    exit 0
fi
[ "$_st_gesamt" -gt 0 ] || st_abbruch "0 Faelle gelaufen -- das ist kein Gruen."
echo "WACHE-SELBSTTEST: ROT."
exit 1
