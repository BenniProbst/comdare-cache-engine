#!/bin/sh
# shellcheck shell=sh
# =============================================================================
#  TEST-REGISTRIERUNGS-WACHE -- keine Test-QUELLDATEI darf ausserhalb des
#  Bauwegs liegen. (MT-L4, 2026-08-09)
#  GOAL v8 Teil VIII, T-7: "Ein Test existiert erst, wenn er in `ctest -N`
#  erscheint UND sein Binary im Bauweg haengt."
# =============================================================================
#
# DER BEFUND, GEGEN DEN SIE GEBAUT IST (am Objekt gemessen, 09.08.2026, ce aebc4f2c):
# Vier Test-Quelldateien lagen im Repository, waren getrackt, waren uebersetzbar --
# und kamen in KEINER CMakeLists.txt vor. Sie wurden nie gebaut, nie ausgefuehrt,
# nie gezaehlt. Gemessen: je 0 Treffer in tests/unit/CMakeLists.txt.
#   tests/unit/test_a9b_active_deklaration_inert.cpp     seit 26.07.2026 (14 Tage)
#   tests/unit/test_c3b_kanal_merge_beleg.cpp            seit 26.07.2026 (14 Tage)
#   tests/unit/test_rf2_admission_marker_inert.cpp       seit 26.07.2026 (14 Tage)
#   tests/unit/test_d4b_container_dll.cpp                seit 02.06.2026 (68 Tage)
# Der Erstlauf von rf2 war ROT: eine Zusicherung stand seit 26.07. auf einer
# CSV-Zellzahl, die der Renderer inzwischen geaendert hat. 14 Tage lang hat das
# niemand bemerkt, weil die Datei nie gebaut wurde.
#
# WARUM DIE VORHANDENE SICHTBARKEITS-WACHE DAS NICHT FINDET -- die Luecke ist strukturell:
# scripts/ci_test_sichtbarkeit_wache.sh vergleicht SOLL (die Registrierungs-AUFRUFE im
# CMake-Quelltext) gegen IST (`ctest -N`). Eine Datei, die gar kein add_test aufruft,
# taucht in ihrem SOLL nie auf -- sie kann von ihr nicht vermisst werden. Beide Wachen
# pruefen deshalb verschiedene Stufen derselben Kette und ersetzen einander NICHT:
#   ci_test_sichtbarkeit_wache.sh  Registrierung   -> ctest-Eintrag   (Stufe 2)
#   ci_test_registrierungs_wache.sh  Quelldatei    -> Bauweg          (Stufe 1, hier)
#
# DER NENNER KOMMT VON AUSSEN (V-7 / T-3). Beide Zahlen dieser Wache stammen aus
# verschiedenen Quellen, und KEINE der beiden ist die CMake-Datei, die geprueft wird:
#   SOLL  `git ls-files` -- der Git-Index. Faellt eine Registrierung aus der
#         CMakeLists.txt, sinkt der SOLL NICHT mit. Genau das ist der Punkt: eine
#         Selbst-Inventur gegen einen Selbst-Selektor waere kein Vergleich.
#   IST   der GEBAUTE Baum (compile_commands.json, sonst build.ninja) -- also der
#         Gegenstand, nicht die Ankuendigung. Ein comdare_add_test(), das hinter einer
#         falschen Bedingung steht, steht im Quelltext und trotzdem nicht im Bauweg;
#         nur der gebaute Baum weiss das.
#
# WARUM NICHT PER TEXT-GREP GEGEN tests/unit/CMakeLists.txt: das beantwortet die
# falsche Frage. Am Objekt gemessen (09.08.2026) stehen vier Dateien
# (test_concepts_compile, test_value_handle, test_three_layer_audit,
# test_six_page_structures) MIT comdare_add_test() im Quelltext -- und sind trotzdem
# nicht im Bauweg, weil ihr if(COMDARE_PRT_ART_LEGACY_AVAILABLE) im Standalone-Bau
# falsch ist. Ein Quelltext-Grep haette sie als "registriert" durchgewinkt.
#
# WAS SIE VERLANGT -- Rechenschaft, nicht Vollstaendigkeit:
# Nicht jede Test-Quelldatei MUSS in jedem Baum gebaut werden; ein Test hinter einem
# optionalen Fremdbaum ist dort legitim abwesend. Verlangt wird, dass jede Abwesenheit
# BEGRUENDET in scripts/ci_test_registrierungs_allowlist.txt steht -- und dass die
# Begruendung noch gilt.
#
# DIE BEGRUENDUNG WIRD AM GEGENSTAND NACHGEPRUEFT (V-8), nicht geglaubt: jede
# Allowlist-Zeile nennt einen GEGENSTAND, dessen ABWESENHEIT die Ausnahme traegt. Ist
# der Gegenstand da, ist die Ausnahme erloschen und die Zeile wird ROT -- auch wenn
# niemand die Allowlist angefasst hat. Eine Allowlist ohne diese Gegenprobe waere ein
# Freibrief mit unbegrenzter Laufzeit.
#
# ZWEI GEGENSTANDSARTEN, WEIL EINE NICHT REICHTE (2026-08-10, Pipeline 15517).
# Bis heute kannte Feld 2 genau eine Art: einen PFAD, geprueft mit '[ -e ]'. Das setzt
# eine Datei voraus -- und der erste echte Fang der Wache haengt an gar keiner Datei:
#   tests/unit/test_ap5_simd_extension_coherence.cpp steht in tests/unit/CMakeLists.txt
#   in einem add_executable() (:4174) INNERHALB von
#   'if(COMDARE_HOST_RUNS_AVX2 AND COMDARE_HOST_RUNS_AVX512F)' (:4165). Auf einem Host
#   ohne AVX-512 entsteht das Ziel nie, die Quelldatei wird nie uebersetzt -- und es gibt
#   keine Datei, deren Abwesenheit man dafuer nennen koennte.
# Feld 2 traegt deshalb ab hier ein PRAEFIX, das die ART benennt:
#   datei:<pfad>              Ausnahme traegt, solange der Pfad NICHT existiert.
#   isa:<merkmal>[+<merkmal>] Ausnahme traegt, solange MINDESTENS EIN Merkmal auf dem
#                             Bau-Host fehlt -- die Negation des CMake-UND-Gatters.
# Eine Art, die diese Wache nicht kennt, ist NICHT pruefbar: die Zeile wird ROT (Abschnitt
# UNPRUEFBARE BEGRUENDUNG). Dasselbe gilt fuer ein leeres Feld 2 und fuer ein unbekanntes
# ISA-Merkmal. Fail-closed heisst hier woertlich: eine Begruendung, die niemand nachpruefen
# kann, ist keine Begruendung, sondern ein Freibrief -- und Freibriefe sind der Zustand,
# gegen den diese Wache gebaut ist.
#
# WORAN 'isa:' NACHGEPRUEFT WIRD -- und warum NICHT an /proc/cpuinfo: gefragt ist nicht,
# was die Maschine kann, auf der die Wache gerade laeuft, sondern was der BAU DIESES BAUMS
# vorgefunden hat. Das faellt auseinander, sobald Container, Cross-Build oder ein zweiter
# Compiler im Spiel sind. Die einzige Quelle, die genau das festhaelt, ist der CMakeCache
# DESSELBEN Baums: dort steht das Ergebnis von check_cxx_source_runs mit
# __builtin_cpu_supports (tests/unit/CMakeLists.txt:4129-4130).
# DAS IST BEWUSST DIESELBE QUELLE UND DASSELBE VOKABULAR wie in
# scripts/ci_test_coverage_guard.sh (Host-Klassen avx512f/avx2/basis, D2-G5) -- zwei Wachen
# duerfen nicht zwei verschiedene Begriffe fuer dieselbe Host-Klasse fuehren. Die Merkmale
# heissen hier klein (avx2, avx512f) und bilden auf COMDARE_HOST_RUNS_<GROSS> ab.
# OFFEN, ausdruecklich: die Cache-Lesung steht damit in ZWEI Skripten. Ein geteilter Helfer
# ist die richtige Auflaufsung, aber ein eigenes Paket -- und die Shell-Menge waechst dafuer
# nicht (Owner-Kern 2026-08-09).
#
# DER BLOSSE WERT IM CACHE GENUEGT NICHT (D2-G5, am Objekt gemessen 2026-08-10): CMake
# entfernt beim Erzwingen per '-D' weder _COMPILED noch _EXITCODE -- sie ueberleben aus dem
# ehrlichen Lauf davor. Eine Wache, die nur den Wert liest, nimmt eine BEHAUPTETE Klasse
# fuer eine gemessene. Geprueft werden deshalb vier Merkmale, alle Pflicht: die Wertzeile
# steht GENAU EINMAL im Cache, ihr Typ ist INTERNAL (nur check_cxx_source_runs schreibt den),
# _COMPILED ist TRUE, und Wert und _EXITCODE decken sich. Faellt eines davon, ist die
# ISA-Frage UNBEANTWORTBAR -- und das ist Exit 2, nicht 'Merkmal fehlt' und damit gruen.
#
# ERLOESCHEN HAT ZWEI RICHTUNGEN -- bis 2026-08-10 kannte diese Wache nur EINE (PA-1).
# Die alte Frage war: "existiert der Gegenstand WIEDER?" Sie faengt den Fall, dass der
# Gegenstand zurueckkehrt. Sie faengt NICHT den Fall, dass er NIE zurueckkehren kann --
# und genau der lag vier Zeilen lang vor:
#   Alle vier prt-art-Zeilen banden an 'datei:prt_art/include/prt_art/prt_art.hpp'.
#   Am Objekt gemessen (2026-08-10): 0 Treffer fuer 'prt_art.hpp' unter /home/comdare,
#   Gegenprobe 'cache_engine.hpp' 45 Treffer -- das Werkzeug sucht. Der Pfad war 2 Tage
#   lang getrackt (angelegt 2026-05-12 in 5c210581), zog am 2026-05-14 mit 'git mv' nach
#   libs/deprecated/prt_art_legacy/ (b9abc720, R100, alle 44 Pfade umbenannt) und fiel dort
#   am 2026-06-01 mit dem ganzen Baum (6b3ed0d9). Seither kann ihn nichts mehr erzeugen.
#   FOLGE: '[ -e ]' war auf ewig falsch, die vier Zeilen konnten per Konstruktion nie
#   erloeschen -- eine Freistellung mit unbegrenzter Laufzeit, also genau der Zustand,
#   gegen den diese Wache gebaut ist. Der Erstlauf gegen einen Baum, dem GENAU diese vier
#   Dateien fehlten, meldete woertlich "OK (470 Quelldateien, 0 ohne Begruendung)".
#
# DIE ZWEITE RICHTUNG heisst hier TOTE AUSNAHME und ist ROT, nicht Warnung: eine Warnung
# reproduziert exakt die Fehlerklasse (14 bzw. 68 Tage lang ist niemandem etwas aufgefallen),
# und die Hausregel kennt nur ZELLE=Warnung, JOB=hart rot.
#
# WAS DIE WACHE DAFUER WIRKLICH MISST -- und was das NICHT beweist:
# "Kann nie entstehen" ist nicht entscheidbar, solange man nur den eigenen Baum kennt.
# Gemessen wird deshalb etwas Engeres und Nachpruefbares: DIESES REPOSITORY ERKLAERT NICHTS
# UEBER DEN GEGENSTAND. Erklaert ist ein Pfad genau dann, wenn eine der drei Quellen ihn
# kennt -- der Git-Index (getrackte Datei ODER Submodul-Gitlink), eine .gitignore-Regel
# (also ein angemeldeter Ablageort fuer Erzeugtes), oder der Index/eine Ignore-Regel fuer
# sein ELTERNVERZEICHNIS. Kennt keine davon ihn, hat der Gegenstand in diesem Repository
# keinen Erzeuger.
# DAS BEWEIST NICHT, dass er nie existieren kann. Jemand kann ihn morgen 'git add'en, ein
# Pruefling-Repo kann irgendwo eingehaengt werden, ein CI-Job kann ihn hineinkopieren.
# Bewiesen ist nur: heute nennt ihn keine Quelle dieses Repos. Das genuegt fuer den Zweck --
# eine Begruendung, deren Gegenstand kein Erzeuger dieses Baums kennt, ist keine
# Begruendung, sondern ein Freibrief.
# WEITERE GRENZEN, ausdruecklich: (a) ein Pfad AUSSERHALB des Repos (absolut oder mit einem
# '..'-Segment) ist so nicht beurteilbar -- er zaehlt als NICHT BEURTEILBAR und wird nie als
# tot gemeldet; (b) ein Gegenstand OHNE Verzeichnisanteil hat die Repo-Wurzel als Elternteil,
# und die ist immer bekannt -- er kann deshalb nie als tot auffallen; (c) ein Gegenstand
# DIREKT in einem vorhandenen Verzeichnis ist immer erreichbar, auch wenn ihn dort nie jemand
# anlegen wird -- 'tests/unit/erfunden.hpp' faellt nicht auf. Die Regel faengt den Fall, dass
# ein ganzer ZWEIG fehlt, nicht den, dass ein einzelner Name erfunden ist.
#
# WARUM DIE VERZEICHNISSE UND NICHT NUR DER PFAD: ein nicht existierender Pfad ist per
# Definition weder getrackt noch (meist) ignoriert -- die Frage waere fuer JEDEN abwesenden
# Gegenstand mit 'ja' zu beantworten und die Regel unbrauchbar. Entschieden wird deshalb an
# der AHNENREIHE, vom direkten Elternteil aufwaerts, und der ERSTE bekannte Vorfahr gibt die
# Antwort:
#   ignoriert       -> erreichbar. 'build/tools/y' darf bei jedem Bau entstehen.
#   Gitlink (160000)-> erreichbar. Ein nicht ausgechecktes Submodul kann beliebig tief
#                      etwas mitbringen; sein Inhalt steht nie im Index des Obenprojekts.
#   getrackter Inhalt, und es ist der DIREKTE Elternteil -> erreichbar. Eine neue Datei in
#                      einem vorhandenen Verzeichnis ist der Normalfall.
#   getrackter Inhalt, aber WEITER OBEN -> tot. git zaehlt dieses Verzeichnis vollstaendig
#                      auf, und der Zweig, den der Gegenstand darunter braucht, ist nicht
#                      dabei.
#   gar kein bekannter Vorfahr bis zur Wurzel -> tot.
# WARUM NICHT NUR DER DIREKTE ELTERNTEIL, am Objekt gefunden (2026-08-10, beim ersten Lauf
# des Google Tests): der direkte Elternteil von 'ext/<submodul>/include/kopf.hpp' ist
# 'ext/<submodul>/include' -- und der steht NICHT im Index, weil dort nur der GITLINK
# 'ext/<submodul>' steht. Die erste Fassung erklaerte diesen Pfad faelschlich fuer tot. Der
# Fall SubmodulGitlinkIstErreichbar in tests/unit/test_pa1_tote_ausnahme.cpp haelt ihn fest.
# ACHTUNG, ebenfalls am Objekt gefunden: 'git check-ignore build' meldet NICHT ignoriert,
# 'git check-ignore build/' meldet ignoriert -- ein Verzeichnismuster braucht den
# Schraegstrich. Die Abfrage unten stellt beide Formen.
#
# DIE DRITTE ART 'frist:' -- fuer die ehrlich unbeweisbare Ausnahme. Es gibt Faelle, in denen
# es KEINEN erreichbaren Gegenstand gibt, den man nennen koennte; die vier prt-art-Dateien
# sind genau das (am Objekt gemessen 2026-08-10: KEINER der Header, die sie inkludieren --
# prt_art/prt_art.hpp, prt_art/concepts/*, prt_art/page_structures/*, prt_art/interpreters/*
# -- liegt im Nachbarrepo comdare-prt-art; die Verzeichnisse concepts/, page_structures/ und
# interpreters/ existieren dort nicht). Eine solche Ausnahme darf nicht als Pfad-Fiktion
# getarnt werden, sie muss als das auftreten, was sie ist: ein geparkter Posten. 'frist:'
# bindet sie an ein DATUM statt an einen Gegenstand -- danach wird sie ROT. Damit kann auch
# sie keine unbegrenzte Laufzeit haben.
#
# AUFRUF:  sh scripts/ci_test_registrierungs_wache.sh <build-verzeichnis>
# EXIT:    0 = jede getrackte Test-Quelldatei ist im Bauweg oder begruendet abwesend
#          1 = mindestens eine Test-Quelldatei OHNE gueltige Begruendung ausserhalb --
#              ohne Allowlist-Zeile, mit ERLOSCHENER, mit TOTER oder mit UNPRUEFBARER
#              Begruendung
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen)
#
# GRENZE, EHRLICH BENANNT -- was diese Wache NICHT deckt:
# Sie prueft, ob die Quelldatei UEBERSETZT wird. Sie prueft NICHT, ob das entstehende
# Binary auch als ctest-Eintrag registriert ist -- das ist Stufe 2 und Sache von
# ci_test_sichtbarkeit_wache.sh. Eine Datei kann also hier gruen sein und trotzdem
# keinen ctest-Eintrag haben; erst BEIDE Wachen zusammen schliessen die Kette
# "Datei -> Uebersetzung -> ctest-Eintrag".
#
# POSIX-sh, ASCII-only, kein Python (Hausdoktrin).
# =============================================================================

