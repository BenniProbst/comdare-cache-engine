# Session-Abschluss 2026-06-19 — Strang A + FF + Erweiterungs-Leitfaden + Build-Modell-Korrektur

> Alle User-Direktiven dieser Strecke geliefert + adversarial/G5-verifiziert + 3-Repo-committet. Single-Source-Kette:
> `20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md` · `BUILD-MODELL-1DLL-1TU-KLARSTELLUNG.md` · `ERWEITERUNGS-LEITFADEN.md`.

## Geliefert (jede adversarial verifiziert, committet)
1. **Strang A — profil-getriebene Mess-Selektion** (Inc 1–6, `bc1f7a3..1e381fd`): die Code-Selektion (lazy_pilot_engine.hpp +
   m3v2_select_profile.hpp) GELÖSCHT; die Selektion kommt AUSSCHLIESSLICH aus dem Diplomarbeit-Profil (`comdare_thesis_profile`-XML)
   via `CEB::run_profile`. EIN Profil fährt Basis-320 + SOTA-Reihen A/B/C in EINE CSV. **G5-Audit `wl3i01xn4`: `strang_a_delivered=True`,
   `reclassification_found=False`** (unabhängig grep/ls/git + 4 reale MSVC-Builds, Golden Mismatch=0/320).
2. **Build-Modell-Korrektur** (`dd16eb3`, User 2026-06-19, code-belegt `wuh70wwyi`): `1 DLL = 1 TU` je Konfiguration, IMMER frisch
   gebaut; KEINE inkrementelle „nur-die-neue-Achse"-Bau-Semantik; einzige Ersparnis = Resume. „Ohne alles andere neu zu bauen" gilt
   NUR für die QUELLE. Memory `reference_thesis_core_contribution_axis_library` korrigiert.
3. **FF / 9-Achsen-Austauschbarkeit** (#168/#162, `e823a2b`): die 4 vertieften Achsen (migration/filter/value_handle/path_compression)
   sweep-fähig im Katalog (per-Achse kleine Sweep-Source-Map + Union, kein C1060); `axis_sweep:migration` baut 2 reale distinkte DLLs;
   SOTA + PRT-ART real. Golden stabil.
4. **#169 — nutzbar machen** (`905d947`): `docs/ERWEITERUNGS-LEITFADEN.md` (file:line-Anpassungs-Stellen für 3 Szenarien: neuer Algo /
   neue Achse / neues Profil-Feld; auf korrektem Build-Modell) + **`--validate`-Flag** (`run_lazy_150 --validate <profil>`: rein-lesend
   gegen die AxisRegistry/EnabledStrategies, fängt getippte Werte VOR dem teuren Bau mit klarer Meldung; test_validate_profile grün).

## Verbleibend
- **#155 (gate-frei, Build-System-Politur):** (a) ~15 neue thesis_tiere-Tests (test_profile_roundtrip / _run_profile_union /
  _sota_series_pilot / _axis_sweep_pilot / _validate_profile / _seg_coverage / _migration_two_tier …) sind standalone-PS-verifiziert,
  aber NICHT in CMake/ctest registriert. (b) **VORBESTEHENDER CMake-Bug** (diagnostiziert): `comdare_add_test` nutzt
  `gtest_discover_tests(DISCOVERY_MODE PRE_TEST)` (cmake/gtest_setup.cmake:60); das erzeugt einen `_include.cmake`-Dateinamen mit
  `[1]`-Klammer (z.B. `test_pressure_state[1]_include.cmake`), die `CTestTestfile.cmake`-`include()` als Glob-Char-Class fehlinterpretiert
  → ctest-Enumeration bricht. Orthogonal zu allem oben. **Braucht einen Reconfigure-Test-Zyklus** (kein blinder Fix; Kandidaten:
  DISCOVERY_MODE-Wechsel ODER Namens-Sanitisierung ODER CMake-Policy/Version). Am besten in frischem Kontext.
- **HELD (#156, getrennt, NICHT gate-frei):** die echte Voll-Messung (320 + 21 SOTA × Two-Phasen × Working-Set × ≥2 Plattformen) +
  voller 7-Host-SOTA-Fanout = Linux+PMC/Infra-Agent (#163/#165). Der profil-getriebene Apparat ist verdrahtet + baubar; nur der teure
  Lauf fehlt.
- **needs_user (nominal):** K1 (RC-Dimension), A5 (Second-Execution).

## Empfohlene Folge-Usability (vom Leitfaden benannt, optional)
generierte „gültige-Werte-je-Achse"-Liste für Profil-Autoren · `.sh`-Mess-Harness für Linux/ZIH · cacheline-Sub-Werte in `--validate`.
