# tools/abi_stability_test — ABI-Stabilitaets-Test (F-EXTRA-8)

Pro Permutation, im CI-Lauf:
1. Bit-Identitaets-Pruefung (Original-Build vs. Splitting-Build)
2. Funktionale Aequivalenz (gleicher Input → gleicher Output)
3. Strukturelle Pruefung (`nm` / `objdump` Symbol-/Section-/Relocation-Vergleich)

Bei Bit-Diff: **HABICH-LOG** erforderlich (siehe habich_log_generator.py).

## Status: Skelett (Phase 4.B)

## Komponenten

- `bit_diff.py` — diff der .o/.so zwischen Original-Build und Splitting-Build
- `struct_diff.py` — `nm`/`objdump`-Vergleich
- `func_diff.py` — funktionale Aequivalenz-Tests
- `habich_log_generator.py` — generiert Log-Datei bei Abweichung

## Ausgabe-Format Habich-Log

```
HABICH-LOG fuer P<XX> <Algo>
=============================
Datum: 2026-XX-XX
Baustein: <bausteine-id>
Original-Compiler: <version>
Aktueller Compiler: <version>

Bit-Diff:
  - Datei: <path>
  - Erwartet: <hash original>
  - Aktuell: <hash splitting>

Source-Code-Identitaet: VERIFIZIERT (siehe diff)

Funktionaler Aequivalenz-Beweis:
  - Tests: <n_tests> bestanden
  - Latenz im Toleranzband (+/-5%)

Hypothese fuer Diff-Ursache:
  <text>

Habich-Entscheidung erforderlich: AKZEPTIEREN | RE-IMPLEMENTATION
```
