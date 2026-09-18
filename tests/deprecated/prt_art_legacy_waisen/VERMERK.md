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
im Git-Index liegt. Wird diese Datei aus dem Index genommen (`git rm`, `git mv`; massgeblich ist
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

Der Anker muss INHALT UND FORM haben: ein `VERMERK.md` mit 0 Byte oder als Symlink ankert NICHT
(Wache: UNPRUEFBARER ANKER, rot; Fall `ArchivAnkerOhneInhaltOderFormAnkertNicht`). Eine
Allowlist-Zeile fuer eine der vier Dateien ist eine Ausnahme OHNE ANLASS (unpruefbar, rot; Fall
`ArchivOrdnerZaehltNurMitVermerkAnker`, Stufe d). Die drei Grenzen der Regel -- Datei direkt unter
`tests/deprecated/` bleibt im SOLL, ein Anker tiefer als das dritte Pfadsegment ankert nichts, ein
Anker nur im Arbeitsbaum ankert nichts -- tragen je einen eigenen Fall (`ArchivGrenze...`).
