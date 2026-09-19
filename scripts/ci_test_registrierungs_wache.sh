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
# UNPRUEFBARE BEGRUENDUNG). Dasselbe gilt fuer ein leeres Feld 2, fuer ein unbekanntes und fuer
# ein leeres ISA-Teilmerkmal. Fail-closed heisst hier woertlich: eine Begruendung, die niemand nachpruefen
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
# ein ganzer ZWEIG fehlt, nicht den, dass ein einzelner Name erfunden ist. 'Vorhanden' heisst:
# ein VERZEICHNIS mit getracktem Inhalt -- eine getrackte Datei gleichen Namens ist keines
# (Folge (11), Fix-r5); (d) ein 'datei:'-Pfad mit einem Zeichen, das git in seiner Ausgabe
# quotiert (Tabulator, Steuerzeichen, Anfuehrungszeichen, Backslash), oder ohne kanonische Form
# (Segment '.', leeres Segment: './', '//', '/' am Ende) ist UNPRUEFBAR, nie erreichbar oder tot
# (Folge (12), Fix-r5).
#
# WARUM DIE VERZEICHNISSE UND NICHT NUR DER PFAD: ein nicht existierender Pfad ist per
# Definition weder getrackt noch (meist) ignoriert -- die Frage waere fuer JEDEN abwesenden
# Gegenstand mit 'ja' zu beantworten und die Regel unbrauchbar. Entschieden wird deshalb an
# der AHNENREIHE, vom direkten Elternteil aufwaerts, und der ERSTE bekannte Vorfahr gibt die
# Antwort:
#   ignoriert       -> erreichbar. 'build/tools/y' darf bei jedem Bau entstehen.
#   Gitlink (160000)-> erreichbar. Ein nicht ausgechecktes Submodul kann beliebig tief
#                      etwas mitbringen; sein Inhalt steht nie im Index des Obenprojekts.
#   getrackte DATEI (100644/100755) -> tot. Unter einer Datei kann nie ein Kind entstehen,
#                      weder im Index noch im Arbeitsbaum (Folge (11), Fix-r5).
#   Symlink (120000) oder Merge-Konflikt mit ungleichen Typen je Stufe -> UNPRUEFBAR (rot).
#                      Die Wache loest keinen Link auf und raet keinen Merge-Ausgang (Folge (11)).
#   getrackter Inhalt DARUNTER (ein Verzeichnis, Folge (11)), und es ist der DIREKTE
#                      Elternteil -> erreichbar. Eine neue Datei in einem vorhandenen
#                      Verzeichnis ist der Normalfall.
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
# DIE VIERTE KLASSE: ARCHIV -- ein ORT statt einer Ausnahme (Owner 2026-09-17, Order 206).
# Die drei Arten oben begruenden, warum eine Datei des SOLL im Bauweg FEHLEN darf. Der Archiv-Fall
# ist ein anderer: die vier prt-art-Waisen liegen seit dem 2026-08-15 (W-B) per 'git mv' unter
# tests/deprecated/prt_art_legacy_waisen/ mit einem VERMERK.md daneben -- sie sind nicht abwesend,
# sie sind ABGELEGT. Ihre 'frist:'-Zeilen waren deshalb die falsche Form: eine Frist erzwingt eine
# Entscheidung, und der Owner hat sie am 2026-09-17 getroffen ("OV-2 Archiv-Variante gilt und frist
# Zeilen entfernen"). Die vier Zeilen sind mit dieser Fassung aus der Allowlist entfernt.
#
# DIE REGEL, eng und nachpruefbar: eine getrackte Test-Quelldatei unter tests/deprecated/<ordner>/
# zaehlt NICHT zum SOLL, WENN UND NUR WENN tests/deprecated/<ordner>/VERMERK.md im Git-Index liegt UND
# die FORM eines Ankers hat: ein regulaeres Blob (Index-Modus 100644 oder 100755) auf Index-Stufe 0
# (kein Merge-Konflikt), lesbar, mit mindestens einem Nicht-Leerraum-Zeichen (Folge (1) unten). Die
# blosse Praesenz im Index genuegt seit den Lens-Funden r1/r2 (2026-09-18) NICHT mehr; dieser Absatz
# ist mit Fix-r2 (Lens C LCW-06) an die Mechanik unten angeglichen. Der VERMERK.md ist der ANKER; er
# traegt Begruendung, Messung und die aufgegebene Deckung. Fehlt er oder hat er die Form nicht, bleibt
# die Datei im SOLL und die Wache beisst wie bisher: OHNE BEGRUENDUNG. Eine Allowlist-Zeile faengt das
# NICHT auf -- fuer einen Pfad unter tests/deprecated/ ist jede Allowlist-Zeile UNPRUEFBAR (Folge (3)):
# der Archiv-Ort kennt nur den Anker.
#
# WARUM NICHT LAUTLOS: eine Klasse, die den Nenner verkleinert, ohne sich zu zeigen, waere genau der
# Freibrief, gegen den diese Wache gebaut ist. Die Archiv-Menge wird deshalb bei JEDEM Lauf mit Zahl,
# Ordner-Zahl und Dateinamen ausgewiesen und steht im NENNER und in der ENDZEILE -- auch im gruenen
# Lauf. Wer den Ordner leert oder den VERMERK.md entfernt, sieht die Zahl sinken, und die Dateien
# fallen in den SOLL zurueck.
#
# GRENZEN, ausdruecklich: <ordner> ist GENAU das dritte Pfadsegment -- ein VERMERK.md tiefer im Baum
# ankert nichts, und eine Datei direkt in tests/deprecated/ (ohne Ordner) hat keinen Anker und bleibt
# im SOLL. Ob eine archivierte Datei uebersetzt wird, prueft diese Wache nicht mehr: sie ist aus dem
# SOLL genommen. Das ist der Preis der Ablage -- und der Grund, warum der Anker eine Datei im INDEX
# ist und keine Zeile in einer Liste.
#
# FOLGEN DER VIERTEN KLASSE, alle fail-closed (Lens-Funde r1 und r2, 2026-09-18):
#   (1) DER ANKER MUSS INHALT UND FORM HABEN (Lens A LA-04/LA3-01, Lens B LB2-01, Lens C LCW-01). Ein
#       Index-Eintrag namens VERMERK.md, der kein regulaeres Blob ist (Symlink 120000, Gitlink 160000),
#       der im Merge-Konflikt steht (Index-Stufe 1-3 statt 0), dessen Blob nicht lesbar ist oder der
#       kein Nicht-Leerraum-Zeichen enthaelt (0 Byte, nur Zeilenumbrueche oder Leerraum), traegt keine
#       Begruendung. Er ankert NICHT (die Dateien bleiben im SOLL) und wird als UNPRUEFBARER ANKER
#       gemeldet -- ROT. Sonst waere der Anker die einzige Stelle dieser Wache, an der Leere gruen
#       traegt; ein leeres Feld 2 ist seit je UNPRUEFBAR.
#   (2) EINE ALLOWLIST-ZEILE FUER EINE ARCHIV-DATEI IST EINE AUSNAHME OHNE ANLASS (Lens B LB-01).
#       Die Allowlist wird nur fuer Dateien gelesen, die dem SOLL fehlen; eine archivierte Datei
#       steht nicht im SOLL, ihre Zeile wuerde also NIE ausgewertet (am Objekt gemessen, Lens B P10:
#       'frist:2000-01-01' fuer eine ARCHIV-Datei -> Exit 0, kein ABGELAUFEN) und laege in Reserve,
#       bis der Anker faellt. Dasselbe gilt fuer eine Zeile, deren Feld 1 gar keine Datei des
#       SOLL-Bestands nennt (getrackte Test-Quelldatei ausserhalb ext/: geloescht, umbenannt, unter
#       ext/, anders geschrieben; Lens A LA-08). Beide Klassen sind UNPRUEFBAR und damit ROT; die
#       Abhilfe ist die Loeschung der Zeile.
#   (3) EINE ALLOWLIST-ZEILE FUER EINEN PFAD UNTER tests/deprecated/ IST UNPRUEFBAR, MIT ODER OHNE
#       ANKER (Lens C LCW-05; Geist der Owner-Order 206 "frist Zeilen entfernen"). Ohne Anker stuende
#       die Datei im SOLL, und eine 'frist:'-Zeile truege sie regulaer als begruendet -- das Archiv
#       waere durch die Hintertuer wieder eine Frist. Der Archiv-Ort kennt deshalb nur den Anker:
#       allow_zeile() gibt fuer solche Pfade keine Zeile zurueck (die Datei bleibt OHNE BEGRUENDUNG),
#       und der Nachscan meldet die Zeile (Zaehler ORT_ZEILE_N).
#   (4) DOPPELTE ALLOWLIST-ZEILEN JE PFAD SIND UNPRUEFBAR (Lens C LCW-08): allow_zeile() nimmt die
#       erste Zeile; jede weitere schliefe und erwachte allein durch die Reihenfolge. Der Nachscan
#       meldet den Pfad einmal (Zaehler DOPPEL_ZEILE_N).
#   (5) EIN LEERES FELD 3 IST UNPRUEFBAR (Lens C LCW-09): der Drei-Feld-Vertrag verlangt die
#       Begruendung; eine Zeile mit zwei Feldern oder leerem dritten Feld liefe sonst als BEGRUENDET
#       durch. Geprueft an jeder ausgewerteten Zeile, in derselben Liste wie eine unbekannte Art.
#   (6) WERKZEUG-AUSFAELLE SIND EXIT 2 (Lens C LCW-02/LCW-03; LC3W-01..04, Fix-r3): POSIX-sh kennt
#       kein 'pipefail', in 'a | b' zaehlt nur der Status von b. Jede Nenner-Pipeline schreibt deshalb
#       Zwischendateien und prueft jedes Glied einzeln; jede Zahl wird als Zahl validiert (Helfer
#       zeilen_zaehlen). Seit Fix-r3 gilt das fuer JEDES Werkzeug: grep laeuft nur ueber grep_in_datei
#       (Status 0/1 = Antwort, ab 2 = Abbruch, auch bei -q), git-Fehler in der Erreichbarkeits-Probe
#       (check-ignore/ls-files, Status 128) und am Anker (cat-file) sind Abbruch statt Datenbefund,
#       'date' ist geprueft, und der ISA-Teil liest den CMakeCache ueber dieselben Helfer. cut, sed
#       und tr sind entfernt: Felder, Ordnernamen und die eingerueckte Ausgabe entstehen mit
#       Parametererweiterung der Shell -- kein Werkzeug, kein Ausfall.
#   (7) DER ANKER MUSS EIN BLOB SEIN (Lens C LC3W-06): der Index-Modus 100644/100755 verspricht ein
#       Blob, prueft es aber nicht -- 'git update-index --cacheinfo' legt jedes Objekt unter jedem
#       Modus ab. Geprueft wird 'git cat-file -t' == blob, davor 'cat-file -e' (Objekt vorhanden;
#       fehlt es, ist der Anker UNPRUEFBAR; scheitert git selbst, ist es Exit 2).
#   (8) DIE FORM GILT FUER JEDE DATENZEILE (Lens C LC3W-05): die Schleife ueber die dem Bauweg
#       fehlenden Dateien prueft Feld 2 und 3 nur an Zeilen, die sie auswertet. Eine Zeile fuer eine
#       Datei IM Bauweg ist stumm (Design, Lens A LA3-08: die 'isa:'-Zeile ist auf dem AVX-512-Host
#       genau so stumm) -- ein Formfehler darin (leeres Feld 3, unbekannte Art, leeres oder
#       unbekanntes Merkmal, kein Datum) laege in Reserve, bis die Datei einmal fehlt. Der Nachscan
#       prueft deshalb die Form ALLER Datenzeilen mit demselben Helfer (feld_form) und meldet sie
#       fuer Dateien im Bauweg als eigene Klasse (Zaehler FORM_ZEILE_N). Am Objekt 806629ca gemessen:
#       eine Zeile fuer die Gegenprobe-Datei mit leerem Feld 3 -> Exit 0.
#   (9) ISA-BELEGE SIND EINDEUTIG (Lens C LC3W-07/LC3W-08): ein leeres Teilmerkmal ('+avx2',
#       'avx2+', 'a++b') ist ein Formfehler und wird nicht uebersprungen; _COMPILED und _EXITCODE
#       muessen wie die Wertzeile GENAU EINMAL im Cache stehen, und ein leerer _EXITCODE ist bei
#       leerem Wert kein Beleg (try_run schreibt beide Zeilen gemeinsam) -- je Exit 2.
#   (10) EIN VERZEICHNIS IST KEIN GITLINK (Fund N-1 der Fix-r3-Berichtsfassung, Lead-Objektprobe
#       K205; Fix-r4): 'git ls-files -s <pfad>' liefert fuer ein VERZEICHNIS alle Eintraege
#       darunter, und ist_gitlink() prueft bis 63f8abd4 nur, ob die Ausgabe mit '160000 ' BEGINNT.
#       Beginnt der Index unter dem Verzeichnis mit einem Gitlink (am Objekt: ext/queuing mit
#       ext/queuing/Q01-concurrentqueue), galt das Verzeichnis selbst als Gitlink und jeder
#       Gegenstand in Tiefe >= 2 darunter als erreichbar statt TOT -- fail-open in der PA-1-
#       Richtung, die Wache blieb gruen. Vorbestand seit 806629ca; seit Fix-r3 die ERSTE Frage
#       der Erreichbarkeits-Probe. Seit Fix-r4 zaehlt eine ls-files-Zeile nur mit Modus 160000
#       UND Pfadfeld == Pfad (zeilenweise ueber eine Zwischendatei, Folge (6)). Fall (27).
#   (11) EINE DATEI IST KEIN VERZEICHNIS, EIN SYMLINK KEINES, EIN TYP-KONFLIKT KEIN GITLINK (Lens A r5
#       LA5-01/LA5-02/LA5-06, Lens B r4 LB4-01/LB4-03; Fix-r5): die Erreichbarkeits-Probe fragte je Ahnen
#       nur 'Gitlink?' und danach 'hat getrackten Inhalt?' -- und 'git ls-files <pfad>' listet fuer eine
#       getrackte DATEI die Datei selbst. Ein Gegenstand DIREKT unter einer Datei ('ext/README.md/x.hpp';
#       am Objekt Koeder K4 'ext/queuing/REPOS_OVERVIEW.md/x.hpp') galt so als 'neue Datei in einem
#       vorhandenen Verzeichnis' = erreichbar, obwohl unter einer Datei nie ein Kind entstehen kann --
#       fail-open in der PA-1-Richtung, Vorbestand seit 806629ca. Ein SYMLINK (120000) als Ahne lief in
#       'git check-ignore' ("beyond a symbolic link": 128 = Exit 2 mit der falschen Diagnose) oder, nur
#       im Index eingetragen, ebenso als erreichbar durch. Ein Pfad im MERGE-KONFLIKT mit ungleichen
#       Typen je Stufe (Datei gegen Gitlink) zaehlte als Gitlink, sobald EINE Stufe einer war. Seit
#       Fix-r5 liest index_eintrag() je Ahnen die exakten Index-Zeilen (Modus, Stufe, Pfadfeld) und
#       entscheidet: Gitlink auf jeder vorhandenen Stufe (auch nur Stufe 1-3, alle vom selben Typ) ->
#       erreichbar; Datei -> TOT; Symlink -> UNPRUEFBAR (die Wache loest keinen Link auf); ungleiche
#       Typen je Stufe -> UNPRUEFBAR (nicht aufgeloest, die Antwort haengt vom Ausgang ab -- wie der
#       Anker im Konflikt, Folge (1)). check-ignore wird fuer solche Pfade nie gefragt. Faelle (27d-f)
#       und (28).
#   (12) DIE SCHREIBWEISE DES GEGENSTANDS MUSS DER DES INDEX ENTSPRECHEN (Lens A r5 LA5-03, Rest des
#       LA-07-Postens; Fix-r5): der Pfadvergleich in index_eintrag() ist exakt, 'git ls-files' quotiert
#       aber Pfade mit Nicht-ASCII-Bytes (core.quotePath: "...\303\244..."), Tabulator, Steuerzeichen,
#       Anfuehrungszeichen oder Backslash, und ein Gegenstand mit './', '//' oder Segment '.' steht so
#       in keinem Index. Bis c62cfc7e traf keiner dieser Pfade seinen Gitlink-Ahnen und lief in
#       check-ignore (Exit 2 mit der falschen Diagnose 'is in submodule'); './' und '//' waren bis
#       63f8abd4 zufaellig gruen. Seit Fix-r5 liest die Probe den Index mit '-c core.quotePath=false'
#       (Nicht-ASCII-Bytes roh und damit vergleichbar), und feld_form weist einen 'datei:'-Pfad mit
#       quotierbarem Zeichen oder ohne kanonische Form als UNPRUEFBAR ab -- fail-closed mit der richtigen
#       Diagnose, auch fuer die stumme Zeile einer Datei im Bauweg (Folge (8)). Fall (29). Die SOLL-
#       und die Anker-Lesung (git ls-files ohne '-c') bleiben der LA-07-Posten: ein Nicht-ASCII-Pfad
#       dort ist weiterhin laut rot (OHNE BEGRUENDUNG bzw. kein Anker), nie still gruen.
#
# DER GEMESSENE BAUM MUSS DERSELBE SEIN WIE DER DER CI (J-0b, am Objekt 2026-09-17): der CI-Baum
# build-covguard wird MIT -DCOMDARE_CE_PRUEFLINGE=<repo>/tests/pruefling_fixture konfiguriert, und
# nur dann steht tests/pruefling_fixture/test_pruefling_fixture_ladung.cpp im Bauweg. Ein lokaler
# Baum ohne diesen Schalter meldet sie als OHNE BEGRUENDUNG -- richtig gemessen, falscher Baum.
# Dasselbe gilt fuer die DREI Tests, die erst NACH 'make inventar' (Werkzeuge bauen, RE-CONFIGURE)
# registriert werden -- am Objekt 2026-09-17 genau test_v41_anatomy_adhoc_autobuilt_load,
# test_v41_anatomy_f15_measurement und test_v41_anatomy_r5i_configure_codegen (Gatter in
# tests/unit/CMakeLists.txt: _r5i_status, _f15_status, _r5g_autobuilt_count). Wer diese Wache von
# Hand faehrt, faehrt sie gegen einen so gebauten Baum. [Berichtigt 2026-09-18, Lens A LA-01: "vier".]
#
# AUFRUF:  sh scripts/ci_test_registrierungs_wache.sh <build-verzeichnis>
# EXIT:    0 = jede getrackte Test-Quelldatei ist im Bauweg oder begruendet abwesend
#          1 = mindestens eine Test-Quelldatei OHNE gueltige Begruendung ausserhalb --
#              ohne Allowlist-Zeile, mit ERLOSCHENER, mit TOTER oder mit UNPRUEFBARER
#              Begruendung (dazu zaehlen ein ARCHIV-Anker ohne Inhalt/Form oder ohne Blob-Objekt,
#              eine Allowlist-Zeile fuer eine ARCHIV-Datei oder einen Pfad unter tests/deprecated/,
#              eine Zeile ohne Gegenstand im SOLL-Bestand, ein doppelt genannter Pfad, ein leeres
#              Feld 3 und ein Formfehler in der stummen Zeile einer Datei im Bauweg, s. oben; seit
#              Fix-r5 auch ein 'datei:'-Gegenstand, dessen Ahnenreihe durch einen Symlink oder einen
#              Typ-Konflikt im Index laeuft, und ein 'datei:'-Pfad mit quotierbarem Zeichen oder ohne
#              kanonische Form, Folgen (11) und (12))
#          2 = die Wache konnte nicht pruefen (fail-closed, ausdruecklich KEIN Gruen) -- auch bei
#              jedem Ausfall eines Werkzeugs (git, grep, sort, uniq, wc, date, mktemp; cut, sed und
#              tr kommen seit Fix-r3 nicht mehr vor; rm laeuft NUR im EXIT-trap zum Aufraeumen der
#              Zwischendateien, also nach dem 'exit' -- sein Ausfall aendert keinen Exit-Status, Lens A
#              r5 LA5-08) und bei jedem ISA-Beleg, der nicht eindeutig ist
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
    printf '%s\n' "ABBRUCH: '$BUILD' ist kein Verzeichnis -- die Wache konnte nicht pruefen." >&2
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
# WERKZEUG-AUSFAELLE SIND EXIT 2 (Lens C LCW-02/LCW-03, 2026-09-18; LC3W-01..04, Fix-r3).
# POSIX-sh kennt kein 'pipefail': in 'a | b' zaehlt nur der Status von b. Stirbt a nach
# einer Teilausgabe, liefert die Pipeline einen unvollstaendigen Nenner mit Status 0 --
# fail-OPEN an der Stelle, die den Nenner erhebt. Am Objekt gemessen (Fix-r2, Koeder
# 'git ls-files -s' mit Teilausgabe + Exit 1 bzw. 'wc' mit Exit 1): die Fassung
# d8e8f53d meldete "OK ( Quelldateien, ...)" mit Exit 0. Deshalb schreibt jedes Glied in
# eine Zwischendatei und wird einzeln geprueft, und jede Zahl wird als Zahl validiert,
# bevor sie in den Nenner geht. Die Helfer geben ueber VARIABLEN zurueck, nicht ueber
# stdout: ein 'exit 2' in $( ) beendet nur die Subshell (s. ISA-Gegenprobe unten).
# SEIT FIX-R3 geht JEDER grep-Aufruf durch grep_in_datei -- auch die mit -q: ein
# 'if ! grep -q' nahm einen Status 2 (unlesbare Datei) als Nichttreffer (Lens C LC3W-02).
# cut, sed und tr kommen nicht mehr vor: Felder, Ordnernamen und die eingerueckte Ausgabe
# entstehen mit Parametererweiterung der Shell (trim, felder, eingerueckt) -- kein
# Werkzeug, kein Ausfall (Lens C LC3W-01/LC3W-04).
# PRINTF STATT ECHO, WO NUTZERTEXT IN DER ZEILE STEHT (Lens A r5 LA5-07, Fix-r5): 'echo' von
# dash und busybox deutet Backslash-Folgen im Text ('\b' wird zum Backspace, '\t' zum
# Tabulator) -- eine Diagnose, die einen Pfad oder ein Bauverzeichnis nennt, kaeme verstuemmelt
# an (am Objekt: 'ext/a\b' erschien als 'ext/a<BS>'). printf '%s\n' gibt den Text unveraendert.
# ---------------------------------------------------------------------------
werkzeug_abbruch() {
    # $1 = was scheiterte, $2 = Exit-Status des Werkzeugs
    printf '%s\n' "ABBRUCH: Werkzeug-Ausfall -- $1 (Exit ${2:-?})." >&2
    printf '%s\n' "         Der Nenner ist damit nicht erhebbar. Fail-closed: Exit 2, ausdruecklich KEIN Gruen." >&2
    exit 2
}

