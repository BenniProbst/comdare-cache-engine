# VERMERK: prt-art-Legacy-Waisen -- Archiv statt Loeschung (W-B, 2026-08-15)

## Was hier liegt

Vier per `git mv` aus `tests/unit/` archivierte Google-Test-Quelldateien (24 TEST-Faelle gesamt),
seit PA-3 geparkt (E-1 `8a8f6877`), nie im heutigen Bau verdrahtet:

| Datei                          | TEST-Faelle | Gegenstand (Alt-Welt)                          |
|--------------------------------|-------------|------------------------------------------------|
| `test_concepts_compile.cpp`    | 5           | Smoke ueber `prt_art/prt_art.hpp` + Enums      |
| `test_six_page_structures.cpp` | 9           | 6 Seitentypen + 6 Interpreter (REV 5 K05)      |
| `test_three_layer_audit.cpp`   | 7           | 3-Schichten-Audit der Concepts (REV 5.1)       |
| `test_value_handle.cpp`        | 3           | ValueHandle Inline/External/ChainRef Roundtrip |

NICHT geloescht (Doku-/Bestandsregel): die Dateien sind die einzige ausformulierte Fassung der
REV-5-Vertraege und bleiben als Referenz fuer eine etwaige Neu-Deckung liegen.

## Owner-Rahmen und warum Bedingung (a) am Objekt nicht erfuellt ist