set -eu

BUILD="${1:-}"
if [ -z "$BUILD" ]; then
    echo "AUFRUF: sh scripts/ci_test_registrierungs_wache.sh <build-verzeichnis>" >&2
    exit 2
fi
if [ ! -d "$BUILD" ]; then
    echo "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
    exit 2
fi

# Den Bau-Baum ZUERST absolut aufloesen -- noch aus dem Verzeichnis des Aufrufers.
# Nach dem Wechsel in die Repo-Wurzel wuerde ein relativer Pfad gegen die Wurzel
# statt gegen den Aufrufer aufgeloest; das faellt im CI nicht auf (er ruft aus der
# Wurzel) und waere von Hand eine stille Fehlmessung.
BUILD_ABS=$(cd "$BUILD" && pwd) || exit 2

WURZEL=$(git rev-parse --show-toplevel 2>/dev/null) || {
    echo "ABBRUCH: kein git-Arbeitsbaum -- der SOLL-Nenner ist nicht erhebbar." >&2
    exit 2
}
cd "$WURZEL" || exit 2

GREP=/usr/bin/grep
[ -x "$GREP" ] || GREP=grep

# ---------------------------------------------------------------------------
# IST-QUELLE: der GEBAUTE Baum. compile_commands.json ist generator-unabhaengig
# und wird bevorzugt; build.ninja ist der Fallback (der CI baut mit
# --with-generator=Ninja). Findet sich keine von beiden, ist das FAIL-CLOSED --
# eine Wache, die nicht pruefen kann, meldet nicht "in Ordnung".
# ---------------------------------------------------------------------------
IST_DATEI=""
IST_ART=""
if [ -f "$BUILD_ABS/compile_commands.json" ]; then
    IST_DATEI="$BUILD_ABS/compile_commands.json"
    IST_ART="compile_commands.json"