zeilen_zaehlen() {
    # $1 = Datei. Ergebnis in ZAHL (nie ueber stdout). 'wc -l DATEI' statt '< DATEI': ein Ausfall
    # nennt so die Datei, und die Zahl ist das erste Wort der Ausgabe -- abgetrennt per
    # Parametererweiterung, ohne tr (Lens C LC3W-01: ein tr-Ausfall endete mit dessen Rohstatus).
    _zd="$1"
    _zz=$(wc -l "$_zd") || werkzeug_abbruch "'wc -l' ueber $_zd" "$?"
    _zw="$_zz"
    while :; do case "$_zw" in [[:space:]]*) _zw=${_zw#?} ;; *) break ;; esac; done
    ZAHL=${_zw%%[[:space:]]*}
    case "$ZAHL" in
        ''|*[!0-9]*) werkzeug_abbruch "'wc -l' ueber $_zd lieferte '$_zz', keine Zahl" 1 ;;
    esac
}

grep_in_datei() {
    # $1 = Zieldatei (/dev/null, wenn nur der Status zaehlt), $2 = Etikett, danach die grep-Argumente.
    # grep: 0 = Treffer, 1 = keine Treffer (kein Fehler), ab 2 = Werkzeug-Ausfall -> Exit 2. Der
    # Status 0/1 steht danach in GREP_RC. LC_ALL=C: jedes Muster dieser Wache ist ASCII, jeder Pfad
    # aus 'git ls-files' auch (core.quotePath), und die Leerraum-Klasse des Ankers (Folge (1)) meint
    # ASCII-Leerraum -- so haengt kein Vergleich an der Locale des Aufrufers.
    _gz="$1"; _ge="$2"; shift 2
    GREP_RC=0
    LC_ALL=C "$GREP" "$@" > "$_gz" || GREP_RC=$?
    [ "$GREP_RC" -le 1 ] || werkzeug_abbruch "'grep' $_ge" "$GREP_RC"
    # Eine gescheiterte Umleitung saehe wie Status 1 aus, ohne dass grep lief: die Zieldatei muss da sein.
    if [ "$_gz" != /dev/null ] && [ ! -f "$_gz" ]; then
        werkzeug_abbruch "Zwischendatei $_gz nicht anlegbar ('grep' $_ge)" 1
    fi
}

