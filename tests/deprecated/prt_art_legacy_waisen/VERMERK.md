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

## Frist und Reaktivierungs-Weg

Die `frist:2026-09-15`-Zeilen der vier Dateien bleiben in
`scripts/ci_test_registrierungs_allowlist.txt` (SOLL ist am DATEINAMEN verankert, Pfad in Feld 1
nachgezogen; `tests/unit/test_pa1_tote_ausnahme.cpp` kGeparkt ebenso). Die Frist gehoert dem Owner
und deckt jetzt die ENDGUELTIGE Form: Archiv bestaetigen ODER Loeschung mit eigenem Owner-GO.
Eine Neu-Deckung der drei Luecken laeuft ueber neue Tests gegen die heutige Slot-Welt bzw. eine
neue Faehigkeit im ce-Kontrakt (`COMDARE_PRUEFLING_BEKANNTE_FAEHIGKEITEN`), nicht ueber Reanimation
dieser Dateien.