elif [ -f "$BUILD_ABS/build.ninja" ]; then
    IST_DATEI="$BUILD_ABS/build.ninja"
    IST_ART="build.ninja"
else
    echo "ABBRUCH: weder compile_commands.json noch build.ninja unter '$BUILD'." >&2
    echo "         Der IST-Nenner waere leer -- jede Datei erschiene als unregistriert." >&2
    echo "         Das ist fail-closed und ausdruecklich KEIN Gruen." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# MESSGERAET-GEGENPROBE (V4): bevor ein Nullbefund etwas bedeutet, muss belegt
# sein, dass das Werkzeug ueberhaupt sucht. Eine Datei, von der wir wissen, dass
# sie gebaut wird, MUSS treffen. Trifft sie nicht, ist nicht der Baum kaputt,
# sondern die Messung -- und dann darf diese Wache kein Urteil faellen.
# ---------------------------------------------------------------------------
GEGENPROBE="tests/unit/test_pressure_state.cpp"
if [ ! -f "$GEGENPROBE" ]; then
    echo "ABBRUCH: die Gegenprobe-Datei '$GEGENPROBE' existiert nicht mehr." >&2
    echo "         Ohne Gegenprobe ist ein Nullbefund nicht von einem kaputten Muster zu trennen." >&2
    exit 2
