# `docs/archiv/messdaten/` — archivierte Mess-Laeufe

**Angelegt 08.08.2026.** Hier liegen Mess-Laeufe, die **nicht mehr Grundlage aktueller Auswertungen**
sind, aber unter der Doktrin *„Messdaten werden nie geloescht — auch wenn dadurch das ABI bricht"*
erhalten bleiben.

**Archiviert heisst: aus dem aktiven Weg genommen, nicht entfernt.** Kein Byte wird veraendert. Wer eine
Datei hier oeffnet, liest genau das, was der Lauf damals geschrieben hat.

---

## `20260606-fullpilot-320/tier150_measurements.csv`

| | |
|---|---|
| **Lauf** | FullPilot 320/320, `major3-320-v1` |
| **Herkunft** | ce-Commit `2dbbdb1a` vom 06.06.2026, *„vollstaendiger FullPilot 320/320 Mess-Lauf"* |
| **Umfang** | 5761 Zeilen (5760 Messzeilen + Kopf), 6.748.937 Bytes, `;`-getrennt |
| **Vorher** | `build/thesis_tiere/tier150_measurements.csv` |
| **Verschoben am** | 08.08.2026, mit `git mv` — Historie erhalten |

### Warum diese Kopie umgezogen ist

Sie lag unter `build/` — einem Verzeichnis, das `.gitignore:2` ausschliesst. Sie war also
**force-added** und war die **einzige getrackte Datei unter `build/` ueberhaupt** (gemessen:
`git ls-files build/ | wc -l` -> 1 vorher, 0 nachher).

Damit war sie eine Falle: **jedes `rm -rf build` loeschte getrackte Messdaten.** Das ist im laufenden
Betrieb bereits einmal passiert und wurde nur von der Vor-Push-Wache gefangen. Der Umzug beseitigt die
Falle, ohne ein Byte anzufassen.

### Sie war ein reines Duplikat

Byte-identisch mit `tests/unit/thesis_tiere/tier150_measurements.csv` (geprueft mit `cmp`, beide
6.748.937 Bytes, beide aus demselben Commit `2dbbdb1a`). **Gelesen wurde sie von niemandem** — gesucht
wurde nach `build/thesis_tiere` in `*.cpp *.hpp *.txt *.cmake *.yml *.sh *.md`; alle Treffer betreffen
**andere** Dateien in diesem Verzeichnis (`obs_phaseA_pilot.csv`, `all19_pilot.csv`,
`m3v2_sota_pilot_measurements.csv`) — allesamt **Ausgabe-Ziele** von Tests, nicht diese CSV.

**Geloescht wurde sie trotzdem nicht.** Ein byte-identisches Duplikat zu entfernen waere kein
Datenverlust, aber die Doktrin ist absolut formuliert und *Loeschung = Owner-GO*. Ob das Duplikat ganz
entfallen darf, ist deshalb ein **offener Owner-Entscheid**, kein Aufraeum-Detail.

### Die aktive Kopie bleibt, wo sie ist — und muss es

`tests/unit/thesis_tiere/tier150_measurements.csv` ist **nicht** archiviert worden, weil zwei Dinge
daran haengen:

| Wer | Wo | Was |
|---|---|---|
| CMake | `tests/unit/CMakeLists.txt:3304` | `COMDARE_ORG18_TIER150_CSV` zeigt mit absolutem Pfad darauf |
| ein Test | `tests/unit/test_org18_persistence_target.cpp:157-165` | `Org18AltIds.AltMessdatenDateiIstByteUnveraendert` pinnt die Datei auf **6.748.937 Bytes** |

Der Test ist ausdruecklich eine **Wache gegen jede kuenftige Anpassung der Datei** — er faengt genau
das, was die Doktrin verbietet. **Ihn zu umgehen, um die Datei zu verschieben, waere ein Rueckschritt.**

### Der Alters-Vermerk

Owner am 08.08.2026: *„tier150_measurements.csv Bitte archivieren, ist veraltet. Wir messen den
Gesamtstrang neu."*

**Der Lauf ist vom 06.06.2026 und damit inhaltlich ueberholt.** Er darf **nicht** Grundlage neuer
Auswertungen sein. Als *Format*-Referenz bleibt er gueltig und wird als solche auch benannt:
`libs/cache_engine/heuristik/measurement_curve_loader.hpp:19` und
`libs/cache_engine/builder/experiment_tree/result_ingest.hpp:33` nennen ihn, und der
Builder-Iterator haelt an mehreren Stellen ausdruecklich die Rueckwaertskompatibilitaet zu
`cowfix-v1/tier150`-Lesern (`cache_engine_builder_iterator.hpp:413,520,527,829`).

**Daraus folgt die Trennung:** als *Datenquelle* ueberholt, als *Format-Zeuge* und
*Kompatibilitaets-Anker* weiterhin in Gebrauch. Wer die Datei fuer eine Auswertung heranzieht, misst
den Stand vom Juni.