trim() {
    # $1 -> TRIM: fuehrender und schliessender Leerraum entfernt (Parametererweiterung, kein sed).
    TRIM="$1"
    while :; do case "$TRIM" in [[:space:]]*) TRIM=${TRIM#?} ;; *) break ;; esac; done
    while :; do case "$TRIM" in *[[:space:]]) TRIM=${TRIM%?} ;; *) break ;; esac; done
}

felder() {
    # $1 = Allowlist-Zeile -> FELD1 und FELD2 (getrimmt), FELD3 (Rest ab dem dritten Feld, links
    # getrimmt). Dieselbe Zerlegung wie 'cut -d"|" -f1 / -f2 / -f3-' der Fassung 806629ca: ohne
    # Trenner ist die ganze Zeile Feld 1, ein fehlendes Feld ist leer.
    _fz="$1"
    trim "${_fz%%|*}"; FELD1=$TRIM
    FELD2=""; FELD3=""
    _fr=${_fz#*|}
    if [ "$_fr" != "$_fz" ]; then
        trim "${_fr%%|*}"; FELD2=$TRIM
        _fr2=${_fr#*|}
        if [ "$_fr2" != "$_fr" ]; then
            FELD3="$_fr2"
            while :; do case "$FELD3" in [[:space:]]*) FELD3=${FELD3#?} ;; *) break ;; esac; done
        fi
    fi
}

eingerueckt() {
    # Jede Zeile der genannten Dateien um zwei Leerzeichen eingerueckt ausgeben -- ohne sed, dessen
    # Ausfall die Ausgabe still kuerzte (Lens C LC3W-04).
    for _ed in "$@"; do
        while IFS= read -r _el || [ -n "$_el" ]; do
            printf '  %s\n' "$_el"
        done < "$_ed"
    done
}

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
    printf '%s\n' "ABBRUCH: weder compile_commands.json noch build.ninja unter '$BUILD'." >&2
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
grep_in_datei /dev/null "Gegenprobe '$GEGENPROBE' in $IST_ART" -q -F -- "/$GEGENPROBE" "$IST_DATEI"
if [ "$GREP_RC" -ne 0 ]; then
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

# Glied fuer Glied mit Status (Kopf, Folge (6)); 'sort -u', weil 'git ls-files' im
# MERGE-KONFLIKT eine Datei je Index-Stufe listet (Lens A LA3-06: "4 getrackte" fuer drei).
git ls-files > "$TMP/index.txt" 2>/dev/null || werkzeug_abbruch "'git ls-files' (SOLL)" "$?"
grep_in_datei "$TMP/soll_1.txt" "-v '^ext/' (SOLL)" -v '^ext/' "$TMP/index.txt"
grep_in_datei "$TMP/soll_2.txt" "-v '/ext/' (SOLL)" -v '/ext/' "$TMP/soll_1.txt"
grep_in_datei "$TMP/soll_3.txt" "-E 'test_*.cpp' (SOLL)" -E '(^|/)test_[^/]*\.cpp$' "$TMP/soll_2.txt"
sort -u "$TMP/soll_3.txt" > "$TMP/soll_roh.txt" || werkzeug_abbruch "'sort -u' (SOLL)" "$?"

zeilen_zaehlen "$TMP/soll_roh.txt"; SOLL_ROH_N=$ZAHL
if [ "$SOLL_ROH_N" -eq 0 ]; then
    echo "ABBRUCH: der SOLL ist leer -- 'git ls-files' fand keine Test-Quelldatei." >&2
    echo "         Ein leerer Nenner macht jede Aussage wahr. Fail-closed." >&2
    exit 2
fi

# ---------------------------------------------------------------------------
# ARCHIV-ABZUG (Owner 2026-09-17, Order 206). Die ANKER zuerst, aus derselben
# Quelle wie der SOLL: 'tests/deprecated/<ordner>/VERMERK.md' im Git-INDEX. Ein
# VERMERK.md, das nur im Arbeitsbaum liegt, ankert NICHTS -- sonst haenge die
# Grundgesamtheit an einer ungetrackten Datei, die niemand sieht.
#
# DIE FORM DES ANKERS WIRD GEPRUEFT (Lens A LA-04, 2026-09-18): 'git ls-files -s'
# liefert 'MODUS SHA STUFE<TAB>PFAD'. Nur ein regulaeres Blob (100644/100755) auf
# Index-Stufe 0, dessen Objekt in der Objektdatenbank liegt und ein Blob IST, mit
# mindestens einem Nicht-Leerraum-Zeichen ankert; ein Symlink (120000), ein Gitlink
# (160000), ein Eintrag im Merge-Konflikt (Stufe 1-3, Lens C LCW-01), ein Eintrag ohne
# Objekt, ein Tree oder Commit unter 100644 (Lens C LC3W-06, Folge (7)) und ein Blob aus
# Leerraum (0 Byte oder nur Zeilenumbrueche, Lens A LA3-01 / Lens B LB2-01) tragen keine
# Begruendung. Ein solcher Eintrag ankert NICHT und wird als UNPRUEFBARER ANKER in
# derselben Liste und Zaehlung gefuehrt wie eine unpruefbare Allowlist-Zeile -- denn er
# ist eine (Kopf, Folge (1)). Am Objekt gemessen (Lens A, Proben R, L, R3, Z1; Fix-r3
# Tree-Probe): 0-Byte-, Symlink-, Newline-, Konflikt- und Tree-VERMERK.md ankerten bis
# zur jeweiligen Fassung wie ein echter Vermerk. Scheitert git selbst (Status 128 statt
# einer Antwort), ist das Exit 2 (Lens C LC3W-03), kein Befund.
# Die Pipeline schreibt Zwischendateien und prueft jedes Glied (Folge (6)): mit
# 'git ls-files -s | grep' saehe die Wache nur den Status von grep.
# ---------------------------------------------------------------------------
_TAB=$(printf '\t')
git ls-files -s > "$TMP/index_s.txt" 2>/dev/null ||
    werkzeug_abbruch "'git ls-files -s' (Index fuer die ARCHIV-Anker)" "$?"
grep_in_datei "$TMP/archiv_anker_roh.txt" "-E ueber den Index (ARCHIV-Anker)" \
    -E "^[0-9]+ [0-9a-f]+ [0-9]+${_TAB}tests/deprecated/[^/]+/VERMERK\.md$" "$TMP/index_s.txt"

: > "$TMP/archiv_anker.txt"
: > "$TMP/anker_unpruefbar.txt"
: > "$TMP/anker_konflikt.txt"
while IFS= read -r z || [ -n "$z" ]; do
    [ -n "$z" ] || continue
    _am=${z%% *}
    _as=${z#* }; _as=${_as%% *}
    _ast=${z#* }; _ast=${_ast#* }; _ast=${_ast%%"$_TAB"*}
    _ap=${z#*"$_TAB"}
    if [ "$_ast" != 0 ]; then
        # MERGE-KONFLIKT (Lens C LCW-01): Stufe 1/2/3 sind Basis, ours und theirs OHNE
        # aufgeloeste Fassung; 'git ls-files -s' listet den Pfad dann bis zu dreimal.
        # Einmal melden, nie ankern.
        grep_in_datei /dev/null "-F -x ueber die Konflikt-Anker" -q -F -x -- "$_ap" "$TMP/anker_konflikt.txt"
        if [ "$GREP_RC" -ne 0 ]; then
            printf '%s\n' "$_ap" >> "$TMP/anker_konflikt.txt"
            _msg="UNPRUEFBARER ANKER: Index-Stufe $_ast statt 0 (Merge-Konflikt, keine aufgeloeste"
            _msg="$_msg Fassung) -- traegt keine Begruendung, ankert nichts"
            printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
        fi
        continue
    fi
    case "$_am" in
        100644|100755) ;;
        *)  _msg="UNPRUEFBARER ANKER: Index-Modus $_am ist kein regulaeres Blob (Symlink 120000 oder"
            _msg="$_msg Gitlink 160000) -- traegt keine Begruendung, ankert nichts"
            printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
            continue ;;
    esac
    # DAS OBJEKT HINTER DEM EINTRAG (Lens C LC3W-03 / LC3W-06, Fix-r3). Erst 'cat-file -e': 0 = das
    # Objekt ist da, 1 = es fehlt in der Objektdatenbank (ein Index-Eintrag ohne Objekt, etwa per
    # 'update-index --cacheinfo' -- UNPRUEFBAR, ein Datenbefund), ab 2 = git selbst scheitert, Exit 2.
    # Danach sind -t, -s und -p reine Werkzeugfragen: jeder Fehler dort ist Exit 2, kein Befund.
    _ae=0
    git cat-file -e "$_as" 2>/dev/null || _ae=$?
    if [ "$_ae" -ge 2 ]; then werkzeug_abbruch "'git cat-file -e' fuer den Anker $_ap ($_as)" "$_ae"; fi
    if [ "$_ae" -eq 1 ]; then
        _msg="UNPRUEFBARER ANKER: Blob $_as fehlt in der Objektdatenbank (git cat-file -e) -- ankert nichts"
        printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
        continue
    fi
    # DER OBJEKTTYP (Kopf, Folge (7)): der Modus 100644/100755 verspricht ein Blob, 'update-index
    # --cacheinfo' legt aber jedes Objekt unter jedem Modus ab -- ein Tree ankerte bis 806629ca, denn
    # sein 'cat-file -p' hat Nicht-Leerraum-Zeichen.
    _at=$(git cat-file -t "$_as" 2>/dev/null) || werkzeug_abbruch "'git cat-file -t' fuer den Anker $_ap" "$?"
    if [ "$_at" != blob ]; then
        _msg="UNPRUEFBARER ANKER: Objekttyp $_at ist kein Blob (Index-Modus $_am verspricht eines) --"
        _msg="$_msg traegt keine Begruendung, ankert nichts"
        printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
        continue
    fi
    _ag=$(git cat-file -s "$_as" 2>/dev/null) || werkzeug_abbruch "'git cat-file -s' fuer den Anker $_ap" "$?"
    case "$_ag" in
        ''|*[!0-9]*) werkzeug_abbruch "'git cat-file -s' fuer den Anker $_ap lieferte '$_ag', keine Zahl" 1 ;;
    esac
    if [ "$_ag" -eq 0 ]; then
        _msg="UNPRUEFBARER ANKER: Blob mit 0 Byte -- eine leere Begruendung traegt nichts, ankert nichts"
        printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
        continue
    fi
    # INHALT, nicht nur GROESSE (Lens A LA3-01 / Lens B LB2-01): ein Blob aus Zeilenumbruechen
    # und Leerraum hat Bytes und sagt nichts. Erst in eine Datei, dann pruefen -- 'cat-file |
    # grep' saehe nur den Status von grep (Folge (6)); grep_in_datei setzt LC_ALL=C: ASCII-Leerraum.
    git cat-file -p "$_as" > "$TMP/anker_blob.txt" 2>/dev/null ||
        werkzeug_abbruch "'git cat-file -p' fuer den Anker $_ap" "$?"
    grep_in_datei /dev/null "'[^[:space:]]' ueber das Anker-Blob $_ap" -q '[^[:space:]]' "$TMP/anker_blob.txt"
    if [ "$GREP_RC" -ne 0 ]; then
        _msg="UNPRUEFBARER ANKER: Blob mit $_ag Byte, aber ohne Nicht-Leerraum-Zeichen (nur Zeilenumbrueche"
        _msg="$_msg oder Leerraum) -- eine leere Begruendung traegt nichts, ankert nichts"
        printf '%s -- %s\n' "$_ap" "$_msg" >> "$TMP/anker_unpruefbar.txt"
        continue
    fi
    printf '%s\n' "$_ap" >> "$TMP/archiv_anker.txt"
done < "$TMP/archiv_anker_roh.txt"
sort -u -o "$TMP/archiv_anker.txt" "$TMP/archiv_anker.txt" ||
    werkzeug_abbruch "'sort -u' ueber die ARCHIV-Anker" "$?"
zeilen_zaehlen "$TMP/anker_unpruefbar.txt"; ANKER_UNPR_N=$ZAHL

: > "$TMP/archiv.txt"
: > "$TMP/soll.txt"
while IFS= read -r f; do
    [ -n "$f" ] || continue
    _arch=nein
    case "$f" in
        tests/deprecated/*/*)
            _ar=${f#tests/deprecated/}
            case "$_ar" in
                */*) _aord=${_ar%%/*} ;;
                *)   _aord="" ;;
            esac
            if [ -n "$_aord" ]; then
                grep_in_datei /dev/null "-F -x ueber die ARCHIV-Anker (Zuordnung)" \
                    -q -F -x -- "tests/deprecated/$_aord/VERMERK.md" "$TMP/archiv_anker.txt"
                if [ "$GREP_RC" -eq 0 ]; then _arch=ja; fi
            fi
            ;;
    esac
    if [ "$_arch" = ja ]; then
        printf '%s\n' "$f" >> "$TMP/archiv.txt"
    else
        printf '%s\n' "$f" >> "$TMP/soll.txt"
    fi
done < "$TMP/soll_roh.txt"

zeilen_zaehlen "$TMP/archiv.txt"; ARCHIV_N=$ZAHL
ARCHIV_ORD_N=0
if [ "$ARCHIV_N" -gt 0 ]; then
    # Der <ordner> ist das dritte Pfadsegment -- per Parametererweiterung statt sed (Folge (6)).
    : > "$TMP/archiv_ordner_roh.txt"
    while IFS= read -r _af || [ -n "$_af" ]; do
        _ao=${_af#tests/deprecated/}
        printf '%s\n' "${_ao%%/*}" >> "$TMP/archiv_ordner_roh.txt"
    done < "$TMP/archiv.txt"
    sort -u "$TMP/archiv_ordner_roh.txt" > "$TMP/archiv_ordner.txt" ||
        werkzeug_abbruch "'sort -u' ueber die ARCHIV-Ordner" "$?"
    zeilen_zaehlen "$TMP/archiv_ordner.txt"; ARCHIV_ORD_N=$ZAHL
fi

zeilen_zaehlen "$TMP/soll.txt"; SOLL_N=$ZAHL
if [ "$SOLL_N" -eq 0 ]; then
    echo "ABBRUCH: der SOLL ist nach dem Archiv-Abzug leer ($ARCHIV_N von $SOLL_ROH_N)." >&2
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
    grep_in_datei /dev/null "-F '/$f' in $IST_ART (Abgleich)" -q -F -- "/$f" "$IST_DATEI"
    if [ "$GREP_RC" -ne 0 ]; then
        printf '%s\n' "$f" >> "$TMP/fehlend.txt"
    fi
done < "$TMP/soll.txt"

zeilen_zaehlen "$TMP/fehlend.txt"; FEHLEND_N=$ZAHL

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
    # printf statt echo: $1 nennt den CMakeCache-Pfad des Bauverzeichnisses (Lens A r5 LA5-07).
    printf '%s\n' "ABBRUCH: $1" >&2
    printf '%s\n' "         Die ISA-Frage ist damit UNBEANTWORTBAR. Fail-closed: das ist Exit 2," >&2
    printf '%s\n' "         nicht 'Merkmal fehlt' und damit eine stillschweigend gehaltene Ausnahme." >&2
    exit 2
}

isa_zeile_lesen() {
    # $1 = Cache-Schluessel (COMDARE_HOST_RUNS_...), $2 = Zwischendatei. Ergebnis: ISA_N (wie oft der
    # Schluessel im Cache steht) und ISA_ZEILE (seine erste Zeile; leer bei ISA_N = 0). Ueber
    # grep_in_datei und zeilen_zaehlen (Folge (6)): die Fassung 806629ca zaehlte mit 'sed | wc | tr',
    # ein sed-Ausfall waere als '0 Zeilen' zum Datenbefund geworden (Lens C LC3W-01).
    grep_in_datei "$2" "'^$1:' in $ISA_QUELLE (ISA)" -- "^$1:" "$ISA_QUELLE"
    zeilen_zaehlen "$2"; ISA_N=$ZAHL
    ISA_ZEILE=""
    if [ "$ISA_N" -gt 0 ]; then
        IFS= read -r ISA_ZEILE < "$2" || werkzeug_abbruch "'read' ueber $2 (ISA)" "$?"
    fi
}

isa_cache_lesen() {
    # $1 = CMake-Variablenname (COMDARE_HOST_RUNS_...)
    _iv="$1"
    if [ ! -f "$ISA_QUELLE" ]; then
        isa_abbruch "eine Allowlist-Zeile begruendet mit 'isa:', aber '$ISA_QUELLE' fehlt."
    fi
    isa_zeile_lesen "$_iv" "$TMP/isa_wert.txt"
    _in=$ISA_N
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
    _iz=$ISA_ZEILE

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
    # nicht' saehen identisch aus. GENAU EINMAL wie die Wertzeile (Lens C LC3W-08): zwei
    # Zeilen sind ein von Hand bearbeiteter Cache; welche gaelte, entschiede die Reihenfolge.
    isa_zeile_lesen "${_iv}_COMPILED" "$TMP/isa_compiled.txt"
    _ic_n=$ISA_N
    if [ "$_ic_n" -eq 0 ]; then
        isa_abbruch "'${_iv}' steht in '$ISA_QUELLE', aber '${_iv}_COMPILED' fehlt -- die Probe wurde nie gefahren."
    fi
    if [ "$_ic_n" -gt 1 ]; then
        _im="'${_iv}_COMPILED' steht ${_ic_n}-mal in '$ISA_QUELLE' -- ein von Hand"
        isa_abbruch "$_im bearbeiteter Cache ist kein Messergebnis."
    fi
    _ic=${ISA_ZEILE#*=}
    if [ "$_ic" != "TRUE" ]; then
        isa_abbruch "'${_iv}_COMPILED' ist '${_ic}', nicht TRUE -- die ISA-Probe hat nicht einmal uebersetzt."
    fi

    # DER WERT. check_cxx_source_runs schreibt 1 oder LEER; eine 0 hat nie eine Probe
    # geschrieben, auch wenn der Typ INTERNAL lautet.
    _iw=${_iz#*=}
    case "$_iw" in
        1)  ISA_ANTWORT=ja ;;
        '') ISA_ANTWORT=nein ;;
        *)  _im="'${_iv}' hat den Wert '${_iw}'; erwartet ist 1 oder leer."
            isa_abbruch "$_im Ein unverstandener Wert wird nicht zu 'nein' gerundet." ;;
    esac

    # MERKMAL (3) WERT GEGEN _EXITCODE -- der zweite, unabhaengige Beleg. Er steht in
    # einer Zeile, die das Erzwingen nicht mitschreibt, und deckt deshalb auch eine
    # Faelschung ab, die den Typ INTERNAL korrekt trifft. GENAU EINMAL (Lens C LC3W-08), und
    # bei leerem Wert darf er nicht leer sein: try_run legt beide Zeilen gemeinsam an, ein
    # leerer _EXITCODE belegt nichts.
    isa_zeile_lesen "${_iv}_EXITCODE" "$TMP/isa_exitcode.txt"
    _ie_n=$ISA_N
    if [ "$_ie_n" -eq 0 ]; then
        isa_abbruch "'${_iv}_COMPILED' ist TRUE, aber '${_iv}_EXITCODE' fehlt -- try_run legt beide gemeinsam an."
    fi
    if [ "$_ie_n" -gt 1 ]; then
        _im="'${_iv}_EXITCODE' steht ${_ie_n}-mal in '$ISA_QUELLE' -- ein von Hand"
        isa_abbruch "$_im bearbeiteter Cache ist kein Messergebnis."
    fi
    _ie=${ISA_ZEILE#*=}
    if [ "$ISA_ANTWORT" = ja ] && [ "$_ie" != "0" ]; then
        isa_abbruch "'${_iv}' ist 1, aber '${_iv}_EXITCODE' ist '${_ie}' -- Wert und Beleg widersprechen sich."
    fi
    if [ "$ISA_ANTWORT" = nein ] && [ "$_ie" = "0" ]; then
        isa_abbruch "'${_iv}' ist leer, aber '${_iv}_EXITCODE' ist 0: die Probe lief und war ERFOLGREICH."
    fi
    if [ "$ISA_ANTWORT" = nein ] && [ -z "$_ie" ]; then
        _im="'${_iv}' ist leer und '${_iv}_EXITCODE' ist es auch -- try_run schreibt beide Zeilen"
        isa_abbruch "$_im gemeinsam, ein leerer Beleg belegt nichts."
    fi
}

# Die Liste der bekannten Merkmale ist ABSCHLIESSEND. Genau diese zwei gattern in
# diesem Repo (tests/unit/CMakeLists.txt:4165/5330/5343/5372/5375); jedes andere Wort
# in einem 'isa:' ist ein Tippfehler oder eine Erfindung -- und wird nicht geraten.
# isa_bekannt ist die EINE Quelle der Liste: feld_form fragt sie fuer jede Zeile,
# isa_merkmal wird nur noch mit bekannten Merkmalen aufgerufen.
isa_bekannt() {
    case "$1" in
        avx2|avx512f) return 0 ;;
    esac
    return 1
}

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
        *)  # unerreichbar: feld_form weist unbekannte Merkmale ab, bevor hier gefragt wird
            _im="isa_merkmal('$1'): kein bekanntes Merkmal -- feld_form haette"
            isa_abbruch "$_im die Zeile abgewiesen." ;;
    esac
}

# ---------------------------------------------------------------------------
# DIE ERREICHBARKEITS-PROBE (PA-1, zweite ERLOSCHEN-Richtung).
# Setzt ERR_ANTWORT auf
#   nein       -- dieses Repo erklaert den Gegenstand (Index, Gitlink oder .gitignore).
#                 Er kann entstehen; die Ausnahme darf getragen werden.
#   ja         -- KEINE Quelle dieses Repos kennt ihn, oder ein Vorfahr ist eine DATEI
#                 (Folge (11)). TOTE AUSNAHME. ERR_GRUND nennt den Grund, wenn er nicht
#                 der allgemeine ist (leer = 'keine Quelle kennt den Zweig').
#   unbekannt  -- der Pfad liegt ausserhalb des Repos; so nicht beurteilbar.
#   unpruefbar -- ein Vorfahr ist ein Symlink oder steht im Typ-Konflikt (Folge (11));
#                 ERR_GRUND nennt ihn. UNPRUEFBARE Begruendung, rot.
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
    # 'git check-ignore -q': 0 = ignoriert, 1 = nicht ignoriert, 128 = git selbst scheitert --
    # das ist KEINE Antwort auf die Frage, sondern Exit 2 (Lens C LC3W-03; bis 806629ca galt
    # 128 als 'nicht ignoriert' und konnte einen Gegenstand fuer tot erklaeren).
    _ci=0
    git check-ignore -q -- "$1" 2>/dev/null || _ci=$?
    [ "$_ci" -le 1 ] || werkzeug_abbruch "'git check-ignore' fuer $1" "$_ci"
    if [ "$_ci" -eq 0 ]; then return 0; fi
    _ci=0
    git check-ignore -q -- "$1/" 2>/dev/null || _ci=$?
    [ "$_ci" -le 1 ] || werkzeug_abbruch "'git check-ignore' fuer $1/" "$_ci"
    if [ "$_ci" -eq 0 ]; then return 0; fi
    return 1
}

index_eintrag() {
    # $1 = repo-relativer Pfad. Setzt INDEX_ART -- was der Index GENAU ueber diesen Pfad sagt:
    #   gitlink   ein Submodul-Gitlink (Modus 160000 = 'commit'), auf Stufe 0 oder auf jeder
    #             vorhandenen Konfliktstufe. Der Inhalt eines nicht ausgecheckten Submoduls steht
    #             NICHT im Index -- der Gitlink schon.
    #   datei     ein regulaeres Blob (100644/100755): eine DATEI, unter der nie ein Kind entsteht.
    #   symlink   ein Symlink (120000).
    #   konflikt  ein Merge-Konflikt mit UNGLEICHEN Typen je Stufe (etwa Datei gegen Gitlink).
    #   inhalt    kein Eintrag fuer den Pfad selbst, aber Eintraege DARUNTER: ein Verzeichnis.
    #   leer      gar kein Eintrag.
    # 'git ls-files' meldet 0 auch ohne Treffer; jeder andere Status ist ein Werkzeug-Ausfall (Exit 2).
    # DIE VERZEICHNIS-FALLE (Fix-r4, Fund N-1 der Fix-r3-Berichtsfassung, Lead-Objektprobe
    # K205): fuer ein VERZEICHNIS liefert 'git ls-files -s' ALLE Eintraege darunter, und die
    # Fassungen 806629ca..63f8abd4 prueften nur, ob die Ausgabe mit '160000 ' BEGINNT. Am
    # Objekt: ':(literal)ext/queuing' liefert 9 Zeilen, die erste ist der Gitlink
    # ext/queuing/Q01-concurrentqueue -- das Verzeichnis ext/queuing galt damit selbst als
    # Gitlink, und 'ext/queuing/nicht_da/x.hpp' war erreichbar statt TOT (fail-open in der
    # PA-1-Richtung). Deshalb zeilenweise: nur eine Zeile, deren Pfadfeld (nach dem TAB) genau
    # "$1" ist, sagt etwas ueber den Pfad SELBST; ihr Modus sagt, was er ist.
    # TYP UND STUFE (Fix-r5, Kopf Folge (11); Lens A r5 LA5-01/LA5-02/LA5-06, Lens B r4 LB4-01/
    # LB4-03, Lead-Entscheid O-12): bis c62cfc7e zaehlte nur '160000', alles andere galt als
    # 'kein Gitlink' und lief in hat_getrackten_inhalt -- eine Datei als Ahne wurde so zum
    # 'vorhandenen Verzeichnis' (fail-open). Jetzt traegt jede exakte Zeile ihren Typ; stimmen
    # alle vorhandenen Stufen im Typ ueberein, gilt dieser Typ (ein Gitlink, der nur auf Stufe
    # 1-3 steht, ist ein Gitlink -- in JEDEM Merge-Ausgang bleibt der Pfad darunter erreichbar);
    # weichen die Typen je Stufe ab, ist es ein 'konflikt' (die Antwort haengt vom Ausgang ab).
    # Ein Modus ausserhalb der vier, die git in den Index schreibt, ist kein Datenbefund: Exit 2.
    # SCHREIBWEISE (Fix-r5, Kopf Folge (12); Lens A r5 LA5-03): '-c core.quotePath=false' gibt
    # Nicht-ASCII-Bytes roh aus, damit ein Pfad wie 'ext/ae-sub-<0xC3 0xA4>' seinem Eintrag
    # gleicht; Tabulator, Steuerzeichen, Anfuehrungszeichen und Backslash quotiert git trotzdem --
    # solche Pfade weist feld_form vorher als UNPRUEFBAR ab, hier begegnen sie keinem Vergleich.
    # Zwischendatei statt Pipe (Folge (6)); Zerlegung per Parametererweiterung wie am Anker.
    git -c core.quotePath=false ls-files -s -- ":(literal)$1" > "$TMP/index_eintrag.txt" 2>/dev/null ||
        werkzeug_abbruch "'git ls-files -s' fuer $1" "$?"
    _gt=$(printf '\t')
    INDEX_ART=leer
    _gtyp=""
    while IFS= read -r _gl || [ -n "$_gl" ]; do
        [ -n "$_gl" ] || continue
        if [ "$INDEX_ART" = leer ]; then INDEX_ART=inhalt; fi
        _gm=${_gl%% *}
        _gp=${_gl#*"$_gt"}
        if [ "$_gp" != "$1" ]; then continue; fi
        case "$_gm" in
            160000)        _g1=gitlink ;;
            100644|100755) _g1=datei ;;
            120000)        _g1=symlink ;;
            *)  _gmsg="'git ls-files -s' fuer $1 lieferte den Index-Modus '$_gm'"
                werkzeug_abbruch "$_gmsg (kein 100644/100755/120000/160000)" 1 ;;
        esac
        if [ -z "$_gtyp" ]; then _gtyp=$_g1; elif [ "$_gtyp" != "$_g1" ]; then _gtyp=konflikt; fi
    done < "$TMP/index_eintrag.txt"
    if [ -n "$_gtyp" ]; then INDEX_ART=$_gtyp; fi
    return 0
}

hat_getrackten_inhalt() {
    # Gibt es IRGENDEINEN Index-Eintrag fuer den Pfad oder darunter? Gefragt fuer den Gegenstand selbst
    # (eine getrackte, im Arbeitsbaum fehlende Datei kann per Checkout wiederkommen) und fuer die Ahnen in
    # der Aufwaerts-Schleife (dort nur noch Verzeichnisse: Datei, Symlink und Typ-Konflikt hat die
    # Index-Lesung davor entschieden). Hier zaehlt nur, OB Zeilen kommen -- ohne '-s', ohne quotePath-
    # Schalter; Fall (23b) pinnt genau diese Aufrufform ('git ls-files --' mit 128 = Exit 2).
    _t=$(git ls-files -- ":(literal)$1" 2>/dev/null) || werkzeug_abbruch "'git ls-files' fuer $1" "$?"
    if [ -n "$_t" ]; then return 0; fi
    return 1
}

erreichbarkeit() {
    # $1 = repo-relativer Pfad aus Feld 2. Setzt ERR_ANTWORT und ERR_GRUND.
    _p="$1"
    ERR_GRUND=""
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
    # ---------------------------------------------------------------------
    # ZUERST DIE INDEX-EINTRAEGE DER AHNENREIHE (Fix-r3: Gitlinks, am Objekt gefunden
    # in Fall SubmodulGitlinkIstErreichbar; Fix-r5: auch Datei, Symlink, Typ-Konflikt,
    # Kopf Folge (11)): 'git check-ignore' antwortet fuer einen Pfad UNTER einem
    # Gitlink nicht mit 0/1, sondern stirbt mit 128 ("is in submodule"), unter einem
    # Symlink im Arbeitsbaum ebenso ("beyond a symbolic link"). Bis 806629ca galt
    # diese 128 als 'nicht ignoriert' -- richtig nur durch Zufall, denn seit Fix-r3
    # ist 128 ein Werkzeug-Ausfall (Lens C LC3W-03). Ein Gitlink darf beliebig tief
    # Ausgechecktes aufnehmen (Kopf): liegt einer in der Ahnenreihe, ist der Pfad
    # erreichbar; eine DATEI in der Ahnenreihe kann kein Kind haben: TOT; ein Symlink
    # oder ein Typ-Konflikt ist UNPRUEFBAR -- und check-ignore wird fuer keinen dieser
    # Pfade gefragt. Am Objekt gefunden (2026-08-10): der direkte Elternteil von
    # 'ext/<submodul>/include/kopf.hpp' ist 'ext/<submodul>/include' -- und der steht
    # NICHT im Index, weil nur der GITLINK 'ext/<submodul>' dort steht; die erste
    # Fassung erklaerte den Pfad deshalb faelschlich fuer tot. Am Objekt gefunden
    # (Lens A r5, Koeder K4): 'ext/queuing/REPOS_OVERVIEW.md/x.hpp' galt bis c62cfc7e
    # als erreichbar, weil die Datei REPOS_OVERVIEW.md 'getrackten Inhalt' hatte.
    # ---------------------------------------------------------------------
    _ga="$_p"
    while :; do
        case "$_ga" in
            */*) _ga=${_ga%/*} ;;
            *)   break ;;   # der naechste waere die Wurzel
        esac
        index_eintrag "$_ga"
        case "$INDEX_ART" in
            gitlink)
                ERR_ANTWORT=nein; return 0 ;;
            datei)
                ERR_GRUND="sein Vorfahr $_ga ist eine getrackte DATEI (Index-Modus 100644/100755) --"
                ERR_GRUND="$ERR_GRUND unter einer Datei kann nie ein Kind entstehen, weder im Index noch im"
                ERR_GRUND="$ERR_GRUND Arbeitsbaum"
                ERR_ANTWORT=ja; return 0 ;;
            symlink)
                ERR_GRUND="sein Vorfahr $_ga ist im Index ein SYMLINK (Modus 120000) -- die Wache loest"
                ERR_GRUND="$ERR_GRUND keinen Link auf und beurteilt den Zweig dahinter nicht; nenne den Zielpfad"
                ERR_GRUND="$ERR_GRUND des Links"
                ERR_ANTWORT=unpruefbar; return 0 ;;
            konflikt)
                ERR_GRUND="sein Vorfahr $_ga steht im Index im MERGE-KONFLIKT mit ungleichen Typen je Stufe"
                ERR_GRUND="$ERR_GRUND (Datei gegen Gitlink) -- ohne aufgeloeste Fassung ist nicht entscheidbar,"
                ERR_GRUND="$ERR_GRUND ob darunter etwas entstehen kann; erst den Konflikt aufloesen"
                ERR_ANTWORT=unpruefbar; return 0 ;;
        esac
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
    # DIE AHNENREIHE HOCHLAUFEN, nicht nur den direkten Elternteil fragen (die
    # exakten Index-Eintraege -- Gitlink, Datei, Symlink, Konflikt -- sind oben schon
    # entschieden; hier zaehlen Ignore-Regeln und getrackter Inhalt DARUNTER, also
    # Verzeichnisse).
    # ---------------------------------------------------------------------
    _erster=ja
    while :; do
        # Ein ignoriertes Verzeichnis darf beliebig tief Erzeugtes aufnehmen.
        if ist_ignoriert "$_a"; then ERR_ANTWORT=nein; return 0; fi
        if hat_getrackten_inhalt "$_a"; then
            if [ "$_erster" = ja ]; then
                # Eine NEUE Datei in einem VERZEICHNIS, das es schon gibt, ist der
                # Normalfall -- daraus darf nie 'tot' werden. (Dass es ein Verzeichnis
                # ist und keine Datei, hat die Index-Lesung oben entschieden, Folge (11).)
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
    HEUTE=$(date +%Y-%m-%d) || werkzeug_abbruch "'date +%Y-%m-%d' (das Heute fuer 'frist:')" "$?"
fi
case "$HEUTE" in
    [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]) ;;
    *)  printf '%s\n' "ABBRUCH: '$HEUTE' ist kein Datum JJJJ-MM-TT (Herkunft: $HEUTE_HERKUNFT)." >&2
        printf '%s\n' "         Ohne belastbares Heute ist keine 'frist:'-Zeile pruefbar. Fail-closed." >&2
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
# Alles andere (unbekannte Art, kein Praefix, leeres Feld, unbekanntes oder leeres
# Merkmal, kein Datum) ist UNPRUEFBAR und damit ROT -- geprueft von feld_form, das
# auch der Nachscan fuer JEDE Datenzeile fragt (Kopf, Folge (8)).
# ---------------------------------------------------------------------------
: > "$TMP/begruendet.txt"
: > "$TMP/unbegruendet.txt"
: > "$TMP/erloschen.txt"
: > "$TMP/unpruefbar.txt"
: > "$TMP/tot.txt"
NBEURT_N=0

pfad_form() {
    # $1 = Pfad aus 'datei:' -> PFAD_FEHLER (leer = Form in Ordnung). Kopf, Folge (12): (a) ein Zeichen,
    # das 'git ls-files' in seiner Ausgabe quotiert (Tabulator, Steuerzeichen, Anfuehrungszeichen,
    # Backslash), macht den Pfad unvergleichbar; (b) ein Segment '.' oder ein leeres Segment ('./',
    # '//', '/' am Ende) steht so in keinem Index. Ein absoluter Pfad bleibt Sache der Erreichbarkeits-
    # Probe (NICHT beurteilt, Kopf-Grenze (a)), ebenso '..'; Leerzeichen und Nicht-ASCII-Bytes sind
    # erlaubt (roh vergleichbar, Folge (12)). Segmentweise per Parametererweiterung, kein Werkzeug.
    PFAD_FEHLER=""
    case "$1" in /*) return 0 ;; esac
    case "$1" in
        *[[:cntrl:]]*|*'"'*|*'\'*)
            PFAD_FEHLER="enthaelt ein Zeichen, das git in seiner Ausgabe quotiert (Tabulator, Steuerzeichen,"
            PFAD_FEHLER="$PFAD_FEHLER Anfuehrungszeichen oder Backslash) -- die Wache vergleicht Pfade nur unquotiert"
            return 0 ;;
    esac
    _pfr="$1"
    while :; do
        case "$_pfr" in
            */*) _pfs=${_pfr%%/*}; _pfr=${_pfr#*/}; _pfl=nein ;;
            *)   _pfs="$_pfr";     _pfr="";         _pfl=ja ;;
        esac
        if [ -z "$_pfs" ] || [ "$_pfs" = . ]; then
            PFAD_FEHLER="ist kein kanonischer repo-relativer Pfad (Segment '.' oder leeres Segment: './', '//',"
            PFAD_FEHLER="$PFAD_FEHLER '/' am Ende) -- so steht kein Pfad im Index"
            return 0
        fi
        if [ "$_pfl" = ja ]; then break; fi
    done
    return 0
}