fi
if ! "$GREP" -q -F -- "/$GEGENPROBE" "$IST_DATEI"; then
    echo "ABBRUCH: die Gegenprobe '$GEGENPROBE' steht NICHT in $IST_ART." >&2
    echo "         Entweder ist der Baum nicht konfiguriert, oder das Suchmuster trifft nicht." >&2
    echo "         In beiden Faellen waere jeder Nichtfund dieser Wache wertlos." >&2
    exit 2
fi

ALLOWLIST="scripts/ci_test_registrierungs_allowlist.txt"

# ---------------------------------------------------------------------------
# SOLL: alle getrackten Test-Quelldateien im GANZEN Baum, ohne den vendorierten
# ext/-Baum (fremder Code, fremde Bauwege).
#
# WARUM REPO-WEIT UND NICHT NUR tests/: die zwei Dateien, an denen dieser ganze
# Befund haengt, liegen gar nicht unter tests/ --
# libs/cache_engine/builder/commands/tests/test_commands.cpp und
# test_engine_adapters.cpp. Ihre 27 gtest-Faelle waren 79 Tage lang in jedem Baum
# unsichtbar (W-1: enable_testing() stand nach dem add_subdirectory). Eine Wache
# gegen unsichtbare Tests, die ausgerechnet diese beiden nicht im Nenner hat,
# waere eine Wache mit einem Loch an genau der Stelle des Vorfalls.
# Am Objekt gemessen (09.08.2026): 459 Dateien insgesamt, 456 unter tests/ und
# 3 darunter -- die zwei oben plus
# libs/cache_engine/builder/workload_driver/test_load_profile_writer.cpp.
#
# Das Muster ist am DATEINAMEN verankert, nicht am Pfad, und zwar bewusst nach
# `git ls-files` statt als Pathspec: in einem git-Pathspec matcht '*' auch '/',
# weshalb '*/test_*.cpp' auch
# libs/test_infra/workload_generator/src/workload_generator.cpp trifft -- eine
# Datei, die kein Test ist. Am Objekt geprueft: Pathspec 460 Treffer,
# Basename-Filter 459.
# ---------------------------------------------------------------------------
TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

git ls-files 2>/dev/null | "$GREP" -v '^ext/' | "$GREP" -v '/ext/' |
    "$GREP" -E '(^|/)test_[^/]*\.cpp$' | sort > "$TMP/soll.txt" || true

SOLL_N=$(wc -l < "$TMP/soll.txt" | tr -d ' ')
if [ "$SOLL_N" -eq 0 ]; then
    echo "ABBRUCH: der SOLL ist leer -- 'git ls-files' fand keine Test-Quelldatei." >&2
    echo "         Ein leerer Nenner macht jede Aussage wahr. Fail-closed." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# Abgleich: liegt der repo-relative Pfad, mit '/' davor verankert, im IST?
# Die Verankerung ist noetig und wurde am Objekt geprueft: ohne sie wuerde
# '/test_value_handle.cpp' faelschlich in '/test_value_handle_real.cpp'
# treffen (gemessen 09.08.2026: 0 vs. 10 Treffer -- die Verankerung trennt).
# ---------------------------------------------------------------------------
: > "$TMP/fehlend.txt"
while IFS= read -r f; do
    [ -n "$f" ] || continue
    if ! "$GREP" -q -F -- "/$f" "$IST_DATEI"; then
        printf '%s\n' "$f" >> "$TMP/fehlend.txt"
    fi
done < "$TMP/soll.txt"

FEHLEND_N=$(wc -l < "$TMP/fehlend.txt" | tr -d ' ')

# ---------------------------------------------------------------------------
# DIE ISA-GEGENPROBE. Setzt ISA_ANTWORT auf 'ja' oder 'nein'; kann sie das nicht
# verantworten, bricht sie mit Exit 2 ab.
#
# SIE WIRD BEWUSST NICHT IN EINER KOMMANDO-SUBSTITUTION AUFGERUFEN. Ein 'exit 2'
# in $( ) beendet nur die Subshell -- die Wache liefe mit leerer Antwort weiter und
# waere genau an der Stelle fail-OPEN, an der sie fail-closed sein muss. Deshalb
# gibt sie ihr Ergebnis ueber eine Variable zurueck und nicht ueber stdout.
# ---------------------------------------------------------------------------
ISA_QUELLE="$BUILD_ABS/CMakeCache.txt"
ISA_ANTWORT=""
ISA_AVX2=""     # leer = noch nicht ermittelt (Merkung, damit der Cache je Lauf
ISA_AVX512F=""  #        einmal statt je Zeile gelesen wird)
ISA_GEFRAGT=nein

isa_abbruch() {
    echo "ABBRUCH: $1" >&2
    echo "         Die ISA-Frage ist damit UNBEANTWORTBAR. Fail-closed: das ist Exit 2," >&2
    echo "         nicht 'Merkmal fehlt' und damit eine stillschweigend gehaltene Ausnahme." >&2
    exit 2
}