Vorlage-B2-GO (Owner 15.08.2026, Ledger "VORLAGE-B-RUNDE", Task #65): Aufraeumen mit Praeferenz
**(a) "neu verdrahten wenn Sinnvoll unter Umbau"**.

Am Objekt gemessen (ce `0817c7bf`, prt-art `a782c56`, 2026-08-15): **(a) hat keinen Gegenstand.**

- **0 von 21** distinkten `#include`-Zielen der vier Dateien loesen unter `prt_art/include` des
  heutigen prt-art auf. Die komplette Alt-Welt fehlt: `prt_art/prt_art.hpp` (laut ce-Kontrakt
  `cmake/pruefling_kontrakt.cmake` seit 14.05.2026 nirgends), `prt_art/concepts/` (8 Header),
  `prt_art/page_structures/` (6), `prt_art/interpreters/` (6).
- Gegenprobe (kein stiller Suchfehler): das heutige `prt_art/include` traegt 42 Dateien in den
  Baeumen `slots/`, `value_handle/`, `prefetch/`, `nodes/`, `identity/`, `internal_search/`, ...
  -- keine davon ist eines der 21 Ziele.
- CMake: **0 aktive Verdrahtungen** der vier Dateien im ganzen ce (Treffer nur: zwei
  W0a/S4-Entfall-KOMMENTARE in `tests/unit/CMakeLists.txt:146-147,209` und das unabhaengige
  Target `test_value_handle_real`). Der Umzug beruehrt deshalb keinen Bau.

Neu-Verdrahtung wuerde bedeuten, die Tests gegen eine Schnittstellen-Welt zu uebersetzen, die es
nicht mehr gibt -- das ist Neuschreiben, nicht Verdrahten. Deshalb Archiv (dieser Ordner) statt
(a); die Loeschung braeuchte ein eigenes Owner-GO (siehe Frist unten).

## Die DREI aufgegebenen Deckungs-Luecken

Mit dem Archiv wird die folgende Deckung AUFGEGEBEN -- kein heutiger Test traegt sie (Gegenprobe:
`fanout`/`next_slot`/`cache_page` treffen im heutigen prt-art nur 3 Dateien -- Registrierungs-Test,
`slots/axis_07_prefetch_slot.hpp`, `prefetch/redirect_prefetch.hpp` -- keine traegt die Vertraege):

1. **Fanout-15-Vertrag**: `kDefaultFanoutWidth == 15` und "IFanout HAT ISearchPages, nicht INodes"
   (`ConceptsCompile.DefaultFanoutWidthIs15`, `ThreeLayerAudit.ISearchPageHasFanoutWidth15`,
   `ThreeLayerAudit.IFanoutHasPagesNotNodes`). Kein Nachfolger prueft eine Fanout-Breite.
2. **ICachePage-Fragmentierung**: der Drei-Zustands-Vertrag `FragmentationKind`
   NotPresent/WhollyContained/Fragmented aus `i_cache_page.hpp`
   (`ThreeLayerAudit.FragmentationKindKnowsThreeStates`). Kein heutiger Fragmentierungs-Test.
3. **Interpreter-SIMD/next_slot**: die `supports_simd`-Matrix der 6 Interpreter (inkl. PEXT/AVX2
   fuer Patricia) und der `next_slot`-Roundtrip
   (`SixInterpreters.SimdSupportFlagsMatchExpectations`, `next_slot` in
   `test_six_page_structures.cpp:101`). Die heutige Slot-Welt hat dafuer keinen Traeger.

Teilweise anders gedeckt (NICHT aufgegeben): der ValueHandle-Roundtrip hat mit
`test_value_handle_real` (value_handle-Achse) und `slots/axis_14_value_handle_slot.hpp` heutige
Nachfolger auf der neuen Achsen-Welt.

## Zaehlung (A2.5-Fix Runde 1, FUND-1)

Es sind GENAU VIER Waisen-Dateien (Tabelle oben; 24 TEST-Faelle = 5+9+7+3). Die "7" des
Archiv-Commits b39d62a2 ist dessen shortstat-Zeile "7 files changed" = 4 R100-Renames
+ dieses VERMERK.md (neu) + 2 Anpassungen (`scripts/ci_test_registrierungs_allowlist.txt`,
`tests/unit/test_pa1_tote_ausnahme.cpp`). Fuer Landungs-/Ledger-Texte gilt:
"4 Waisen-Dateien (7 geaenderte Dateien im Commit)" -- nie "7 Waisen".

## Frist und Reaktivierungs-Weg

Die `frist:2026-09-15`-Zeilen [Stand W-B 15.08.; seit `57757fe3` (Owner 15.09., Frage 1)
`frist:2026-09-18`; am 17.09. mit OV-2 entfernt, s. Nachtrag unten] der vier Dateien bleiben in
`scripts/ci_test_registrierungs_allowlist.txt` (SOLL ist am DATEINAMEN verankert, Pfad in Feld 1
nachgezogen; `tests/unit/test_pa1_tote_ausnahme.cpp` kGeparkt ebenso). Die Frist gehoert dem Owner
und deckt jetzt die ENDGUELTIGE Form: Archiv bestaetigen ODER Loeschung mit eigenem Owner-GO.
Eine Neu-Deckung der drei Luecken laeuft ueber neue Tests gegen die heutige Slot-Welt bzw. eine
neue Faehigkeit im ce-Kontrakt (`COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN`), nicht ueber Reanimation
dieser Dateien.

## OV-2-NACHTRAG (Owner-Entscheid 2026-09-17, Order 206)

Owner verbatim: "OV-2 Archiv-Variante gilt und frist Zeilen entfernen".

Damit ist der Abschnitt "Frist und Reaktivierungs-Weg" oben UEBERHOLT, soweit er die `frist:`-Zeilen
betrifft: die vier Zeilen sind aus `scripts/ci_test_registrierungs_allowlist.txt` ENTFERNT. Das
Archiv ist BESTAETIGT, die vier Dateien bleiben liegen -- die Loeschung braucht weiter ein eigenes
Owner-GO. Alles uebrige oben (Zaehlung, die drei aufgegebenen Deckungs-Luecken, Bedingung (a)) gilt
unveraendert fort.

**Diese Datei ist ab jetzt ein ANKER, nicht nur ein Vermerk.**
`scripts/ci_test_registrierungs_wache.sh` nimmt eine getrackte Test-Quelldatei unter
`tests/deprecated/<ordner>/` aus ihrem SOLL, wenn und nur wenn `tests/deprecated/<ordner>/VERMERK.md`
im Git-Index liegt UND die Form eines Ankers hat (regulaeres Blob 100644/100755 auf Index-Stufe 0,
lesbar, mit mindestens einem Nicht-Leerraum-Zeichen; NACHTRAG 2, 3 und 4 unten). Wird diese Datei aus
dem Index genommen (`git rm`, `git mv`; massgeblich ist
allein der INDEX -- ein blosses `rm` im Arbeitsbaum laesst den Index-Eintrag stehen und die Wache
lokal gruen, in CI ohne Wirkung, weil dort frisch ausgecheckt wird; Lens A LA-05), fallen die
vier `.cpp` sofort in den SOLL zurueck, und die Wache meldet sie als OHNE BEGRUENDUNG (rot). Die
Archiv-Menge steht bei jedem Wachen-Lauf sichtbar in der Ausgabe, im Nenner und in der Endzeile --
die Ablage ist damit nicht leiser als die Frist es war, nur ohne Ablaufdatum.

Beweis am Objekt (2026-09-17): `tests/unit/test_pa1_tote_ausnahme.cpp`, Faelle
`EchteAllowlistTraegtKeineToteZeile` (die vier hier, gegen das echte Repo) und
`ArchivOrdnerZaehltNurMitVermerkAnker` (Wegwerf-Repo: ohne Anker rot, Anker im NACHBARordner rot,
mit eigenem Anker gruen -- erst die Nachbar-Stufe macht aus dem "wenn" ein "wenn und nur wenn").

## NACHTRAG 2 (Lens-Funde r1, 2026-09-18)

Der Anker muss INHALT UND FORM haben: ein `VERMERK.md` mit 0 Byte, nur aus Leerraum, als Symlink
oder Gitlink, oder im Merge-Konflikt (Index-Stufe 1-3) ankert NICHT (Wache: UNPRUEFBARER ANKER, rot;
Faelle `ArchivAnkerOhneInhaltOderFormAnkertNicht` und `ArchivAnkerImMergeKonfliktAnkertNicht`). Eine
Allowlist-Zeile fuer eine der vier Dateien ist eine Ausnahme OHNE ANLASS (unpruefbar, rot; Fall
`ArchivOrdnerZaehltNurMitVermerkAnker`, Stufe d). Die drei Grenzen der Regel -- Datei direkt unter
`tests/deprecated/` bleibt im SOLL, ein Anker tiefer als das dritte Pfadsegment ankert nichts, ein
Anker nur im Arbeitsbaum ankert nichts -- tragen je einen eigenen Fall (`ArchivGrenze...`).

## NACHTRAG 3 (Fix-r2: Lens A r3, Lens B r2, Lens C r2; 2026-09-18)

Beleg am Objekt -- die Wache am Worktree-Stand nach Fix-r2, `sh scripts/ci_test_registrierungs_wache.sh
build-gcc-release` (sh = dash), Nenner und Endzeile literal (Lens C LCT-05-Rest, Mitnahme M5):

    544 getrackte Test-Quelldatei(en) im Baum (ohne ext/).
    davon 4 ARCHIV-Datei(en) in 1 Ordner(n) unter tests/deprecated/ mit VERMERK.md-Anker abgezogen -- SOLL: 540.
    TEST-REGISTRIERUNGS-WACHE: OK (540 Quelldateien, 0 ohne Begruendung ausserhalb, 4 archiviert).

Die 4 ARCHIV-Dateien sind die vier dieses Ordners (Tabelle oben), der 1 Ordner ist dieser; der SOLL der
Wache ist 540 = 544 - 4. Fall `EchteAllowlistTraegtKeineToteZeile` pinnt genau diese drei Zeilen mit den
selbst gemessenen Zahlen. Seit Fix-r2 ist eine Allowlist-Zeile fuer einen Pfad unter `tests/deprecated/`
auch OHNE Anker unpruefbar (rot): der Archiv-Ort kennt nur diesen Anker (Wache, Kopf Folge (3); Fall
`AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar`).

## NACHTRAG 4 (Fix-r3: Lens C r3 LC3W-01..08; 2026-09-18)

Die Anker-Form ist um das OBJEKT ergaenzt: der Index-Modus 100644/100755 verspricht ein Blob, prueft es
aber nicht (`git update-index --cacheinfo` legt jedes Objekt unter jedem Modus ab). Die Wache fragt jetzt
`git cat-file -e` (fehlt das Objekt, ist der Anker UNPRUEFBAR) und `git cat-file -t` == blob (ein Tree oder
Commit unter 100644 ankert nicht); scheitert git selbst, ist das Exit 2, kein Befund (Fall
`ArchivAnkerMussBlobInDerObjektdatenbankSein`). Werkzeug-Ausfaelle sind durchgehend Exit 2 (grep-Status 2,
git 128 in der Erreichbarkeits-Probe, date; cut, sed und tr sind aus der Wache entfernt), ISA-Belege
muessen eindeutig sein (ein leeres Teilmerkmal wie `+avx2` ist ein Formfehler), und die Form einer
Allowlist-Zeile gilt auch fuer Dateien im Bauweg (sechstes Nenner-Feld "mit Formfehler fuer Dateien im
Bauweg"). Fuer DIESEN Ordner aendert sich nichts: die Wache am Worktree-Stand nach Fix-r3 meldet weiterhin
die drei Zeilen aus NACHTRAG 3 (544 getrackt, 4 ARCHIV in 1 Ordner, SOLL 540, Endzeile OK 540/0/4); die
Nenner-Zeile "dazu UNPRUEFBAR ohne Bezug zum Bauweg" endet neu auf "0 mit Formfehler fuer Dateien im
Bauweg." Am Objekt gefunden: `git check-ignore` stirbt fuer Pfade unter einem Submodul-Gitlink mit 128
(kein Werkzeugfehler, eine Datenlage) -- die Wache prueft die Gitlink-Ahnenreihe deshalb zuerst (Fall
`SubmodulGitlinkIstErreichbar`).

## NACHTRAG 5 (Fix-r7: Lens A r7, Lens B r6, Lens C r5; 2026-09-19)

Die zwei Anker-Unterfaelle aus NACHTRAG 2 ("Gitlink" = Index-Modus 160000, und ein Commit-Objekt unter
Modus 100644) sind seit Fix-r7 direkte Google-Stufen (Fall `ArchivAnkerMussBlobInDerObjektdatenbankSein`,
Stufen (d) und (e)): `git update-index --cacheinfo` legt beide Formen an (Machbarkeitsprobe im Beweisort,
FIX-r7.md), die Wache meldet je UNPRUEFBARER ANKER mit Exit 1. Der Satz "oder Gitlink [...] ankert NICHT"
ist damit am Google-Test belegt, nicht nur an Shell-Proben. Neu in JEDER Ausgabe der Wache steht die
Nenner-Zeile "Quotierte Index-Pfade: N von git auch mit core.quotePath=false quotiert (Tabulator,
Steuerzeichen, Anfuehrungszeichen, Backslash), davon S im SOLL-Muster und A als VERMERK.md-Anker (beide
UNPRUEFBAR)." -- solche Pfade fielen bis 8ae59179 still aus dem SOLL, jetzt sind sie eine eigene
UNPRUEFBAR-Klasse (Exit 1). Fuer DIESEN Ordner: 0/0/0, die vier Dateien und der Anker tragen ASCII-Namen
ohne Anfuehrungszeichen, Backslash oder Tabulator; die drei Zeilen aus NACHTRAG 3 bleiben (544 getrackt,
4 ARCHIV in 1 Ordner, SOLL 540, Endzeile OK 540/0/4). Ebenfalls ohne Wirkung auf diesen Ordner: die
Ahnenreihe eines Allowlist-Gegenstands wird bis zur Wurzel geprueft (Rangfolge UNPRUEFBAR vor TOT vor
Gitlink), ein `..` zaehlt per Tiefenzaehler (verlaesst die Wurzel = Grenze (a), repo-intern aufgeloest =
UNPRUEFBAR), und die Existenzprobe der Allowlist-Datei nimmt auch einen Symlink an (`[ -e ] || [ -L ]`).

## NACHTRAG 6 (Fix-r8: Lens A r8, Lens B r7, Lens C r6; 2026-09-19)

BERICHTIGUNG ZU NACHTRAG 5 (Lens B r7 LB7-04 = Lens C r6 LC6T-04): der Satz "Die zwei Anker-Unterfaelle aus NACHTRAG 2
(Gitlink = Index-Modus 160000, und ein Commit-Objekt unter Modus 100644)" ordnet den Commit-Fall falsch zu. NACHTRAG 2
nennt "als Symlink oder Gitlink, oder im Merge-Konflikt" -- keinen Commit; der Commit unter 100644 steht erst in
NACHTRAG 4 ("ein Tree oder Commit unter 100644 ankert nicht"). Richtig lautet der Satz: die zwei Anker-Unterfaelle
aus NACHTRAG 2 (Gitlink = Index-Modus 160000) und NACHTRAG 4 (Commit-Objekt unter Modus 100644). Der Gitlink-Teil war
richtig. NACHTRAG 5 bleibt unveraendert stehen; dieser Nachtrag gilt.

BERICHTIGUNG ZU NACHTRAG 5, ZWEITER SATZ (Lens B r7 LB7-05 = Lens C r6 LC6T-05): "Neu in JEDER Ausgabe der Wache steht
die Nenner-Zeile" ist zu weit. Die Zeile "Quotierte Index-Pfade: ..." steht in jedem VOLLSTAENDIGEN Wachen-Bericht
(Exit 0 oder 1); Abbrueche mit Exit 2 (AUFRUF ohne Argument, kein Verzeichnis, Werkzeug-Ausfall, Signal) enden VOR dem
Nenner-Block und tragen sie nicht (Lens B r7, Proben B1a/B1b/B1c/B2: je rc=2, Quotierte-Zeile=0).

Die Stufen-Etiketten des Falls `ArchivAnkerMussBlobInDerObjektdatenbankSein` sind seit Fix-r8 eindeutig: (a) Tree,
(b) Fantasie-SHA, (c) echtes Blob, (d) Commit unter 100644, (e) Index-Modus 160000, (f) git-Koeder 'cat-file -t'
128 (Lens B r7 LB7-03; bis 89cf7103 hiessen Commit und git-Koeder beide "(d)"). Der Verweis "Stufen (d) und (e)" in
NACHTRAG 5 meint damit genau Commit und Modus 160000.

Fix-r8 der Wache ohne Wirkung auf diesen Ordner: (a) ein VERMERK.md auf Index-Stufe 0 gegen Eintraege DARUNTER
(`VERMERK.md/...` auf Stufe 1-3, D/F-Konflikt) ankert NICHT mehr (UNPRUEFBARER ANKER; Lens C r6 LC6W-01, Fall (27g4)
zeigt die Form am Ahnen); (b) jede Eingabe-Umleitung, der Berichtskanal stdout, der Abbruchkanal stderr und das Signal
PIPE sind im 0/1/2-Vertrag (Lens A r8 LA8-01..04; Google-Faelle (32) und (33)); (c) der Lese-Abgleich zaehlt auch
Bytes; (d) die OK-Zeile steht erst nach dem Aufraeumen des Zwischenverzeichnisses. Fuer DIESEN Ordner gemessen am
Fix-r8-Stand (`sh scripts/ci_test_registrierungs_wache.sh build-gcc-release`, sh = dash, ebenso bash --posix und
busybox sh): weiterhin 544 getrackt, 4 ARCHIV in 1 Ordner, SOLL 540, Quotierte 0/0/0, Endzeile OK 540/0/4; die Bilanz
ohne Heute-Zeile ist byte-gleich zu Fix-r7 (md5 09ddc181 gcc / f644b9fb clang).

## NACHTRAG 6a (Fix-r9: Lens B r8 LB8-05, Lens C r7 LC7T-10; 2026-09-20; additiv, nichts geloescht)

BERICHTIGUNG ZU NACHTRAG 1, ZEILEN 99-100 (Lens C r7 LC7T-10): "Die Archiv-Menge steht bei jedem Wachen-Lauf sichtbar
in der Ausgabe, im Nenner und in der Endzeile" ist zu weit -- dieselbe Einschraenkung wie in NACHTRAG 6 zur
Quotiert-Zeile gilt auch hier: die Archiv-Zeile "davon N ARCHIV-Datei(en) in M Ordner(n) unter tests/deprecated/ mit
VERMERK.md-Anker abgezogen -- SOLL: S." und die Endzeile stehen in jedem VOLLSTAENDIGEN Wachen-Bericht (Exit 0 oder 1);
Abbrueche mit Exit 2 (AUFRUF ohne Argument, kein oder nicht betretbares Verzeichnis, Werkzeug-Ausfall, Signal) enden
VOR dem Nenner-Block und tragen weder Archiv-Zeile noch Endzeile. Richtig lautet der Satz: die Archiv-Menge steht in
jedem vollstaendigen Wachen-Bericht sichtbar in der Ausgabe, im Nenner und in der Endzeile. NACHTRAG 1 bleibt stehen.

BERICHTIGUNG ZU NACHTRAG 6, ERSTER ABSATZ (Lens B r8 LB8-05): das dort wiedergegebene Zitat aus NACHTRAG 5 laesst die
Innen-Anfuehrungszeichen weg. NACHTRAG 5 lautet woertlich: "Die zwei Anker-Unterfaelle aus NACHTRAG 2 ("Gitlink" =
Index-Modus 160000, und ein Commit-Objekt unter Modus 100644)". Die Berichtigung von NACHTRAG 6 (Commit-Fall gehoert
zu NACHTRAG 4, Gitlink-Fall zu NACHTRAG 2) gilt unveraendert; nur das Zitat war ungenau.