feld_form() {
    # $1 = Feld 2, $2 = Feld 3 -> FORM_FEHLER (leer = Form in Ordnung) sowie ART und WERT aus Feld 2.
    # Die FORM einer Zeile, getrennt von der Bewertung ihres Gegenstands: ein leeres Feld 3 (Folge (5)),
    # eine unbekannte Art oder ein Feld 2 ohne Praefix, 'datei:' ohne Pfad, 'frist:' ohne Datum
    # JJJJ-MM-TT, 'isa:' ohne Merkmal, mit leerem Teilmerkmal ('+avx2', 'avx2+', 'a++b'; Lens C
    # LC3W-07, Folge (9)) oder mit unbekanntem Merkmal. Die Wortlaute sind die der Fassung 806629ca.
    FORM_FEHLER=""
    case "$1" in
        *:*) ART=${1%%:*}; WERT=${1#*:} ;;
        *)   ART=""; WERT="$1" ;;
    esac
    case "$2" in
        *[![:space:]]*) ;;
        *)  FORM_FEHLER="UNPRUEFBAR: Feld 3 (Begruendung) ist leer -- eine Ausnahme ohne Begruendungstext"
            FORM_FEHLER="$FORM_FEHLER ist keine (Format: <datei> | <art>:<gegenstand> | <begruendung>)"
            return 0 ;;
    esac
    case "$ART" in
    datei)
        if [ -z "$WERT" ]; then FORM_FEHLER='UNPRUEFBAR: "datei:" ohne Pfad'; return 0; fi
        # Die FORM des Pfads (Folge (12), Fix-r5): quotierbare Zeichen und nicht kanonische Segmente sind
        # UNPRUEFBAR -- vor jeder Frage an den Index, auch in der stummen Zeile (Nachscan, Folge (8)).
        pfad_form "$WERT"
        if [ -n "$PFAD_FEHLER" ]; then FORM_FEHLER="UNPRUEFBAR: \"datei:$WERT\" $PFAD_FEHLER"; fi
        ;;
    frist)
        # JJJJ-MM-TT ist Pflicht: der Vergleich in der Schleife ist ein Zeichenkettenvergleich,
        # der nur in dieser Form chronologisch sortiert.
        case "$WERT" in
            [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]) ;;
            *)  FORM_FEHLER="UNPRUEFBAR: \"frist:$WERT\" ist kein Datum JJJJ-MM-TT" ;;
        esac
        ;;
    isa)
        if [ -z "$WERT" ]; then FORM_FEHLER='UNPRUEFBAR: "isa:" ohne Merkmal'; return 0; fi
        _fr="$WERT"; _funbek=""
        while :; do
            case "$_fr" in
                *+*) _fm=${_fr%%+*}; _fr=${_fr#*+}; _fmehr=ja ;;
                *)   _fm="$_fr";     _fr="";        _fmehr=nein ;;
            esac
            if [ -z "$_fm" ]; then
                # Bis 806629ca wurde ein leeres Teilmerkmal still uebersprungen ('+avx2' galt wie 'avx2').
                FORM_FEHLER="UNPRUEFBAR: \"isa:$WERT\" hat ein leeres Teilmerkmal (Form isa:<merkmal>[+<merkmal>],"
                FORM_FEHLER="$FORM_FEHLER kein '+' am Rand und kein '++')"
                return 0
            fi
            if ! isa_bekannt "$_fm"; then _funbek="$_funbek $_fm"; fi
            if [ "$_fmehr" = nein ]; then break; fi
        done
        if [ -n "$_funbek" ]; then
            FORM_FEHLER="UNPRUEFBAR: unbekannte(s) ISA-Merkmal(e):$_funbek (bekannt: avx2, avx512f)"
        fi
        ;;
    *)
        FORM_FEHLER="UNPRUEFBAR: Feld 2 ist \"$1\" -- keine bekannte Art"
        FORM_FEHLER="$FORM_FEHLER (erwartet \"datei:\", \"isa:\" oder \"frist:\")"
        ;;
    esac
    return 0
}