isa_cache_lesen() {
    # $1 = CMake-Variablenname (COMDARE_HOST_RUNS_...)
    _iv="$1"
    if [ ! -f "$ISA_QUELLE" ]; then
        isa_abbruch "eine Allowlist-Zeile begruendet mit 'isa:', aber '$ISA_QUELLE' fehlt."
    fi
    _in=$(sed -n "/^${_iv}:/p" "$ISA_QUELLE" | wc -l | tr -d ' ')
    if [ "$_in" -eq 0 ]; then
        _im="'${_iv}' steht nicht in '$ISA_QUELLE' -- dieser Baum wurde"
        isa_abbruch "$_im ohne die ISA-Probe konfiguriert (Cross-Build?)."
    fi
    # GENAU EINMAL: CMake legt jeden Eintrag einmal an und nimmt beim Lesen die LETZTE
    # Zeile; diese Wache liest die erste. Zwei Zeilen heisst von Hand bearbeitet.
    if [ "$_in" -gt 1 ]; then
        _im="'${_iv}' steht ${_in}-mal in '$ISA_QUELLE' -- ein von Hand"
        isa_abbruch "$_im bearbeiteter Cache ist kein Messergebnis."
    fi
    _iz=$(sed -n "/^${_iv}:/p" "$ISA_QUELLE" | sed -n '1p')

    # MERKMAL (1) TYP: 'NAME:TYP=WERT'. Nur INTERNAL stammt aus check_cxx_source_runs;
    # jedes '-D' von aussen schreibt einen anderen Typ (UNINITIALIZED, BOOL, ...).
    _it=${_iz#*:}
    _it=${_it%%=*}
    if [ "$_it" != "INTERNAL" ]; then
        _im="'${_iv}' hat in '$ISA_QUELLE' den Typ '${_it}', nicht INTERNAL --"
        isa_abbruch "$_im das hat ein '-D' geschrieben, keine Probe."
    fi

    # MERKMAL (2) _COMPILED: fehlt sie, lief die Probe nie. Ist sie nicht TRUE, ist ein
    # leerer Wert zweideutig -- 'diese CPU kann es nicht' und 'die Probe uebersetzte
    # nicht' saehen identisch aus.
    _ic_n=$(sed -n "/^${_iv}_COMPILED:/p" "$ISA_QUELLE" | wc -l | tr -d ' ')
    if [ "$_ic_n" -eq 0 ]; then
        isa_abbruch "'${_iv}' steht in '$ISA_QUELLE', aber '${_iv}_COMPILED' fehlt -- die Probe wurde nie gefahren."
    fi
    _ic=$(sed -n "s/^${_iv}_COMPILED:[^=]*=//p" "$ISA_QUELLE" | sed -n '1p')
    if [ "$_ic" != "TRUE" ]; then
        isa_abbruch "'${_iv}_COMPILED' ist '${_ic}', nicht TRUE -- die ISA-Probe hat nicht einmal uebersetzt."
    fi

    # DER WERT. check_cxx_source_runs schreibt 1 oder LEER; eine 0 hat nie eine Probe
    # geschrieben, auch wenn der Typ INTERNAL lautet.
    _iw=$(sed -n "s/^${_iv}:[^=]*=//p" "$ISA_QUELLE" | sed -n '1p')
    case "$_iw" in
        1)  ISA_ANTWORT=ja ;;
        '') ISA_ANTWORT=nein ;;
        *)  _im="'${_iv}' hat den Wert '${_iw}'; erwartet ist 1 oder leer."
            isa_abbruch "$_im Ein unverstandener Wert wird nicht zu 'nein' gerundet." ;;
    esac

    # MERKMAL (3) WERT GEGEN _EXITCODE -- der zweite, unabhaengige Beleg. Er steht in
    # einer Zeile, die das Erzwingen nicht mitschreibt, und deckt deshalb auch eine
    # Faelschung ab, die den Typ INTERNAL korrekt trifft.
    _ie_n=$(sed -n "/^${_iv}_EXITCODE:/p" "$ISA_QUELLE" | wc -l | tr -d ' ')
    if [ "$_ie_n" -eq 0 ]; then
        isa_abbruch "'${_iv}_COMPILED' ist TRUE, aber '${_iv}_EXITCODE' fehlt -- try_run legt beide gemeinsam an."
    fi
    _ie=$(sed -n "s/^${_iv}_EXITCODE:[^=]*=//p" "$ISA_QUELLE" | sed -n '1p')
    if [ "$ISA_ANTWORT" = ja ] && [ "$_ie" != "0" ]; then
        isa_abbruch "'${_iv}' ist 1, aber '${_iv}_EXITCODE' ist '${_ie}' -- Wert und Beleg widersprechen sich."
    fi
    if [ "$ISA_ANTWORT" = nein ] && [ "$_ie" = "0" ]; then
        isa_abbruch "'${_iv}' ist leer, aber '${_iv}_EXITCODE' ist 0: die Probe lief und war ERFOLGREICH."
    fi
}

# Die Liste der bekannten Merkmale ist ABSCHLIESSEND. Genau diese zwei gattern in
# diesem Repo (tests/unit/CMakeLists.txt:4165/5330/5343/5372/5375); jedes andere Wort
# in einem 'isa:' ist ein Tippfehler oder eine Erfindung -- und wird nicht geraten.
isa_merkmal() {
    ISA_GEFRAGT=ja
    case "$1" in
        avx2)
            [ -n "$ISA_AVX2" ] || { isa_cache_lesen COMDARE_HOST_RUNS_AVX2; ISA_AVX2="$ISA_ANTWORT"; }
            ISA_ANTWORT="$ISA_AVX2"
            ;;
        avx512f)
            [ -n "$ISA_AVX512F" ] || { isa_cache_lesen COMDARE_HOST_RUNS_AVX512F; ISA_AVX512F="$ISA_ANTWORT"; }
            ISA_ANTWORT="$ISA_AVX512F"
            ;;
        *)  ISA_ANTWORT="" ;;   # unbekannt -- der Aufrufer macht daraus ROT, nicht gruen
    esac
}