ERGAENZUNG ZU NACHTRAG 6, LETZTER ABSATZ (Lens B r8 LB8-05): die Fix-r8-Klassen (a) und (c) sind seit Fixer r8b auch
als Google-Stufen gepinnt -- (a) ARCHIV-Anker auf Stufe 0 gegen einen Eintrag DARUNTER = UNPRUEFBARER ANKER: Fall (34a)
`AnkerDFTeilrestGrepZielModifyDeleteGitlinkLinkSindUnpruefbarOderExit2` (eigener Wegwerf-Fall mit Waise im
Archiv-Ordner, wie Fall (12d)); (c) Byte-Abgleich der Allowlist: Stufe (34b) derselben Fall-Funktion (wc -c um 1 Byte
zu hoch = Exit 2), seit Fix-r9 dazu (34b2) (zaehlender wc-Koeder, nur der zweite Aufruf +1) und (34b3) (echtes
NUL-Byte in der Datenzeile = Exit 2).

Fix-r9 der Wache ohne Wirkung auf diesen Ordner: (a) jeder open-Fehler einer Eingabe-Umleitung und des grep-Ziels ist
ABBRUCH + Exit 2 (Lens C r7 LC7W-03/04); (b) ein transienter Lesefehler mit Teilrest ist Exit 2, kein Dateiende
(LC7W-02); (c) der IST-Abgleich hat eine rechte Pfadgrenze -- `test_x.cpp.extra.cpp` deckt `test_x.cpp` nicht mehr
(LC7W-10); (d) ein nicht betretbares Bau-Verzeichnis traegt eine ABBRUCH-Zeile (Lens A r9 LA9-03); (e) die Index-
Konfliktarten am Ahnen heissen nach ihrer git-Klasse, add/add (Datei auf Stufe 2 UND 3) ist TOT (LA9-01); (f) das
Zwischenverzeichnis entsteht per `mktemp -u -d` + `mkdir -m 700`, der Pfad steht vor der Erzeugung fest (LC7W-09).
Fuer DIESEN Ordner gemessen am Fix-r9-Stand (`sh scripts/ci_test_registrierungs_wache.sh build-gcc-release` und
build-clang-release, sh = dash, ebenso bash --posix und busybox sh): weiterhin 544 getrackt, 4 ARCHIV in 1 Ordner,
SOLL 540, Quotierte 0/0/0, Endzeile OK 540/0/4; die Bilanz ohne Heute-Zeile ist byte-gleich zu Fix-r7 und Fix-r8
(md5 09ddc181 gcc / f644b9fb clang; messungen/fix-r9c/bilanz/tafel-r9c-v2.out).