allow_zeile() {
    # $1 = gesuchte Datei; gibt die Allowlist-Zeile aus oder nichts
    [ -f "$ALLOWLIST" ] || return 0
    # Ein Pfad unter tests/deprecated/ hat per Definition keine Allowlist-Zeile: der Archiv-Ort
    # kennt nur den VERMERK.md-Anker (Kopf, Folge (3)); eine Zeile dafuer meldet der Nachscan.
    case "$1" in tests/deprecated/*) return 0 ;; esac
    # '|| [ -n "$z" ]': auch eine LETZTE Zeile OHNE Zeilenumbruch wird gelesen (Lens C LCW-04,
    # Lens A LA3-05) -- 'read' liefert dort Status 1, obwohl es Zeichen gelesen hat.
    while IFS= read -r z || [ -n "$z" ]; do
        case "$z" in ''|'#'*) continue ;; esac
        felder "$z"
        [ "$FELD1" = "$1" ] || continue
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
    felder "$z"
    geg=$FELD2
    txt=$FELD3

    # ERST DIE FORM (feld_form: Feld 3, Art, Wert; Kopf Folgen (5), (8), (9)), dann der Gegenstand.
    feld_form "$geg" "$txt"
    if [ -n "$FORM_FEHLER" ]; then
        printf '%s -- %s\n' "$f" "$FORM_FEHLER" >> "$TMP/unpruefbar.txt"
        continue
    fi
    art=$ART
    wert=$WERT

    case "$art" in
    datei)
        if [ -e "$wert" ]; then
            printf '%s -- ERLOSCHEN: "%s" existiert wieder, die Ausnahme traegt nicht mehr\n' \
                "$f" "$wert" >> "$TMP/erloschen.txt"
        else
            # ZWEITE RICHTUNG (PA-1): abwesend genuegt nicht. Ein Gegenstand, den keine
            # Quelle dieses Repos kennt, kann nicht wiederkommen -- die Ausnahme koennte
            # dann per Konstruktion nie erloeschen und waere eine Freistellung ohne Ende.
            erreichbarkeit "$wert"
            if [ "$ERR_ANTWORT" = ja ]; then
                _tmsg="TOTE AUSNAHME: \"$wert\" existiert nicht, und"
                if [ -n "$ERR_GRUND" ]; then
                    # Ein Vorfahr ist eine DATEI (Folge (11)): der Grund steht dabei.
                    _tmsg="$_tmsg $ERR_GRUND"
                else
                    _tmsg="$_tmsg keine Quelle dieses Repos"
                    _tmsg="$_tmsg kennt den Zweig, in dem er liegt (weder Index noch Submodul-Gitlink"
                    _tmsg="$_tmsg noch .gitignore, bis hinauf zur Wurzel)"
                fi
                _tmsg="$_tmsg -- diese Zeile koennte nie erloeschen"
                printf '%s -- %s\n' "$f" "$_tmsg" >> "$TMP/tot.txt"
            elif [ "$ERR_ANTWORT" = unpruefbar ]; then
                # Symlink oder Typ-Konflikt in der Ahnenreihe (Folge (11)): UNPRUEFBAR, in derselben Liste
                # und Zaehlung wie ein Formfehler der Zeile -- die Wache kann die Begruendung nicht pruefen.
                _umsg="UNPRUEFBAR: \"$wert\" existiert nicht, und $ERR_GRUND"
                printf '%s -- %s\n' "$f" "$_umsg" >> "$TMP/unpruefbar.txt"
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
        # chronologisch, und genau darum ist das Format Pflicht (feld_form prueft es).
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
        # Jedes Teilmerkmal ist nicht leer und bekannt -- das hat feld_form geprueft.
        _rest="$wert"; _alle_da=ja; _bericht=""
        while [ -n "$_rest" ]; do
            case "$_rest" in
                *+*) _m=${_rest%%+*}; _rest=${_rest#*+} ;;
                *)   _m="$_rest";     _rest="" ;;
            esac
            isa_merkmal "$_m"
            _bericht="$_bericht ${_m}=${ISA_ANTWORT}"
            [ "$ISA_ANTWORT" = ja ] || _alle_da=nein
        done
        if [ "$_alle_da" = ja ]; then
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
        # unerreichbar: feld_form laesst nur datei/frist/isa durch
        werkzeug_abbruch "feld_form liess die Art '$art' fuer $f durch" 1
        ;;
    esac
done < "$TMP/fehlend.txt"

# ---------------------------------------------------------------------------
# JEDE WIRKSAME ALLOWLIST-ZEILE BRAUCHT EINEN GEGENSTAND IM SOLL (Lens B LB-01 /
# Lens A LA-08, 2026-09-18). Die Schleife oben laeuft nur ueber fehlend.txt -- eine
# Zeile, deren Feld 1 keine Datei des SOLL nennt, wird NIE ausgewertet: nicht bei
# diesem Lauf, nicht beim naechsten. Fuenf Klassen, alle UNPRUEFBAR und damit ROT:
#   ARCHIV   Feld 1 nennt eine archivierte Datei. Der Ort traegt sie, die Zeile hat
#            keinen Anlass -- und laege in Reserve: faellt der Anker, wird sie ohne
#            Ablauf wirksam. Am Objekt gemessen (Lens B, Probe P10): 'frist:2000-01-01'
#            fuer eine ARCHIV-Datei -> Exit 0, kein ABGELAUFEN.
#   ORT      Feld 1 nennt einen Pfad unter tests/deprecated/, der NICHT archiviert ist
#            (kein oder kein wirksamer Anker, direkt unter tests/deprecated/, oder gar
#            nicht getrackt). Der Archiv-Ort kennt nur den Anker (Kopf, Folge (3)); die
#            Zeile wuerde die Datei sonst durch die Hintertuer als 'frist:' tragen.
#   GEIST    Feld 1 nennt keine Datei des SOLL-Bestands -- der getrackten Test-Quell-
#            dateien ausserhalb ext/ (geloescht, umbenannt, unter ext/, kein test_*.cpp,
#            anders geschrieben, ein eingerueckter Kommentar, oder leer). Auch diese
#            Zeile schlaeft und erwacht mit dem naechsten Namensgleichen.
#   DOPPELT  Feld 1 steht mehrfach (Kopf, Folge (4)): nur die erste Zeile wuerde je
#            gelesen, die weiteren schlafen -- welche, entscheidet die Reihenfolge.
#   FORM     Feld 1 nennt eine Datei IM Bauweg (Lens C LC3W-05, Kopf Folge (8)): die Zeile
#            ist stumm (Design, Lens A LA3-08 -- die 'isa:'-Zeile ist auf dem AVX-512-Host
#            genau so stumm), ihre Form gilt trotzdem: leeres Feld 3, unbekannte Art,
#            leeres oder unbekanntes Merkmal, kein Datum -- geprueft mit demselben
#            feld_form wie in der Schleife. Zeilen fuer dem Bauweg FEHLENDE Dateien hat die
#            Schleife schon geprueft (die erste je Pfad; jede weitere ist DOPPELT).
# Eine Zeile nur aus Leerraum ist eine Leerzeile, keine Datenzeile (Lens A LA3-03; so
# liest sie auch test_mt_l4_registrierungs_wache_isa Fall (8)). Alle Klassen zaehlen
# als UNPRUEFBAR, in derselben Liste wie eine Zeile mit unbekannter Art: eine
# Begruendung ohne Gegenstand ist keine. Abhilfe: die Zeile loeschen bzw. berichtigen.
# ---------------------------------------------------------------------------
: > "$TMP/zeilen_unpruefbar.txt"
: > "$TMP/zeilen_feld1.txt"
ARCHIV_ZEILE_N=0
ORT_ZEILE_N=0
GEIST_ZEILE_N=0
DOPPEL_ZEILE_N=0
FORM_ZEILE_N=0
if [ -f "$ALLOWLIST" ]; then
    while IFS= read -r z || [ -n "$z" ]; do
        case "$z" in ''|'#'*) continue ;; esac
        case "$z" in *[![:space:]]*) ;; *) continue ;; esac
        felder "$z"
        _d=$FELD1
        if [ -z "$_d" ]; then
            _msg="UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 ist leer; die Zeile wird nie"
            _msg="$_msg ausgewertet und gehoert geloescht"
            printf '(leeres Feld 1) -- %s\n' "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
            GEIST_ZEILE_N=$((GEIST_ZEILE_N + 1))
            continue
        fi
        printf '%s\n' "$_d" >> "$TMP/zeilen_feld1.txt"
        case "$_d" in
            '#'*)
                # Ein Kommentar beginnt in Spalte 1; eingerueckt ist er fuer beide Leser eine
                # Datenzeile ohne Gegenstand (Lens A LA3-04, Lens B LB2-02) -- und heisst hier so.
                _msg="UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 beginnt mit '#': ein"
                _msg="$_msg EINGERUECKTER Kommentar? Kommentare beginnen in Spalte 1; so ist die Zeile"
                _msg="$_msg eine Datenzeile ohne Gegenstand und wird nie ausgewertet"
                printf '%s -- %s\n' "$_d" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
                GEIST_ZEILE_N=$((GEIST_ZEILE_N + 1))
                continue ;;
        esac
        grep_in_datei /dev/null "-F -x ueber archiv.txt (Nachscan)" -q -F -x -- "$_d" "$TMP/archiv.txt"
        if [ "$GREP_RC" -eq 0 ]; then
            _msg="UNPRUEFBAR: ARCHIV-Datei mit Allowlist-Zeile -- Ausnahme ohne Anlass (der VERMERK.md-"
            _msg="${_msg}Anker nimmt die Datei aus dem SOLL; die Zeile wird nie ausgewertet, gehoert geloescht)"
            printf '%s -- %s\n' "$_d" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
            ARCHIV_ZEILE_N=$((ARCHIV_ZEILE_N + 1))
            continue
        fi
        case "$_d" in
            tests/deprecated/*)
                _msg="UNPRUEFBAR: Allowlist-Zeile fuer einen Pfad unter tests/deprecated/ -- der Archiv-Ort"
                _msg="$_msg kennt nur den VERMERK.md-Anker, keine Ausnahme (Owner-Order 206: frist-Zeilen"
                _msg="$_msg entfernt); ohne Anker bleibt die Datei OHNE BEGRUENDUNG, die Zeile gehoert geloescht"
                printf '%s -- %s\n' "$_d" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
                ORT_ZEILE_N=$((ORT_ZEILE_N + 1))
                continue ;;
        esac
        grep_in_datei /dev/null "-F -x ueber soll_roh.txt (Nachscan)" -q -F -x -- "$_d" "$TMP/soll_roh.txt"
        if [ "$GREP_RC" -ne 0 ]; then
            _msg="UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 steht nicht im SOLL-Bestand der"
            _msg="$_msg getrackten Test-Quelldateien ausserhalb ext/ (geloescht, umbenannt, unter ext/, kein"
            _msg="$_msg test_*.cpp?); die Zeile wird nie ausgewertet"
            printf '%s -- %s\n' "$_d" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
            GEIST_ZEILE_N=$((GEIST_ZEILE_N + 1))
            continue
        fi
        # FORM FUER DATEIEN IM BAUWEG (Kopf, Folge (8)): Feld 1 steht im SOLL, ist weder ARCHIV noch
        # Archiv-Pfad -- fehlt die Datei dem Bauweg, hat die Schleife oben ihre Zeile geprueft; sonst
        # ist die Zeile stumm und nur ihre Form zaehlt.
        grep_in_datei /dev/null "-F -x ueber fehlend.txt (Nachscan)" -q -F -x -- "$_d" "$TMP/fehlend.txt"
        if [ "$GREP_RC" -eq 0 ]; then continue; fi
        feld_form "$FELD2" "$FELD3"
        if [ -n "$FORM_FEHLER" ]; then
            _msg="$FORM_FEHLER -- die Zeile gilt einer Datei IM Bauweg und ist stumm, ihre Form gilt"
            _msg="$_msg trotzdem (sie erwachte mit dem naechsten Fehlen der Datei als Freibrief)"
            printf '%s -- %s\n' "$_d" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
            FORM_ZEILE_N=$((FORM_ZEILE_N + 1))
        fi
    done < "$ALLOWLIST"
    # DOPPELTE ZEILEN JE PFAD (Lens C LCW-08): Feld 1 aller Datenzeilen, sortiert, 'uniq -d'
    # nennt jeden mehrfachen Pfad genau einmal; die Haeufigkeit kommt aus der ungekuerzten Liste.
    sort "$TMP/zeilen_feld1.txt" > "$TMP/zeilen_feld1_sortiert.txt" ||
        werkzeug_abbruch "'sort' ueber Feld 1 der Allowlist" "$?"
    uniq -d "$TMP/zeilen_feld1_sortiert.txt" > "$TMP/zeilen_doppelt.txt" ||
        werkzeug_abbruch "'uniq -d' ueber Feld 1 der Allowlist" "$?"
    while IFS= read -r _dd || [ -n "$_dd" ]; do
        [ -n "$_dd" ] || continue
        grep_in_datei "$TMP/zeilen_doppelt_treffer.txt" "-F -x ueber Feld 1 (DOPPELT)" \
            -F -x -- "$_dd" "$TMP/zeilen_feld1.txt"
        zeilen_zaehlen "$TMP/zeilen_doppelt_treffer.txt"
        _msg="UNPRUEFBAR: DOPPELTE ALLOWLIST-ZEILE -- Feld 1 steht ${ZAHL}-mal in der Allowlist; nur die"
        _msg="$_msg erste Zeile wuerde gelesen, die weiteren schlafen und erwachen mit der Reihenfolge --"
        _msg="$_msg gehoert auf EINE Zeile"
        printf '%s -- %s\n' "$_dd" "$_msg" >> "$TMP/zeilen_unpruefbar.txt"
        DOPPEL_ZEILE_N=$((DOPPEL_ZEILE_N + 1))
    done < "$TMP/zeilen_doppelt.txt"
fi

zeilen_zaehlen "$TMP/begruendet.txt"; BEGR_N=$ZAHL
zeilen_zaehlen "$TMP/unbegruendet.txt"; UNBEGR_N=$ZAHL
zeilen_zaehlen "$TMP/erloschen.txt"; ERL_N=$ZAHL
zeilen_zaehlen "$TMP/unpruefbar.txt"; UNPR_FEHLEND_N=$ZAHL
zeilen_zaehlen "$TMP/tot.txt"; TOT_N=$ZAHL
# UNPRUEFBAR gesamt: die Zeilen aus der Schleife (je eine dem Bauweg fehlende Datei)
# PLUS die Klassen ohne Bezug zum Bauweg (Anker ohne Inhalt/Form/Blob, Zeilen fuer ARCHIV-
# Dateien und Archiv-Pfade, Zeilen ohne Gegenstand, doppelte Pfade, Formfehler stummer
# Zeilen). Der Nenner unten weist alle Anteile getrennt aus.
UNPR_N=$((UNPR_FEHLEND_N + ANKER_UNPR_N + ARCHIV_ZEILE_N + ORT_ZEILE_N + GEIST_ZEILE_N + DOPPEL_ZEILE_N))
UNPR_N=$((UNPR_N + FORM_ZEILE_N))

echo "-----------------------------------------------------------------------------"
echo "TEST-REGISTRIERUNGS-WACHE (MT-L4) -- Quelldatei gegen Bauweg"
echo "  SOLL-Quelle: git ls-files (Git-Index)"
printf '%s\n' "  IST-Quelle : $IST_ART im Baum '$BUILD'"
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ]; then
    echo ""
    echo "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS -- diese Dateien werden nie uebersetzt:"
    eingerueckt "$TMP/unbegruendet.txt"
fi
if [ "$ERL_N" -gt 0 ]; then
    echo ""
    echo "AUSNAHME ERLOSCHEN -- die Begruendung gilt am Gegenstand nicht mehr:"
    eingerueckt "$TMP/erloschen.txt"
fi
if [ "$TOT_N" -gt 0 ]; then
    echo ""
    echo "TOTE AUSNAHME -- der Gegenstand kann in KEINEM erklaerten Baum entstehen:"
    eingerueckt "$TMP/tot.txt"
fi
if [ "$UNPR_N" -gt 0 ]; then
    echo ""
    echo "UNPRUEFBARE BEGRUENDUNG -- nichts davon kann diese Wache nachpruefen (Feld 2 oder 3, Anker-Form," \
         "Zeile ohne Gegenstand, Archiv-Pfad, doppelter Pfad, Formfehler einer stummen Zeile):"
    eingerueckt "$TMP/unpruefbar.txt" "$TMP/anker_unpruefbar.txt" "$TMP/zeilen_unpruefbar.txt"
fi
if [ "$BEGR_N" -gt 0 ]; then
    echo ""
    echo "ABWESEND, ABER BEGRUENDET (Allowlist $ALLOWLIST):"
    eingerueckt "$TMP/begruendet.txt"
fi

if [ "$ARCHIV_N" -gt 0 ]; then
    echo ""
    echo "ARCHIV (tests/deprecated/, VERMERK.md-Anker): $ARCHIV_N Datei(en) in" \
         "$ARCHIV_ORD_N Ordner(n), nicht im SOLL:"
    eingerueckt "$TMP/archiv.txt"
fi

echo ""
echo "-----------------------------------------------------------------------------"
echo "NENNER (nie eine nackte Zahl):"
echo "  $SOLL_ROH_N getrackte Test-Quelldatei(en) im Baum (ohne ext/)."
echo "  davon $ARCHIV_N ARCHIV-Datei(en) in $ARCHIV_ORD_N Ordner(n) unter tests/deprecated/ mit" \
     "VERMERK.md-Anker abgezogen -- SOLL: $SOLL_N."
printf '%s\n' "  $FEHLEND_N davon NICHT im Bauweg des Baums '$BUILD'."
echo "  davon $BEGR_N begruendet, $ERL_N mit ERLOSCHENER, $TOT_N TOTE AUSNAHME," \
     "$UNPR_FEHLEND_N mit UNPRUEFBARER Begruendung, $UNBEGR_N ohne."
echo "  dazu UNPRUEFBAR ohne Bezug zum Bauweg: $ANKER_UNPR_N ARCHIV-Anker ohne Inhalt/Form," \
     "$ARCHIV_ZEILE_N Allowlist-Zeile(n) fuer ARCHIV-Dateien, $ORT_ZEILE_N fuer Pfade unter tests/deprecated/" \
     "ohne wirksamen Anker, $GEIST_ZEILE_N ohne Gegenstand im SOLL-Bestand, $DOPPEL_ZEILE_N Pfad(e) mit" \
     "doppelter Zeile, $FORM_ZEILE_N mit Formfehler fuer Dateien im Bauweg."
echo "  Erreichbarkeit: $NBEURT_N der $BEGR_N begruendeten nennen einen Gegenstand AUSSERHALB"
echo "                  des Repos -- fuer die ist 'kann nie entstehen' NICHT beurteilt worden."
echo "  Heute (fuer 'frist:'): $HEUTE -- Herkunft: $HEUTE_HERKUNFT."
echo "  Gegenprobe des Messgeraets: '$GEGENPROBE' trifft in $IST_ART (das Muster sucht)."
if [ "$ISA_GEFRAGT" = ja ]; then
    printf '%s\n' "  ISA-Gegenprobe: $ISA_QUELLE"
    echo "                  avx2=${ISA_AVX2:-nicht gefragt}, avx512f=${ISA_AVX512F:-nicht gefragt}"
    echo "                  (vier Merkmale je Variable geprueft: einmalig, Typ INTERNAL,"
    echo "                   _COMPILED=TRUE, Wert deckt _EXITCODE)"
else
    echo "  ISA-Gegenprobe: nicht gefragt -- keine abwesende Datei ist mit einer wohlgeformten 'isa:'-Zeile" \
         "begruendet."
fi
echo "-----------------------------------------------------------------------------"

if [ "$UNBEGR_N" -gt 0 ] || [ "$ERL_N" -gt 0 ] || [ "$UNPR_N" -gt 0 ] || [ "$TOT_N" -gt 0 ]; then
    echo "TEST-REGISTRIERUNGS-WACHE: ROT ($UNBEGR_N von $SOLL_N ohne Begruendung," \
         "$ERL_N erloschen, $TOT_N tot, $UNPR_N unpruefbar, $ARCHIV_N archiviert)."
    echo "Abhilfe: die Datei per comdare_add_test()/add_test() in den Bauweg nehmen -- ODER"
    echo "         sie mit einem nachpruefbaren Gegenstand in $ALLOWLIST begruenden"
    echo "         ('datei:<pfad>', 'isa:<merkmal>[+<merkmal>]' oder 'frist:<JJJJ-MM-TT>', s. Kopf"
    echo "         der Allowlist) -- ODER sie nach Owner-Entscheid unter tests/deprecated/<ordner>/"
    echo "         ablegen, mit einem VERMERK.md (Nicht-Leerraum-Inhalt, im Index auf Stufe 0) daneben:"
    echo "         ARCHIV, s. Kopf dieser Wache; ein Pfad unter tests/deprecated/ braucht und vertraegt"
    echo "         KEINE Allowlist-Zeile (auch nicht ohne Anker -- dann fehlt der Anker, nicht die Zeile)."
    if [ "$TOT_N" -gt 0 ]; then
        echo "Bei einer TOTEN AUSNAHME hilft kein anderer Pfad: gibt es keinen erreichbaren"
        echo "         Gegenstand, gehoert die Zeile auf 'frist:<JJJJ-MM-TT>' -- eine geparkte"
        echo "         Ausnahme mit Ablaufdatum statt einer Adresse, die nie jemand belegt."
    fi
    exit 1
fi

echo "TEST-REGISTRIERUNGS-WACHE: OK ($SOLL_N Quelldateien, $UNBEGR_N ohne Begruendung ausserhalb," \
     "$ARCHIV_N archiviert)."
exit 0