# ---------------------------------------------------------------------------
# DIE ERREICHBARKEITS-PROBE (PA-1, zweite ERLOSCHEN-Richtung).
# Setzt ERR_ANTWORT auf
#   nein      -- dieses Repo erklaert den Gegenstand (Index, Gitlink oder .gitignore).
#                Er kann entstehen; die Ausnahme darf getragen werden.
#   ja        -- KEINE Quelle dieses Repos kennt ihn. TOTE AUSNAHME.
#   unbekannt -- der Pfad liegt ausserhalb des Repos; so nicht beurteilbar.
# Was das NICHT beweist, steht im Kopf. Hier nur die Mechanik.
# ---------------------------------------------------------------------------
# JEDE PRUEFUNG STEHT IN EINEM 'if'. Ein blankes '[ ... ] && return 0' waere unter
# 'set -e' eine Falle: schlaegt der Test fehl, ist der Status der AND-Liste ungleich 0
# und die ganze Wache braeche wortlos ab -- mit Exit 1, also als "Befund".
#
# ':(literal)' schaltet die Pathspec-Magie ab: ein '*' im Gegenstand darf hier nicht
# als Glob wirken.
ist_ignoriert() {
    # Verzeichnismuster in .gitignore ('build/') greifen NUR mit Schraegstrich -- am
    # Objekt gemessen 2026-08-10: 'check-ignore build' = nicht ignoriert,
    # 'check-ignore build/' = ignoriert. Beide Formen fragen.
    if git check-ignore -q -- "$1" 2>/dev/null; then return 0; fi
    if git check-ignore -q -- "$1/" 2>/dev/null; then return 0; fi
    return 1
}

ist_gitlink() {
    # Modus 160000 = 'commit', also ein Submodul-Eintrag. Der Inhalt eines nicht
    # ausgecheckten Submoduls steht NICHT im Index -- der Gitlink schon.
    _s=$(git ls-files -s -- ":(literal)$1" 2>/dev/null || true)
    case "$_s" in 160000\ *) return 0 ;; esac
    return 1
}

hat_getrackten_inhalt() {
    _t=$(git ls-files -- ":(literal)$1" 2>/dev/null || true)
    if [ -n "$_t" ]; then return 0; fi
    return 1
}

erreichbarkeit() {
    # $1 = repo-relativer Pfad aus Feld 2. Setzt ERR_ANTWORT.
    _p="$1"
    case "$_p" in
        /*) ERR_ANTWORT=unbekannt; return 0 ;;
    esac
    # '..' SEGMENTWEISE pruefen, nicht als Teilzeichenkette: 'libs/a..b/x' ist ein
    # ganz normaler Pfad im Repo. (Fallen-Register: '/build' frisst sonst '/builds'.)
    _rest="$_p"
    while [ -n "$_rest" ]; do
        case "$_rest" in
            */*) _seg=${_rest%%/*}; _rest=${_rest#*/} ;;
            *)   _seg="$_rest";     _rest="" ;;
        esac
        if [ "$_seg" = ".." ]; then ERR_ANTWORT=unbekannt; return 0; fi
    done
    # Der Gegenstand selbst: getrackt oder als Bauprodukt angemeldet?
    if hat_getrackten_inhalt "$_p"; then ERR_ANTWORT=nein; return 0; fi
    if ist_ignoriert "$_p"; then ERR_ANTWORT=nein; return 0; fi
    # Ohne Verzeichnisanteil ist der Elternteil die Repo-Wurzel -- immer bekannt.
    case "$_p" in
        */*) _a=${_p%/*} ;;
        *)   ERR_ANTWORT=nein; return 0 ;;
    esac
    # ---------------------------------------------------------------------
    # DIE AHNENREIHE HOCHLAUFEN, nicht nur den direkten Elternteil fragen.
    # Am Objekt gefunden (2026-08-10, Fall SubmodulGitlinkIstErreichbar): der
    # direkte Elternteil von 'ext/<submodul>/include/kopf.hpp' ist
    # 'ext/<submodul>/include' -- und der steht NICHT im Index, weil nur der
    # GITLINK 'ext/<submodul>' dort steht. Die erste Fassung erklaerte den Pfad
    # deshalb faelschlich fuer tot. Ein nicht ausgechecktes Submodul kann ihn
    # aber jederzeit bringen.
    # ---------------------------------------------------------------------
    _erster=ja
    while :; do
        # Ein ignoriertes Verzeichnis darf beliebig tief Erzeugtes aufnehmen.
        if ist_ignoriert "$_a"; then ERR_ANTWORT=nein; return 0; fi
        # Ein Gitlink darf beliebig tief Ausgechecktes aufnehmen.
        if ist_gitlink "$_a"; then ERR_ANTWORT=nein; return 0; fi
        if hat_getrackten_inhalt "$_a"; then
            if [ "$_erster" = ja ]; then
                # Eine NEUE Datei in einem Verzeichnis, das es schon gibt, ist der
                # Normalfall -- daraus darf nie 'tot' werden.
                ERR_ANTWORT=nein
            else
                # git zaehlt dieses Verzeichnis VOLLSTAENDIG auf, und der Zweig, den
                # der Gegenstand darunter braucht, ist nicht dabei. Kein Erzeuger.
                ERR_ANTWORT=ja
            fi
            return 0
        fi
        _erster=nein
        case "$_a" in
            */*) _a=${_a%/*} ;;
            *)   break ;;   # der naechste waere die Wurzel
        esac
    done
    # Bis zur Wurzel hoch kannte kein Verzeichnis den Zweig.
    ERR_ANTWORT=ja
    return 0
}

# ---------------------------------------------------------------------------
# HEUTE. Fuer 'frist:'. Der Wert ist ueberschreibbar, damit der Google Test beide
# Seiten des Datums fahren kann, ohne die Systemuhr zu stellen -- und er wird im
# NENNER ausgewiesen, samt Herkunft: eine Wache, deren Zeitbegriff man lautlos
# verschieben kann, waere selbst wieder ein Freibrief (V-1).
# ---------------------------------------------------------------------------
HEUTE_HERKUNFT="Systemuhr"
if [ -n "${COMDARE_WACHE_HEUTE:-}" ]; then
    HEUTE="$COMDARE_WACHE_HEUTE"
    HEUTE_HERKUNFT="COMDARE_WACHE_HEUTE (ueberschrieben)"
else
    HEUTE=$(date +%Y-%m-%d)
fi
case "$HEUTE" in
    [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]) ;;
    *)  echo "ABBRUCH: '$HEUTE' ist kein Datum JJJJ-MM-TT (Herkunft: $HEUTE_HERKUNFT)." >&2
        echo "         Ohne belastbares Heute ist keine 'frist:'-Zeile pruefbar. Fail-closed." >&2
        exit 2 ;;
esac

# ---------------------------------------------------------------------------
# Allowlist auswerten. Format je Zeile, drei Felder, '|'-getrennt:
#   <test-quelldatei> | <art>:<gegenstand-der-fehlen-muss> | <begruendung>
# Feld 2 ist die Gegenprobe der Begruendung. Drei Arten, s. Kopf:
#   datei:<pfad>              existiert der Pfad -> ERLOSCHEN
#                             kennt ihn keine Quelle des Repos -> TOTE AUSNAHME
#   isa:<merkmal>[+<merkmal>] sind ALLE Merkmale auf dem Bau-Host da -> ERLOSCHEN
#   frist:<JJJJ-MM-TT>        ist das Datum verstrichen -> ABGELAUFEN
# Alles andere (unbekannte Art, kein Praefix, leeres Feld, unbekanntes Merkmal) ist
# UNPRUEFBAR und damit ROT.
# ---------------------------------------------------------------------------
: > "$TMP/begruendet.txt"
: > "$TMP/unbegruendet.txt"
: > "$TMP/erloschen.txt"
: > "$TMP/unpruefbar.txt"
: > "$TMP/tot.txt"
NBEURT_N=0

allow_zeile() {
    # $1 = gesuchte Datei; gibt die Allowlist-Zeile aus oder nichts
    [ -f "$ALLOWLIST" ] || return 0
    while IFS= read -r z; do
        case "$z" in ''|'#'*) continue ;; esac
        _d=$(printf '%s' "$z" | cut -d'|' -f1 | sed 's/[[:space:]]*$//;s/^[[:space:]]*//')
        [ "$_d" = "$1" ] || continue
        printf '%s\n' "$z"
        return 0
    done < "$ALLOWLIST"
}

