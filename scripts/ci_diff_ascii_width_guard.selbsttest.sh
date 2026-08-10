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

# st_bewerte <fallname> <erwarteter_exit> <erwartetes_muster_oder_leer> <tatsaechlicher_exit> <ausgabedatei>
st_bewerte() {
    _st_fall="$1"; _st_soll="$2"; _st_muster="$3"; _st_ist="$4"; _st_datei="$5"
    _st_gesamt=$(( _st_gesamt + 1 ))
    _st_ok=1
    if [ "$_st_ist" -ne "$_st_soll" ]; then
        _st_ok=0
        _st_grund="Exit ${_st_ist}, erwartet ${_st_soll}"
    elif [ -n "$_st_muster" ]; then
        if LC_ALL=C grep -q -F -- "$_st_muster" "$_st_datei"; then
            _st_grund=""
        else
            _st_ok=0
            _st_grund="Exit stimmt (${_st_ist}), aber die Ausgabe enthaelt nicht: '${_st_muster}'"
        fi
    else
        _st_grund=""
    fi
    if [ "$_st_ok" -eq 1 ]; then
        _st_gruen=$(( _st_gruen + 1 ))
        printf '  [ok ] %-34s Exit %d wie erwartet\n' "$_st_fall" "$_st_ist"
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
( cd "$_st_repo" && sh scripts/ci_diff_ascii_width_guard.sh ) > "$_st_out" 2>&1
# Das Muster ist bewusst die NENNER-Zeile und nicht bloss "GRUEN": genau EINE der
# beiden Zusatzzeilen darf geprueft worden sein (die .sh), die andere (die .md)
# gehoert in die uebersprungene Menge. Ein blosses "Exit 0" waere auch von einer
# Wache zu bestehen, die BEIDE Dateien uebersieht -- also von genau dem Defekt,
# den Fall 10 aufdeckt. So ist der Gegeneingang eine AUSSAGE, keine Anwesenheit.
st_bewerte "11 md-Ausnahme haelt, sh geprueft" 0 \
    "1 Zusatzzeilen in selbst verfasstem Code geprueft" "$?" "$_st_out"
rm -rf "$_st_repo"

rm -f "$_st_out"

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