while IFS= read -r f; do
    [ -n "$f" ] || continue
    z=$(allow_zeile "$f")
    if [ -z "$z" ]; then
        printf '%s\n' "$f" >> "$TMP/unbegruendet.txt"
        continue
    fi
    geg=$(printf '%s' "$z" | cut -d'|' -f2 | sed 's/[[:space:]]*$//;s/^[[:space:]]*//')
    txt=$(printf '%s' "$z" | cut -d'|' -f3- | sed 's/^[[:space:]]*//')

    case "$geg" in
        *:*) art=${geg%%:*}; wert=${geg#*:} ;;
        *)   art=""; wert="$geg" ;;
    esac

    case "$art" in
    datei)
        if [ -z "$wert" ]; then
            printf '%s -- UNPRUEFBAR: "datei:" ohne Pfad\n' "$f" >> "$TMP/unpruefbar.txt"
        elif [ -e "$wert" ]; then
            printf '%s -- ERLOSCHEN: "%s" existiert wieder, die Ausnahme traegt nicht mehr\n' \
                "$f" "$wert" >> "$TMP/erloschen.txt"
        else
            # ZWEITE RICHTUNG (PA-1): abwesend genuegt nicht. Ein Gegenstand, den keine
            # Quelle dieses Repos kennt, kann nicht wiederkommen -- die Ausnahme koennte
            # dann per Konstruktion nie erloeschen und waere eine Freistellung ohne Ende.
            erreichbarkeit "$wert"
            if [ "$ERR_ANTWORT" = ja ]; then
                _tmsg="TOTE AUSNAHME: \"$wert\" existiert nicht, und keine Quelle dieses Repos"
                _tmsg="$_tmsg kennt den Zweig, in dem er liegt (weder Index noch Submodul-Gitlink"
                _tmsg="$_tmsg noch .gitignore, bis hinauf zur Wurzel) -- diese Zeile koennte nie erloeschen"
                printf '%s -- %s\n' "$f" "$_tmsg" >> "$TMP/tot.txt"
            elif [ "$ERR_ANTWORT" = unbekannt ]; then
                NBEURT_N=$((NBEURT_N + 1))
                printf '%s -- %s (datei abwesend: %s -- ausserhalb des Repos, Erreichbarkeit NICHT beurteilt)\n' \
                    "$f" "$txt" "$wert" >> "$TMP/begruendet.txt"
            else
                printf '%s -- %s (datei abwesend: %s)\n' "$f" "$txt" "$wert" >> "$TMP/begruendet.txt"
            fi
        fi
        ;;
    frist)
        # Eine Ausnahme OHNE erreichbaren Gegenstand. Sie ist ehrlich unbeweisbar und
        # deshalb an ein Datum gebunden statt an eine Pfad-Fiktion. Der Vergleich ist
        # ein reiner Zeichenkettenvergleich -- JJJJ-MM-TT sortiert lexikografisch wie
        # chronologisch, und genau darum ist das Format Pflicht.
        case "$wert" in
            [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]) ;;
            *)  printf '%s -- UNPRUEFBAR: "frist:%s" ist kein Datum JJJJ-MM-TT\n' \
                    "$f" "$wert" >> "$TMP/unpruefbar.txt"
                continue ;;
        esac
        if [ "$HEUTE" \> "$wert" ]; then
            printf '%s -- ABGELAUFEN: die Frist %s ist am %s verstrichen -- %s\n' \
                "$f" "$wert" "$HEUTE" "$txt" >> "$TMP/erloschen.txt"
        else
            printf '%s -- %s (Frist laeuft bis %s, heute %s)\n' \
                "$f" "$txt" "$wert" "$HEUTE" >> "$TMP/begruendet.txt"
        fi
        ;;
    isa)
        # ALLE Merkmale da -> das CMake-UND-Gatter waere WAHR gewesen und die Datei
        # haette uebersetzt werden muessen: ERLOSCHEN. Fehlt eines, traegt die Ausnahme.
        _rest="$wert"; _alle_da=ja; _unbek=""; _bericht=""
        while [ -n "$_rest" ]; do
            case "$_rest" in
                *+*) _m=${_rest%%+*}; _rest=${_rest#*+} ;;
                *)   _m="$_rest";     _rest="" ;;
            esac
            [ -n "$_m" ] || continue
            isa_merkmal "$_m"
            if [ -z "$ISA_ANTWORT" ]; then
                _unbek="$_unbek $_m"
            else
                _bericht="$_bericht ${_m}=${ISA_ANTWORT}"
                [ "$ISA_ANTWORT" = ja ] || _alle_da=nein
            fi
        done
        if [ -n "$_unbek" ]; then
            printf '%s -- UNPRUEFBAR: unbekannte(s) ISA-Merkmal(e):%s (bekannt: avx2, avx512f)\n' \
                "$f" "$_unbek" >> "$TMP/unpruefbar.txt"
        elif [ -z "$_bericht" ]; then
            printf '%s -- UNPRUEFBAR: "isa:" ohne Merkmal\n' "$f" >> "$TMP/unpruefbar.txt"
        elif [ "$_alle_da" = ja ]; then
            # GENAU EINE ZEILE je Eintrag -- die Zaehler unten sind 'wc -l'. Eine
            # zweizeilige Meldung machte aus EINEM Befund ZWEI; der NENNER waere falsch.
            # Am Objekt gemessen (2026-08-10): die erste Fassung meldete "2 mit
            # ERLOSCHENER Begruendung" fuer eine einzige Datei.
            _emsg="ERLOSCHEN: der Bau-Host hat ALLE genannten ISA-Merkmale ($_bericht ) --"
            _emsg="$_emsg das Gatter waere wahr gewesen, die Datei haette uebersetzt werden muessen"
            printf '%s -- %s\n' "$f" "$_emsg" >> "$TMP/erloschen.txt"
        else
            printf '%s -- %s (ISA am Bau-Host:%s )\n' "$f" "$txt" "$_bericht" >> "$TMP/begruendet.txt"
        fi
        ;;
    *)
        printf '%s -- UNPRUEFBAR: Feld 2 ist "%s" -- keine bekannte Art (erwartet "datei:" oder "isa:")\n' \
            "$f" "$geg" >> "$TMP/unpruefbar.txt"
        ;;
    esac
done < "$TMP/fehlend.txt"

BEGR_N=$(wc -l < "$TMP/begruendet.txt" | tr -d ' ')
UNBEGR_N=$(wc -l < "$TMP/unbegruendet.txt" | tr -d ' ')
ERL_N=$(wc -l < "$TMP/erloschen.txt" | tr -d ' ')
UNPR_N=$(wc -l < "$TMP/unpruefbar.txt" | tr -d ' ')
TOT_N=$(wc -l < "$TMP/tot.txt" | tr -d ' ')

echo "-----------------------------------------------------------------------------"
echo "TEST-REGISTRIERUNGS-WACHE (MT-L4) -- Quelldatei gegen Bauweg"
echo "  SOLL-Quelle: git ls-files (Git-Index)"
echo "  IST-Quelle : $IST_ART im Baum '$BUILD'"
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ]; then
    echo ""
    echo "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS -- diese Dateien werden nie uebersetzt:"
    sed 's/^/  /' "$TMP/unbegruendet.txt"
fi
if [ "$ERL_N" -gt 0 ]; then
    echo ""
    echo "AUSNAHME ERLOSCHEN -- die Begruendung gilt am Gegenstand nicht mehr:"
    sed 's/^/  /' "$TMP/erloschen.txt"
fi
if [ "$TOT_N" -gt 0 ]; then
    echo ""
    echo "TOTE AUSNAHME -- der Gegenstand kann in KEINEM erklaerten Baum entstehen:"
    sed 's/^/  /' "$TMP/tot.txt"
fi
if [ "$UNPR_N" -gt 0 ]; then
    echo ""
    echo "UNPRUEFBARE BEGRUENDUNG -- Feld 2 nennt nichts, was diese Wache nachpruefen kann:"
    sed 's/^/  /' "$TMP/unpruefbar.txt"
fi
if [ "$BEGR_N" -gt 0 ]; then
    echo ""
    echo "ABWESEND, ABER BEGRUENDET (Allowlist $ALLOWLIST):"
    sed 's/^/  /' "$TMP/begruendet.txt"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Zahl):"
echo "  $SOLL_N getrackte Test-Quelldatei(en) im Baum (ohne ext/) -- SOLL."
echo "  $FEHLEND_N davon NICHT im Bauweg des Baums '$BUILD'."
echo "  davon $BEGR_N begruendet, $ERL_N mit ERLOSCHENER, $TOT_N TOTE AUSNAHME," \
     "$UNPR_N mit UNPRUEFBARER Begruendung, $UNBEGR_N ohne."
echo "  Erreichbarkeit: $NBEURT_N der $BEGR_N begruendeten nennen einen Gegenstand AUSSERHALB"
echo "                  des Repos -- fuer die ist 'kann nie entstehen' NICHT beurteilt worden."
echo "  Heute (fuer 'frist:'): $HEUTE -- Herkunft: $HEUTE_HERKUNFT."
echo "  Gegenprobe des Messgeraets: '$GEGENPROBE' trifft in $IST_ART (das Muster sucht)."
if [ "$ISA_GEFRAGT" = ja ]; then
    echo "  ISA-Gegenprobe: $ISA_QUELLE"
    echo "                  avx2=${ISA_AVX2:-nicht gefragt}, avx512f=${ISA_AVX512F:-nicht gefragt}"
    echo "                  (vier Merkmale je Variable geprueft: einmalig, Typ INTERNAL,"
    echo "                   _COMPILED=TRUE, Wert deckt _EXITCODE)"
else
    echo "  ISA-Gegenprobe: nicht gefragt -- keine abwesende Datei ist mit 'isa:' begruendet."
fi
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ] || [ "$ERL_N" -gt 0 ] || [ "$UNPR_N" -gt 0 ] || [ "$TOT_N" -gt 0 ]; then
    echo "TEST-REGISTRIERUNGS-WACHE: ROT ($UNBEGR_N von $SOLL_N ohne Begruendung," \
         "$ERL_N erloschen, $TOT_N tot, $UNPR_N unpruefbar)."
    echo "Abhilfe: die Datei per comdare_add_test()/add_test() in den Bauweg nehmen -- ODER"
    echo "         sie mit einem nachpruefbaren Gegenstand in $ALLOWLIST begruenden"
    echo "         ('datei:<pfad>' oder 'isa:<merkmal>[+<merkmal>]', s. Kopf der Allowlist)."
    if [ "$TOT_N" -gt 0 ]; then
        echo "Bei einer TOTEN AUSNAHME hilft kein anderer Pfad: gibt es keinen erreichbaren"
        echo "         Gegenstand, gehoert die Zeile auf 'frist:<JJJJ-MM-TT>' -- eine geparkte"
        echo "         Ausnahme mit Ablaufdatum statt einer Adresse, die nie jemand belegt."
    fi
    exit 1
fi

echo "TEST-REGISTRIERUNGS-WACHE: OK ($SOLL_N Quelldateien, $UNBEGR_N ohne Begruendung ausserhalb)."
exit 0
